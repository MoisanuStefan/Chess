/* servTCPCSel.c - Exemplu de server TCP concurent

   Asteapta un "nume" de la clienti multipli si intoarce clientilor sirul
   "Hello nume" corespunzator; multiplexarea intrarilor se realizeaza cu select().

   Cod sursa preluat din [Retele de Calculatoare,S.Buraga & G.Ciobanu, 2003] si modificat de
   Lenuta Alboaie  <adria@infoiasi.ro> (c)2009

*/
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdbool.h>

/* portul folosit */

#define PORT 2726

extern int errno;		/* eroarea returnata de unele apeluri */

/* functie de convertire a adresei IP a clientului in sir de caractere */
char * conv_addr (struct sockaddr_in address)
{
    static char str[25];
    char port[7];

    /* adresa IP a clientului */
    strcpy (str, inet_ntoa (address.sin_addr));
    /* portul utilizat de client */
    bzero (port, sizeof(port));
    sprintf (port, ":%d", ntohs (address.sin_port));
    strcat (str, port);
    return (str);
}


void SendFd(int socket, int fd)  // send fd by socket
{
    struct msghdr msg = { 0 };
    char buf[CMSG_SPACE(sizeof(fd))];
    memset(buf, '\0', sizeof(buf));
    struct iovec io = { .iov_base = "ABC", .iov_len = 3 };

    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);

    struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));

    *((int *) CMSG_DATA(cmsg)) = fd;

    msg.msg_controllen = CMSG_SPACE(sizeof(fd));

    if (sendmsg(socket, &msg, 0) < 0)
        perror("Failed to send message\n");
}


int ReceiveFd(int socket)  // receive fd from socket
{
    struct msghdr msg = {0};

    char m_buffer[256];
    struct iovec io = { .iov_base = m_buffer, .iov_len = sizeof(m_buffer) };
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;

    char c_buffer[256];
    msg.msg_control = c_buffer;
    msg.msg_controllen = sizeof(c_buffer);

    if (recvmsg(socket, &msg, 0) < 0)
        perror("Failed to receive message\n");

    struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);

    unsigned char * data = CMSG_DATA(cmsg);

    int fd = *((int*) data);

    return fd;
}

void InitializeBoard(char b[10][10]){
    int i,j;
    for (i = 0;i <= 9; ++i)     // borders
    {
        b[0][i] = '\0';
        b[9][i] = '\0';
        b[i][0] = '\0';
        b[i][9] = '\0';
    }

    for (i=2; i<8; ++i)
        for(j=1;j<9;++j)
        {
            if (i == 2)
                b[i][j] = 'P';  //PION
            else if (i == 7)
                b[i][j] = 'p';  //pion
            else
                b[i][j] = 'z';  // empty slot
        }
    b[1][1] = b[1][8] = 'R';    //ROOK
    b[8][1] = b[8][8] = 'r';
    b[1][2] = b[1][7] = 'H';    //HORSE( KNIGHT, k taken by king)
    b[8][2] = b[8][7] = 'h';
    b[1][3] = b[1][6] = 'B';    //BISHOP
    b[8][3] = b[8][6] = 'b';
    b[1][4] = 'Q';              //QUEEN
    b[8][4] = 'q';
    b[1][5] = 'K';              //KING
    b[8][5] = 'k';

}

void PrintBoard(char b[10][10]){
    int i,j;
    for (i=0; i<10; ++i) {
        for (j = 0; j < 10; ++j)
            printf("%c", b[i][j]);
        printf("\n");
    }
}

bool CheckLine (char board [10][10], int i, int j0, int j){
    int c;
    for (c = j0 + 1; c < j; ++c)
        if (board[i][c] != 'z')
            return true;
    for (c = j0 - 1; c > j; c--)
        if (board[i][c] != 'z')
            return true;
    return false;

}
bool CheckColumn (char board [10][10], int j, int i0, int i) {
    int c;
    for (c = i0 + 1; c < i; ++c)
        if (board[c][j] != 'z')
            return true;
    for (c = i0 - 1; c > i; c--)
        if (board[c][j] != 'z')
            return true;
    return false;
}

bool CheckDiag2 (char board [10][10], int i0, int j0, int i){

    int c, c1;
    for( c = i0 - 1, c1=j0 + 1; c>i; c--, c1++)
        if (board[c][c1] != 'z')
            return true;
    for( c = i0 + 1, c1=j0 - 1; c<i; c++, c1--)
        if (board[c][c1] != 'z')
            return true;
    return false;
}

bool CheckDiag1 (char board [10][10], int i0, int j0, int i){

    int c, c1;
    for( c = i0 + 1, c1=j0 + 1; c<i; c++, c1++)
        if (board[c][c1] != 'z')
            return true;
    for( c = i0 - 1, c1=j0 - 1; c>i; c--, c1--)
        if (board[c][c1] != 'z')
            return true;
    return false;

}

bool HasPieceBetween(char board [10][10], int i0, int j0, int i, int j){

    int c, c1;
    if (i0 == i) {                      // check on same line

        if (CheckLine(board, i, j0, j))
            return true;
    }

    else if (j0 == j) {                 // check on same column

        if ( CheckColumn(board, j, i0, i) )
            return true;
    }

    else if (i0 + j0 == i + j ){

        if ( CheckDiag2(board, i0, j0, i) )
            return true;
    }
    else if (i0 - j0 == i - j)

        if ( CheckDiag1(board, i0, j0, i))
            return true;
    return false;
}

bool IsValidMove(char board[10][10], int i0, int j0, int i, int j) {
    if (board[i0][j0] >'a' && board[i0][j0] <'z' && board[i][j] < 'z' )       //slot occupied by same player
        return false;
    else if (board[i0][j0] > 'A' && board[i0][j0] < 'Z' && board[i][j] > 'A' && board[i][j] < 'Z')
        return false;
    if (i > 8 || j > 8 || i < 1 || j < 1)
        return false;
    else if (board[i0][j0] == 'p' && ((i == i0 - 1) && (j == j0 || ((j==j0-1 || j==j0+1) && board[i][j] >'A' && board[i][j] < 'Z')))) // PION
        return true;
    else if (board[i0][j0] == 'P' && ((i == i0 + 1) && (j == j0 || ((j==j0-1 || j==j0+1) && board[i][j] >'a' && board[i][j] < 'z'))))     // pion
        return true;
    else if (((board[i0][j0] == 'r' || board[i0][j0] == 'R') && (i == i0 || j == j0)))           // rook
        return true;
    else if ((board[i0][j0] == 'h' || board[i0][j0] == 'H') && (((i == i0 + 2 || i == i0 - 2) && (j == j0 - 1 || j == j0 + 1)) ||
                                                                ((i == i0 + 1 || i == i0 - 1) &&
                                                                 (j == j0 - 2 || j == j0 + 2))))      //knight
        return true;
    else if ((board[i0][j0] == 'B' || board[i0][j0] == 'b') && ((i + j == i0 + j0) || (i - j == i0 - j0)))     //bishop
        return true;
    else if ((board[i0][j0] == 'Q' || board[i0][j0] == 'q') && ((i + j == i0 + j0) || (i - j == i0 - j0) || i == i0 || j == j0))      //queen
        return true;
    else if ((board[i0][j0] == 'K' || board[i0][j0] == 'k') && (i != i0 || j != j0) && i >= i0 - 1 && i <= i0 + 1 && j >= j0 - 1)     //king
        return true;
    return false;
}

bool isCheck(char board[10][10], int i, int j){


}
/* programul */

int main ()
{
    struct sockaddr_in server;	/* structurile pentru server si clienti */
    struct sockaddr_in from;
    fd_set readfds;		/* multimea descriptorilor de citire */
    fd_set actfds;		/* multimea descriptorilor activi */
    struct timeval tv;		/* structura de timp pentru select() */
    int sd, client, max_fd, fds[2], UNIXsocket[2];		/* descriptori de socket */
    int optval=1; 			/* optiune folosita pentru setsockopt()*/
    int fd, clientCounter = 0, nrOfChilds = 0, txt_fd;			// descriptor folosit pentru
    pid_t pid = 1;			   //parcurgerea listelor de descriptori
    int nfds;			/* numarul maxim de descriptori */
    int len, msglen;			/* lungimea structurii sockaddr_in */
    char board[10][10], player_name[2][20];


    InitializeBoard(board);

    if(-1 == mkfifo("my_fifo", 0600) ) {
        if (errno == EEXIST) {
            printf("Using already existent fifo: \"my_fifo\" ...\n");
        } else {
            perror("Error at creating \"my_fifo\" file.\n");
            exit(1);
        }

    }
    /* creare socket */
    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror ("[server] Eroare la socket().\n");
        return errno;
    }

    /*setam pentru socket optiunea SO_REUSEADDR */
    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,&optval,sizeof(optval));

    /* pregatim structurile de date */
    bzero (&server, sizeof (server));

    /* umplem structura folosita de server */
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl (INADDR_ANY);
    server.sin_port = htons (PORT);

    /* atasam socketul */
    if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
        perror ("[server] Eroare la bind().\n");
        return errno;
    }

    /* punem serverul sa asculte daca vin clienti sa se conecteze */
    if (listen (sd, 5) == -1)
    {
        perror ("[server] Eroare la listen().\n");
        return errno;
    }



    /* servim in mod concurent clientii... */
    while (1)
    {
        printf ("[server] Asteptam la portul %d...\n", PORT);
        fflush (stdout);

        /* pregatirea structurii client */
        len = sizeof(from);
        bzero(&from, sizeof(from));

        /* a venit un client, acceptam conexiunea */
        client = accept(sd, (struct sockaddr *) &from, &len);
        /* eroare la acceptarea conexiunii de la un client */
        if (client < 0) {
            perror("[server] Eroare la accept().\n");
            continue;
        }

        printf("[server] S-a conectat clientul cu descriptorul %d, de la adresa %s.\n", client, conv_addr(from));
        fflush(stdout);
        clientCounter++;

        if (clientCounter % 2 == 1)       // if it's a player waiting for opponent
        {

            int player = 1;
            if (-1 == write(client, &player, sizeof(int))) {        // letting the client know he's player 1, making first move
                perror("Error at writing to player1");
                return errno;
            }

            if (socketpair(AF_UNIX, SOCK_DGRAM, 0, UNIXsocket )!= 0) {
                perror("Failed to create Unix-domain socket pair\n");
                return errno;
            }

            if (-1 == (pid = fork())) {
                perror("Error at fork.\n");
                return errno;
            }
            if (pid == 0) {     // [child that handles a chess match]
                int player1_fd = client, player2_fd, nameLen;
                bool bothConnected = false;
                int infoFromPipe;
                char msgrasp[20]=" ";

                bzero(player_name, sizeof(player_name));
                player2_fd = ReceiveFd(UNIXsocket[0]);       // waiting for second player connection

                max_fd = (player1_fd > player2_fd) ? player1_fd : player2_fd;

                /* completam multimea de descriptori de citire */
                FD_ZERO (&actfds);		/* initial, multimea este vida */
                FD_SET (player1_fd, &actfds);		/* includem in multime socketul creat */
                FD_SET (player2_fd, &actfds);

                /* valoarea maxima a descriptorilor folositi */

                fds[0] = player1_fd;
                fds[1] = player2_fd;
                printf("%d %d",fds[0], fds[1] );

                //printf("pipe: %d %d", p[0],p[1]);
                fflush(stdout);
                while(1) {
                    //PrintBoard(board);
                    /* ajustam multimea descriptorilor activi (efectiv utilizati) */
                    bcopy((char *) &actfds, (char *) &readfds, sizeof(readfds));


                    /* apelul select() */
                    if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
                        perror("[server] Eroare la select().\n");
                        return errno;
                    }



                    for (fd = 0; fd <= 1; ++fd)    /* parcurgem multimea de descriptori */
                    {
                        /* este un socket de citire pregatit? */
                        if (FD_ISSET (fds[fd], &readfds)) {
                            printf("in isset if with fd = %d\n", fd);
                            char nameOrMove;
                            if (-1 == read(fds[fd], &nameOrMove, sizeof(char)))     // reading the name or move char
                            {
                                perror("Error at reading from pipe\n");
                                return errno;
                            }
                            printf("nameormove: %c\n", nameOrMove);
                            if (nameOrMove == '0') {       // name
                                int bytesRead;

                                if (read (fds[fd], &nameLen, sizeof(int)) <= 0) {
                                    perror("[client]Eroare la write() spre server.\n");
                                    return errno;
                                }

                                bytesRead = read(fds[fd], player_name[fd], nameLen);
                                if (bytesRead < 0) {
                                    perror("Eroare la read() de la client.\n");
                                    return errno;
                                }

                                printf("got name: %s", player_name[fd]);
                                fflush(stdout);

                                if (-1 == write(fds[1 - fd], &nameLen, sizeof(int))) {
                                    perror("Error at writing to player");
                                    return errno;
                                }
                                if (-1 == write(fds[1 - fd], player_name[fd], nameLen)) {
                                    perror("Error at writing to player");
                                    return errno;
                                }
                                printf("[server] Name sent to player %d\n", 1 - fd + 1);
                                fflush(stdout);



                            } else {       //move

                                int curr_i, curr_j, dest_i, dest_j;
                                if (-1 == read(fds[fd], &curr_i, sizeof(int))) {
                                    perror("Error at reading from pipe.\n");
                                    return errno;
                                }
                                if (-1 == read(fds[fd], &curr_j, sizeof(int))) {
                                    perror("Error at reading from pipe.\n");
                                    return errno;
                                }
                                if (-1 == read(fds[fd], &dest_i, sizeof(int))) {
                                    perror("Error at reading from pipe.\n");
                                    return errno;
                                }
                                if (-1 == read(fds[fd], &dest_j, sizeof(int))) {
                                    perror("Error at reading from pipe.\n");
                                    return errno;
                                }


                                bzero(msgrasp, sizeof(msgrasp));
                                sprintf(msgrasp, "%d %d %d %d", curr_i, curr_j, dest_i, dest_j);

                                printf("[server]Trimitem mesajul inapoi...%s\n", msgrasp);
                                fflush(stdout);

                                if (-1 == write(fds[1 - fd], msgrasp, 7)) {
                                    perror("Error at writing to player1");
                                    return errno;
                                }
                                printf("[server] Move sent to player %d\n", fd + 1);
                                fflush(stdout);


                            }


                        }

                    }


                }
            }
            nrOfChilds++;
        } else {     // if it's the second player to be assigned to a waiting player 1
            if (pid > 0) {
                /* All messages from parent to it's children processes will start with a char
                 * that shows if it's information coming from an already connected player or the fd of a new connection:
                 * '0' - old client
                 * '1' - new client
                 *      If the first char = '0', it will be followed by a char specifying from which player is the info coming from:
                 *      '1' - player1
                 *      '2' - player2
                 *             The third char tells me if I'm getting the name of the player or a move:
                 *             '0' - name
                 *             '1' - move
                 */
                int player = 2;
                if (-1 == write(client, &player, sizeof(int))) {        // letting the client know he's player 2, waiting for player 1 move
                    perror("Error at writing to player1");
                    return errno;
                }

                SendFd(UNIXsocket[1] , client);





            }
        }

    }/* while */
}
/* main */


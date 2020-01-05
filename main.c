
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
#include <wait.h>
#include <sqlite3.h>

#define PORT 2726

struct leaderboard{
    char username[100];
    int score;
}myLB[100];

char * conv_addr (struct sockaddr_in address)
{
    static char str[25];
    char port[7];

    strcpy (str, inet_ntoa (address.sin_addr));
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

void UpdateBoard(char b[10][10], int i0, int j0, int i, int j){

    b[i][j] = b[i0][j0];
    b[i0][j0] = 'z';
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

}                                                         // following functions check if a piece exists on the path from initial pos to destination
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


    if (i0 == i) {                      // check on same line

        if (CheckLine(board, i, j0, j))
            return true;
    }

    else if (j0 == j) {                 // check on same column

        if ( CheckColumn(board, j, i0, i) )
            return true;
    }

    else if (i0 + j0 == i + j ){        // check on diag2

        if ( CheckDiag2(board, i0, j0, i) )
            return true;
    }
    else if (i0 - j0 == i - j)          //check on diag1

        if ( CheckDiag1(board, i0, j0, i))
            return true;
    return false;
}

bool IsValidMove(char board[10][10], int i0, int j0, int i, int j) {
    if (board[i0][j0] >'a' && board[i0][j0] <'z' && board[i][j] < 'z' )       //slot occupied by same player
        return false;
    else if (board[i0][j0] > 'A' && board[i0][j0] < 'Z' && board[i][j] > 'A' && board[i][j] < 'Z')      // slot occupied by same player
        return false;
    if (i > 8 || j > 8 || i < 1 || j < 1)       // out of board
        return false;
    else if (board[i0][j0] == 'p' && ((i == i0 - 1) && ((j==j0 && board[i][j] == 'z') || ((j==j0-1 || j==j0+1) && board[i][j] >'A' && board[i][j] < 'Z')))) // PION
        return true;
    else if (board[i0][j0] == 'P' && ((i == i0 + 1) && ((j==j0 && board[i][j] == 'z') || ((j==j0-1 || j==j0+1) && board[i][j] >'a' && board[i][j] < 'z'))))     // pion
        return true;
    else if (((board[i0][j0] == 'r' || board[i0][j0] == 'R') && (i == i0 || j == j0)) && !HasPieceBetween(board, i0, j0, i, j))        // rook
        return true;
    else if ((board[i0][j0] == 'h' || board[i0][j0] == 'H') && (((i == i0 + 2 || i == i0 - 2) && (j == j0 - 1 || j == j0 + 1)) ||
                                                                ((i == i0 + 1 || i == i0 - 1) &&
                                                                 (j == j0 - 2 || j == j0 + 2))))      //knight
        return true;
    else if ((board[i0][j0] == 'B' || board[i0][j0] == 'b') && ((i + j == i0 + j0) || (i - j == i0 - j0)) && !HasPieceBetween(board, i0,j0,i,j))     //bishop
        return true;
    else if ((board[i0][j0] == 'Q' || board[i0][j0] == 'q') && ((i + j == i0 + j0) || (i - j == i0 - j0) || i == i0 || j == j0) && !HasPieceBetween(board, i0,j0,i,j))      //queen
        return true;
    else if ((board[i0][j0] == 'K' || board[i0][j0] == 'k') && (i != i0 || j != j0) && i >= i0 - 1 && i <= i0 + 1 && j >= j0 - 1)     //king
        return true;
    return false;
}

int CodeMove ( int a, int b, int c, int d)
{
    return a*1000 + b*100 + c*10 + d;
}                                                                        // creates 4-digit int


void DecodeMove (int move, int *i0, int *j0, int *i, int *j)
{
    *j = move % 10;
    move /= 10;
    *i = move % 10;
    move /= 10;
    *j0 = move % 10;
    move /= 10;
    *i0 = move;

}                                                      // gets 4 ints from 4-digit int

int Intread(int fd, int *readDest)
{
    int bytesRead;
    bytesRead = read (fd, readDest, sizeof(int));
    if (bytesRead < 0){
        perror("Error at reading.\n");
        exit(2);
    }
    return bytesRead;
}                                                                                // read wrapper

int Charread(int fd, char *readDest, int len)
{
    int bytesRead;
    bytesRead = read (fd, readDest, len);
    if (bytesRead < 0){
        perror("Error at reading.\n");
        exit(2);
    }
    return bytesRead;
}

void Intwrite (int fd, int *writeDest){
    if (-1 == write(fd, writeDest, sizeof(int))) {
        perror("Error at writing\n");
        exit(2);
    }
}                                                                             // write wrapper

void Charwrite (int fd, char *writeDest, int len){
    if (-1 == write(fd, writeDest, len)) {
        perror("Error at writing\n");
        exit(2);
    }
}

int main ()
{
    struct sockaddr_in server;
    struct sockaddr_in from;
    fd_set readfds;
    fd_set actfds;
    int sd, client, max_fd, fds[2], UNIXsocket[2], optval=1, fd, clientCounter = 0, len, errCode = 0;
    pid_t pid = 1;
    char board[10][10], player_name[2][20], sql[200], *error;

    sqlite3_stmt *res;
    sqlite3* db;

    InitializeBoard(board);                                                                                             // set pions on matrix for new game

    if (sqlite3_open("/home/stef/Leaderboard.db", &db))
        perror("Error at opening database.\n");

    if ((sd = socket (AF_INET, SOCK_STREAM, 0)) == -1)
    {
        perror ("Error at socket().\n");
        exit(3);
    }

    setsockopt(sd, SOL_SOCKET, SO_REUSEADDR,&optval,sizeof(optval));

    bzero (&server, sizeof (server));

    server.sin_family = AF_INET;
    server.sin_addr.s_addr = htonl (INADDR_ANY);
    server.sin_port = htons (PORT);

    if (bind (sd, (struct sockaddr *) &server, sizeof (struct sockaddr)) == -1)
    {
        perror ("Error at bind().\n");
        exit(3);
    }

    if (listen (sd, 5) == -1)
    {
        perror ("Error at listen().\n");
        exit(3);
    }

    printf ("Waiting at port nr: %d...\n", PORT);
    fflush (stdout);
    while (1)
    {


        len = sizeof(from);
        bzero(&from, sizeof(from));

        client = accept(sd, (struct sockaddr *) &from, &len);
        if (client < 0) {
            perror("Error at accept().\n");
            continue;
        }

        printf("Client with fd %d, connected at address %s.\n", client, conv_addr(from));
        fflush(stdout);
        clientCounter++;

        if (clientCounter % 2 == 1)                                                                                     // if it's a player waiting for opponent
        {

            int player = 0;
            Intwrite(client, &player);                                                                                  // letting the client know he's player 1, making first move

            if (socketpair(AF_UNIX, SOCK_DGRAM, 0, UNIXsocket )!= 0) {
                perror("Failed to create Unix-domain socket pair\n");
                exit(3);
            }

            if (-1 == (pid = fork())) {
                perror("Error at fork.\n");
                exit(3);
            }
            if (pid == 0) {                                                                                             // [child that handles a chess match]
                int player1_fd = client, player2_fd, nameLen, move, bytesRead;
                char specialCode;

                bzero(player_name, sizeof(player_name));
                player2_fd = ReceiveFd(UNIXsocket[0]);                                                                  // waiting for second player connection

                max_fd = (player1_fd > player2_fd) ? player1_fd : player2_fd;

                FD_ZERO (&actfds);
                FD_SET (player1_fd, &actfds);
                FD_SET (player2_fd, &actfds);

                fds[0] = player1_fd;
                fds[1] = player2_fd;

                while(1) {

                    bcopy((char *) &actfds, (char *) &readfds, sizeof(readfds));

                    if (select(max_fd + 1, &readfds, NULL, NULL, NULL) < 0) {
                        perror("Error at select().\n");
                        exit(3);
                    }

                    for (fd = 0; fd <= 1; ++fd) {
                        if (FD_ISSET (fds[fd], &readfds)) {

                            bytesRead = Charread(fds[fd], &specialCode, sizeof(char));                                  // reading the specialCode char

                            if (bytesRead == 0) {                                                                       // player disconnected => I disconnect opponent as well
                                int x = 0;
                                close(fds[fd]);
                                Intwrite(fds[1 - fd], &x);                                                              // let opponent know about disconnection by sending special code to identify disconnections
                                close(fds[1 - fd]);
                                clientCounter -= 2;
                            }

                            if (specialCode == '0') {                                                                   // specialCode -> name

                                Intread(fds[fd], &nameLen);                                                             // getting name length
                                Charread(fds[fd], player_name[fd], nameLen);                                            // getting name

                                sprintf(sql, "SELECT * FROM leaderboard WHERE username = '%s'", player_name[fd]);       // query to identify if player_name[fd] exists in username column
                                errCode = sqlite3_prepare_v2(db, sql, -1, &res, 0);
                                if (errCode != SQLITE_OK) {
                                    fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
                                    exit(2);
                                }

                                int step = sqlite3_step(res);
                                if (step != SQLITE_ROW) {                                                               // if username is not in table already, I add it with score 0
                                    sprintf(sql, "INSERT INTO leaderboard VALUES('%s',%d)", player_name[fd], 0);
                                    errCode = sqlite3_exec(db, sql, NULL, 0,&error);
                                    if (errCode != SQLITE_OK) {
                                        fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
                                        exit(2);
                                    }
                                }

                                Intwrite(fds[1 - fd], &nameLen);                                                        // sending name length
                                Charwrite(fds[1 - fd], player_name[fd], nameLen);                                       // sending name


                            }

                            else if(specialCode == '2'){                                                                // specialCode -> leaderboard request

                                if(sqlite3_prepare_v2(db, "SELECT * FROM leaderboard ORDER BY score DESC", -1, &res, 0) != SQLITE_OK)   // get leaderboard from database ordered by score
                                {
                                    sqlite3_close(db);
                                    printf("Can't retrieve data: %s\n", sqlite3_errmsg(db));
                                    exit(2);
                                }
                                int LBSize = 0;

                                while(sqlite3_step(res) == SQLITE_ROW)                                                  // as long as there are lines in database, I populate myLB
                                {

                                    strcpy(myLB[LBSize].username, sqlite3_column_text(res, 0));
                                    myLB[LBSize++].score = sqlite3_column_int(res,1);

                                }
                                write(fds[fd], &LBSize, sizeof(int));
                                write(fds[fd], &myLB, sizeof(myLB));
                            }


                            else {                                                                                      // specialCode -> move

                                int curr_i, curr_j, dest_i, dest_j;                                                     // coordinates for piece to be moved and its destination

                                Intread(fds[fd], &move);                                                                // getting coordinates as a 4-digit int

                                DecodeMove(move, &curr_i, &curr_j, &dest_i, &dest_j);                                   // getting each individual coord from 4-digit int

                                if ( IsValidMove(board, curr_i, curr_j, dest_i, dest_j) && ((board[curr_i][curr_j] > 'a' && fd == 0) || ((board[curr_i][curr_j] < 'Z' && fd == 1)))) {        // checking if the move is valid and each player moves its own pieces

                                    if (board[dest_i][dest_j] == 'K' && fd == 0)                                        // Player 2 king removed from game
                                    {
                                        move = 2;
                                        Intwrite(fds[1 - fd], &move);                                                   // sending special code to identify player 1 win
                                        Intwrite(fds[fd], &move);
                                        close(fds[0]);
                                        close(fds[1]);

                                        // updating score for player 1 in database
                                        sprintf(sql, "UPDATE leaderboard SET score = (SELECT score FROM leaderboard WHERE username ='%s') + 1 WHERE username = '%s'", player_name[0], player_name[0]);
                                        errCode = sqlite3_exec(db, sql, NULL, 0,&error);
                                        if (errCode != SQLITE_OK) {
                                            fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
                                            exit(2);
                                        }
                                    }

                                    else if (board[dest_i][dest_j] == 'k' && fd == 1)                                   // Player 1 king removed from game
                                    {
                                        move = 3;
                                        Intwrite(fds[1 - fd], &move);                                                   // sending special code to identify player 2 win
                                        Intwrite(fds[fd], &move);
                                        close(fds[0]);
                                        close(fds[1]);

                                        // updating score for player 2 in database
                                        sprintf(sql, "UPDATE leaderboard SET score = (SELECT score FROM leaderboard WHERE username ='%s') + 1 WHERE username = '%s'", player_name[1], player_name[1]);
                                        errCode = sqlite3_exec(db, sql, NULL, 0,&error);
                                        if (errCode != SQLITE_OK) {
                                            fprintf(stderr, "Failed to execute statement: %s\n", sqlite3_errmsg(db));
                                            exit(2);
                                        }
                                    }
                                    else {

                                        UpdateBoard(board, curr_i, curr_j, dest_i, dest_j);                             // update the game state matrix
                                        PrintBoard(board);

                                        move = CodeMove(curr_i, curr_j, dest_i, dest_j);                                // creating 4-digit int from coordinates
                                        Intwrite(fds[1 - fd], &move);                                                   // sending move to opponent
                                    }

                                } else {                                                                                // not a valid move

                                    move = 1;
                                    Intwrite(fds[fd], &move);                                                           // sending special code to client for identifying invalid moves

                                }
                            }  // move else
                        }  // FD_ISSET if
                    }   // for
                }   //while
            }   // child if
        }

        else {                                                                                                          // if it's the second player to be assigned to a waiting player 1
            if (pid > 0) {

                int player = 1;

                Intwrite(client, &player);                                                                              // letting the client know he's player 2
                SendFd(UNIXsocket[1] , client);                                                                         // send its fd to child
            }

            while(waitpid(-1, NULL, WNOHANG) > 0 ){}
        }



    }   // while
}



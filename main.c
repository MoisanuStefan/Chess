
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <stdbool.h>

int port = 2726;

struct leaderboard{         // structure for receiving database from server
    char id[100];
    int score;
}myLB[100];

void PrintLB(int LBSize)
{
    printf("\nUsername            |            Score\n");
    for (int i = 1; i <= 38; ++i)
        printf("_");
    printf("\n");
    for (int i = 0; i< LBSize; ++i)
    {
        printf("%-20s|%16d\n", myLB[i].id, myLB[i].score);
    }
    printf("\n");
}
int CodeMove ( int a, int b, int c, int d)
{
    return a*1000 + b*100 + c*10 + d;
}
void GetMove (int *a, int *b, int *c, int *d){
    scanf("%d", a);
    scanf("%d", b);
    scanf("%d", c);
    scanf("%d", d);
}

int main (int argc, char *argv[]) {
    int sd, player, nameLen, move,curr_i ,curr_j, dest_i,dest_j;
    struct sockaddr_in server;
    char player_names[2][100], c;


    if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Error at socket.\n");
        return errno;
    }


    /* umplem structura folosita pentru realizarea conexiunii cu serverul */
    /* familia socket-ului */
    server.sin_family = AF_INET;
    /* adresa IP a serverului */
    server.sin_addr.s_addr = inet_addr("127.0.0.1");
    /* portul de conectare */
    server.sin_port = htons(port);

    if (connect(sd, (struct sockaddr *) &server, sizeof(struct sockaddr)) == -1) {
        perror("Error at connect().\n");
        return errno;
    }

    if (read(sd, &player, sizeof(int)) < 0) {                                                                           // server tells clients if he's player 1 or 2
        perror("Error at reading from server.\n");
        return errno;
    }

    bzero(player_names[player], 100);

    printf("Insert name: ");
    fflush(stdout);

    read(0, player_names[player], 100);

    if (write(sd, "0", sizeof(char)) <= 0) {                                                                            // let server know a player name is coming
        perror("Error at writing to server.\n");
        return errno;
    }

    nameLen = strlen(player_names[player]);
    player_names[player][nameLen--] = '\0';

    if (write(sd, &nameLen, sizeof(int)) <= 0) {                                                                        // sending name length and name to server
        perror("Error at writing to server.\n");
        return errno;
    }
    if (write(sd, player_names[player], nameLen) <= 0) {
        perror("Error at writing to server.\n");
        return errno;
    }

    if ( player == 1) {
        printf("Waiting for an opponent to connect...\n");
    }

    bzero(player_names[1 - player], sizeof(player_names[1 - player]));
    if (read(sd, &nameLen, sizeof(int) ) < 0) {                                                                         // player 1 blocks on read until player 2 is connected and inputs his username
        perror("Error at reading from server.\n");
        return errno;
    }
    if (read(sd, player_names[1 - player], nameLen ) < 0) {
        perror("Error at reading from server.\n");
        return errno;
    }
    player_names[1 - player][nameLen--] = '\0';
    printf("Game started. Playing with %s\n", player_names[1 - player]);



    if ( player == 0)                                                                                                   // player 1 has one extra move because he always starts
    {
        tag: printf("Insert move (i0 j0 i j) or request leaderboard (0 0 0 0): ");
        fflush(stdout);

        GetMove(&curr_i ,&curr_j,&dest_i,&dest_j);

        move = CodeMove(curr_i ,curr_j, dest_i,dest_j);
        if (move == 0)                                                                                                  // client requested leaderboard
        {
            c = '2';
            if (write(sd, &c, sizeof(char)) <= 0) {                                                                     // send special code to server for leaderboard request
                perror("Error at writing to server.\n");
                return errno;
            }
            int LBSize;
            read(sd, &LBSize, sizeof(int));                                                                             // get leaderboard in special struct myLB
            read(sd, &myLB, sizeof(myLB));

            PrintLB(LBSize);

            fflush(stdout);
            goto tag;                                                                                                   // continue playing by inserting move
        }


        else
            c = '1';                                                                                                    // otherwise, send special code to server for incoming move

        if (write(sd, &c, sizeof(char)) <= 0) {
            perror("Error at writing to server.\n");
            return errno;
        }
        if (write(sd, &move, sizeof(int)) <= 0) {
            perror("Error at writing to server.\n");
            return errno;
        }
    }


    while (1) {

        if (read(sd, &move, sizeof(int)) < 0) {
            perror("Error at reading from server.\n");
            return errno;
        }
        if ( move == 0)                                                                                                 // special code from server to announce opponent disconnection
        {
            printf("Opponent disconnected. Connect again to start new match.\n");
            close(sd);
            break;
        }

        else if ( move == 1)                                                                                            // special code from server for invalid move
        {
            printf("Wrong move. Try again: ");
            fflush(stdout);

            GetMove(&curr_i ,&curr_j,&dest_i,&dest_j);

            c = '1';

            if (write(sd, &c, sizeof(char)) <= 0) {
                perror("Error at writing to server.\n");
                return errno;
            }

            move = CodeMove(curr_i ,curr_j, dest_i,dest_j);

            if (write(sd, &move, sizeof(int)) <= 0) {
                perror("Error at writing to server.\n");
                return errno;
            }

            continue;
        }

        else if ( move == 2) {                                                                                          // special code from server for player 1 win
            printf("%s won. Reconnect to play again.\n", player_names[0]);
            close(sd);
            break;
        }
        else if ( move == 3){
            printf("%s won. Reconnect to play again.\n", player_names[1]);
            close(sd);
            break;
        }

        tag1: printf("Insert move (i0 j0 i j) or request leaderboard (0 0 0 0): ");
        fflush(stdout);

        GetMove(&curr_i ,&curr_j,&dest_i,&dest_j);

        move = CodeMove(curr_i ,curr_j, dest_i,dest_j);

        if (move == 0)
        {
            c = '2';
            if (write(sd, &c, sizeof(char)) <= 0) {
                perror("Error at writing to server.\n");
                return errno;
            }
            int LBSize;
            read(sd, &LBSize, sizeof(int));
            read(sd, &myLB, sizeof(myLB));

            PrintLB(LBSize);

            fflush(stdout);
            goto tag1;
        }
        else {
            c = '1';


            if (write(sd, &c, sizeof(char)) <= 0) {
                perror("Error at writing to server.\n");
                return errno;
            }

            if (write(sd, &move, sizeof(int)) <= 0) {
                perror("Error at writing to server.\n");
                return errno;
            }
        }



    }


}

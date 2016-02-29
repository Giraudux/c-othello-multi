/**
 * \author Alexis Giraudet
 * gcc -ansi -Wall -pedantic -o othello-server-test -I src ./test/othello-server-test.c
 */

#define _BSD_SOURCE

#include <othello.h>

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

int main(int argc, char * argv[]) {
    int sock;
    unsigned short port;
    struct sockaddr_in address;

    ssize_t bytes_read;
    ssize_t bytes_write;

    char query_connect[1 + OTHELLO_PLAYER_NAME_LENGTH];
    char reply_connect[2];

    /*char query_room_list[1];
    char reply_room_list[1 + (1 + OTHELLO_ROOM_LENGTH * OTHELLO_PLAYER_NAME_LENGTH) * OTHELLO_NUMBER_OF_ROOMS];*/

    char query_room_join[2];
    char reply_room_join[2];

    char query_room_leave[1];
    char reply_room_leave[2];

    if(argc != 3) {
        fprintf(stderr, "usage: %s [HOST] [PORT]\n", argv[0]);
        return EXIT_FAILURE;
    }

    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;

    if(sscanf(argv[2], "%hu", &port) != 1) {
        perror("sscanf");
        return EXIT_FAILURE;
    }
    address.sin_port = htons(port);

    if(inet_aton(argv[1], &(address.sin_addr)) == 0) {
        perror("inet_aton");
        return EXIT_FAILURE;
    }

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return EXIT_FAILURE;
    }

    if(connect(sock, (struct sockaddr*) &address, sizeof(struct sockaddr_in)) < 0) {
        perror("connect");
        close(sock);
        return EXIT_FAILURE;
    }

    printf("connect");
    query_connect[0] = OTHELLO_QUERY_CONNECT;
    bytes_write = write(sock, query_connect, sizeof(query_connect));
    bytes_read = read(sock, reply_connect, sizeof(reply_connect));
    if(bytes_write == sizeof(query_connect) &&
       bytes_read == sizeof(reply_connect) &&
       reply_connect[0] == OTHELLO_QUERY_CONNECT &&
       reply_connect[1] == OTHELLO_SUCCESS) {
        printf(" SUCCESS\n");
    } else {
        printf(" FAILURE\n");
    }

    getc(stdin);

    printf("room join");
    query_room_join[0] = OTHELLO_QUERY_ROOM_JOIN;
    query_room_join[1] = 0;
    bytes_write = write(sock, query_room_join, sizeof(query_room_join));
    bytes_read = read(sock, reply_room_join, sizeof(reply_room_join));
    if(bytes_write == sizeof(query_room_join) &&
       bytes_read == sizeof(reply_room_join) &&
       reply_room_join[0] == OTHELLO_QUERY_ROOM_JOIN &&
       reply_room_join[1] == OTHELLO_SUCCESS) {
        printf(" SUCCESS\n");
    } else {
        printf(" FAILURE\n");
    }

    getc(stdin);

    printf("room leave");
    query_room_leave[0] = OTHELLO_QUERY_ROOM_LEAVE;
    bytes_write = write(sock, query_room_leave, sizeof(query_room_leave));
    bytes_read = read(sock, reply_room_leave, sizeof(reply_room_leave));
    if(bytes_write == sizeof(query_room_leave) &&
       bytes_read == sizeof(reply_room_leave) &&
       reply_room_leave[0] == OTHELLO_QUERY_ROOM_LEAVE &&
       reply_room_leave[1] == OTHELLO_SUCCESS) {
        printf(" SUCCESS\n");
    } else {
        printf(" FAILURE\n");
    }

    getc(stdin);

    close(sock);

    return EXIT_SUCCESS;
}
/**
 * \author Alexis Giraudet
 */

#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "othello.h"

struct othello_player_s;
struct othello_room_s;

struct othello_player_s {
    pthread_t thread;
    int socket;
    char name[OTHELLO_PLAYER_NAME_LENGTH];
    struct othello_room_s * room;
    pthread_mutex_t mutex;
    bool ready;
    enum othello_state_e state;
};

struct othello_room_s {
    struct othello_player_s * players[OTHELLO_ROOM_LENGTH];
    pthread_mutex_t mutex;
    char othellier[OTHELLO_BOARD_LENGTH][OTHELLO_BOARD_LENGTH];
};

typedef struct othello_player_s othello_player_t;
typedef struct othello_room_s othello_room_t;

static othello_room_t rooms[OTHELLO_NUMBER_OF_ROOMS];

/**
 * \return the result of the last call to read
 */
ssize_t othello_read_all(int fd, void * buf, size_t count) {
    ssize_t bytes_read = 0;

    while(count > 0 && (bytes_read = read(fd, buf, count)) > 0) {
        count -= bytes_read;
        buf += bytes_read;
    }

    return bytes_read;
}

/**
 * \return the result of the last call to write
 */
ssize_t othello_write_all(int fd, void * buf, size_t count) {
    ssize_t bytes_write = 0;

    while(count > 0 && (bytes_write = write(fd, buf, count)) > 0) {
        count -= bytes_write;
        buf += bytes_write;
    }

    return bytes_write;
}

int othello_connect(othello_player_t * player) {
    /*read message length*/
    /*unsigned char name_length;*/
    unsigned char reply[2];

    reply[0] = OTHELLO_QUERY_CONNECT;
    reply[1] = OTHELLO_SUCCESS;

    /*othello_read_all(player->socket, &name_length, 1);*/
    /*TODO: check length + check read*/
    /*read message*/
    pthread_mutex_lock(&(player->mutex));
    othello_read_all(player->socket, player->name, OTHELLO_PLAYER_NAME_LENGTH);
    /*check player state*/
    /*if player state and player name ok then update player state*/
    /*else send error ?*/

    othello_write_all(player->socket, reply, sizeof(reply));
    pthread_mutex_unlock(&(player->mutex));

    return 0;
}

int othello_list_room(othello_player_t * player) {
    unsigned char reply[1 + (1 + OTHELLO_ROOM_LENGTH * OTHELLO_PLAYER_NAME_LENGTH) * OTHELLO_NUMBER_OF_ROOMS];
    unsigned char * cursor;
    othello_room_t * room;
    othello_player_t * other_player;
    int i, j;

    memset(reply, 0, sizeof(reply));
    reply[0] = OTHELLO_QUERY_LIST_ROOM;
    cursor = reply + 1;

    for(i = 0; i < OTHELLO_NUMBER_OF_ROOMS; i++) {
        room = &(rooms[i]);
        pthread_mutex_lock(&(room->mutex));
        for(j = 0; j < OTHELLO_ROOM_LENGTH; j++) {
            other_player = room->players[j];
            pthread_mutex_lock(&(player->mutex));
            if(other_player != NULL) {
                (*cursor)++;
                memcpy(cursor + 1 + j * OTHELLO_PLAYER_NAME_LENGTH, other_player->name, OTHELLO_PLAYER_NAME_LENGTH);
            }
            pthread_mutex_unlock(&(player->mutex));
        }
        pthread_mutex_unlock(&(room->mutex));
        cursor += 1 + OTHELLO_ROOM_LENGTH * OTHELLO_PLAYER_NAME_LENGTH;
    }

    pthread_mutex_lock(&(player->mutex));
    othello_write_all(player->socket, reply, sizeof(reply));
    pthread_mutex_unlock(&(player->mutex));

    return 0;
}

int othello_join_room(othello_player_t * player) {
    unsigned char room_id;
    char reply[2];
    int i;

    reply[0] = OTHELLO_QUERY_JOIN_ROOM;
    reply[1] = OTHELLO_FAILURE;

    pthread_mutex_lock(&(player->mutex));
    othello_read_all(player->socket, &room_id, 1);

    if(player->room == NULL && /*room_id >= 0 &&*/ room_id < OTHELLO_NUMBER_OF_ROOMS) {
        /*reply[1] = OTHELLO_ROOM_FULL_ERROR;*/

        pthread_mutex_lock(&(rooms[room_id].mutex));
        for(i = 0; i < OTHELLO_ROOM_LENGTH; i++) {
            if(rooms[room_id].players[i] == NULL) {
                rooms[room_id].players[i] = player;
                player->room = &(rooms[room_id]);
                reply[1] = OTHELLO_SUCCESS;
                break;
            }
        }
        pthread_mutex_unlock(&(rooms[room_id].mutex));
    }

    othello_write_all(player->socket, reply, sizeof(reply));
    pthread_mutex_unlock(&(player->mutex));

    return 0;
}

int othello_leave_room(othello_player_t * player) {
    unsigned char reply[2];
    int i;

    reply[0] = OTHELLO_QUERY_LEAVE_ROOM;
    reply[1] = OTHELLO_FAILURE;

    pthread_mutex_lock(&(player->mutex));

    if(player->room != NULL) {
        pthread_mutex_lock(&(player->room->mutex));
        for(i = 0; i < OTHELLO_ROOM_LENGTH; i++) {
            if(player->room->players[i] == player) {
                player->room->players[i] = NULL;
                break;
            }
        }
        pthread_mutex_unlock(&(player->room->mutex));
        player->room = NULL;
        reply[1] = OTHELLO_SUCCESS;
    }

    othello_write_all(player->socket, reply, sizeof(reply));
    pthread_mutex_unlock(&(player->mutex));

    return 0;
}

int othello_send_message(othello_player_t * player) {
    unsigned char reply[1 + OTHELLO_PLAYER_NAME_LENGTH + OTHELLO_MESSAGE_LENGTH];
    othello_player_t * other_player;
    othello_room_t * room;
    int i;

    reply[0] = OTHELLO_QUERY_SEND_MESSAGE;

    pthread_mutex_lock(&(player->mutex));
    memcpy(reply + 1, player->name, OTHELLO_PLAYER_NAME_LENGTH);
    othello_read_all(player->socket, reply + 1 + OTHELLO_PLAYER_NAME_LENGTH, OTHELLO_MESSAGE_LENGTH);
    room = player->room;
    pthread_mutex_unlock(&(player->mutex));

    if(room != NULL) {
        pthread_mutex_lock(&(room->mutex));
        for(i = 0; i < OTHELLO_ROOM_LENGTH; i++) {
            other_player = room->players[i];
            if(other_player != NULL && other_player != player) {
                pthread_mutex_lock(&(other_player->mutex));
                othello_write_all(other_player->socket, reply, sizeof(reply));
                pthread_mutex_unlock(&(other_player->mutex));
            }
        }
        pthread_mutex_unlock(&(room->mutex));
    }

    return 0;
}

int othello_ready(othello_player_t * player) {
    return 0;
}

int othello_play_turn(othello_player_t * player) {


    return 0;
}

void othello_end(othello_player_t * player) {
    int i;

    /*leave room*/
    pthread_mutex_lock(&(player->mutex));
    if(player->room != NULL) {
        pthread_mutex_lock(&(player->room->mutex));
        for(i = 0; i < OTHELLO_ROOM_LENGTH; i++) {
            if(player->room->players[i] == player) {
                player->room->players[i] = NULL;
                break;
            }
        }
        pthread_mutex_unlock(&(player->room->mutex));
    }

    /*TODO: manage if player in game*/

    /*close socket*/
    close(player->socket);
    /*destroy mutex*/
    pthread_mutex_unlock(&(player->mutex));
    pthread_mutex_destroy(&(player->mutex));
    /*free memory*/
    free(player);
    /*exit thread*/
    pthread_exit(NULL);
}

void * othello_start(void * player) {
    char query_code;

    while(othello_read_all(((othello_player_t*)player)->socket, &query_code, 1) > 0) {
        /*switch over query code and player state*/
        switch(query_code) {
        case OTHELLO_QUERY_CONNECT:
            othello_connect(player);
            break;
        default:
            break; /*error*/
        }
    }

    /*leave room if player in one*/
    othello_end((othello_player_t*) player);

    return NULL;
}

int othello_create_socket_stream(unsigned short port) {
    int sock;
    struct sockaddr_in address;

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket");
        return -1;
    }

    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sock, (struct sockaddr *) & address, sizeof(struct sockaddr_in)) < 0) {
        close(sock);
        perror("bind");
        return -1;
    }

    return sock;
}

int main(int argc, char * argv[]) {
    int sock, i;
    othello_player_t * player;

    memset(rooms, 0, sizeof(othello_room_t) * OTHELLO_NUMBER_OF_ROOMS);
    for(i = 0; i < OTHELLO_NUMBER_OF_ROOMS; i++) {
        if(pthread_mutex_init(&(rooms[i].mutex), NULL)) {
            perror("pthread_mutex_init");
            return EXIT_FAILURE;
        }
    }


    if((sock = othello_create_socket_stream(5000)) < 0) {
        perror("othello_create_socket_stream");
        return EXIT_FAILURE;
    }

    listen(sock, 5);

    for(;;) {
        if((player = malloc(sizeof(othello_player_t))) == NULL) {
            perror("malloc");
            return EXIT_FAILURE;
        }

        memset(player, 0, sizeof(othello_player_t));

        if ((player->socket = accept(sock, NULL, NULL)) < 0) {
            perror("accept");
            return EXIT_FAILURE;
        }

        if(pthread_mutex_init(&(player->mutex), NULL)) {
            perror("pthread_mutex_init");
            return EXIT_FAILURE;
        }

        if(pthread_create(&(player->thread), NULL, othello_start, player)) {
            perror("pthread_create");
            return EXIT_FAILURE;
        }
    }

    close(sock);

    return EXIT_SUCCESS;
}

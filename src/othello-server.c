/**
 * \author Alexis Giraudet
 */

#include "othello.h"
#include "othello-server.h"

#ifdef OTHELLO_WITH_SYSLOG
#define _BSD_SOURCE
#endif

#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>

struct othello_player_s {
    pthread_t thread;
    int socket;
    char name[OTHELLO_PLAYER_NAME_LENGTH];
    othello_room_t * room;
    pthread_mutex_t mutex;
    bool ready;
    enum othello_state_e state;
};

struct othello_room_s {
    othello_player_t * players[OTHELLO_ROOM_LENGTH];
    pthread_mutex_t mutex;
    char grid[OTHELLO_BOARD_LENGTH][OTHELLO_BOARD_LENGTH];
};

static othello_room_t rooms[OTHELLO_NUMBER_OF_ROOMS];
static int sock;

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

void othello_log(int priority, const char * format, ...) {
    va_list ap;

    va_start(ap, format);
#ifdef OTHELLO_WITH_SYSLOG
    vsyslog(priority, format, ap);
#else
    switch(priority) {
        case LOG_EMERG:
            fputs("EMERGENCY ", stdout);
            break;
        case LOG_ALERT:
            fputs("ALERT     ", stdout);
            break;
        case LOG_CRIT:
            fputs("CRITICAL  ", stdout);
            break;
        case LOG_ERR:
            fputs("ERROR     ", stdout);
            break;
        case LOG_WARNING:
            fputs("WARNING   ", stdout);
            break;
        case LOG_NOTICE:
            fputs("NOTICE    ", stdout);
            break;
        case LOG_INFO:
            fputs("INFO      ", stdout);
            break;
        case LOG_DEBUG:
            fputs("DEBUG     ", stdout);
            break;
        default:
            fputs("          ", stdout);
            break;
    }
    vprintf(format, ap);
    putc('\n', stdout);
#endif
}

void othello_end(othello_player_t * player) {
    int i;

    /*leave room*/
    pthread_mutex_lock(&(player->mutex));
    close(player->socket);

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

    /*destroy mutex*/
    pthread_mutex_unlock(&(player->mutex));
    pthread_mutex_destroy(&(player->mutex));
    /*free memory*/
    free(player);
    /*exit thread*/
    pthread_exit(NULL);
}

int othello_connect(othello_player_t * player) {
    unsigned char reply[2];
    int status;

    reply[0] = OTHELLO_QUERY_CONNECT;
    reply[1] = OTHELLO_SUCCESS;

    status = OTHELLO_SUCCESS;

    /*TODO: check protocol version ?*/
    pthread_mutex_lock(&(player->mutex));

    if(player->state != OTHELLO_STATE_NOT_CONNECTED) {
        reply[1] = OTHELLO_FAILURE;
    }

    if(othello_read_all(player->socket, player->name, OTHELLO_PLAYER_NAME_LENGTH) <= 0) {
        status =  OTHELLO_FAILURE;
    }

    if(othello_write_all(player->socket, reply, sizeof(reply)) <= 0) {
        status = OTHELLO_FAILURE;
    }

    player->state = OTHELLO_STATE_CONNECTED;

    pthread_mutex_unlock(&(player->mutex));

    return status;
}

/*TODO: add room id*/
int othello_list_room(othello_player_t * player) {
    unsigned char reply[1 + (1 + OTHELLO_ROOM_LENGTH * OTHELLO_PLAYER_NAME_LENGTH) * OTHELLO_NUMBER_OF_ROOMS];
    unsigned char * cursor;
    othello_room_t * room;
    othello_player_t * other_player;
    int status;
    int i, j;

    memset(reply, 0, sizeof(reply));
    reply[0] = OTHELLO_QUERY_ROOM_LIST;
    cursor = reply + 1;

    status = OTHELLO_SUCCESS;

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

    return status;
}

int othello_join_room(othello_player_t * player) {
    unsigned char room_id;
    char reply[2];
    char notif[1 + OTHELLO_PLAYER_NAME_LENGTH];
    int status;
    int i;

    reply[0] = OTHELLO_QUERY_ROOM_JOIN;
    reply[1] = OTHELLO_FAILURE;

    notif[0] = OTHELLO_NOTIF_ROOM_JOIN;

    status = OTHELLO_FAILURE;

    pthread_mutex_lock(&(player->mutex));
    othello_read_all(player->socket, &room_id, sizeof(room_id));

    if(player->room == NULL &&
       room_id < OTHELLO_NUMBER_OF_ROOMS &&
       player->state == OTHELLO_STATE_CONNECTED) {

       memcpy(notif + 1, player->name, OTHELLO_PLAYER_NAME_LENGTH);
        /*TODO: notif*/
        pthread_mutex_lock(&(rooms[room_id].mutex));
        for(i = 0; i < OTHELLO_ROOM_LENGTH; i++) {
            if(rooms[room_id].players[i] == NULL) {
                rooms[room_id].players[i] = player;
                player->room = &(rooms[room_id]);
                player->state = OTHELLO_STATE_IN_ROOM;
                player->ready = false;
                reply[1] = OTHELLO_SUCCESS;
                status = OTHELLO_SUCCESS;
                break;
            }
        }
        pthread_mutex_unlock(&(rooms[room_id].mutex));
    }

    if(othello_write_all(player->socket, reply, sizeof(reply)) <= 0) {
        status = OTHELLO_FAILURE;
    }
    pthread_mutex_unlock(&(player->mutex));

    return status;
}

int othello_leave_room(othello_player_t * player) {
    unsigned char reply[2];
    unsigned char notif[1 + OTHELLO_PLAYER_NAME_LENGTH];
    othello_room_t * room;
    int status;
    int i;

    reply[0] = OTHELLO_QUERY_ROOM_LEAVE;
    reply[1] = OTHELLO_FAILURE;

    notif[0] = OTHELLO_NOTIF_ROOM_LEAVE;

    status = OTHELLO_FAILURE;

    pthread_mutex_lock(&(player->mutex));
    room = player->room;
    if(player->state == OTHELLO_STATE_IN_ROOM && room != NULL) {
        reply[1] = OTHELLO_SUCCESS;
        status = OTHELLO_SUCCESS;
    }
    player->room = NULL;
    player->state = OTHELLO_STATE_CONNECTED;
    memcpy(notif + 1, player->name, OTHELLO_PLAYER_NAME_LENGTH);
    othello_write_all(player->socket, reply, sizeof(reply));/*TODO: check return value*/
    pthread_mutex_unlock(&(player->mutex));

    if(room != NULL) {
        pthread_mutex_lock(&(room->mutex));
        for(i = 0; i < OTHELLO_ROOM_LENGTH; i++) {
            if(room->players[i] == player) {
                room->players[i] = NULL;
            } else if(room->players[i] != NULL) {
                pthread_mutex_lock(&(room->players[i]->mutex));
                othello_write_all(room->players[i]->socket, notif, sizeof(notif));
                pthread_mutex_unlock(&(room->players[i]->mutex));
            }
        }
        pthread_mutex_unlock(&(player->room->mutex));
    }

    return status;
}

int othello_send_message(othello_player_t * player) {
    unsigned char reply[2];
    unsigned char notif[1 + OTHELLO_PLAYER_NAME_LENGTH + OTHELLO_MESSAGE_LENGTH];
    othello_player_t * other_player;
    othello_room_t * room;
    int status;
    int i;

    reply[0] = OTHELLO_QUERY_MESSAGE;
    reply[1] = OTHELLO_SUCCESS;

    notif[0] = OTHELLO_NOTIF_MESSAGE;

    status = OTHELLO_SUCCESS;

    pthread_mutex_lock(&(player->mutex));
    room = player->room;
    memcpy(notif + 1, player->name, OTHELLO_PLAYER_NAME_LENGTH);

    if(room == NULL) {
        reply[1] = OTHELLO_FAILURE;
    }
    if(othello_read_all(player->socket, notif + 1 + OTHELLO_PLAYER_NAME_LENGTH, OTHELLO_MESSAGE_LENGTH) <= 0) {
        status = OTHELLO_FAILURE;
    }
    if(othello_write_all(player->socket, reply, sizeof(reply)) <= 0) {
        status = OTHELLO_FAILURE;
    }
    pthread_mutex_unlock(&(player->mutex));

    if(room != NULL) {
        pthread_mutex_lock(&(room->mutex));
        for(i = 0; i < OTHELLO_ROOM_LENGTH; i++) {
            other_player = room->players[i];
            if(other_player != NULL && other_player != player) {
                pthread_mutex_lock(&(other_player->mutex));
                othello_write_all(other_player->socket, notif, sizeof(notif));
                pthread_mutex_unlock(&(other_player->mutex));
            }
        }
        pthread_mutex_unlock(&(room->mutex));
    }

    return status;
}

int othello_ready(othello_player_t * player) {
    unsigned char ready;

    /*TODO: check player's state*/

    pthread_mutex_lock(&(player->mutex));
    othello_read_all(player->socket, &ready, sizeof(ready));
    if(ready)
        player->ready = true;
    else
        player->ready = false;
    pthread_mutex_unlock(&(player->mutex));

    /*TODO: reply to player*/
    /*TODO: notify players*/

    return 0;
}

int othello_play(othello_player_t * player) {
    unsigned char stroke[2];

    /*TODO: check player state*/

    pthread_mutex_lock(&(player->mutex));
    othello_read_all(player->socket, stroke, sizeof(stroke));
    pthread_mutex_unlock(&(player->mutex));

    /*TODO: valid stroke*/
    /*TODO: reply to player*/
    /*TODO: notify players*/

    return 0;
}

void * othello_start(void * arg) {
    othello_player_t * player;
    char query;
    int status;

    player = (othello_player_t*) arg;

    while(othello_read_all(player->socket, &query, sizeof(query)) > 0) {
        /*pthread_mutex_lock(&(player->mutex));*/
        switch(query) {
        case OTHELLO_QUERY_CONNECT:
            status = othello_connect(player);
            break;
        case OTHELLO_QUERY_ROOM_LIST:
            status = othello_list_room(player);
            break;
        case OTHELLO_QUERY_ROOM_JOIN:
            status = othello_list_room(player);
            break;
        case OTHELLO_QUERY_ROOM_LEAVE:
            status = othello_leave_room(player);
            break;
        case OTHELLO_QUERY_MESSAGE:
            status = othello_send_message(player);
            break;
        case OTHELLO_QUERY_READY:
            status = othello_ready(player);
            break;
        case OTHELLO_QUERY_PLAY:
            status = othello_play(player);
            break;
        default:
            status = OTHELLO_FAILURE;
            break;
        }
        /*pthread_mutex_unlock(&(player->mutex));*/

        if(status)
            break;
    }

    /*leave room if player in one*/
    othello_end(player);

    return NULL;
}

int othello_create_socket_stream(unsigned short port) {
    int socket_stream, status;
    struct sockaddr_in address;

    if((socket_stream = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        othello_log(LOG_ERR, "socket");
        return socket_stream;
    }

    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if((status = bind(socket_stream, (struct sockaddr *) & address, sizeof(struct sockaddr_in))) < 0) {
        close(socket_stream);
        othello_log(LOG_ERR, "bind");
        return status;
    }

    return socket_stream;
}

void othello_exit() {
    if(sock >= 0)
        close(sock);
#ifdef OTHELLO_WITH_SYSLOG
    closelog();
#endif
}

int main(int argc, char * argv[]) {
    othello_player_t * player;
    int i;

    /* init global */
    /* init socket */
    sock = -1;
#ifdef OTHELLO_WITH_SYSLOG
    openlog(NULL, LOG_CONS | LOG_PID, LOG_USER);
#endif

    if(atexit(othello_exit))
        return EXIT_FAILURE;

    /* init rooms */
    memset(rooms, 0, sizeof(othello_room_t) * OTHELLO_NUMBER_OF_ROOMS);
    for(i = 0; i < OTHELLO_NUMBER_OF_ROOMS; i++) {
        if(pthread_mutex_init(&(rooms[i].mutex), NULL)) {
            othello_log(LOG_ERR, "pthread_mutex_init");
            return EXIT_FAILURE;
        }
    }

    /* open socket */
    if((sock = othello_create_socket_stream(5000)) < 0) {
        othello_log(LOG_ERR, "othello_create_socket_stream");
        return EXIT_FAILURE;
    }

    if(listen(sock, SOMAXCONN)) {
        othello_log(LOG_ERR, "listen");
        return EXIT_FAILURE;
    }

    for(;;) {
        if((player = malloc(sizeof(othello_player_t))) == NULL) {
            othello_log(LOG_ERR, "malloc");
            return EXIT_FAILURE;
        }

        memset(player, 0, sizeof(othello_player_t));
        if(pthread_mutex_init(&(player->mutex), NULL)) {
            othello_log(LOG_ERR, "pthread_mutex_init");
            return EXIT_FAILURE;
        }

        if ((player->socket = accept(sock, NULL, NULL)) < 0) {
            othello_log(LOG_ERR, "accept");
            return EXIT_FAILURE;
        }

        if(pthread_mutex_init(&(player->mutex), NULL)) {
            othello_log(LOG_ERR, "pthread_mutex_init");
            return EXIT_FAILURE;
        }

        if(pthread_create(&(player->thread), NULL, othello_start, player)) {
            othello_log(LOG_ERR, "pthread_create");
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

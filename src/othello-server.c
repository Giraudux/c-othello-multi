/**
 * \author Alexis Giraudet
 */

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
#include <unistd.h>
#include <syslog.h>

#include "othello.h"
#include "othello-server.h"

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

    /*check and end here ?*/
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
    int status = OTHELLO_SUCCESS;
    unsigned char reply[2];

    reply[0] = OTHELLO_QUERY_CONNECT;
    reply[1] = OTHELLO_SUCCESS;

    /*TODO: check protocol version ?*/
    pthread_mutex_lock(&(player->mutex));

    if(player->state != OTHELLO_STATE_NOT_CONNECTED) {
        status =  OTHELLO_FAILURE;
        reply[1] = OTHELLO_FAILURE;
    }

    if(othello_read_all(player->socket, player->name, OTHELLO_PLAYER_NAME_LENGTH) <= 0) {
        status =  OTHELLO_FAILURE;
        reply[1] = OTHELLO_FAILURE;
    }

    if(othello_write_all(player->socket, reply, sizeof(reply)) <= 0) {
        status = OTHELLO_FAILURE;
        reply[1] = OTHELLO_FAILURE;
    }

    pthread_mutex_unlock(&(player->mutex));

    return status;
}

/*TODO: add room id*/
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
    othello_read_all(player->socket, &room_id, sizeof(room_id));

    if(player->room == NULL &&
       room_id < OTHELLO_NUMBER_OF_ROOMS &&
       player->state == OTHELLO_STATE_CONNECTED) {

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
    unsigned char reply[2];
    unsigned char notif[1 + OTHELLO_PLAYER_NAME_LENGTH + OTHELLO_MESSAGE_LENGTH];
    othello_player_t * other_player;
    othello_room_t * room;
    int i;

    reply[0] = OTHELLO_QUERY_SEND_MESSAGE;
    reply[1] = OTHELLO_FAILURE;

    notif[0] = OTHELLO_NOTIFICATION_MESSAGE;

    pthread_mutex_lock(&(player->mutex));
    memcpy(notif + 1, player->name, OTHELLO_PLAYER_NAME_LENGTH);
    othello_read_all(player->socket, notif + 1 + OTHELLO_PLAYER_NAME_LENGTH, OTHELLO_MESSAGE_LENGTH);
    room = player->room;

    if(room != NULL &&
       (player->state == OTHELLO_STATE_IN_ROOM ||
        player->state == OTHELLO_STATE_IN_GAME)) {
        reply[1] = OTHELLO_SUCCESS;
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

    othello_read_all(player->socket, reply, sizeof(reply));
    pthread_mutex_unlock(&(player->mutex));

    return 0;
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

int othello_play_turn(othello_player_t * player) {
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

void * othello_start(void * player) {
    char query;
    int status;

    while(othello_read_all(((othello_player_t*)player)->socket, &query, sizeof(query)) > 0) {
        switch(query) {
        case OTHELLO_QUERY_CONNECT:
            status = othello_connect(player);
            break;
        case OTHELLO_QUERY_LIST_ROOM:
            status = othello_list_room(player);
            break;
        case OTHELLO_QUERY_JOIN_ROOM:
            status = othello_list_room(player);
            break;
        case OTHELLO_QUERY_LEAVE_ROOM:
            status = othello_leave_room(player);
        case OTHELLO_QUERY_SEND_MESSAGE:
            status = othello_send_message(player);
        case OTHELLO_QUERY_READY:
            status = othello_ready(player);
            break;
        default:
            status = OTHELLO_FAILURE;
            break;
        }

        if(status)
            break;
    }

    /*leave room if player in one*/
    othello_end((othello_player_t*) player);

    return NULL;
}

int othello_create_socket_stream(unsigned short port) {
    int sock;
    struct sockaddr_in address;

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        othello_log(LOG_ERR, "socket");
        return -1;
    }

    memset(&address, 0, sizeof(struct sockaddr_in));
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    if(bind(sock, (struct sockaddr *) & address, sizeof(struct sockaddr_in)) < 0) {
        close(sock);
        othello_log(LOG_ERR, "bind");
        return -1;
    }

    return sock;
}

void othello_exit() {
    if(sock != 0)
        close(sock);
#ifdef OTHELLO_WITH_SYSLOG
    closelog();
#endif
}

int main(int argc, char * argv[]) {
    othello_player_t * player;
    int i;

    sock = 0;
#ifdef OTHELLO_WITH_SYSLOG
    openlog(NULL, LOG_CONS | LOG_PID, LOG_USER);
#endif

    if(atexit(othello_exit))
        return EXIT_FAILURE;

    memset(rooms, 0, sizeof(othello_room_t) * OTHELLO_NUMBER_OF_ROOMS);
    for(i = 0; i < OTHELLO_NUMBER_OF_ROOMS; i++) {
        if(pthread_mutex_init(&(rooms[i].mutex), NULL)) {
            othello_log(LOG_ERR, "pthread_mutex_init");
            return EXIT_FAILURE;
        }
    }


    if((sock = othello_create_socket_stream(5000)) < 0) {
        othello_log(LOG_ERR, "othello_create_socket_stream");
        return EXIT_FAILURE;
    }

    listen(sock, 5);

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

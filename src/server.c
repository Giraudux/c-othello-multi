/*----------------------------------------------
Serveur à lancer avant le client
------------------------------------------------*/
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "othello.h"

#define TAILLE_MAX_NOM 256

struct othello_player_s;
struct othello_room_s;

struct othello_player_s {
    pthread_t thread;
    int socket;
    char name[32];/*define macro*/
    struct othello_room_s * room;
    pthread_mutex_t mutex;/*to write on socket*/
    bool ready;
    enum othello_state_e state;
};

struct othello_room_s {
    struct othello_player_s * players[2];/*define macro*/
    int players_size;
    pthread_mutex_t mutex;
    char othellier[OTHELLO_BOARD_LENGTH][OTHELLO_BOARD_LENGTH];
};

typedef struct othello_player_s othello_player_t;
typedef struct othello_room_s othello_room_t;

/*------------------------------------------------------*/
void renvoi (int sock) {

    char buffer[256];
    int longueur;
   
    if ((longueur = read(sock, buffer, sizeof(buffer))) <= 0) 
        return;
    
    printf("message lu : %s \n", buffer);
    
    buffer[0] = 'R';
    buffer[1] = 'E';
    buffer[longueur] = '#';
    buffer[longueur+1] ='\0';
    
    printf("message apres traitement : %s \n", buffer);
    
    printf("renvoi du message traite.\n");

    /* mise en attente du prgramme pour simuler un delai de transmission */
    sleep(3);
    
    write(sock,buffer,strlen(buffer)+1);
    
    printf("message envoye. \n");
        
    return;
    
}
/*------------------------------------------------------*/

/**
 * \return the result of the last call to read
 */
ssize_t othello_read_all(int fd, void * buf, size_t count) {
    ssize_t bytes_read;

    while((bytes_read = read(fd, buf, count)) > 0 && count > 0) {
        count -= bytes_read;
        buf += bytes_read;
    }

    return bytes_read;
}

/**
 * \return the result of the last call to write
 */
ssize_t othello_write_all(int fd, void * buf, size_t count) {
    ssize_t bytes_write;

    while((bytes_write = write(fd, buf, count)) > 0 && count > 0) {
        count -= bytes_write;
        buf += bytes_write;
    }

    return bytes_write;
}

int othello_connect(othello_player_t * player) {
    /*read message length*/
    char name_length;
    char reply[2];
    othello_read_all(player->socket, &name_length, 1);
    /*TODO: check length + check read*/
    /*read message*/
    othello_read_all(player->socket, player->name, name_length);
    /*check player state*/
    /*if player state and player name ok then update player state*/
    /*else send error ?*/

    pthread_mutex_lock(&(player->mutex));
    reply[0] = OTHELLO_QUERY_CONNECT;
    reply[1] = OTHELLO_SUCCESS;
    othello_write_all(player->socket, reply, 2);
    pthread_mutex_unlock(&(player->mutex));

    return 0;
}

int othello_list_room(othello_player_t * player) {
    return 0;
}

int othello_join_room(othello_player_t * player) {
    return 0;
}

int othello_leave_room(othello_player_t * player) {
    return 0;
}

int othello_send_message(othello_player_t * player) {
    return 0;
}

int othello_play_turn(othello_player_t * player) {
    return 0;
}

void * othello_start(void * player) {
    char query_code;

    while(othello_read_all(((othello_player_t*)player)->socket, &query_code, 1) > 0) {
        /*switch over query code and player state*/
        switch(query_code) {
            case OTHELLO_QUERY_CONNECT:
                othello_connect(player);
                break;
            default: break; /*error*/
        }
    }

    /*leave room if player in one*/
    close(((othello_player_t*)player)->socket);
    free(player);

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
    int sock;
    othello_player_t * player;

    if((sock = othello_create_socket_stream(5000)) < 0) {
        perror("othello_create_socket_stream");
        return 1;
    }

    listen(sock, 5);

    for(;;) {
        if((player = malloc(sizeof(othello_player_t))) == NULL) {
            perror("malloc");
            return 1;
        }

        memset(player, 0, sizeof(othello_player_t));

        if ((player->socket = accept(sock, NULL, NULL)) < 0) {
            perror("accept");
            return 1;
        }

        if(pthread_mutex_init(&(player->mutex), NULL)) {
            perror("pthread_mutex_init");
            return 1;
        }

        if(pthread_create(&(player->thread), NULL, othello_start, player)) {
            perror("pthread_create");
            return 1;
        }
    }

    close(sock);

    return 0;
}

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

int othello_connect(othello_player_t * player) {
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
    renvoi(((othello_player_t*)player)->socket);
    close(((othello_player_t*)player)->socket);
    free(player);

    return NULL;
}

int create_socket_stream(unsigned short port) {
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

    if((sock = create_socket_stream(5000)) < 0) {
        perror("create_socket_stream");
        return 1;
    }

    listen(sock, 5);

    for(;;) {
        if((player = malloc(sizeof(othello_player_t))) == NULL) {
            perror("malloc");
            return 1;
        }

        if ((player->socket = accept(sock, NULL, NULL)) < 0) {
            perror("accept");
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

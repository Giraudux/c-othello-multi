/*-----------------------------------------------------------
Client a lancer apres le serveur avec la commande :
client <adresse-serveur> <message-a-transmettre>
------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>

#include <stdbool.h>
#include "othello.h"


#define OTHELLO_USER_INPUT_SIZE 256
#define OTHELLO_ANSWER_FRAGMENT_SIZE 256
#define OTHELLO_ANSWER_TOTAL_SIZE 256

typedef struct sockaddr 	sockaddr;
typedef struct sockaddr_in 	sockaddr_in;
typedef struct hostent 		hostent;
typedef struct servent 		servent;


bool othello_is_number(char*);
void othello_ask_user_input(char*,size_t);
void othello_write_mesg(int,char*);
char* othello_read_mesg(int);
void othello_shift_array(char*,size_t);

int main(int argc, char **argv) {
    int socket_descriptor; /* descripteur de socket */
	sockaddr_in adresse_locale; /* adresse de socket local */
    hostent* ptr_host; /* info sur une machine hote */
    servent* ptr_service; /* info sur service */

	int i; /* use for loops iterator */
	char* server_answer; /* fragments sum composing the final answer */
	char user_input[OTHELLO_USER_INPUT_SIZE]; /* gathers the user inputs */
	
   
	
	printf("Welcome, please enter the server adresse : ");

	othello_ask_user_input(user_input,sizeof user_input);
	if ((ptr_host = gethostbyname(user_input)) == NULL) {
		perror("erreur : impossible de trouver le serveur a partir de son adresse.");
		exit(1);
    }
  
    // copie caractere par caractere des infos de ptr_host vers adresse_locale
    bcopy((char*)ptr_host->h_addr, (char*)&adresse_locale.sin_addr, ptr_host->h_length);
    adresse_locale.sin_family = AF_INET; // ou ptr_host->h_addrtype;

	adresse_locale.sin_port = htons(5000);

 	//creation de la socket
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("erreur : impossible de creer la socket de connexion avec le serveur.");
		exit(1);
    }

	//tentative de connexion au serveur dont les infos sont dans adresse_locale
    if ((connect(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
		perror("erreur : impossible de se connecter au serveur.");
		exit(1);
    }
	
	printf("connexion succed ! \n");

	/**********************************/
	/******** NICKNAME CHOICE *********/
	/**********************************/
	printf("Choose a nickname : ");
	othello_ask_user_input(user_input,sizeof user_input);
	othello_write_mesg(socket_descriptor,user_input);
	server_answer = othello_read_mesg(socket_descriptor);
	while(!strcmp(server_answer,"ok")){
		printf("Invalid nickname, try again : ");
		othello_ask_user_input(user_input,sizeof user_input);
		othello_write_mesg(socket_descriptor,user_input);
		server_answer = othello_read_mesg(socket_descriptor);
	}

	/**********************************/
	/********** GAME ROUTINE **********/
	/**********************************/
	memset(user_input,0,sizeof(user_input));
	while(strcmp(user_input,"quit")!=0){
		
		/**********************************/
		/********** ROOM CHOICE ***********/
		/**********************************/
		server_answer = "";
		while(strcmp(server_answer,"ok")!=0){
			printf("Choose the room you want to enter or type 'list' to get the list of them : ");
			othello_ask_user_input(user_input,sizeof user_input);
			while(!(othello_is_number(user_input) || strcmp(user_input,"list")==0)){
				if(strcmp(user_input,"exit")==0){ /* player whant to leave the game */
					printf("Thanks for playing!");
					return 0;
				}
				printf("You didn't entered a number...\nType 'list' to get the list of valid rooms or a room number : ");
				othello_ask_user_input(user_input,sizeof user_input);
			}
			if(strcmp(user_input,"list")==0){
				memset(user_input,0,sizeof(user_input));
				user_input[0] = OTHELLO_QUERY_LIST_ROOM;
				othello_write_mesg(socket_descriptor,user_input);
				server_answer = othello_read_mesg(socket_descriptor);
				//display answer
			}
			if(othello_is_number(user_input)){
				othello_shift_array(user_input,sizeof user_input);
				user_input[0] = OTHELLO_QUERY_JOIN_ROOM;
				othello_write_mesg(socket_descriptor,user_input);
				server_answer = othello_read_mesg(socket_descriptor);
				if(strcmp(server_answer,"ok")!=0){
					printf("The choosen room doesn't exist or hasn't empty slot...\n");
				}
			}
		}

		/******************************/
		/***** ASK READY TO USER ******/
		/******************************/
		memset(user_input,0,sizeof(user_input));
		server_answer = "";
		while(strcmp(server_answer,"ok")!=0){
			while(strcmp(user_input,"ready") != 0){
				printf("Please enter 'ready' when you are : ");
				othello_ask_user_input(user_input,sizeof user_input);
				if(strcmp(user_input,"exit")==0){ /* player whant to leave the game */
					printf("Thanks for playing!");
					return 0;
				}
			}
			othello_shift_array(user_input,sizeof user_input);
			user_input[0] = OTHELLO_QUERY_READY;
			othello_write_mesg(socket_descriptor,user_input);
			server_answer = othello_read_mesg(socket_descriptor);
			if(strcmp(server_answer,"ok")!=0){
				printf("The server can't ready you, try again later...\n");
			}
		}

		/****************************/
		/**** WAIT FOR JOUEUR 2 *****/
		/****************************/
		while(strcmp(server_answer = othello_read_mesg(socket_descriptor), "j2ready") != 0){
			printf("En attente du joueur 2 ... ");
		}
	
		/**************************/
		/******* GAME START *******/
		/**************************/	
	}
	return 0;
}

bool othello_is_number(char* str){
	int i;
	for(i = 0; i < strlen(str); ++i){
		if(!(((int)(str[i]) > 47)&&((int)(str[i]) < 58))){
			return false;
		}
	}
	return true;
}

void othello_ask_user_input(char* usr_input,size_t input_size){
	char * buff_cleaner;
	memset(usr_input,0,input_size);
	fgets(usr_input,input_size,stdin);
	if((buff_cleaner = strchr(usr_input, '\n')) != NULL){*buff_cleaner = '\0';}
}

void othello_write_mesg(int sock_descr,char* mesg){
	if ((write(sock_descr, mesg, strlen(mesg))) < 0) {
		perror("erreur : impossible d'ecrire le message destine au serveur.");
		exit(1);
    }
}

char* othello_read_mesg(int sock){
	
	int i;
	int	last_byte_index;
	int bytes_read;
	char frag[OTHELLO_ANSWER_FRAGMENT_SIZE];
	char* complete_answer;

	i = 0;
	last_byte_index = 0;
	complete_answer = (char*)malloc(sizeof(char)*OTHELLO_ANSWER_TOTAL_SIZE);
	while((bytes_read = read(sock, frag, sizeof(frag))) > 0) {
		for(i = 0; i < bytes_read; ++i){
			complete_answer[last_byte_index] = frag[i];
			++last_byte_index;
		}
    }
	return complete_answer;
}

void othello_shift_array(char* arr,size_t arr_size){
	int i;
	for(i = strlen(arr); i > 0; --i){
		if(i < arr_size){
			arr[i] = arr[i-1];
		}
	}
}

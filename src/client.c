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
void othello_create_user_request(char*,size_t,othello_query_t);
void othello_read_user_input(char* , size_t);
void othello_write_mesg(int,char*);
void othello_read_mesg(int,char*,size_t);
void othello_shift_array(char*,size_t);
othello_status_t othello_choose_nickname(int);
othello_status_t othello_choose_room(int);
othello_status_t othello_ask_ready(int);
othello_status_t othello_other_ready(int);
othello_status_t othello_play_turn(int);
void othello_return_tokens(char*);
void othello_display_moves();
othello_status_t othello_send_move(int);

int main(int argc, char **argv) {
    int socket_descriptor; /* descripteur de socket */
	sockaddr_in adresse_locale; /* adresse de socket local */
    hostent* ptr_host; /* info sur une machine hote */
    servent* ptr_service; /* info sur service */

	int i; /* use for loops iterator */
	char* server_answer; /* fragments sum composing the final answer */
	char user_input[OTHELLO_USER_INPUT_SIZE]; /* gathers the user inputs */
	othello_status_t state;
   
	
	printf("Welcome, please enter the server adresse : ");

	othello_read_user_input(user_input,sizeof user_input);
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
	while((state = othello_choose_nickname(socket_descriptor)) != OTHELLO_SUCCESS){
		printf("Invalid nickname, try again : ");	
	}	

	/**********************************/
	/********** GAME ROUTINE **********/
	/**********************************/
	for(;;){
		
		/**********************************/
		/********** ROOM CHOICE ***********/
		/**********************************/
		while((state = othello_choose_room(socket_descriptor)) != OTHELLO_SUCCESS){
			printf("Invalid room ID, try again : ");	
		}

		/******************************/
		/***** ASK READY TO USER ******/
		/******************************/
		while((state = othello_ask_ready(socket_descriptor)) != OTHELLO_SUCCESS){
			printf("Server can't ready you, try again : ");	
		}

		/****************************/
		/**** WAIT FOR JOUEUR 2 *****/
		/****************************/
		while((state = othello_other_ready(socket_descriptor)) != OTHELLO_QUERY_READY){
			printf("Waiting for player 2 to be ready ... ");
		}
	
		/**************************/
		/******* GAME START *******/
		/**************************/
		while((state = othello_play_turn(socket_descriptor)) != OTHELLO_GAME_FINISH){
			printf("Player 2 is playing ... ");
		}
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

void othello_create_user_request(char* usr_input, size_t input_size, othello_query_t query){
	othello_shift_array(usr_input,sizeof usr_input);
	usr_input[0] = query;
}

void othello_read_user_input(char* usr_input, size_t input_size){
	char * buff_cleaner;
	fgets(usr_input,input_size,stdin);

	if(strcmp(usr_input,"exit")==0){
		printf("Thanks for playing ...\n");
		exit(1);
	}
	if((buff_cleaner = strchr(usr_input, '\n')) != NULL){*buff_cleaner = '\0';}
	free(buff_cleaner);
}

void othello_write_mesg(int sock_descr,char* mesg){
	if ((write(sock_descr, mesg, strlen(mesg))) < 0) {
		perror("erreur : impossible d'ecrire le message destine au serveur.");
		exit(1);
    }
}

void othello_read_mesg(int sock, char* buff,size_t bytes_to_read){
	int readed;
	char answer[bytes_to_read];
	if((readed = read(sock, answer, sizeof(answer))) != bytes_to_read){
		printf("Can't read the server answer");
		answer[0] = OTHELLO_ANSWER_READ_ERROR;
	}
	memset(buff,0,bytes_to_read);
	memcpy(buff,answer,bytes_to_read);
}

void othello_shift_array(char* arr,size_t arr_size){
	int i;
	for(i = strlen(arr); i > 0; --i){
		if(i < arr_size){
			arr[i] = arr[i-1];
		}
	}
}

othello_status_t othello_choose_nickname(int socket_descriptor){
	char user_input[9];
	char answer[1];

	printf("Choose a nickname : ");
	othello_read_user_input(user_input,sizeof user_input);
	othello_create_user_request(user_input,sizeof user_input,OTHELLO_QUERY_CONNECT);
	othello_write_mesg(socket_descriptor,user_input);
	othello_read_mesg(socket_descriptor,answer,sizeof answer);
	return answer[0];
}

othello_status_t othello_choose_room(int socket_descriptor){
	char user_input[4];
	char answer[OTHELLO_NUMBER_OF_ROOMS*32+1];

	printf("Type a room ID or 'list' to display the list of them : ");
	othello_read_user_input(user_input,sizeof user_input);
	if(othello_is_number(user_input)){
		othello_create_user_request(user_input,sizeof user_input,OTHELLO_QUERY_JOIN_ROOM);
	}else if(strcmp(user_input,"list")==0){
		othello_create_user_request(user_input,1,OTHELLO_QUERY_LIST_ROOM);
	}else{
		printf("Bad input");
		return OTHELLO_ANSWER_READ_ERROR;
	}
	othello_write_mesg(socket_descriptor,user_input);
	othello_read_mesg(socket_descriptor,answer,sizeof answer);
	if(answer[0]==OTHELLO_QUERY_LIST_ROOM){
		printf("Rooms list : ...\n");
		//call room list
	}
	return answer[0];
}

othello_status_t othello_ask_ready(int socket_descriptor){
	char user_input[6];
	char answer[1];
	othello_read_user_input(user_input,sizeof user_input);
	while(strcmp(user_input,"ready")!=0){
		printf("Invalid input, please enter 'ready' when you are : ");
		othello_read_user_input(user_input,sizeof user_input);
	}
	othello_create_user_request(user_input,1,OTHELLO_QUERY_READY);
	othello_write_mesg(socket_descriptor,user_input);
	othello_read_mesg(socket_descriptor,answer,sizeof answer);
	return answer[0];
}

othello_status_t othello_other_ready(int socket_descriptor){
	char answer[1];	
	othello_read_mesg(socket_descriptor,answer,sizeof answer);
	return answer[0];
}

othello_status_t othello_play_turn(int socket_descriptor){
	char answer[37]; /* maximum 18 tokens returned *2 for x/y coords + 1 for header */
	othello_status_t state;
	othello_read_mesg(socket_descriptor,answer,sizeof answer);
	if(answer[0] == OTHELLO_QUERY_PLAY_TURN){ /* player 2 just played */
		othello_return_tokens(answer);
		othello_display_moves();
		while((state = othello_send_move(socket_descriptor)) != OTHELLO_SUCCESS){
			printf("You can't play here according to the server ...\n");
		}
	}
	return answer[0];
}

void othello_return_tokens(char* moves){
	int i = 1;
	while(moves[i] != '\0'){
		/* table[moves[i]][moves[i+1]] = opposite color */
		i+=2;
	}
}

void othello_display_moves(){
	int i,j,cpt = 0;
	printf("Possible moves : \n");
	for (i = 0; i < OTHELLO_BOARD_LENGTH; ++i){
		for(j = 0; j < OTHELLO_BOARD_LENGTH; ++j){
			/* if is_valid(i,j) -> printf("Move number %d : %d / %d",cpt,i,j); ++cpt; */
		}
	}
}

othello_status_t othello_send_move(int socket_descriptor){
	char user_input[3];
	char answer[1];
	printf("Type your move : ");
	othello_read_user_input(user_input,sizeof user_input);
	while( ((int)user_input[0] < 65) || ((int)user_input[0] > 72) ||  ((int)user_input[0] < 49) || ((int)user_input[0] > 56) ){
		printf("The coordinate are out of board, please try again :");
		othello_read_user_input(user_input,sizeof user_input);
	}
	othello_create_user_request(user_input,sizeof user_input,OTHELLO_QUERY_PLAY_TURN);
	othello_write_mesg(socket_descriptor,user_input);
	othello_read_mesg(socket_descriptor,answer,sizeof answer);
	return answer[0];
}

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
#include "othello-client.h"

othello_client_state_t client_state;

int main(int argc, char **argv) {
    int socket_descriptor; /* socket descriptor */
	sockaddr_in adresse_locale; /* socket local adress */
    hostent* ptr_host; /* host machine informations */
    //servent* ptr_service; /* service informations */

	//char user_input[15]; /* gathers the user inputs */
	
	pthread_t thread_write;

	//othello_status_t state;
	//othello_notif_t notif;   

	while((ptr_host = othello_ask_server_adress()) == NULL){
		printf("Impossible to find a server at this adress, please try again :");
	}
  
    // copy char by char of informations from ptr_host to adresse_locale
    bcopy((char*)ptr_host->h_addr, (char*)&adresse_locale.sin_addr, ptr_host->h_length);
    adresse_locale.sin_family = AF_INET; // ou ptr_host->h_addrtype;

	adresse_locale.sin_port = htons(5000);

 	//socket creation
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("erreur : impossible de creer la socket de connexion avec le serveur.");
		exit(1);
    }

	//server connection try with informations onto adresse_locale
    if ((connect(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
		perror("erreur : impossible de se connecter au serveur.");
		exit(1);
    }
	
	printf("connexion succed ! \n");

	
	if(pthread_create(&thread_write, NULL, othello_write_thread, &socket_descriptor)) {
		//othello_log(LOG_ERR, "pthread_create");
		return EXIT_FAILURE;
    }


	/*
	while((state = othello_choose_nickname(socket_descriptor)) != OTHELLO_SUCCESS){
		printf("Invalid nickname, try again : ");	
	}	

	for(;;){
		

		while((state = othello_choose_room(socket_descriptor)) != OTHELLO_SUCCESS){
			printf("Invalid room ID, try again : ");	
		}


		while((state = othello_ask_ready(socket_descriptor)) != OTHELLO_SUCCESS){
			printf("Server can't ready you, try again : ");	
		}


		while((notif = othello_other_ready(socket_descriptor)) != OTHELLO_NOTIF_READY){
			printf("Waiting for player 2 to be ready ... ");
		}
	

		while((state = othello_play_turn(socket_descriptor)) != OTHELLO_GAME_FINISH){
			printf("Player 2 is playing ... ");
		}
	}*/
	(void) pthread_join(thread_write, NULL);
	close(socket_descriptor);

	printf("Thanks for playing, see you soon!");
	return 0;
}

hostent* othello_ask_server_adress(){
	char user_input[15];
	printf("Welcome, please enter the server adresse : ");
	if(othello_read_user_input(user_input,sizeof user_input)>0){exit(1);}
	return gethostbyname(user_input);
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

int othello_read_user_input(char* usr_input, size_t input_size){
	char * buff_cleaner;

	fgets(usr_input,input_size,stdin);
	if(strcmp(usr_input,"exit")==0){ /* on each input, user can enter 'exit' to quit the application */
		client_state = OTHELLO_CLIENT_STATE_EXIT;
		return 1;
	}
	if((buff_cleaner = strchr(usr_input, '\n')) != NULL){*buff_cleaner = '\0';} /* avoid a carriage return to stay in buffer and skip inputs later */
	free(buff_cleaner);
	return 0;
}

void othello_write_mesg(int sock_descr,char* mesg){
	if ((write(sock_descr, mesg, strlen(mesg))) < 0) {
		perror("Error : Impossible to write message to the server ...\n");
		exit(1);
    } 
}

void othello_read_mesg(int sock, char* buff,size_t bytes_to_read){
	int readed;
	char answer[bytes_to_read];
	if((readed = read(sock, answer, sizeof(answer))) != bytes_to_read){
		printf("Error : Can't read the server answer\n");
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

void othello_choose_nickname(int socket_descriptor){
	char user_input[9];
	//char answer[1];

	printf("Choose a nickname : ");
	if(othello_read_user_input(user_input,sizeof user_input)==0){
		othello_create_user_request(user_input,sizeof user_input,OTHELLO_QUERY_CONNECT);
		othello_write_mesg(socket_descriptor,user_input);
		//othello_read_mesg(socket_descriptor,answer,sizeof answer);
		//return answer[0];
		client_state = OTHELLO_CLIENT_STATE_WAITING;
		printf("Waiting for server answer ");
	}
}

void othello_choose_room(int socket_descriptor){
	char user_input[4];
	//char answer[OTHELLO_NUMBER_OF_ROOMS*32+1];

	printf("Type a room ID or 'list' to display the list of them : ");
	if(othello_read_user_input(user_input,sizeof user_input)==0){
		if(othello_is_number(user_input)){
			othello_create_user_request(user_input,sizeof user_input,OTHELLO_QUERY_ROOM_JOIN);
			othello_write_mesg(socket_descriptor,user_input);
			client_state = OTHELLO_CLIENT_STATE_WAITING;
			printf("Waiting for server answer ");
		}else if(strcmp(user_input,"list")==0){
			othello_create_user_request(user_input,1,OTHELLO_QUERY_ROOM_LIST);
			othello_write_mesg(socket_descriptor,user_input);
			client_state = OTHELLO_CLIENT_STATE_WAITING;
			printf("Waiting for server answer ");
		}else{
			printf("Invalid input !\n");
		}
		//othello_read_mesg(socket_descriptor,answer,sizeof answer);
		/*if(answer[0]==OTHELLO_QUERY_ROOM_LIST){
			printf("Rooms list : ...\n");
			//call room list
		}*/
	}
	//return answer[0];
}

void othello_send_ready(int socket_descriptor){
	char user_input[6];
	//char answer[1];
	if(othello_read_user_input(user_input,sizeof user_input)==0){
		if(strcmp(user_input,"ready")!=0){
			printf("Invalid input!\n");
		}else{
			othello_create_user_request(user_input,1,OTHELLO_QUERY_READY);
			othello_write_mesg(socket_descriptor,user_input);
		}
	}
	//othello_read_mesg(socket_descriptor,answer,sizeof answer);
	//return answer[0];
}

othello_notif_t othello_other_ready(int socket_descriptor){
	char answer[1];	
	othello_read_mesg(socket_descriptor,answer,sizeof answer);
	return answer[0];
}
/*
othello_status_t othello_play_turn(int socket_descriptor){
	char answer[37]; // maximum 18 tokens returned *2 for x/y coords + 1 for header
	othello_status_t state;
	othello_read_mesg(socket_descriptor,answer,sizeof answer);
	if(answer[0] == OTHELLO_NOTIF_PLAY){ // player 2 just played
		othello_return_tokens(answer);
		othello_display_moves();
		while((state = othello_send_move(socket_descriptor)) != OTHELLO_SUCCESS){
			printf("You can't play here according to the server ...\n");
		}
	}
	return answer[0];
}
*/
void othello_return_tokens(char* moves){
	int i = 1;
	while(moves[i] != '\0'){
		/* table[moves[i]][moves[i+1]] = opposite color */
		i+=2;
	}
}

void othello_display_moves(){
	int i,j = 0;
	//int i,j,cpt = 0;
	/* display board here ? */
	printf("Possible moves : \n");
	for (i = 0; i < OTHELLO_BOARD_LENGTH; ++i){
		for(j = 0; j < OTHELLO_BOARD_LENGTH; ++j){
			/* if is_valid(i,j) -> printf("Move number %d : %d / %d",cpt,i,j); ++cpt; */
		}
	}
}

void othello_send_move(int socket_descriptor){
	char user_input[3];
	//char answer[1];
	printf("Type your move : ");
	if(othello_read_user_input(user_input,sizeof user_input)==0){
		if( ((int)user_input[0] < 65) || ((int)user_input[0] > 72) ||  ((int)user_input[0] < 49) || ((int)user_input[0] > 56) ){ /* A1 to H8 */
			printf("The coordinate are out of board, please try again : ");
			//if(othello_read_user_input(user_input,sizeof user_input)>0){exit(1);}
		}else{
			othello_create_user_request(user_input,sizeof user_input,OTHELLO_QUERY_PLAY);
			othello_write_mesg(socket_descriptor,user_input);
		}
		//othello_read_mesg(socket_descriptor,answer,sizeof answer);
		//return answer[0];
	}
}


void* othello_write_thread(void* sock){
	pthread_t thread_read;	
	if(pthread_create(&thread_read, NULL, othello_read_thread, sock)) {
		//othello_log(LOG_ERR, "pthread_create");
		exit(1);
	}
	int socket_descriptor = *((int*)sock);

	while(client_state != OTHELLO_CLIENT_STATE_EXIT){
		switch(client_state){
			case OTHELLO_CLIENT_STATE_NICKNAME:
				othello_choose_nickname(socket_descriptor);
			break;
			case OTHELLO_CLIENT_STATE_CONNECTED:
				othello_choose_room(socket_descriptor);
			break;
			case OTHELLO_CLIENT_STATE_INROOM:
				othello_send_ready(socket_descriptor);
			break;
			case OTHELLO_CLIENT_STATE_READY:
				// messages?
			break;
			case OTHELLO_CLIENT_STATE_PLAYING:
				othello_send_move(socket_descriptor);
			break;
			case OTHELLO_CLIENT_STATE_WAITING:
				// messages?
				printf(".");
				sleep(1);
			break;
			default:
			break;
		}
	}
	(void) pthread_join(thread_read, NULL);
}

void* othello_read_thread(void* sock){
	int socket_desctiptor = *((int*)sock);
	for(;;){
		
	}
}

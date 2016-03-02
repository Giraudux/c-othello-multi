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
char othello_board[OTHELLO_BOARD_LENGTH][OTHELLO_BOARD_LENGTH];
char my_color;
char opponent_color;

int main(int argc, char **argv) {
    int socket_descriptor; /* socket descriptor */
	sockaddr_in adresse_locale; /* socket local adress */
    hostent* ptr_host; /* host machine informations */
    //servent* ptr_service; /* service informations */

	pthread_t thread_write; 

	while((ptr_host = othello_ask_server_adress()) == NULL){
		printf("Impossible to find a server at this adress, please try again :\n");
	}
  
    // copy char by char of informations from ptr_host to adresse_locale
    bcopy((char*)ptr_host->h_addr, (char*)&adresse_locale.sin_addr, ptr_host->h_length);
    adresse_locale.sin_family = AF_INET; // ou ptr_host->h_addrtype;

	adresse_locale.sin_port = htons(5000);

 	//socket creation
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("erreur : impossible de creer la socket de connexion avec le serveur.\n");
		exit(1);
    }

	//server connection try with informations onto adresse_locale
    if ((connect(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
		perror("erreur : impossible de se connecter au serveur.\n");
		exit(1);
    }
	
	printf("connexion succed ! \n");

	if(pthread_create(&thread_write, NULL, othello_write_thread, &socket_descriptor)) {
		//othello_log(LOG_ERR, "pthread_create");
		return EXIT_FAILURE;
    }

	(void) pthread_join(thread_write, NULL);
	close(socket_descriptor);

	printf("Thanks for playing, see you soon!\n");
	return 0;
}

hostent* othello_ask_server_adress(){
	char* user_input = "172.16.134.149";
	
	//printf("Welcome, please enter the server adresse : ");
	//if(othello_read_user_input(user_input,sizeof user_input)==0){
		return gethostbyname(user_input);
	//}
	//return NULL;
}

void othello_init_board(char color){
	int i,j;	
	my_color = color;
	for (i = 0; i < OTHELLO_BOARD_LENGTH; ++i){
		for(j = 0; j < OTHELLO_BOARD_LENGTH; ++j){
			othello_board[i][j] = '*';
		}
	}
	othello_board[OTHELLO_BOARD_LENGTH/2 - 1][OTHELLO_BOARD_LENGTH/2 - 1] = 'o';
	othello_board[OTHELLO_BOARD_LENGTH/2 - 1][OTHELLO_BOARD_LENGTH/2] = 'x';
	othello_board[OTHELLO_BOARD_LENGTH/2][OTHELLO_BOARD_LENGTH/2 - 1] = 'x';
	othello_board[OTHELLO_BOARD_LENGTH/2][OTHELLO_BOARD_LENGTH/2] = 'o';
}

void othello_display_board(char color){
	int i,j;

	printf("   ");
	for (i = 0; i < OTHELLO_BOARD_LENGTH; ++i){
		printf("%c",i+1);
	}
	printf("\n  _\n |");
	for (i = 0; i < OTHELLO_BOARD_LENGTH; ++i){
		printf("_");
	}
	for (i = 0; i < OTHELLO_BOARD_LENGTH; ++i){
		printf("\n%c| ",(char)(i+65));
		for(j = 0; j < OTHELLO_BOARD_LENGTH; ++j){
			printf("%c",othello_board[i][j]);
		}
	}
}

bool othello_is_number(char* str){
	int i;
	if(strlen(str) == 0){
		return false;
	}
	for(i = 0; i < strlen(str); ++i){
		if(!(((int)(str[i]) > 47)&&((int)(str[i]) < 58))){;
			return false;
		}
	}
	return true;
}

void othello_return_tokens(int x, int y, char color){
	int x_iter,y_iter;

	x_iter = x-1;
	y_iter = y;
	while((x_iter-1 >= 0) && othello_board[x_iter-1][y_iter] != color && othello_board[x_iter-1][y_iter] != '*'){
		--x_iter;
	}
	if(othello_board[x_iter-1][y_iter] == color){
		while(x_iter < x){
			othello_board[x_iter][y_iter] = color;
			++x_iter;
		}
	}

	x_iter = x;
	y_iter = y+1;
	while((y_iter+1 <= 7) && othello_board[x_iter][y_iter+1] != color && othello_board[x_iter][y_iter+1] != '*'){
		++y_iter;
	}
	if(othello_board[x_iter][y_iter+1] == color){
		while(y_iter > y){
			othello_board[x_iter][y_iter] = color;
			--y_iter;
		}
	}
	
	x_iter = x+1;
	y_iter = y;
	while((x_iter+1 <= 7) && othello_board[x_iter+1][y_iter] != color && othello_board[x_iter+1][y_iter] != '*'){
		++x_iter;
	}
	if(othello_board[x_iter+1][y_iter] == color){
		while(x_iter > x){
			othello_board[x_iter][y_iter] = color;
			--x_iter;
		}
	}

	x_iter = x;
	y_iter = y-1;
	while((y_iter-1 >= 0) && othello_board[x_iter][y_iter-1] != color && othello_board[x_iter][y_iter-1] != '*'){		
		--y_iter;
	}
	if(othello_board[x_iter][y_iter-1] == color){
		while(y_iter < y){
			othello_board[x_iter][y_iter] = color;
			++y_iter;
		}
	}
	
	x_iter = x-1;
	y_iter = y+1;
	while((x_iter-1 >= 0) && (y_iter+1 <= 7) && othello_board[x_iter-1][y_iter+1] != color && othello_board[x_iter-1][y_iter+1] != '*'){
		--x_iter;
		++y_iter;
	}
	if(othello_board[x_iter-1][y_iter+1] == color){
		while(x_iter < x){
			othello_board[x_iter][y_iter] = color;
			++x_iter;
			--y_iter;
		}
	}

	x_iter = x+1;
	y_iter = y+1;
	while((x_iter+1 <= 7) && (y_iter+1 <= 7) && othello_board[x_iter+1][y_iter+1] != color && othello_board[x_iter+1][y_iter+1] != '*'){
		++x_iter;
		++y_iter;
	}
	if(othello_board[x_iter+1][y_iter+1] == color){
		while(x_iter > x){
			othello_board[x_iter][y_iter] = color;
			--x_iter;
			--y_iter;
		}
	}

	x_iter = x+1;
	y_iter = y-1;
	while((x_iter+1 <= 7) && (y_iter-1 >= 0) && othello_board[x_iter+1][y_iter-1] != color && othello_board[x_iter+1][y_iter-1] != '*'){
		++x_iter;
		--y_iter;
	}
	if(othello_board[x_iter+1][y_iter-1] == color){
		while(x_iter > x){
			othello_board[x_iter][y_iter] = color;
			--x_iter;
			++y_iter;
		}
	}
	x_iter = x-1;
	y_iter = y-1;
	while((x_iter-1 >= 0) && (y_iter-1 >= 0) && othello_board[x_iter-1][y_iter-1] != color && othello_board[x_iter-1][y_iter-1] != '*'){
		--x_iter;
		--y_iter;
	}
	if(othello_board[x_iter-1][y_iter-1] == color){
		while(x_iter > x){
			othello_board[x_iter][y_iter] = color;
			++x_iter;
			++y_iter;
		}
	}
}

bool othello_move_valid(int x, int y, char color){
	int x_iter,y_iter;

	x_iter = x-1;
	y_iter = y;
	while((x_iter-1 >= 0) && othello_board[x_iter-1][y_iter] != color && othello_board[x_iter-1][y_iter] != '*'){
		--x_iter;
	}
	if(othello_board[x_iter-1][y_iter] == color){
		return true;
	}

	x_iter = x;
	y_iter = y+1;
	while((y_iter+1 <= 7) && othello_board[x_iter][y_iter+1] != color && othello_board[x_iter][y_iter+1] != '*'){
		++y_iter;
	}
	if(othello_board[x_iter][y_iter+1] == color){
		return true;
	}
	
	x_iter = x+1;
	y_iter = y;
	while((x_iter+1 <= 7) && othello_board[x_iter+1][y_iter] != color && othello_board[x_iter+1][y_iter] != '*'){
		++x_iter;
	}
	if(othello_board[x_iter+1][y_iter] == color){
		return true;
	}

	x_iter = x;
	y_iter = y-1;
	while((y_iter-1 >= 0) && othello_board[x_iter][y_iter-1] != color && othello_board[x_iter][y_iter-1] != '*'){		
		--y_iter;
	}
	if(othello_board[x_iter][y_iter-1] == color){
		return true;
	}
	
	x_iter = x-1;
	y_iter = y+1;
	while((x_iter-1 >= 0) && (y_iter+1 <= 7) && othello_board[x_iter-1][y_iter+1] != color && othello_board[x_iter-1][y_iter+1] != '*'){
		--x_iter;
		++y_iter;
	}
	if(othello_board[x_iter-1][y_iter+1] == color){
		return true;
	}

	x_iter = x+1;
	y_iter = y+1;
	while((x_iter+1 <= 7) && (y_iter+1 <= 7) && othello_board[x_iter+1][y_iter+1] != color && othello_board[x_iter+1][y_iter+1] != '*'){
		++x_iter;
		++y_iter;
	}
	if(othello_board[x_iter+1][y_iter+1] == color){
		return true;
	}

	x_iter = x+1;
	y_iter = y-1;
	while((x_iter+1 <= 7) && (y_iter-1 >= 0) && othello_board[x_iter+1][y_iter-1] != color && othello_board[x_iter+1][y_iter-1] != '*'){
		++x_iter;
		--y_iter;
	}
	if(othello_board[x_iter+1][y_iter-1] == color){
		return true;
	}
	x_iter = x-1;
	y_iter = y-1;
	while((x_iter-1 >= 0) && (y_iter-1 >= 0) && othello_board[x_iter-1][y_iter-1] != color && othello_board[x_iter-1][y_iter-1] != '*'){
		--x_iter;
		--y_iter;
	}
	if(othello_board[x_iter-1][y_iter-1] == color){
		return true;
	}
	return false;
}

void othello_create_user_request(char* usr_input, size_t input_size, othello_query_t query){
	othello_shift_array(usr_input,sizeof usr_input);
	usr_input[0] = query;
}

/*
int othello_read_user_input(char* usr_input, size_t input_size){
	char* cleaner;
	printf("ASKING USER INPUT HERE \n");
	fgets(usr_input,input_size,stdin);
	//removing '\n' added when user press ENTER to validate the input
	cleaner = usr_input;
	while(*cleaner!='\0'){
		if(*cleaner=='\n'){
			printf(" BACKSLASH FOUND ! \n");
			*cleaner='\0';
			break;
		}
		++cleaner;
	}

	if(strlen(usr_input) > 3){
		char test_msg[5];
		memcpy(test_msg,usr_input,4);
		test_msg[4] = '\0';
		if(strcmp(test_msg,"msg:")==0){ // the user want to write a message to his opponent
			client_state = OTHELLO_CLIENT_STATE_EXIT;
			return 1;
		}
	}
	if(strcmp(usr_input,"exit")==0){ // on each input, user can enter 'exit' to quit the application
		client_state = OTHELLO_CLIENT_STATE_EXIT;
		return 2;
	}
	
	return 0;
}*/

int othello_read_user_input(char* usr_input, size_t input_size){
	char* inputs = NULL;
	size_t len = 0;
	size_t read = 0;
	if ((read = getline(&inputs, &len, stdin)) == -1){
		printf("La defaite ... \n");
	}
	printf("line = %s\n",inputs);
	memcpy(usr_input,inputs,(len>input_size)?len:input_size);
	printf("line = %s\n",usr_input);
	usr_input[(len>input_size)?len-1:input_size-1] = '\0';
	if(inputs != NULL)	
		free(inputs);
}

void othello_write_mesg(int sock_descr,char* mesg,size_t msg_len){
	if ((write(sock_descr, mesg, msg_len)) < 0) {
		perror("Error : Impossible to write message to the server ...\n");
		exit(1);
    } 
}

void othello_read_mesg(int sock, char* buff,size_t bytes_to_read){
  ssize_t n;
	if((n = read(sock, buff, bytes_to_read)) != bytes_to_read){
		printf("Error : Can't read the server answer : n -> %zu / bytes -> %zu\n",n,bytes_to_read);
		
	}
}

void othello_shift_array(char* arr,size_t arr_size){
	char* mover;
	mover = arr + strlen(arr);
	if(strlen(arr) < arr_size-1){
		*(mover+1) = '\0';
	}else{
		--mover;
	}
	while(mover!=arr){
		*mover = *(mover-1);
		--mover;
	}
}

void othello_choose_nickname(int socket_descriptor){
	char user_input[33];
	int i;
	printf("Choose a nickname : \n");
	if(othello_read_user_input(user_input,sizeof user_input)==0){
		for(i=0; i < strlen(user_input);++i){
			printf("char %d : %c\n",i,user_input[i]);
		}
		othello_create_user_request(user_input,sizeof user_input,OTHELLO_QUERY_CONNECT);
		othello_write_mesg(socket_descriptor,user_input,sizeof user_input);
		client_state = OTHELLO_CLIENT_STATE_WAITING;
		printf("Waiting for server answer \n");
	}
}

void othello_choose_room(int socket_descriptor){
	char user_input[5];
	char room_list;
	char room_join[2];
	int i;
	printf("Type a room ID or 'list' to display the list of them : \n");
	if(othello_read_user_input(user_input,sizeof user_input)==0){
		if(othello_is_number(user_input)){
			room_join[0] = atoi(user_input);
			room_join[1] = ' ';
			othello_create_user_request(room_join, sizeof room_join, OTHELLO_QUERY_ROOM_JOIN);
			othello_write_mesg(socket_descriptor, room_join, sizeof room_join);
			client_state = OTHELLO_CLIENT_STATE_WAITING;
			printf("Waiting for server answer \n");
		}else if(strcmp(user_input,"list")==0){
			othello_create_user_request(&room_list, sizeof room_list, OTHELLO_QUERY_ROOM_LIST);
			othello_write_mesg(socket_descriptor, &room_list, sizeof room_list);
			client_state = OTHELLO_CLIENT_STATE_WAITING;
			printf("Waiting for server answer \n");
		}else{
			printf("Choose Room : Invalid input !\n");
		}
	}
}

void othello_send_ready(int socket_descriptor){
	char user_input[7];
	int read_result = othello_read_user_input(user_input,sizeof user_input);
	if(read_result==0){
		if(strcmp(user_input,"ready")!=0){
			printf("Invalid input!\n");
		}else{
			othello_create_user_request(user_input,1,OTHELLO_QUERY_READY);
			othello_write_mesg(socket_descriptor,user_input,sizeof user_input);
		}
	}else if(read_result==1){
		//othello_send_message
	}
}

void othello_display_moves(){
	int i,j = 0;
	printf("Possible moves : \n");
	for (i = 0; i < OTHELLO_BOARD_LENGTH; ++i){
		for(j = 0; j < OTHELLO_BOARD_LENGTH; ++j){
			if(othello_move_valid(i,j,my_color)){
				printf("(%c;%d) - ",(char)(i + 65),j+1);
			}
		}
	}
	printf("\n");
}

void othello_send_move(int socket_descriptor){
	char user_input[4];
	int read_result = othello_read_user_input(user_input,sizeof user_input);
	printf("Type your move : \n");
	if(read_result==0){
		if( ((int)user_input[0] < 65) || ((int)user_input[0] > 72) ||  ((int)user_input[1] < 49) || ((int)user_input[1] > 56) ){ /* A1 to H8 */
			printf("The coordinate are out of board, please try again : \n");
		}else{
			user_input[0] = (char)((int)user_input[0] - 17); // A -> 0, B -> 1, etc ...
			user_input[1] = (char)((int)user_input[1] - 1); // 1 -> 0, 2 -> 1, 3-> 2, etc...
			othello_create_user_request(user_input,sizeof user_input,OTHELLO_QUERY_PLAY);
			othello_write_mesg(socket_descriptor,user_input,sizeof user_input);
		}
	}else if(read_result==1){
		//othello_send_message
	}
}

void othello_send_message(int socket_descriptor, char* msg){
	char message[strlen(msg)-3];
	memcpy(&message[1],&msg[4],strlen(msg)-4);
	message[strlen(message)] = '\0';
	othello_create_user_request(message, sizeof message, OTHELLO_QUERY_MESSAGE);
	othello_write_mesg(socket_descriptor, message, sizeof message);
}

void othello_server_connect(int socket_descriptor){
	char server_answer;
	othello_read_mesg(socket_descriptor, &server_answer, sizeof(server_answer));
	if(server_answer == OTHELLO_SUCCESS){
		client_state = OTHELLO_CLIENT_STATE_CONNECTED;
		printf("You are now connected to the server\n");
	}else{
		printf("Invalid nickname ...\n");
		client_state = OTHELLO_CLIENT_STATE_NICKNAME;
	}
}
void othello_server_room_list(int socket_descriptor){
	char server_answer[(2 + OTHELLO_ROOM_LENGTH * OTHELLO_PLAYER_NAME_LENGTH) * OTHELLO_NUMBER_OF_ROOMS];
	othello_read_mesg(socket_descriptor,server_answer,sizeof(server_answer));
	printf("List of rooms :\n");
	client_state = OTHELLO_CLIENT_STATE_CONNECTED;
}
void othello_server_room_join(int socket_descriptor){
	char server_answer;
	othello_read_mesg(socket_descriptor,&server_answer,sizeof(server_answer));
	if(server_answer == OTHELLO_SUCCESS){
		client_state = OTHELLO_CLIENT_STATE_INROOM;
		printf("You are now into a room\n");
	}else{
		printf("Impossible to join the room ...\n");
		client_state = OTHELLO_CLIENT_STATE_CONNECTED;
	}
}
void othello_server_message(int socket_descriptor){
	printf("L'autre joueur vous envoie un message : ...\n");
}
void othello_server_ready(int socket_descriptor){
	char server_answer;
	othello_read_mesg(socket_descriptor,&server_answer,sizeof(server_answer));
	if(server_answer == OTHELLO_SUCCESS){
		client_state = OTHELLO_CLIENT_STATE_READY;
		printf("You are now ready to play\n");
	}else{
		OTHELLO_CLIENT_STATE_INROOM;
		printf("Server can't ready you ...\n");
	}
}
void othello_server_play(int socket_descriptor){
	char server_answer;
	othello_read_mesg(socket_descriptor,&server_answer,sizeof(server_answer));
	if(server_answer == OTHELLO_SUCCESS){
		client_state = OTHELLO_CLIENT_STATE_WAITING;
		printf("Votre coup a été validé\n");
		othello_notif_play(socket_descriptor,my_color);
	}else{
		client_state = OTHELLO_CLIENT_STATE_PLAYING;
		printf("Votre coup est invalide ...\n");
	}
}

void othello_notif_play(int socket_descriptor, char color){
	char server_answer[2];
	othello_read_mesg(socket_descriptor,server_answer,sizeof(server_answer));
	othello_board[server_answer[0]][server_answer[1]] = color;
	othello_return_tokens((int)(server_answer[0]-48),(int)(server_answer[1]-48),color);
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
				//display board
				//display possible moves
				othello_send_move(socket_descriptor);
			break;
			case OTHELLO_CLIENT_STATE_WAITING:
				// messages?

			break;
			default:
			break;
		}
	}
	(void) pthread_join(thread_read, NULL);
}

void* othello_read_thread(void* sock){
	int socket_descriptor = *((int*)sock);
	char server_answer_type;
	for(;;){
		othello_read_mesg(socket_descriptor, &server_answer_type, 1);
		switch(server_answer_type){
			case OTHELLO_QUERY_CONNECT:
				othello_server_connect(socket_descriptor);
			break;
			case OTHELLO_QUERY_ROOM_LIST:
				othello_server_room_list(socket_descriptor);
			break;
			case OTHELLO_QUERY_ROOM_JOIN:
				othello_server_room_join(socket_descriptor);
			break;
			case OTHELLO_QUERY_ROOM_LEAVE:
			break;
			case OTHELLO_QUERY_MESSAGE:
				othello_server_message(socket_descriptor);
			break;
			case OTHELLO_QUERY_READY:
				othello_server_ready(socket_descriptor);
			break;
			case OTHELLO_QUERY_PLAY:
				othello_server_play(socket_descriptor);
			break;
			case OTHELLO_NOTIF_PLAY:
				printf("Opponent just played\n");
				othello_notif_play(socket_descriptor,opponent_color);
			break;
			default:
			break;
		}
	}
}

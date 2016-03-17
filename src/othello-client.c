/*-----------------------------------------------------------
Client a lancer apres le serveur avec la commande :
client <adresse-serveur> <message-a-transmettre>
------------------------------------------------------------*/

#define _GNU_SOURCE

#include "othello.h"
#include "othello-client.h"

#include <stdlib.h>
#include <stdio.h>
#include <linux/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <stdbool.h>
#include <pthread.h>
#include <strings.h>
#include <unistd.h>

#define OTHELLO_DEFAULT_SERVER_ADRESS "78.226.157.77"

othello_client_enum_t client_state;
char othello_board[OTHELLO_BOARD_LENGTH][OTHELLO_BOARD_LENGTH];
char my_color;
char opponent_color;
unsigned char xMove;
unsigned char yMove;
bool auto_mode;


/************************************/
/********* TABLE FUNCTIONS **********/
/************************************/

void othello_init_board(){
	int i,j;
	auto_mode = false;	
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

void othello_display_board(){
	int i,j;

	printf("\n   ");
	for (i = 0; i < OTHELLO_BOARD_LENGTH; ++i){
		printf("%d ",i+1);
	}
	printf("\n   ");
	for (i = 0; i < OTHELLO_BOARD_LENGTH * 2 - 1; ++i){
		printf("-");
	}
	for (i = 0; i < OTHELLO_BOARD_LENGTH; ++i){
		printf("\n%c| ",(char)(i+65));
		for(j = 0; j < OTHELLO_BOARD_LENGTH; ++j){
			printf("%c ",othello_board[i][j]);
		}
	}
	printf("\n\n");
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

void othello_place_token(int socket_descriptor, char color){
	printf("New token added to the board in : (%c : %c)\n", xMove, yMove);
	othello_board[xMove][yMove] = color;
	othello_return_tokens(xMove, yMove, color);
}


void othello_return_tokens(int x, int y, char color){
	int x_iter,y_iter;

	x_iter = x;
	y_iter = y;
	while((x_iter-1 >= 0) && othello_board[x_iter-1][y_iter] != color && othello_board[x_iter-1][y_iter] != '*'){
		--x_iter;
	}
	if((x_iter-1 >= 0)){
		if(othello_board[x_iter-1][y_iter] == color){
			while(x_iter < x){
				othello_board[x_iter][y_iter] = color;
				++x_iter;
			}
		}
	}
	x_iter = x;
	y_iter = y;
	while((y_iter+1 <= 7) && othello_board[x_iter][y_iter+1] != color && othello_board[x_iter][y_iter+1] != '*'){
		++y_iter;
	}
	if((y_iter+1 <= 7)){
		if(othello_board[x_iter][y_iter+1] == color){
			while(y_iter > y){
				othello_board[x_iter][y_iter] = color;
				--y_iter;
			}
		}
	}
	x_iter = x;
	y_iter = y;
	while((x_iter+1 <= 7) && othello_board[x_iter+1][y_iter] != color && othello_board[x_iter+1][y_iter] != '*'){
		++x_iter;
	}
	if((x_iter+1 <= 7)){
		if(othello_board[x_iter+1][y_iter] == color){
			while(x_iter > x){
				othello_board[x_iter][y_iter] = color;
				--x_iter;
			}
		}
	}

	x_iter = x;
	y_iter = y;
	while((y_iter-1 >= 0) && othello_board[x_iter][y_iter-1] != color && othello_board[x_iter][y_iter-1] != '*'){		
		--y_iter;
	}
	if((y_iter-1 >= 0)){
		if(othello_board[x_iter][y_iter-1] == color){
			while(y_iter < y){
				othello_board[x_iter][y_iter] = color;
				++y_iter;
			}
		}
	}
	
	x_iter = x;
	y_iter = y;
	while((x_iter-1 >= 0) && (y_iter+1 <= 7) && othello_board[x_iter-1][y_iter+1] != color && othello_board[x_iter-1][y_iter+1] != '*'){
		--x_iter;
		++y_iter;
	}
	if((x_iter-1 >= 0) && (y_iter+1 <= 7)){
		if(othello_board[x_iter-1][y_iter+1] == color){
			while(x_iter < x){
				othello_board[x_iter][y_iter] = color;
				++x_iter;
				--y_iter;
			}
		}
	}
	x_iter = x;
	y_iter = y;
	while((x_iter+1 <= 7) && (y_iter+1 <= 7) && othello_board[x_iter+1][y_iter+1] != color && othello_board[x_iter+1][y_iter+1] != '*'){
		++x_iter;
		++y_iter;
	}
	if((x_iter+1 <= 7) && (y_iter+1 <= 7)){
		if(othello_board[x_iter+1][y_iter+1] == color){
			while(x_iter > x){
				othello_board[x_iter][y_iter] = color;
				--x_iter;
				--y_iter;
			}
		}
	}
	x_iter = x;
	y_iter = y;
	while((x_iter+1 <= 7) && (y_iter-1 >= 0) && othello_board[x_iter+1][y_iter-1] != color && othello_board[x_iter+1][y_iter-1] != '*'){
		++x_iter;
		--y_iter;
	}
	if((x_iter+1 <= 7) && (y_iter-1 >= 0)){
		if(othello_board[x_iter+1][y_iter-1] == color){
			while(x_iter > x){
				othello_board[x_iter][y_iter] = color;
				--x_iter;
				++y_iter;
			}
		}
	}
	x_iter = x;
	y_iter = y;
	while((x_iter-1 >= 0) && (y_iter-1 >= 0) && othello_board[x_iter-1][y_iter-1] != color && othello_board[x_iter-1][y_iter-1] != '*'){
		--x_iter;
		--y_iter;
	}
	if((x_iter-1 >= 0) && (y_iter-1 >= 0)){
		if(othello_board[x_iter-1][y_iter-1] == color){
			while(x_iter < x){
				othello_board[x_iter][y_iter] = color;
				++x_iter;
				++y_iter;
			}
		}
	}
}

int othello_move_valid(int x, int y, char color){
	int x_iter,y_iter,nb_returned,final_returned;

	if (othello_board[x][y] != '*')
		return 0;

	nb_returned = 0;
	final_returned = 0;

	x_iter = x; /* 6 */
	y_iter = y; /* 7 */
	while((x_iter-1 >= 0) && othello_board[x_iter-1][y_iter] != color && othello_board[x_iter-1][y_iter] != '*'){
		--x_iter;
		++nb_returned;
	}
	if((x_iter-1 >= 0)){
		if(othello_board[x_iter-1][y_iter] == color){
			final_returned += nb_returned;
		}
	}

	nb_returned = 0;
	x_iter = x;
	y_iter = y;
	while((y_iter+1 <= 7) && othello_board[x_iter][y_iter+1] != color && othello_board[x_iter][y_iter+1] != '*'){
		++y_iter;
		++nb_returned;
	}
	if((y_iter+1 <= 7)){
		if(othello_board[x_iter][y_iter+1] == color){
			final_returned += nb_returned;
		}
	}
	
	nb_returned = 0;
	x_iter = x;
	y_iter = y;
	while((x_iter+1 <= 7) && othello_board[x_iter+1][y_iter] != color && othello_board[x_iter+1][y_iter] != '*'){
		++x_iter;
		++nb_returned;
	}
	if((x_iter+1 <= 7)){
		if(othello_board[x_iter+1][y_iter] == color){
			final_returned += nb_returned;
		}
	}

	nb_returned = 0;
	x_iter = x;
	y_iter = y;
	while((y_iter-1 >= 0) && othello_board[x_iter][y_iter-1] != color && othello_board[x_iter][y_iter-1] != '*'){		
		--y_iter;
		++nb_returned;
	}
	if((y_iter-1 >= 0)){
		if(othello_board[x_iter][y_iter-1] == color){
			final_returned += nb_returned;
		}
	}
	
	nb_returned = 0;
	x_iter = x;
	y_iter = y;
	while((x_iter-1 >= 0) && (y_iter+1 <= 7) && othello_board[x_iter-1][y_iter+1] != color && othello_board[x_iter-1][y_iter+1] != '*'){
		--x_iter;
		++y_iter;
		++nb_returned;
	}
	if ((x_iter-1 >= 0) && (y_iter+1 <= 7)){
		if(othello_board[x_iter-1][y_iter+1] == color){
			final_returned += nb_returned;
		}
	}

	nb_returned = 0;
	x_iter = x;
	y_iter = y;
	while((x_iter+1 <= 7) && (y_iter+1 <= 7) && othello_board[x_iter+1][y_iter+1] != color && othello_board[x_iter+1][y_iter+1] != '*'){
		++x_iter;
		++y_iter;
		++nb_returned;
	}
	if ((x_iter+1 <= 7) && (y_iter+1 <= 7)){
		if(othello_board[x_iter+1][y_iter+1] == color){
			final_returned += nb_returned;
		}
	}

	nb_returned = 0;
	x_iter = x; /* 3 */
	y_iter = y; /* 7 */
	while((x_iter+1 <= 7) && (y_iter-1 >= 0) && othello_board[x_iter+1][y_iter-1] != color && othello_board[x_iter+1][y_iter-1] != '*'){
		++x_iter;
		--y_iter;
		++nb_returned;
	}
	if ((x_iter+1 <= 7) && (y_iter-1 >= 0)){
		if(othello_board[x_iter+1][y_iter-1] == color){
			final_returned += nb_returned;
		}
	}
	
	nb_returned = 0;
	x_iter = x;
	y_iter = y;
	while((x_iter-1 >= 0) && (y_iter-1 >= 0) && othello_board[x_iter-1][y_iter-1] != color && othello_board[x_iter-1][y_iter-1] != '*'){
		--x_iter;
		--y_iter;
		++nb_returned;
	}
	if ((x_iter-1 >= 0) && (y_iter-1 >= 0)){
		if(othello_board[x_iter-1][y_iter-1] == color){
			final_returned += nb_returned;
		}
	}
	return final_returned;
}

void othello_display_moves(){
	int i,j = 0;
	printf("Possible moves : \n");
	for (i = 0; i < OTHELLO_BOARD_LENGTH; ++i){
		for(j = 0; j < OTHELLO_BOARD_LENGTH; ++j){
			if(othello_move_valid(i,j,my_color) > 0){
				printf("(%c;%d) ",(char)(i + 65),j+1);
			}
		}
	}
	printf("\n");
}

void othello_calc_best_move(int* x_pos, int* y_pos){
	int i,j;
	int returned = 0;
	int max_returned = 0;
	for (i = 0; i < OTHELLO_BOARD_LENGTH; ++i){
		for(j = 0; j < OTHELLO_BOARD_LENGTH; ++j){
			if((returned = othello_move_valid(i,j,my_color)) > 0){
				if(returned > max_returned){
					*x_pos = i;
					*y_pos = j;
				}
			}
		}
	}
	printf("\n");
}

/************************************/
/***** INPUT/OUTPUT FUNCTIONS *******/
/************************************/

othello_client_enum_t othello_read_user_input(char** usr_input, size_t* input_len){
	char* stdin_value;
	char* clear_cr;
	char* realloc_input;
	size_t stdin_len;
	size_t stdin_real_len;
	size_t bytes_test;	

	*usr_input = NULL;
	stdin_value = NULL;

	if ((bytes_test = getline(&stdin_value, &stdin_len, stdin)) == -1){
		printf("Input readind failed ... \n");
	}else{
		clear_cr = stdin_value;
		while(*clear_cr != '\n'){ clear_cr += 1; } *clear_cr = '\0'; /* remove '\n' from buffer */

		stdin_real_len = strlen(stdin_value);

		if(stdin_real_len == 3){
			if(strncmp(stdin_value, "/ff", 3) == 0){
				input_len = 0;
				free(stdin_value);
				return OTHELLO_CLIENT_INPUT_GIVEUP; }
		}

		if(stdin_real_len > 4){
			if(stdin_real_len == 5){
				if(strncmp(stdin_value, "/list", 5) == 0){
					input_len = 0;
					free(stdin_value);
					return OTHELLO_CLIENT_INPUT_LIST; }
				if(strncmp(stdin_value, "/exit", 5) == 0){ 
					free(stdin_value);
					input_len = 0;
					return OTHELLO_CLIENT_INPUT_EXIT; }
				if(strncmp(stdin_value, "/auto", 5) == 0){ 
					free(stdin_value);
					input_len = 0;
					return OTHELLO_CLIENT_INPUT_AUTO; }
				if(strncmp(stdin_value, "/help", 5) == 0){ 
					free(stdin_value);
					input_len = 0;
					return OTHELLO_CLIENT_INPUT_HELP; }
			}

			if(stdin_real_len > 7){
				if(stdin_real_len == 8){
					if(strncmp(stdin_value, "/connect", 8) == 0){
						input_len = 0;
						free(stdin_value);
						return OTHELLO_CLIENT_INPUT_CONNECT_DEFAULT; }
				}else{
					if(stdin_real_len > 9){
						if(strncmp(stdin_value, "/connect", 8) == 0){
							*input_len = stdin_real_len - 9;
							if((realloc_input = (char*)realloc(*usr_input, (*input_len) * sizeof(char))) == NULL){
								printf("Error reallocating user_input\n");
								exit(1);
							}
							*usr_input = realloc_input;
							memcpy(*usr_input, stdin_value + 9, *input_len);
							free(stdin_value);
							return OTHELLO_CLIENT_INPUT_CONNECT;
						}
					}
				}
			}

			if(stdin_real_len > 5){
				if(strncmp(stdin_value, "/ready", 6) == 0){
					input_len = 0;
					free(stdin_value);
					return OTHELLO_CLIENT_INPUT_READY; }

				*input_len = stdin_real_len - 5;
				
				if((realloc_input = (char*)realloc(*usr_input, (*input_len) * sizeof(char))) == NULL){
					printf("Error reallocating user_input\n");
					exit(1);
				}
				*usr_input = realloc_input;
				memcpy(*usr_input, stdin_value + 5, *input_len);
				
				if(strncmp(stdin_value, "/play", 5) == 0){ free(stdin_value); return OTHELLO_CLIENT_INPUT_PLAY; }
				if(strncmp(stdin_value, "/mesg", 5) == 0){ free(stdin_value); return OTHELLO_CLIENT_INPUT_MESG; }
				if(strncmp(stdin_value, "/join", 5) == 0){ free(stdin_value); return OTHELLO_CLIENT_INPUT_JOIN; }
				if(strncmp(stdin_value, "/nick", 5) == 0){ free(stdin_value); return OTHELLO_CLIENT_INPUT_NICK; }
			}
			
			if(stdin_real_len > 8){
				if(strncmp(stdin_value, "/notready", 9) == 0){
					input_len = 0;
					free(stdin_value);
					return OTHELLO_CLIENT_INPUT_NOT_READY; }
			}
		}
	}
	printf("Unknow command ...\n");
	*usr_input = 0;
	*input_len = 0;
	free(stdin_value);
	return OTHELLO_CLIENT_INPUT_FAIL;
}

void othello_write_mesg(int sock_descr,char* mesg,size_t msg_len){
	if ((write(sock_descr, mesg, msg_len)) < 0) {
		perror("Error : Impossible to write message to the server ...\n");
		exit(1);
    } 
}

ssize_t othello_read_mesg(int sock, char* buff,size_t bytes_to_read){
	ssize_t n;
	if((n = read(sock, buff, bytes_to_read)) != bytes_to_read){
		printf("Error : Can't read the server answer!\n");	
	}
	return n;
}

void othello_display_help(){
	printf("HELP IS HERE !\n");
}

/************************************/
/**** SERVER REQUEST FUNCTIONS ******/
/************************************/

hostent* othello_ask_server_adress(){
	char* user_input = NULL;
	size_t input_len;
	othello_client_enum_t test_con;
	
	test_con = othello_read_user_input(&user_input, &input_len);


	if(test_con == OTHELLO_CLIENT_INPUT_CONNECT){
		return gethostbyname(user_input);
	}

	if(test_con == OTHELLO_CLIENT_INPUT_CONNECT_DEFAULT){
		return gethostbyname(OTHELLO_DEFAULT_SERVER_ADRESS);
	}
	
	printf("Bad input, please try again ...\n");
	return NULL;
}

void othello_choose_nickname(int socket_descriptor, char* usr_inpt, size_t inpt_len){
	char user_input[34];
	if (client_state == OTHELLO_CLIENT_STATE_NICKNAME){
		if(inpt_len > 1){
			memcpy(user_input+1, usr_inpt, (inpt_len<34)?inpt_len:33);
			if(inpt_len < 33){user_input[inpt_len] = '\0';}
			user_input[0] = OTHELLO_QUERY_LOGIN;
			user_input[1] = OTHELLO_PROTOCOL_VERSION;
			othello_write_mesg(socket_descriptor, user_input, sizeof user_input);
			client_state = OTHELLO_CLIENT_STATE_WAITING;
		}
	}else{
		printf("You can't choose a nickname now!\n");
	}
}


void othello_ask_list(int socket_descriptor){
	char user_input = OTHELLO_QUERY_ROOM_LIST;
	if (client_state == OTHELLO_CLIENT_STATE_CONNECTED){
		othello_write_mesg(socket_descriptor, &user_input, sizeof user_input);
	}else{
		printf("You can't ask the server rooms list now!\n");
	}
}

void othello_choose_room(int socket_descriptor, char* usr_inpt, size_t inpt_len){
	char user_input[2];
	
	if (client_state == OTHELLO_CLIENT_STATE_CONNECTED){
		if(inpt_len > 1){
			if(othello_is_number(usr_inpt + 1)){
				user_input[0] = OTHELLO_QUERY_ROOM_JOIN;
				user_input[1] = atoi(usr_inpt + 1);
				othello_write_mesg(socket_descriptor, user_input, sizeof user_input);
				client_state = OTHELLO_CLIENT_STATE_WAITING;
			}else{
				printf("No room ID doesn't exist!\n");
			}
		}else{
			printf("No room ID entered!\n");
		}
	}else{
		printf("You can't join a server room now!\n");
	}
}

void othello_send_ready(int socket_descriptor){
	char user_input = OTHELLO_QUERY_READY;	
	if (client_state == OTHELLO_CLIENT_STATE_INROOM){
		othello_write_mesg(socket_descriptor, &user_input, sizeof user_input);
		client_state = OTHELLO_CLIENT_STATE_WAITING;
	}else{
		printf("you can't send ready request now!\n");
	}
}

void othello_send_not_ready(int socket_descriptor){
	char user_input = OTHELLO_QUERY_NOT_READY;	
	if (client_state == OTHELLO_CLIENT_STATE_INROOM){
		othello_write_mesg(socket_descriptor, &user_input, sizeof user_input);
		client_state = OTHELLO_CLIENT_STATE_WAITING;
	}else{
		printf("you can't send unready request now!\n");
	}
}

void othello_send_move(int socket_descriptor, char* usr_inpt, size_t inpt_len){
	char user_input[3];
	if (client_state == OTHELLO_CLIENT_STATE_PLAYING){
		if(inpt_len == 3){
			if( ((int)usr_inpt[1] < 65) || ((int)usr_inpt[1] > 72) ||  ((int)usr_inpt[2] < 49) || ((int)usr_inpt[2] > 56) ){
				printf("The move coordinates are out of board, please try again : \n");
			}else{
				user_input[0] = OTHELLO_QUERY_PLAY;
				user_input[1] = (int)usr_inpt[1] - 65; /* A -> 0, B -> 1, C -> 2 etc ... */
				user_input[2] = (int)usr_inpt[2] - 49; /* 1 -> 0, 2 -> 1, 3 -> 2 etc ... */
				xMove = user_input[1];
				yMove = user_input[2];
				othello_write_mesg(socket_descriptor,user_input,sizeof user_input);
				client_state = OTHELLO_CLIENT_STATE_WAITING;
			}
		}else{
			printf("Then entered move is in invalid format!\n");
		}
	}else{
		printf("you can't send a move now!\n");
	}
}

void othello_send_auto_move(int socket_descriptor){
	int i=0, j=0;
	char user_input[3];
	othello_calc_best_move(&i, &j);
	user_input[0] = OTHELLO_QUERY_PLAY;
	user_input[1] = i;
	user_input[2] = j;
	xMove = i;
	xMove = j;
	printf("COMPUTER CHOOSED ( %d / %d ) MOVE !!\n",i,j);
	othello_write_mesg(socket_descriptor, user_input, sizeof user_input);
}

void othello_send_mesg(int socket_descriptor, char* usr_inpt, size_t inpt_len){
	char user_input[OTHELLO_MESSAGE_LENGTH + 1];
	if (client_state == OTHELLO_CLIENT_STATE_INROOM ||
		client_state == OTHELLO_CLIENT_STATE_READY ||
		client_state == OTHELLO_CLIENT_STATE_PLAYING ||
		client_state == OTHELLO_CLIENT_STATE_WAITING ){
		if(inpt_len > 1){
			memcpy(user_input, usr_inpt, (inpt_len<(OTHELLO_MESSAGE_LENGTH + 2))?inpt_len:(OTHELLO_MESSAGE_LENGTH + 1));
			if(inpt_len < (OTHELLO_MESSAGE_LENGTH + 1)){user_input[inpt_len] = '\0';}
			user_input[0] = OTHELLO_QUERY_MESSAGE;
			othello_write_mesg(socket_descriptor, user_input, sizeof user_input);
		}
	}else{
		printf("You can't send a message now:\n");
	}
}

void othello_send_giveup(int socket_descriptor){
	char user_input = OTHELLO_QUERY_GIVE_UP;	
	if (client_state == OTHELLO_CLIENT_STATE_PLAYING){
		othello_write_mesg(socket_descriptor, &user_input, sizeof user_input);
		client_state = OTHELLO_CLIENT_STATE_WAITING;
	}else{
		printf("you can't send forfeit now!\n");
	}
}

void othello_send_exit(socket_descriptor){
	char user_input = OTHELLO_QUERY_LOGOFF;
	othello_write_mesg(socket_descriptor, &user_input, sizeof user_input);
	client_state = OTHELLO_CLIENT_STATE_EXIT;
}

/************************************/
/***** SERVER ANSWER FUNCTIONS ******/
/************************************/

void othello_server_connect(int socket_descriptor){
	char server_answer;
	othello_read_mesg(socket_descriptor, &server_answer, sizeof(server_answer));
	if(server_answer == OTHELLO_SUCCESS){
		client_state = OTHELLO_CLIENT_STATE_CONNECTED;
		printf("You are now connected to the server\n");
		printf("You can display the server list with /list or join a room with /join\n");
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
		printf("Enter /ready whenever you are!\n");
	}else{
		printf("Impossible to join the room ...\n");
		client_state = OTHELLO_CLIENT_STATE_CONNECTED;
	}
}

void othello_server_message(int socket_descriptor){
	char server_answer;
	othello_read_mesg(socket_descriptor,&server_answer,sizeof(server_answer));
	if(server_answer == OTHELLO_SUCCESS){
		printf("Message succefully sent!\n");
	}else{
		printf("Impossible to send the message ...\n");
	}
}

void othello_server_ready(int socket_descriptor){
	char server_answer;
	othello_read_mesg(socket_descriptor,&server_answer,sizeof(server_answer));
	if(server_answer == OTHELLO_SUCCESS){
		client_state = OTHELLO_CLIENT_STATE_READY;
		printf("You are now ready to play\n");
		printf("Wait for server to say you when to play ...\n");
	}else{
		client_state = OTHELLO_CLIENT_STATE_INROOM;
		printf("Server can't ready you ...\n");
	}
}

void othello_server_not_ready(int socket_descriptor){
	char server_answer;
	othello_read_mesg(socket_descriptor,&server_answer,sizeof(server_answer));
	if(server_answer == OTHELLO_SUCCESS){
		client_state = OTHELLO_CLIENT_STATE_READY;
		printf("You are now unready to play\n");
		printf("Enter /ready whenever you are!\n");
	}else{
		client_state = OTHELLO_CLIENT_STATE_INROOM;
		printf("Server can't unready you ...\n");
	}
}

void othello_server_play(int socket_descriptor){
	char server_answer;
	othello_read_mesg(socket_descriptor,&server_answer,sizeof(server_answer));
	if(server_answer == OTHELLO_SUCCESS){
		client_state = OTHELLO_CLIENT_STATE_WAITING;
		printf("Votre coup a ete valide\n");
		othello_place_token(socket_descriptor, my_color);
	}else{
		client_state = OTHELLO_CLIENT_STATE_PLAYING;
		printf("Votre coup est invalide ...\nEnter a new one!\n");
	}
}

void othello_server_giveup(int socket_descriptor){
	char server_answer;
	othello_read_mesg(socket_descriptor,&server_answer,sizeof(server_answer));
	if(server_answer == OTHELLO_SUCCESS){
		client_state = OTHELLO_CLIENT_STATE_INROOM;
		printf("You gave up and lamentably lost!\n");
	}else{
		client_state = OTHELLO_CLIENT_STATE_PLAYING;
		printf("Server refused give up ...\n");
	}
}

void othello_notif_room_join(int socket_descriptor){
	char server_answer[32];
	othello_read_mesg(socket_descriptor, server_answer, sizeof(server_answer));
	printf("The player '%s' joined the room!\n",server_answer);
}

void othello_notif_room_leave(int socket_descriptor){
	char server_answer[32];
	othello_read_mesg(socket_descriptor, server_answer, sizeof(server_answer));
	printf("The player '%s' leaved the room!\n",server_answer);
}

void othello_notif_mesg(int socket_descriptor){
	char server_answer_user_name[32];
	char server_answer_message[OTHELLO_MESSAGE_LENGTH];
	othello_read_mesg(socket_descriptor, server_answer_user_name, sizeof(server_answer_user_name));
	othello_read_mesg(socket_descriptor, server_answer_message, sizeof(server_answer_message));
	printf("The player '%s' said : %s\n", server_answer_user_name, server_answer_message);
}

void othello_notif_ready(int socket_descriptor){
	char server_answer[32];
	othello_read_mesg(socket_descriptor, server_answer, sizeof(server_answer));
	printf("The player '%s' is ready!\n",server_answer);
}

void othello_notif_not_ready(int socket_descriptor){
	char server_answer[32];
	othello_read_mesg(socket_descriptor, server_answer, sizeof(server_answer));
	printf("The player '%s' isn't ready anymore!\n",server_answer);
}

void othello_notif_play(int socket_descriptor, char color){
	char server_answer[2];	
	printf("Opponent just played!\n");
	othello_read_mesg(socket_descriptor,server_answer,sizeof(server_answer));
	xMove = server_answer[0];
	yMove = server_answer[1];
	othello_place_token(socket_descriptor,color);
}

void othello_notif_your_turn(int socket_descriptor){
	client_state = OTHELLO_CLIENT_STATE_PLAYING;
	printf("This is your turn to play, enter a move:\n");
	othello_display_moves();
	othello_display_board();
	if(auto_mode)
		othello_send_auto_move(socket_descriptor);
}
void othello_notif_start(int socket_descriptor){
	char server_answer;
	othello_read_mesg(socket_descriptor,&server_answer,sizeof(server_answer));
	if(server_answer){
		my_color = othello_board[OTHELLO_BOARD_LENGTH/2-1][OTHELLO_BOARD_LENGTH/2];
		opponent_color = othello_board[OTHELLO_BOARD_LENGTH/2-1][OTHELLO_BOARD_LENGTH/2-1];
		client_state = OTHELLO_CLIENT_STATE_PLAYING;
		printf("Your play with '%c' tokens!\n",my_color);
		printf("You start, enter your move:\n");
		othello_display_moves();
		if(auto_mode)
			othello_send_auto_move(socket_descriptor);
	}else{
		my_color = othello_board[OTHELLO_BOARD_LENGTH/2-1][OTHELLO_BOARD_LENGTH/2-1];
		opponent_color = othello_board[OTHELLO_BOARD_LENGTH/2-1][OTHELLO_BOARD_LENGTH/2];
		client_state = OTHELLO_CLIENT_STATE_WAITING;
		printf("Your play with '%c' tokens!\n",my_color);
		printf("Opponent play first, please wait ...\n");
	}
	othello_display_board();
}

void othello_notif_giveup(int socket_descriptor){
	char server_answer[32];
	othello_read_mesg(socket_descriptor, server_answer, sizeof(server_answer));
	printf("The player '%s' gived up! You won!\n",server_answer);
	client_state = OTHELLO_CLIENT_STATE_INROOM;
}

void othello_notif_end(int socket_descriptor){
	char server_answer;
	othello_read_mesg(socket_descriptor,&server_answer,sizeof(server_answer));
	othello_display_board();
	if(server_answer){
		printf("Game ended, you won!\n");
	}else{
		printf("Game ended, you lost!\n");
	}
	client_state = OTHELLO_CLIENT_STATE_CONNECTED;
}

/************************************/
/******** THREAD FUNCTIONS **********/
/************************************/

void* othello_write_thread(void* sock){
	pthread_t thread_read;	
	int socket_descriptor = *((int*)sock);
	char* usr_input = NULL;
	size_t input_len;
	char input_type;

	if(pthread_create(&thread_read, NULL, othello_read_thread, sock)) {
		exit(1);
	}
	othello_init_board();
	printf("You can now enter your nickname :\n");	

	while(client_state != OTHELLO_CLIENT_STATE_EXIT){
	
		if(!auto_mode){
			input_type = othello_read_user_input(&usr_input, &input_len);

			switch(input_type){
				case OTHELLO_CLIENT_INPUT_NICK:
					othello_choose_nickname(socket_descriptor, usr_input, input_len);
				break;
				case OTHELLO_CLIENT_INPUT_LIST:
					othello_ask_list(socket_descriptor);
				break;
				case OTHELLO_CLIENT_INPUT_JOIN:
					othello_choose_room(socket_descriptor, usr_input, input_len);
				break;
				case OTHELLO_CLIENT_INPUT_READY:
					othello_send_ready(socket_descriptor);
				break;
				case OTHELLO_CLIENT_INPUT_NOT_READY:
					othello_send_not_ready(socket_descriptor);
				break;
				case OTHELLO_CLIENT_INPUT_HELP:
					othello_display_help();
				case OTHELLO_CLIENT_INPUT_PLAY:
					othello_send_move(socket_descriptor, usr_input, input_len);
				break;
				case OTHELLO_CLIENT_INPUT_GIVEUP:
					othello_send_giveup(socket_descriptor);
				break;
				case OTHELLO_CLIENT_INPUT_MESG:
					othello_send_mesg(socket_descriptor, usr_input, input_len);
				break;
				case OTHELLO_CLIENT_INPUT_AUTO:
					auto_mode = !auto_mode;
					if(client_state == OTHELLO_CLIENT_STATE_PLAYING)
						othello_send_auto_move(socket_descriptor);
				break;
				case OTHELLO_CLIENT_INPUT_EXIT:
					othello_send_exit(socket_descriptor);
				break;
			}
		}
	}
	free(usr_input);
	(void) pthread_join(thread_read, NULL);
	return NULL;
}

void* othello_read_thread(void* sock){
	int socket_descriptor = *((int*)sock);
	char server_answer_type;
	while(othello_read_mesg(socket_descriptor, &server_answer_type, 1) > 0){
		switch(server_answer_type){
			case OTHELLO_QUERY_LOGIN:
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
			case OTHELLO_QUERY_NOT_READY:
				othello_server_not_ready(socket_descriptor);
			break;
			case OTHELLO_QUERY_PLAY:
				othello_server_play(socket_descriptor);
			break;
			case OTHELLO_QUERY_GIVE_UP:
				othello_server_giveup(socket_descriptor);
			break;


			case OTHELLO_NOTIF_ROOM_JOIN:
				othello_notif_room_join(socket_descriptor);
			break;
			case OTHELLO_NOTIF_ROOM_LEAVE:
				auto_mode = false;
				othello_notif_room_leave(socket_descriptor);
			break;
			case OTHELLO_NOTIF_MESSAGE:
				othello_notif_mesg(socket_descriptor);
			break;
			case OTHELLO_NOTIF_READY:
				othello_notif_ready(socket_descriptor);
			break;
			case OTHELLO_NOTIF_NOT_READY:
				othello_notif_not_ready(socket_descriptor);
			break;
			case OTHELLO_NOTIF_PLAY:
				othello_notif_play(socket_descriptor,opponent_color);
			break;
			case OTHELLO_NOTIF_YOUR_TURN:
				othello_notif_your_turn(socket_descriptor);
			break;
			case OTHELLO_NOTIF_GAME_START:
				othello_notif_start(socket_descriptor);
			break;
			case OTHELLO_NOTIF_GIVE_UP:
				othello_notif_giveup(socket_descriptor);
			break;
			case OTHELLO_NOTIF_GAME_END:
				auto_mode = false;
				othello_notif_end(socket_descriptor);
			break;
			default:
			break;
		}
	}
	return NULL;
}

/************************************/
/*************** MAIN ***************/
/************************************/

int main(int argc, char **argv) {
    int socket_descriptor; /* socket descriptor */
	sockaddr_in adresse_locale; /* socket local adress */
    hostent* ptr_host; /* host machine informations */
    /*servent* ptr_service;*/ /* service informations */

	pthread_t thread_write; 
	printf("Welcome, pls type /connect xxx.xxx.xxx.xxx (server_adress) :\n");
	while((ptr_host = othello_ask_server_adress()) == NULL){
		printf("Impossible to find a server at this adress, please try again :\n");
	}
  	client_state = OTHELLO_CLIENT_STATE_NICKNAME;
    /* copy char by char of informations from ptr_host to adresse_locale */
    bcopy((char*)ptr_host->h_addr, (char*)&adresse_locale.sin_addr, ptr_host->h_length);
    adresse_locale.sin_family = AF_INET; /* ou ptr_host->h_addrtype; */

	adresse_locale.sin_port = htons(5000);

 	/*socket creation*/
    if ((socket_descriptor = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("erreur : impossible de creer la socket de connexion avec le serveur.\n");
		exit(1);
    }

	/*server connection try with informations onto adresse_locale*/
    if ((connect(socket_descriptor, (sockaddr*)(&adresse_locale), sizeof(adresse_locale))) < 0) {
		perror("erreur : impossible de se connecter au serveur.\n");
		exit(1);
    }
	
	printf("connexion succed ! \n");

	if(pthread_create(&thread_write, NULL, othello_write_thread, &socket_descriptor)) {
		/*othello_log(LOG_ERR, "pthread_create");*/
		return EXIT_FAILURE;
    }

	(void) pthread_join(thread_write, NULL);
	close(socket_descriptor);

	printf("Thanks for playing, see you soon!\n");
	return 0;
}

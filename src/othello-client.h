#ifndef OTHELLO_CLIENT_H
#define OTHELLO_CLIENT_H

typedef struct sockaddr 	sockaddr;
typedef struct sockaddr_in 	sockaddr_in;
typedef struct hostent 		hostent;
typedef struct servent 		servent;

enum othello_client_state_e{
	OTHELLO_CLIENT_STATE_NICKNAME,
	OTHELLO_CLIENT_STATE_CONNECTED,
	OTHELLO_CLIENT_STATE_INROOM,
	OTHELLO_CLIENT_STATE_READY,
	OTHELLO_CLIENT_STATE_PLAYING,
	OTHELLO_CLIENT_STATE_WAITING,
	OTHELLO_CLIENT_STATE_EXIT
};

typedef enum othello_client_state_e othello_client_state_t;

hostent* othello_ask_server_adress();
void othello_init_board(char);
void othello_display_board(char);
/* return if yes or not a char* can be converted into a number */
bool othello_is_number(char*);
void othello_return_tokens(int,int,char);
bool othello_move_valid(int,int,char);
/* shift a char* to the righ and and the query_t in parameter onto indexe 0 */
void othello_create_user_request(char*,size_t,othello_query_t);
/* get a user input into the char* in paramter */
int othello_read_user_input(char* , size_t);
/* send the char* to the server using the socket in paramter */
void othello_write_mesg(int,char*,size_t);
/* read the size_t first bytes of a server answer using the socket in paramter */
void othello_read_mesg(int,char*,size_t);
/* shift all char of a char* to the right */
void othello_shift_array(char*,size_t);
/* ask user a nickname and return the server status answer */
void othello_choose_nickname(int);
/* ask user to choose a room and return the server status answer */
void othello_choose_room(int);
/* ask user to put himself ready and return the server status answer */
void othello_send_ready(int);
/* display the list of possible moves to the user */
void othello_display_moves();
/* send a move to the server and return if the move is valid or not */
void othello_send_move(int);
void othello_send_message(int,char*);
void othello_server_connect(int socket_descriptor);
void othello_server_room_list(int socket_descriptor);
void othello_server_room_join(int socket_descriptor);
void othello_server_message(int socket_descriptor);
void othello_server_ready(int socket_descriptor);
void othello_server_play(int socket_descriptor);
void othello_notif_play(int socket_descriptor,char);
/* read all users input until he enter exit and execute the corresponding function according to his state */
void* othello_write_thread(void*);
/* read all server answers */
void* othello_read_thread(void*);


int main(int argc, char **argv);

#endif

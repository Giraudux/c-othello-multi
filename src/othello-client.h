#ifndef OTHELLO_CLIENT_H
#define OTHELLO_CLIENT_H

#include <stdbool.h>
#include <sys/types.h>

typedef struct sockaddr 	sockaddr;
typedef struct sockaddr_in 	sockaddr_in;
typedef struct hostent 		hostent;
typedef struct servent 		servent;

enum othello_client_enum_e{
	OTHELLO_CLIENT_STATE_NICKNAME,
	OTHELLO_CLIENT_STATE_CONNECTED,
	OTHELLO_CLIENT_STATE_INROOM,
	OTHELLO_CLIENT_STATE_READY,
	OTHELLO_CLIENT_STATE_PLAYING,
	OTHELLO_CLIENT_STATE_WAITING,
	OTHELLO_CLIENT_STATE_EXIT,

	OTHELLO_CLIENT_INPUT_CONNECT,
	OTHELLO_CLIENT_INPUT_CONNECT_DEFAULT,
	OTHELLO_CLIENT_INPUT_NICK,
	OTHELLO_CLIENT_INPUT_LIST,
	OTHELLO_CLIENT_INPUT_JOIN,
	OTHELLO_CLIENT_INPUT_READY,
	OTHELLO_CLIENT_INPUT_NOT_READY,
	OTHELLO_CLIENT_INPUT_PLAY,
	OTHELLO_CLIENT_INPUT_MESG,
	OTHELLO_CLIENT_INPUT_AUTO,
	OTHELLO_CLIENT_INPUT_HELP,
	OTHELLO_CLIENT_INPUT_GIVEUP,	
	OTHELLO_CLIENT_INPUT_EXIT,
	OTHELLO_CLIENT_INPUT_FAIL
};

typedef enum othello_client_enum_e othello_client_enum_t;

hostent* othello_ask_server_adress();
void othello_init_board();
void othello_display_board();
/* return if yes or not a char* can be converted into a number */
bool othello_is_number(char*);
void othello_return_tokens(int,int,char);
void othello_place_token(int,char);
int othello_move_valid(int,int,char);
/* get a user input into the char* in paramter */
othello_client_enum_t othello_read_user_input(char**, size_t*);
/* send the char* to the server using the socket in paramter */
void othello_write_mesg(int,char*,size_t);
/* read the size_t first bytes of a server answer using the socket in paramter */
ssize_t othello_read_mesg(int,char*,size_t);
/* shift all char of a char* to the right */
void othello_shift_array(char*,size_t);
/* ask user a nickname and return the server status answer */
void othello_choose_nickname(int, char*, size_t);
void othello_ask_list(int);
void othello_choose_room(int, char*, size_t);
void othello_send_ready(int);
void othello_send_not_ready(int);
void othello_send_move(int, char*, size_t);
void othello_send_auto_move(int);
void othello_send_mesg(int, char*, size_t);
void othello_send_giveup(int);
void othello_send_exit(int);
/* display the list of possible moves to the user */
void othello_display_moves();
void othello_display_help();
void othello_calc_best_move(int*, int*);

void othello_server_message(int);
void othello_server_ready(int);
void othello_server_not_ready(int);
void othello_server_play(int);
void othello_server_giveup(int);
void othello_notif_room_join(int);
void othello_notif_room_leave(int);
void othello_notif_mesg(int);
void othello_notif_ready(int);
void othello_notif_not_ready(int);
void othello_notif_play(int,char);
void othello_notif_your_turn(int);
void othello_notif_start(int);
void othello_notif_giveup(int);
void othello_notif_end(int);
/* read all users input until he enter exit and execute the corresponding function according to his state */
void* othello_write_thread(void*);
/* read all server answers */
void* othello_read_thread(void*);

int main(int argc, char **argv);

#endif

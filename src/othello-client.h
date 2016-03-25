#ifndef OTHELLO_CLIENT_H
#define OTHELLO_CLIENT_H

#include <stdbool.h>
#include <sys/types.h>

typedef struct sockaddr sockaddr;
typedef struct sockaddr_in sockaddr_in;
typedef struct hostent hostent;
typedef struct servent servent;

enum othello_client_enum_e {
  OTHELLO_CLIENT_STATE_NICKNAME,  /* user is choosing his nickname */
  OTHELLO_CLIENT_STATE_CONNECTED, /* user is logged but not into any room */
  OTHELLO_CLIENT_STATE_INROOM,    /* user is in a room */
  OTHELLO_CLIENT_STATE_READY,     /* user is ready to play */
  OTHELLO_CLIENT_STATE_PLAYING,   /* user turn to play */
  OTHELLO_CLIENT_STATE_WAITING,   /* user waiting for a server answer */
  OTHELLO_CLIENT_STATE_EXIT,      /* user is trying to exit the app */

  /* all different types of user possible inputs */
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
  OTHELLO_CLIENT_INPUT_LEAVE,
  OTHELLO_CLIENT_INPUT_EXIT,
  OTHELLO_CLIENT_INPUT_FAIL
};

typedef enum othello_client_enum_e othello_client_enum_t;

/************************************/
/********* TABLE FUNCTIONS **********/
/************************************/
/* set all the global variables to a default value */
void othello_init_board();
/* display the board matrice in the stdin output */
void othello_display_board();
/* return if yes or not a char* can be converted into a number */
bool othello_is_number(char *);
/* return all tokens affected by the new token just placed */
void othello_return_tokens(int, int, char);
/* place a token on the board and call othello_return_tokens */
void othello_place_token(int, char);
/* return if a move is valid or not */
int othello_move_valid(int, int, char);
/* display the list of possible moves to the user */
void othello_display_moves();
/* used for the AI, calculating the move return the higher number of tokens */
void othello_calc_best_move(int *, int *);

/************************************/
/***** INPUT/OUTPUT FUNCTIONS *******/
/************************************/
/* get a user input into the char* in paramter */
othello_client_enum_t othello_read_user_input(char **, size_t *);
/* send the char* to the server using the socket in paramter */
void othello_write_mesg(int, char *, size_t);
/* read the size_t first bytes of a server answer using the socket in paramter
 */
ssize_t othello_read_mesg(int, char *, size_t);
/* display the list of user commands */
void othello_display_help();

/************************************/
/**** SERVER REQUEST FUNCTIONS ******/
/************************************/
hostent *othello_ask_server_adress();
/* ask user a nickname and return the server status answer */
void othello_choose_nickname(int, char *, size_t);
/* request the rooms list to the server */
void othello_ask_list(int);
/* try to connect the user into a room */
void othello_choose_room(int, char *, size_t);
/* try to leave the user current room */
void othello_send_room_leave(int);
/* try to put the user into a ready state (ready to play) */
void othello_send_ready(int);
/* try to put the user into an unready state */
void othello_send_not_ready(int);
/* try to send the user move to the server for validation */
void othello_send_move(int, char *, size_t);
/* move automatically send by the AI */
void othello_send_auto_move(int);
/* try to send a message to the opponent */
void othello_send_mesg(int, char *, size_t);
/* try to forfeit the game */
void othello_send_giveup(int);
/* call the app exit */
void othello_send_exit(int);

/************************************/
/***** SERVER ANSWER FUNCTIONS ******/
/************************************/
/* server is answering if yes or not the user succed to connect to the server */
void othello_server_connect(int);
void othello_server_room_list(int);
/* server is answering if yes or not the user succed to join a room */
void othello_server_room_join(int);
/* server is answering if yes or not the user succed to leave a room */
void othello_server_room_leave(int);
/* server is answering if yes or not the user succed to send a message */
void othello_server_message(int);
/* server is answering if yes or not the user is allowed to be ready */
void othello_server_ready(int);
/* server is answering if yes or not the user is allowed to be unready */
void othello_server_not_ready(int);
/* server is answering if yes or not the user move is valid */
void othello_server_play(int);
/* server is answering if yes or not the user allowed to forfeit */
void othello_server_giveup(int);

/* notif the user that the opponent join his room */
void othello_notif_room_join(int);
/* notif the user that the opponent left his room */
void othello_notif_room_leave(int);
/* display the opponent message */
void othello_notif_mesg(int);
/* notif the user that the opponent is ready */
void othello_notif_ready(int);
/* notif the user that the opponent isn't ready */
void othello_notif_not_ready(int);
/* notif the user that the opponent just played (with the opponent move) */
void othello_notif_play(int, char);
/* notif the user that it's his turn to play */
void othello_notif_your_turn(int);
/* notif the user that he is starting */
void othello_notif_start(int);
/* notif the user that the opponent gave up */
void othello_notif_giveup(int);
/* notif the user that the game ends */
void othello_notif_end(int);

/************************************/
/******** THREAD FUNCTIONS **********/
/************************************/
/* read all users input until he enter exit and execute the corresponding
 * function according to his state */
void *othello_write_thread(void *);
/* read all server answers */
void *othello_read_thread(void *);

/************************************/
/*************** MAIN ***************/
/************************************/
int main(int argc, char **argv);

#endif

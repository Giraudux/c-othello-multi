/**
 * \author Alexis Giraudet
 */

#ifndef OTHELLO_SERVER_H
#define OTHELLO_SERVER_H

#include <stdbool.h>
#include <sys/types.h>

struct othello_player_s;
struct othello_room_s;

typedef struct othello_player_s othello_player_t;
typedef struct othello_room_s othello_room_t;

/**
 * create a IPv4 TCP socket
 * \param port port to listen
 */
int othello_create_socket_stream(unsigned short port);

/**
 * read data to a given file descriptor until error or buffer filled
 * \param fd file descriptor to read
 * \param buf buffer to fill
 * \param count count of data to read
 */
ssize_t othello_read_all(int fd, void *buf, size_t count);

/**
 * write data to a given file descriptor until error or buffer writed
 * \param fd file descriptor to write
 * \param buf buffer to write
 * \param count count of data to write
 */
ssize_t othello_write_all(int fd, void *buf, size_t count);

/**
 * log a message to standard output or to syslog
 * \param priority message priority (see syslog message level)
 * \param format message to log
 */
void othello_log(int priority, const char *format, ...);

/**
 * cleanup function
 */
void othello_exit(void);

/**
 * daemonize the program
 */
void othello_daemonize(void);

/**
 * print help on standard output
 */
void othello_print_help(void);

/**
 * start to handle player queries
 * \param player current player
 */
void *othello_player_start(void *player);

/**
 * cleanup function
 * \param player current player
 */
void othello_player_end(othello_player_t *player);

/**
 * log the player in the server
 * \param player current player
 */
othello_status_t othello_handle_login(othello_player_t *player);

/**
 * send the list of rooms
 * \param player current player
 */
othello_status_t othello_handle_room_list(othello_player_t *player);

/**
 * put the player in a room
 * \param player current player
 */
othello_status_t othello_handle_room_join(othello_player_t *player);

/**
 * remove the player to the room
 * \param player current player
 */
othello_status_t othello_handle_room_leave(othello_player_t *player);

/**
 * receive the player message and send it
 * \param player current player
 */
othello_status_t othello_handle_message(othello_player_t *player);

/**
 * set the player ready
 * \param player current player
 */
othello_status_t othello_handle_ready(othello_player_t *player);

/**
 * set the player not ready
 * \param player current player
 */
othello_status_t othello_handle_not_ready(othello_player_t *player);

/**
 * play the player stroke
 * \param player current player
 */
othello_status_t othello_handle_play(othello_player_t *player);

/**
 * manage player give up
 * \param player current player
 */
othello_status_t othello_handle_give_up(othello_player_t *player);

/**
 * compute the score of the player
 * \param player current player
 */
int othello_game_score(othello_player_t *player);

/**
 * check if the player is able to play
 * \param player current player
 */
bool othello_game_able_to_play(othello_player_t *player);

/**
 * valid the stroke of the player
 * \param player current player
 * \return OTHELLO_SUCCESS if the stroke is valid
 */
othello_status_t othello_game_play_stroke(othello_player_t *player,
                                          unsigned char x, unsigned char y);

/**
 * check if the player stroke is valid
 * \param player current player
 * \return 0 if the stroke is not valid
 */
int othello_game_is_stroke_valid(othello_player_t *player, unsigned char x,
                                 unsigned char y);

/**
 * main
 */
int main(int argc, char *argv[]);

#endif

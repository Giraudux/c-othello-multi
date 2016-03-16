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

int othello_create_socket_stream(unsigned short port);
ssize_t othello_read_all(int fd, void *buf, size_t count);
ssize_t othello_write_all(int fd, void *buf, size_t count);
void othello_log(int priority, const char *format, ...);
void othello_exit(void);
void othello_daemonize(void);
void othello_print_help(void);

void *othello_player_start(void *player);
void othello_player_end(othello_player_t *player);

othello_status_t othello_handle_login(othello_player_t *player);
othello_status_t othello_handle_room_list(othello_player_t *player);
othello_status_t othello_handle_room_join(othello_player_t *player);
othello_status_t othello_handle_room_leave(othello_player_t *player);
othello_status_t othello_handle_message(othello_player_t *player);
othello_status_t othello_handle_ready(othello_player_t *player);
othello_status_t othello_handle_not_ready(othello_player_t *player);
othello_status_t othello_handle_play(othello_player_t *player);
othello_status_t othello_handle_give_up(othello_player_t *player);

int othello_game_score(othello_player_t *player);
bool othello_game_able_to_play(othello_player_t *player);
othello_status_t othello_game_play_stroke(othello_player_t *player,
                                          unsigned char x, unsigned char y);
int othello_game_is_stroke_valid(othello_player_t *player, unsigned char x,
                                 unsigned char y);

int main(int argc, char *argv[]);

#endif

/**
 * \author Alexis Giraudet
 */

#ifndef OTHELLO_SERVER_H
#define OTHELLO_SERVER_H

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

int othello_handle_connect(othello_player_t *player);
int othello_handle_room_list(othello_player_t *player);
int othello_handle_room_join(othello_player_t *player);
int othello_handle_room_leave(othello_player_t *player);
int othello_handle_message(othello_player_t *player);
int othello_handle_ready(othello_player_t *player);
int othello_handle_not_ready(othello_player_t *player);
int othello_handle_play(othello_player_t *player);

int othello_player_score(othello_player_t *player);
int othello_player_can_play(othello_player_t *player);
int othello_player_valid_stroke(othello_player_t *player, unsigned char x,
                                unsigned char y);
othello_player_t *othello_is_game_over(othello_room_t *room);

int main(int argc, char *argv[]);

#endif

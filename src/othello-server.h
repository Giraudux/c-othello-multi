
#ifndef OTHELLO_SERVER_H
#define OTHELLO_SERVER_H

struct othello_player_s;
struct othello_room_s;

typedef struct othello_player_s othello_player_t;
typedef struct othello_room_s othello_room_t;

int othello_create_socket_stream(unsigned short port);
ssize_t othello_read_all(int fd, void * buf, size_t count);
ssize_t othello_write_all(int fd, void * buf, size_t count);
void othello_log(int priority, const char * format, ...);

void othello_exit();

void * othello_start(void * player);
void othello_end(othello_player_t * player);

int othello_connect(othello_player_t * player);
int othello_list_room(othello_player_t * player);
int othello_join_room(othello_player_t * player);
int othello_leave_room(othello_player_t * player);
int othello_send_message(othello_player_t * player);
int othello_ready(othello_player_t * player);
int othello_play_turn(othello_player_t * player);

int main(int argc, char * argv[]);

#endif

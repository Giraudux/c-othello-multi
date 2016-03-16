/**
 * \author Alexis Giraudet
 */

#define _BSD_SOURCE
#define _GNU_SOURCE

#include "othello.h"
#include "othello-server.h"

#include <arpa/inet.h>
#include <getopt.h>
#include <netdb.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <syslog.h>
#include <unistd.h>

struct othello_player_s {
  pthread_t thread;
  int socket;
  char name[OTHELLO_PLAYER_NAME_LENGTH];
  othello_room_t *room;
  pthread_mutex_t mutex;
  bool ready;
  othello_state_t state;
};

struct othello_room_s {
  othello_player_t *players[OTHELLO_ROOM_LENGTH];
  pthread_mutex_t mutex;
  othello_player_t *grid[OTHELLO_BOARD_LENGTH][OTHELLO_BOARD_LENGTH];
};

static othello_room_t othello_server_rooms[OTHELLO_NUMBER_OF_ROOMS];
static othello_room_t othello_server_players[OTHELLO_NUMBER_OF_PLAYERS];
static int othello_server_socket;
static bool othello_server_daemon;
static pthread_mutex_t othello_server_log_mutex;

/**
 * \return the result of the last call to read
 */
ssize_t othello_read_all(int fd, void *buf, size_t count) {
  ssize_t bytes_read;
  char *cursor;

  bytes_read = 0;
  cursor = buf;

  while (count > 0 && (bytes_read = read(fd, cursor, count)) > 0) {
    count -= bytes_read;
    cursor += bytes_read;
  }

  return bytes_read;
}

/**
 * \return the result of the last call to write
 */
ssize_t othello_write_all(int fd, void *buf, size_t count) {
  ssize_t bytes_write;
  char *cursor;

  bytes_write = 0;
  cursor = buf;

  while (count > 0 && (bytes_write = write(fd, cursor, count)) > 0) {
    count -= bytes_write;
    cursor += bytes_write;
  }

  return bytes_write;
}

/**
 *
 */
void othello_log(int priority, const char *format, ...) {
  va_list ap;

  va_start(ap, format);

  pthread_mutex_lock(&othello_server_log_mutex);
  if (othello_server_daemon) {
    vsyslog(priority, format, ap);
  } else {
    switch (priority) {
    case LOG_EMERG:
      fputs("EMERGENCY ", stdout);
      break;
    case LOG_ALERT:
      fputs("ALERT     ", stdout);
      break;
    case LOG_CRIT:
      fputs("CRITICAL  ", stdout);
      break;
    case LOG_ERR:
      fputs("ERROR     ", stdout);
      break;
    case LOG_WARNING:
      fputs("WARNING   ", stdout);
      break;
    case LOG_NOTICE:
      fputs("NOTICE    ", stdout);
      break;
    case LOG_INFO:
      fputs("INFO      ", stdout);
      break;
    case LOG_DEBUG:
      fputs("DEBUG     ", stdout);
      break;
    default:
      fputs("          ", stdout);
      break;
    }
    vprintf(format, ap);
    putc('\n', stdout);
  }
  pthread_mutex_unlock(&othello_server_log_mutex);
}

/**
 *
 */
void othello_player_end(othello_player_t *player) {
  char notif[1 + sizeof(player->name)];
  othello_player_t **player_cursor;

  othello_log(LOG_INFO, "%p end", player);

  if (player->state == OTHELLO_STATE_IN_GAME) {
    notif[0] = OTHELLO_NOTIF_GIVE_UP;
    memcpy(notif + 1, player->name, sizeof(player->name));

    pthread_mutex_lock(&(player->room->mutex));
    for (player_cursor = player->room->players;
         player_cursor < player->room->players + OTHELLO_ROOM_LENGTH;
         player_cursor++) {
      if (*player_cursor != NULL) {
        if (*player_cursor != player) {
          pthread_mutex_lock(&((*player_cursor)->mutex));
          othello_write_all((*player_cursor)->socket, notif, sizeof(notif));
          pthread_mutex_unlock(&((*player_cursor)->mutex));
        }
        (*player_cursor)->ready = false;
        (*player_cursor)->state = OTHELLO_STATE_IN_ROOM;
      }
    }
    pthread_mutex_unlock(&(player->room->mutex));
  }

  if (player->state == OTHELLO_STATE_IN_ROOM) {
    notif[0] = OTHELLO_NOTIF_ROOM_LEAVE;
    memcpy(notif + 1, player->name, sizeof(player->name));

    pthread_mutex_lock(&(player->room->mutex));
    for (player_cursor = player->room->players;
         player_cursor < player->room->players + OTHELLO_ROOM_LENGTH;
         player_cursor++) {
      if (*player_cursor == player) {
        *player_cursor = NULL;
      } else if (*player_cursor != NULL) {
        pthread_mutex_lock(&((*player_cursor)->mutex));
        othello_write_all((*player_cursor)->socket, notif, sizeof(notif));
        pthread_mutex_unlock(&((*player_cursor)->mutex));
      }
    }
    pthread_mutex_unlock(&(player->room->mutex));
  }

  pthread_mutex_destroy(&(player->mutex));
  close(player->socket);
  free(player);
}

/**
 *
 */
othello_status_t othello_handle_login(othello_player_t *player) {
  othello_status_t status;
  unsigned char protocol_version;
  char reply[2];

  othello_log(LOG_INFO, "%p login #1", player);

  status = OTHELLO_SUCCESS;

  reply[0] = OTHELLO_QUERY_LOGIN;
  reply[1] = OTHELLO_FAILURE;

  if (othello_read_all(player->socket, &protocol_version,
                       sizeof(protocol_version)) <= 0 ||
      protocol_version != OTHELLO_PROTOCOL_VERSION ||
      othello_read_all(player->socket, player->name, sizeof(player->name)) <=
          0) {
    status = OTHELLO_FAILURE;
  }

  if (status == OTHELLO_SUCCESS &&
      player->state == OTHELLO_STATE_NOT_CONNECTED && player->name[0] != '\0') {
    reply[1] = OTHELLO_SUCCESS;
    player->state = OTHELLO_STATE_CONNECTED;
  }

  pthread_mutex_lock(&(player->mutex));
  if (othello_write_all(player->socket, reply, sizeof(reply)) <= 0) {
    status = OTHELLO_FAILURE;
  }
  pthread_mutex_unlock(&(player->mutex));

  othello_log(LOG_INFO, "%p login #2", player);

  return status;
}

/*
 *
 */
othello_status_t othello_handle_room_list(othello_player_t *player) {
  othello_status_t status;
  char reply[1 +
             (2 + OTHELLO_ROOM_LENGTH * OTHELLO_PLAYER_NAME_LENGTH) *
                 OTHELLO_NUMBER_OF_ROOMS];
  char *reply_cursor;
  char room_id;
  char room_size;
  othello_player_t **player_cursor;
  othello_room_t *room_cursor;

  othello_log(LOG_INFO, "%p room list #1", player);

  status = OTHELLO_SUCCESS;

  memset(reply, 0, sizeof(reply));
  reply[0] = OTHELLO_QUERY_ROOM_LIST;

  reply_cursor = reply + 1;
  room_id = 0;
  for (room_cursor = othello_server_rooms;
       room_cursor < othello_server_rooms + OTHELLO_NUMBER_OF_ROOMS;
       room_cursor++) {
    *reply_cursor = room_id;
    reply_cursor++;
    room_size = 0;
    pthread_mutex_lock(&(room_cursor->mutex));
    for (player_cursor = room_cursor->players;
         player_cursor < room_cursor->players + OTHELLO_ROOM_LENGTH;
         player_cursor++) {
      if (*player_cursor != NULL) {
        room_size++;
        memcpy(reply_cursor, (*player_cursor)->name,
               OTHELLO_PLAYER_NAME_LENGTH);
      }
      reply_cursor += OTHELLO_PLAYER_NAME_LENGTH;
    }
    pthread_mutex_unlock(&(room_cursor->mutex));
    *reply_cursor = room_size;
    reply_cursor++;
    room_id++;
  }

  pthread_mutex_lock(&(player->mutex));
  if (othello_write_all(player->socket, reply, sizeof(reply)) <= 0) {
    status = OTHELLO_FAILURE;
  }
  pthread_mutex_unlock(&(player->mutex));

  othello_log(LOG_INFO, "%p room list #3", player);

  return status;
}

/**
 *
 */
othello_status_t othello_handle_room_join(othello_player_t *player) {
  othello_status_t status;
  unsigned char room_id;
  char reply[2];
  char notif[1 + OTHELLO_PLAYER_NAME_LENGTH];
  othello_room_t *room;
  othello_player_t **player_cursor;

  status = OTHELLO_SUCCESS;

  othello_log(LOG_INFO, "%p room join #1", player);

  reply[0] = OTHELLO_QUERY_ROOM_JOIN;
  reply[1] = OTHELLO_FAILURE;

  notif[0] = OTHELLO_NOTIF_ROOM_JOIN;

  othello_log(LOG_INFO, "%p room join #2", player);

  if (othello_read_all(player->socket, &room_id, sizeof(room_id)) <= 0) {
    status = OTHELLO_FAILURE;
  }

  if (status == OTHELLO_SUCCESS && room_id < OTHELLO_NUMBER_OF_ROOMS &&
      player->state == OTHELLO_STATE_CONNECTED) {

    room = &(othello_server_rooms[room_id]);

    pthread_mutex_lock(&(room->mutex));
    for (player_cursor = room->players;
         player_cursor < room->players + OTHELLO_ROOM_LENGTH; player_cursor++) {
      if (*player_cursor == NULL) {
        *player_cursor = player;
        player->room = room;
        player->state = OTHELLO_STATE_IN_ROOM;
        player->ready = false;
        break;
      }
    }
    if (player->room != NULL) {
      reply[1] = OTHELLO_SUCCESS;
      memcpy(notif + 1, player->name, OTHELLO_PLAYER_NAME_LENGTH);

      for (player_cursor = room->players;
           player_cursor < room->players + OTHELLO_ROOM_LENGTH;
           player_cursor++) {
        if (*player_cursor != NULL && *player_cursor != player) {
          pthread_mutex_lock(&((*player_cursor)->mutex));
          othello_write_all((*player_cursor)->socket, notif, sizeof(notif));
          pthread_mutex_unlock(&((*player_cursor)->mutex));
        }
      }
    }
    pthread_mutex_unlock(&(room->mutex));
  }

  pthread_mutex_lock(&(player->mutex));
  if (othello_write_all(player->socket, reply, sizeof(reply)) <= 0) {
    status = OTHELLO_FAILURE;
  }
  pthread_mutex_unlock(&(player->mutex));

  othello_log(LOG_INFO, "%p room join #3", player);

  return status;
}

/**
 *
 */
othello_status_t othello_handle_room_leave(othello_player_t *player) {
  othello_status_t status;
  char reply[2];
  char notif[1 + OTHELLO_PLAYER_NAME_LENGTH];
  othello_player_t **player_cursor;

  othello_log(LOG_INFO, "%p room leave #1", player);

  status = OTHELLO_SUCCESS;

  reply[0] = OTHELLO_QUERY_ROOM_LEAVE;
  reply[1] = OTHELLO_FAILURE;

  notif[0] = OTHELLO_NOTIF_ROOM_LEAVE;

  if (player->state == OTHELLO_STATE_IN_ROOM) {
    reply[1] = OTHELLO_SUCCESS;
    memcpy(notif + 1, player->name, OTHELLO_PLAYER_NAME_LENGTH);

    pthread_mutex_lock(&(player->room->mutex));
    for (player_cursor = player->room->players;
         player_cursor < player->room->players + OTHELLO_ROOM_LENGTH;
         player_cursor++) {
      if (*player_cursor == player) {
        *player_cursor = NULL;
      } else if (*player_cursor != NULL) {
        pthread_mutex_lock(&((*player_cursor)->mutex));
        othello_write_all((*player_cursor)->socket, notif, sizeof(notif));
        pthread_mutex_unlock(&((*player_cursor)->mutex));
      }
    }
    pthread_mutex_unlock(&(player->room->mutex));

    player->room = NULL;
    player->state = OTHELLO_STATE_CONNECTED;
  }

  pthread_mutex_lock(&(player->mutex));
  if (othello_write_all(player->socket, reply, sizeof(reply)) <= 0) {
    status = OTHELLO_FAILURE;
  }
  pthread_mutex_unlock(&(player->mutex));

  othello_log(LOG_INFO, "%p room leave #2", player);

  return status;
}

/**
 *
 */
othello_status_t othello_handle_message(othello_player_t *player) {
  othello_status_t status;
  char reply[2];
  char notif[1 + OTHELLO_PLAYER_NAME_LENGTH + OTHELLO_MESSAGE_LENGTH];
  othello_player_t **player_cursor;

  status = OTHELLO_SUCCESS;

  othello_log(LOG_INFO, "%p message #1", player);

  reply[0] = OTHELLO_QUERY_MESSAGE;
  reply[1] = OTHELLO_FAILURE;

  notif[0] = OTHELLO_NOTIF_MESSAGE;
  memcpy(notif + 1, player->name, OTHELLO_PLAYER_NAME_LENGTH);

  if (othello_read_all(player->socket, notif + 1 + OTHELLO_PLAYER_NAME_LENGTH,
                       OTHELLO_MESSAGE_LENGTH) <= 0) {
    status = OTHELLO_FAILURE;
  }

  if (status == OTHELLO_SUCCESS && (player->state == OTHELLO_STATE_IN_ROOM ||
                                    player->state == OTHELLO_STATE_IN_GAME)) {
    reply[1] = OTHELLO_SUCCESS;

    pthread_mutex_lock(&(player->room->mutex));
    for (player_cursor = player->room->players;
         player_cursor < player->room->players + OTHELLO_ROOM_LENGTH;
         player_cursor++) {
      if (*player_cursor != NULL && *player_cursor != player) {
        pthread_mutex_lock(&((*player_cursor)->mutex));
        othello_write_all((*player_cursor)->socket, notif, sizeof(notif));
        pthread_mutex_unlock(&((*player_cursor)->mutex));
      }
    }
    pthread_mutex_unlock(&(player->room->mutex));
  }

  pthread_mutex_lock(&(player->mutex));
  if (othello_write_all(player->socket, reply, sizeof(reply)) <= 0) {
    status = OTHELLO_FAILURE;
  }
  pthread_mutex_unlock(&(player->mutex));

  othello_log(LOG_INFO, "%p message #2", player);

  return status;
}

/**
 *
 */
othello_status_t othello_handle_ready(othello_player_t *player) {
  othello_status_t status;
  char reply[2];
  char notif_ready[1 + OTHELLO_PLAYER_NAME_LENGTH];
  char notif_start[2];
  othello_player_t **player_cursor;
  int players_ready;

  othello_log(LOG_INFO, "%p ready #1", player);

  status = OTHELLO_SUCCESS;

  reply[0] = OTHELLO_QUERY_READY;
  reply[1] = OTHELLO_FAILURE;

  notif_ready[0] = OTHELLO_NOTIF_READY;
  notif_start[0] = OTHELLO_NOTIF_GAME_START;

  if (player->state == OTHELLO_STATE_IN_ROOM && !player->ready) {
    reply[1] = OTHELLO_SUCCESS;
    player->ready = true;
    memcpy(notif_ready + 1, player->name, OTHELLO_PLAYER_NAME_LENGTH);
    players_ready = 1;

    pthread_mutex_lock(&(player->room->mutex));
    for (player_cursor = player->room->players;
         player_cursor < player->room->players + OTHELLO_ROOM_LENGTH;
         player_cursor++) {
      if (*player_cursor != NULL && *player_cursor != player) {
        if ((*player_cursor)->ready) {
          players_ready++;
        }
        pthread_mutex_lock(&((*player_cursor)->mutex));
        othello_write_all((*player_cursor)->socket, notif_ready,
                          sizeof(notif_ready));
        pthread_mutex_unlock(&((*player_cursor)->mutex));
      }
    }

    othello_log(LOG_INFO, "%p ready #2", player);

    pthread_mutex_lock(&(player->mutex));
    if (othello_write_all(player->socket, &reply, sizeof(reply)) <= 0) {
      status = OTHELLO_FAILURE;
    }
    pthread_mutex_unlock(&(player->mutex));

    othello_log(LOG_INFO, "%p ready %d / %d", player, players_ready,
                OTHELLO_ROOM_LENGTH);

    if (players_ready == OTHELLO_ROOM_LENGTH) {
      memset(player->room->grid, 0, sizeof(player->room->grid));

      othello_log(LOG_INFO, "%p ready #3", player);

      for (player_cursor = player->room->players;
           player_cursor < player->room->players + OTHELLO_ROOM_LENGTH;
           player_cursor++) {
        if (*player_cursor != NULL) {
          if (player_cursor == player->room->players) {
            notif_start[1] = true; /* first player of the room start to play */

            player->room->grid[4][3] = *player_cursor;
            player->room->grid[3][4] = *player_cursor;
          } else {
            notif_start[1] = false;
            (*player_cursor)->ready = false; /* can't play */

            player->room->grid[3][3] = *player_cursor;
            player->room->grid[4][4] = *player_cursor;
          }
          (*player_cursor)->state = OTHELLO_STATE_IN_GAME;
          pthread_mutex_lock(&((*player_cursor)->mutex));
          othello_write_all((*player_cursor)->socket, notif_start,
                            sizeof(notif_start));
          pthread_mutex_unlock(&((*player_cursor)->mutex));
        }
      }
    }
    pthread_mutex_unlock(&(player->room->mutex));
  } else {
    pthread_mutex_lock(&(player->mutex));
    if (othello_write_all(player->socket, &reply, sizeof(reply)) <= 0) {
      status = OTHELLO_FAILURE;
    }
    pthread_mutex_unlock(&(player->mutex));
  }

  return status;
}

othello_status_t othello_handle_not_ready(othello_player_t *player) {
  othello_status_t status;
  char reply[2];
  char notif_not_ready[1 + OTHELLO_PLAYER_NAME_LENGTH];
  othello_player_t **player_cursor;

  othello_log(LOG_INFO, "%p not ready #1", player);

  status = OTHELLO_SUCCESS;

  reply[0] = OTHELLO_QUERY_NOT_READY;
  reply[1] = OTHELLO_FAILURE;

  notif_not_ready[0] = OTHELLO_NOTIF_NOT_READY;

  if (player->state == OTHELLO_STATE_IN_ROOM && player->ready) {
    reply[1] = OTHELLO_SUCCESS;
    player->ready = false;
    memcpy(notif_not_ready + 1, player->name, OTHELLO_PLAYER_NAME_LENGTH);

    pthread_mutex_lock(&(player->room->mutex));
    for (player_cursor = player->room->players;
         player_cursor < player->room->players + OTHELLO_ROOM_LENGTH;
         player_cursor++) {
      if (*player_cursor != NULL && *player_cursor != player) {
        pthread_mutex_lock(&((*player_cursor)->mutex));
        othello_write_all((*player_cursor)->socket, notif_not_ready,
                          sizeof(notif_not_ready));
        pthread_mutex_unlock(&((*player_cursor)->mutex));
      }
    }
    pthread_mutex_unlock(&(player->room->mutex));

    othello_log(LOG_INFO, "%p not ready #2", player);
  }

  pthread_mutex_lock(&(player->mutex));
  if (othello_write_all(player->socket, &reply, sizeof(reply)) <= 0) {
    status = OTHELLO_FAILURE;
  }
  pthread_mutex_unlock(&(player->mutex));

  othello_log(LOG_INFO, "%p not ready #3", player);

  return status;
}

/*TODO: remove*/
void print_grid(othello_player_t *player) {
  int i, j;

  for (i = 0; i < OTHELLO_BOARD_LENGTH; i++) {
    for (j = 0; j < OTHELLO_BOARD_LENGTH; j++) {
      if (player->room->grid[i][j] == player) {
        putc('x', stdout);
      } else if (player->room->grid[i][j] == NULL) {
        putc('*', stdout);
      } else {
        putc('o', stdout);
      }
      putc(' ', stdout);
    }
    putc('\n', stdout);
  }
  putc('\n', stdout);
}

/**
 *
 */
othello_status_t othello_handle_play(othello_player_t *player) {
  othello_status_t status;
  unsigned char stroke[2];
  char reply[2 + 2]; /*TODO: fix me*/
  char notif_play[3];
  char notif_end[2];
  char notif_your_turn[1];
  othello_player_t **player_cursor;
  othello_player_t **player_next;
  othello_player_t *player_winner;
  othello_player_t *player_turn;
  int best_score, score;

  othello_log(LOG_INFO, "%p play #1", player);

  status = OTHELLO_SUCCESS;

  reply[0] = OTHELLO_QUERY_PLAY;
  reply[1] = OTHELLO_FAILURE;

  notif_play[0] = OTHELLO_NOTIF_PLAY;
  notif_end[0] = OTHELLO_NOTIF_GAME_END;

  notif_your_turn[0] = OTHELLO_NOTIF_YOUR_TURN;

  if (othello_read_all(player->socket, stroke, sizeof(stroke)) <= 0) {
    status = OTHELLO_FAILURE;
  }

  memcpy(reply + 2, stroke, sizeof(stroke)); /*TODO: fix me*/

  if (status == OTHELLO_SUCCESS && player->state == OTHELLO_STATE_IN_GAME &&
      player->ready) {

    othello_log(LOG_INFO, "%p play [%d, %d]", player, stroke[0], stroke[1]);
    if (othello_game_is_stroke_valid(player, stroke[0], stroke[1])) {
      othello_log(LOG_INFO, "%p play stroke is valid", player);
    } else {
      othello_log(LOG_INFO, "%p play stroke is invalid", player);
    }

    print_grid(player); /*TODO: remove*/

    pthread_mutex_lock(&(player->room->mutex));
    if (othello_game_play_stroke(player, stroke[0], stroke[1]) ==
        OTHELLO_SUCCESS) {
      reply[1] = OTHELLO_SUCCESS;
      player->ready = false;
    }
    pthread_mutex_unlock(&(player->room->mutex));

    print_grid(player); /*TODO: remove*/

    pthread_mutex_lock(&(player->mutex));
    if (othello_write_all(player->socket, reply, sizeof(reply)) <= 0) {
      status = OTHELLO_FAILURE;
    }
    pthread_mutex_unlock(&(player->mutex));

    /*invalid stroke*/
    if (reply[1] != OTHELLO_SUCCESS) {
      return status;
    }

    /*notify stroke*/
    memcpy(notif_play + 1, stroke, sizeof(stroke));

    pthread_mutex_lock(&(player->room->mutex));
    for (player_cursor = player->room->players;
         player_cursor < player->room->players + OTHELLO_ROOM_LENGTH;
         player_cursor++) {
      if (*player_cursor == player) {
        player_next = player_cursor + 1;
      } else if (*player_cursor != NULL) {
        pthread_mutex_lock(&((*player_cursor)->mutex));
        othello_write_all((*player_cursor)->socket, notif_play,
                          sizeof(notif_play));
        pthread_mutex_unlock(&((*player_cursor)->mutex));
      }
    }

    /*find next player or winner if game is over*/
    player_winner = player;
    player_turn = NULL;
    player_cursor = player_next;
    do {
      if (player_cursor >= player->room->players + OTHELLO_ROOM_LENGTH) {
        player_cursor = player->room->players;
      }
      if (othello_game_able_to_play(*player_cursor)) {
        player_turn = *player_cursor;
        break;
      }
      if ((score = othello_game_score(*player_cursor)) > best_score) {
        player_winner = *player_cursor;
        best_score = score;
      }

      player_cursor++;
    } while (player_cursor != player_next);

    othello_log(LOG_INFO, "%p play #2 -- %p", player, player_turn);

    /*game over*/
    if (player_turn == NULL) {
      othello_log(LOG_INFO, "%p play #3", player);

      for (player_cursor = player->room->players;
           player_cursor < player->room->players + OTHELLO_ROOM_LENGTH;
           player_cursor++) {
        if (*player_cursor != NULL) {
          (*player_cursor)->ready = false;
          (*player_cursor)->state = OTHELLO_STATE_IN_ROOM;

          if (*player_cursor == player_winner) {
            notif_end[1] = true;
          } else {
            notif_end[1] = false;
          }
          pthread_mutex_lock(&((*player_cursor)->mutex));
          othello_write_all((*player_cursor)->socket, notif_end,
                            sizeof(notif_end));
          pthread_mutex_unlock(&((*player_cursor)->mutex));
        }
      }
    } else {
      player_turn->ready = true;
      pthread_mutex_lock(&(player_turn->mutex));
      othello_write_all(player_turn->socket, notif_your_turn,
                        sizeof(notif_your_turn));
      pthread_mutex_unlock(&(player_turn->mutex));
    }

    pthread_mutex_unlock(&(player->room->mutex));
  } else {
    pthread_mutex_lock(&(player->mutex));
    if (othello_write_all(player->socket, reply, sizeof(reply)) <= 0) {
      status = OTHELLO_FAILURE;
    }
    pthread_mutex_unlock(&(player->mutex));
  }

  othello_log(LOG_INFO, "%p play #4", player);

  return status;
}

/**
 *
 */
othello_status_t othello_handle_give_up(othello_player_t *player) {
  othello_status_t status;
  char reply[2];
  char notif[1 + OTHELLO_PLAYER_NAME_LENGTH];
  othello_player_t **player_cursor;

  status = OTHELLO_SUCCESS;

  reply[0] = OTHELLO_QUERY_GIVE_UP;
  reply[1] = OTHELLO_FAILURE;

  notif[0] = OTHELLO_NOTIF_GIVE_UP;

  if (player->state == OTHELLO_STATE_IN_GAME) {
    reply[1] = OTHELLO_SUCCESS;
    memcpy(notif + 1, player->name, sizeof(player->name));

    pthread_mutex_lock(&(player->room->mutex));
    for (player_cursor = player->room->players;
         player_cursor < player->room->players + OTHELLO_ROOM_LENGTH;
         player_cursor++) {
      if (*player_cursor != NULL) {
        if (*player_cursor != player) {
          pthread_mutex_lock(&((*player_cursor)->mutex));
          othello_write_all((*player_cursor)->socket, notif, sizeof(notif));
          pthread_mutex_unlock(&((*player_cursor)->mutex));
        }
        (*player_cursor)->ready = false;
        (*player_cursor)->state = OTHELLO_STATE_IN_ROOM;
      }
    }
    pthread_mutex_unlock(&(player->room->mutex));
  }

  pthread_mutex_lock(&(player->mutex));
  if (othello_write_all(player->socket, &reply, sizeof(reply)) <= 0) {
    status = OTHELLO_FAILURE;
  }
  pthread_mutex_unlock(&(player->mutex));

  return status;
}

/**
 *
 */
void *othello_player_start(void *arg) {
  othello_player_t *player;
  char query;
  othello_status_t status;

  player = (othello_player_t *)arg;

  othello_log(LOG_INFO, "%p start", player);

  while (othello_read_all(player->socket, &query, sizeof(query)) > 0) {
    othello_log(LOG_INFO, "%p read query %d", player, query);

    switch (query) {
    case OTHELLO_QUERY_LOGIN:
      status = othello_handle_login(player);
      break;
    case OTHELLO_QUERY_ROOM_LIST:
      status = othello_handle_room_list(player);
      break;
    case OTHELLO_QUERY_ROOM_JOIN:
      status = othello_handle_room_join(player);
      break;
    case OTHELLO_QUERY_ROOM_LEAVE:
      status = othello_handle_room_leave(player);
      break;
    case OTHELLO_QUERY_MESSAGE:
      status = othello_handle_message(player);
      break;
    case OTHELLO_QUERY_READY:
      status = othello_handle_ready(player);
      break;
    case OTHELLO_QUERY_NOT_READY:
      status = othello_handle_not_ready(player);
      break;
    case OTHELLO_QUERY_PLAY:
      status = othello_handle_play(player);
      break;
    case OTHELLO_QUERY_GIVE_UP:
      status = othello_handle_give_up(player);
      break;
    case OTHELLO_QUERY_LOGOFF:;
    default:
      status = OTHELLO_FAILURE;
      break;
    }

    if (status != OTHELLO_SUCCESS) {
      break;
    }
  }

  othello_player_end(player);

  return NULL;
}

/**
 *
 */
int othello_create_socket_stream(unsigned short port) {
  int socket_stream, status, optval;
  struct sockaddr_in address;

  if ((socket_stream = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    othello_log(LOG_ERR, "socket");
    return socket_stream;
  }

  memset(&address, 0, sizeof(struct sockaddr_in));
  address.sin_family = AF_INET;
  address.sin_port = htons(port);
  address.sin_addr.s_addr = htonl(INADDR_ANY);

  optval = 1;
  if ((status = setsockopt(socket_stream, SOL_SOCKET, SO_REUSEADDR, &optval,
                           sizeof(int))) < 0) {
    othello_log(LOG_ERR, "setsockopt");
    return status;
  }

  if ((status = bind(socket_stream, (struct sockaddr *)&address,
                     sizeof(struct sockaddr_in))) < 0) {
    othello_log(LOG_ERR, "bind");
    return status;
  }

  return socket_stream;
}

/*
 * TODO: destroy all rooms/players
 */
void othello_exit(void) {
  if (othello_server_socket >= 0) {
    close(othello_server_socket);
  }
  closelog();
  pthread_mutex_destroy(&othello_server_log_mutex);
}

/**
 *
 */
int othello_game_score(othello_player_t *player) {
  int i, j, score;

  score = 0;

  for (i = 0; i < OTHELLO_BOARD_LENGTH; i++) {
    for (j = 0; j < OTHELLO_BOARD_LENGTH; j++) {
      if (player->room->grid[i][j] == player) {
        score++;
      }
    }
  }

  return score;
}

/**
 *
 */
bool othello_game_able_to_play(othello_player_t *player) {
  int i, j;

  for (i = 0; i < OTHELLO_BOARD_LENGTH; i++) {
    for (j = 0; j < OTHELLO_BOARD_LENGTH; j++) {
      if (othello_game_is_stroke_valid(player, i, j)) {
        return true;
      }
    }
  }

  return false;
}

/**
 *
 */
othello_status_t othello_game_play_stroke(othello_player_t *player,
                                          unsigned char x, unsigned char y) {
  int x_iter, y_iter;

  if (!othello_game_is_stroke_valid(player, x, y)) {
    return OTHELLO_FAILURE;
  }

  player->room->grid[x][y] = player;

  x_iter = x;
  y_iter = y;
  while ((x_iter - 1 >= 0) &&
         player->room->grid[x_iter - 1][y_iter] != player &&
         player->room->grid[x_iter - 1][y_iter] != NULL) {
    --x_iter;
  }
  if ((x_iter - 1 >= 0)) {
    if (player->room->grid[x_iter - 1][y_iter] == player) {
      while (x_iter < x) {
        player->room->grid[x_iter][y_iter] = player;
        ++x_iter;
      }
    }
  }
  x_iter = x;
  y_iter = y;
  while ((y_iter + 1 <= 7) &&
         player->room->grid[x_iter][y_iter + 1] != player &&
         player->room->grid[x_iter][y_iter + 1] != NULL) {
    ++y_iter;
  }
  if ((y_iter + 1 <= 7)) {
    if (player->room->grid[x_iter][y_iter + 1] == player) {
      while (y_iter > y) {
        player->room->grid[x_iter][y_iter] = player;
        --y_iter;
      }
    }
  }
  x_iter = x;
  y_iter = y;
  while ((x_iter + 1 <= 7) &&
         player->room->grid[x_iter + 1][y_iter] != player &&
         player->room->grid[x_iter + 1][y_iter] != NULL) {
    ++x_iter;
  }
  if ((x_iter + 1 <= 7)) {
    if (player->room->grid[x_iter + 1][y_iter] == player) {
      while (x_iter > x) {
        player->room->grid[x_iter][y_iter] = player;
        --x_iter;
      }
    }
  }
  x_iter = x;
  y_iter = y;
  while ((y_iter - 1 >= 0) &&
         player->room->grid[x_iter][y_iter - 1] != player &&
         player->room->grid[x_iter][y_iter - 1] != NULL) {
    --y_iter;
  }
  if ((y_iter - 1 >= 0)) {
    if (player->room->grid[x_iter][y_iter - 1] == player) {
      while (y_iter < y) {
        player->room->grid[x_iter][y_iter] = player;
        ++y_iter;
      }
    }
  }
  x_iter = x;
  y_iter = y;
  while ((x_iter - 1 >= 0) && (y_iter + 1 <= 7) &&
         player->room->grid[x_iter - 1][y_iter + 1] != player &&
         player->room->grid[x_iter - 1][y_iter + 1] != NULL) {
    --x_iter;
    ++y_iter;
  }
  if ((x_iter - 1 >= 0) && (y_iter + 1 <= 7)) {
    if (player->room->grid[x_iter - 1][y_iter + 1] == player) {
      while (x_iter < x) {
        player->room->grid[x_iter][y_iter] = player;
        ++x_iter;
        --y_iter;
      }
    }
  }
  x_iter = x;
  y_iter = y;
  while ((x_iter + 1 <= 7) && (y_iter + 1 <= 7) &&
         player->room->grid[x_iter + 1][y_iter + 1] != player &&
         player->room->grid[x_iter + 1][y_iter + 1] != NULL) {
    ++x_iter;
    ++y_iter;
  }
  if ((x_iter + 1 <= 7) && (y_iter + 1 <= 7)) {
    if (player->room->grid[x_iter + 1][y_iter + 1] == player) {
      while (x_iter > x) {
        player->room->grid[x_iter][y_iter] = player;
        --x_iter;
        --y_iter;
      }
    }
  }
  x_iter = x;
  y_iter = y;
  while ((x_iter + 1 <= 7) && (y_iter - 1 >= 0) &&
         player->room->grid[x_iter + 1][y_iter - 1] != player &&
         player->room->grid[x_iter + 1][y_iter - 1] != NULL) {
    ++x_iter;
    --y_iter;
  }
  if ((x_iter + 1 <= 7) && (y_iter - 1 >= 0)) {
    if (player->room->grid[x_iter + 1][y_iter - 1] == player) {
      while (x_iter > x) {
        player->room->grid[x_iter][y_iter] = player;
        --x_iter;
        ++y_iter;
      }
    }
  }
  x_iter = x;
  y_iter = y;
  while ((x_iter - 1 >= 0) && (y_iter - 1 >= 0) &&
         player->room->grid[x_iter - 1][y_iter - 1] != player &&
         player->room->grid[x_iter - 1][y_iter - 1] != NULL) {
    --x_iter;
    --y_iter;
  }
  if ((x_iter - 1 >= 0) && (y_iter - 1 >= 0)) {
    if (player->room->grid[x_iter - 1][y_iter - 1] == player) {
      while (x_iter < x) {
        player->room->grid[x_iter][y_iter] = player;
        ++x_iter;
        ++y_iter;
      }
    }
  }

  return OTHELLO_SUCCESS;
}

/**
 *
 */
int othello_game_is_stroke_valid(othello_player_t *player, unsigned char x,
                                 unsigned char y) {
  int x_iter, y_iter, nb_returned, final_returned;

  nb_returned = 0;
  final_returned = 0;

  if (player->room->grid[x][y] != NULL || x >= OTHELLO_BOARD_LENGTH ||
      y >= OTHELLO_BOARD_LENGTH) {
    return final_returned;
  }

  x_iter = x;
  y_iter = y;
  while ((x_iter - 1 >= 0) &&
         player->room->grid[x_iter - 1][y_iter] != player &&
         player->room->grid[x_iter - 1][y_iter] != NULL) {
    --x_iter;
    ++nb_returned;
  }
  if ((x_iter - 1 >= 0)) {
    if (player->room->grid[x_iter - 1][y_iter] == player) {
      final_returned += nb_returned;
    }
  }

  nb_returned = 0;
  x_iter = x;
  y_iter = y;
  while ((y_iter + 1 <= 7) &&
         player->room->grid[x_iter][y_iter + 1] != player &&
         player->room->grid[x_iter][y_iter + 1] != NULL) {
    ++y_iter;
    ++nb_returned;
  }
  if ((y_iter + 1 <= 7)) {
    if (player->room->grid[x_iter][y_iter + 1] == player) {
      final_returned += nb_returned;
    }
  }

  nb_returned = 0;
  x_iter = x;
  y_iter = y;
  while ((x_iter + 1 <= 7) &&
         player->room->grid[x_iter + 1][y_iter] != player &&
         player->room->grid[x_iter + 1][y_iter] != NULL) {
    ++x_iter;
    ++nb_returned;
  }
  if ((x_iter + 1 <= 7)) {
    if (player->room->grid[x_iter + 1][y_iter] == player) {
      final_returned += nb_returned;
    }
  }

  nb_returned = 0;
  x_iter = x;
  y_iter = y;
  while ((y_iter - 1 >= 0) &&
         player->room->grid[x_iter][y_iter - 1] != player &&
         player->room->grid[x_iter][y_iter - 1] != NULL) {
    --y_iter;
    ++nb_returned;
  }
  if ((y_iter - 1 >= 0)) {
    if (player->room->grid[x_iter][y_iter - 1] == player) {
      final_returned += nb_returned;
    }
  }

  nb_returned = 0;
  x_iter = x;
  y_iter = y;
  while ((x_iter - 1 >= 0) && (y_iter + 1 <= 7) &&
         player->room->grid[x_iter - 1][y_iter + 1] != player &&
         player->room->grid[x_iter - 1][y_iter + 1] != NULL) {
    --x_iter;
    ++y_iter;
    ++nb_returned;
  }
  if ((x_iter - 1 >= 0) && (y_iter + 1 <= 7)) {
    if (player->room->grid[x_iter - 1][y_iter + 1] == player) {
      final_returned += nb_returned;
    }
  }

  nb_returned = 0;
  x_iter = x;
  y_iter = y;
  while ((x_iter + 1 <= 7) && (y_iter + 1 <= 7) &&
         player->room->grid[x_iter + 1][y_iter + 1] != player &&
         player->room->grid[x_iter + 1][y_iter + 1] != NULL) {
    ++x_iter;
    ++y_iter;
    ++nb_returned;
  }
  if ((x_iter + 1 <= 7) && (y_iter + 1 <= 7)) {
    if (player->room->grid[x_iter + 1][y_iter + 1] == player) {
      final_returned += nb_returned;
    }
  }

  nb_returned = 0;
  x_iter = x;
  y_iter = y;
  while ((x_iter + 1 <= 7) && (y_iter - 1 >= 0) &&
         player->room->grid[x_iter + 1][y_iter - 1] != player &&
         player->room->grid[x_iter + 1][y_iter - 1] != NULL) {
    ++x_iter;
    --y_iter;
    ++nb_returned;
  }
  if ((x_iter + 1 <= 7) && (y_iter - 1 >= 0)) {
    if (player->room->grid[x_iter + 1][y_iter - 1] == player) {
      final_returned += nb_returned;
    }
  }

  nb_returned = 0;
  x_iter = x;
  y_iter = y;
  while ((x_iter - 1 >= 0) && (y_iter - 1 >= 0) &&
         player->room->grid[x_iter - 1][y_iter - 1] != player &&
         player->room->grid[x_iter - 1][y_iter - 1] != NULL) {
    --x_iter;
    --y_iter;
    ++nb_returned;
  }
  if ((x_iter - 1 >= 0) && (y_iter - 1 >= 0)) {
    if (player->room->grid[x_iter - 1][y_iter - 1] == player) {
      final_returned += nb_returned;
    }
  }

  return final_returned;
}

/**
 *
 */
void othello_daemonize(void) {
  int fd;

  chdir("/");
  if (fork() != 0) {
    exit(EXIT_SUCCESS);
  }
  setsid();
  if (fork() != 0) {
    exit(EXIT_SUCCESS);
  }
  for (fd = 0; fd < FOPEN_MAX; fd++) {
    close(fd);
  }
}

/**
 *
 */
void othello_print_help(void) {
  printf("Usage: othello-server [-p | --port <port>] [-d | --daemon]\n");
}

/**
 *
 */
int main(int argc, char *argv[]) {
  othello_player_t *player;
  othello_room_t *room_cursor;
  unsigned short port;
  int option;
  char *short_options = "hp:d";
  struct option long_options[] = {{"help", no_argument, NULL, 'h'},
                                  {"port", required_argument, NULL, 'p'},
                                  {"daemon", no_argument, NULL, 'd'},
                                  {NULL, 0, NULL, 0}};

  /* init global */
  /* init socket */
  port = 5000;
  othello_server_daemon = false;

  while ((option = getopt_long(argc, argv, short_options, long_options,
                               NULL)) != -1) {
    switch (option) {
    case 'h':
      othello_print_help();
      return EXIT_SUCCESS;
    case 'd':
      othello_server_daemon = true;
      break;
    case 'p':
      if (optarg && sscanf(optarg, "%hu", &port) == 1) {
        break;
      }
    default:
      othello_print_help();
      return EXIT_FAILURE;
    }
  }

  if (othello_server_daemon) {
    othello_daemonize();
  }

  openlog(NULL, LOG_CONS | LOG_PID, LOG_USER);
  if (pthread_mutex_init(&othello_server_log_mutex, NULL)) {
    return EXIT_FAILURE;
  }

  othello_server_socket = -1;

  memset(othello_server_rooms, 0, sizeof(othello_server_rooms));
  memset(othello_server_players, 0, sizeof(othello_server_players));

  for (room_cursor = othello_server_rooms;
       room_cursor < othello_server_rooms + OTHELLO_NUMBER_OF_ROOMS;
       room_cursor++) {
    if (pthread_mutex_init(&(room_cursor->mutex), NULL)) {
      return EXIT_FAILURE;
    }
  }

  if (atexit(othello_exit)) {
    return EXIT_FAILURE;
  }

  /* open socket */
  if ((othello_server_socket = othello_create_socket_stream(port)) < 0) {
    othello_log(LOG_ERR, "othello_create_socket_stream");
    return EXIT_FAILURE;
  }

  if (listen(othello_server_socket, SOMAXCONN)) {
    othello_log(LOG_ERR, "listen");
    return EXIT_FAILURE;
  }

  othello_log(LOG_INFO, "server listening on port %d", port);

  for (;;) {
    if ((player = malloc(sizeof(othello_player_t))) == NULL) {
      othello_log(LOG_ERR, "malloc");
      return EXIT_FAILURE;
    }

    memset(player, 0, sizeof(othello_player_t));
    if (pthread_mutex_init(&(player->mutex), NULL)) {
      othello_log(LOG_ERR, "pthread_mutex_init");
      return EXIT_FAILURE;
    }

    if ((player->socket = accept(othello_server_socket, NULL, NULL)) < 0) {
      othello_log(LOG_ERR, "accept");
      return EXIT_FAILURE;
    }

    if (pthread_mutex_init(&(player->mutex), NULL)) {
      othello_log(LOG_ERR, "pthread_mutex_init");
      return EXIT_FAILURE;
    }

    if (pthread_create(&(player->thread), NULL, othello_player_start, player)) {
      othello_log(LOG_ERR, "pthread_create");
      return EXIT_FAILURE;
    }

    if (pthread_detach(player->thread)) {
      othello_log(LOG_ERR, "pthread_detach");
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}

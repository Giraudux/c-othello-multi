/**
 *
 */

#define OTHELLO_BOARD_LENGTH 8

enum othello_query_e {
    OTHELLO_QUERY_CONNECT,
    OTHELLO_QUERY_LIST_ROOM,
    OTHELLO_QUERY_JOIN_ROOM,
    OTHELLO_QUERY_CREATE_ROOM,
    OTHELLO_QUERY_LEAVE_ROOM,
    OTHELLO_QUERY_SEND_MESSAGE,
    OTHELLO_QUERY_READY,
    OTHELLO_QUERY_PLAY_TURN
};

enum othello_state_e {
    OTHELLO_STATE_NOT_CONNECTED,
    OTHELLO_STATE_CONNECTED,
    OTHELLO_STATE_IN_ROOM,
    OTHELLO_STATE_IN_GAME
};

enum othello_status_e {
    OTHELLO_SUCCESS,
    OTHELLO_BLA_ERROR
};

typedef enum othello_query_e othello_query_t;
typedef enum othello_state_e othello_state_t;
typedef enum othello_status_e othello_status_t;
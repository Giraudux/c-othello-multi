/**
 *
 */

#define OTHELLO_BOARD_LENGTH 8

enum othello_code_e {
    CONNECT,
    LIST_ROOM,
    JOIN_ROOM,
    CREATE_ROOM,
    LEAVE_ROOM,
    SEND_MESSAGE,
    READY,
    PLAY_TURN
};

enum othello_state_e {
    NOT_CONNECTED,
    CONNECTED,
    IN_ROOM,
    IN_GAME
};

typedef enum othello_code_e othello_code_t;
typedef enum othello_state_e othello_state_t;

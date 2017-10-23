#ifndef DEFS_H
#define DEFS_H

#include <stdio.h>
#include <stdint.h>
#include <limits.h>
#include <unistd.h>
#include <pthread.h>

#define BOARD_WIDTH     7
#define BOARD_HEIGHT    6 
#define TOTAL_SPACES    (BOARD_WIDTH * BOARD_HEIGHT)

#define MIN_SCORE       (-(BOARD_WIDTH * BOARD_HEIGHT)/2 + 3)
#define MAX_SCORE       ((BOARD_WIDTH * BOARD_HEIGHT + 1)/2 - 3)
#define MIDDLE_COLUMN   (BOARD_WIDTH/2)
#define INVALID_INDEX   (BOARD_WIDTH + 1)

#define COMPUTER        (YELLOW)
#define USER            (RED)

#define MAX_NUM_GAMES   10
#define INVALID_GAME_ID UINT_MAX

#define CONSOLE_CLEAR   "\033[2J"

#define BACKLOG         3

#define NAME_BUF_SIZE   256

typedef uint64_t bool;
#define true 1
#define false 0

typedef unsigned int index_t;

typedef enum
{
    RED,
    YELLOW,
} color_t;

typedef enum
{
    EMPTY,
    OCCUPIED_YELLOW,
    OCCUPIED_RED,
    INVALID,
} space_t;

typedef enum
{
	RED_WIN,
	YELLOW_WIN,
	DRAW,
	NOT_FINISHED,
} outcome_t;

typedef struct _board
{
    uint64_t current_position;
    uint64_t mask;
    volatile uint64_t move_count;
} board_t;

typedef struct _entry
{
    uint64_t key: 56;
    uint8_t value;
} entry_t;

typedef struct _state_map
{
    entry_t *entries;
    uint64_t size;
    uint64_t capacity;
} state_map_t;

typedef struct _player
{
    color_t color;
    index_t (*get_move)(struct _player*, board_t*);
    state_map_t *map;
    FILE *io;
} player_t;

typedef int16_t game_time_t;

typedef struct _game_clock
{
    game_time_t elapsed_time;
    game_time_t current_base;
} game_clock_t;

typedef struct _game
{
    board_t *board;
    player_t *yellow;
    player_t *red;
    char *name;
    board_t *board_states[TOTAL_SPACES];
    game_time_t *move_times;
    game_clock_t *cpu_clock;
    FILE *io;
} game_t;

typedef int score_t;

typedef struct _score_entry
{
    uint64_t move;
    score_t score;
} score_entry_t;

typedef struct _move_sorter
{
    unsigned int size;
    score_entry_t entries[BOARD_WIDTH];
} move_sorter_t;

typedef struct _observer
{
    FILE *io;
    game_t *game;
} observer_t;

typedef unsigned int game_id_t;

typedef struct _manager
{
    unsigned int num_games;
    game_t *games[MAX_NUM_GAMES];
    pthread_t registered_observers[MAX_NUM_GAMES];
} manager_t;

#endif /* DEFS_H */

#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#include <game.h>
#include <clock.h>
#include <player.h>
#include <util.h>
#include <board.h>

#define PRINT(...) {fprintf(game->io, __VA_ARGS__); fflush(game->io);}

game_t* new_default_game(char *name, FILE *io)
{
    game_t *game;
    unsigned int x;

    game = malloc(sizeof(game_t));
    game->io = io;
    game->yellow = new_computer_player(YELLOW, io);
    game->red = new_user_player(RED, io);
    game->board = new_board(io);
    game->cpu_clock = new_game_clock();
    game->name = malloc(strlen(name));
    game->move_times = calloc(sizeof(game_time_t), TOTAL_SPACES/2);
    strncpy(game->name, name, strlen(name));

    for (x = 1; x < TOTAL_SPACES; x ++) {
        game->board_states[x] = NULL;
    } 

    play_column(game->board, MIDDLE_COLUMN - 1);
    game->board_states[0] = clone_board(game->board);
    
    return game;
}

void free_game(game_t *game)
{
    int x;

    fclose(game->io);
    free_board(game->board);
    free_player(game->red);
    free_player(game->yellow);
    free_game_clock(game->cpu_clock);
    free(game->move_times);
    free(game->name);

    for (x = 0; x < TOTAL_SPACES; x ++) {
        free(game->board_states[x]);
    }
    free(game);
}

player_t* whos_turn(game_t *game)
{
    if (game->board->move_count % 2) {
        return game->red;
    } else {
        return game->yellow;
    }
}

void play(game_t *game)
{
    player_t* current_player;
    index_t selected_column;
    outcome_t outcome;
    unsigned int option;
    game_time_t time;

    print_board(game->board, game->io);
    outcome = get_outcome(game->board);

    while (outcome == NOT_FINISHED) {
        current_player = whos_turn(game); 
        if (current_player->color == COMPUTER) {
            start_clock(game->cpu_clock);
        }

        selected_column = current_player->get_move(current_player, game->board);
        if (selected_column == INVALID_INDEX) {
            return;
        }
        play_column(game->board, selected_column);
        game->board_states[game->board->move_count - 1] = clone_board(game->board);

        print_board(game->board, game->io);
        outcome = get_outcome(game->board);
        switch (outcome) {
            case RED_WIN:    PRINT("Red wins!\n"); break;
            case YELLOW_WIN: PRINT("Yellow wins!\n"); break;
            case DRAW:       PRINT("Draw game...\n"); break;
            default: /* Should never be here */ break;
        }

        if (current_player->color == COMPUTER) {
            pause_clock(game->cpu_clock);
            time = get_elapsed_time(game->cpu_clock);
            game->move_times[(game->board->move_count-1)/2] = time;
            PRINT("The computer has taken %u mS total\n", time);
        }
    }

    start_clock(game->cpu_clock);
    PRINT("Press return to exit\n");
    get_unsigned_input(game->io, &option);
}

int move_index_for_cpu_time(game_t *game, game_time_t cpu_time)
{
    // Our buggy function
    unsigned int x, move_index;

    move_index = -1;
    for (x = 0; x < (game->board->move_count - 1) / 2; x++) {
        if (game->move_times[x] < cpu_time && cpu_time < game->move_times[x+1]) {
            move_index = x;
        } 
    }

    // If the time of the final move overflows, this will be -1
    return move_index;
}

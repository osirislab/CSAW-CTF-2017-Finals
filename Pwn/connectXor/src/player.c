#include <time.h>
#include <stdlib.h>
#include <stdio.h>

#include <player.h>
#include <state_map.h>
#include <move_sorter.h>
#include <util.h>

#define START_PLAYING_SMART_TURN 7
#define PRINT(...) {fprintf(player->io, __VA_ARGS__); fflush(player->io);}

static index_t COLUMN_ORDER[BOARD_WIDTH] = { 3, 4, 2, 5, 1, 6, 0 };

index_t get_user_move(player_t *player, board_t *board)
{
    index_t column;
    int retval;

    do {
        PRINT("Input a column to place your piece > ");
        retval = get_unsigned_input(player->io, &column);
        if (retval != 1) {
            return INVALID_INDEX;
        }
    } while (!can_play(board, column));
        
    return column;
}

score_t negamax(board_t *board, state_map_t *map, score_t alpha, score_t beta)
{
    index_t x;
    score_t min, max, memoized_value, score;
    board_t *board_clone;
    uint64_t next, tmp_move;
    move_sorter_t *sorter;

    // Check if there are any non-losing moves we can make
    next = possible_non_losing_moves(board);
    if (next == 0) {
        return -(BOARD_WIDTH*BOARD_HEIGHT - num_moves_taken(board))/2;
    }

    // Check if there is a draw
    if (num_moves_taken(board) >= BOARD_WIDTH*BOARD_HEIGHT - 2) {
        return 0;
    }

    // Get lower score bounds
    min = -(BOARD_WIDTH*BOARD_HEIGHT - 2 - num_moves_taken(board))/2;
    if (alpha < min) {
        alpha = min;
        if (alpha >= beta) {
            return alpha;
        }
    } 
    // Check memoized scores
    max = (BOARD_WIDTH*BOARD_HEIGHT - 1 - num_moves_taken(board))/2;
    if ((memoized_value = get(map, key(board)))) {
        max = memoized_value + MIN_SCORE - 1;
    }

    // Get upper score bounds
    if (beta > max) {
        beta = max;
        if (alpha >= beta) {
            return beta;
        }
    }

    sorter = new_move_sorter();
    for (x = 0; x < BOARD_WIDTH; x ++) {
        if ((tmp_move = (next & column_mask(COLUMN_ORDER[x])))) {
            add_to_sorter(sorter, tmp_move, score_move(board, tmp_move)); 
        }
    }

    while ((next = get_next_move(sorter))) {
        board_clone = clone_board(board);
        play_move(board_clone, next);
        score = -negamax(board_clone, map, -beta, -alpha);
        free_board(board_clone);

        if (score >= beta) {
            return score;
        } 

        if (score > alpha) {
            alpha = score;
        }
    }

    free_move_sorter(sorter);
    put(map, key(board), alpha - MIN_SCORE + 1);
    return alpha;
}

score_t solve(state_map_t *map, board_t *board)
{
    score_t min, max, med, med_p1, ret;

    if (can_win_next(board)) {
        return (BOARD_WIDTH * BOARD_HEIGHT + 1 - num_moves_taken(board))/2;
    }

    min = -(BOARD_WIDTH*BOARD_HEIGHT - num_moves_taken(board))/2;
    max = (BOARD_WIDTH*BOARD_HEIGHT + 1 - num_moves_taken(board))/2;

    while (min < max) {
        med = min + (max - min)/2;
        if (med <=0 && min/2 < med) {
            med = min/2;
        } else if (med >= 0 && max/2 > med) {
            med = max/2;
        }
        
        med_p1 = med + 1;
        ret = negamax(board, map, med, med_p1);
        if (ret <= med) {
            max = ret;
        } else {
            min = ret;
        }
    }

    return min;
}

index_t get_computer_move(player_t *player, board_t *board)
{
    score_t next_turn_score, target_score;
    index_t x;
    board_t *clone;

    if (num_moves_taken(board) > START_PLAYING_SMART_TURN) {
        // Playing smart
        target_score = solve(player->map, board);
        for (x = 0; x < BOARD_WIDTH; x ++) {
            if (can_play(board, x) && is_winning_move(board, x)) {
                return x;
            }

            else if (can_play(board, x)) {
                clone = clone_board(board);
                play_column(clone, x);
                next_turn_score = -solve(player->map, clone);
                free_board(clone);

                // Try to find score that matches board
                if (next_turn_score == target_score) {
                    return x;
                }
            }
        }

        return x;
    } else {
        // Playing dumb
        PRINT("I'm a dummy\n");
        if (can_play(board, MIDDLE_COLUMN)) {
            return MIDDLE_COLUMN;
        } else {
            do {
                x = rand() % BOARD_WIDTH;
            } while (!can_play(board, x));

            return x;
        }
    }
}

player_t* new_computer_player(color_t color, FILE *io)
{
    srand(time(NULL));
    player_t *player = malloc(sizeof(player_t));
    player->get_move = get_computer_move;
    player->color = color;
    player->map = new_state_map();

    player->io = io;
    return player;
}

player_t* new_user_player(color_t color, FILE *io)
{
    player_t *player = malloc(sizeof(player_t));
    player->get_move = get_user_move;
    player->color = color;
    // This isn't a useful field for a user player
    player->map = NULL;

    player->io = io;
    return player;
}

void free_player(player_t *player)
{
    if (player->map != NULL) {
        free_state_map(player->map);
    }
    free(player);
}

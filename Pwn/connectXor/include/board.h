#ifndef BOARD_H
#define BOARD_H

#include <defs.h>

board_t *new_board();
board_t *clone_board(board_t *board);
void free_board(board_t *board);
board_t *empty_board();

bool can_play(board_t *board, index_t column);
void play_move(board_t *board, uint64_t move);
void play_column(board_t *board, index_t column);
bool is_winning_move(board_t *board, index_t column);
int num_moves_taken(board_t *board);
uint64_t key(board_t *board);

bool alignment(uint64_t pos);
uint64_t top_mask(index_t column);
uint64_t column_mask(index_t column);
bool can_win_next(board_t *board);
uint64_t possible_non_losing_moves(board_t *board);
uint64_t compute_winning_position(uint64_t position, uint64_t mask);
int score_move(board_t *board, uint64_t move);

outcome_t get_outcome(board_t *board);
void print_board(board_t *board, FILE *io);

#endif /* BOARD_H */

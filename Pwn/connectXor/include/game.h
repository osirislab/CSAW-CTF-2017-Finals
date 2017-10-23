#ifndef DRIVER_H
#define DRIVER_H

#include <defs.h>

/* Create a new default game.  Computer is yellow, user is red. */
game_t* new_default_game(char *name, FILE *io);
/* Delete a game */
void free_game(game_t *game);
/* Get's the turn from the user and applies it to the board */
void get_user_turn(game_t *game);
/* Get's the turn from the computer and applies it to the board */
void get_cpu_turn(game_t *game);
/* Drive the game */
void play(game_t *game);
/* Get the move index for a given game time */
int move_index_for_cpu_time(game_t *game, game_time_t cpu_time);

#endif /* DRIVER_H */

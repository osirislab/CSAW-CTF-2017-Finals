#ifndef PLAYER_H
#define PLAYER_H

#include <defs.h>
#include <board.h>

/* Get a new computer player */
player_t* new_computer_player(color_t color, FILE *io);
/* Get a new user player */
player_t* new_user_player(color_t color, FILE *io);
/* Delete a player */
void free_player(player_t *player);

#endif /* PLAYER_H */

#ifndef CLOCK_H
#define CLOCK_H

#include <defs.h>

game_clock_t* new_game_clock();
void free_game_clock(game_clock_t *clock);

void start_clock(game_clock_t *clock);
void pause_clock(game_clock_t *clock);
game_time_t get_elapsed_time(game_clock_t *clock);

#endif /* CLOCK_H */

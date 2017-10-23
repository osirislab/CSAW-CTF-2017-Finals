#ifndef OBSERVER_H
#define OBSERVER_H

#include <defs.h>

observer_t* new_observer(game_t *game, FILE *io);
void free_observer(observer_t* observer);

void start_observing(observer_t *observer);
void replay_from_point(observer_t *observer, int move_index);

#endif /* OBSERVER_H */

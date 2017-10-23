#ifndef MOVE_SORTER_H
#define MOVE_SORTER_H

#include <defs.h>

move_sorter_t* new_move_sorter();
void free_move_sorter(move_sorter_t *sorter);

void add_to_sorter(move_sorter_t *sorter, uint64_t move, score_t entry);
uint64_t get_next_move(move_sorter_t *sorter);
void reset(move_sorter_t *sorter);

#endif /* MOVE_SORTER_H */

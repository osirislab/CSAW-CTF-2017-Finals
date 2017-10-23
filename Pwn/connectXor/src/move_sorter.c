#include <stdlib.h>
#include <stdio.h>

#include <move_sorter.h>

move_sorter_t* new_move_sorter()
{
    move_sorter_t *sorter = malloc(sizeof(move_sorter_t));
    reset(sorter);
    return sorter;
}

void free_move_sorter(move_sorter_t *sorter)
{
    free(sorter);
}

void add_to_sorter(move_sorter_t *sorter, uint64_t move, score_t score)
{
    int pos;

    pos = sorter->size++;
    for (; pos && sorter->entries[pos - 1].score > score; --pos) {
        sorter->entries[pos] = sorter->entries[pos - 1];
    }

    sorter->entries[pos].move = move;
    sorter->entries[pos].score = score;
}

uint64_t get_next_move(move_sorter_t *sorter)
{
    if (sorter->size) {
        return sorter->entries[--sorter->size].move;
    } else {
        return 0;
    }
}

void reset(move_sorter_t *sorter)
{
    sorter->size = 0;
}

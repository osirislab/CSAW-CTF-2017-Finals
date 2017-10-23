#ifndef STATE_MAP_H
#define STATE_MAP_H

#include <defs.h>

/* Create a new state map */
state_map_t* new_state_map();
/* Delete a state map */
void free_state_map(state_map_t *map);
/* Clone a state map */
state_map_t* clone_state_map(state_map_t* map);

/* Get the internal index for a map key */
uint64_t map_index(state_map_t *map, uint64_t key);
/* Reset the map to empty */
void reset_map(state_map_t *map);
/* Put a value in the map */
void put(state_map_t *map, uint64_t key, uint8_t val);
/* Get a value from the map */
uint8_t get(state_map_t *map, uint64_t key);

#endif /* STATE_MAP_H */

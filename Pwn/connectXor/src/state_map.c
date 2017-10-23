#include <string.h>
#include <stdlib.h>

#include <state_map.h>

#define DEFAULT_START_SIZE      (8388593) // 64M array prime sized

state_map_t* new_state_map()
{
    state_map_t *map = malloc(sizeof(state_map_t));
    map->entries = calloc(DEFAULT_START_SIZE, sizeof(entry_t));
    map->capacity = DEFAULT_START_SIZE;
    map->size = 0;
    reset_map(map);
    return map;
}

void free_state_map(state_map_t *map)
{
    free(map->entries);
    free(map);
}

state_map_t* clone_state_map(state_map_t *map)
{
    state_map_t *clone = new_state_map();
    memcpy(clone->entries, map->entries, map->capacity);
    clone->capacity = map->capacity;
    clone->size = map->size;
    return clone;
}

uint64_t map_index(state_map_t *map, uint64_t key)
{
    return key % (map->capacity);
}

void reset_map(state_map_t *map)
{
    memset(map->entries, 0, map->capacity * sizeof(entry_t));
}

void put(state_map_t *map, uint64_t key, uint8_t value)
{
    uint64_t entries_index;
    if (key < (1LL << 56)) {
        // Valid 56-bit key
        entries_index = map_index(map, key);
        map->entries[entries_index].key = key;
        map->entries[entries_index].value = value;
        map->size += 1;
    }
}

uint8_t get(state_map_t *map, uint64_t key)
{
    uint64_t entries_index;
    if (key < (1LL << 56)) {
        // Valid 56-bit key
        entries_index = map_index(map, key);

        if (map->entries[entries_index].key == key) {
            return map->entries[entries_index].value; 
        }
    }

    return 0;
}

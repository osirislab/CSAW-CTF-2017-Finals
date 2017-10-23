#ifndef MANAGER_H
#define MANAGER_H

#include <defs.h>

manager_t* new_manager();
void free_manager(manager_t *manager);

game_id_t create_game(manager_t *manager, FILE *io);
void participate_in_game(manager_t *manager, game_id_t id);
void observe_game(manager_t *manager, game_id_t id, FILE *io);

void show_menu(manager_t *manager, FILE *io);
void print_games(manager_t *manager, FILE *io);

#endif /* MANAGER_H */

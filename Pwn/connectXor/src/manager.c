#include <stdlib.h>
#include <stdio.h>
#include <sys/sendfile.h>
#include <string.h>
#include <unistd.h>

#include <manager.h>
#include <game.h>
#include <board.h>
#include <clock.h>
#include <util.h>
#include <observer.h>

#define PRINT(...) {fprintf(io, __VA_ARGS__); fflush(io);}

manager_t* new_manager()
{
    unsigned int x;

    manager_t *manager;
    manager = malloc(sizeof(manager_t));

    for (x = 0; x < MAX_NUM_GAMES; x ++) {
        manager->games[x] = NULL;
        manager->registered_observers[x] = 0;
    }
    manager->num_games = 0;
    return manager;
}

void free_manager(manager_t *manager)
{
    int x;

    for (x = 0; x < manager->num_games; x ++) {
        free_game(manager->games[x]);
    }

    free(manager); 
}

game_id_t create_game(manager_t *manager, FILE *io)
{
    unsigned int x;
    char name_buf[NAME_BUF_SIZE];
    game_t *new_game;
    char* retval;
  
    PRINT("Assign a name to your game > ");
    retval = fgets(name_buf, NAME_BUF_SIZE, io);
    if (retval == NULL) { return INVALID_GAME_ID; }

    while (strlen(retval) == 0) {
        PRINT("Enter a valid name > ");
    }
    name_buf[strlen(name_buf) - 1] = 0;
    new_game = new_default_game(name_buf, io);
     
    if (manager->num_games < MAX_NUM_GAMES) {
        for (x = 0; x < MAX_NUM_GAMES; x ++) {
            if (manager->games[x] == NULL) {
                manager->games[x] = new_game;
                manager->num_games ++;
                return x;
            }
        }
    } 
    
    return INVALID_GAME_ID;
}

game_t* get_game(manager_t *manager, game_id_t id)
{
    if (id < manager->num_games) {
        return manager->games[id];
    }
    
    return NULL;
}

void participate_in_game(manager_t *manager, game_id_t id)
{
    game_t *game;

    game = get_game(manager, id);

    if (game != NULL && game->board->move_count == 1) {
        play(game);
    }
}

void end_game(manager_t *manager, game_id_t id, FILE *io)
{
    game_t *game;

    game = get_game(manager, id);
    if(manager->registered_observers[id] != 0) {
        PRINT("Waiting for observers to finish\n");
        pthread_join(manager->registered_observers[id], NULL);
    }
    free_game(game);
    manager->games[id] = NULL;
    manager->num_games -= 1;
}

void show_menu(manager_t *manager, FILE *io)
{
    unsigned int option;
    game_id_t id;
    game_t *game;
    observer_t *observer;
    int retval;

    PRINT(CONSOLE_CLEAR);
    do {
        PRINT("Select an option\n");
        PRINT("[0] Start a game\n");
        PRINT("[1] Observe a game\n");
        PRINT("[2] Exit\n");
        PRINT("Selection > ");
        retval = get_unsigned_input(io, &option);
        if (retval == -1) {
            return;
        }
    } while (retval != 1 && option > 2);

    id = 0;
    PRINT(CONSOLE_CLEAR);
    switch (option) {
        case 0: 
            id = create_game(manager, io);
            if (id == INVALID_GAME_ID) {
                return;
            }
            PRINT("Your game ID is [%d]\n", id);
            participate_in_game(manager, id);
            end_game(manager, id, io);
            break;
        case 1:
            if (manager->num_games > 0) {
                do {
                    PRINT("These are the currently running games:\n");
                    print_games(manager, io);
                    PRINT("Selection > ");
                    retval = get_unsigned_input(io, &option);
                    if (retval == -1) {
                        return;
                    }
                } while (option > manager->num_games);

                game = get_game(manager, option);
                if (game == NULL) {
                    PRINT("That's not a valid ID\n");
                    return;
                }
                if (manager->registered_observers[option] != 0) {
                    PRINT("Sorry, someone is already observing that game\n");
                }
                else {
                    observer = new_observer(game, io);
                    manager->registered_observers[id] = pthread_self();
                    start_observing(observer);
                    free_observer(observer);
                    manager->registered_observers[id] = 0;
                }
            } else {
                PRINT("There are no games currently running to observe.\n");
                PRINT("Goodbye\n");
                fclose(io);
                return;
            }
            break;
        case 2:
        default:
            PRINT("Goodbye\n");
            fclose(io);
            break;
    }
}

void print_games(manager_t *manager, FILE *io)
{
    unsigned int i;

    for (i = 0; i < manager->num_games; i ++) {
        PRINT("    [%d] %s\n", i, manager->games[i]->name);
    }
}

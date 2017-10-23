#include <stdlib.h>

#include <observer.h>
#include <game.h>
#include <clock.h>
#include <board.h>
#include <util.h>

#define PRINT(...) {fprintf(observer->io, __VA_ARGS__); fflush(observer->io);}

observer_t* new_observer(game_t *game, FILE *io)
{
    observer_t *observer;
    observer = malloc(sizeof(observer_t));
    observer->game = game;
    observer->io = io;

    return observer;
}

void free_observer(observer_t* observer)
{
    fclose(observer->io);
    free(observer);
}

void report_game_action(observer_t *observer, int move_index)
{
	board_t *board;
	board = observer->game->board_states[move_index];
    print_board(board, observer->io);
}

void start_observing(observer_t *observer)
{
    unsigned int move_index, option, time_max;
    volatile int current_move;
    int retval;
    outcome_t outcome;

    for (move_index = 0; move_index < TOTAL_SPACES; move_index ++) {
        // Poll for new move
        do {
            current_move = observer->game->board->move_count;
        } while (move_index >= current_move - 1);

        report_game_action(observer, move_index);
        outcome = get_outcome(observer->game->board);

        if (outcome != NOT_FINISHED) { break; }
    }

    // Report game ending action
    report_game_action(observer, move_index + 1);

    if (outcome == DRAW) {
        PRINT("Would you replay the game from a previous move?\n");
        PRINT("[0] Yes\n");
        PRINT("[1] No\n");
        PRINT("Selection > ");

        retval = get_unsigned_input(observer->io, &option);
        if (retval == 1) {
            if (option == 0) {
                // Valid 'yes' response
                PRINT(CONSOLE_CLEAR);
                PRINT("Select by move number or elapsed CPU time?\n");
                PRINT("[0] Move number\n");
                PRINT("[1] Elapsed CPU time\n");
                PRINT("Selection > ");

                retval = get_unsigned_input(observer->io, &option);
                if (retval == 1) {
                    // Valid response
                    PRINT(CONSOLE_CLEAR);
                    if (option == 0) {
                        PRINT("Enter a move number [0 - %lu]\n", observer->game->board->move_count);
                        PRINT("Selection > ");

                        retval = get_unsigned_input(observer->io, &option);
                        if (retval == 1 && option < observer->game->board->move_count) {
                            PRINT(CONSOLE_CLEAR);
                            replay_from_point(observer, option); 
                        }
                    } else {
                        time_max = get_elapsed_time(observer->game->cpu_clock);
                        PRINT("Enter a CPU time [0mS - %d mS]\n", time_max);
                        PRINT("Selection > ");

                        retval = get_unsigned_input(observer->io, &option);
                        if (retval == 1 && option < time_max) {
                            move_index = move_index_for_cpu_time(observer->game, option);
                            replay_from_point(observer, move_index);
                        }
                    }
                }
            }
        }
    }
    PRINT(CONSOLE_CLEAR);
    PRINT("Goodbye\n");
}

void replay_from_point(observer_t *observer, int move_index)
{
    int x;
    for (x = move_index; x < (int) observer->game->board->move_count; x ++) {
        sleep(2);
        report_game_action(observer, x);
    }
}

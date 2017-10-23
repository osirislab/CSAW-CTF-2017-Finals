#include <clock.h>
#include <sys/time.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define SEC_2_MSEC(sec)     (sec * 1000)
#define USEC_2_MSEC(usec)   (usec / 1000)
#define TIMEVAL_2_MSEC(tp)  (SEC_2_MSEC(tp.tv_sec) + USEC_2_MSEC(tp.tv_usec))

game_clock_t* new_game_clock()
{
    game_clock_t *clock;
    clock = malloc(sizeof(game_clock_t));
    memset(clock, 0, sizeof(game_clock_t));
    return clock;
}

void free_game_clock(game_clock_t *clock)
{
    free(clock);
}

void start_clock(game_clock_t *clock)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    clock->current_base = TIMEVAL_2_MSEC(tp);
}

void pause_clock(game_clock_t *clock)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    clock->elapsed_time += (TIMEVAL_2_MSEC(tp) - clock->current_base);
    clock->current_base = 0;
}

game_time_t get_elapsed_time(game_clock_t *clock)
{
    struct timeval tp;

    if (clock->current_base == 0) {
        // Paused clock
        return clock->elapsed_time;
    }

    gettimeofday(&tp, NULL);
    return clock->elapsed_time + (TIMEVAL_2_MSEC(tp) - clock->current_base);
}

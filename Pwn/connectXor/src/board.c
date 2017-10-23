#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <board.h>

#define PRINT(...) {fprintf(io, __VA_ARGS__); fflush(io);}

void print_board(board_t *board, FILE *io);

/* Hack for static creation of values */
static bool statics_inited = false;
static uint64_t bottom_mask;
static uint64_t board_mask;

uint64_t bottom(index_t width, index_t height)
{
    return width == 0 ? 0 : bottom(width-1, height) | 1LL << (width-1)*(height+1);
}

uint64_t top_mask_column(index_t column)
{
    return UINT64_C(1) << ((BOARD_HEIGHT - 1) + column * (BOARD_HEIGHT + 1));
}

uint64_t bottom_mask_column(index_t column)
{
    return (UINT64_C(1) << column * (BOARD_HEIGHT + 1));
}

uint64_t column_mask(index_t column)
{
    return ((UINT64_C(1) << BOARD_HEIGHT) - 1) << column * (BOARD_HEIGHT + 1);
}

uint64_t top_mask(index_t column)
{
    return (UINT64_C(1) << (BOARD_HEIGHT - 1)) << column * (BOARD_HEIGHT + 1);
}

board_t* new_board()
{
    if (!statics_inited) {
        bottom_mask = bottom(BOARD_WIDTH, BOARD_HEIGHT);
        board_mask = bottom_mask * ((1LL << BOARD_HEIGHT) - 1);
        statics_inited = true;
    }
    board_t *board = malloc(sizeof(board_t));
    memset(board, 0, sizeof(board_t));
    return board;
}

board_t* clone_board(board_t *board)
{
    board_t *clone = new_board();
    clone->current_position = board->current_position;
    clone->mask = board->mask;
    clone->move_count = board->move_count;
    return clone;
}

void free_board(board_t *board)
{
    free(board);
}

bool can_play(board_t *board, index_t column)
{
    return (column < BOARD_WIDTH) && (board->mask & top_mask_column(column)) == 0;
}

void play_move(board_t *board, uint64_t move)
{
    board->current_position ^= board->mask;
    board->mask |= move;
    board->move_count++;
}

void play_column(board_t *board, index_t column)
{
    play_move(board, (board->mask + bottom_mask_column(column)) & column_mask(column));
}

unsigned int pop_count(uint64_t move)
{
    unsigned int c;
    for (c = 0; move; c++) {
        move &= move - 1;
    }

    return c;
}

int score_move(board_t *board, uint64_t move)
{
    return pop_count(compute_winning_position(board->current_position | move, board->mask));
}

uint64_t compute_winning_position(uint64_t position, uint64_t mask)
{
    uint64_t r, p; 
    
    // vertical
    r = (position << 1) & (position << 2) & (position << 3);

    //horizontal
    p = (position << (BOARD_HEIGHT+1)) & (position << 2*(BOARD_HEIGHT+1));
    r |= p & (position << 3*(BOARD_HEIGHT+1));
    r |= p & (position >> (BOARD_HEIGHT+1));
    p = (position >> (BOARD_HEIGHT+1)) & (position >> 2*(BOARD_HEIGHT+1));
    r |= p & (position << (BOARD_HEIGHT+1));
    r |= p & (position >> 3*(BOARD_HEIGHT+1));

    //diagonal 1
    p = (position << BOARD_HEIGHT) & (position << 2*BOARD_HEIGHT);
    r |= p & (position << 3*BOARD_HEIGHT);
    r |= p & (position >> BOARD_HEIGHT);
    p = (position >> BOARD_HEIGHT) & (position >> 2*BOARD_HEIGHT);
    r |= p & (position << BOARD_HEIGHT);
    r |= p & (position >> 3*BOARD_HEIGHT);

    //diagonal 2
    p = (position << (BOARD_HEIGHT+2)) & (position << 2*(BOARD_HEIGHT+2));
    r |= p & (position << 3*(BOARD_HEIGHT+2));
    r |= p & (position >> (BOARD_HEIGHT+2));
    p = (position >> (BOARD_HEIGHT+2)) & (position >> 2*(BOARD_HEIGHT+2));
    r |= p & (position << (BOARD_HEIGHT+2));
    r |= p & (position >> 3*(BOARD_HEIGHT+2));

    return r & (board_mask ^ mask);
}

uint64_t possible(board_t *board)
{
    return (board->mask + bottom_mask) & board_mask;
}

uint64_t winning_position(board_t *board)
{
    return compute_winning_position(board->current_position, board->mask);
}

uint64_t opponent_winning_position(board_t *board)
{
    return compute_winning_position(board->current_position ^ board->mask, board->mask);
}

bool is_winning_move(board_t *board, index_t column)
{
    return winning_position(board) & possible(board) & column_mask(column);
}

int num_moves_taken(board_t *board)
{
    return board->move_count;
}

uint64_t key(board_t *board)
{
    return board->current_position + board->mask;
}

bool alignment(uint64_t pos)
{
    uint64_t m;
   
    // horizontal
    m = pos & (pos >> (BOARD_HEIGHT + 1));
    if (m & (m >> (2 * (BOARD_HEIGHT + 1)))) {
        return true;
    }

    // diagonal 1
    m = pos & (pos >> BOARD_HEIGHT);
    if (m & (m >> (2 * BOARD_HEIGHT))) {
         return true;
    }

    // diagonal 2
    m = pos & (pos >> (BOARD_HEIGHT + 2));
    if (m & (m >> (2 * (BOARD_HEIGHT + 2)))) {
        return true;
    }

    // vertical
    m = pos & (pos >> 1);
    if (m & (m >> 2)) {
        return true;
    }

    return false;
}

bool can_win_next(board_t *board)
{
    return winning_position(board) & possible(board);
}

uint64_t possible_non_losing_moves(board_t *board)
{
    uint64_t possible_mask, opponent_win, forced_moves;
    possible_mask = possible(board);
    opponent_win = opponent_winning_position(board);
    forced_moves = possible_mask & opponent_win;

    if(forced_moves) {
        if(forced_moves & (forced_moves - 1)) {
            return 0;
        } else {
            possible_mask = forced_moves;
        }
    }

    return possible_mask & ~(opponent_win >> 1);
}

outcome_t get_outcome(board_t *board)
{
    uint64_t opponent_pos;
    opponent_pos = board->current_position ^ board->mask;

    if (alignment(board->current_position)) {
        if (board->move_count % 2) {
            return RED_WIN;
        } else {
            return YELLOW_WIN;
        }
    } else if (alignment(opponent_pos)) {
        if (board->move_count % 2) {
            return YELLOW_WIN;
        } else {
            return RED_WIN;
        }
    } else if (board->move_count >= (BOARD_WIDTH * BOARD_HEIGHT)) {
        return DRAW;
    } else {
        return NOT_FINISHED;
    }
}

unsigned int bits_set(uint64_t num)
{
    unsigned int count;

    count = 0;
    while (num) {
        count += 0x1 & num;
        num >>= 1;
    }

    return count;
}

space_t space_at_indices(board_t *board, index_t x, index_t y)
{
    uint64_t space_mask;
    uint8_t space_index;

    if (x >= BOARD_WIDTH || y >= BOARD_HEIGHT) {
        return INVALID;
    }

    if (bits_set(board->current_position) > bits_set(board->mask)) {
        // Invalid board state
        // There should never be a space taken in current_position
        // that's not taken in mask
        return INVALID;
    }

    space_index = (BOARD_WIDTH * x + y);
    space_mask = UINT64_C(1) << space_index;
    
    if (board->mask & space_mask) {
        if (!(board->current_position & space_mask) ^ (board->move_count % 2)) {
            return OCCUPIED_RED;
        } else {
            return OCCUPIED_YELLOW;
        }
    }

    else {
        return EMPTY;
    }    
}

void print_board(board_t *board, FILE *io)
{
    index_t x, y;

    PRINT(CONSOLE_CLEAR);
    PRINT("-Current board-\n");
    for (y = BOARD_HEIGHT; y >= 1; y--)
    {
        PRINT("|");
        for (x = 0; x < BOARD_WIDTH; x++)
        {
            space_t space = space_at_indices(board, x, y - 1);
            switch (space) {
                case OCCUPIED_RED:
                    PRINT("R|");
                    break;
                case OCCUPIED_YELLOW:
                    PRINT("Y|");
                    break;
                case EMPTY:
                    PRINT(" |");
                    break;
                case INVALID:
                    // Print out the board if we get an invalid space
                    // Obviously should escape this string
                    PRINT((char *) board);
                    break;
            }
        }
        PRINT("\n");
    }    

    PRINT("---------------\n");
    PRINT("|0|1|2|3|4|5|6|\n");
}

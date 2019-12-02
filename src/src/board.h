
#ifndef _board_h
#define _board_h

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TILE_HEIGHT 8
#define TILE_WIDTH 8

#define TILE_HEIGHT_1 (TILE_HEIGHT - 1)
#define TILE_WIDTH_1 (TILE_WIDTH - 1)

#define TILE_SIZE TILE_HEIGHT * TILE_WIDTH

#define GET_TILE( __tiles, __i, __j, NTILESC)  (__tiles[(__i / TILE_HEIGHT) * NTILESC + (__j / TILE_WIDTH)])
#define TILE(__tile, __i, __j) (__tile[(__i % TILE_HEIGHT) * TILE_HEIGHT + (__j % TILE_HEIGHT)])

#define _BOARD_(__tiles, __i, __j, NTILESC) (TILE(GET_TILE(__tiles, __i, __j, NTILESC).tile_0, __i, __j))
#define _BOARD_NEXT(__tiles, __i, __j, NTILESC) (TILE(GET_TILE(__tiles, __i, __j, NTILESC).next_tile, __i, __j))
#define _BOARD_2(__tiles, __i, __j, NTILESC) (TILE(GET_TILE(__tiles, __i, __j, NTILESC).tile_2, __i, __j))

typedef struct {
    char* tile_2;
    char* tile_1;
    char* tile_0;
    char* next_tile;
    int tile_row;
    int tile_col;
    int gen;
    char repeating;
    char top_updated;
    char right_updated;
    char bot_updated;
    char left_updated;
    char top_left_updated;
    char top_right_updated;
    char bot_right_updated;
    char bot_left_updated;
} TileState;

void swap_all_tiles(TileState* global_ts, int ntilesr, int ntilesc);

char board(TileState* ts, int i, int j, int ntilesc) {
    int b_i = i / TILE_HEIGHT;
    int b_j = j / TILE_WIDTH;
    int o_i = i % TILE_HEIGHT;
    int o_j = j % TILE_WIDTH;

    return ts[b_i * ntilesc + b_j].tile_0[o_i * TILE_WIDTH + o_j];
}

void print_board(char* board, int nrows, int ncols) {
    printf("Printing board:\n");
    for (int row = 0; row < nrows; row++) {
        for (int col = 0; col < ncols; col++) {
            printf("%d", board[row * ncols + col]);
        }
        printf("\n");
    }
    printf("\n");
}

void print_board_state(TileState *ts) {
    printf("current\n");
    print_board(ts->tile_0, TILE_HEIGHT, TILE_WIDTH);
    printf("prev2\n");
    print_board(ts->tile_2, TILE_HEIGHT, TILE_WIDTH);

}
void print_surrounding_states(TileState* ts, TileState* global_ts, int ntilesc, int ntilesr) {

    int i, i_start, i_end, i_gap1, i_gap2;
    int j, j_start, j_end, j_gap1, j_gap2;

    i_start = mod((ts->tile_row - 1), ntilesr) * TILE_HEIGHT;
    i_end = mod((ts->tile_row + 2),ntilesr) * TILE_HEIGHT;
    j_start = mod ((ts->tile_col - 1), ntilesc) * TILE_WIDTH;
    j_end = mod((ts->tile_col + 2),ntilesc) * TILE_WIDTH;

    i_gap1 = mod((ts->tile_row),ntilesr) * TILE_HEIGHT;
    i_gap2 = mod((ts->tile_row + 1), ntilesr) * TILE_HEIGHT;
    j_gap1 = mod((ts->tile_col), ntilesc) * TILE_WIDTH;
    j_gap2 = mod((ts->tile_col + 1), ntilesc) * TILE_WIDTH;


    printf("i_start %d ", i_start);
    printf("i_end %d ", i_end);
    printf("i_gap1 %d ", i_gap1);
    printf("i_gap2 %d\n", i_gap2);

    printf("j_start %d ", j_start);
    printf("j_end %d ", j_end);
    printf("j_gap1 %d ", j_gap1);
    printf("j_gap2 %d\n", j_gap2);

    i = i_start;
    j = j_start;
    printf("Current Tile:\n");
    while (i != i_end) {
        if (i == i_gap1 || i == i_gap2) {
            printf("\n");
        }
        while (j != j_end) {
            if (j == j_gap1 || j == j_gap2) {
                printf(" ");
            }
            printf("%d", _BOARD_(global_ts,i, j, ntilesc));
            j = (j + 1) % (ntilesc * TILE_HEIGHT);
            
        }
        

        printf("\n");
        i = (i + 1) % (ntilesr * TILE_WIDTH);
        j = j_start;
    }

    i = i_start;
    j = j_start;
    printf("\n2 tiles ago:\n");
    while (i != i_end) {
        if (i == i_gap1 || i == i_gap2) {
            printf("\n");
        }
        while (j != j_end) {
            if (j == j_gap1 || j == j_gap2) {
                printf(" ");
            }
            printf("%d", _BOARD_2(global_ts,i, j, ntilesc));
            j = (j + 1) % (ntilesc * TILE_HEIGHT);
            
        }
        

        printf("\n");
        i = (i + 1) % (ntilesr * TILE_WIDTH);
        j = j_start;
    }
}

char read_board(TileState* ts, int i, int j, int ntilesc, int gen) {
    TileState* s = ts + i / TILE_HEIGHT * ntilesc + j / TILE_WIDTH;
    if (gen == s->gen) {
        return s->tile_0[i % TILE_HEIGHT * TILE_WIDTH + j % TILE_WIDTH];
    } else if (gen + 1 == s->gen) {
        return s->next_tile[i % TILE_HEIGHT * TILE_WIDTH + j % TILE_WIDTH];
    } else {
        printf("WRONG VERSION BUG!\n");
        return 0;
    }
}

void inline
swap_all_tiles(TileState* global_ts, int ntilesr, int ntilesc) {
    int i, j;
    char* temp;
    int id;
    for (i = 0; i < ntilesr; i++) {
        for (j = 0; j < ntilesc; j++) {

            id = i*ntilesc+j;

            temp = global_ts[id].tile_2;
            global_ts[id].tile_2 = global_ts[id].tile_1;
            global_ts[id].tile_1 = global_ts[id].tile_0;
            global_ts[id].tile_0 = global_ts[id].next_tile;
            global_ts[id].next_tile = temp;

        }
    }
}

int* init_gen_board(int ntilesr, int ntilesc) {
    int* gen_tiles = (int*) malloc(ntilesr * ntilesc * sizeof(int));
    memset((char*) (gen_tiles), 0, ntilesr * ntilesc * sizeof(int));
    return gen_tiles;
} 

TileState* init_board(char* board, int nrows, int ncols) {
    int num_tiles_h = ceil(nrows / TILE_HEIGHT);
    int num_tiles_w = ceil(ncols / TILE_WIDTH);
    TileState* res = (TileState* ) malloc(num_tiles_h * num_tiles_w * sizeof(TileState));
    if (res == NULL) {
        return NULL;
    }

    int i, j, ii, jj;
    int is, ie, js, je;
    char* curr_tile;
    char* next_tile;
    char* prev_tile;
    char* pprev_tile;

    int id;

    for (i = 0; i < num_tiles_h; i++) {
        for (j = 0; j < num_tiles_w; j++) {
            pprev_tile = (char*) malloc(TILE_HEIGHT * TILE_WIDTH * sizeof(char));
            prev_tile = (char*) malloc(TILE_HEIGHT * TILE_WIDTH * sizeof(char));
            curr_tile = (char*) malloc(TILE_HEIGHT * TILE_WIDTH * sizeof(char));
            next_tile = (char*) malloc(TILE_HEIGHT * TILE_WIDTH * sizeof(char));

            if (curr_tile == NULL || next_tile == NULL) {
                return NULL;
            }

            id = i * num_tiles_w + j;
            res[id].tile_2 = pprev_tile;
            res[id].tile_1 = prev_tile;
            res[id].tile_0 = curr_tile;
            res[id].next_tile = next_tile;
            res[id].tile_row = i;
            res[id].tile_col = j;
            res[id].gen = 0;
            res[id].repeating = 0;
            res[id].top_updated = 0;
            res[id].right_updated = 0;
            res[id].bot_updated= 0;
            res[id].left_updated= 0;
            res[id].top_left_updated= 0;
            res[id].top_right_updated= 0;
            res[id].bot_right_updated= 0;
            res[id].bot_left_updated= 0;

            is = i * TILE_HEIGHT;
            ie = is + TILE_HEIGHT;
            js = j * TILE_WIDTH;
            je = js + TILE_WIDTH;
            
            for (ii = is; ii < ie; ii++) {
                for (jj = js; jj < je; jj++) {
                    curr_tile[(ii-is)*TILE_WIDTH+(jj-js)] = board[ii * ncols + jj];
                }
            }
        }
    }
    return res;
}


void board_exit(TileState* tiles, char* board, int nrows, int ncols) {
    int ntilesc = ceil((ncols) / TILE_WIDTH);

    int i, j;
    for (i = 0; i < nrows; i++) {
        for (j = 0; j < ncols; j++) {
            board[i * ncols + j] = _BOARD_(tiles, i, j, ntilesc);
        }
    }
}

#endif

#ifndef _board_h
#define _board_h

#include <stdlib.h>
#include <string.h>
#include <math.h>

#define TILE_HEIGHT 8
#define TILE_WIDTH 8

#define GET_TILE( __tiles, __i, __j, NTILESC)  (__tiles[(__i / TILE_HEIGHT) * NTILESC + (__j / TILE_WIDTH)])
#define TILE(__tile, __i, __j) (__tile[(__i % TILE_HEIGHT) * TILE_HEIGHT + (__j % TILE_HEIGHT)])

#define _BOARD_(__tiles, __i, __j, NTILESC) (TILE(GET_TILE(__tiles, __i, __j, NTILESC).tile_0, __i, __j))
#define _BOARD_NEXT(__tiles, __i, __j, NTILESC) (TILE(GET_TILE(__tiles, __i, __j, NTILESC).next_tile, __i, __j))


typedef struct {
    char* tile_0;
    char* next_tile;
    int tile_row;
    int tile_col;
    int gen;
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
    printf("what\n");
    printf("Printing board:\n");
    for (int row = 0; row < nrows; row++) {
        for (int col = 0; col < ncols; col++) {
            printf("%d", board[row * ncols + col]);
        }
        printf("\n");
    }
    printf("\n");
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
            temp = global_ts[id].tile_0;
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

    int id;

    for (i = 0; i < num_tiles_h; i++) {
        for (j = 0; j < num_tiles_w; j++) {
            curr_tile = (char*) malloc(TILE_HEIGHT * TILE_HEIGHT * sizeof(char));
            next_tile = (char*) malloc(TILE_HEIGHT * TILE_HEIGHT * sizeof(char));
            if (curr_tile == NULL || next_tile == NULL) {
                return NULL;
            }
            id = i * num_tiles_w + j;
            res[id].tile_0 = curr_tile;
            res[id].next_tile = next_tile;
            res[id].tile_row = i;
            res[id].tile_col = j;
            res[id].gen = 0;

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
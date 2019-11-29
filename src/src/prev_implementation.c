/*****************************************************************************
 * life.c
 * The original sequential implementation resides here.
 * Do not modify this file, but you are encouraged to borrow from it
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include "save.h"
#include <math.h>
#include "barrier.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>


/**
 * Swapping the two boards only involves swapping pointers, not
 * copying values.
 */
#define SWAP_BOARDS( b1, b2 )  do { \
  char* temp = b1; \
  b1 = b2; \
  b2 = temp; \
} while(0)

#define NUM_THREADS 16

#define SAVE_INTERVAL 1
#define SAVE_START 0

#define BOARD( __board, __i, __j )  (__board[(__i)*LDA + (__j)])
#define _BOARD( __board, __i, __j)  (__board[(__i)*NUM_THREADS + (__j)])

pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t bsync;


int LDA;

char* board_t_2;
char* board_t_1;
char* board_t_0;
char* next_board;

int tile_height = 8;
int tile_width;
int num_tiles_vert;
int num_tiles_hori;

int copy = 0;
int num_cols;

typedef struct {
    int thread_num;
    int nrows;
    int ncols;
    int gens_max;
} worker_struct;

void swap_board() {
    char* temp = board_t_2;
    board_t_2 = board_t_1;
    board_t_1 = board_t_0;
    board_t_0 = next_board;
    next_board = temp;
}

char same_board(char* dst, char* src, int is, int ie, int js, int je, int ncols) {
    int num_cols_copy = je - js;
    int i;
    int start = is * ncols +js;
    char* dst_i = dst + start;
    char* src_i = src + start;

    for (i = is; i < ie; i++) {
        if (memcmp(dst_i, src_i, num_cols_copy) != 0) return 0;
        dst_i += ncols;
        src_i += ncols;
    }
    return 1;
}

void print_board(char* board, int nrows, int ncols) {
    printf("%d", board[0]);
    printf("Printing board:\n");
    for (int row = 0; row < nrows; row++) {
        for (int col = 0; col < ncols; col++) {
            printf("%d", *(board + 1));
        }
        printf("\n");
    }
    printf("\n");
}

void copy_board(char* dst, char* src, int is, int ie, int js, int je, int ncols) {
    int num_cols_copy = je - js;
    int i;
    int start = is * ncols +js;
    char* dst_i = dst + start;
    char* src_i = src + start;

    for (i = is; i < ie; i++) {
        memcpy(dst_i, src_i, num_cols_copy);
        dst_i += ncols;
        src_i += ncols;
    }
}

char tile_should_update(int thread_num, int tile_num, int rows, int curgen) {
    // comparing
    //return 1;
    if (curgen < 200) return 1;
    if (thread_num > 0 && thread_num + 1 < NUM_THREADS && tile_num > 0 && tile_num + 1 < num_tiles_hori) {
        int i_start = tile_num * tile_height - 1;
        int i_end = (tile_num + 1)* tile_height + 1;
        int j_start = thread_num * tile_width - 1;
        int j_end = (thread_num + 1) * tile_width + 1;

        if (same_board(board_t_2, board_t_0, i_start, i_end, j_start, j_end, num_cols)) {
            //printf("Should U: %d %d %d %d\n", i_start, i_end, j_start, j_end);
            return 0;
        } else {
            return 1;
        }
    }
    return 1;
}

void tile_update(int row_bound, int ti, int start, int end, int end_1, int nrows, int ncols, 
        char* inboard, char* outboard) {

    int inorth, isouth, jwest, jeast;
    char q, w, e, a, s, d, z, x, c, neighbor_count;
    int i, j;

    for (i = ti; i < row_bound; i++) {
        j = start;
        inorth = mod (i-1, nrows);
        isouth = mod (i+1, nrows);
        jwest = mod (j-1, ncols);
        jeast = mod (j+1, ncols);

        q = BOARD (inboard, inorth, jwest);
        w = BOARD (inboard, inorth, j);
        e = BOARD (inboard, inorth, jeast);
        a = BOARD (inboard, i, jwest);
        s = BOARD (inboard, i, j);
        d = BOARD (inboard, i, jeast);
        z = BOARD (inboard, isouth, jwest);
        x = BOARD (inboard, isouth, j);
        c = BOARD (inboard, isouth, jeast);

        neighbor_count = q + w + e + a + d + z + x + c;
        BOARD(outboard, i, start) = alivep (neighbor_count, BOARD (inboard, i, start));

        for (j = start+1; j < end; j++)
        {
            jeast = mod (j+1, ncols);

            q = w;
            w = e;
            a = s;
            s = d;
            z = x;
            x = c;

            e = BOARD (inboard, inorth, jeast);
            d = BOARD (inboard, i, jeast);
            c = BOARD (inboard, isouth, jeast);

            neighbor_count = q + w + e + a + d + z + x + c;
            BOARD(outboard, i, j) = alivep (neighbor_count, BOARD (inboard, i, j));
        }
    }
}

void * work(void *args) {
    worker_struct *actual_args = args;
    int nrows = actual_args->nrows;
    int ncols = actual_args->ncols;
    int thread_num = actual_args->thread_num;
    int gens_max = actual_args->gens_max;

    int curgen, ti, row_bound;
    
    int start = ((thread_num * ncols) / NUM_THREADS);
    int end = ((thread_num + 1) * ncols) / NUM_THREADS;
    int end_1 = end - 1;
    if (end > nrows) {
        end = nrows;
    }

    int updated_rows = nrows / tile_height;
    for (curgen = 0; curgen < gens_max; curgen+=1) {
        for (ti = 0; ti < nrows; ti = row_bound) {
            row_bound = ti + tile_height;
            
            if (tile_should_update(thread_num, ti / tile_height, updated_rows, curgen)) {
                tile_update(row_bound, ti, start, end, end_1, nrows, ncols, board_t_0, next_board);

            } else {
                pthread_mutex_lock(&global_lock);
                copy++;
                pthread_mutex_unlock(&global_lock);
                copy_board(board_t_1, next_board, ti, row_bound, start, end, num_cols);
                //printf("copying %d %d %d %d\n", ti, row_bound, start, end);
            }
        }
        pthread_barrier_wait(&bsync);

        if (thread_num == 0) {
            swap_board();
            // if (curgen > SAVE_START && curgen % SAVE_INTERVAL == 0) {
            //     char buf[128];
            //     snprintf(buf, 128, "outputs/copy/1k_%d.pbm", curgen); // puts string into buffer
            //     printf("Saving %d\n", curgen);
            //     FILE* out = fopen (buf, "w");
            //     save_board(out, board_t_0, nrows, ncols);
            //     fclose(out);
            // }
        }
        pthread_barrier_wait(&bsync);
    }
    return NULL;
}

char*
sequential_game_of_life (char* outboard, 
        char* inboard,
        const int nrows,
        const int ncols,
        const int gens_max)
{

    board_t_2 = malloc(nrows * ncols * sizeof(char));
    board_t_1 = malloc(nrows * ncols * sizeof(char));
    board_t_0 = inboard;
    next_board = outboard;

    pthread_t tid[NUM_THREADS];
    pthread_barrier_init(&bsync, NULL, NUM_THREADS);
    LDA = nrows;
    
    tile_width = ceil(ncols / NUM_THREADS);
    num_tiles_vert = ceil(nrows / tile_height);
    num_tiles_hori = NUM_THREADS;

    //printf("%d %d %d %d\n", tile_width, tile_height, num_tiles_vert, num_tiles_hori);

    num_cols = ncols;


    // Init worker arguments
    worker_struct args[NUM_THREADS];

    int i;
    for (i = 0; i < NUM_THREADS; i++) {
        args[i].thread_num = i;
        args[i].nrows = nrows;
        args[i].ncols = ncols;
        args[i].gens_max = gens_max;
    }

    for (i = 0; i < NUM_THREADS; i++) {
        pthread_create(&tid[i], NULL, work, &args[i]);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(tid[i], NULL);
        
    }
    pthread_barrier_destroy(&bsync);
    printf("Copy: %d\n", copy);
    return board_t_0;
}


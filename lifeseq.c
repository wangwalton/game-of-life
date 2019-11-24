/*****************************************************************************
 * life.c
 * The original sequential implementation resides here.
 * Do not modify this file, but you are encouraged to borrow from it
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include <pthread.h>

/**
 * Swapping the two boards only involves swapping pointers, not
 * copying values.
 */
#define SWAP_BOARDS( b1, b2 )  do { \
  char* temp = b1; \
  b1 = b2; \
  b2 = temp; \
} while(0)

#define BOARD( __board, __i, __j )  (__board[(__i) + LDA*(__j)])

NUM_THREADS = 8;
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    char* outboard;
    char* inboard;
    int thread_num;
    int nrows;
    int ncols;
} worker_struct;

void * work(void *args) {
    worker_struct *actual_args = args;
    char* outboard = actual_args->outboard;
    char* inboard = actual_args->inboard;
    int nrows = actual_args->nrows;
    int ncols = actual_args->ncols;
    int thread_num = actual_args->thread_num;
    const int LDA = nrows;

    int i, j;
    int start = (thread_num * nrows) / NUM_THREADS;
    int end = ((thread_num + 1) * nrows) / NUM_THREADS;
    if (end > nrows) {
        end = nrows;
    }

    for (i = start; i < end; i++)
    {


        for (j = 0; j < ncols; j++)
        {
            const int inorth = mod (i-1, nrows);
            const int isouth = mod (i+1, nrows);
            const int jwest = mod (j-1, ncols);
            const int jeast = mod (j+1, ncols);

            const char neighbor_count = 
                BOARD (inboard, inorth, jwest) + 
                BOARD (inboard, inorth, j) + 
                BOARD (inboard, inorth, jeast) + 
                BOARD (inboard, i, jwest) +
                BOARD (inboard, i, jeast) + 
                BOARD (inboard, isouth, jwest) +
                BOARD (inboard, isouth, j) + 
                BOARD (inboard, isouth, jeast);

            BOARD(outboard, i, j) = alivep (neighbor_count, BOARD (inboard, i, j));
        }
        if (i == 0) {
            pthread_mutex_lock(&global_lock);
            printf("Row: %03d Input:\n", i);
            for (j = 0; j < ncols; j++) {
                printf("%d", BOARD(inboard, i, j));
            }
            printf("\n");
            printf("Row: %03d Output:\n", i);
            for (j = 0; j < ncols; j++) {
                printf("%d", BOARD(outboard, i, j));
            }
            printf("\n");
            pthread_mutex_unlock(&global_lock);
        }
    }
}

char*
sequential_game_of_life (char* outboard, 
        char* inboard,
        const int nrows,
        const int ncols,
        const int gens_max)
{
    pthread_t tid[NUM_THREADS];
    
    // Init worker arguments
    worker_struct args[NUM_THREADS];
    int i;
    
    for (i = 0; i < NUM_THREADS; i++) {
        args[i].outboard = outboard;
        args[i].inboard = inboard;
        args[i].thread_num = i;
        args[i].nrows = nrows;
        args[i].ncols = ncols;
    }

    /* HINT: in the parallel decomposition, LDA may not be equal to
       nrows! */

    int curgen;

    for (curgen = 0; curgen < gens_max; curgen++)
    {
        for (i = 0; i < NUM_THREADS; i++) {
            pthread_create(&tid[i], NULL, work, &args[i]);
        }
        for (i = 0; i < NUM_THREADS; i++) {
            pthread_join(tid[i], NULL);
        }
        const int LDA = nrows;
        int j;
        printf("\nIteration %d:\n", curgen);
        printf("Row: %03d Input:\n", 0);
        for (j = 0; j < 50; j++) {
            printf("%d", BOARD(inboard, 0, j));
        }
        printf("\n");
        printf("Row: %03d Output:\n", 0);
        for (j = 0; j < 50; j++) {
            printf("%d", BOARD(outboard, 0, j));
        }
        printf("\n");

        SWAP_BOARDS( outboard, inboard );
    }
    /* 
     * We return the output board, so that we know which one contains
     * the final result (because we've been swapping boards around).
     * Just be careful when you free() the two boards, so that you don't
     * free the same one twice!!! 
     */
    return inboard;
}



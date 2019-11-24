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
#define NUM_THREADS 16

pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t bsync;

typedef struct {
    char* outboard;
    char* inboard;
    int thread_num;
    int nrows;
    int ncols;
    int gens_max;
} worker_struct;

void * work(void *args) {
    worker_struct *actual_args = args;
    char* outboard = actual_args->outboard;
    char* inboard = actual_args->inboard;
    int nrows = actual_args->nrows;
    int ncols = actual_args->ncols;
    int thread_num = actual_args->thread_num;
    int gens_max = actual_args->gens_max;
    const int LDA = nrows;

    int i, j, curgen;
    int start = (thread_num * ncols) / NUM_THREADS;
    int end = ((thread_num + 1) * ncols) / NUM_THREADS;
    if (end > nrows) {
        end = nrows;
    }
    int inorth, isouth, jwest, jeast;
    char q, w, e, a, s, d, z, x, c, neighbor_count;

    for (curgen = 0; curgen < gens_max; curgen++) {
        for (i = 0; i < nrows; i++)
        {
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
        pthread_barrier_wait(&bsync);
        SWAP_BOARDS( outboard, inboard );
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
    pthread_t tid[NUM_THREADS];
    pthread_barrier_init(&bsync, NULL, NUM_THREADS);

    // Init worker arguments
    worker_struct args[NUM_THREADS];

    int i;
    for (i = 0; i < NUM_THREADS; i++) {
        args[i].outboard = outboard;
        args[i].inboard = inboard;
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
    return inboard;
}

/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include "work_queue.h"
#include "board.h"
#include "barrier.h"
#include "util.h"

#include <stdio.h>
#include <pthread.h>
/*****************************************************************************
 * Helper function definitions
 ****************************************************************************/

#define NUM_THREADS 16


// Global variables
pthread_mutex_t global_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_barrier_t bsync;

TileState* global_ts;

WorkQueue* wq;
WorkQueue* nwq;

int num_rows;
int num_cols;
int ntilesc;
int ntilesr;
int max_gens;
int max_gens_1;

/*****************************************************************************
 * Game of life implementation
 ****************************************************************************/

void swap_wq() {
	WorkQueue* temp = wq;
	wq = nwq;
	nwq = temp;
}

void update_tile(TileState* ts) {
	int i, j;
	int el_i, el_j;
	int id;
	char new_val;

	for (i = 0; i < TILE_HEIGHT; i++) {
		for (j = 0; j < TILE_WIDTH; j++) {

			el_i = ts->tile_row * TILE_HEIGHT + i;
			el_j = ts->tile_col * TILE_WIDTH + j;

			
			const int inorth = mod (el_i-1, num_rows);
			const int isouth = mod (el_i+1, num_rows);
			const int jwest = mod (el_j-1, num_cols);
			const int jeast = mod (el_j+1, num_cols);

			const char neighbor_count = 
				board (global_ts, inorth, jwest, ntilesc) + 
				board (global_ts, inorth, el_j, ntilesc) + 
				board (global_ts, inorth, jeast, ntilesc) + 
				board (global_ts, el_i, jwest, ntilesc) +
				board (global_ts, el_i, jeast, ntilesc) + 
				board (global_ts, isouth, jwest, ntilesc) +
				board (global_ts, isouth, el_j, ntilesc) + 
				board (global_ts, isouth, jeast, ntilesc);

			id = i * TILE_WIDTH + j;
			new_val = alivep(neighbor_count, ts->tile_0[id]);
			ts->next_tile[id] = new_val;
		}
	}
}

void* work(void *args) {
	int thread_num = *((int*) (args));
	Node* n;

	while (1) {
		n = deque(wq);
		
		if (n == NULL) {
			pthread_barrier_wait(&bsync);
			if (thread_num == 0) {
				swap_all_tiles(global_ts, ntilesr, ntilesc);
			}
			if (empty(nwq)) break;

			pthread_barrier_wait(&bsync);
			if (thread_num == 0) {
				swap_wq();
			}

			pthread_barrier_wait(&bsync);

		} else {
			// printf("Thread %d executing work tile (%d, %d), gen: %d\n", 
			// 		thread_num, n->w->tile_row,n->w->tile_col,n->w->gen);
			update_tile(n->w);
			n->w->gen += 1;
			if (n->w->gen < max_gens) {
				enque(nwq, n);
			}
		}
	}
	return NULL;
}

char*
game_of_life (char* outboard, 
	      char* inboard,
	      const int nrows,
	      const int ncols,
	      const int gens_max)
{
	// printf("Input board\n");
	// print_board(inboard, nrows, ncols);
	num_rows = nrows;
	num_cols = ncols;
	max_gens = gens_max;
	max_gens_1 = gens_max - 1;

	pthread_t tid[NUM_THREADS];
	pthread_barrier_init(&bsync, NULL, NUM_THREADS);

	ntilesc = nrows / TILE_WIDTH;
	ntilesr = nrows / TILE_HEIGHT;

	//printf("ntilesc: %d ntilesr %d\n", ntilesc, ntilesr);
	global_ts = init_board(inboard, nrows, ncols);

	wq = init_queue_with_work(global_ts, ntilesr, ntilesc);
	nwq = init_queue();

    int args[NUM_THREADS];
	int i;
    for (i = 0; i < NUM_THREADS; i++) {
		args[i] = i;
    	pthread_create(&tid[i], NULL, work, &args[i]);
    }

    for (i = 0; i < NUM_THREADS; i++) {
        pthread_join(tid[i], NULL);
    }

	//print_board(global_ts[0].tile_0, TILE_HEIGHT, TILE_WIDTH);
	pthread_barrier_destroy(&bsync);
	board_exit(global_ts, outboard, nrows, ncols);

	// printf("inboard\n");
	// print_board(inboard, nrows, ncols);

	// printf("tile_0\n");
	// print_board(global_ts[0].tile_0, TILE_HEIGHT, TILE_WIDTH);

	// printf("next_tile\n");
	// print_board(global_ts[0].next_tile, TILE_HEIGHT, TILE_WIDTH);

	// printf("outboard\n");
	// print_board(outboard, nrows, ncols);
	return outboard;
}
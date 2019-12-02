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
#include <assert.h>
/*****************************************************************************
 * Helper function definitions
 ****************************************************************************/

#define NUM_THREADS 16
#define GEN_THRESH 5

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

int total_tiles = 0;
int repeating_tile = 0;
int repeating_tile_init = 0;
int repeating_body = 0;

/*****************************************************************************
 * Game of life implementation
 ****************************************************************************/

char compare_char(char* left, char* right, int len) {
	if (memcmp(left, right, len) == 0) {
		return 1;
	}
	return 0;
}

void inline
atomic_increment(int* count) {
	pthread_mutex_lock(&global_lock);
	(*count)++;
	pthread_mutex_unlock(&global_lock);
}

char top_edge_repeating(TileState* ts) {
	char* current = ts->tile_0;
	char* prev_2 = ts->tile_2;

	if (memcmp(current, prev_2, TILE_WIDTH) == 0) {
		// printf("Top Edge:\ncurrent:\t");
		// for (int i = 0; i < TILE_WIDTH; i++) {
		// 	printf("%d", current[i]);
		// }
		// printf("\nprev_2  :\t");
		// for (int i = 0; i < TILE_WIDTH; i++) {
		// 	printf("%d", prev_2[i]);
		// } printf("\n");
		return 1;
	}

	return 0;
}

char bot_edge_repeating(TileState* ts) {
	char* current = ts->tile_0 + TILE_SIZE - TILE_WIDTH ;
	char* prev_2 = ts->tile_2 + TILE_SIZE - TILE_WIDTH ;
	if (memcmp(current, prev_2, TILE_WIDTH) == 0) return 1;
	return 0;
}

char left_edge_repeating(TileState* ts) {
	char* current = ts->tile_0;
	char* prev_2 = ts->tile_2;

	char* current_max = current + TILE_SIZE;

	while (current < current_max) {
		if (*current != *prev_2) return 0;
		current += TILE_WIDTH;
		prev_2 += TILE_WIDTH;
	}
	return 1;
}

char right_edge_repeating(TileState* ts) {
	char* current = ts->tile_0 + TILE_WIDTH_1;
	char* prev_2 = ts->tile_2 + TILE_WIDTH_1;

	char* current_max = current + TILE_SIZE;

	while (current < current_max) {
		if (*current != *prev_2) return 0;
		current += TILE_WIDTH;
		prev_2 += TILE_WIDTH;
	}

	// if (ts->gen == 30) {
	// 	printf("current\n");
	// 	print_board(ts->tile_0, TILE_WIDTH, TILE_HEIGHT);
	// 	printf("prev\n");
	// 	print_board(ts->tile_2, TILE_WIDTH, TILE_HEIGHT);
	// }
	return 1;
}



void swap_wq() {
	WorkQueue* temp = wq;
	wq = nwq;
	nwq = temp;
}

char surrounding_edge_repeating(TileState* ts) {
	//printf("Starting\n");
	int i = ts->tile_row;
	int j = ts->tile_col;

	int inorth = mod (i-1, ntilesr);
	int isouth = mod (i+1, ntilesr);
	int jwest = mod (j-1, ntilesc);
	int jeast = mod (j+1, ntilesc);

	TileState* top = global_ts + inorth * ntilesc + j;
	TileState* right = global_ts + i * ntilesc + jeast;
	TileState* bot = global_ts + isouth * ntilesc + j;
	TileState* left = global_ts + i * ntilesc + jwest;

	TileState* top_left = global_ts + inorth * ntilesc + jwest;
	TileState* top_right = global_ts + inorth * ntilesc + jeast;
	TileState* bot_left = global_ts + isouth * ntilesc + jwest;
	TileState* bot_right = global_ts + isouth * ntilesc + jeast;
	
	int top_left_id = TILE_SIZE - 1;
	int top_right_id = TILE_HEIGHT_1 * TILE_WIDTH;
	int bot_left_id = TILE_WIDTH_1;
	int bot_right_id = 0;
	
	char top_left_repeating = (top_left->tile_0[top_left_id] == top_left->tile_2[top_left_id]);
	char top_right_repeating = (top_right->tile_0[top_right_id] == top_right->tile_2[top_right_id]);
	char bot_left_repeating = (bot_left->tile_0[bot_left_id] == bot_left->tile_2[bot_left_id]);
	char bot_right_repeating = (bot_right->tile_0[bot_right_id] == bot_right->tile_2[bot_right_id]);
	//printf("ya\n");
	if (top_edge_repeating(bot) && bot_edge_repeating(top)
			&& left_edge_repeating(right) && right_edge_repeating(left)
			&& top_left_repeating && top_right_repeating 
			&& bot_left_repeating && bot_right_repeating) {
		
		// printf("Row/Col\n");
		// printf("(%d, %d) (%d, %d), (%d, %d)\n", top_left->tile_row, top_left->tile_col,
		// 		top->tile_row, top->tile_col, top_right->tile_row, top_right->tile_col);

		// printf("(%d, %d) (%d, %d), (%d, %d)\n", left->tile_row, left->tile_col,
		// 		ts->tile_row, ts->tile_col, right->tile_row, right->tile_col);

		// printf("(%d, %d) (%d, %d), (%d, %d)\n", bot_left->tile_row, bot_left->tile_col,
		// 		bot->tile_row, bot->tile_col, bot_right->tile_row, bot_right->tile_col);	
		// print_surrounding_states(ts, global_ts, ntilesc, ntilesr);
		// printf("Ending\n");
		return 1;
	}
	//printf("Ending\n\n");
	return 0;
}



char update_tile_element(TileState* ts, int tile_i, int tile_j) {
	int i, j;
	int inorth, isouth, jwest, jeast;
	char neighbor_count;
	char new_val;
	int id;

	i = ts->tile_row * TILE_HEIGHT + tile_i;
	j = ts->tile_col * TILE_WIDTH + tile_j;

	inorth = mod (i-1, num_rows);
	isouth = mod (i+1, num_rows);
	jwest = mod (j-1, num_cols);
	jeast = mod (j+1, num_cols);

	neighbor_count = 
		board (global_ts, inorth, jwest, ntilesc) + 
		board (global_ts, inorth, j, ntilesc) + 
		board (global_ts, inorth, jeast, ntilesc) + 
		board (global_ts, i, jwest, ntilesc) +
		board (global_ts, i, jeast, ntilesc) + 
		board (global_ts, isouth, jwest, ntilesc) +
		board (global_ts, isouth, j, ntilesc) + 
		board (global_ts, isouth, jeast, ntilesc);
	
	id = tile_i * TILE_WIDTH + tile_j;
	new_val = alivep(neighbor_count, ts->tile_0[id]);
	if (ts->next_tile[id] != new_val) {
		ts->next_tile[id] = new_val;
		return 1;
	}
	return 0;
}

void update_tile_element_top_left(TileState* ts) {
	ts->top_left_updated = update_tile_element(ts, 0, 0);
}

void update_tile_element_top_right(TileState* ts) {
	ts->top_right_updated = update_tile_element(ts, 0, TILE_WIDTH_1);
}

void update_tile_element_bot_left(TileState* ts) {
	ts->bot_left_updated = update_tile_element(ts, TILE_HEIGHT_1, 0);
}

void update_tile_element_bot_right(TileState* ts) {
	ts->bot_right_updated = update_tile_element(ts, TILE_HEIGHT_1, TILE_WIDTH_1);
}

void update_tile_element_top(TileState* ts) {
	char updated = 0;
	int i;
	for (i = 1; i < TILE_WIDTH_1; i++) {
		if (update_tile_element(ts, 0, i)) {
			updated = 1;
		}
	}
	ts->top_updated = updated;
}

void update_tile_element_right(TileState* ts) {
	char updated = 0;
	int i;
	for (i = 1; i < TILE_HEIGHT_1; i++) {
		if (update_tile_element(ts, i, TILE_WIDTH_1)) {
			updated = 1;
		}
	}
	ts->right_updated = updated;
}

void update_tile_element_bot(TileState* ts) {
	char updated = 0;
	int i;
	for (i = 1; i < TILE_WIDTH_1; i++) {
		if (update_tile_element(ts, TILE_HEIGHT_1, i)) {
			updated = 1;
		}
	}
	ts->bot_updated = updated;
}

void update_tile_element_left(TileState* ts) {
	char updated = 0;
	int i;
	for (i = 1; i < TILE_HEIGHT_1; i++) {
		if (update_tile_element(ts, i, 0)) {
			updated = 1;
		}
	}
	ts->left_updated = updated;
}

void update_tile_element_body(TileState* ts) {
	int i, j;
	for (i = 1; i < TILE_HEIGHT_1; i++) {
		for (j = 1; j < TILE_WIDTH_1; j++) {
			update_tile_element(ts, i, j);
		}
	}
}
void update_tile_edge(TileState* ts) {
	update_tile_element_top_left(ts);
	update_tile_element_top_right(ts);
	update_tile_element_bot_left(ts);
	update_tile_element_bot_right(ts);
	update_tile_element_top(ts);
	update_tile_element_left(ts);
	update_tile_element_bot(ts);
	update_tile_element_right(ts);
}

void update_tile(TileState* ts) {
	update_tile_element_top_left(ts);
	update_tile_element_top_right(ts);
	update_tile_element_bot_left(ts);
	update_tile_element_bot_right(ts);
	update_tile_element_top(ts);
	update_tile_element_left(ts);
	update_tile_element_bot(ts);
	update_tile_element_right(ts);
	update_tile_element_body(ts);
}

void process_tile(TileState* ts) {
	// update_tile(ts);
	// return;
	//printf("process t\n");
	atomic_increment(&total_tiles);
	if (!ts->repeating) {
		if (ts->gen > GEN_THRESH && compare_char(ts->tile_0, ts->tile_2, TILE_SIZE) == 1
				&& surrounding_edge_repeating(ts)) {
			//printf("\t\tSurrounding repeating\n");
			memcpy(ts->next_tile, ts->tile_1, TILE_SIZE);
			ts->repeating = 1;
			atomic_increment(&repeating_tile_init);
		} else {
			update_tile(ts);
		}
	} else {
		if (!surrounding_edge_repeating(ts)) {
			ts->repeating = 0;
			update_tile_edge(ts);
			atomic_increment(&repeating_body);
		} else {
			atomic_increment(&repeating_tile);
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
			process_tile(n->w);
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
	// print_board(inboard, nrows, ncols);
	num_rows = nrows;
	num_cols = ncols;
	max_gens = gens_max;
	max_gens_1 = gens_max - 1;

	pthread_t tid[NUM_THREADS];
	pthread_barrier_init(&bsync, NULL, NUM_THREADS);

	ntilesc = nrows / TILE_WIDTH;
	ntilesr = nrows / TILE_HEIGHT;

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

	pthread_barrier_destroy(&bsync);
	board_exit(global_ts, outboard, nrows, ncols);
	printf("total_tiles: %d, repeating_tile %d\n", total_tiles, repeating_tile);
	printf("repeating_tile_init: %d, repeating_body %d\n", repeating_tile_init, repeating_body);
	return outboard;
}
/*****************************************************************************
 * life.c
 * Parallelized and optimized implementation of the game of life resides here
 ****************************************************************************/
#include "life.h"
#include "util.h"
#include "work_queue.h"
#include "board.h"
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

int ntilesc_m_ntilesr_s1;
int ntilesc_s1;
int ntilesr_s1;

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

void
atomic_increment(int* count) {
	pthread_mutex_lock(&global_lock);
	(*count)++;
	pthread_mutex_unlock(&global_lock);
}

char top_edge_repeating(TileState* ts) {
	if (ts->repeating) return 1;
	char* current = ts->tile_0;
	char* prev_2 = ts->tile_2;

	if (memcmp(current, prev_2, TILE_WIDTH) == 0) {
		return 1;
	}

	return 0;
}

char bot_edge_repeating(TileState* ts) {
	if (ts->repeating) return 1;
	char* current = ts->tile_0 + TILE_SIZE - TILE_WIDTH ;
	char* prev_2 = ts->tile_2 + TILE_SIZE - TILE_WIDTH ;
	if (memcmp(current, prev_2, TILE_WIDTH) == 0) return 1;
	return 0;
}

char left_edge_repeating(TileState* ts) {
	if (ts->repeating) return 1;
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
	if (ts->repeating) return 1;
	char* current = ts->tile_0 + TILE_WIDTH_1;
	char* prev_2 = ts->tile_2 + TILE_WIDTH_1;

	char* current_max = current + TILE_SIZE;

	while (current < current_max) {
		if (*current != *prev_2) return 0;
		current += TILE_WIDTH;
		prev_2 += TILE_WIDTH;
	}
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

	if (top_left_repeating && top_right_repeating 
			&& bot_left_repeating && bot_right_repeating && top_edge_repeating(bot) && bot_edge_repeating(top)
			&& left_edge_repeating(right) && right_edge_repeating(left)){
		
		return 1;
	}

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

void update_tile_element_body(TileState* ts) {
	char q, w, e, a, s, d, z, x, c;
	int i;
	char neighbor_count;
	char* top_left = ts->tile_0;
	char* new_cell = ts->next_tile + TILE_WIDTH_P1;

	char* cap;
	char* start;
	char* i_cap = top_left + TILE_SIZE;

	for (i = 0; i < TILE_WIDTH_S2; i++) {
		q = *(top_left);
		w = *(top_left+1);
		e = *(top_left+2);
		a = *(top_left+TILE_WIDTH);
		s = *(top_left+TILE_WIDTH_P1);
		d = *(top_left+TILE_WIDTH_P2);

		z = *(top_left+TILE_WIDTH_M2);
		x = *(top_left+TILE_WIDTH_M2_P1);
		c = *(top_left+TILE_WIDTH_M2_P2);

		cap = top_left + TILE_WIDTH_S2;

		while (top_left < cap) {
			neighbor_count = q + w + e + a + d + z + x + c;

			*new_cell = alivep(neighbor_count, s);
			new_cell++;
			top_left++;
			q = w;
			w = e;
			a = s;
			s = d;
			z = x;
			x = c;
			e = *(top_left+2);
			d = *(top_left+TILE_WIDTH_P2);
			c = *(top_left+TILE_WIDTH_M2_P2);

		}
		top_left += 2;
		new_cell += 2;
	}
}

void update_tile_element_top_left(TileState* ts) {
	update_tile_element(ts, 0, 0);
}

void update_tile_element_top_right(TileState* ts) {
	update_tile_element(ts, 0, TILE_WIDTH_1);
}

void update_tile_element_bot_left(TileState* ts) {
	update_tile_element(ts, TILE_HEIGHT_1, 0);
}

void update_tile_element_bot_right(TileState* ts) {
	update_tile_element(ts, TILE_HEIGHT_1, TILE_WIDTH_1);
}

void update_tile_element_top(TileState* ts) {
	TileState* other;
	if (ts->tile_row == 0) {
		other = ts + ntilesc_m_ntilesr_s1;
	} else {
		other = ts - ntilesc;
	}

	char q, w, e, a, s, d, z, x, c;
	int i;
	char neighbor_count;
	char* mid = ts->tile_0;
	char* top = other->tile_0 + TILE_SIZE_SWIDTH;
	char* new_cell = ts->next_tile + 1;
	q = *(top);
	w = *(top+1);
	e = *(top+2);

	a = *(mid);
	s = *(mid+1);
	d = *(mid+2);

	z = *(mid+TILE_WIDTH);
	x = *(mid+TILE_WIDTH_P1);
	c = *(mid+TILE_WIDTH_P2);

	for (i = 1; i < TILE_HEIGHT_1; i++) {

		neighbor_count = q + w + e + a + d + z + x + c;

		*new_cell = alivep(neighbor_count, s);

		mid++;
		top++;
		new_cell++;

		q = w;
		w = e;
		a = s;
		s = d;
		z = x;
		x = c;
		e = *(top+2);
		d = *(mid+2);
		c = *(mid+TILE_WIDTH_P2);
	}
}

void update_tile_element_right(TileState* ts) {
	TileState* other;
	if (ts->tile_col == ntilesc_s1) {
		other = ts - ntilesc_s1;
	} else {
		other = ts + 1;
	}

	char q, w, e, a, s, d, z, x, c;
	int i;
	char neighbor_count;

	char* left = ts->tile_0 + TILE_WIDTH_S2;
	char* right = other->tile_0;
	char* new_cell = ts->next_tile + TILE_WIDTH_M2_S1;

	q = *(left);
	a = *(left+TILE_WIDTH);
	z = *(left+TILE_WIDTH_M2);

	w = *(left+1);
	s = *(left+TILE_WIDTH_P1);
	x = *(left+TILE_WIDTH_M2_P1);

	e = *(right);
	d = *(right+TILE_WIDTH);
	c = *(right+TILE_WIDTH_M2);

	for (i = 0; i < TILE_WIDTH_S2; i++) {

		neighbor_count = q + w + e + a + d + z + x + c;

		*new_cell = alivep(neighbor_count, s);

		right += TILE_WIDTH;
		left += TILE_WIDTH;
		new_cell += TILE_WIDTH;
		
		q = a;
		a = z;
		w = s;
		s = x; 
		e = d;
		d = c;
		z = *(left+TILE_WIDTH_M2);
		x = *(left+TILE_WIDTH_M2_P1);
		c = *(right+TILE_WIDTH_M2);

	}
}

void update_tile_element_bot(TileState* ts) {
	TileState* other;
	if (ts->tile_row == ntilesr_s1) {
		other = ts - ntilesc_m_ntilesr_s1;
	} else {
		other = ts + ntilesc;
	}

	char q, w, e, a, s, d, z, x, c;
	int i;
	char neighbor_count;

	char* top = ts->tile_0 + TILE_SIZE_M2SWIDTH;
	char* bot = other->tile_0;
	char* new_cell = ts->next_tile + TILE_SIZE_SWIDTH_P1;

	q = *(top);
	w = *(top+1);
	e = *(top+2);

	a = *(top+TILE_WIDTH);
	s = *(top+TILE_WIDTH_P1);
	d = *(top+TILE_WIDTH_P2);

	z = *(bot);
	x = *(bot+1);
	c = *(bot+2);

	for (i = 1; i < TILE_HEIGHT_1; i++) {

		neighbor_count = q + w + e + a + d + z + x + c;

		*new_cell = alivep(neighbor_count, s);

		bot++;
		top++;
		new_cell++;

		q = w;
		w = e;
		a = s;
		s = d;
		z = x;
		x = c;
		e = *(top+2);
		d = *(top+TILE_WIDTH_P2);
		c = *(bot+2);
	}
}

void update_tile_element_left(TileState* ts) {
	TileState* other;
	if (ts->tile_col == 0) {
		other = ts + ntilesc_s1;
	} else {
		other = ts - 1;
	}

	char q, w, e, a, s, d, z, x, c;
	int i;
	char neighbor_count;

	char* left = other->tile_0 + TILE_WIDTH_1;
	char* mid = ts->tile_0;
	char* new_cell = ts->next_tile + TILE_WIDTH;

	q = *(left);
	a = *(left+TILE_WIDTH);
	z = *(left+TILE_WIDTH_M2);

	w = *(mid);
	s = *(mid+TILE_WIDTH);
	x = *(mid+TILE_WIDTH_M2);

	e = *(mid+1);
	d = *(mid+TILE_WIDTH_P1);
	c = *(mid+TILE_WIDTH_M2_P1);

	for (i = 1; i < TILE_HEIGHT_1; i++) {
		neighbor_count = q + w + e + a + d + z + x + c;

		*new_cell = alivep(neighbor_count, s);

		mid+= TILE_WIDTH;
		left+= TILE_WIDTH;
		new_cell += TILE_WIDTH;
		
		q = a;
		a = z;
		w = s;
		s = x; 
		e = d;
		d = c;

		z = *(left+TILE_WIDTH_M2);
		x = *(mid+TILE_WIDTH_M2);
		c = *(mid+TILE_WIDTH_M2_P1);

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
	atomic_increment(&total_tiles);
	if (!ts->repeating) {
		if (ts->gen > GEN_THRESH && compare_char(ts->tile_0, ts->tile_2, TILE_SIZE) == 1
				&& surrounding_edge_repeating(ts)) {
			memcpy(ts->next_tile, ts->tile_1, TILE_SIZE);
			ts->next_repeating = 1;
			atomic_increment(&repeating_tile_init);
		} else {
			update_tile(ts);
		}
	} else {
		if (!surrounding_edge_repeating(ts)) {
			ts->next_repeating = 0;
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

			if (empty(nwq)) break;
			pthread_barrier_wait(&bsync);
			if (thread_num == 0) {
				swap_all_tiles(global_ts, ntilesr, ntilesc);
			}
			pthread_barrier_wait(&bsync);
			if (thread_num == 0) {
				swap_wq();
			}

			pthread_barrier_wait(&bsync);

		} else {
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
	ntilesc_m_ntilesr_s1 = ntilesc * (ntilesr - 1);
	ntilesc_s1 = ntilesc - 1;
	ntilesr_s1 = ntilesr - 1;

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
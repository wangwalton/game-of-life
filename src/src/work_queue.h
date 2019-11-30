
#ifndef _work_queue_h
#define _work_queue_h

#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

#include "board.h"

typedef struct Node {
	TileState *w;
	struct Node* next;
	struct Node* prev;
} Node;

typedef struct {
	Node* head;
	Node* tail;
	pthread_mutex_t queue_lock;
} WorkQueue;

void insert_head(WorkQueue* wq, Node* newNode);

//Creates a new Node and returns pointer to it. 
Node* new_node(TileState* w) {
	Node* newNode = (Node*) malloc(sizeof(Node));
    newNode->w = w;
	newNode->prev = NULL;
	newNode->next = NULL;
	return newNode;
}

WorkQueue* init_queue() {
	WorkQueue* wq = (WorkQueue*) malloc(sizeof(WorkQueue));
	TileState* temp_work = NULL;
	pthread_mutex_init(&(wq->queue_lock), NULL);
	wq->head = new_node(temp_work);
	wq->tail = new_node(temp_work);
	wq->head->next = wq->tail;
	wq->tail->prev = wq->head;
	return wq;
}

WorkQueue* init_queue_with_work(TileState* global_ts, int ntilesr, int ntilesc) {
	WorkQueue* wq = init_queue();
	TileState* ts;
	int i, j;
	for (i = 0; i < ntilesr; i++) {
		for (j = 0; j < ntilesc; j++) {
			ts = &global_ts[i * ntilesc + j];
			insert_head(wq, new_node(ts));
		}
	}
	return wq;
}

char empty(WorkQueue* wq) {
	pthread_mutex_lock(&(wq->queue_lock));
	char res = (wq->head->next == wq->tail);
	pthread_mutex_unlock(&(wq->queue_lock));
	return res;
}

//Inserts a Node at head of doubly linked list
void insert_head(WorkQueue* wq, Node* newNode) {
	pthread_mutex_lock(&(wq->queue_lock));
	newNode->next = wq->head->next;
	newNode->prev = wq->head;
	wq->head->next->prev = newNode;
	wq->head->next = newNode;
	pthread_mutex_unlock(&(wq->queue_lock));
}

Node* extract_tail(WorkQueue* wq) {
	pthread_mutex_lock(&(wq->queue_lock));
	if (wq->head->next == wq->tail) {
		pthread_mutex_unlock(&(wq->queue_lock));
		return NULL;
	}
	Node* res = wq->tail->prev;
	wq->tail->prev = res->prev;
	res->prev->next = wq->tail;
	res->next = NULL;
	res->prev = NULL;
	pthread_mutex_unlock(&(wq->queue_lock));
	return res;
}

void enque(WorkQueue* wq, Node* n) {
	insert_head(wq, n);
}

Node* deque(WorkQueue* wq) {
	return extract_tail(wq);
}

void print_queue_head(WorkQueue* wq) {
	printf("Printing wq from head:\n");
	if (wq->head->next == wq->tail) return;
	Node* temp = wq->head->next;
	while (temp->next != wq->tail) {
		printf("(%d, %d, %d) ", temp->w->tile_row, temp->w->tile_col, temp->w->gen);
		temp = temp->next;
	}
	printf("\n");
}

void print_queue_tail(WorkQueue* wq) {
	printf("Printing wq from tail:\n");
	if (wq->head->next == wq->tail) return;
	Node* temp = wq->tail->prev;
	while (temp->prev != wq->head) {
		printf("(%d, %d, %d) ", temp->w->tile_row, temp->w->tile_col, temp->w->gen);
		temp = temp->prev;
	}
	printf("\n");
}

#endif
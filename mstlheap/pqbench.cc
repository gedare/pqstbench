/*
 * Copyright 2014 Gedare Bloom (gedare@rtems.org)
 *
 * This file's license is 2-clause BSD as in this distribution's LICENSE.2 file.
 */

#include "../shared/pqbench.h"

/* PQ implementation */
#include "mstlheap.h"

#include "../shared/params.h"
static pq_t* the_heap[NUM_APERIODIC_TASKS];

/* test interface */
void pq_initialize( rtems_task_argument tid, int size ) { 
 the_heap[tid] = new pq_t();
}

void pq_insert( rtems_task_argument tid, long p ) {
  the_heap[tid]->insert(p);
}

//void pq_extract( pq_node *n ) { heap_remove(n->hIndex); }

long pq_first( rtems_task_argument tid ) {
  return the_heap[tid]->first();
}

long pq_pop( rtems_task_argument tid ) {
  return the_heap[tid]->pop();
}

long pq_search( rtems_task_argument tid, int key ) {
  return the_heap[tid]->search(key);
}

long pq_extract( rtems_task_argument tid, int key ) {
  return the_heap[tid]->extract(key);
}


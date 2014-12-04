/*
 * Copyright 2014 Gedare Bloom (gedare@rtems.org)
 *
 * This file's license is 2-clause BSD as in this distribution's LICENSE.2 file.
 */

#include "../shared/pqbench.h"

/* PQ implementation */
#include "miheap.h"

/* test interface */
void pq_initialize(rtems_task_argument tid, int size ) { 
  heap_initialize(tid,size);
}

void pq_insert( rtems_task_argument tid, uint64_t p ) {
  heap_insert(tid,p); 
}

uint64_t pq_first( rtems_task_argument tid ) {
  return heap_min(tid);
}

uint64_t pq_pop( rtems_task_argument tid  ) {
  return heap_pop_min(tid);
}

uint64_t pq_search( rtems_task_argument tid, int key ) {
  return heap_search(tid, key);
}

uint64_t pq_extract( rtems_task_argument tid, int key ) {
  return heap_extract(tid, key);
}

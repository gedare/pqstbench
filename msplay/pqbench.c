/*
 * Copyright 2014 Gedare Bloom (gedare@rtems.org)
 *
 * This file's license is 2-clause BSD as in this distribution's LICENSE.2 file.
 */

#include "../shared/pqbench.h"

/* PQ implementation */
#include "msplay.h"

/* test interface */
void pq_initialize(rtems_task_argument tid, int size ) { 
  splay_initialize(tid,size);
}

void pq_insert(rtems_task_argument tid, uint64_t p ) {
  splay_insert(tid,p); 
}

uint64_t pq_first( rtems_task_argument tid ) {
  return splay_min(tid);
}

uint64_t pq_pop( rtems_task_argument tid ) {
  return splay_pop_min(tid);
}

uint64_t pq_search( rtems_task_argument tid, int key ) {
  return splay_search(tid, key);
}

uint64_t pq_extract( rtems_task_argument tid, int key ) {
  return splay_extract(tid, key);
}

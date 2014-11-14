/*
 * Copyright 2014 Gedare Bloom (gedare@rtems.org)
 *
 * This file's license is 2-clause BSD as in this distribution's LICENSE.2 file.
 */

#include "../shared/pqbench.h"

/* PQ implementation */
#include "mlist.h"

/* test interface */
void pq_initialize(rtems_task_argument tid, int size ) { 
  list_initialize(tid,size);
}

void pq_insert( rtems_task_argument tid, long p ) {
  list_insert(tid,p); 
}

long pq_first( rtems_task_argument tid ) {
  return list_min(tid);
}

long pq_pop( rtems_task_argument tid  ) {
  return list_pop_min(tid);
}

long pq_search( rtems_task_argument tid, int key ) {
  return list_search(tid, key);
}

long pq_extract( rtems_task_argument tid, int key ) {
  return list_extract(tid, key);
}

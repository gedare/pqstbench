/*
 * Copyright 2014 Gedare Bloom (gedare@rtems.org)
 *
 * This file's license is 2-clause BSD as in this distribution's LICENSE.2 file.
 */
/*
 * linked list implementation for priority queue
 */

#ifndef __MLIST_H_
#define __MLIST_H_

#include "../shared/pqbench.h"

#include <rtems.h>
#include "rtems/chain.h"

typedef struct {
  int key;
  int val;
} pq_node;

typedef struct {
  rtems_chain_node link;
  pq_node data;
} node;

// container-of magic
#define PQ_NODE_TO_NODE(hn) \
  ((node*)((uintptr_t)hn - ((uintptr_t)(&((node *)0)->data))))

#define PQ_NODE_TO_KV(n) ((((uint64_t)(n)->key) << (sizeof(uint64_t)*4L)) | (uint64_t)(n)->val)

void list_initialize( rtems_task_argument tid, int size );
void list_insert( rtems_task_argument tid, uint64_t kv );
uint64_t list_min( rtems_task_argument tid );
uint64_t list_pop_min( rtems_task_argument tid );
uint64_t list_search( rtems_task_argument tid, int key );
uint64_t list_extract( rtems_task_argument tid, int key );

#endif

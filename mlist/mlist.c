/*
 * Copyright 2014 Gedare Bloom (gedare@rtems.org)
 *
 * This file's license is 2-clause BSD as in this distribution's LICENSE.2 file.
 */

#include "mlist.h"

#include <stdlib.h>
#include <tmacros.h>
#include <rtems/chain.h>
#include "../shared/params.h"

const char rtems_test_name[] = "PQST BENCHMARK LIST";

/* data structure */
static rtems_chain_control the_list[NUM_APERIODIC_TASKS];
/* data */
static node* the_nodes[NUM_APERIODIC_TASKS];
/* free storage */
rtems_chain_control freenodes[NUM_APERIODIC_TASKS];

/* helpers */
static node *alloc_node(rtems_task_argument tid) {
  node *n = (node*)rtems_chain_get_unprotected( &freenodes[tid] );
  return n;
}
static void free_node(rtems_task_argument tid, node *n) {
  rtems_chain_append_unprotected( &freenodes[tid], &n->link );
}

static inline void initialize_helper(rtems_task_argument tid, int size)
{
  rtems_chain_initialize_empty ( &the_list[tid] );
}

static inline void insert_helper(rtems_task_argument tid, node *before, node *n)
{
  rtems_chain_insert_unprotected(&before->link, &n->link);
}

/* Returns node with same key, first key greater, or tail of list */
static node* search_helper(rtems_task_argument tid, int key)
{
  rtems_chain_node *iter;
  rtems_chain_control *list;
  
  list = &the_list[tid];
  iter = rtems_chain_first(list); // unprotected
  while ( !rtems_chain_is_tail(list, iter) ) {
    node *n = (node*)iter;
    if (n->data.key >= key) {
      return n;
    }
    iter = rtems_chain_next(iter);
  }
  return (node*)iter;
}

static inline void extract_helper(rtems_task_argument tid, node *n) {
  rtems_chain_extract_unprotected(&n->link);
  free_node(tid, n);
}

/**
 * benchmark interface
 */
void list_initialize( rtems_task_argument tid, int size ) {
  int i;

  the_nodes[tid] = (node*)malloc(sizeof(node)*size);
  if ( ! the_nodes[tid] ) {
    printk("failed to alloc nodes\n");
    while(1);
  }

  rtems_chain_initialize(&freenodes[tid], the_nodes[tid], size, sizeof(node));

  initialize_helper(tid, size);
}

void list_insert(rtems_task_argument tid, uint64_t kv ) {
  node *new_node = alloc_node(tid);
  node *target;
  int key = kv_key(kv);

  target = search_helper(tid, key);
  target = (node*)rtems_chain_previous(&target->link);

  new_node->data.key = kv_key(kv);
  new_node->data.val = kv_value(kv);
  insert_helper(tid, target, new_node);
}

uint64_t list_min( rtems_task_argument tid ) {
  node *n;

  n = (node*)rtems_chain_first(&the_list[tid]); // unprotected
  if (n) {
    return PQ_NODE_TO_KV(&n->data);
  }
  return (uint64_t)-1;
}

uint64_t list_pop_min( rtems_task_argument tid ) {
  uint64_t kv;
  node *n;
  n = (node*)rtems_chain_get_unprotected(&the_list[tid]);
  if (n) {
    kv = PQ_NODE_TO_KV(&n->data);
    free_node(tid, n);
  } else {
    kv = (uint64_t)-1;
  }
  return kv;
}

uint64_t list_search( rtems_task_argument tid, int k ) {
  node* n = search_helper(tid, k);
  if (!rtems_chain_is_tail(&the_list[tid], &n->link)) {
    return PQ_NODE_TO_KV(&n->data);
  }
  return (uint64_t)-1;
}

uint64_t list_extract( rtems_task_argument tid, int k ) {
  node* n = search_helper(tid, k);
  uint64_t kv;
  if (!rtems_chain_is_tail(&the_list[tid], &n->link) && n->data.key == k) {
    kv = PQ_NODE_TO_KV(&n->data);
    extract_helper(tid, n);
  } else {
    kv = (uint64_t)-1;
  }
  return kv;
}


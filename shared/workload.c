/*
 * Copyright 2014 Gedare Bloom (gedare@rtems.org)
 *
 * This file's license is 2-clause BSD as in this distribution's LICENSE.2 file.
 */
/*
 * PQ workload
 */

#include "pqbench.h"
#include "workload.h"
#include "params.h"
#include "params.i"

#include <stdlib.h>

#define ARG_TO_LONG(n) ((((uint64_t)n->key) << (sizeof(uint64_t)*4L)) | (uint64_t)n->val)

inline uint64_t pq_add_to_key(uint64_t p, uint32_t i) {
  return p+((uint64_t)i<<(sizeof(uint64_t)*4L));
}

inline uint64_t pq_node_initialize( PQ_arg *a ) {
  return ARG_TO_LONG(a);
}

#if defined(GAB_PRINT) || defined(GAB_DEBUG)
inline void pq_print_node( uint64_t p ) {
  printk("%d\t%d\n", kv_key(p), kv_value(p));
}
#endif

static int execute( rtems_task_argument tid, int current_op );
static void drain_and_check(rtems_task_argument tid);
static int measure( rtems_task_argument tid, int current_op );

void initialize(rtems_task_argument tid ) {
  pq_initialize(tid, PQ_MAX_SIZE);
}

void warmup( rtems_task_argument tid ) {
  int i = 0;

#if defined GAB_PRINT
  printk("%d\tWarmup: %d\n",tid, PQ_WARMUP_OPS[tid]);
#endif
  for ( i = 0; i < PQ_WARMUP_OPS[tid]; i++ ) {
    execute(tid, i);
  }
}

void work( rtems_task_argument tid  ) {
  int i = 0;
#if defined(GAB_PRINT)
  printk("%d\tWork: %d\n",tid, PQ_WORK_OPS[tid]);
#endif

  for ( i = 0; i < PQ_WORK_OPS[tid]; i++ ) {
    execute(tid, PQ_WARMUP_OPS[tid] + i);
  }

#if defined(GAB_CHECK)
  drain_and_check(tid);
#endif
}

static int execute( rtems_task_argument tid, int current_op ) {
  uint64_t n;

  switch (ops[tid][current_op]) {
    case f:
      n = pq_first(tid);
#if defined(GAB_PRINT)
      printk("%d\tPQ first:\t",tid);
      pq_print_node(p);
#endif
      break;
    case i:
      n = pq_node_initialize( &args[tid][current_op] );
      pq_insert( tid, n );
#if defined(GAB_PRINT)
      printk("%d\tPQ insert (args[tid]=%d,%d):\t",
          tid, args[tid][current_op].key, args[tid][current_op].val);
      pq_print_node(n);
#endif
      break;
    case p:
      n = pq_pop(tid);
#if defined(GAB_DEBUG)
      if ( kv_key(n) != args[tid][current_op].key ) {
        printk("%d\tInvalid node popped (args[tid]=%d,%d):\t",
            tid, args[tid][current_op].key, args[tid][current_op].val);
        pq_print_node(n);
      }
#endif
#if defined(GAB_PRINT) && defined(GAB_DEBUG)
      if ( kv_value(n) != args[tid][current_op].val ) {
        printk("%d\tUnexpected node (non-FIFO/stable) popped (args[tid]=%d,%d):\t",
            tid,args[tid][current_op].key, args[tid][current_op].val);
        pq_print_node(n);
      }
#endif
#if defined(GAB_PRINT)
      printk("%d\tPQ pop (args[tid]=%d,%d):\t",tid, args[tid][current_op].key, args[tid][current_op].val);
      pq_print_node(n);
#endif
      break;
    case h:
        n = pq_pop(tid);
#if defined(GAB_DEBUG)
      if ( kv_key(n) != args[tid][current_op].val ) {
        printk("%d\tInvalid node popped (args[tid]=%d,%d):\t",tid, args[tid][current_op].key, args[tid][current_op].val);
        pq_print_node(n);
      }
#endif
      n = pq_add_to_key(n, args[tid][current_op].key);/* add to prio */
#if defined(GAB_PRINT)
      printk("%d\tPQ hold (args[tid]=%d,%d):\t",tid, args[tid][current_op].key, args[tid][current_op].val);
      pq_print_node(n);
#endif
      pq_insert(tid,n);
      break;
    case s:
      n = pq_search(tid, args[tid][current_op].key); // UNIQUE!
#if defined(GAB_DEBUG)
      if (kv_value(n) != args[tid][current_op].val) {
        printk("%d\tInvalid node found (args[tid]=%d,%d):\t",
            tid, args[tid][current_op].key, args[tid][current_op].val);
        pq_print_node(n);
      }
#endif
#if defined(GAB_PRINT)
      printk("%d\tPQ search (args[tid]=%d,%d):\t",
          tid, args[tid][current_op].key, args[tid][current_op].val);
      pq_print_node(n);
#endif
      break;
    case x:
      n = pq_extract(tid, args[tid][current_op].key);
#if defined(GAB_DEBUG)
      if (kv_value(n) != args[tid][current_op].val) {
        printk("%d\tInvalid node found (args[tid]=%d,%d):\t",
            tid, args[tid][current_op].key, args[tid][current_op].val);
        pq_print_node(n);
      }
#endif
#if defined(GAB_PRINT)
      printk("%d\tPQ extract (args[tid]=%d,%d):\t",
          tid, args[tid][current_op].key, args[tid][current_op].val);
      pq_print_node(n);
#endif
      break;
    default:
#if defined(GAB_PRINT)
      printk("%d\tInvalid Op: %d\n",tid,ops[tid][current_op]);
#endif
      break;
  }
  current_op++;
}

static void drain_and_check(rtems_task_argument tid) {
  uint64_t s = 0;

  uint64_t n;
  while ((n = pq_pop(tid)) != (uint64_t)-1) { // FIXME: casting -1 :(
    s = s + kv_key(n);
  }
  printk("%d\tChecksum: 0x%llX\n", tid, s);

}


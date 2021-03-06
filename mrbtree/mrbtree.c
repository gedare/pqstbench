/*
 * Copyright 2014 Gedare Bloom (gedare@rtems.org)
 *
 * This file's license is 2-clause BSD as in this distribution's LICENSE.2 file.
 */

#include "mrbtree.h"

#include <stdlib.h>

const char rtems_test_name[] = "PQST BENCHMARK RED-BLACK TREE";

node the_nodes[NUM_NODES][NUM_APERIODIC_TASKS];
rtems_rbtree_control the_rbtree[NUM_APERIODIC_TASKS];
rtems_chain_control freelist[NUM_APERIODIC_TASKS];

node *alloc_node(rtems_task_argument tid) {
  node *n = (node*)rtems_chain_get_unprotected( &freelist[tid] );
  return n;
}
void free_node(rtems_task_argument tid, node *n) {
  rtems_chain_append_unprotected( &freelist[tid], (rtems_chain_node*)n );
}

static rtems_rbtree_compare_result rbtree_compare(
  const rtems_rbtree_node* n1,
  const rtems_rbtree_node* n2
) {
  int key1 = RTEMS_CONTAINER_OF( n1, node, rbt_node )->data.key; 
  int key2 = RTEMS_CONTAINER_OF( n2, node, rbt_node )->data.key;

  return key1 - key2;
}

static void print_node( rtems_rbtree_node* n )
{
  printk("%X\t%X\t%X\t%X\t%d\n",
      n, n->parent, n->child[0], n->child[1], n->color);
}

static int rb_assert ( rtems_rbtree_node *root )
{
  int lh, rh;

  if ( root == NULL )
    return 0;
  else {
    rtems_rbtree_node *ln = rtems_rbtree_left(root);
    rtems_rbtree_node *rn = rtems_rbtree_right(root);

    /* Consecutive red links */
    if ( _RBTree_Get_color(root) == RBT_RED ) {
      if (_RBTree_Is_red(ln) || _RBTree_Is_red(rn)) {
        puts ( "Red violation" );
        return -1;
      }
    }

      print_node(root); // pre-order print
      lh = rb_assert ( ln );
      rh = rb_assert ( rn );
    if ( !rtems_rbtree_parent(root) ) {
      int bh = _RBTree_Get_black_height(root->parent);
      if ( lh != -1 && lh != bh ) {
        printk ( "Tree black height violation: %d != %d\n", lh, bh );
        return -1;
      }
    }

    /* Invalid binary search tree */
    if ( ( ln != NULL && rbtree_compare(ln, root) > 0 )
        || ( rn != NULL && rbtree_compare(rn, root) < 0 ) )
    {
      puts ( "Binary tree violation" );
      return -1;
    }

    /* Black height mismatch */
    if ( lh != -1 && rh != -1 && lh != rh ) {
      puts ( "Black violation" );
      return -1;
    }

    /* Only count black links */
    if ( lh != -1 && rh != -1 )
      return ( _RBTree_Get_color(root) == RBT_RED ) ? lh : lh + 1;
    else
      return -1;
  }
}

#if 0
/** @brief Find the node with given key in the tree
 *
 *  This function returns a pointer to the node in @a the_rbtree 
 *  having key equal to key of  @a the_node if it exists,
 *  and NULL if not. @a the_node has to be made up before a search.
 */
rtems_rbtree_node* test_rbtree_find_int32(
    rtems_rbtree_control *the_rbtree,
    rtems_rbtree_node *the_node
    )
{
  int32_t find_key = rtems_rbtree_container_of(the_node, node, rbt_node)->data.key;
  int32_t iter_key;
  rtems_rbtree_node* iter_node = the_rbtree->root;
  rtems_rbtree_node* found = NULL;
  while (iter_node) {
    iter_key = rtems_rbtree_container_of( iter_node, node, rbt_node )->data.key;
    if (find_key == iter_key)
      found = iter_node;

    rtems_rbtree_direction dir = find_key > iter_key;
    iter_node = iter_node->child[dir];
  } /* while(iter_node) */

  return found;
}

/** @brief Insert a Node (unprotected)
 *
 *  This routine inserts @a the_node on the Red-Black Tree @a the_rbtree.
 *
 *  @retval 0 Successfully inserted.
 *  @retval -1 NULL @a the_node.
 *  @retval RBTree_Node* if one with equal key to the key of @a the_node exists
 *          in @a the_rbtree.
 *
 *  @note It does NOT disable interrupts to ensure the atomicity
 *        of the extract operation.
 */
rtems_rbtree_node* test_rbtree_insert_int32(
    rtems_rbtree_control *the_rbtree,
    rtems_rbtree_node *the_node
    )
{
  int32_t the_key = rtems_rbtree_container_of(the_node, node, rbt_node)->data.key;
  int32_t iter_key;
  rtems_rbtree_direction dir;

  if(!the_node) 
    return (rtems_rbtree_node*)-1;

  rtems_rbtree_node *iter_node = the_rbtree->root;

  if (!iter_node) { /* special case: first node inserted */
    the_node->color = RBT_BLACK;
    the_rbtree->root = the_node;
    the_rbtree->first[0] = the_rbtree->first[1] = the_node;
    the_node->parent = (rtems_rbtree_node *) the_rbtree;
    the_node->child[RBT_LEFT] = the_node->child[RBT_RIGHT] = NULL;
  } else {
    /* typical binary search tree insert, descend tree to leaf and insert */
    while (iter_node) {
      iter_key = rtems_rbtree_container_of( iter_node, node, rbt_node )->data.key;
      dir = the_key >= iter_key;
      if (!iter_node->child[dir]) {
        the_node->child[RBT_LEFT] = the_node->child[RBT_RIGHT] = NULL;
        the_node->color = RBT_RED;
        iter_node->child[dir] = the_node;
        the_node->parent = iter_node;
        /* update min/max */
        if (rtems_rbtree_is_first(the_rbtree, iter_node, dir)) {
          the_rbtree->first[dir] = the_node;
        }
        break;
      } else {
        iter_node = iter_node->child[dir];
      }
    } /* while(iter_node) */
  }
  return (rtems_rbtree_node*)0;
}
#endif

void rbtree_initialize( rtems_task_argument tid, int size ) {
  int i;

  rtems_chain_initialize_empty ( &freelist[tid] );
  for ( i = 0; i < size; i++ ) {
    rtems_chain_append(&freelist[tid], &the_nodes[i][tid].link);
  }

  rtems_rbtree_initialize_empty(
      &the_rbtree[tid]
  );

#if 0
  rtems_rbtree_initialize_empty(
      &the_rbtree[tid],
      &test_rbtree_insert_int32,
      &test_rbtree_find_int32,
      RTEMS_RBTREE_DUPLICATE
  );
#endif
}

void rbtree_insert( rtems_task_argument tid,  uint64_t kv ) {
  node *n = alloc_node(tid);
  pq_node *pn = &n->data;
  pn->key = kv_key(kv);
  pn->val = kv_value(kv);
  rtems_rbtree_insert( &the_rbtree[tid], &n->rbt_node, &rbtree_compare, false );
#if defined(USE_RB_ASSERT)
  rb_assert(the_rbtree[tid].root);
#endif
}

uint64_t rbtree_min( rtems_task_argument tid ) {
  uint64_t kv;
  rtems_rbtree_node *rn;
  node *n;
  pq_node *p;

  rn = rtems_rbtree_min(&the_rbtree[tid]);

  if ( rn ) {
    n = RTEMS_CONTAINER_OF(rn, node, rbt_node);
    p = &n->data;
    kv = PQ_NODE_TO_KV(p);
    return kv;
  } 
  return (uint64_t)-1; // FIXME: error handling
}

uint64_t rbtree_pop_min( rtems_task_argument tid ) {
  uint64_t kv;
  rtems_rbtree_node *rn;
  node *n;
  pq_node *p;

  rn = rtems_rbtree_get_min(&the_rbtree[tid]);

  if ( rn ) {
    n = RTEMS_CONTAINER_OF(rn, node, rbt_node);
    p = &n->data;
    kv = PQ_NODE_TO_KV(p);
    free_node(tid, n);
  } else {
    kv = (uint64_t)-1;
  }
#if defined(USE_RB_ASSERT)
  rb_assert(the_rbtree[tid].root);
#endif
  return kv;
}

uint64_t rbtree_search( rtems_task_argument tid, int k )
{
  rtems_rbtree_node *rn;
  node search_node;

  node *n;
  pq_node *p;
  uint64_t kv;

  search_node.data.key = k;

  rn = rtems_rbtree_find(&the_rbtree[tid], &search_node.rbt_node, &rbtree_compare, false);
  if ( rn ) {
    n = RTEMS_CONTAINER_OF(rn, node, rbt_node);
    p = &n->data;
    kv = PQ_NODE_TO_KV(p);
  } else {
    kv = (uint64_t)-1;
  }
  return kv;
}

uint64_t rbtree_extract( rtems_task_argument tid, int k )
{
  rtems_rbtree_node *rn;
  node search_node;

  node *n;
  pq_node *p;
  uint64_t kv;

  search_node.data.key = k;

  rn = rtems_rbtree_find(&the_rbtree[tid], &search_node.rbt_node, &rbtree_compare, false);
  if ( rn ) {
    rtems_rbtree_extract(&the_rbtree[tid], rn);
    n = RTEMS_CONTAINER_OF(rn, node, rbt_node);
    p = &n->data;
    kv = PQ_NODE_TO_KV(p);
    free_node(tid, n);
  } else {
    kv = (uint64_t)-1;
  }
#if defined(USE_RB_ASSERT)
  rb_assert(the_rbtree[tid].root);
#endif
  return kv;
}


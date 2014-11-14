/*
 * Copyright 2014 Gedare Bloom (gedare@rtems.org)
 *
 * This file's license is 2-clause BSD as in this distribution's LICENSE.2 file.
 */
/* command line: -s 256 -d 1 -p 10 -i 100000 -t 4 -a 0.0 -o 1024 -m 1  */
/* i: 100000
   alpha: 0.0
   beta: 0.5
 *//* This is a generated file. DO NOT EDIT. */



/* [1, 0, 0, 0] duplicate enqueues during warmup */

/* [2, 1, 3, 0] duplicate enqueues during work */

/* The maximum priority value during warmup is [2026392, 1809016, 2339285, 2497165] */

/* The maximum priority value during work is [2552705, 2325984, 2950538, 2939360] */


#ifndef __PARAMS_H_
#define __PARAMS_H_

/* task parameters */
#define NUM_PERIODIC_TASKS  (0)
#define NUM_APERIODIC_TASKS (4)
#define NUM_TASKS           ( NUM_PERIODIC_TASKS + NUM_APERIODIC_TASKS )

#define build_task_name() do { \
Task_name[ 1 ] =  rtems_build_name( 'A', 'T', '0', '1' );\
Task_name[ 2 ] =  rtems_build_name( 'A', 'T', '0', '2' );\
Task_name[ 3 ] =  rtems_build_name( 'A', 'T', '0', '3' );\
Task_name[ 4 ] =  rtems_build_name( 'A', 'T', '0', '4' );\
} while(0)

/* PQ parameters */
#define PQ_MAX_SIZE  (256)
extern const int PQ_SIZE[4];
extern const int PQ_WARMUP_OPS[4];
extern const int PQ_WORK_OPS[4];

#endif

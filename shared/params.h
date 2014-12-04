/* command line: -s 256 -d 1 -p 10 -i 100000 -t 1 -a 0.0 -o 1024 -m 4  */
/* i: 100000
   alpha: 0.0
   beta: 0.5
 *//* This is a generated file. DO NOT EDIT. */



/* [0] duplicate enqueues during warmup */

/* [0] duplicate enqueues during work */

/* The maximum priority value during warmup is [99394] */

/* The maximum priority value during work is [0] */


#ifndef __PARAMS_H_
#define __PARAMS_H_

/* task parameters */
#define NUM_PERIODIC_TASKS  (0)
#define NUM_APERIODIC_TASKS (1)
#define NUM_TASKS           ( NUM_PERIODIC_TASKS + NUM_APERIODIC_TASKS )

#define build_task_name() do { \
Task_name[ 1 ] =  rtems_build_name( 'A', 'T', '0', '1' );\
} while(0)

/* PQ parameters */
#define PQ_MAX_SIZE  (256)
extern const int PQ_SIZE[1];
extern const int PQ_WARMUP_OPS[1];
extern const int PQ_WORK_OPS[1];

#endif
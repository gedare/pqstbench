/*
 * Copyright 2014 Gedare Bloom (gedare@rtems.org)
 *
 * This file's license is 2-clause BSD as in this distribution's LICENSE.2 file.
 */
/*  Init
 *
 *  Input parameters:  NONE
 *
 *  Output parameters:  NONE
 */

#define GAB_TIMESLICE
//#define CACHE_TASK

#include "params.h"
#define CONFIGURE_MAXIMUM_SEMAPHORES        5
#if defined(CACHE_TASK)
  #define CONFIGURE_MAXIMUM_TASKS               (2+NUM_TASKS)
#else
  #define CONFIGURE_MAXIMUM_TASKS               (1+NUM_TASKS)
#endif
#define CONFIGURE_MAXIMUM_PERIODS             (1+NUM_PERIODIC_TASKS)
#define CONFIGURE_APPLICATION_NEEDS_CLOCK_DRIVER
#define CONFIGURE_APPLICATION_NEEDS_CONSOLE_DRIVER
#define CONFIGURE_RTEMS_INIT_TASKS_TABLE
#if defined(GAB_TIMESLICE)
#define CONFIGURE_TICKS_PER_TIMESLICE       1
#define CONFIGURE_MICROSECONDS_PER_TICK   RTEMS_MILLISECONDS_TO_MICROSECONDS(200)
#endif
#define CONFIGURE_INIT
#include <rtems/confdefs.h>

#include <rtems.h>
#include <rtems/counter.h>

#define TESTS_USE_PRINTK
#include <tmacros.h>
#include <stdlib.h>

#include "workload.h"
#include "tasks.i"

static uint32_t  tasks_completed;
static rtems_id  tasks_complete_sem;
static rtems_id  final_barrier;

rtems_id   Task_id[ 1+NUM_TASKS ];
rtems_name Task_name[ 1+NUM_TASKS ];

extern const char rtems_test_name[];

rtems_task PQ_Cache_Task(rtems_task_argument argument)
{
  int *p, x;
  rtems_status_code status;
  printk("CT01\n");

  x = 0;
  p = 0;
  /* Barrier: tasks will be released by the init function */
  status = rtems_semaphore_obtain(tasks_complete_sem,RTEMS_DEFAULT_OPTIONS,0);

  while (FOREVER) {
    x += *p;
    p += 4;
    if ( x == -1 ) {
      printk("Killing cache task!\n");
      status = rtems_task_delete(RTEMS_SELF); /* probably unreached... */
      directive_failed(status, "rtems_task_delete of RTEMS_SELF");
      return (rtems_task)x; /* unreached */
    }
  }
}

rtems_task PQ_Periodic_Task(rtems_task_argument argument)
{
  rtems_id          rmid;
  rtems_status_code status;

  status = rtems_rate_monotonic_create( argument, &rmid );
  directive_failed( status, "rtems_rate_monotonic_create" );

  /* Barrier: tasks will be released by the init function */
  status = rtems_semaphore_obtain(tasks_complete_sem,RTEMS_DEFAULT_OPTIONS,0);

  /* Periodic Loop */
  while (FOREVER) {
    if (rtems_rate_monotonic_period(rmid, Periods[argument])==RTEMS_TIMEOUT) {
      puts("Error: deadline missed!\n");
    }
  }
  /* Shouldn't reach here. */

  /* And should block forever if it does. */
  status = rtems_semaphore_obtain( final_barrier, RTEMS_DEFAULT_OPTIONS, 0 );
  directive_failed( status, "rtems_semaphore_obtain" );

  printk( "Killing task %d\n", argument);
  status = rtems_task_delete(RTEMS_SELF);
  directive_failed(status, "rtems_task_delete of RTEMS_SELF");
}

rtems_task PQ_Workload_Task(rtems_task_argument argument)
{
  int               index;
  rtems_status_code status;
  rtems_counter_ticks start;
  rtems_counter_ticks middle;
  rtems_counter_ticks end;
  rtems_counter_ticks d_warmup;
  rtems_counter_ticks d_work;

  /* initialize PQ structures */
  initialize(argument - 1);

  /* TODO: use a proper barrier if more than one task */

  /* Barrier: tasks will be released by the init function */
  status = rtems_semaphore_obtain(tasks_complete_sem, RTEMS_DEFAULT_OPTIONS, 0);

  start = rtems_counter_read();
  
  /* reach PQ steady state */
  warmup(argument - 1);

  middle = rtems_counter_read();

  /* workload */
  work(argument - 1);
  
  end = rtems_counter_read();

  d_warmup = rtems_counter_difference(middle, start);
  d_work = rtems_counter_difference(end, middle);

  printk(
      "%d\t%" PRIu64 "\t%" PRIu64"\n",
      argument,
      rtems_counter_ticks_to_nanoseconds(d_warmup),
      rtems_counter_ticks_to_nanoseconds(d_work)
  );

  status = rtems_semaphore_obtain(tasks_complete_sem, RTEMS_DEFAULT_OPTIONS, 0);
  directive_failed( status, "rtems_semaphore_obtain" );
    tasks_completed++;
    if (NUM_APERIODIC_TASKS == tasks_completed) {
      TEST_END();
      rtems_test_exit(0);
    }
  status = rtems_semaphore_release( tasks_complete_sem );
  directive_failed( status, "rtems_semaphore_release" );

  /* should block forever */
  status = rtems_semaphore_obtain( final_barrier, RTEMS_DEFAULT_OPTIONS, 0 );
  directive_failed( status, "rtems_semaphore_obtain" );

  /* shouldn't reach this */
  printk( "Killing task %d", argument);
  status = rtems_task_delete(RTEMS_SELF);
  directive_failed(status, "rtems_task_delete of RTEMS_SELF");
}

rtems_task Init(
  rtems_task_argument ignored
)
{
  uint32_t          index;
  uint32_t          freq; 
  rtems_status_code status;

#if defined(CACHE_TASK)
  rtems_id   cacheTask_id;
  rtems_name cacheTask_name;

  cacheTask_name = rtems_build_name( 'C', 'T', '0', '1' );
#endif

  TEST_BEGIN();

  if ( NUM_APERIODIC_TASKS == 0 ) {
    puts("error: Need at least 1 aperiodic task to ensure termination");
    rtems_test_exit(0);
  }

  tasks_completed = 0;

  build_task_name();

  status = rtems_semaphore_create(
      rtems_build_name( 'S', 'E', 'M', '0' ),  /* name = SEM0 */
      0,                                       /* locked */
      RTEMS_BINARY_SEMAPHORE | RTEMS_FIFO,     /* mutex w/desired discipline */
      0,                                       /* IGNORED */
      &tasks_complete_sem
    );
  directive_failed( status, "rtems_semaphore_create" );

  status = rtems_semaphore_create(
      rtems_build_name( 'S', 'E', 'M', '1' ),  /* name = SEM1 */
      0,                                       /* locked */
      RTEMS_SIMPLE_BINARY_SEMAPHORE,           /* mutex w/desired discipline */
      0,                                       /* IGNORED */
      &final_barrier
    );
  directive_failed( status, "rtems_semaphore_create" );

  for ( index = 1 ; index <= NUM_TASKS ; index++ ) {
    status = rtems_task_create(
      Task_name[ index ],
      Priorities[ index ],
      RTEMS_MINIMUM_STACK_SIZE,
#if defined(GAB_TIMESLICE)
      RTEMS_PREEMPT|RTEMS_TIMESLICE,
#else
      RTEMS_DEFAULT_MODES,
#endif
      RTEMS_DEFAULT_ATTRIBUTES,
      &Task_id[ index ]
    );
    directive_failed( status, "rtems_task_create loop" );
  }

  for ( index = 1 ; index <= NUM_APERIODIC_TASKS ; index++ ) {
    status = rtems_task_start( Task_id[ index ], PQ_Workload_Task, index );
    directive_failed( status, "rtems_task_start loop" );
  }

  for ( index = NUM_APERIODIC_TASKS+1 ; index <= NUM_TASKS ; index++ ) {
    status = rtems_task_start( Task_id[ index ], PQ_Periodic_Task, index );
    directive_failed( status, "rtems_task_start loop" );
  }

#if defined(CACHE_TASK)

  status = rtems_task_create(
      cacheTask_name,
      200, /* arbitrary priority */
      RTEMS_MINIMUM_STACK_SIZE,
  #if defined(GAB_TIMESLICE)
      RTEMS_PREEMPT|RTEMS_TIMESLICE,
  #else
      RTEMS_DEFAULT_MODES,
  #endif
      RTEMS_DEFAULT_ATTRIBUTES,
      &cacheTask_id
  );
  status = rtems_task_start( cacheTask_id, PQ_Cache_Task, NUM_TASKS+1 );
#endif
 
  rtems_task_wake_after( 1 );

  /* release all of the waiting tasks */
  status = rtems_semaphore_flush( tasks_complete_sem );
  directive_failed( status, "rtems_semaphore_flush" );

  status = rtems_semaphore_release( tasks_complete_sem );
  directive_failed( status, "rtems_semaphore_release" );

  /* Should block forever */
  status = rtems_semaphore_obtain( final_barrier, RTEMS_DEFAULT_OPTIONS, 0 );
  directive_failed( status, "rtems_semaphore_obtain" );

  /* unreached */
  puts("Init killing self\n");

  status = rtems_task_delete( RTEMS_SELF );
  directive_failed( status, "rtems_task_delete of RTEMS_SELF" );
}


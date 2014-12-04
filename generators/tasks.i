/* command line: -s 256 -d 1 -p 10 -i 100000 -t 1 -a 0.0 -o 1024 -m 4  */
/* i: 100000
   alpha: 0.0
   beta: 0.5
 *//* This is a generated file. DO NOT EDIT. */

/* Task parameters */
rtems_task_priority Priorities[1+NUM_TASKS] = {
          0,
          200,
};
uint32_t  Periods[1] = { 0 };
uint32_t  Execution_us[1+NUM_TASKS] = {
          0*CONFIGURE_MICROSECONDS_PER_TICK,
          200*1*CONFIGURE_MICROSECONDS_PER_TICK,
};

/*
 * Copyright 2014 Gedare Bloom (gedare@rtems.org)
 *
 * This file's license is 2-clause BSD as in this distribution's LICENSE.2 file.
 */

#ifndef _WORKLOAD_H_
#define _WORKLOAD_H_
#include <rtems.h>
void initialize( rtems_task_argument tid );

void warmup( rtems_task_argument tid );

void work( rtems_task_argument tid );

// WARMUP must be defined for opal to work properly currently.
//#define WARMUP
// Measurement directives...
//#define DOMEASURE
//#define MEASURE_CS
//#define MEASURE_DEQUEUE
//#define MEASURE_ENQUEUE
//#define MEASURE_SPILL
//#define MEASURE_FILL


#endif

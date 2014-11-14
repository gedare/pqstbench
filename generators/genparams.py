#!/bin/python

#
# Copyright 2014 Gedare Bloom (gedare@rtems.org)
#
# This file's license is 2-clause BSD as in this distribution's LICENSE.2 file.
#

##
## Generate param.h and param.i for benchmark 
##

import random
import math
import sys
import getopt
import array
import os
import copy

## TODO:
#
# Make each task have separate workloads
# Allow some tasks to share a workload
#

def usage():
  print "\
Usage: genparams.py -[hs:p:i:d:o:m:a:b:t:]\n\
  -h --help           print this help\n\
  -s --size=          specify size (max) of priority queue\n\
  -p --priority=      specify the initial priority range to generate\n\
  -i --increment=     specify the expected priority increment to generate;\n\
                      increment values are quantized over i*d\n\
  -d --distribution=  specify priority increment distribution, one of:\n\
                        1. Exponential with mean 0.5 (-ln(rand))\n\
                        2. Uniform between 0 and 2 (2rand)\n\
                        3. Biased between 0.9 to 1.1 (0.9 + 0.2rand)\n\
                        4. Bimodal: 0.95238rand +\n\
                             9.5238 if rand < 0.1 \n\
                             0 otherwise\n\
                        5. Triangular 1.5rand\n\
  -o --ops=           specify number of hold ops (1000)\n\
  -m --model=         specify access pattern model, one of:\n\
                        1. Classic Hold\n\
                        2. Markov Hold\n\
                        3. Up/Down\n\
                        4. Search BG\n\
  -a --alpha=         specify alpha transition probability (Markov Hold)\n\
  -b --beta=          specify beta transition probability (Markov Hold)\n\
  -t --tasks=         number of tasks\n\
"

def get_increment( i, d ):
  r = random.random()
  if ( d == 1 ):
    return i*(0.0-math.log(r))
  elif ( d == 2 ):
    return i*(2.0*r); # 2
  elif ( d == 3 ):
    return i*(0.9 + 0.2*r);
  elif ( d == 4 ):
    if (r < 0.1):
      return i*(0.95238 + 9.5238)
    else:
      return i*(0.95238)
  elif ( d == 5 ):
    return i*(1.5*math.sqrt(r))
  ## double-check the following two cases.
  elif ( d == 6 ): # erlang(4,2) -- mean 2 stddev 1
    r2 = random.random()
    return i*(0.0 - (1.0/4.0) * math.log(r*r2))
  elif ( d == 7 ): # 2 stage hyperexponential with mean 2 stddev 4
    x = 2.0
    s = 4.0
    cv = s/x
    cv_sq = cv*cv
    p = 0.5*(1.0 - math.sqrt((cv_sq-1.0)/(cv_sq+1.0)))
    z = 0.0
    if ( r > p ):
       z = x / (1.0 - p)
    else:
      z = x/p
    return i*((0.0 - 0.5)*z*math.log(random.random()))
  else:
    assert False, "Invalid distribution"

##
# create_classic_markov_warmup_ops
#
# create a PQ of size s by a series of enqueues and dequeues with
# slightly more enqueues than dequeues
##
def create_classic_markov_warmup_ops(s, p, i, d):
  args = []
  ops = []
  pq = []
  DQ = []
  duplicates = 0
  max_priority = 0

  slot = [v for v in xrange(s)]
  priority = random.randint(0,p)
  items = 1
  seq = 0
  args.append('{'+str(priority)+','+str(items-1)+'}')
  ops.append('i')
  pq.append([priority, seq])
  DQ.append(priority)
  max_priority = priority
  while ( items < s ):
    if ( items > 0 and random.random() < 0.4):
      plist = pq[0]
      priority = plist[0]
      replace = plist[1]
      args.append('{'+str(priority)+','+str(replace)+'}')
      items = items - 1
      ops.append('p')
      pq.pop(0)
      DQ.pop(0)
    else:
      increment = priority + int(get_increment(i,d))
      items = items + 1
      args.append('{'+str(increment)+','+str(seq)+'}')
      ## count duplicates
      if DQ.count(increment) > 0:
        duplicates = duplicates + 1
      ops.append('i')
      pq.append([increment,seq])
      DQ.append(increment)
      if increment > max_priority:
        max_priority = increment
    pq.sort()
    DQ.sort()
    seq = seq + 1

  return (args,ops,pq,DQ,duplicates,max_priority)

##
# create_updown_warmup_ops
#
# create a PQ of size s by a series of enqueues
##
def create_updown_warmup_ops(s, p, i, d):
  args = []
  ops = []
  pq = []
  DQ = []
  duplicates = 0
  max_priority = 0

  increment = random.randint(0,i)
  trials = 1
  args.append('{'+str(increment)+','+str(trials)+'}')
  ops.append('i')
  pq.append([increment, trials])
  DQ.append(increment)
  max_priority = increment
  
  ## Up
  while ( trials < s ):
    increment = increment + int(get_increment(i,d))
    args.append('{'+str(increment)+','+str(trials)+'}')
    ## count duplicates
    if DQ.count(increment) > 0:
      duplicates = duplicates + 1
    ops.append('i')
    pq.append([increment,trials])
    DQ.append(increment)
    if increment > max_priority:
      max_priority = increment
    trials = trials + 1
  pq.sort()
  DQ.sort()

  return (args,ops,pq,DQ,duplicates,max_priority)

##
# create_search_bell_gupta_warmup_ops
#
# create a search tree of size s by a series of enqueues
##
def create_search_bell_gupta_warmup_ops(s, p, i, d):
  args = []
  ops = []
  APT = [[] for x in range(s)]
  DQ = []
  max_key = 0
  duplicates = 0 ## no duplicate keys
  
  # Enqueue s unique integer keys randomly from a uniform distribution
  increment = random.randint(0,i)
  trials = 1
  args.append('{'+str(increment)+','+str(trials)+'}')
  ops.append('i')
  max_key = increment
  slot = random.randint(0,s-1)
  APT[slot] = [increment, trials]
  DQ.append(increment)

  while ( trials < s ):
    increment = random.randint(0,i)
    while DQ.count(increment) > 0: # no dupes allowed!
      increment = random.randint(0,i)

    args.append('{'+str(increment)+','+str(trials)+'}')
    ops.append('i')
    DQ.append(increment)
    if increment > max_key:
      max_key = increment

    # generate an access probability for the new key by putting it in a
    # randomly generated slot in DQ. Do this by generating a random number
    # in the number of remaining slots, and then scan through the DQ (APT)
    # counting open slots until the random number is reached.
    skip_empty = random.randint(0,s-trials-1)
    index = 0
    count_empty = 0
    while count_empty < skip_empty:
      if APT[index] == []: # count unused slots
        count_empty = count_empty + 1
      index = index + 1
    while APT[index] != []: # skip used slots
      index = index + 1
    assert index < s, "Overrun: index >= s"
    APT[index] = [increment, trials]
    trials = trials + 1
  return (args,ops,APT,DQ,duplicates,max_key)

def create_warmup_ops(s, p, i, d, m):

  if ( m == 1 or m == 2 ): # Classic and Markov
    return create_classic_markov_warmup_ops(s,p,i,d)
  elif ( m == 3 ):  # Up/Down
    return create_updown_warmup_ops(s,p,i,d)
  elif ( m == 4 ): # Search BG
    return create_search_bell_gupta_warmup_ops(s,p,i,d)

  assert False, "Unknown model" + str(m)

def create_classic_hold_ops(i, d, o, pq, DQ):
  args = []
  ops = []
  duplicates = 0
  max_priority = 0
  max_pqsize = len(pq)
  
  trials = 0
  while ( trials < o ):
    plist = pq[0]
    priority = plist[0]
    replace = plist[1]
    increment = int(get_increment(i,d))
    args.append('{'+str(increment)+','+str(priority)+'}')
    ops.append('h')
    pq.pop(0)
    DQ.pop(0)
    pq.append([priority + increment,replace])
    if DQ.count(priority+increment) > 0:
      duplicates = duplicates + 1
    if (priority+increment) > max_priority:
      max_priority = priority + increment
    DQ.append(priority+increment)
    pq.sort()
    DQ.sort()
    trials = trials + 1

  return (args, ops, pq, duplicates, max_priority, max_pqsize)

def create_updown_ops(i, d, o, pq, DQ):
  args = []
  ops = []
  duplicates = 0
  max_priority = 0

  trials = 0
  max_pqsize = len(pq)

  ## Down
  while ( trials < max_pqsize ):
    plist = pq[0]
    priority = plist[0]
    replace = plist[1]
    args.append('{'+str(priority)+','+str(replace)+'}')
    ops.append('p')
    pq.pop(0)
    DQ.pop(0)
    pq.sort()
    DQ.sort()
    trials = trials + 1

  return (args, ops, pq, duplicates, max_priority, max_pqsize)


def markov_transition(state, alpha, beta):
  next_state = state
  r = random.random()
  if ( state == "dequeue" ):
    if ( r > alpha ):
      next_state = "enqueue"
  else:
    if ( r > beta ):
      next_state = "dequeue"

  return next_state


def create_markov_hold_ops(i, d, o, pq, DQ, a, b):
  args = []
  ops = []
  duplicates = 0
  max_priority = 0
  max_pqsize = 0

  items = len(pq)

  state = "dequeue"
  # dequeue first item to get first priority value
  plist = pq[0]
  priority = plist[0]
  replace = plist[1]
  args.append('{'+str(priority)+','+str(replace)+'}')
  items = items - 1
  ops.append('p')
  pq.pop(0)
  DQ.pop(0)
  pq.sort()
  DQ.sort()
  max_priority = priority

  trials = 1
  while ( trials < o ):
    next_state = markov_transition(state, a, b)
    increment = 0
    if ( next_state == "enqueue" ):
      if ( state == "dequeue" ): # hold
        increment = priority + int(get_increment(i,6))
      else:
        increment = priority + int(get_increment(i,7))
      items = items + 1
      args.append('{'+str(increment)+','+str(trials)+'}')
      ## count duplicates
      if DQ.count(increment) > 0:
        duplicates = duplicates + 1
      ops.append('i')
      pq.append([increment,trials])
      DQ.append(increment)
      if increment > max_priority:
        max_priority = increment
      if len(pq) > max_pqsize:
        max_pqsize = len(pq)
    else: #dequeue
      if ( items == 0 ):
        return (args,ops,pq,duplicates,2147483648+1)
      plist = pq[0]
      priority = plist[0]
      replace = plist[1]
      args.append('{'+str(priority)+','+str(replace)+'}')
      items = items - 1
      ops.append('p')
      pq.pop(0)
      DQ.pop(0)
    pq.sort()
    DQ.sort()

    state = next_state
    trials = trials + 1
  
  return (args, ops, pq, duplicates, max_priority, max_pqsize)

##
# return the number of updates per 100 operations based on the d argument
##
def get_activity_ratio(d):
  if d == 1:
    return 0
  elif d == 2:
    return 20
  elif d == 3:
    return 50
  elif d == 4:
    return 80
  else:
    assert False, "Unknown activity ratio: " + str(d)

def generate_modified_BG_zipf_prob(a, n):
  C_inv = math.fsum( [1.0 / math.pow(float(i+1), a) for i in xrange(n)] )
  C = 1.0 / C_inv
  p = [C / math.pow(float(i+1), a) for i in xrange(n)]
  return p

##
# given a random number r [0,1) return the index of the corresponding key
# in the APT using the CDF 
##
def get_index_by_modified_BG_zipf(prob_list, r, n):
  cdf = 0
  for i in xrange(n):
    cdf = cdf + prob_list[i]
    if ( cdf >= r ):
      return i
  assert False, "index not found for " + r


def create_search_BG_ops(i, d, num_ops, APT, DQ, a):
  args = []
  ops = []
  duplicates = 0
  max_key = 0

  size = len(APT)
  prob_list = generate_modified_BG_zipf_prob(a, size)

  # carry out ops operations according to the activity ratio.
  # The activity ratio specifies how many updates per search and is
  # listed as updates per 100 operations.
  updates_per_100_ops = get_activity_ratio(d)

  # proceed in sets of 100
  for x in range(num_ops/100):
    # do updates
    for u in range(updates_per_100_ops):
      ## select a key at random
      key_index = random.randint(0,len(APT)-1)
      kv = APT[key_index]
      key = kv[0]
      value = kv[1]

      ## delete key
      args.append('{'+str(key)+','+str(value)+'}')
      ops.append('x')
      DQ.remove(key)

      ## replace key
      increment = random.randint(0,i)
      while DQ.count(increment) > 0: # no dupes allowed!
        increment = random.randint(0,i)
      args.append('{'+str(increment)+','+str(value)+'}')
      ops.append('i')
      if increment > max_key:
        max_key = increment

      APT[key_index] = [increment, value]
      DQ.append(increment)
    
    # do searches
    for s in range(100-updates_per_100_ops):
      r = random.random()
      key_index = get_index_by_modified_BG_zipf(prob_list, r, size)
      kv = APT[key_index]
      key = kv[0]
      value = kv[1]
      args.append('{'+str(kv[0])+','+str(value)+'}')
      ops.append('s')

  beta = math.fsum( prob_list[0:size/100] )
  
  return (args, ops, APT, duplicates, max_key, beta)

def create_work_ops(i, d, o, pq, DQ, m, a, b):
  if ( m == 1 ):
    return create_classic_hold_ops(i,d,o,pq,DQ)
  elif ( m == 2 ):
    return create_markov_hold_ops(i,d,o,pq,DQ,a,b)
  elif ( m == 3 ):
    return create_updown_ops(i,d,o,pq,DQ)
  elif ( m == 4 ):
    # d=updates_per_100_ops, pq=ST, dq=APT
    return create_search_BG_ops(i,d,o,pq,DQ,a)
  else:
    print "Invalid model: " + str(m)

def generate(s, p, i, d, o, m, a, b):
  size = int(s)
  priority = int(p)
  increment = int(i)
  distribution = int(d)
  ops = int(o)
  model = int(m)
  alpha = float(a)
  beta = float(b)
  # Generate queue creation; enqueue and dequeue operations with slightly
  # more enqueues than dequeues until the PQ size (size) is reached
  (warmup_args,warmup_ops,warmup_pq,warmup_dq,warmup_dupes,warmup_primax)=\
      create_warmup_ops(size, priority, increment, distribution, model)
  while warmup_primax > 2147483648:
    increment = increment/2; # relax increment
    print "Retrying: max priority (" + str(warmup_primax) + ") too large\n"
    (warmup_args,warmup_ops,warmup_pq,warmup_dq,warmup_dupes,warmup_primax)=\
        create_warmup_ops(size, priority, increment, distribution, model)

  # Generate priority increment values distributed by (distribution) until
  # ops values are created
  (work_args, work_ops, work_pq, work_dupes, work_primax, work_pqsizemax)=\
      create_work_ops(increment, distribution, ops, warmup_pq[:], warmup_dq[:], model, alpha, beta)
  while work_primax > 2147483648:
    if ( model == 2 ):
      alpha = alpha - 0.01      # relax alpha
      beta = beta + 0.01        # tighten beta
    else:
      increment = increment/2  # relax increment
    print "Retrying: max priority (" + str(work_primax) + ") too large\n"
    (work_args, work_ops, work_pq, work_dupes, work_primax, work_pqsizemax)=\
        create_work_ops(increment, distribution, ops, warmup_pq[:], warmup_dq[:], model, alpha, beta)
  if ( model == 4 ):
    beta = work_pqsizemax
    work_pqsizemax = len(work_pq)
  return (warmup_dupes, work_dupes, warmup_primax, work_primax, work_pqsizemax, warmup_ops, work_ops, warmup_args, work_args)

def print_params_headers(header, tasks, g_warmup_dupes,  g_work_dupes,  g_warmup_primax,  g_work_primax,  g_work_pqsizemax,  g_warmup_ops,  g_work_ops,  g_warmup_args,  g_work_args):

  null_op = "s"
  null_arg = "{0,0}"

  # Create output
  f = open('params.h', 'w')
  f.write(header)
  f.write("\n\n/* " + str(g_warmup_dupes) + " duplicate enqueues during warmup */\n")
  f.write("\n/* " + str(g_work_dupes) + " duplicate enqueues during work */\n")
  f.write("\n/* The maximum priority value during warmup is " + str(g_warmup_primax) + " */\n")
  f.write("\n/* The maximum priority value during work is " + str(g_work_primax) + " */\n")

  ## FIXME: add task parameters
  f.write('\n\n#ifndef __PARAMS_H_\n\
#define __PARAMS_H_\n\
\n\
/* task parameters */\n\
#define NUM_PERIODIC_TASKS  (0)\n\
#define NUM_APERIODIC_TASKS (' + tasks + ')\n\
#define NUM_TASKS           ( NUM_PERIODIC_TASKS + NUM_APERIODIC_TASKS )\n\
\n\
#define build_task_name() do { \\\n')
  for t in range(int(tasks)):
    f.write('Task_name[ ' + str(t+1) + ' ] =  rtems_build_name( \'A\', \'T\','
        + ' \'' + str((t+1)/10) + '\', \'' + str((t+1)%10) + '\' );\\\n')
  f.write('} while(0)\n\n/* PQ parameters */\n')
  f.write('#define PQ_MAX_SIZE  (')
  max_size = 0
  for t in range(int(tasks)):
    if g_work_pqsizemax[t] > max_size:
      max_size = g_work_pqsizemax[t]
  f.write(str(max_size) + ')\n')
  f.write('extern const int PQ_SIZE[' + tasks + '];\n')
  f.write('extern const int PQ_WARMUP_OPS[' + tasks + '];\n')
  f.write('extern const int PQ_WORK_OPS[' + tasks + '];\n')
  f.write('\n#endif')
  f.close()
  print "params.h written"

  max_ops = 0
  max_args = 0
  for t in range(int(tasks)):
    if len(g_warmup_ops[t]) + len(g_work_ops[t]) > max_ops:
      max_ops = len(g_warmup_ops[t]) + len(g_work_ops[t])
    if len(g_warmup_args[t]) + len(g_work_args[t]) > max_args:
      max_args = len(g_warmup_args[t]) + len(g_work_args[t])

  f = open('params.i', 'w')
  f.write(header)
  f.write('/* PQ parameters */\n')
  f.write('const int PQ_SIZE[' + tasks + '] = {\n')
  for t in range(int(tasks)):
    f.write(str(g_work_pqsizemax[t]) + ',\n')
  f.write('};\n')

  f.write('const int PQ_WARMUP_OPS[' + tasks + '] = {\n')
  for t in range(int(tasks)):
    f.write(str(len(g_warmup_ops[t])) + ',\n')
  f.write('};\n')

  f.write('const int PQ_WORK_OPS[' + tasks + '] = {\n')
  for t in range(int(tasks)):
    f.write(str(len(g_work_ops[t])) + ',\n')
  f.write('};\n')


  f.write('/* pq inputs and operations */\n')
  f.write('PQ_op ops[' + tasks + '][' + str(max_ops) + '] = {\n')
  for t in range(int(tasks)):
    count = 0
    f.write('{')
    while ( len(g_warmup_ops[t]) > 0 ):
      f.write(g_warmup_ops[t][0])
      g_warmup_ops[t].pop(0)
      f.write(',')
      count = count + 1
    while ( len(g_work_ops[t]) > 0 ):
      f.write(g_work_ops[t][0])
      g_work_ops[t].pop(0)
      f.write(',')
      count = count + 1
    ## add no-ops to pad length
    while ( count < max_ops ):
      f.write(null_op + ',')
      count = count + 1
    f.write('},\n')
  f.write('};\n')
  f.write('\n')
  f.write('PQ_arg args[' + tasks + '][' + str(max_args) + '] = {\n')
  for t in range(int(tasks)):
    count = 0
    f.write('{')
    while ( len(g_warmup_args[t]) > 0 ):
      f.write(g_warmup_args[t][0])
      g_warmup_args[t].pop(0)
      f.write(',')
      count = count + 1
    while ( len(g_work_args[t]) > 0 ):
      f.write(g_work_args[t][0])
      g_work_args[t].pop(0)
      f.write(',')
      count = count + 1
    ## add no-args to pad length
    while ( count < max_args ):
      f.write(null_arg + ',')
      count = count + 1
    f.write('},\n')
  f.write('};\n')
  f.write('\n')
  f.close()
  print "params.i written"

  f = open('tasks.i', 'w')
  f.write(header)
  f.write('/* Task parameters */\n')
  f.write('rtems_task_priority Priorities[1+NUM_TASKS] = {\n')
  f.write('          0,\n')
  for t in range(int(tasks)):
    f.write('          200,\n')
  f.write('};\n')

  f.write('uint32_t  Periods[1] = { 0 };\n')
  f.write('uint32_t  Execution_us[1+NUM_TASKS] = {\n')
  f.write('          0*CONFIGURE_MICROSECONDS_PER_TICK,\n')
  for t in range(int(tasks)):
    f.write('          200*1*CONFIGURE_MICROSECONDS_PER_TICK,\n')
  f.write('};\n')
  f.write('spillpq_policy_t pqbench_policy[NUM_TASKS] = {\n')
  for t in range(int(tasks)):
    f.write('  {false,false,false,0,0,0},\n')
  f.write('};\n')
  f.write('\n')
  f.close()
  print "tasks.i written"



def main():

  header = "/* command line: "
  model = 1
  alpha = 0.5
  beta = 0.5
  tasks = "1"

  # Process args
  try:
    opts, args = getopt.getopt(sys.argv[1:], "hs:p:i:d:o:m:a:b:t:",
        ["help", "size=", "priority=", "increment=", "distribution=", "ops=",
          "model=", "alpha=", "beta=", "tasks="])
  except getopt.GetoptError, err:
    print str(err)
    usage()
    sys.exit(2)
  for opt, arg in opts:
    if opt in ("-h", "--help"):
      usage()
      sys.exit()
    elif opt in ("-s", "--size"): ## FIXME: let params vary for each task...
      size = int(arg)
    elif opt in ("-p", "--priority"):
      priority = int(arg)
    elif opt in ("-i", "--increment"):
      increment = int(arg)
    elif opt in ("-d", "--distribution"):
      distribution = int(arg)
    elif opt in ("-o", "--ops"):
      ops = int(arg)
    elif opt in ("-m", "--model"):
      model = int(arg)
    elif opt in ("-a", "--alpha"):
      alpha = float(arg)
    elif opt in ("-b", "--beta"):
      beta = float(arg)
    elif opt in ("-t", "--tasks"):
      tasks = arg
    else:
      assert False, "unhandled option"
    header = header + opt + " " + arg + " "

  header = header + " */"

  g_warmup_dupes = []
  g_work_dupes = []
  g_warmup_primax = []
  g_work_primax = []
  g_work_pqsizemax = []
  g_warmup_ops = []
  g_work_ops = []
  g_warmup_args = []
  g_work_args = []

  # Create parameters for each task
  for t in range(int(tasks)):
    (warmup_dupes, work_dupes, warmup_primax, work_primax, work_pqsizemax, warmup_ops, work_ops, warmup_args, work_args) = \
        generate( size,priority,increment,distribution,ops,model,alpha,beta )
    g_warmup_dupes.append(warmup_dupes)
    g_work_dupes.append(work_dupes)
    g_warmup_primax.append(warmup_primax)
    g_work_primax.append(work_primax)
    g_work_pqsizemax.append(work_pqsizemax)
    g_warmup_ops.append(warmup_ops)
    g_work_ops.append(work_ops)
    g_warmup_args.append(warmup_args)
    g_work_args.append(work_args)
    #if size > 16:
    #  size = size / 2

  header = header + "\n/* i: " + str(increment) + "\n   alpha: " + str(alpha) + "\n   beta: " + "{0:.2g}".format(beta) + "\n */"
  header = header + "/* This is a generated file. DO NOT EDIT. */\n\n"

  print_params_headers(header, tasks, g_warmup_dupes,  g_work_dupes,  g_warmup_primax,  g_work_primax,  g_work_pqsizemax,  g_warmup_ops,  g_work_ops,  g_warmup_args,  g_work_args)



if __name__ == "__main__":
  main()

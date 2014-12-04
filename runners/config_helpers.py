
import sys
import os

def get_model_name(model):
  if model == "1":
    return "classic"
  elif model == "2":
    return "markov"
  elif model == "3":
    return "updown"
  elif model == "4":
    return "search"
  else:
    return "unknown" + model

def validate_config(cfglist):
  
  if len(cfglist) < 8:
    print "Config file not long enough, only found " + str(len(cfglist))
    sys.exit(1)

  sw_imps = [ "miheap","mrbtree","msplay","mstlheap","mlist","mskiplist"]
  for imp in cfglist[0]:
    if imp not in sw_imps:
      print("Unknown PQ implementation: " + imp)
      sys.exit(1)

  for size in cfglist[1]:
    if not size.isdigit():
      print("Invalid size: " + size)
      sys.exit(1)

  if not cfglist[2].isdigit():
    print("Invalid priority increment expected value " + cfglist[2])
    sys.exit(1)

  if not cfglist[3].isdigit():
    print("Invalid number of ops past transient state " + cfglist[3])
    sys.exit(1)

  if not os.path.exists(cfglist[4]):
    print("Invalid path to pqstbench: " + cfglist[4])
    sys.exit(1)

  if not os.path.exists(cfglist[5]):
    print("Invalid path to pqstbench_build: " + cfglist[5])
    sys.exit(1)

  for d in cfglist[6]:
    if not os.path.exists(d):
      print("Invalid path to workspace: " + d)
      sys.exit(1)

  if not cfglist[7].isdigit():
    print("Invalid repetitions count " + cfglist[7])
    sys.exit(1)


def parse_config_file(cfgfile):
  ## pqbench is configured for a set of pq implementations, 
  ## pq sizes, priority increment expected value, number of operations

  f = open(cfgfile, 'r')
  retcfg= [
      f.readline().strip().split(','), # ds implementations
      f.readline().strip().split(','), # ds sizes
      f.readline().strip(),            # priority increment
      f.readline().strip(),            # ops
      f.readline().strip(),            # pqstbench directory
      f.readline().strip(),            # pqstbench build
      f.readline().strip().split(','), # workspace directories
      f.readline().strip()             # number of times to repeat each run
      ]
  f.close()
  return retcfg

def parse_params_config_file(cfgfile):
  ## Parameters are rows one for each task with each row comma-delimited:
  ##  size, priority(init), increment, distribution, ops, model, alpha, beta
  f = open(cfgfile, 'r')
  params = []
  for row in f:
    params.append(row.strip().split(','))
  return params

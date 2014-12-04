#!/bin/python
##
## driver for mpqbench
##

import random
import math
import sys
import getopt
import array
import os
import copy

import config_helpers

def usage():
  print "\
Usage: pqbenchdriver.py -[hvrc:n:m:t:]\n\
  -h --help           print this help\n\
  -v --verbose        print more\n\
  -r --rerun          run tests again\n\
  -c --config=        config file\n\
  -n --name=          test name\n\
  -m --model=         specify access pattern model, one of:\n\
                        1. Classic Hold\n\
                        2. Markov Hold\n\
                        3. Up/Down\n\
                        4. Search\n\
  -t --tasks=         number of tasks\n\
"

def main():
  # default args
  cfgfile = ""
  name = "noname"
  verbose = False
  rerun = False
  model = "1"
  tasks = "1"

  # Process args
  try:
    opts, args = getopt.getopt(sys.argv[1:], "hvrc:n:m:t:",
        ["help", "verbose", "rerun",
          "config=", "name=", "model=", "tasks="])
  except getopt.GetoptError, err:
    print str(err)
    usage()
    sys.exit(2)
  for opt, arg in opts:
    if opt in ("-h", "--help"):
      usage()
      sys.exit()
    elif opt in ("-v", "--verbose"):
      verbose = True
    elif opt in ("-r", "--rerun"):
      rerun = True
    elif opt in ("-c", "--config"):
      cfgfile = arg
    elif opt in ("-n", "--name"):
      name = arg
    elif opt in ("-m", "--model"):
      model = arg
    elif opt in ("-t", "--tasks"):
      tasks = arg
    else:
      assert False, "unhandled option"

  # verify args  
  if cfgfile == "":
    print("Invalid config file")
    usage()
    sys.exit(2)

  configs = config_helpers.parse_config_file(cfgfile)
  config_helpers.validate_config(configs)
  
  if (verbose):
    print(configs)

  imps = configs[0]
  sizes = configs[1]
  inc = configs[2]
  ops = configs[3]
  pqstbench_dir = configs[4]
  pqstbench_build = configs[5]
  wkspace_dir = configs[6]
  repetitions = configs[7]

  pwd = os.system("pwd")

  if model == "4":
    for s in sizes:
      if int(inc) <= int(s):
        assert False, "Invalid increment: must be greater than size"

  if not os.path.exists(name):
    os.system("mkdir " + name)
    if not os.path.exists(name):
      print("Unable to create output directory: " + name)
      sys.exit(1)

  name = name + "/" + config_helpers.get_model_name(model)

  if not rerun:
    if os.path.exists(name):
      print("Error: output directory already exists, choose another: " + name)
      sys.exit(1)

    # create test run output directory
    os.system("mkdir " + name)
    if not os.path.exists(name):
      print("Unable to create output directory: " + name)
      sys.exit(1)

    # archive some files
    os.system("cp " + cfgfile + " " + name)
    os.system("cp pqbenchdriver.py " + name)
    os.system("cp " + pqstbench_dir + "/generators/genparams.py " + name)
    os.system("cp " + wkspace_dir + "/*.sh " + name)

  ## FIXME: these should come from the config.
  a_list = ["0.0"]
  #d_list = ["1","3","4"]
  d_list = ["1","2","3","4"]
  #d_list = ["1"]
  if model == "2": #markov
    d_list = ["1"]
    a_list = ["0.5_0.5","0.3_0.7","0.8_0.8","0.0_0.0"]
  if model == "4": #search
    #d_list = ["1","2","3","4"]
    d_list = ["1","4"]
    #a_list = ["0.0","0.516","0.687","0.892","0.975","1.058","1.257","1.420"]
    #a_list = ["0.0","0.516","1.058","1.420"]
    a_list = ["0.0","1.420"]

  for a in a_list:
    os.system("mkdir " + name + "/" + a);
    if not os.path.exists(name + "/" + a):
      print("Unable to find output directory: " + name + "/" + a)
      sys.exit(1)

  for s in sizes:
    for a in a_list:
      for d in d_list:
        params = "-s " + s
        params = params + " -d " + d
        params = params + " -p 10"
        params = params + " -i " + inc
        params = params + " -t " + tasks
  
        if model == "2": #markov
          b = a.split('_')[1]
          a = a.split('_')[0]
          params = params + " -b " + b
        params = params + " -a " + a
        params = params + " -o " + str((int(s)*int(ops))) # multiply size by ops
  #      params = params + " -o " + ops # use hard-coded ops
        params = params + " -m " + model
        if (verbose):
          print params

        # Repeat benchmark execution
        for r in range(int(repetitions)):
          tag = "_" + s + "_" + d + "_" + str(r)
          # generate params files
          # symlink should propagate the params files to each imp
          if not rerun:
            generators = os.path.join(pqstbench_dir, "generators")
            os.system("cd " + generators + " && python genparams.py " + params)
            os.system("cp " + generators + "/params.i " + name + "/" + a +
                "/params" + tag + ".i")
            os.system("cp " + generators + "/params.h " + name + "/" + a +
                "/params" + tag + ".h")
            os.system("cp " + generators + "/tasks.i " + name + "/" + a +
                "/tasks" + tag + ".i")
          os.system("cp " + name + "/" + a + "/params" + tag + ".i " +
                  pqstbench_dir + "/shared/params.i")
          os.system("cp " + name + "/" + a + "/params" + tag + ".h " +
                  pqstbench_dir + "/shared/params.h")
          os.system("cp " + name + "/" + a + "/tasks" + tag + ".i " +
                  pqstbench_dir + "/shared/tasks.i")

          # re-make pqstbench_dir
          os.system("cd " + pqstbench_dir + " && waf")

          # run
          for imp in imps:
            os.system("cp " + os.path.join(pqstbench_build,
                os.path.join(imp, imp + ".exe")) +
                " " + os.path.join(wkspace_dir, imp + ".exe"))
            os.system("cd " + wkspace_dir + " && ./run.sh " + imp + ".exe")

            if (not rerun):
              os.system("mv " + wkspace_dir + "/output.txt " +
                  name + "/" + a + "/" + imp + tag + ".txt")
            else:
              os.system("mv " + wkspace_dir + "/output.txt " + name +
                  "/" + a + "/" + imp + tag + "_rerun.txt")

if __name__ == "__main__":
  main()

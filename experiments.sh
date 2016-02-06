#!/bin/bash

#./geocron_simulator_runner.py  --npaths 5 --disasters 1,2 --fprobs 0.1 --runs 24 --nprocs 8
#./geocron_simulator_runner.py  --npaths 5 --disasters 1,2 --fprobs 0.1 0.2 0.3 0.5 --runs 24 --nprocs 8 --heuristics ideal --traces_name fprobs_ron_output
#./geocron_simulator_runner.py  --npaths 3 9 17 --disasters 1,2 --fprobs 0.1 --runs 24 --nprocs 8 --traces_name npaths_ron_output --heuristics intergeodivrp
./geocron_simulator_runner.py  --npaths 5 --disasters 1,2 --fprobs 0.1 --runs 24 --nprocs 8 --traces_name gsford_ron_output --heuristics gsford[D=100] gsford[D=1000] gsford[D=2500]
./geocron_simulator_runner.py  --npaths 5 --disasters 2,1 --fprobs 0.1 0.2 --runs 24 --nprocs 8 --topology_file src/topology-read/examples/geocron_topology_redundant.txt --traces_name redundant_ron_output

#! /usr/bin/env python2
from __future__ import print_function
GEOCRON_SIMULATOR_RUNNER_DESCRIPTION = '''Handles building command line arguments to be passed to the Geocron Simulator.'''

# @author: Kyle Benson
# (c) Kyle Benson 2012

import argparse
from os import system
#from os.path import isdir
#from os import listdir

default_runs=20
default_start=0 # num to start run IDs on
default_nprocs=8
default_file=['src/brite/examples/conf_files/TD_ASBarabasi_RTWaxman200.conf',
              #'3356', #level3
              #'1239', #sprintlink
              #'2914', #verio
              #smaller ones
              #'6461',
              #'1755',
              #'3967',
              ]
default_heuristics=['rand',
                    'ortho',
                    'newreg',
                    'close',
                    'far',
                    'angle',
                    'dist',
                    ]
default_fprobs=["0.1",
                "0.2",
                "0.3",
                "0.4",
                "0.5",
                #"0.6"
                ]
#TODO: make this configurable for BRITE topology...
default_disasters = {'3356' : '0,0'}
#default_disasters = {}
#default_disasters['1755']='"Amsterdam,_Netherlands-London,_UnitedKingdom-Paris,_France"'
#default_disasters['3967']='"Herndon,_VA-Irvine,_CA-Santa_Clara,_CA"'
#default_disasters['6461']='"San_Jose,_CA-Los_Angeles,_CA-New_York,_NY"'
#default_disasters['3356']='"New_York,_NY-Los_Angeles,_CA-Miami,_FL"'
#default_disasters['2914']='"New_York,_NY-Irvine,_CA"' #New_Orleans,_LA-
#default_disasters['1239']='"New_York,_NY-Dallas,_TX-Washington,_DC"'
default_verbosity_level= -1 # means verbose in the runner but not in the experiment itself

def parse_args(args):
## DEFAULTS
##################################################################################
#################      ARGUMENTS       ###########################################
# ArgumentParser.add_argument(name or flags...[, action][, nargs][, const][, default][, type][, choices][, required][, help][, metavar][, dest])
# action is one of: store[_const,_true,_false], append[_const], count
# nargs is one of: N, ?(defaults to const when no args), *, +, argparse.REMAINDER
# help supports %(var)s: help='default value is %(default)s'
##################################################################################

    parser = argparse.ArgumentParser(description=GEOCRON_SIMULATOR_RUNNER_DESCRIPTION,
                                     add_help=True,
                                     #formatter_class=argparse.RawTextHelpFormatter,
                                     #epilog='Text to display at the end of the help print',
                                     )
    # Topology generation/loading parameters
    parser.add_argument('--topology_file', '--file', '--as', nargs='+', default=default_file, dest='topologies',
                        help='''choose the rocketfuel AS topologies or BRITE configuration files for the simulations (default=%(default)s)''')
    parser.add_argument('--topology_type', '--topo', default="brite",
                        help='''choose how to read/generate topology (rocketfuel or brite, default=%(default)s)''')
    parser.add_argument('--seed', default=0, type=int,
                        help='''Random seed for generating topologies with brite, default is to let the GeocronExperiment choose it itself,
                        which will result in using a combination of the time and pid, hence giving a unique seed to each process.  Using this
                        parameter will result in all of the simulations having the same seed and hence same topology.''')

    # GeoCRON Simulation parameters
    parser.add_argument('--disasters', type=str, nargs='*', default=default_disasters,
                        help='''disaster locations to apply to ALL AS choices (cities currently) (default depends on AS)''')
    parser.add_argument('--fprobs', '-f', type=str, nargs='*',
                        default=default_fprobs,
                        help='''failure probabilities to use (default=%(default)s)''')
    parser.add_argument('--heuristics', nargs='*', default=default_heuristics,
                        help='''which heuristics to run, optionally with parameters specified within brackets
                        (e.g. rand, ideal, gsford[D=10], areageodivrp[A=0.5,B=1.5]...) (default=%(default)s)''')
    parser.add_argument('--nservers', default=['1'], nargs='+',
                        help='''number of servers for each node to attempt contact with when reporting data (default=%(default)s)''')
    parser.add_argument('--npaths', default=['1'], nargs='+',
                        help='''number of paths to send messages along when using overlay (default=%(default)s)''')
    parser.add_argument('--contact_attempts', default=1, type=int,
                        help='''number of times a node will attempt to try another path after a timeout while using the overlay (default=%(default)s)''')
    parser.add_argument('--timeout', default=0.5, type=float,
                        help='''number of seconds a node will wait before timing out a packet and (potentially) trying another path (default=%(default)s)''')

    # Control simulation repetition
    parser.add_argument('--runs', '-r', nargs='?',default=default_runs, type=int,
                        help='''number of times to run simulation for each set of parameters (default=%(default)s)''')
    parser.add_argument('--nprocs', '-n', nargs='?', action='store', type=int,
                        const=1, default=default_nprocs,
                        help='''number of parallel ns-3 instances to run  (default=%(default)s if flag unspecified, %(const)s if flag specified)''')
    parser.add_argument('--start', '-s', type=int, action='store',
                        default=default_start,
                        help='''unique ID number to start runs on  (default=%(default)s). Useful for running parallel instances.''')

    # Control waf/build
    parser.add_argument('--optimized', '-o', action="store_true",
                        help='''Configure waf in optimized mode before building.''')
    parser.add_argument('--extra_args',
                        help='''Pass additional args to ns-3. NOTE: you
                        need to put quotes around the arguments or else this
                        program may interpret them as additional arguments!''')

    # Control UI
    parser.add_argument('--debug', '-d', action="store_true",
                        help='''run simulator through GDB''')
    parser.add_argument('--valgrind', action="store_true",
                        help='''run simulator with valgrind to debug memory errors''')
    parser.add_argument('--visualize', action="store_true",
                        help='''run with PyViz visualizer''')
    parser.add_argument('--verbose', '-v', nargs='?',
                        const=default_verbosity_level, default=0, type=int,
                        help='''run with verbose printing (default=%(const)s when no arg given)''')
    parser.add_argument('--log', '-l', nargs='+',
                        help='''Specify ns-3 logging components to enable through waf.'''
                        '''Example: RonClientApplication[=func|warn|prefix_[node|time|func|level]]'''
                        '''(prefix_ not necessary, defaults to all/*)'''
                        '''remember quotes when using pipe symbol''')
    parser.add_argument('--show-cmd', '-c', action="store_true", dest="show_cmd", default=False,
                        help='''print the system command to be executed then exit''')
    parser.add_argument('--test', '-t', action="store_true",
                        help='''simulator will only run once in a single process, then exit''')
    parser.add_argument('--no-email', '-ne', action="store_true", dest='no_email',
                        help='''simulator will by default email you when finished; this disables that feature''')

    args = parser.parse_args(args)

    #set default values for when quickly testing
    if args.test:
        args.no_email = True
        if args.verbose is None:
            args.verbose = default_verbosity_level
        if args.runs is default_runs:
            args.runs = 1
        if args.nprocs is default_nprocs:
            args.nprocs = 1
        if args.heuristics is default_heuristics:
            args.heuristics = args.heuristics[:1]
        #args.topologies = args.topologies[:1]
        if args.topologies == default_file:
            args.topologies = ['src/brite/examples/conf_files/RTWaxman20.conf']
            #args.topologies = ['3356'] #small topology in US
            #TODO: something for rocketfuel once we feed that in to the GeocronExperiment
            if args.disasters == default_disasters:
                args.disasters = ['2,2'] #25 nodes
        if args.fprobs is default_fprobs:
            args.fprobs = [str(0.5)]

    if args.show_cmd and args.verbose is None:
        args.verbose = default_verbosity_level

    return args

def makecmds(args):
    # convert commands to pass to geocron-simulator
    separator = '~'
    fprobs = '"%s"' % separator.join(args.fprobs)
    npaths = '"%s"' % separator.join(args.npaths)
    nservers = '"%s"' % separator.join(args.nservers)

    # the heuristics can take parameters so we turn the , into |,
    # which is how ns-3 expects them to be separated
    heuristics = '"%s"' % separator.join(args.heuristics)
    heuristics = heuristics.replace(",", "|")

    # determine # procs for each topology
    procs_per_topology = 1
    remainder_procs_per_topology = 0
    if args.nprocs < len(args.topologies) and not (args.debug):
        print("WARNING: more topology files specified than number of processes to be run. "
              "Exit and re-run with correct parameters to avoid creating too many processes")
    else:
        procs_per_topology = args.nprocs/len(args.topologies)
        remainder_procs_per_topology = args.nprocs % len(args.topologies)
        if args.verbose:
            print("Running %i procs for each of %i topologies" % (procs_per_topology, len(args.topologies)))

    # build a command for each proc, or each topology if its more
    for topology in args.topologies:
        startnum = args.start # reset for each topology

        # determine how many procs for this topology
        numprocs = procs_per_topology
        if remainder_procs_per_topology:
            numprocs += 1
            remainder_procs_per_topology -= 1

        # set nruns for each proc in this topology
        runs_per_proc = args.runs/numprocs
        remainder_runs_per_proc = args.runs % numprocs

        for i in range(numprocs):
            nruns = runs_per_proc
            if remainder_runs_per_proc:
                nruns += 1
                remainder_runs_per_proc -= 1

            cmd = "./waf --run %s --command-template='" % 'geocron-example' #was 'ron'
            if args.debug:
                cmd += "gdb -ex run --args "
            if args.valgrind:
                cmd += "valgrind -v --leak-check=full "
            if args.valgrind and args.debug:
                print("using both valgrind and debug not supported!")
                system.exit()
            cmd += r'%s '

            # first, ns3 typeId system configurations
            cmd += '--ns3::GeocronExperiment::TopologyType=%s ' % args.topology_type

            if args.extra_args:
                cmd += args.extra_args + ' '

            # individual parameters
            if args.disasters != default_disasters:
                disasters = ('"%s"' % '-'.join(args.disasters))
            else:
                disasters = default_disasters[topology]
            cmd += '--disaster=%s ' % disasters

            # set topology type specific parameters
            if args.topology_type == 'rocketfuel':
                cmd += '--file=rocketfuel/maps/%s.cch ' % topology
            elif args.topology_type == 'brite':
                cmd += '--file=%s ' % topology
            else:
                print("Unrecognized topology generator type %s, exiting..." % topology_type)
                system.exit()

            cmd += '--fail_prob=%s ' % fprobs
            cmd += '--runs=%i ' % nruns
            cmd += '--start_run=%i ' % startnum
            cmd += '--heuristic=%s ' % heuristics
            cmd += '--contact_attempts=%d ' % args.contact_attempts
            cmd += '--seed=%d ' % args.seed
            cmd += '--npaths=%s ' % npaths
            cmd += "--timeout=%f " % args.timeout
            cmd += '--nservers=%s ' % nservers

            # static args
            if args.topology_type == 'rocketfuel':
                cmd += "--latencies=rocketfuel/weights/all_latencies.intra "
                cmd += "--locations=rocketfuel/city_locations.txt "

            ## $$$$ NO LONGER ASSUME SPACES " " AFTER COMMANDS!

            if args.verbose > -1:
                cmd += ' --verbose=%i' % args.verbose
            cmd += "'" #terminate command template (non-waf args)

            if args.visualize:
                cmd += ' --visualize'

            startnum += nruns

            yield cmd

# Main
if __name__ == "__main__":

    import sys, os, subprocess, signal

    args = parse_args(sys.argv[1:])

    if args.log:
        os.environ['NS_LOG'] = ':'.join(args.log)

    #TODO: fix these 2
    #if args.optimized:
        #subprocess.call("./waf -d optimized configure --enable-examples --disable-python", shell=True)

    # run waf build if we're using multiple processes
    if args.nprocs > 1 and subprocess.call("./waf build", shell=True) == 1:
        exit(1) #error building

    children = []
    for cmd in makecmds(args):
        if args.verbose:
            print(cmd)
        if args.show_cmd:
            exit(0)

        children.append(subprocess.Popen(cmd, shell=True))

    def __sigint_handler(sig, frame):
        '''Called when user presses Ctrl-C to kill whole process.  Kills all children.'''
        for c in children:
            c.terminate()
        #subprocess.call('pkill ron') #seems to not need this currently?
        exit(1)

    signal.signal(signal.SIGINT, __sigint_handler)

    # wait for children
    for c in children:
        c.wait()

    if not args.no_email:
        subprocess.call('ssmtp kyle.edward.benson@gmail.com < done_sims.email', shell=True)
        #TODO: add stuff to email: time,

    #if args.optimized:
      #  subprocess.call("./waf configure --enable-examples --enable-tests", shell=True)

    if args.log:
        del os.environ['NS_LOG']

#!/bin/bash

# config vars
verbosity_level=1
runs=500
start=0 # which run number to start on
nprocs=8

################################
## ARGS

if [ $nprocs -gt 1 ]
then
    parallel=1
else
    parallel=0
fi

# check args

if [ "$1" == "test" -o "$1" == "debug" ]
then
    verbose=1
    testing=1
    parallel=0
fi

if [ "$1" == "debug" ]
then
    debug=1
fi

if [ "$1" == "visualize" -o "$1" == "vis" ]
then
    visualize=1
fi

if [ "$1" == "--as" ]
then
    AS_choices=$2
else
#AS_choices='1239' # 3356 2914' #sprint#Level 3, Verio, Sprintlink
    AS_choices='3356' # 1239
#6461' # 1755 3967' #smaller ones
fi

######## MAIN

## Set trap for Ctrl-C to kill all processes
function kill_children {
    # we just have to do this line for now...
    pkill ron

    for pid in $pids
    do
	echo "killing $pid..."
	kill -9 $pid
    done
    echo "exiting..."
    exit
}

trap kill_children SIGINT SIGTERM SIGHUP

#set number of runs for each spawned process if we run more than one proc
if [ $parallel ]
then
    remainder=$((runs%nprocs))
    for i in `seq $nprocs`;
    do
	this_proc_runs=$((runs/nprocs))
	# split the remainder when divided evenly among the first #remainder procs
	if [ $remainder -gt 0 ]
	then
	    this_proc_runs=$((this_proc_runs+1))
	    remainder=$((remainder-1))
	fi
	nruns="$nruns $this_proc_runs"
    done
    
fi

fail_probs='"0.5-0.4-0.3-0.2-0.1-0.0"'
#fail_probs='"0.1-0.2-0.3-0.4-0.5-0.6-0.7"'
#fail_probs='"0.9-0.8-0.0"' #for testing IID-ness of sims
disasters[1755]='"Amsterdam,_Netherlands-London,_UnitedKingdom-Paris,_France"'
disasters[3967]='"Herndon,_VA-Irvine,_CA-Santa_Clara,_CA"'
disasters[6461]='"San_Jose,_CA-Los_Angeles,_CA-New_York,_NY"'
disasters[3356]='"Los_Angeles,_CA-New_York,_NY"' #Miami,_FL-
#disasters[3356]='"Miami,_FL-New_York,_NY-"'
disasters[2914]='"New_York,_NY-New_Orleans,_LA-Irvine,_CA"'
disasters[1239]='"New_York,_NY-Dallas,_TX"' #Washington,_DC
heuristics='"2-1"' # random, orthogonal network path

for AS in $AS_choices;
do
    for runs in $nruns;
    do
    #out_dir=ron_output/$pfail/$AS/$disaster/`if [ "$local_overlays" == '0' ]; then echo external; else echo internal; fi;`
    #mkdir --parent $out_dir
               	    #echo $out_dir/run$i.out
    #command="./waf --run "
	command="./waf --run ron --command-template='"

    #command="$command'ron "

	if [ $debug ]
	then
	    command="$command""gdb --args "
	fi

	command="$command%s "
	command="$command--fail_prob=$fail_probs "
	command="$command--file=rocketfuel/maps/$AS.cch "
	command="$command--disaster=${disasters[$AS]} "
	command="$command--runs=$runs "

	if [ $parallel ]
	then
	    command="$command--start_run=$start "
	    start=$((start+runs))
	fi

	command="$command--latencies=rocketfuel/weights/all_latencies.intra "
	command="$command--locations=rocketfuel/city_locations.txt "
	command="$command--contact_attempts=20 --timeout=0.5 --heuristic=$heuristics"

    ##### $$$$ NO LONGER ASSUME SPACES " " AFTER COMMANDS
	
	if [ $verbose ];
	then
	    command="$command --verbose=$verbosity_level"
	fi
	
	command="$command'"  ##### terminate ron program's args

	if [ $visualize ];
	then
	    command="$command --visualize"
	fi

	if [ $verbose ];
	then
	    echo $command
	fi
	
    # MUST use eval, or waf parses things strangely and tries to use the ron program's args...
	if [ "$parallel" ]
	then
	    eval $command &
	else
	    eval $command
	fi
	#echo $command
#quit after first run if argument is 'test'
	if [ $testing ];
	then
	    break
	    #exit
	fi

        # collect PIDs so we can wait on them                                                                                                                                                                                                                        
        pids="$pids $!"
    done
    if [ $testing ];
    then
	break
    fi
done

################################
## Done spawning processes
# wait for them to complete
# if they are there
date > waiting-pids.time
if [ $parallel ]
then
    ## TODO: fix this, it doesn't work!!!!!!!!!!!!!!!!!!!!
    ## First, we need to add the PIDs of the actual programs (we only got the bash scripts)
    #NAME=`basename "${BASH_SOURCE[0]}"`/build/scratch/ron/ron
    NEW_PIDS=`ps -o pid -C ron | tail -n +2`

    for pid in $NEW_PIDS
    do
	pids="$pids $pid"
    done

    for pid in $pids
    do
	wait $pid
    done
fi

date > done-waiting-pids.time
ssmtp kyle.edward.benson@gmail.com < done_sims.email

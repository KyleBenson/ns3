#!/bin/bash

fail_probs='0.1 0.2 0.3 0.4 0.5 0.6 0.7 0.8'
AS_choices='3356' #'3356 2914 1239' #Level 3, Verio, Sprintlink
#'6461 1755 3967'
disasters[1755]='Amsterdam,_Netherlands London,_UnitedKingdom Paris,_France'
disasters[3967]='Herndon,_VA Irvine,_CA Santa_Clara,_CA'
disasters[6461]='San_Jose,_CA Los_Angeles,_CA New_York,_NY'
disasters[3356]='New_York,_NY' #Miami,_FL Los_Angeles,_CA'
disasters[2914]='New_York,_NY New_Orleans,_LA Irvine,_CA'
disasters[1239]='Dallas,_TX' #New_York,_NY Washington,_DC'
heuristics='random orthogonal' #'0 1'

for AS in $AS_choices
do
    for disaster in ${disasters[$AS]}
    do
	pfail_label=''
	trace_dirs=''
	for pfail in $fail_probs
	do
	    for heuristic in $heuristics
	    do
		trace_dirs+=ron_output/$AS/$disaster/$pfail/$heuristic' '
		pfail_label+=$pfail' '
	    done
	done

	./ron_trace_analyzer.py --dirs $trace_dirs -ns --t_test -s -i -t -c --label $pfail_label --prepend_label 'p(fail)=' --append_title "(AS $AS - ${disaster//_/ })" 
    done
done



#!/bin/bash
if [ "$1" == '-nw' ];
then em geocron_simulator_runner.py src/geocron/model/* src/geocron/helper/* src/geocron/test/geocron-test-suite.cc
else emacs geocron_simulator_runner.py src/geocron/model/* src/geocron/helper/* src/geocron/test/geocron-test-suite.cc &
fi

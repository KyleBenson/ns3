#!/bin/bash
./waf --run test-runner --command-template='gdb -ex run -ex quit --args %s --suite=geocron --verbose'

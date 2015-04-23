#!/bin/bash
./test.py -s geocron -v -t .results.txt
less .results.txt
rm .results.txt
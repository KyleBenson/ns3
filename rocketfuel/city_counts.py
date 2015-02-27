#! /usr/bin/python

city_counts_description = '''\
Goes through a specified Rocketfuel .cch file and prints the
degree of each of the city vertices.'''

import os, os.path, string, sys

if __name__ == "__main__":
    
    if len(sys.argv) < 2:
        print city_counts_description
        exit()

    degree_map = {}

    with open(sys.argv[1]) as f:
        for line in f.readlines():
            if line.startswith('-'):
                break

            (nodeID, city, garbage) = line.split(' ', 2)

            city = city.replace('@', '').replace('+',' ')
            degree_map[city] = degree_map.get(city,0) + 1

    sorted_pairs = sorted(zip(degree_map.values(), degree_map.keys()))
    for v,k in sorted_pairs:
        print '%-30s %s' % (k, v)


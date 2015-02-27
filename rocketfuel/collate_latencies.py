#! /usr/bin/python

collate_latencies_description = '''\
Recursively gathers together all latencies.intra files in the directory
specified as arg1 in order to build a single file that has every single
latency estimate in one file.'''

import os, os.path, string, sys

target_filename = 'latencies.intra'

if __name__ == "__main__":
    
    if len(sys.argv) < 2:
        print collate_latencies_description
        exit()

    latency_map = {}

    for (dirpath, dirnames, filenames) in os.walk(sys.argv[1]):
        if target_filename not in filenames:
            continue

        with open(os.path.join(dirpath, target_filename)) as f:
            for line in f.readlines():
                (from_city, to_city, latency) = line.split()

                key = (from_city.rstrip(string.digits),
                       to_city.rstrip(string.digits)) #.replace('+', ' ')

                if key not in latency_map:
                    latency_map[key] = latency

    with open(os.path.join(sys.argv[1], "all_" + target_filename), 'w') as f:
        for k,v in latency_map.items():
            f.write('%s %s %s\n' % (k[0], k[1], v))

#! /usr/bin/python
NEW_SCRIPT_DESCRIPTION = '''Read an input file (specified on the command line) and a directory full of .cch rocketfuel files (also specified via command line), parsing each file for the requested cities, looking up the associated latitude and longitude of the city, outputting just the city names and lat/lons to another file.'''

# @author: Kyle Benson
# (c) Kyle Benson 2012

import argparse, sys
from string import maketrans
#from os.path import isdir
#from os import listdir
#from getpass import getpass
#password = getpass('Enter password: ')

def ParseArgs(args=sys.argv[1:]):
##################################################################################
#################      ARGUMENTS       ###########################################
# ArgumentParser.add_argument(name or flags...[, action][, nargs][, const][, default][, type][, choices][, required][, help][, metavar][, dest])
# action is one of: store[_const,_true,_false], append[_const], count
# nargs is one of: N, ?(defaults to const when no args), *, +, argparse.REMAINDER
# help supports %(var)s: help='default value is %(default)s'
##################################################################################

    parser = argparse.ArgumentParser(description=NEW_SCRIPT_DESCRIPTION,
                                     #formatter_class=argparse.RawTextHelpFormatter,
                                     #epilog='Text to display at the end of the help print',
                                     )

    parser.add_argument('--infile',
                        help='''file from which to read city location data''')
    parser.add_argument('--outfile',
                        help='''file to which we output the requested cities' location data''')
    parser.add_argument('--cch_files', nargs='+',
                        help='''files from which to read city names that we're requesting location data about''')

    return parser.parse_args(args)


def ParseCchCityName(line):
    #name = line.split(' ')[1].replace('@','').replace('+',' ')
    ## Ignore blank lines
    if line.strip() == "":
        return None
    try:
        name = line.split(' ')[1].translate(maketrans('+',' '), '@')
    except IndexError:
        #print "Error splitting line: %s" % line
        name = None
    ## Hacky name massaging
    '''
    if name == 'Washington, DC':
        name = 'Washington, D. C.'
    if name == 'New York, NY':
        name = 'New York City, NY'''
    return name

def ReadCchFile(filename, cities):
    with open(filename) as f:
        for line in f.readlines():
            ## external links at end of file don't have city names
            if line.startswith('-'):
                break

            city_name = ParseCchCityName(line)
            if city_name is not None and city_name not in cities and city_name not in ['IAD', 'T']:
                cities[city_name] = None

    return cities

def ReadRequestedCities(cch_files):
    cities = {}
    for f in cch_files:
        cities = ReadCchFile(f, cities)

    return cities

nameidx = 1
countryidx = 8
stateidx = 10
latidx = 4
lonidx = 5
def ParseCityLocation(line):
    parts = line.split('\t')
    try:
        name = parts[nameidx]
        # Remove 'City' for NY, and everything after the ',' for Washington, DC (state code will add it back)
        name = name.split(',')[0].replace('New York City', 'New York').replace('Saint','St.')
        if parts[countryidx] == 'US':
            name = name + ', ' + parts[stateidx]

        return name, parts[latidx], parts[lonidx]
    except IndexError:
        print "Error parsing line: %s" % line

def GetCityLocations(cities, locfile):
    '''Go through the locations file and record the lat/lon of each city that's been requested from the CCH files'''
    with open(locfile) as infile:
        for line in infile.readlines():
            city, lat, lon = ParseCityLocation(line)

            if city is None or lat is None or lon is None:
                print "Invalid parsed city attributes: name=%s lat=%s lon=%s" % (city, lat, lon)

            if city in cities:
                #print city, lat, lon
                cities[city] = (lat, lon)

    return cities
        
def WriteLocations(cities, outfile):
    with open(outfile, 'w') as f:
        for city,latlon in cities.iteritems():
            try:
                f.write('\t'.join([city,latlon[0],latlon[1]]) + '\n')
            except:
                print 'Error writing to outfile: city=%s, latlon=%s' % (city, latlon)


def Main(args):
    cities = ReadRequestedCities(args.cch_files)
    cities = GetCityLocations(cities, args.infile)
    WriteLocations(cities, args.outfile)

# Main
if __name__ == "__main__":
    
    import sys

    args = ParseArgs(sys.argv[1:])

    Main(args)

    

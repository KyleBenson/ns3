#########################################################################
## Copyright 2007
## The Regents of the University of California
## All Rights Reserved
##
## Permission to use, copy, modify and distribute any part of
## this software package for educational, research and non-profit
## purposes, without fee, and without a written agreement is hereby
## granted, provided that the above copyright notice, this paragraph
## and the following paragraphs appear in all copies.
##
## Those desiring to incorporate this into commercial products or
## use for commercial purposes should contact the Technology Transfer
## Office, University of California, San Diego, 9500 Gilman Drive,
## La Jolla, CA 92093-0910, Ph: (858) 534-5815, FAX: (858) 534-7345.
##
## IN NO EVENT SHALL THE UNIVERSITY OF CALIFORNIA BE LIABLE TO ANY
## PARTY FOR DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL
## DAMAGES, INCLUDING LOST PROFITS, ARISING OUT OF THE USE OF THIS
## SOFTWARE, EVEN IF THE UNIVERSITY OF CALIFORNIA HAS BEEN ADVISED
## OF THE POSSIBILITY OF SUCH DAMAGE.
##
## THE SOFTWARE PROVIDED HEREIN IS ON AN "AS IS" BASIS, AND THE
## UNIVERSITY OF CALIFORNIA HAS NO OBLIGATION TO PROVIDE MAINTENANCE,
## SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS. THE UNIVERSITY
## OF CALIFORNIA MAKES NO REPRESENTATIONS AND EXTENDS NO WARRANTIES
## OF ANY KIND, EITHER IMPLIED OR EXPRESS, INCLUDING, BUT NOT LIMITED
## TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY OR FITNESS FOR A
## PARTICULAR PURPOSE, OR THAT THE USE OF THE SOFTWARE WILL NOT
## INFRINGE ANY PATENT, TRADEMARK OR OTHER RIGHTS.
##
##
## Written by Priya Mahadevan <pmahadevan@cs.ucsd.edu>  and
##            Calvin Hubble <chubble@cs.ucsd.edu
##
#########################################################################

CC= g++
CXX= g++
DEBUG = -g #-pg#

OS = LINUX
CCFLAGS= -Wall $(DEBUG) -D$(OS) -O2 
CXXFLAGS= -Wall $(DEBUG) -D$(OS) -O2
INCS=  -I/home/kyle/Downloads/boost_1_49_0/
LDFLAGS += -lboost_program_options

all: dkRewire dkTopoGen0k dkTopoGen1k dkTopoGen2k dkDist dkRescale dkMetrics

COMMON_SRCS= dkUtils.cc

DKREWIRE_SRCS= $(COMMON_SRCS) dkRewire.cc #randomizeGraph1k.h randomizeGraph2k.h \
#	randomizeGraph3k.h
dkRewire: $(DKREWIRE_SRCS)
	$(CXX) $(CXXFLAGS) $(INCS) $^ -o $@ $(LDFLAGS)

DKTOPOGEN_0K_SRCS= $(COMMON_SRCS) dkTopoGen0k.cc #dkTopoGen1k.h
dkTopoGen0k: $(DKTOPOGEN_0K_SRCS)
	$(CXX) $(CXXFLAGS) $(INCS) $^ -o $@  $(LDFLAGS)

DKTOPOGEN_1K_SRCS= $(COMMON_SRCS) dkTopoGen1k.cc #dkTopoGen1k.h
dkTopoGen1k: $(DKTOPOGEN_1K_SRCS)
	$(CXX) $(CXXFLAGS) $(INCS) $^ -o $@ $(LDFLAGS)

DKTOPOGEN_2K_SRCS= $(COMMON_SRCS) dkTopoGen2k.cc dkTopoGen2k.h
dkTopoGen2k: $(DKTOPOGEN_2K_SRCS)
	$(CXX) $(CXXFLAGS) $(INCS) $^ -o $@ $(LDFLAGS)

DKDIST_SRCS= $(COMMON_SRCS) dkDist.cc
dkDist: $(DKDIST_SRCS)
	$(CXX) $(CXXFLAGS) $(INCS) $^ -o $@ $(LDFLAGS)

DKRESCALE_SRCS= $(COMMON_SRCS) dkRescale.cc
dkRescale: $(DKRESCALE_SRCS)
	$(CXX) $(CXXFLAGS) $(INCS) $^ -o $@ $(LDFLAGS)

DKMETRICS_SRCS= $(COMMON_SRCS) dkMetrics.cc
dkMetrics: $(DKMETRICS_SRCS)
	$(CXX) $(CXXFLAGS) $(INCS) $^ -o $@ $(LDFLAGS)


#include $(DKTOPOGEN_1K_SRCS:.o:=.d)
#include $(OBJS:.o:=.d)
#include $(subst .cc,.d,$(DKREWIRE_SRCS))

%.d : %.cc
	set -e; $(CXX) -MM $(CPPFLAGS) $< \
	| sed 's/\($*\)\.o[ :]*/\1.o $@ : /g' > $@; \
	[ -s $@ ] || rm -f $@


#%.o : %.c
#	$(CC) -c $(CCFLAGS) $<
#
#%.o : %.cc 
# 	$(CC) -c $(CCFLAGS) $<
#
# $(TARGET): $(OBJS)
# 	$(CC) -o $(TARGET) $(OBJS) $(CCFLAGS) $(LDFLAGS) 

clean:
	rm -f $(TARGET) core *.o *~ dkRewire dkTopoGen0k dkTopoGen1k dkTopoGen2k dkRescale dkDist dkMetrics

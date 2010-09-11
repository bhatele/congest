# ==============================================================================
# Common Variables
INC	= ../../TopoMgrAPI
# ==============================================================================
# Blue Gene/L

# ==============================================================================
# Blue Gene/P
CC      = mpixlc
CXX     = mpixlcxx
COPTS   = -c -O3 -DCMK_BLUEGENEP=1 -I$(INC)
LOPTS   =

# ==============================================================================
# Cray XT3 (BigBen)
#CC      = cc
#CXX     = CC
#COPTS   = -c -O3 -DCMK_CRAYXT -DXT3_TOPOLOGY=1 #-DCRAYNBORTABLE=`pwd`/CrayNeighborTable
#LOPTS   = 

# ==============================================================================
# Cray XT4/5 (Jaguar, Kraken)
#CC      = cc
#CXX     = CC
#COPTS   = -c -O3 -DCMK_CRAYXT -DXT5_TOPOLOGY=1
#LOPTS   = -lrca -lhpm 

all: partial flow

bandwidthX: bandwidth.C
	$(CXX) $(COPTS) -o bandwidth.o bandwidth.C
	$(CXX) -o bandwidth bandwidth.o $(INC)/libtmgr.a $(LOPTS)

full: full_overlap.C
	$(CXX) $(COPTS) -o full.o full_overlap.C
	$(CXX) -o full full.o $(INC)/libtmgr.a $(LOPTS)

partial: partial_overlap.C
	$(CXX) $(COPTS) -o partial.o partial_overlap.C
	$(CXX) -o partial partial.o $(INC)/libtmgr.a $(LOPTS)

flow: flow.C 
	$(CXX) $(COPTS) -o flow.o flow.C
	$(CXX) -o flow flow.o $(INC)/libtmgr.a $(LOPTS)

clean:
	rm -f *.o partial flow


# ==============================================================================
# Common Variables
INC	= ../../TopoMgrAPI

# ==============================================================================
# Blue Gene/P
CC      = mpixlc
CXX     = mpixlcxx
COPTS   = -c -O3 -DCMK_BLUEGENEP=1 -I$(INC)
LOPTS   =

# ==============================================================================
# Cray XT4/5 (Jaguar, Kraken)
#CC      = cc
#CXX     = CC
#COPTS   = -c -O3 -DCMK_CRAYXT -DXT5_TOPOLOGY=1
#LOPTS   = -lrca -lhpm 

all: wocon wicon wicon2

wocon: wocon.c
	$(CC) $(COPTS) -o wocon.o wocon.c
	$(CC) -o wocon wocon.o

wicon: wicon.c
	$(CC) $(COPTS) -DRANDOMNESS=0 -o wicon.o wicon.c
	$(CC) -o wicon-nn wicon.o
	$(CC) $(COPTS) -DRANDOMNESS=1 -o wicon.o wicon.c
	$(CC) -o wicon-rnd wicon.o

wicon2: wicon2.C
	$(CXX) $(COPTS) -o wicon2.o wicon2.C
	$(CXX) -o wicon2 wicon2.o $(INC)/libtmgr.a $(LOPTS)

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
	rm -f *.o wocon wicon-nn wicon-rnd wicon2 partial flow


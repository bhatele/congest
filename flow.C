/** \file bandwidth.C
 *  Author: Gagan 
 *  Date Created: May 4, 2010
 *
 *  Bandwidth Benchmark:
 *  --------------------------------------------------------------------------
 *  It creates contention on a specific line by adding more
 *  and more pairs around a given link and reports the bandwidth per rank.
 *  Rank  Msg_Size Time Bandwidth
 *  Q. How to figure out if other paths are being used to send the data? 
 */
#include <iostream>
#include <iomanip>
#include <fstream>
#include <string>
#include <sstream>

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "TopoManager.h"
using namespace std;
#if USE_HPM
  #include <libhpm.h>

extern "C" {
void HPM_Init(void);
void HPM_Start(char *);
void HPM_Stop(char *);
void HPM_Print(void);
void HPM_Print_Flops(void);
void HPM_Print_Flops_Agg(void);
void HPM_Flops( char *, double *, double * );
}
#endif

// Minimum message size (bytes)
#define MIN_MSG_SIZE 4

// Maximum message size (bytes)
#define MAX_MSG_SIZE (1024 * 1024)

#define NUM_MSGS 10

#define wrap_z(a)	(((a)+dimNZ)%dimNZ)

/* Creating a map for processors to use
 */
void dump_map(int size, int * map)
{
  int i;
  for(i = 0; i < size; i++) {
    printf("map[%03d] = %03d\n", i, map[i]); 
  }

  fflush(stdout); 
}

void check_map(int size, int *map) {
  int i;
  for(i = 0; i < size; i++) {
    if(map[i] != -1)
      if(i != map[map[i]]) {
	printf("map[%d]=%d map[%d]=%d\n", i,map[i], map[i], map[map[i]]);
	dump_map(size,map);
	abort();
      }
  }
}

void build_process_map(int size, int *smap, int *rmap, int *pmap, int file)
{
  TopoManager tmgr;
  int pe, pe1, pe2, x, y, z1, t;
  int dimNX, dimNY, dimNZ, dimNT;
  dimNX = tmgr.getDimNX();
  dimNY = tmgr.getDimNY();
  dimNZ = tmgr.getDimNZ();
  dimNT = tmgr.getDimNT();
  int count = 0;
  for(int i=0; i<size; i++)
  {
      smap[i]=-1;
      rmap[i]=-1;
      pmap[i]=-1;

  }
  cout << "Loading Map" << endl;
  char name[50];
  sprintf(name,"%d.map",file);
  ifstream mapFile(name);
  string line_s;
  while(mapFile.good()   ){
    #ifdef DEBUG
      cout << "     >  Loading " << name << endl;
    #endif
    int c1,c2,c3,c4,c5,c6;
    getline(mapFile,line_s);
    istringstream line(line_s);
    line >> c1 >> c2 >>c3 >> c4 >> c5>> c6;

    for(int i=0;i<dimNZ;i++)
    {
      pe = tmgr.coordinatesToRank(c1, c2, i, 0);
      pe1 = tmgr.coordinatesToRank(c4, c5, i, 0);
      smap[pe] = pe1;
      rmap[pe1] = pe;
      if(i==0)
      {
	pmap[pe] =1;
	pmap[pe1]=2;
      }
    }
  }
  dump_map(size,rmap);
  dump_map(size,smap);
}

int main(int argc, char *argv[]) {
  int numprocs, myrank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  double sendTime, recTime, min, avg, max;
  double time[3] = {0.0, 0.0, 0.0};
  int msg_size;
  MPI_Status mstat;
  int i=0,j, pe, pe1, pe2, trial, hops;
  char name[30];
  char locname[30];
  char blockname[50];
  double newTime, oldTime;
  double storeTime[NUM_MSGS];
  double recvTime[NUM_MSGS];
  double storeBw[NUM_MSGS];
  char *send_buf = (char *)malloc(MAX_MSG_SIZE);
  char *recv_buf = (char *)malloc(MAX_MSG_SIZE);
  FILE *locf;
  for(i = 0; i < MAX_MSG_SIZE; i++) {
    recv_buf[i] = send_buf[i] = (char) (i & 0xff);
  }

  // allocate the routing map.
  int *rmap = (int *) malloc(sizeof(int) * numprocs);
  int *smap = (int *) malloc(sizeof(int) * numprocs);
  
  TopoManager *tmgr;
  int dimNZ, numRG, x, y, z, t, bcastSend[3], bcastRecv[3];

  if(myrank == 0) {
    tmgr = new TopoManager();
#if CREATE_JOBS
    numRG = tmgr->getDimNX() * (tmgr->getDimNY() - 2) * 2 * tmgr->getDimNT();
#else
    numRG = tmgr->getDimNX() * tmgr->getDimNY() * 2;
#endif
    dimNZ = tmgr->getDimNZ();
    for (int i=1; i<numprocs; i++) {
      bcastSend[0] = dimNZ;
      bcastSend[1] = numRG;
      tmgr->rankToCoordinates(i, x, y, z, t);
      bcastSend[2] = z;
      MPI_Send(bcastSend, 3, MPI_INT, i, 1, MPI_COMM_WORLD);
    }
    tmgr->rankToCoordinates(0, x, y, z, t);
  } else {
      MPI_Recv(bcastRecv, 3, MPI_INT, 0, 1, MPI_COMM_WORLD, &mstat);
      dimNZ = bcastRecv[0];
      numRG = bcastRecv[1];
      z = bcastRecv[2];
  }
  MPI_Barrier(MPI_COMM_WORLD);
  if (myrank == 0) {
    printf("Torus Dimensions %d %d %d %d\n", tmgr->getDimNX(), tmgr->getDimNY(), dimNZ, tmgr->getDimNT());
  }
#if USE_HPM
    HPM_Init();
#endif
  for (hops=0; hops < 1; hops++) {
    // To print the recv times for certain ranks
    int *pmap = (int *) malloc(sizeof(int) * numprocs);
    if (myrank == 0) {
      // Rank 0 makes up a routing map.
      build_process_map(numprocs, smap, rmap, pmap, 2);
    }
    // Broadcast the routing map.
    MPI_Bcast(smap, numprocs, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(rmap, numprocs, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(pmap, numprocs, MPI_INT, 0, MPI_COMM_WORLD);
    sprintf(blockname, "Block_%d.hpm",hops);
    if (myrank == 0) {
       printf( " Broadcasted the map \n");
    }
#if USE_HPM
    HPM_Start(blockname);
#endif
#if CREATE_JOBS
    sprintf(name, "xt4_job_%d_%d.dat", numprocs, hops);
#else
    sprintf(name, "bgp_line_%d_%d.dat", numprocs, hops);
#endif
	if(pmap[myrank]>0)
	{
	sprintf(locname, "bgp_print_%d.dat", myrank);
	locf = fopen(locname, "a");
   	}
    for (msg_size=MIN_MSG_SIZE; msg_size<=MAX_MSG_SIZE; msg_size=(msg_size<<1)) {
      for (trial=0; trial<1; trial++) {
	 if (myrank == 0) {
	     printf( " Going to begin the trial \n");
	  }
	pe1 = smap[myrank]; // Am I a sender?
	pe2 = rmap[myrank]; // Am I a reciever? 
	MPI_Barrier(MPI_COMM_WORLD);
	// Actual Data Transfer
	if(pe1 != -1) {
	    sendTime = MPI_Wtime();
	    oldTime = sendTime;
	    j=0;
	    for(i=0; i<NUM_MSGS; i++)
	    {
		  storeTime[i] = MPI_Wtime(); // Just before the next send operation
		  MPI_Send(send_buf, msg_size, MPI_CHAR, pe1, 1, MPI_COMM_WORLD);
	    }
	    MPI_Recv(recv_buf, msg_size, MPI_CHAR, pe1, 1, MPI_COMM_WORLD, &mstat);
	    recTime = (MPI_Wtime() - sendTime) / (NUM_MSGS+1);
	    //printf(" My Rank : %d Experiment: %d  MSG_SIZE: %d -- Completed send recv \n", myrank, hops, msg_size);
	  }
	if(pe2 !=1)
	{
	    sendTime = MPI_Wtime();
	    oldTime = sendTime;
	    j=0;
	    for(i=0; i<NUM_MSGS; i++)
	      {
		  MPI_Recv(recv_buf, msg_size, MPI_CHAR, pe2, 1, MPI_COMM_WORLD, &mstat);
		  recvTime[i] = MPI_Wtime(); // Just after the next recv operation
	      }	  
	    MPI_Send(send_buf, msg_size, MPI_CHAR, pe2, 1, MPI_COMM_WORLD);
	    recTime = (MPI_Wtime() - sendTime) / (NUM_MSGS+1);
        }
	// Recv times sent back to the Senders for b/w calculations 
	if(myrank==0)
	{
	  printf(" My Rank : %d Experiment: %d  MSG_SIZE: %d -- Reached barrier in middle \n", myrank, hops, msg_size);
	}
	pe1 = smap[myrank]; // Am I a sender?
	pe2 = rmap[myrank]; // Am I a reciever? 
	MPI_Barrier(MPI_COMM_WORLD);
	if(pe1 != -1) {
	    MPI_Recv(recvTime, NUM_MSGS, MPI_DOUBLE, pe1, 1, MPI_COMM_WORLD, &mstat);
	    if(pmap[myrank]==1)
	    {
	      printf(" My Rank : %d Hops: %d  MSG_SIZE: %d Sender Side Exp trial: %d   Avg recv time %g \n", myrank, hops, msg_size, trial, recTime );
	      //printf(" My Rank : %d Hops: %d  MSG_SIZE: %d Sender Side Exp trial: %d   Recv time %g \n", myrank, hops, msg_size, trial, recvTime );
	      for(i=0;i<NUM_MSGS; i++)
		{
		  storeBw[i]= msg_size/(recvTime[i] - storeTime[i]);
		  fprintf(locf,  "%d   %d   %d   %g   %g   %g   %g \n", hops, myrank, msg_size, 500000*(storeTime[i]+recvTime[i]), storeBw[i],1000000*recvTime[i],1000000*storeTime[i]); 
		}
	    }
	  }
	if(pe2 !=1) 
	    {
	      MPI_Send(recvTime, NUM_MSGS, MPI_DOUBLE, pe2, 1, MPI_COMM_WORLD);
	    }
      } // end for loop of trials
    } // end for loop of msgs
    if(pmap[myrank]>0)
	{
  		fflush(NULL);
		fclose(locf);
	}
    free(pmap);
#if USE_HPM
  HPM_Stop(blockname);
#endif
  } // end for loop of hops
#if USE_HPM
  HPM_Print();
#endif
  if(myrank == 0)
    printf("Program Complete\n");
  MPI_Finalize();
  return 0;
}

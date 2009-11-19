/*****************************************************************************
 * $Source$
 * $Author$
 * $Date$
 * $Revision$
 *****************************************************************************/

/** \file contention_hops.C
 *  Author: Abhinav S Bhatele
 *  Date Created: October 23rd, 2008
 *  E-mail: bhatele@illinois.edu
 *
 *  This benchmark compares hop bytes with max dilation and shows that max
 *  dilation is important too.
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include "TopoManager.h"

#if USE_HPM
extern "C" void HPM_Init(void);
extern "C" void HPM_Start(char *);
extern "C" void HPM_Stop(char *);
extern "C" void HPM_Print(void);
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

void build_process_map(int size, int *map, int mode)
{
  /** if mode == 1, most ranks send 1 hop away
   *  if mode == 2, everyone sends 2 hops away
   */
  TopoManager tmgr;
  int x, y, z, t, z1, t1 = 0;
  int dimNX, dimNY, dimNZ, dimNT;
  int away = (mode + 1) / 2;

  dimNX = tmgr.getDimNX();
  dimNY = tmgr.getDimNY();
  dimNZ = tmgr.getDimNZ();
  dimNT = tmgr.getDimNT();

  if (away % 2 == 1) {
    // most ranks send 1 hop away
    for(int i=0; i<size; i++) {
      tmgr.rankToCoordinates(i, x, y, z, t);
      if (t < dimNT/2) {
	if (z%2 == 0)
	  z1 = wrap_z(z + away);
	else
	  z1 = wrap_z(z - away);
      }
      else {
	if (z%2 == 0)
	  z1 = wrap_z(z - away);
	else
	  z1 = wrap_z(z + away);
      }
      
      t1 = t;
      if(mode == 2) {
	// for now this will only work on a 8 x 8 x 16 partition
	// we swap some of the ranks
	if(z == 0) z1 = 8;
	if(z == 8) z1 = 0;
	if(z == 1  && t < dimNT/2)  { z1 = 15; t1 = t+2; }
	if(z == 15 && t >= dimNT/2) { z1 = 1;  t1 = t-2; }
	if(z == 7  && t >= dimNT/2) { z1 = 9;  t1 = t-2; }
	if(z == 9  && t < dimNT/2)  { z1 = 7;  t1 = t+2; }
      }
      map[i] = tmgr.coordinatesToRank(x, y, z1, t1);
    }
  } else {
    // send 2 hops away
    for(int i=0; i<size; i++) {
      tmgr.rankToCoordinates(i, x, y, z, t);
      if (t < dimNT/2) {
	if (z%4 < 2)
	  z1 = z + away;
	else
	  z1 = z - away;
      } else {
	if (z%4 < 2)
	  z1 = wrap_z(z - away);
	else
	  z1 = wrap_z(z + away);
      }

      t1 = t;
      if(mode == 4) {
	// for now this will only work on a 8 x 8 x 16 partition
	// we swap some of the ranks
	if(z == 0) z1 = 8;
	if(z == 8) z1 = 0;
	if(z == 2  && t < dimNT/2)  { z1 = 14; t1 = t+2; }
	if(z == 14 && t >= dimNT/2) { z1 = 2;  t1 = t-2; }
	if(z == 6  && t >= dimNT/2) { z1 = 10; t1 = t-2; }
	if(z == 10 && t < dimNT/2)  { z1 = 6;  t1 = t+2; }
      }
      map[i] = tmgr.coordinatesToRank(x, y, z1, t1);
    }
  }

  int hops = 0;
  for(int i=0; i<size; i++) {
    hops += tmgr.getHopsBetweenRanks(i, map[i]);
  }
  printf("Hops for mode %d = %d\n", mode, hops);
  // dump_map(size, map);
}

int main(int argc, char *argv[]) {
  int numprocs, myrank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  MPI_Request mreq;
  MPI_Status mstat;

  double sendTime, recvTime, time = 0.0;
  int msg_size;
  int i=0, pe, trial, hops;
  char name[30];

  char *send_buf = (char *)memalign(64 * 1024, MAX_MSG_SIZE);
  char *recv_buf = (char *)memalign(64 * 1024, MAX_MSG_SIZE);

  for(i = 0; i < MAX_MSG_SIZE; i++) {
    recv_buf[i] = send_buf[i] = (char) (i & 0xff);
  }

  // allocate the routing map.
  int *map = (int *) malloc(sizeof(int) * numprocs);
  TopoManager *tmgr;
  int sendNZ, dimNZ, bcastNZ[1];

  if(myrank == 0) {
    tmgr = new TopoManager();
    sendNZ = tmgr->getDimNZ();
    bcastNZ[0] = sendNZ;
  }
  
  // Broadcast dimNZ
  MPI_Bcast(bcastNZ, 1, MPI_INT, 0, MPI_COMM_WORLD);
  dimNZ = bcastNZ[0];

  int maxHops = dimNZ;
  
  if (myrank == 0) {
    printf("Torus Dimensions %d %d %d %d hops %d\n", tmgr->getDimNX(), tmgr->getDimNY(), dimNZ, tmgr->getDimNT(), maxHops);
  }

  for (hops=1; hops <= 5; hops++) {
    sprintf(name, "xt4_mode_%d_%d.dat", numprocs, hops);
    // Rank 0 makes up a routing map.
    if (myrank == 0)
      build_process_map(numprocs, map, hops);

    // Broadcast the routing map.
    MPI_Bcast(map, numprocs, MPI_INT, 0, MPI_COMM_WORLD);

#if USE_HPM
    HPM_Init();
    HPM_Start("Sending");
#endif
    for (msg_size=MIN_MSG_SIZE; msg_size<=MAX_MSG_SIZE; msg_size=(msg_size<<1)) {
      for (trial=0; trial<11; trial++) {

	pe = map[myrank];

	if(myrank == 0 && trial > 0) sendTime = MPI_Wtime();
	MPI_Barrier(MPI_COMM_WORLD);

	if(myrank < pe) {
	  for(i=0; i<NUM_MSGS; i++) {
	    MPI_Irecv(recv_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD, &mreq);
	    MPI_Send(send_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD);
	    MPI_Wait(&mreq, &mstat);
	  }

	  MPI_Barrier(MPI_COMM_WORLD);
	  if(myrank == 0 && trial > 0) recvTime = (MPI_Wtime() - sendTime) / NUM_MSGS;
	} else {
	  for(i=0; i<NUM_MSGS; i++) {
	    MPI_Irecv(recv_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD, &mreq);
	    MPI_Send(send_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD);
	    MPI_Wait(&mreq, &mstat);
	  }

	  MPI_Barrier(MPI_COMM_WORLD);
	}

	if(myrank == 0 && trial > 0)
	  time += recvTime;
      }
      if (myrank == 0) {
	FILE *outf = fopen(name, "a");
	fprintf(outf, "%d %g\n", msg_size, time/10);
	fflush(NULL);
	fclose(outf);
	time = 0.0;
      }
    }
#if USE_HPM
    HPM_Stop("Sending");
    HPM_Print();
#endif
  }
 
  if(myrank == 0)
    printf("Program Complete\n");

  MPI_Finalize();
  return 0;
}


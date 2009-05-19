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
 *  This benchmark captures the effect of distance on the message
 *  latencies in the presence of contention. Controlled congestion is
 *  introduced by fixing the number of hops each message travels. Message
 *  latencies are plotted for different message sizes as the number of
 *  hops every message travels increases.
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>
#include "TopoManager.h"

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

void build_process_map(int size, int *map, int away)
{
  TopoManager tmgr;
  int x, y, z, t, z1;
  int dimNX, dimNY, dimNZ;

  dimNX = tmgr.getDimNX();
  dimNY = tmgr.getDimNY();
  dimNZ = tmgr.getDimNZ();

  if (away%2 == 1) {
    // no. of hops is even
    for(int i=0; i<size; i++) {
      tmgr.rankToCoordinates(i, x, y, z, t);      
      if (t < 2) {
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
      map[i] = tmgr.coordinatesToRank(x, y, z1, t);
    }
  } else {
    // no. of hops is odd
    for(int i=0; i<size; i++) {
      tmgr.rankToCoordinates(i, x, y, z, t);      
      if (t < 2) {
	if (z%(2*away) < away)
	  z1 = z + away;
	else
	  z1 = z - away;
      } else {
	if (z%(2*away) < away)
	  z1 = wrap_z(z - away);
	else
	  z1 = wrap_z(z + away);
      }
      map[i] = tmgr.coordinatesToRank(x, y, z1, t);
    }
  }

  if (away == 6) {
    for(int i=0; i<size; i++) {
      tmgr.rankToCoordinates(i, x, y, z, t);      
      if(t < 2) {
	if( ((int)(z/2)) % 2 == 0)
	  z1 = wrap_z(z + away);
	else
	  z1 = wrap_z(z - away);
      } else {
	if( ((int)(z/2)) % 2 == 0)
	  z1 = wrap_z(z - away);
	else
	  z1 = wrap_z(z + away);
      }
      map[i] = tmgr.coordinatesToRank(x, y, z1, t);
    } 
  }
  // dump_map(size, map);
}

int main(int argc, char *argv[]) {
  int numprocs, myrank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

  double sendTime, recvTime, min, avg, max;
  double time[3] = {0.0, 0.0, 0.0};
  int msg_size;
  MPI_Status mstat;
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

  for (hops=1; hops <= maxHops; hops++) {

    sprintf(name, "bgp_hops_%d_%d.dat", numprocs, hops);
    // Rank 0 makes up a routing map.
    if (myrank == 0)
      build_process_map(numprocs, map, hops);

    // Broadcast the routing map.
    MPI_Bcast(map, numprocs, MPI_INT, 0, MPI_COMM_WORLD);

    for (msg_size=MIN_MSG_SIZE; msg_size<=MAX_MSG_SIZE; msg_size=(msg_size<<1)) {
      for (trial=0; trial<10; trial++) {

	pe = map[myrank];

	MPI_Barrier(MPI_COMM_WORLD);

	if(myrank < pe) {
	  // warmup
	  for(i=0; i<2; i++) {
	    MPI_Send(send_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD);
	    MPI_Recv(recv_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD, &mstat);
	  }

	  sendTime = MPI_Wtime();
	  for(i=0; i<NUM_MSGS; i++)
	    MPI_Send(send_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD);
	  for(i=0; i<NUM_MSGS; i++)
	    MPI_Recv(recv_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD, &mstat);
	  recvTime = (MPI_Wtime() - sendTime) / NUM_MSGS;
      
	  // cooldown
	  for(i=0; i<2; i++) {
	    MPI_Send(send_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD);
	    MPI_Recv(recv_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD, &mstat);
	  }

	  MPI_Barrier(MPI_COMM_WORLD);
	} else {
	  // warmup
	  for(i=0; i<2; i++) {
	    MPI_Recv(recv_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD, &mstat);
	    MPI_Send(send_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD);
	  }

	  sendTime = MPI_Wtime();
	  for(i=0; i<NUM_MSGS; i++)
	    MPI_Recv(recv_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD, &mstat);
	  for(i=0; i<NUM_MSGS; i++)
	    MPI_Send(send_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD);
	  recvTime = (MPI_Wtime() - sendTime) / NUM_MSGS;

	  // cooldown
	  for(i=0; i<2; i++) {
	    MPI_Recv(recv_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD, &mstat);
	    MPI_Send(send_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD);
	  }

	  MPI_Barrier(MPI_COMM_WORLD);
	}

	MPI_Allreduce(&recvTime, &min, 1, MPI_DOUBLE, MPI_MIN, MPI_COMM_WORLD);
	MPI_Allreduce(&recvTime, &avg, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);
	MPI_Allreduce(&recvTime, &max, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);

	avg /= numprocs;

	if(myrank == 0) {
	  time[0] += min;
	  time[1] += avg;
	  time[2] += max;
	}
      }
      if (myrank == 0) {
	FILE *outf = fopen(name, "a");
	fprintf(outf, "%d %g %g %g\n", msg_size, time[0]/10, time[1]/10, time[2]/10);
	fflush(NULL);
	fclose(outf);
	time[0] = time[1] = time[2] = 0.0;
      }
    }
  }
 
  if(myrank == 0)
    printf("Program Complete\n");

  MPI_Finalize();
  return 0;
}


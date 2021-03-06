/** \file wicon3.C
 *  Author: Abhinav S Bhatele
 *  Date Created: May 17th, 2009
 *  E-mail: bhatele@illinois.edu
 *
 *  WICON3 Benchmark:
 *  --------------------------------------------------------------------------
 *  This benchmark tries to captures the intereference between jobs when the
 *  batch scheduler on a supercomputer does not allocate partitions which are
 *  in a nice geometric shape respecting the topology of the machine.
 *  
 *  In the secong mode it creates contention on a specific line by adding more
 *  and more pairs around a given link.
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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

void check_map(int size, int *map) {
  int i;
  for(i = 0; i < size; i++) {
    if(map[i] != -1)
      if(i != map[map[i]]) {
	printf("map[%d]=%d map[%d]=%d\n", i, map[map[i]]);
	abort();
      }
  }
}

void build_process_map(int size, int *map, int dist, int numRG, int *mapRG)
{
  TopoManager tmgr;
  int pe1, pe2, x, y, z, t;
  int dimNX, dimNY, dimNZ, dimNT;

  dimNX = tmgr.getDimNX();
  dimNY = tmgr.getDimNY();
  dimNZ = tmgr.getDimNZ();
  dimNT = tmgr.getDimNT();

  int count = 0;

#if CREATE_JOBS
  for(int i=0; i<size; i++)
    map[i] = -1;

  // assumes a cubic partition such as 8 x 8 x 8
  // inner brick is always used
  for(int i=0; i<dimNX; i++)
    for(int j=1; j<dimNY-1; j++)
      for(int k=1; k<dimNZ-1; k++)
	for(int l=0; l<dimNT; l++) {
          if(k == 2 || k == dimNZ-3) {
	    pe1 = tmgr.coordinatesToRank(i, j, k, l);
            if(k == 2)
              pe2 = tmgr.coordinatesToRank(i, j, dimNZ-3, l);
            else
              pe2 = tmgr.coordinatesToRank(i, j, 2, l);
	    map[pe1] = pe2;
            mapRG[count++] = pe1;
	    printf("%d ", pe1);
	  }
	}

  printf("\n");
  if(dist == 1) {
    // outer brick is used only when dist == 1
    for(int i=0; i<dimNX; i++)
      for(int j=0; j<dimNY; j++)
	for(int k=0; k<dimNZ; k++)
	  for(int l=0; l<dimNT; l++) {
	    if(j == 0 || j == dimNY-1 || k == 0 || k == dimNZ-1) {
	      pe1 = tmgr.coordinatesToRank(i, j, k, l);
	      pe2 = tmgr.coordinatesToRank(i, k, j, l);
	      if(j == 0 && k == 0)
		pe2 = tmgr.coordinatesToRank(i, dimNY-1, dimNZ-1, l);
	      else if(j == dimNY-1 && k == dimNZ-1)
		pe2 = tmgr.coordinatesToRank(i, 0, 0, l);
	      map[pe1] = pe2;
	    }
	  }
  }
#else
  for(int i=0; i<dimNX; i++)
    for(int j=0; j<dimNY; j++)
      for(int k=0; k<dimNZ; k++)
	for(int l=0; l<dimNT; l++) {
	  pe1 = tmgr.coordinatesToRank(i, j, k, l);
	  if( abs(dimNZ - 1 - 2*k) <= (2*dist+1) ) {
	    pe2 = tmgr.coordinatesToRank(i, j, (dimNZ-1-k), l);
	    map[pe1] = pe2;

	    if(i==0 && j==0 && l==0) {
	      printf("Hops %d [%d] [%d]\n", 2*dist+1, pe1, pe2);
	    }

	    if(k == dimNZ/2-1 || k == dimNZ/2)
	      mapRG[count++] = pe1;
	  } else
	    map[pe1] = -1;
	}
#endif
  printf("Barrier Process %d %d\n", count, numRG);
  check_map(size, map);
}

int main(int argc, char *argv[]) {
  int numprocs, myrank, grank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

  MPI_Group orig_group, new_group; 
  MPI_Comm new_comm; 

  /* Extract the original group handle */ 
  MPI_Comm_group(MPI_COMM_WORLD, &orig_group); 

  double sendTime, recvTime, min, avg, max;
  double time[3] = {0.0, 0.0, 0.0};
  int msg_size;
  MPI_Status mstat;
  int i=0, pe, trial, hops;
  char name[30];

  char *send_buf = (char *)malloc(MAX_MSG_SIZE);
  char *recv_buf = (char *)malloc(MAX_MSG_SIZE);

  for(i = 0; i < MAX_MSG_SIZE; i++) {
    recv_buf[i] = send_buf[i] = (char) (i & 0xff);
  }

  // allocate the routing map.
  int *map = (int *) malloc(sizeof(int) * numprocs);
  TopoManager *tmgr;
  int dimNZ, numRG, x, y, z, t, bcastSend[3], bcastRecv[3];

  if(myrank == 0) {
    tmgr = new TopoManager();
#if CREATE_JOBS
    numRG = tmgr->getDimNX() * (tmgr->getDimNY() - 2) * 2 * tmgr->getDimNT();
#else
    numRG = tmgr->getDimNX() * tmgr->getDimNY() * 2 * tmgr->getDimNT();
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

#if CREATE_JOBS
  for (hops=0; hops < 2; hops++) {
#else
  for (hops=0; hops < dimNZ/2; hops++) {
#endif
    int *mapRG = (int *) malloc(sizeof(int) * numRG);
    if (myrank == 0) {
      // Rank 0 makes up a routing map.
      build_process_map(numprocs, map, hops, numRG, mapRG);
    }

    // Broadcast the routing map.
    MPI_Bcast(map, numprocs, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(mapRG, numRG, MPI_INT, 0, MPI_COMM_WORLD);

    MPI_Group_incl(orig_group, numRG, mapRG, &new_group);
    MPI_Comm_create(MPI_COMM_WORLD, new_group, &new_comm);
    MPI_Group_rank(new_group, &grank);
    
#if CREATE_JOBS
    sprintf(name, "xt4_job_%d_%d.dat", numprocs, hops);
#else
    sprintf(name, "xt4_line_%d_%d.dat", numprocs, hops);
#endif
   
    for (msg_size=MIN_MSG_SIZE; msg_size<=MAX_MSG_SIZE; msg_size=(msg_size<<1)) {
      for (trial=0; trial<10; trial++) {

	pe = map[myrank];
	if(pe != -1) {
          if(grank != MPI_UNDEFINED) MPI_Barrier(new_comm);

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

	    if(grank != MPI_UNDEFINED) MPI_Barrier(new_comm);
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

	    if(grank != MPI_UNDEFINED) MPI_Barrier(new_comm);
	  }

	  if(grank != MPI_UNDEFINED) {
  	    MPI_Allreduce(&recvTime, &min, 1, MPI_DOUBLE, MPI_MIN, new_comm);
  	    MPI_Allreduce(&recvTime, &avg, 1, MPI_DOUBLE, MPI_SUM, new_comm);
	    MPI_Allreduce(&recvTime, &max, 1, MPI_DOUBLE, MPI_MAX, new_comm);
          }

	  avg /= numRG;

	} // end if map[pe] != -1
	if(grank == 0) {
	  time[0] += min;
	  time[1] += avg;
	  time[2] += max;
	}
      } // end for loop of trials
      if (grank == 0) {
	FILE *outf = fopen(name, "a");
	fprintf(outf, "%d %g %g %g\n", msg_size, time[0]/10, time[1]/10, time[2]/10);
	fflush(NULL);
	fclose(outf);
	time[0] = time[1] = time[2] = 0.0;
      }
    } // end for loop of msgs
    free(mapRG);
  } // end for loop of hops

  if(grank == 0)
    printf("Program Complete\n");

  MPI_Finalize();
  return 0;
}


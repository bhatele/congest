/*****************************************************************************
 * $Source$
 * $Author$
 * $Date$
 * $Revision$
 *****************************************************************************/

/** \file contention_stencil.C
 *  Author: Abhinav S Bhatele
 *  Date Created: September 24th, 2009
 *  E-mail: bhatele@illinois.edu
 *
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

#define wrap_x(a)	(((a)+dimNX)%dimNX)
#define wrap_y(a)	(((a)+dimNY)%dimNY)
#define wrap_z(a)	(((a)+dimNZ)%dimNZ)

int main(int argc, char *argv[]) {
  int numprocs, myrank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);
  MPI_Request mreq[6], mreq2[2], mreq3;
  MPI_Status mstat[6], mstat2[2], mstat3;

  double sendTime, recvTime, time = 0.0;
  int msg_size;
  int i=0, j=0, trial, hops;
  char name[50];
  char *send_buf[9], *recv_buf[9];

  for(i=0; i<9; i++) {
    send_buf[i] = (char *)memalign(64 * 1024, MAX_MSG_SIZE);
    recv_buf[i] = (char *)memalign(64 * 1024, MAX_MSG_SIZE);
    for(j=0; j<MAX_MSG_SIZE; j++)
      recv_buf[i][j] = send_buf[i][j] = (char) (j & 0xff);
  }

  TopoManager tmgr;
  int dimNX = tmgr.getDimNX();
  int dimNY = tmgr.getDimNY();
  int dimNZ = tmgr.getDimNZ();
  int dimNT = tmgr.getDimNT();

  if (myrank == 0) {
    printf("Torus Dimensions %d %d %d [%d]\n", dimNX, dimNY, dimNZ, dimNT);
  }

  // set up of partners
  int nbrs[6];
  int x, y, z, t;
  tmgr.rankToCoordinates(myrank, x, y, z, t);
  nbrs[0] = tmgr.coordinatesToRank(wrap_x(x-1), y, z, t);
  nbrs[1] = tmgr.coordinatesToRank(wrap_x(x+1), y, z, t);
  nbrs[2] = tmgr.coordinatesToRank(x, wrap_y(y-1), z, t);
  nbrs[3] = tmgr.coordinatesToRank(x, wrap_y(y+1), z, t);
  nbrs[4] = tmgr.coordinatesToRank(x, y, wrap_z(z-1), t);
  nbrs[5] = tmgr.coordinatesToRank(x, y, wrap_z(z+1), t);

  int hops9P = tmgr.coordinatesToRank(wrap_x(x+3), wrap_y(y+3), wrap_z(z+3), t);
  int hops9N = tmgr.coordinatesToRank(wrap_x(x-3), wrap_y(y-3), wrap_z(z-3), t);
  int hops6P = tmgr.coordinatesToRank(wrap_x(x+2), wrap_y(y+2), wrap_z(z+2), t);
  int hops6N = tmgr.coordinatesToRank(wrap_x(x-2), wrap_y(y-2), wrap_z(z-2), t);
  int hops3P = tmgr.coordinatesToRank(wrap_x(x+1), wrap_y(y+1), wrap_z(z+1), t);
  int hops3N = tmgr.coordinatesToRank(wrap_x(x-1), wrap_y(y-1), wrap_z(z-1), t);

  for (hops=1; hops <= 3; hops++) {
    sprintf(name, "bgp_dilation_%d_%d.dat", numprocs, hops);

#if USE_HPM
    HPM_Init();
    HPM_Start("Sending");
#endif
    for (msg_size=MIN_MSG_SIZE; msg_size<=MAX_MSG_SIZE; msg_size=(msg_size<<1)) {
      for (trial=0; trial<11; trial++) {

	if(myrank == 0 && trial > 0) sendTime = MPI_Wtime();
	MPI_Barrier(MPI_COMM_WORLD);

	for(i=0; i<NUM_MSGS; i++) {
	  // set up the recvs
	  for(j=0; j<6; j++)
	    MPI_Irecv(recv_buf[j], msg_size, MPI_CHAR, nbrs[j], 1, MPI_COMM_WORLD, &mreq[j]);
	  if(hops == 2) {
	    MPI_Irecv(recv_buf[6], msg_size, MPI_CHAR, hops6N, 2, MPI_COMM_WORLD, &mreq2[0]);
	    MPI_Irecv(recv_buf[7], msg_size, MPI_CHAR, hops3P, 2, MPI_COMM_WORLD, &mreq2[1]);
	  }
	  if(hops == 3)
	    MPI_Irecv(recv_buf[8], msg_size, MPI_CHAR, hops9N, 3, MPI_COMM_WORLD, &mreq3);

	  // set up the sends
	  for(j=0; j<6; j++)
	    MPI_Send(send_buf[j], msg_size, MPI_CHAR, nbrs[j], 1, MPI_COMM_WORLD);
	  if(hops == 2) {
	    MPI_Send(send_buf[6], msg_size, MPI_CHAR, hops6P, 2, MPI_COMM_WORLD);
	    MPI_Send(send_buf[7], msg_size, MPI_CHAR, hops3N, 2, MPI_COMM_WORLD);
	  }
	  if(hops == 3)
	    MPI_Send(send_buf[8], msg_size, MPI_CHAR, hops9P, 3, MPI_COMM_WORLD);

	  // wait
	  MPI_Waitall(6, mreq, mstat);
	  if(hops == 2)
	    MPI_Waitall(2, mreq2, mstat2);
	  if(hops == 3)
	    MPI_Wait(&mreq3, &mstat3);
	}

	MPI_Barrier(MPI_COMM_WORLD);
	if(myrank == 0 && trial > 0) recvTime = (MPI_Wtime() - sendTime) / NUM_MSGS;

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


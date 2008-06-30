/*****************************************************************************
 * $Source$
 * $Author$
 * $Date$
 * $Revision$
 *****************************************************************************/

/** \file latency_old.c
 *  Author: Abhinav S Bhatele
 *  Date Created: May 11th, 2008
 *  E-mail: bhatele2@uiuc.edu
 *
 *  This benchmark uses one particular processor to send messages
 *  to all other processors in the allocated partition. Message latency
 *  for the ping-pong for each pair is stored as a function of the 
 *  message size.
 */

#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>

// Minimum message size (bytes)
#define MIN_MSG_SIZE 4

// Maximum message size (bytes)
#define MAX_MSG_SIZE (1024 * 1024)

#define UNIQ_PE 11

int main(int argc, char *argv[]) {
  int numprocs, myrank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

  double sendTime, recvTime;
  double time[numprocs];
  int msg_size, num_msgs;
  MPI_Status mstat;
  int i=0, j=0;
  char name[30];
  sprintf(name, "bgp_COmode_latency_%d.dat", numprocs);

  char *send_buf = (char *)memalign(64 * 1024, MAX_MSG_SIZE);
  char *recv_buf = (char *)memalign(64 * 1024, MAX_MSG_SIZE);

  for(i = 0; i < MAX_MSG_SIZE; i++) {
    recv_buf[i] = send_buf[i] = (char) (i & 0xff);
  }

  for(msg_size=MAX_MSG_SIZE; msg_size>=MIN_MSG_SIZE; msg_size=(msg_size>>1)) {
    if(msg_size < 2048)
      num_msgs = 100;
    else
      num_msgs = 20;

    if(myrank == UNIQ_PE) {
      for(i=0; i<numprocs; i++) {
	if(i != UNIQ_PE) {
	  // warm up
	  for(j=0; j<10; j++)
	    MPI_Sendrecv(send_buf, msg_size, MPI_CHAR, i, 1,
		       recv_buf, msg_size, MPI_CHAR, i, 1, MPI_COMM_WORLD, &mstat);
	  sendTime = MPI_Wtime();
	  for(j=0; j<num_msgs; j++)
	    MPI_Sendrecv(send_buf, msg_size, MPI_CHAR, i, 1,
			 recv_buf, msg_size, MPI_CHAR, i, 1, MPI_COMM_WORLD, &mstat);
	  recvTime = MPI_Wtime();
	  time[i] = (recvTime - sendTime) / num_msgs;
	}
      }
    }
    else {
      // warm up
      for(i=0; i<10; i++)
	MPI_Sendrecv(send_buf, msg_size, MPI_CHAR, UNIQ_PE, 1, 
		   recv_buf, msg_size, MPI_CHAR, UNIQ_PE, 1, MPI_COMM_WORLD, &mstat);
      for(i=0; i<num_msgs; i++)
	MPI_Sendrecv(send_buf, msg_size, MPI_CHAR, UNIQ_PE, 1, 
		     recv_buf, msg_size, MPI_CHAR, UNIQ_PE, 1, MPI_COMM_WORLD, &mstat);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    if(myrank == UNIQ_PE) {
      FILE *outf = fopen(name, "a");
      for(i=0; i<numprocs; i++) {
	if(i != 128) {
	  fprintf(outf, "%d %g\n", msg_size, time[i]);
	  fflush(outf);
	}
      }
      fclose(outf);
    }
  }

  if(myrank == UNIQ_PE)
    printf("Program Complete\n");

  MPI_Finalize();
  return 0;
}


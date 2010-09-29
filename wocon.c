/** \file wocon.c
 *  Author: Abhinav S Bhatele
 *  Date Created: May 11th, 2008
 *  E-mail: bhatele2@uiuc.edu
 *
 *  WOCON Benchmark:
 *  --------------------------------------------------------------------------
 *  This benchmark uses one particular processor to send messages to all other
 *  processors in the allocated partition. Message latency for the ping-pong
 *  for each pair is stored as a function of the message size.
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

  double sendTime, recvTime, min, max, avg, cent;
  double time[numprocs];
  int msg_size, num_msgs;
  int i=0, j=0;
  char name1[30], name2[30];
  sprintf(name1, "xt4_latency_%d.dat", numprocs);
  sprintf(name2, "xt4_latMAM_%d.dat", numprocs);

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
	  MPI_Send(send_buf, msg_size, MPI_CHAR, i, 1, MPI_COMM_WORLD);
	  MPI_Recv(recv_buf, msg_size, MPI_CHAR, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

	  sendTime = MPI_Wtime();
	  for(j=0; j<num_msgs; j++) {
	    MPI_Send(send_buf, msg_size, MPI_CHAR, i, 1, MPI_COMM_WORLD);
	    MPI_Recv(recv_buf, msg_size, MPI_CHAR, i, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	  }
	  recvTime = MPI_Wtime();
	  time[i] = (recvTime - sendTime) / (num_msgs * 2);
	}
      }
    }
    else {
      // warm up
      MPI_Recv(recv_buf, msg_size, MPI_CHAR, UNIQ_PE, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
      MPI_Send(send_buf, msg_size, MPI_CHAR, UNIQ_PE, 1, MPI_COMM_WORLD);

      for(i=0; i<num_msgs; i++) {
	MPI_Recv(recv_buf, msg_size, MPI_CHAR, UNIQ_PE, 1, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	MPI_Send(send_buf, msg_size, MPI_CHAR, UNIQ_PE, 1, MPI_COMM_WORLD);
      }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    if(myrank == UNIQ_PE) {
      FILE *outf = fopen(name1, "a");
      min = 100000000.0;
      max = avg = 0.0;
      for(i=0; i<numprocs; i++) {
	if(i != UNIQ_PE) {
	  fprintf(outf, "%d %g\n", msg_size, time[i]);
	  if(time[i] < min) min = time[i];
	  if(time[i] > max) max = time[i];
	  avg += time[i];
	}
      }
      fflush(outf);
      fclose(outf);

      avg /= (numprocs-1);
      cent = ((max - min) * 100) / min;
      outf = fopen(name2, "a");
      fprintf(outf, "%d %g %g %g %g\n", msg_size, min, avg, max, cent);
      fflush(outf);
      fclose(outf);   
    }
  }

  if(myrank == UNIQ_PE)
    printf("Program Complete\n");

  MPI_Finalize();
  return 0;
}


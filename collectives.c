/** \file latency.c
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
#include <math.h>

// Minimum message size (bytes)
//#define MIN_MSG_SIZE 4

// Maximum message size (bytes)
#define MAX_MSG_SIZE (1024 * 128)

#define UNIQ_PE 0

int main(int argc, char *argv[]) {
  int numprocs, myrank;
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &numprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &myrank);

  double sendTime, recvTime, min, max, avg, cent;
  double time[numprocs*100];
  double mytime[numprocs*100];
  int msg_size, num_msgs;
  int i=0, j=0;
  char name1[30], name2[30];
  sprintf(name1, "alltoall_%d.dat", numprocs);
  sprintf(name2, "ataMAM_%d.dat", numprocs);

  char *send_buf = (char *)memalign(64 * 1024, MAX_MSG_SIZE*numprocs);
  char *recv_buf = (char *)memalign(64 * 1024, MAX_MSG_SIZE*numprocs);
  int cnts[numprocs];
  int displs[numprocs];

  for (i = 0; i < numprocs*100; i++){
    time[i] = 0;
    mytime[i] = 0;
  }

  for(i = 0; i < MAX_MSG_SIZE*numprocs; i++) {
    recv_buf[i] = send_buf[i] = (char) (i & 0xff);
  }

  for(msg_size=MAX_MSG_SIZE; msg_size>=numprocs; msg_size=(msg_size>>1)) {
    if(msg_size < 2048)
      num_msgs = 100;
    else
      num_msgs = 20;
    for (i = 0; i < numprocs; i++){
      cnts[i] = msg_size;
      displs[i] = i*msg_size;
    }

    for(j=0; j<num_msgs; j++) {
      sendTime = MPI_Wtime();
      MPI_Alltoallv(send_buf, cnts, displs, MPI_CHAR, recv_buf, cnts, displs, MPI_CHAR, MPI_COMM_WORLD);
      recvTime = MPI_Wtime();
      mytime[num_msgs*myrank + j] = recvTime - sendTime;
      MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Reduce(mytime, time, num_msgs*numprocs, MPI_DOUBLE, MPI_SUM, UNIQ_PE, MPI_COMM_WORLD);
  
    MPI_Barrier(MPI_COMM_WORLD);
  
    if(myrank == UNIQ_PE) {
      FILE *outf = fopen(name1, "a");
      min = 100000000.0;
      max = avg = 0.0;
      for(i=0; i<numprocs; i++) {
	for (j = 0; j <num_msgs; j++){
	  fprintf(outf, "%d %d %g\n", msg_size, i, time[i*num_msgs + j]);
	  if(time[i*num_msgs + j] < min) min = time[i*num_msgs + j];
	  if(time[i*num_msgs + j] > max) max = time[i*num_msgs + j];
	  avg += time[i*num_msgs + j];
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


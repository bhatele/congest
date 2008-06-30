/** \file contention.c 
 *  Author: Abhinav S Bhatele
 *  Date Created: May 11th, 2008
 *  E-mail: bhatele2@uiuc.edu
 *
 *  This benchmark captures the effect of distance on the message
 *  latencies in the presence of contention. Every processor sends
 *  a message to some other random processor. The number of messages
 *  being sent can be varied and the random processor is changed in
 *  every trial. These latencies for different message sizes are
 *  compared to the latencies for messages sent to the nearest neighbor
 */


#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <malloc.h>

// Minimum message size (bytes)
#define MIN_MSG_SIZE 4

// Maximum message size (bytes)
#define MAX_MSG_SIZE (1024 * 1024)

#define NUM_MSGS 10

static unsigned long next = 1;

int myrand(int numpes) {
  next = next * 1103515245 + 12345;
  return((unsigned)(next/65536) % numpes);
}

/* creating a random map code taken from Matt Reilly's benchmark
 *
 */
static int random_ready = 0; 
void init_random()
{
  srand48(33550336); 
  random_ready = 1; 
}
  
double get_random_double()
{
  if(random_ready == 0) init_random();
  return drand48(); 
}

int get_random_int(int max)
{
  double dr;
  int res = max;
  while (res >= max) {
    dr = get_random_double() * ((double) max);
    res = (int) floor(dr); 
  }
  return res; 
}

void dump_map(int size, int * map)
{
  int i;
  for(i = 0; i < size; i++) {
    printf("map[%03d] = %03d\n", i, map[i]); 
  }

  fflush(stdout); 
}

#define SHUFFLE_ITERATIONS 4

void build_random_map(int init, int size, int * map)
{
#if RANDOMNESS
  int i, j, k, p, q;
  
  if(size & 1) {
    fprintf(stderr, "Random maps must be even length\n"); 
    exit(-1); 
  }
  
  if(init) {
    // build an initial map that maps all entries K
    // to K + (K mod 2)
    for(i = 0; i < size; i++) {
      map[i] = i ^ 1; 
    }
  }

  // Now do random pair swaps
  for(j = 0; j < SHUFFLE_ITERATIONS; j++) {
    for(i = 0; i < size; i++) {
      // for each mapping entry, pick someone to swap with.
      k = i;
      while ((k == i) || (k == map[i])) {
	k = get_random_int(size);
      }

      // Now swap all the appropriate entries. 
      // The map right now looks like this: 
      // map[i] = p
      // map[p] = i
      // map[k] = q
      // map[q] = k
      //
      p = map[i];
      q = map[k];

      // We want the map to be
      // map[i] = q
      // map[p] = k
      // map[k] = p
      // map[q] = i
      map[i] = q;
      map[p] = k;
      map[k] = p;
      map[q] = i; 

    }
  }
#else
  int i, j;
  for(i=0; i<size; i=i+8) {
    for(j=0; j<4; j++) {
      map[i + j] = i + j + 4;
      map[i + j + 4] = i + j;
    }
  }
#endif

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
  int i=0, pe, trial;
  char name[30];

  char *send_buf = (char *)memalign(64 * 1024, MAX_MSG_SIZE);
  char *recv_buf = (char *)memalign(64 * 1024, MAX_MSG_SIZE);

  for(i = 0; i < MAX_MSG_SIZE; i++) {
    recv_buf[i] = send_buf[i] = (char) (i & 0xff);
  }

  // allocate the routing map.
  int *map = (int *) malloc(sizeof(int) * numprocs);

#if RANDOMNESS    
  sprintf(name, "bgp_COmode_rnd_%d.dat", numprocs);
#else
  // Rank 0 makes up a routing map.
  if(myrank == 0) {
    build_random_map(1, numprocs, map);
  }
  // Broadcast the routing map.
  MPI_Bcast(map, numprocs, MPI_INT, 0, MPI_COMM_WORLD);

  sprintf(name, "bgp_COmode_nn_%d.dat", numprocs);
#endif

  for(msg_size=MAX_MSG_SIZE; msg_size>=MIN_MSG_SIZE; msg_size=(msg_size>>1)) {
    for(trial=0; trial<10; trial++) {

#if RANDOMNESS    
      // Rank 0 makes up a routing map.
      if(myrank == 0) {
	build_random_map(trial == 0, numprocs, map);
      }

      // Broadcast the routing map.
      MPI_Bcast(map, numprocs, MPI_INT, 0, MPI_COMM_WORLD);
#endif

      pe = map[myrank];

      MPI_Barrier(MPI_COMM_WORLD);

      if(myrank < pe) {
	// warmup
	for(i=0; i<10; i++) {
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
	for(i=0; i<10; i++) {
	  MPI_Send(send_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD);
          MPI_Recv(recv_buf, msg_size, MPI_CHAR, pe, 1, MPI_COMM_WORLD, &mstat);
	}

	MPI_Barrier(MPI_COMM_WORLD);
      } else {
	// warmup
        for(i=0; i<10; i++) {
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
        for(i=0; i<10; i++) {
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
    if(myrank == 0) {
      FILE *outf = fopen(name, "a");
      fprintf(outf, "%d %g %g %g\n", msg_size, time[0]/10, time[1]/10, time[2]/10);
      fclose(outf);
      time[0] = time[1] = time[2] = 0.0;
    }
  }
  
  if(myrank == 0)
    printf("Program Complete\n");

  MPI_Finalize();
  return 0;
}


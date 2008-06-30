all:	contention_kmsgs.c
	mpicc -O3 -DRANDOMNESS=0 -DNUM_MSGS=1 -c contention_kmsgs.c
	mpicc -o nnd1 contention_kmsgs.o -lm
	mpicc -O3 -DRANDOMNESS=1 -DNUM_MSGS=1 -c contention_kmsgs.c
	mpicc -o rndd1 contention_kmsgs.o -lm
	mpicc -O3 -DRANDOMNESS=0 -DNUM_MSGS=5 -c contention_kmsgs.c
	mpicc -o nnd5 contention_kmsgs.o -lm
	mpicc -O3 -DRANDOMNESS=1 -DNUM_MSGS=5 -c contention_kmsgs.c
	mpicc -o rndd5 contention_kmsgs.o -lm
	mpicc -O3 -DRANDOMNESS=0 -DNUM_MSGS=10 -c contention_kmsgs.c
	mpicc -o nnd10 contention_kmsgs.o -lm
	mpicc -O3 -DRANDOMNESS=1 -DNUM_MSGS=10 -c contention_kmsgs.c
	mpicc -o rndd10 contention_kmsgs.o -lm
	mpicc -O3 -DRANDOMNESS=0 -DNUM_MSGS=20 -c contention_kmsgs.c
	mpicc -o nnd20 contention_kmsgs.o -lm
	mpicc -O3 -DRANDOMNESS=1 -DNUM_MSGS=20 -c contention_kmsgs.c
	mpicc -o rndd20 contention_kmsgs.o -lm

#include <ilib.h>
#include <stdio.h>
#include <tmc/cmem.h>

#define N 1000
#define CORES 10
#define ROOT 0

#define MSG_HANDLE 0

int mem(void);
int mpi(void);
int master(int size);
int slave(int rank);

typedef struct {
	ilibMutex mutex;
	int i;
} locked_int;

int main() {
	printf("in main\n");
	ilib_init();

	if(ilib_proc_go_parallel(CORES, NULL) != ILIB_SUCCESS) {
		ilib_die("Failed to go to parallel.");
	}

	int rank = ilib_group_rank(ILIB_GROUP_SIBLINGS);
	int size = ilib_group_size(ILIB_GROUP_SIBLINGS);

	if(rank == ROOT) {
		master(size);
	}
	slave(rank);

	ilib_finish();
	return(0);
}

int master(int size) {
	printf("PRINT FROM ROOT\n");
	int i;
	int j;
	for(i = 0; i < size; i++) {
		j = i * 100;
		if(i != ROOT) {
			printf("sending to %i\n", i);
			ilib_msg_send(ILIB_GROUP_SIBLINGS, i, MSG_HANDLE, &j, sizeof(j));
		}
	}
	return(0);
}

int slave(int rank) {
	if(rank != ROOT) {
		printf("print from %i, receiving...\n", rank);
	}
	int a;
	ilibStatus status;
	if(rank != ROOT) {
		ilib_msg_receive(ILIB_GROUP_SIBLINGS, ROOT, MSG_HANDLE, &a, sizeof(a), &status);
		printf("a is %i\n", a);
	}
	return(0);
}

/**
 * array_sum() with shared memory communication.
 */
int mem() {
	int i;
	locked_int* r;
	int a[N];
	int start, stop;
	int rank, size;

	ilib_init();

	if(ilib_proc_go_parallel(CORES, NULL) != ILIB_SUCCESS) {
		ilib_die("Failed to go parallel.");
	}

	rank = ilib_group_rank(ILIB_GROUP_SIBLINGS);
	size = ilib_group_size(ILIB_GROUP_SIBLINGS);

	if(rank == 0) {
		// initialize the counter
		r = (locked_int *) tmc_cmem_malloc(sizeof(locked_int));
		r->i = 0;
		ilib_mutex_init(&r->mutex);

		// send lock to all children
		for(i = 1; i< size; i++) {
			ilib_msg_send(ILIB_GROUP_SIBLINGS, i, 0, &r, sizeof(locked_int));
		}
	} else {
		ilibStatus status;
		ilib_msg_receive(ILIB_GROUP_SIBLINGS, 0, 0, &r, sizeof(locked_int), &status);
	}

	start = (rank * N) / size;
	stop = ((rank + 1) * N) / size;

	for(i = start; i < stop; i++) {
		a[i] = i;
	}

	// actual processing, with shared memory lock on r-access
	for(i = start; i < stop; i++) {
		ilib_mutex_lock(&r->mutex);
		r->i += a[i];
		ilib_mutex_unlock(&r->mutex);
	}

	// block thread 0 from printing out until all threads have finished
	ilib_msg_barrier(ILIB_GROUP_SIBLINGS);

	if(rank == 0) {
		ilib_mutex_lock(&r->mutex);
		printf("The sum of the %i integers is %i\n", N, r->i);
		ilib_mutex_unlock(&r->mutex);
	}

	ilib_finish();
	return(0);
}



/**
 * Parallel implementation of array_sum(), with MPI communication.
 */
int mpi() {
	printf("Program started.\n");

	// declaring variables, allotting memory
	int i;
	int r = 0;
	int a[N];

	int start, stop;
	int rank, size;

	// intialize parallel
	ilib_init();

	if(ilib_proc_go_parallel(CORES, NULL) != ILIB_SUCCESS) {
		ilib_die("Failed to go to parallel.");
	}

	// rank is different for each parallel-ized task
	rank = ilib_group_rank(ILIB_GROUP_SIBLINGS);
	size = ilib_group_size(ILIB_GROUP_SIBLINGS);

	// ...which implies that the start and stop conditions will also be different
	start = (rank * N) / size;
	stop = ((rank + 1) * N) / size;

	// computing data
	for(i = start; i < stop; i++) {
		a[i] = i;
	}
	for(i = start; i < stop; i++) {
		r += a[i];
	}

	// merging data via MPI
	if(rank == 0) {
		int temp;
		ilibStatus status;

		// wait for messages from each status
		// NOTE: this is blocking!
		for(i = 1; i < size; i++) {
			printf("Receiving results from thread %i\n", i);
			ilib_msg_receive(ILIB_GROUP_SIBLINGS, i, 0, &temp, sizeof(temp), &status);
			printf("Received results from thread %i\n", i);
			r += temp;
		}
		printf("The sum of the %i integers is %i\n", N, r);
	} else {
		printf("About to send results from thread %i\n", rank);

		// this will block thread 1 from returning - which will block receiving thread, and thus block threads i > 1
		// if(rank == 1) {
		// 	while(1);
		// }

		// send message to the main thread
		ilib_msg_send(ILIB_GROUP_SIBLINGS, 0, 0, &r, sizeof(r));
		printf("Sent results from thread %i\n", rank);
	}

	ilib_finish();
	return(0);
}

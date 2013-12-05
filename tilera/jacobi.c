#include <errno.h>
#include <ilib.h>
#include <math.h>
#include <stdio.h>
#include <tmc/cmem.h>

#include "config.h"
#include "data.h"
#include "file.h"
#include "jacobi.h"

/* Minimize the use of message passing in the mode == DMA case. */
address * create_address_instance(ilib_mutex_t *error_lock, double *global_error, double **a, double *b, double *x, double *xt) {
	address *instance = tmc_cmem_malloc(sizeof(address));
	instance->error_lock = error_lock;
	instance->global_error = global_error;
	instance->a = a;
	instance->b = b;
	instance->x = x;
	instance->xt = xt;
	return(instance);	
}

int main(int argc, char *argv[]) {
	char *folder = (char *) calloc(MAX_BUF, sizeof(char));
	int cores;
	int mode;
	if(argc == 4) {
		sscanf(argv[1], "%s", folder);
		sscanf(argv[2], "%i", &cores);
		sscanf(argv[3], "%i", &mode);
	} else {
		char *cmd = calloc(MAX_BUF, sizeof(char));
		sscanf(argv[0], "/%s", cmd);
		fprintf(stderr, "usage: %s [ folder ] [ cores ] [ mode ]\n", cmd);
		return(0);
	}

	char *pipe_a = (char *) calloc(MAX_BUF, sizeof(char));
	char *pipe_b = (char *) calloc(MAX_BUF, sizeof(char));
	char *pipe_x = (char *) calloc(MAX_BUF, sizeof(char));

	sprintf(pipe_a, "pipe/%s/a.txt", folder);
	sprintf(pipe_b, "pipe/%s/b.txt", folder);
	sprintf(pipe_x, "pipe/%s/x_%s_%i.txt", folder, (mode == MPI) ? "mpi" : "dma", cores);

	ilib_init();

	if(ilib_proc_go_parallel(cores, argv) != ILIB_SUCCESS) {
		ilib_die("Fatal error - cannot proceed.");
	}

	int tid = ilib_group_rank(ILIB_GROUP_SIBLINGS);
	int lim = ilib_group_size(ILIB_GROUP_SIBLINGS);

	data *thread_data = data_init();
	thread_data->tid = tid;
	thread_data->lim = lim;

	if(thread_data->tid == ROOT) {
		int dim = DIM;
		double *b = get_b(pipe_b, &dim);
		double **a = get_a(pipe_a, dim);
		double *x = (double *) calloc(dim, sizeof(double));
	
		ilib_status_t status;
		ilib_msg_broadcast(ILIB_GROUP_SIBLINGS, ROOT, &dim, sizeof(dim), &status);

		data_dim(thread_data, dim, mode);
		mastr_initialize(thread_data, lim, a, b, dim, mode);

		fprintf(stderr, "\nfolder %s, dim %i, cores %i, protocol %s\n", folder, dim, cores, (mode == MPI) ? "mpi" : "dma");
	} else {
		int dim;
		ilib_status_t status;
		ilib_msg_broadcast(ILIB_GROUP_SIBLINGS, ROOT, &dim, sizeof(dim), &status);

		data_dim(thread_data, dim, mode);
		slave_initialize(thread_data, mode);
	}

	int steps;
	int result = iterate(thread_data, &steps, mode);

	// return data
	if(thread_data->tid == ROOT) {
		fprintf(stderr, "finished iterating in %i / %i steps\n", steps + 1, MAX_ITERATIONS);
		set_x(thread_data->thread_xt, thread_data->dim, pipe_x);
		if(result == FAILURE) {
			fprintf(stderr, "failed to converge\n");
		}
	}

	ilib_finish();
	return(result);
}

int iterate(data *thread_data, int *iterations, int mode) {
	for(int step = 0; step < MAX_ITERATIONS; step++) {
		if(mode == MPI) {
			thread_data->thread_error = 0.0;
		} else {
			if(thread_data->tid == ROOT) {
				*(thread_data->global_error) = 0.0;
			}

			// ensure global_error is reset before proceeding
			ilib_msg_barrier(ILIB_GROUP_SIBLINGS);
		}
		int i;
		for(int row = 0; row < thread_data->thread_rows; row++) {		// i
			i = thread_data->thread_offset + row;				// global_i
			double lower = 0.0;
			double upper = 0.0;
			for(int j = 0; j < i; j++) {
				lower += thread_data->thread_a[row][j] * thread_data->thread_xt[j];
			}
			for(int j = i + 1; j < thread_data->dim; j++) {
				upper += thread_data->thread_a[row][j] * thread_data->thread_xt[j];
			}

			thread_data->thread_x[row] = (thread_data->thread_b[row] - (lower + upper)) / thread_data->thread_a[row][i];

			// check if estimates converge
			double x = thread_data->thread_x[row] - thread_data->thread_xt[i];
			if(mode == MPI) {
				thread_data->thread_error += x * x;
			} else {
				ilib_mutex_lock(thread_data->error_lock);
				*(thread_data->global_error) += x * x;
				ilib_mutex_unlock(thread_data->error_lock);
			}
		}

		// sync x guesses, and get global error
		double error;
		double error_t;
		if(mode == MPI) {
			error = thread_data->thread_error;
		} else {
			error = *(thread_data->global_error);
		}

		// sync data among threads

		// copy x into x_t
		memcpy(thread_data->thread_xt + thread_data->thread_offset, thread_data->thread_x, thread_data->thread_rows * sizeof(double));

		if(mode == MPI) {
			for(int tid = 0; tid < thread_data->lim; tid++) {
				ilib_status_t status;
				if(tid != thread_data->tid) {
					// receive error from everyone
					ilib_msg_broadcast(ILIB_GROUP_SIBLINGS, tid, &error_t, sizeof(double), &status);
					error += error_t;

					// receive iterative x vector
					ilib_msg_broadcast(ILIB_GROUP_SIBLINGS, tid, thread_data->thread_xt + (tid * thread_data->thread_rows), thread_data->thread_rows * sizeof(double), &status);
				} else {
					// broadcast error to everyone else
					ilib_msg_broadcast(ILIB_GROUP_SIBLINGS, tid, &(thread_data->thread_error), sizeof(double), &status);

					// broadcast own x vector calculations
					ilib_msg_broadcast(ILIB_GROUP_SIBLINGS, tid, thread_data->thread_x, thread_data->thread_rows * sizeof(double), &status);
				}
			}
		}

		// test for convergence
		if(error < EPSILON) {
			*iterations = step;
			return(SUCCESS);
		} else if(error == INFINITY) {
			*iterations = step;
			return(FAILURE);
		}
	}
	return(FAILURE);
}

/**
 * Pass initial data to the other nodes.
 */
int mastr_initialize(data *thread_data, int lim, double **a, double *b, int dim, int mode) {
	if(mode == MPI) {
		// pass along a and b vectors
		for(int tid = 1; tid < lim; tid++) {
			for(int row = 0; row < thread_data->thread_rows; row++) {
				ilib_msg_send(ILIB_GROUP_SIBLINGS, tid, MSG_HANDLE, a[tid * thread_data->thread_rows + row], dim * sizeof(double));
			}
			ilib_msg_send(ILIB_GROUP_SIBLINGS, tid, MSG_HANDLE + 1, b + tid * thread_data->thread_rows, thread_data->thread_rows * sizeof(double));
		}

		// ROOT data initialization doesn't need message passing.
		for(int row = 0; row < thread_data->thread_rows; row++) {
			memcpy(thread_data->thread_a[row], a[row], dim * sizeof(double));
		}
		memcpy(thread_data->thread_b, b, thread_data->thread_rows * sizeof(double));
	} else {
		// if mode == DMA, copy arrays into main root node, and set offset pointers to other nodes
		//	note that this means the nodes are NOT identical - root node **a is set at a[0][0], other nodes
		//	are offset at a[offset][0]
		address *instance = create_address_instance(thread_data->error_lock, thread_data->global_error, thread_data->thread_a, thread_data->thread_b, thread_data->thread_x, thread_data->thread_xt);
		for(int tid = 1; tid < lim; tid++) {
			ilib_msg_send(ILIB_GROUP_SIBLINGS, tid, MSG_HANDLE, &instance, sizeof(address *));
		}

		// set the memory contents
		for(int row = 0; row < dim; row++) {
			memcpy(thread_data->thread_a[row], a[row], dim * sizeof(double));
		}
		memcpy(thread_data->thread_b, b, dim * sizeof(double));
	}
	return(0);
}

/**
 * Prep data for processing.
 */
int slave_initialize(data *thread_data, int mode) {
	ilib_status_t status;
	if(mode == MPI) {
		for(int row = 0; row < thread_data->thread_rows; row++) {
			ilib_msg_receive(ILIB_GROUP_SIBLINGS, ROOT, MSG_HANDLE, thread_data->thread_a[row], thread_data->dim * sizeof(double), &status);
		}
		ilib_msg_receive(ILIB_GROUP_SIBLINGS, ROOT, MSG_HANDLE + 1, thread_data->thread_b, thread_data->thread_rows * sizeof(double), &status);
	} else {
		address *instance;
		ilib_msg_receive(ILIB_GROUP_SIBLINGS, ROOT, MSG_HANDLE, &instance, sizeof(address *), &status);

		thread_data->thread_a = instance->a + thread_data->tid * thread_data->thread_rows;
		thread_data->thread_b = instance->b + thread_data->tid * thread_data->thread_rows;
		thread_data->thread_x = instance->x + thread_data->tid * thread_data->thread_rows;
		thread_data->thread_xt = instance->xt;
		thread_data->error_lock = instance->error_lock;
		thread_data->global_error = instance->global_error;

	}
	return(0);
}

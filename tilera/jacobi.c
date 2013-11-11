#include <errno.h>
#include <ilib.h>
#include <math.h>
#include <stdio.h>
#include <tmc/cmem.h>

#include "config.h"
#include "data.h"
#include "file.h"
#include "jacobi.h"

typedef ilibStatus ilib_status_t;

int main(int argc, char *argv[]) {
	char *folder = (char *) calloc(MAX_BUF, sizeof(char));
	int cores;
	if(argc == 3) {
		sscanf(argv[1], "%s", folder);
		sscanf(argv[2], "%i", &cores);
	} else {
		char *cmd = calloc(MAX_BUF, sizeof(char));
		sscanf(argv[0], "/%s", cmd);
		fprintf(stderr, "usage: %s [ folder ] [ cores ]\n", cmd);
		return(0);
	}

	char *pipe_a = (char *) calloc(MAX_BUF, sizeof(char));
	char *pipe_b = (char *) calloc(MAX_BUF, sizeof(char));
	char *pipe_x = (char *) calloc(MAX_BUF, sizeof(char));

	sprintf(pipe_a, "pipe/%s/a.txt", folder);
	sprintf(pipe_b, "pipe/%s/b.txt", folder);
	sprintf(pipe_x, "pipe/%s/x_%i.txt", folder, cores);

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

		data_dim(thread_data, dim);
		mastr_initialize(thread_data, lim, a, b, dim);

		fprintf(stderr, "\nfolder %s, dim %i, cores %i\n", folder, dim, cores);
	} else {
		int dim;
		ilib_status_t status;
		ilib_msg_broadcast(ILIB_GROUP_SIBLINGS, ROOT, &dim, sizeof(dim), &status);

		data_dim(thread_data, dim);
		slave_initialize(thread_data);
	}

	int result = iterate(thread_data);

	// return data
	if(thread_data->tid == ROOT) {
		set_x(thread_data->thread_xt, thread_data->dim, pipe_x);
		if(result == FAILURE) {
			fprintf(stderr, "failed to converge\n");
		}
	}

	ilib_msg_barrier(ILIB_GROUP_SIBLINGS);

	ilib_finish();
	return(result);
}

int iterate(data *thread_data) {
	for(int step = 0; step < MAX_ITERATIONS; step++) {
		thread_data->thread_error = 0.0;
		int i;
		for(int row = 0; row < thread_data->thread_rows; row++) {		// i
			i = thread_data->thread_offset + row;				// global_i
			double lower = 0.0;
			double upper = 0.0;
			for(int j = 0; j < i; j++) {
				lower += thread_data->thread_a[row][j] * thread_data->thread_xt[j];
				// printf("lower (row = %i, j = %i) (= %f) * x[j = %i] (= %f) = %f\n", row, j, thread_data->thread_a[row][j], j, thread_data->thread_xt[j], thread_data->thread_a[row][j] * thread_data->thread_xt[j]);

				
			}
			for(int j = i + 1; j < thread_data->dim; j++) {
				upper += thread_data->thread_a[row][j] * thread_data->thread_xt[j];
				// printf("upper (row = %i, j = %i) (= %f) * x[j = %i] (= %f) = %f\n", row, j, thread_data->thread_a[row][j], j, thread_data->thread_xt[j], thread_data->thread_a[row][j] * thread_data->thread_xt[j]);
			}

			thread_data->thread_x[row] = (thread_data->thread_b[row] - (lower + upper)) / thread_data->thread_a[row][i];

			// printf("x[%i] = %f\n", row, thread_data->thread_x[row]);

			// check if estimates converge
			double x = thread_data->thread_x[row] - thread_data->thread_xt[i];
			thread_data->thread_error += x * x;
		}

		// sync x guesses, and get global error
		double error = thread_data->thread_error;
		double error_t;

		// copy x into x_t
		memcpy(thread_data->thread_xt + thread_data->thread_offset, thread_data->thread_x, thread_data->thread_rows * sizeof(double));

		// for(int i = 0; i < thread_data->dim; i++) {
		//	printf("old x[%i] = %f\n", i, thread_data->thread_xt[i]);
		// }

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

		// test for convergence
		if(error < EPSILON) {
			return(SUCCESS);
		} else if(error == INFINITY) {
			return(FAILURE);
		}
	}
	return(FAILURE);
}

/**
 * Pass initial data to the other nodes.
 */
int mastr_initialize(data *thread_data, int lim, double **a, double *b, int dim) {
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

	return(0);
}

/**
 * Prep data for processing.
 */
int slave_initialize(data *thread_data) {
	ilib_status_t status;
	for(int row = 0; row < thread_data->thread_rows; row++) {
		ilib_msg_receive(ILIB_GROUP_SIBLINGS, ROOT, MSG_HANDLE, thread_data->thread_a[row], thread_data->dim * sizeof(double), &status);
	}
	ilib_msg_receive(ILIB_GROUP_SIBLINGS, ROOT, MSG_HANDLE + 1, thread_data->thread_b, thread_data->thread_rows * sizeof(double), &status);
	return(0);
}

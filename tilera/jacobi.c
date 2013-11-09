#include <ilib.h>
#include <stdio.h>
#include <tmc/cmem.h>
#include <math.h>

#include "config.h"
#include "data.h"
#include "file.h"
#include "jacobi.h"

typedef ilibStatus ilib_status_t;

void * p_calloc(size_t x, size_t y, int line) {
	void *ret = calloc(x, y);
	// check calloc isn't allocating kernel space memory
	if(ret > (void *) PHYS_BASE) {
		printf("calloc_error with return val 0x%08x at line %d\n", ret, line);
		return(NULL);
	}

	return(ret);
}

#define calloc(x, y) p_calloc(x, y, __LINE__)

int main() {
	ilib_init();

	if(ilib_proc_go_parallel(CORES, NULL) != ILIB_SUCCESS) {
		ilib_die("Fatal error - cannot proceed.");
	}

	int tid = ilib_group_rank(ILIB_GROUP_SIBLINGS);
	int lim = ilib_group_size(ILIB_GROUP_SIBLINGS);

	data *thread_data = data_init();
	thread_data->tid = tid;
	thread_data->lim = lim;
	thread_data->thread_rows = (int) floor(DIM / lim);
	thread_data->thread_offset = tid * thread_data->thread_rows;

	// allocate memory for data in each thread
	thread_data->thread_a = (double **) calloc(thread_data->thread_rows, sizeof(double *));
	for(int row = 0; row < thread_data->thread_rows; row++) {
		thread_data->thread_a[row] = (double *) calloc(DIM, sizeof(double));
	}
	thread_data->thread_b = (double *) calloc(thread_data->thread_rows, sizeof(double));
	thread_data->thread_x = (double *) calloc(thread_data->thread_rows, sizeof(double));
	thread_data->thread_xt = (double *) calloc(DIM, sizeof(double));
	for(int row = 0; row < DIM; row++) {
		thread_data->thread_xt[row] = 1.0;
	}

	if(tid == ROOT) {
		int dim;

		double *b = get_b("input/simple_b.txt", &dim);

		// double *b = (double *) calloc(DIM, sizeof(double));
		// for(int i = 0; i < DIM; i++) {
		//	b[i] = 12.4;
		// }

		// printf("jacobi dim 0x%08x val %i\n", &dim, dim);
		// printf("jacobi b 0x%08x\n", b);
		// for(int i = 0; i < dim; i++) {
		//	printf("jacobi: b[%i] %f\n", i, b[i]);
		// }
		
		// placeholder - assuming data is loaded
		double **a = (double **) calloc(DIM, sizeof(double *));
		for(int i = 0; i < DIM; i++) {
			a[i] = (double *) calloc(DIM, sizeof(double));
		}

		// DEBUG test set thingies
		for(int i = 0; i < DIM; i++) {
			for(int j = 0; j < DIM; j++) {
				if(i == j) {
					a[i][j] = 1.0;
				}
			}
		}

		double *x = (double *) calloc(DIM, sizeof(double));

		mastr_initialize(thread_data, lim, a, b);
	}
	slave_initialize(thread_data, tid);

	for(int step = 0; step < 10; step++) {
		thread_data->thread_error = 0.0;
		for(int row = 0; row < thread_data->thread_rows; row++) {		// i
			int i = thread_data->thread_offset + row;			// global_i
			double sum_1 = 0.0;
			double sum_2 = 0.0;
			for(int j = 0; j < i; j++) {
				sum_1 += thread_data->thread_a[row][j] * thread_data->thread_xt[j];
			}
			for(int j = i + 1; j < DIM; j++) {
				sum_2 += thread_data->thread_a[row][j] * thread_data->thread_xt[j];
			}

			thread_data->thread_x[i] = (-sum_1 - sum_2 + thread_data->thread_b[row]) / thread_data->thread_a[row][i];
			double x = thread_data->thread_x[row] - thread_data->thread_xt[i];
			thread_data->thread_error += x * x;
		}

		// get global error and x vectors
		double error = thread_data->thread_error;
		double te;

		memcpy(thread_data->thread_xt + thread_data->thread_offset, thread_data->thread_x, thread_data->thread_rows * sizeof(double));
		for(int tid = 0; tid < thread_data->lim; tid++) {
			ilib_status_t status;
			if(tid != thread_data->tid) {
				// receive error from everyone
				// ilib_msg_broadcast(ILIB_GROUP_SIBLINGS, tid, &te, sizeof(double), &status);
				error += te;

				// receive iterative x vector
				ilib_msg_broadcast(ILIB_GROUP_SIBLINGS, tid, thread_data->thread_xt + (tid * thread_data->thread_rows), thread_data->thread_rows * sizeof(double), &status);
			} else {
				// broadcast error to everyone else
				// ilib_msg_broadcast(ILIB_GROUP_SIBLINGS, tid, &(thread_data->thread_error), sizeof(double), &status);

				// broadcast own x vector calculations
				ilib_msg_broadcast(ILIB_GROUP_SIBLINGS, tid, thread_data->thread_x, thread_data->thread_rows * sizeof(double), &status);
			}
		}
		if(error < 0.001) {
			break;
		}
	}

	data_vomit(thread_data);

	// ... gather data and process by ROOT
	if(tid == ROOT) {
	} else {
	}

	ilib_finish();
	return(0);
}

/**
 * Pass initial data to the other nodes.
 */
int mastr_initialize(data *thread_data, int lim, double **a, double *b) {
	for(int tid = 1; tid < lim; tid++) {
		for(int row = 0; row < thread_data->thread_rows; row++) {
			ilib_msg_send(ILIB_GROUP_SIBLINGS, tid, MSG_HANDLE, a[tid * thread_data->thread_rows + row], DIM * sizeof(double));
		}
		ilib_msg_send(ILIB_GROUP_SIBLINGS, tid, MSG_HANDLE + 1, b + tid * thread_data->thread_rows, thread_data->thread_rows * sizeof(double));
	}

	// ROOT data initialization doesn't need message passing.
	for(int row = 0; row < thread_data->thread_rows; row++) {
		memcpy(thread_data->thread_a[row], a[row], DIM * sizeof(double));
	}
	memcpy(thread_data->thread_b, b, thread_data->thread_rows * sizeof(double));

	return(0);
}

/**
 * Prep data for processing.
 */
int slave_initialize(data *thread_data, int tid) {
	ilib_status_t status;
	if(tid != ROOT) {
		for(int row = 0; row < thread_data->thread_rows; row++) {
			ilib_msg_receive(ILIB_GROUP_SIBLINGS, ROOT, MSG_HANDLE, thread_data->thread_a[row], DIM * sizeof(double), &status);
		}
		ilib_msg_receive(ILIB_GROUP_SIBLINGS, ROOT, MSG_HANDLE + 1, thread_data->thread_b, thread_data->thread_rows * sizeof(double), &status);
	}

	return(0);
}

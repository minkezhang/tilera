#include <math.h>
#include <stdlib.h>
#include <stdio.h>

#include <tmc/cmem.h>

#include "config.h"
#include "data.h"

/**
 * Allocates a data_t struct and returns a pointer to the instance.
 */
data * data_init() {
	data *instance = calloc(1, sizeof(data));

	instance->thread_error = 0.0;
	return(instance);
}

/**
 * Allocates vectors space for each data struct.
 */
void data_allocate(data *thread, int mode) {
	if(mode == MPI) {
		thread->thread_a = (double **) calloc(thread->thread_rows, sizeof(double *));
		for(int row = 0; row < thread->thread_rows; row++) {
			thread->thread_a[row] = (double *) calloc(thread->dim, sizeof(double));
		}
		thread->thread_b = (double *) calloc(thread->thread_rows, sizeof(double));
		thread->thread_x = (double *) calloc(thread->thread_rows, sizeof(double));
		thread->thread_xt = (double *) calloc(thread->dim, sizeof(double));
		// set initial guess to x = [ 1, 1, ... ]
		for(int row = 0; row < thread->dim; row++) {
			thread->thread_xt[row] = 1.0;
		}
	} else {
		// if mode == DMA, we only need to allocate one copy of data stored in memory
		if(thread->tid == ROOT) {
			thread->thread_a = (double **) tmc_cmem_malloc(thread->dim * sizeof(double *));
			for(int row = 0; row < thread->dim; row++) {
				thread->thread_a[row] = (double *) tmc_cmem_malloc(thread->dim * sizeof(double));
			}
			thread->thread_b = (double *) tmc_cmem_malloc(thread->dim * sizeof(double));
			thread->thread_x = (double *) tmc_cmem_malloc(thread->dim * sizeof(double));
			thread->thread_xt = (double *) tmc_cmem_malloc(thread->dim * sizeof(double));
			for(int row = 0; row < thread->dim; row++) {
				thread->thread_xt[row] = 1.0;
			}
			
			thread->global_error = (double *) tmc_cmem_malloc(sizeof(double));
			thread->error_lock = (ilib_mutex_t *) tmc_cmem_malloc(sizeof(ilib_mutex_t));

			// initialze the lock
			ilib_mutex_init(thread->error_lock);
		}
	}
}

/**
 * Sets the dimensions of the data struct arrays and allocates enough memory to store data.
 */
void data_dim(data *thread, int dim, int mode) {
	thread->dim = dim;

	printf("dim = %i, t->dim / t->lim = %i / %i = %i\n", dim, thread->dim, thread->lim, (int) floor(thread->dim / thread->lim));
	thread->thread_rows = (int) floor(thread->dim / thread->lim);
	thread->thread_offset = thread->tid * thread->thread_rows;

	// allocate memory for data in each thread
	data_allocate(thread, mode);
}

void data_vomit(data *thread) {
	printf("
thread vomit for %i
	thread_rows	%i
	thread_lim	%i
	thread_offset	%i
	thread_error	%.2f
	thread_xt	0x%08x\n" \
	// thread_a	0x%08x
	// thread_b	0x%08x
	// thread_x	0x%08x\n", thread->tid, thread->thread_rows, thread->lim, thread->thread_offset, thread->thread_error, thread->thread_xt, thread->thread_a, thread->thread_b, thread->thread_x);
	, thread->tid, thread->thread_rows, thread->lim, thread->thread_offset, thread->thread_error, thread->thread_xt);

	for(int row = 0; row < thread->thread_rows; row++) {
		printf("%i) b[%i] = %f\n", thread->tid, row, thread->thread_b[row]);
	}
}

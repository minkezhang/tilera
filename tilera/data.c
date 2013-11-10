#include <math.h>
#include <stdlib.h>
#include <stdio.h>

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
void data_allocate(data *thread) {
	thread->thread_a = (double **) calloc(thread->thread_rows, sizeof(double *));
	for(int row = 0; row < thread->thread_rows; row++) {
		thread->thread_a[row] = (double *) calloc(DIM, sizeof(double));
	}
	thread->thread_b = (double *) calloc(thread->thread_rows, sizeof(double));
	thread->thread_x = (double *) calloc(thread->thread_rows, sizeof(double));
	thread->thread_xt = (double *) calloc(thread->dim, sizeof(double));
	for(int row = 0; row < thread->dim; row++) {
		thread->thread_xt[row] = 1.0;
	}
}

/**
 * Sets the dimensions of the data struct arrays and allocates enough memory to store data.
 */
void data_dim(data *thread, int dim) {
	thread->dim = dim;

	thread->thread_rows = (int) floor(thread->dim / thread->lim);
	thread->thread_offset = thread->tid * thread->thread_rows;

	// allocate memory for data in each thread
	data_allocate(thread);
}

void data_vomit(data *thread) {
	printf("
thread vomit for %i
	thread_rows	%i
	thread_lim	%i
	thread_offset	%i
	thread_error	%.2f
	thread_a	0x%08x
	thread_b	0x%08x
	thread_x	0x%08x\n", thread->tid, thread->thread_rows, thread->lim, thread->thread_offset, thread->thread_error, thread->thread_a, thread->thread_b, thread->thread_x);

	// print local a
	for(int i = 0; i < thread->thread_rows; i++) {
		char *line_buf = calloc(MAX_BUF, sizeof(char));
		int offset = 0;
		for(int j = 0; j < DIM; j++) {
			offset += sprintf(line_buf + offset, "%.2f ", thread->thread_a[i][j]);
		}
		printf("\t(%i) a[%i] %s\n", thread->tid, i, line_buf);
	}

	// print local b
	for(int i = 0; i < thread->thread_rows; i++) {
		printf("\t(%i) b[%i]\t%.2f\n", thread->tid, i, thread->thread_b[i]);
	}

	// print local x
	for(int i = 0; i < thread->thread_rows; i++) {
		printf("\t(%i) thread_x[%i] = %.2f\n", thread->tid, i, thread->thread_x[i]);
	}

	// print merged x
	if(thread->tid == ROOT) {
		for(int i = 0; i < thread->dim; i++) {
			printf("\t(%i) x[%i]\t%.2f\n", thread->tid, i, thread->thread_xt[i]);
		}
	}
}

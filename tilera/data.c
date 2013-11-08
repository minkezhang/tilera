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

void data_vomit(data *thread) {
	printf("
thread vomit for %i
	thread_rows	%i
	thread_offset	%i
	thread_error	%f
	thread_a	0x%08x
	thread_b	0x%08x
	thread_x	0x%08x\n", thread->tid, thread->thread_rows, thread->thread_offset, thread->thread_error, thread->thread_a, thread->thread_b, thread->thread_x);

	/*
	for(int i = 0; i < thread->thread_rows; i++) {
		for(int j = 0; j < DIM; j++) {
			printf("\t(%i) a[%i][%i]\t%f\n", thread->tid, i, j, thread->thread_a[i][j]);
		}
	}

	for(int i = 0; i < thread->thread_rows; i++) {
		printf("\t(%i) b[%i]\t%f\n", thread->tid, i, thread->thread_b[i]);
	}
	*/
	if(thread->tid == ROOT) {
		for(int i = 0; i < DIM; i++) {
			printf("\t(%i) x[%i]\t%f\n", thread->tid, i, thread->thread_xt[i]);
		}
	}
}

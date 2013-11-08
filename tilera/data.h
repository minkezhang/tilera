#ifndef DATA_H
#define DATA_H

typedef struct data_t {
	int tid;
	int lim;

	int thread_rows;
	int thread_offset;

	// error counters
	double thread_error;

	// in a Jacobi transformation, we can assume that the matrix
	//	is an n x n square, invertible matrix
	double **thread_a;
	double *thread_b;
	double *thread_x;
	// x_temporary array to do initial calculations in
	double *thread_xt;
} data;

data * data_init(void);
void data_vomit(data *);

#endif

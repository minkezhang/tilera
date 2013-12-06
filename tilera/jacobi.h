#ifndef JACOBI_H
#define JACOBI_H

#include "data.h"

typedef ilibStatus ilib_status_t;

// keep track of misellanous address spaces
typedef struct address_t {
	ilib_mutex_t *error_lock;
	double *global_error;
	double **a;
	double *b;
	double *x;
	double *xt;
} address;


address * create_address_instance(ilib_mutex_t *, double *, double **, double *, double *, double *);
int mastr_initialize(data *, int, double **, double *, int, int);
int slave_initialize(data *, int);
int iterate(data *, int *, int);

#endif

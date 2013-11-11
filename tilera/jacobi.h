#ifndef JACOBI_H
#define JACOBI_H

#include "data.h"

int mastr_initialize(data *, int, double **, double *, int);
int slave_initialize(data *);
int iterate(data *, int *);

#endif

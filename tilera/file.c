#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "file.h"

FILE * get_fp(char *, char *);

/**
 * The a matrix is given in the file with format (where a_ij = a[i][j]) (and where i is the row within the file).
 *
 * a_11 ... a_1n
 *      ...
 * a_n1 ... a_nn
 *
 * The int dim refers the dimension of the dim x dim array, and is set as a side-effect by get_b().
 */
double ** get_a(char *filename, int dim) {
	double **a = calloc(dim, sizeof(double *));
	for(int row = 0; row < dim; row++) {
		a[row] = calloc(dim, sizeof(double));
	}

	FILE *fp = get_fp(filename, "r");

	size_t m = MAX_BUF;
	char *line_buf = calloc(m, sizeof(char));

	double result;
	int cur_row = 0;
	int cur_element;

	char *tok;
	char *tok_ptr;

	while(getline(&line_buf, &m, fp) != -1) {
		printf("line buf %s\n", line_buf);
		cur_element = 0;
		for(tok = strtok_r(line_buf, " ", &tok_ptr); tok; tok = strtok_r(NULL, " ", &tok_ptr)) {
			sscanf(tok, "%lf", &result);
			a[cur_row][cur_element] = result;
			cur_element++;
		}
		cur_row++;
	}

	return(a);
}

/**
 * The b vector is given in the file with format
 *
 * b_1
 * ...
 * b_n
 *
 * The pointer to dim is either NULL or refers to some best guess of the dimensions of the array;
 *	get_b() will set dim to the actual size as a side-effect.
 */
double * get_b(char *filename, int *dim) {
	int n = 0;
	size_t size;

	size = *dim;
	double *b = calloc(size, sizeof(double));

	// Get the file specified.
	FILE *fp = get_fp(filename, "r");

	size_t m = MAX_BUF;
	char *line_buf = calloc(m, sizeof(char));
	double result;

	while(getline(&line_buf, &m, fp) != -1) {
		sscanf(line_buf, "%lf\n", &result);
		if(n >= size) {
			size++;
			b = realloc(b, size * sizeof(double));
		}
		b[n] = result;
		n++;
	}

	*dim = n;
	return(b);
}

/**
 * Error-tolerant file open function.
 */
FILE * get_fp(char *filename, char *mode) {
	FILE *fp = fopen(filename, mode);
	if(fp == NULL) {
		printf("Cannot open file %s\n", filename);
		exit(EXIT_FAILURE);
	}
	return(fp);
}

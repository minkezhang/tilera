#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config.h"
#include "file.h"

FILE * get_fp(char *, char *);

/**
 * The a matrix is given in the file with format (where a_ij = a[i][j])
 *
 * a_11 ... a_1n
 *      ...
 * a_n1 ... a_nn
 */
double ** get_a(char *filename, int *dim) {
	int n = 0;
	size_t size = 0;
	double **a = NULL;

	FILE *fp = get_fp(filename, "r");

	size_t e = MAX_BUF;
	char *line_buf = calloc(e, sizeof(char));
	double *row;
	double result;
	int m = 0;
	size_t m_size = 0;

	char *tok;
	char *tok_ptr;

	while(getline(&line_buf, &e, fp) != -1) {
		printf("new line %s\n", line_buf);
		if(n >= size) {
			size++;
			a = realloc(a, size * sizeof(double *));
			printf("reallocated a\n");
		}

		m = 0;
		m_size = 0;
		row = NULL;
		for(tok = strtok_r(line_buf, " ", &tok_ptr); tok; tok = strtok_r(NULL, " ", &tok_ptr)) {
		// while((tok = strtok_r(line_buf, " ", &tok_ptr)) != NULL) {
			printf("new token %s\n", tok);
			if(m >= m_size) {
				m_size++;
				row = realloc(row, m_size * sizeof(double));	
			}
			printf("reallocated row\n");
			sscanf(tok, "%lf", &result);
			printf("scanned double %f\n", result);
			row[m] = result;
			printf("stored double into spot %i\n", m);
			m++;
		}
		a[n] = row;
		n++;
	}

	*dim = n;
	return(a);
}

/**
 * The b vector is given in the file with format
 *
 * b_1
 * ...
 * b_n
 */
double * get_b(char *filename, int *dim) {
	int n = 0;
	size_t size = 0;

	double *b = NULL;

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

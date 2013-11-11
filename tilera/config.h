#ifndef CONFIG
#define CONFIG

/**
 * Global preprocessor directives for various configurational data.
 *
 * Some will eventually be converted into command-line arguments.
 */

// Define the upper limit of user memory space.
#define PHYS_BASE 0xc0000000

// Define the maximum number of times in which the program attempts to converge the x-vector guess.
#define MAX_ITERATIONS 10000

// Maximum line buffer length.
#define MAX_BUF 1024

// Defines the convergence limit.
#define EPSILON 0.001

// Total number of cores to simulate.
#define CORES 1

// Defines the root node ID.
#define ROOT 0

// Defines a message ID for sending and receiving messages using MPI.
#define MSG_HANDLE 0

// The number of rows [ and columns ] in the input matrix; should be divisible by CORES.
#define DIM 10

// Return codes for int-returned functions.
#define SUCCESS 0
#define FAILURE -1

#endif

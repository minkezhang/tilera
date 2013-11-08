#ifndef CONFIG
#define CONFIG

/**
 * Global preprocessor directives for various configurational data.
 *
 * Some will eventually be converted into command-line arguments.
 */

// Total number of cores to simulate.
#define CORES 10

// Defines the root node ID.
#define ROOT 0

// Defines a message ID for sending and receiving messages using MPI.
#define MSG_HANDLE 0

// The number of rows [ and columns ] in the input matrix; should be divisible by CORES
#define DIM 10

#endif
// CCID: hmhoang
// Name : Hai Hoang
// Student ID: 1624290
// References: APUE 3/E Textbook, Linux/C manual page (man7.org) and resources on eClass.
#include <stdio.h>
#include "a4w22.h"
#include "simulator.h"

// Main function, receive user input and start simulating jobs.
int main (int argc, char *argv[]) {
    char inputFile[MAX_BUFFER];
    int monitorTime = 0, NITER = 0;

    // Only proceed if argument = 4 i.e a4w22 inputfile monitortime NITER.
    if (argc == 4) {
        // If valid arguments save arguments
        strcpy(inputFile, argv[1]);
        monitorTime = atoi(argv[2]);
        NITER = atoi(argv[3]);
        // Start simulator
        simulator(inputFile, monitorTime, NITER);
    }
    else {
        printf("Invalid arguments.\n");
        exit(1);
    }
}
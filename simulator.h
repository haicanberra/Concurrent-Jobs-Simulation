// CCID: hmhoang
// Name : Hai Hoang
// Student ID: 1624290
// References: APUE 3/E Textbook, Linux/C manual page (man7.org) and resources on eClass.
#ifndef simulator_h
#define simulator_h

#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/times.h>
#include <unistd.h>

#define NRES_TYPES 10
#define NJOBS 25

#define MAX_NAME 32
#define MAX_LINE 512
#define WAIT 0
#define RUN 1
#define IDLE 2
#define TRUE 3
#define FALSE 4

// Change this to change pause time (millisecond)
#define PAUSE 1
#define PAUSESYNC 5

// Resources, maxAvail when in a Job is the needed amount.
typedef struct {
    char name[MAX_NAME];
    int maxAvail;
} Resource;

// Job, contain information of each job.
typedef struct {
    char name[MAX_NAME];
    int busyTime;
    int idleTime;
    int iterations;
    int state;
    int working;
    Resource resources[NRES_TYPES];
    int resources_length;
    double sum_waitTime;
} Job;

void simulator(char *inputFile, int monitorTime, int inputNITER);
void millisleep(int amount);
void *thread_function(void *arg);
void *monitor_function(void *arg);

#endif
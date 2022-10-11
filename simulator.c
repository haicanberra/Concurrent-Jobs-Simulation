// CCID: hmhoang
// Name : Hai Hoang
// Student ID: 1624290
// References: APUE 3/E Textbook, Linux/C manual page (man7.org) and resources on eClass.
#include <stdio.h>
#include "a4w22.h"
#include "simulator.h"

int NITER, numJobs, numResources;
static long clktck = 0;

// APUE page 281
struct tms tmsstart, tmsend;
clock_t start, end;

Job jobs[NJOBS];
Resource resources[NRES_TYPES];

pthread_t threadIds[NJOBS];
pthread_mutex_t jobLock, iterLock, monitorLock;

// Simulate the execution of each job.
void simulator(char *inputFile, int monitorTime, int inputNITER) {
    // Save numper of iterations
    NITER = inputNITER;

    // Get clock ticks, APUE page 281
    if (clktck == 0) {
        if ((clktck = sysconf(_SC_CLK_TCK)) < 0) {
            printf("Failed to set clocktick.\n");
            exit(1);
        }
    }

    // Initialize mutexes
    if (pthread_mutex_init(&jobLock, NULL) != 0 || pthread_mutex_init(&iterLock, NULL) != 0 || pthread_mutex_init(&monitorLock, NULL) != 0) {
        printf("Failed to initialize mutex.\n");
        exit(1);
    }

    // Start timer
    start = times(&tmsstart);

    // Read data from input file
    char data[MAX_LINE][MAX_BUFFER];
    char line[MAX_BUFFER];
    FILE *file;
    file = fopen(inputFile, "r");
    int currentLine = 0;
    while (fgets(line, sizeof(line), file) && (currentLine < MAX_LINE)) {
        strcpy(data[currentLine], line);
        memset(line, 0, sizeof(line));
        currentLine++;
    }
    // Currentline = total number of lines now, numJobs & numRes minus 1 to use as index.
    numJobs = 0;
    numResources = 0;
    for (int i = 0; i < currentLine; i++) {
        if (data[i][0] != '#' && data[i][0] != '\n') {
            // Get first word of the line
            char *token = strtok(data[i], " \n");
            // Resources name0:value0 name1:value1..
            //     0         1             2        (value of temp)
            if (token != NULL) {
                if (strcmp(token, "resources") == 0) {
                    // Name and value at index 0
                    char *token_name = strtok(NULL, " :\n");
                    char *token_maxAvail = strtok(NULL, " :\n");
                    while (token_name != NULL && token_maxAvail != NULL) {
                        // Get resource name
                        if (numResources < NRES_TYPES) {
                            strcpy(resources[numResources].name, token_name);
                            if (atoi(token_maxAvail) != 0) {
                                resources[numResources].maxAvail = atoi(token_maxAvail);
                            }
                            // Get next resource type
                            token_name = strtok(NULL, " :\n");
                            token_maxAvail = strtok(NULL, " :\n");
                            numResources++;
                        }
                    }
                }
                // job jobname busytime idletime name0:value0 name1:value1..
                //  0     1       2         3        4              5
                if (strcmp(token, "job") == 0 && numJobs < NJOBS) {
                    // Get jobName, resources..
                    char *token1 = strtok(NULL, " :\n");
                    int temp1 = 1, temp2 = 0;
                    while (token1 != NULL) {
                        switch (temp1) {
                            case 1:
                                // Job name
                                strcpy(jobs[numJobs].name, token1);
                                break;
                            case 2:
                                // Busy time
                                if (atoi(token1) != 0) {
                                    jobs[numJobs].busyTime = atoi(token1);
                                }
                                break;
                            case 3:
                                // Idle time
                                if (atoi(token1) != 0) {
                                    jobs[numJobs].idleTime = atoi(token1);
                                }
                                break;
                            default:
                                // For resources of the job
                                // Token1 now contains name0
                                strcpy(jobs[numJobs].resources[temp2].name, token1);
                                // Token1 assigned to resource's value of a job
                                token1 = strtok(NULL, " :\n");
                                if (token1 != NULL) {
                                    jobs[numJobs].resources[temp2].maxAvail = atoi(token1);
                                }
                                else {
                                    printf("Token1 error.\n");
                                    exit(1);
                                }
                                temp2++;
                                break;
                        }
                        temp1++;
                        token1 = strtok(NULL, " :\n");
                    }
                    jobs[numJobs].iterations = 0;
                    jobs[numJobs].state = IDLE;
                    jobs[numJobs].working = FALSE;
                    jobs[numJobs].resources_length = temp2;
                    jobs[numJobs].sum_waitTime = 0;
                    numJobs++;
                }
            }
        }
    }

    // Monitor thread
    pthread_t monitortid;
    int monitorTimeArray[1];
    monitorTimeArray[0] = monitorTime;
    if (pthread_create(&monitortid, NULL, monitor_function, &monitorTimeArray[0])) {
        printf("Failed to create monitor thread.\n");
        exit(1);
    }

    // Job threads, each job is 1 thread
    pthread_t jobtid;
    int indices[numJobs];
    for (int i = 0; i < numJobs; i++) {
        if (pthread_mutex_lock(&jobLock)) {
            printf("Failed to lock job.\n");
            exit(1);
        }
        indices[i] = i;
        if (pthread_create(&jobtid, NULL, thread_function, &indices[i])) {
            printf("Failed to run job.\n");
            exit(1);
        }
    }

    // Pause to sync
    millisleep(PAUSESYNC);
    for (int i = 0; i < numJobs; i++) {
        if (pthread_join(threadIds[i], NULL)) {
            printf("Failed to join threads.\n");
            exit(1);
        }
    }

    printf("\nSystem Resources:\n");
    for (int i = 0; i < numResources; i++) {
        printf("    %s: (maxAvail= %d, held= 0)\n", resources[i].name, resources[i].maxAvail);
    }

    printf("\nSystem Jobs:\n");
    char currentState[MAX_BUFFER];
    strcpy(currentState, " ");
    for (int i = 0; i < numJobs; i++) {
        // Print jobs info
        memset(currentState, 0, sizeof(currentState));
        if (jobs[i].state == WAIT) {
            strcpy(currentState, "WAIT");
        }
        else if (jobs[i].state == IDLE) {
            strcpy(currentState, "IDLE");
        }
        else if (jobs[i].state == RUN) {
            strcpy(currentState, "RUN");
        }
        printf("[%d] %s (%s, runTime= %d msec, idleTime= %d msec):\n        (tid= %lu)\n", i, jobs[i].name, currentState, jobs[i].busyTime, jobs[i].idleTime, threadIds[i]);

        // Print job's resources
        for (int j = 0; j < jobs[i].resources_length; j++) {
            printf("        %s: (needed= %d, held= 0)\n", jobs[i].resources[j].name, jobs[i].resources[j].maxAvail);
        }

        // Print times
        printf("        (RUN: %d times, WAIT: %.0f msec)\n\n", jobs[i].iterations, jobs[i].sum_waitTime);
    }

    // Print running time
    end = times(&tmsend);
    printf("Running time= %.0f msec.\n\n", (double) ((end-start)/(double)clktck *1000));
}

// Sleep for "amount" milliseconds.
void millisleep(int amount) {
    int temp = 0;
    struct timespec sleepduration;
    // Convert amount to nanosecond, get second then get the rest
    sleepduration.tv_sec = (long) amount/1000;
    sleepduration.tv_nsec = (long) ((amount%1000)*1000000);
    temp = nanosleep(&sleepduration, NULL);
    if (temp != 0) {
        printf("Failed to sleep.\n");
        exit(1);
    }
}

// Function for job thread to process the job.
void *thread_function(void *arg) {
    // Save thread id
    int jobIndex = *((int*) arg);
    threadIds[jobIndex] = pthread_self();
    // Prepare for running job
    int temp = 0, done = 1;
    struct tms tmswstart, tmswend;
    clock_t wstart, wend;
    // Get the first job that doesnt have a thread yet and work on it
    for (int i = 0; i < numJobs; i++) {
        if (jobs[i].working != TRUE) {
            jobs[i].working = TRUE;
            // Unlock mutex
            if (pthread_mutex_unlock(&jobLock)) {
                printf("Failed to unlock job.\n");
                exit(1);
            }
            // Process the job and also measure wait time, lock monitor thread 
            // to pause monitor from fetching state when changing.
            if (pthread_mutex_lock(&monitorLock)) {
                printf("Failed to lock monitor for job.\n");
                exit(1);
            }
            jobs[i].state = WAIT;
            if (pthread_mutex_unlock(&monitorLock)) {
                printf("Failed to unlock monitor for job.\n");
                exit(1);
            }

            // Start timer
            wstart = times(&tmswstart);

            // Process job
            while (1) {
                if (pthread_mutex_lock(&iterLock)) {
                    printf("Failed to lock each iteration for job.\n");
                    exit(1);
                }
                
                // Get resources, done = 1 is resources amount satisfied.
                done = 1;
                for (int j = 0; j < jobs[i].resources_length; j++) {
                    for (int k = 0; k < numResources; k++) {
                        if (strcmp(jobs[i].resources[j].name, resources[k].name) == 0) {
                            if (resources[k].maxAvail < jobs[i].resources[j].maxAvail) {
                                done = 0;
                            }
                        }
                    }
                }
                
                // If resources left not satisfied
                if (done == 0) {
                    if (pthread_mutex_unlock(&iterLock)) {
                        printf("Failed to lock each iteration for job.\n");
                        exit(1);
                    }
                    millisleep(PAUSE);
                }
                else {
                    // Deduct resources from main pool
                    for (int m = 0; m < jobs[i].resources_length; m++) {
                        for (int n = 0; n < numResources; n++) {
                            if (strcmp(jobs[i].resources[m].name, resources[n].name) == 0) {
                                resources[n].maxAvail -= jobs[i].resources[m].maxAvail;
                            }
                        }
                    }
                    // Pause to make sure resources are synced.
                    millisleep(PAUSE);

                    // End wait timer
                    wend = times(&tmswend);
                    jobs[i].sum_waitTime += (((wend-wstart) / (double) clktck) * 1000);
                    if (pthread_mutex_unlock(&iterLock)) {
                        printf("Failed to unlock each iteration for job.\n");
                        exit(1);
                    }

                    // Got resource now do job
                    if (pthread_mutex_lock(&monitorLock)) {
                        printf("Failed to lock monitor for job.\n");
                        exit(1);
                    }
                    jobs[i].state = RUN;
                    if (pthread_mutex_unlock(&monitorLock)) {
                        printf("Failed to unlock monitor for job.\n");
                        exit(1);
                    }
                    millisleep(jobs[i].busyTime);

                    // Return resource
                    if (pthread_mutex_lock(&iterLock)) {
                        printf("Failed to lock each iteration for job.\n");
                        exit(1);
                    }
                    for (int m = 0; m < jobs[i].resources_length; m++) {
                        for (int n = 0; n < numResources; n++) {
                            if (strcmp(jobs[i].resources[m].name, resources[n].name) == 0) {
                                resources[n].maxAvail += jobs[i].resources[m].maxAvail;
                            }
                        }
                    }
                    if (pthread_mutex_unlock(&iterLock)) {
                        printf("Failed to unlock each iteration for job.\n");
                        exit(1);
                    }

                    // Idle
                    if (pthread_mutex_lock(&monitorLock)) {
                        printf("Failed to lock monitor for job.\n");
                        exit(1);
                    }
                    jobs[i].state = IDLE;
                    if (pthread_mutex_unlock(&monitorLock)) {
                        printf("Failed to unlock monitor for job.\n");
                        exit(1);
                    }
                    millisleep(jobs[i].idleTime);
                    jobs[i].iterations++;
                    temp++;

                    // End
                    end = times(&tmsend);
                    printf("job: %s (tid= %lu, iter= %d, time= %.0f msec)\n", jobs[i].name, pthread_self(), temp, (double) ((end-start)/(double)clktck *1000)); 

                    if (NITER == temp) {
                        pthread_exit(NULL);
                    }
                    else {
                        if (pthread_mutex_lock(&monitorLock)) {
                            printf("Failed to lock monitor for job.\n");
                            exit(1);
                        }
                        jobs[i].state = WAIT;
                        if (pthread_mutex_unlock(&monitorLock)) {
                            printf("Failed to unlock monitor for job.\n");
                            exit(1);
                        }
                        wstart = times(&tmswstart);
                    }
                }
            }
            break;
        }
    }
    pthread_exit(NULL);
}

// Function for monitor thread to print and monitor job states.
void *monitor_function(void *arg) {
    int monitorTime = *((int *) arg);
    char wait[MAX_BUFFER], run[MAX_BUFFER], idle[MAX_BUFFER];
    while (1) {
        memset(wait, 0, sizeof(wait));
        memset(run, 0, sizeof(run));
        memset(idle, 0, sizeof(idle));
        millisleep(monitorTime);
        if (pthread_mutex_lock(&monitorLock)) {
            printf("Failed to lock monitor for job.\n");
            exit(1);
        }
        
        strcpy(wait, "  [WAIT] ");
        strcpy(run, "  [RUN] ");
        strcpy(idle, "  [IDLE] ");
        for (int i = 0; i < numJobs; i++) {
            switch (jobs[i].state) {
                case WAIT:
                    strcat(wait, jobs[i].name);
                    strcat(wait, " ");
                    break;
                case RUN:
                    strcat(run, jobs[i].name);
                    strcat(run, " ");
                    break;
                case IDLE:
                    strcat(idle, jobs[i].name);
                    strcat(idle, " ");
                    break;
            }
        }
        printf("\nmonitor:\n    %s\n    %s\n    %s\n\n", wait, run, idle);

        if (pthread_mutex_unlock(&monitorLock)) {
            printf("Failed to unlock monitor for job.\n");
            exit(1);
        }
    }
}

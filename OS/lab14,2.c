#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <semaphore.h>


#define SUCCESS 0
#define NUMBER_OF_LINES 10
#define BUFFER_SIZE 256
#define SEMAPHORE1 0
#define SEMAPHORE2 1
#define SEMAPHORE3 2
#define SEMAPHORE_COUNT 3
#define BUFFER_SIZE 256
#define STEP 1


typedef struct pthreadParameters {
    sem_t semaphore[SEMAPHORE_COUNT];
}pthreadParameters;


void destroyParameters(struct pthreadParameters* parameters) {
    sem_destroy(&parameters->semaphore[SEMAPHORE1]);
    sem_destroy(&parameters->semaphore[SEMAPHORE2]);
}


void printError(const char* prefix, int code) {
    if (prefix == NULL) {
        prefix = "error";
    }
    char buffer[BUFFER_SIZE];
    if (strerror_r(code, buffer, sizeof(buffer)) != 0) {
        strcpy(buffer, "(unable to generate error!)");
    }
    fprintf(stderr, "%s: %s\n", prefix, buffer);
}

void semInit(sem_t* sem, int pshared, unsigned value) {
    int errorCode = sem_init(sem, pshared, value);
    if (errorCode != SUCCESS) {
        printError("Unable to init sem : ", errorCode);
    }
}

void* printLines(struct pthreadParameters* parameters, char* message, int step) {
    if (parameters == NULL) {
        fprintf(stderr, "Param is NULL\n");
        return NULL;
    }

    for (int i = 0; i < NUMBER_OF_LINES; ++i) {
        sem_wait(&parameters->semaphore[(SEMAPHORE1 + step) % SEMAPHORE_COUNT]);
        printf("%s\n", message);
        sem_post(&parameters->semaphore[(SEMAPHORE2 + step) % SEMAPHORE_COUNT]);
    }
    return NULL;
}

void* threadFunction(void* param) {
    printLines((pthreadParameters*)param, "Child1", 0);
    pthread_exit(NULL);
}

void* threadFunction2(void* param) {
    printLines((pthreadParameters*)param, "Child2", STEP + STEP);
    destroyParameters(param);
    pthread_exit(NULL);
}

int main(int argc, char** argv) {
    pthread_t thread1;
    pthread_t thread2;
    pthreadParameters params;

    semInit(&params.semaphore[SEMAPHORE1], 0, 0);
    semInit(&params.semaphore[SEMAPHORE2], 0, 0);
    semInit(&params.semaphore[SEMAPHORE3], 0, 1);


    int errorCode = pthread_create(&thread1, NULL, threadFunction, &params);
    if (errorCode != 0) {
        printError("Unable to create thread", errorCode);
        destroyParameters(&params);
        return EXIT_FAILURE;
    }
    errorCode = pthread_create(&thread2, NULL, threadFunction2, &params);
    if (errorCode != 0) {
        printError("Unable to create thread", errorCode);
        pthread_cancel(thread1);
        destroyParameters(&params);
        return EXIT_FAILURE;
    }
    
    printLines(&params, "Parent", STEP);
    pthread_exit(NULL);
}

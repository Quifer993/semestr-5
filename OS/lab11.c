#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define NUMBER_OF_LINES 10
#define BUFFER_SIZE 256
#define NUMBER_OF_MUTEXES 3
#define ZERO_MUTEX 0
#define SECOND_MUTEX 2
#define STEP 1
#define SLEEP_TIME 300000
#define INIT_SUCCESS 0
#define INIT_FAILURE -1
#define PTHREAD_SUCCESS 0

typedef struct pthreadParameters {
    pthread_mutex_t mutexes[NUMBER_OF_MUTEXES];
}pthreadParameters;

void printError(const char *prefix, int code) {
    if (prefix == NULL) {
        prefix = "error";
    }
    char buffer[BUFFER_SIZE];
    if (strerror_r(code, buffer, sizeof(buffer)) != 0) {
        strcpy(buffer, "(unable to generate error!)");
    }
    fprintf(stderr, "%s: %s\n", prefix, buffer);
}

void lockMutex(pthread_mutex_t *mutex) {
    if (mutex == NULL){
        fprintf(stderr, "lock_mutex: mutex was NULL\n");
        return;
    }
    int errorCode = pthread_mutex_lock(mutex);
    if (errorCode != 0) {
        printError("Unable to lock mutex: ", errorCode);
    }
}

void unlockMutex(pthread_mutex_t *mutex) {
    if (mutex == NULL){
        fprintf(stderr, "lock_mutex: mutex was NULL\n");
        return;
    }
    int errorCode = pthread_mutex_unlock(mutex);
    if (errorCode != 0) {
        printError("Unable to unlock mutex", errorCode);
    }
}

void* printLines(struct pthreadParameters *parameters, int currentMutex, char *string){
    if (parameters == NULL){
        fprintf(stderr, "Param is NULL\n");
        return NULL;
    }

    for (int i = 0; i < NUMBER_OF_MUTEXES * NUMBER_OF_LINES; ++i) {
        if (currentMutex == ZERO_MUTEX){
            write(STDOUT_FILENO, string, strlen(string));
        }
        int nextMutex = (currentMutex + STEP) % NUMBER_OF_MUTEXES;
        lockMutex(&parameters->mutexes[nextMutex]);
        unlockMutex(&parameters->mutexes[currentMutex]);
        currentMutex = nextMutex;
    }
    return NULL;
}

void *secondPrint(void *param) {
    if (param == NULL){
        fprintf(stderr, "Param is NULL\n");
        return NULL;
    }
    struct pthreadParameters *parameters = (pthreadParameters *) param;
    lockMutex(&parameters->mutexes[SECOND_MUTEX]);
    printLines(parameters, SECOND_MUTEX, "Child\n");
    //unlockMutex(&parameters->mutexes[SECOND_MUTEX]);
    return NULL;
}

void destroyParameters(struct pthreadParameters *parameters, int count) {
    for (int i = 0; i < count; i++) {
        pthread_mutex_destroy(&parameters->mutexes[i]);
    }
}

int init_parameters(struct pthreadParameters *parameters) {
    pthread_mutexattr_t mutexAttrs;
    int errorCode = pthread_mutexattr_init(&mutexAttrs);
    if (errorCode != 0) {
        printError("Unable to init mutex attrs", errorCode);
        return INIT_FAILURE;
    }

    errorCode = pthread_mutexattr_settype(&mutexAttrs, PTHREAD_MUTEX_ERRORCHECK);
    if (errorCode != 0) {
        printError("Unable to init mutex attrs type", errorCode);
        pthread_mutexattr_destroy(&mutexAttrs);
        return INIT_FAILURE;
    }

    for (int i = 0; i < NUMBER_OF_MUTEXES; i++) {
        errorCode = pthread_mutex_init(&parameters->mutexes[i], &mutexAttrs);
        if (errorCode != 0) {
            pthread_mutexattr_destroy(&mutexAttrs);
            destroyParameters(parameters, i);
            return INIT_FAILURE;
        }
    }
    pthread_mutexattr_destroy(&mutexAttrs);
    return INIT_SUCCESS;
}

int main() {
    struct pthreadParameters parameters;
    int errorCode = init_parameters(&parameters);
    if (errorCode != INIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    lockMutex(&parameters.mutexes[ZERO_MUTEX]);

    pthread_t thread;
    errorCode = pthread_create(&thread, NULL, secondPrint, &parameters);
    if (errorCode != PTHREAD_SUCCESS) {
        printError("Unable to create thread", errorCode);
        unlockMutex(&parameters.mutexes[ZERO_MUTEX]);
        destroyParameters(&parameters, NUMBER_OF_MUTEXES);
        return EXIT_FAILURE;
    }

    while (1) {
        errorCode = pthread_mutex_trylock(&parameters.mutexes[SECOND_MUTEX]);
        if (errorCode == EBUSY) {
            break;
        }
        if (errorCode != 0){
            printError("Error trylocking mutex", errorCode);
            return EXIT_FAILURE;
        }
        errorCode = pthread_mutex_unlock(&parameters.mutexes[SECOND_MUTEX]);
        if (errorCode != 0){
            printError("Error unlocking mutex", errorCode);
            return EXIT_FAILURE;
        }
        usleep(SLEEP_TIME);
    }

    printLines(&parameters, ZERO_MUTEX, "Parent\n");
    unlockMutex(&parameters.mutexes[ZERO_MUTEX]);

    destroyParameters(&parameters, NUMBER_OF_MUTEXES);
    pthread_exit(NULL);
    return EXIT_SUCCESS;
}

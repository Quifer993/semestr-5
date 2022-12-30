#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define NUMBER_OF_LINES 10
#define BUFFER_SIZE 256
#define NUMBER_OF_MUTEXES 1
#define ZERO_MUTEX 0
#define SECOND_MUTEX 2
#define STEP 1
#define SLEEP_TIME 300000
#define INIT_SUCCESS 0
#define INIT_FAILURE -1
#define PTHREAD_SUCCESS 0
#define CHILD_OWNER 1
#define PARENT_OWNER 0



typedef struct pthreadParameters {
    pthread_mutex_t mutexes[NUMBER_OF_MUTEXES];
    pthread_cond_t stack_cond;
    int owner;
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

void* printLines(struct pthreadParameters *parameters, int currentMutex, char *string, int whoIsThread){
    if (parameters == NULL){
        fprintf(stderr, "Param is NULL\n");
        return NULL;
    }
    
    for (int i = 0; i < NUMBER_OF_LINES; ++i) {
        lockMutex(&parameters->mutexes[ZERO_MUTEX]);
        usleep((rand() % 1000) * 1000);
        while (parameters->owner != whoIsThread) {
            //printf("wait %i\n", whoIsThread);
            pthread_cond_wait(&parameters->stack_cond, &parameters->mutexes[ZERO_MUTEX]);
        }
        write(STDOUT_FILENO, string, strlen(string));
        parameters->owner = (parameters->owner + 1) % 2;

        int errorCode = pthread_cond_signal(&parameters->stack_cond);
        if (errorCode != 0) {
            printError("Signal to alarm not sended: ", errorCode);
            return NULL;
        }

        unlockMutex(&parameters->mutexes[ZERO_MUTEX]);
        //неопределнное поведение, если unlock -> signal
    }
    return NULL;
}

void destroyParameters(struct pthreadParameters* parameters, int count) {
    for (int i = 0; i < count; i++) {
        pthread_mutex_destroy(&parameters->mutexes[i]);
    }
    pthread_cond_destroy(&parameters->stack_cond);
}

void *secondPrint(void *param) {
    struct pthreadParameters *parameters =  (pthreadParameters *) param;
    printLines(parameters, ZERO_MUTEX, "Child\n", CHILD_OWNER);
    destroyParameters(parameters, NUMBER_OF_MUTEXES);
    pthread_exit(NULL);
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

    errorCode = pthread_cond_init(&parameters->stack_cond, NULL);
    if (errorCode != 0) {
        pthread_mutexattr_destroy(&mutexAttrs);
        destroyParameters(parameters, NUMBER_OF_MUTEXES);
        return INIT_FAILURE;
    }

    parameters->owner = PARENT_OWNER;

    return INIT_SUCCESS;
}

int main() {
    struct pthreadParameters parameters;
    int errorCode = init_parameters(&parameters);
    if (errorCode != INIT_SUCCESS) {
        return EXIT_FAILURE;
    }

    pthread_t thread;
    errorCode = pthread_create(&thread, NULL, secondPrint, &parameters);
    if (errorCode != PTHREAD_SUCCESS) {
        printError("Unable to create thread", errorCode);
        unlockMutex(&parameters.mutexes[ZERO_MUTEX]);
        destroyParameters(&parameters, NUMBER_OF_MUTEXES);
        return EXIT_FAILURE;
    }

    printLines(&parameters, ZERO_MUTEX, "Parent\n", PARENT_OWNER);

    
    pthread_exit(NULL);
    return EXIT_SUCCESS;
}

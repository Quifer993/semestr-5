#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <stdbool.h>
#include <math.h>


#define COUNT_THREAD 1000
#define STEP_WHILE_CHECK_SIG 1000000
#define OFFSET 3

static bool isSignal = false;
static bool isExitWhile = false;
static int loopIterationMax = 0;


struct ThreadParams {
    long int startPoint;
    long int stepSize;
    double sum;
    pthread_barrier_t* barrier;
} ThreadParams;


void signalHandler(int signum) {
    isSignal = true;
}


void* calculate(void* param){
    long int start = (long int)(((struct ThreadParams*)param)[0].startPoint);
    long int step = (long int)(((struct ThreadParams*)param)[0].stepSize);
    pthread_barrier_t* barrier = (pthread_barrier_t*)(((struct ThreadParams*)param)[0].barrier);
    double sum = 0;
    long int loopIteration = 1;
    long int i = start;
    bool isSignalLocal = false;

    while (!isSignalLocal ) {
        isSignalLocal = isSignal;
        if (isSignalLocal) { isExitWhile = true; }
        pthread_barrier_wait(barrier);
        if (isExitWhile) { isSignalLocal = true; }

        for (i; i < STEP_WHILE_CHECK_SIG * loopIteration; i +=step) {
            sum += 1.0 / (i * 4.0 + 1.0);
            sum -= 1.0 / (i * 4.0 + 3.0);
        }
        loopIteration++;
        if (loopIterationMax < loopIteration) {
            loopIterationMax = loopIteration;
        }

        pthread_barrier_wait(barrier);
    }

    ((struct ThreadParams*)param)[0].sum = sum;
    printf("%d - pid; %ld 000000 - iteration\n", gettid(), loopIteration);

    pthread_exit(NULL);
}


int main(int argc, char** argv) {
    signal(SIGINT, signalHandler);
    printf("%d : count of threads\n", COUNT_THREAD);

    pthread_t threads[COUNT_THREAD];
    double arrayBorder[COUNT_THREAD * OFFSET];
    struct ThreadParams params[COUNT_THREAD];
    int error_code = 0;
    int count_created_threads = COUNT_THREAD;
    double pi = 0;

    bool isEndLocal = false;
    pthread_barrier_t barrier;

    int status = pthread_barrier_init(&barrier, NULL, COUNT_THREAD);
    if (status != 0) {
        printf("main error: can't init barrier, status = %d\n", status);
        return EXIT_FAILURE;
    }
    isSignal = false;
    for(int i = 0; i < COUNT_THREAD; i++){
        params[i].startPoint = i;
        params[i].stepSize = COUNT_THREAD;
        params[i].sum = 0;
        //params[i].isSignal = &isEndLocal;
        params[i].barrier = &barrier;
    }

    for (int i = 0; i < COUNT_THREAD; ++i) {
        error_code = pthread_create(&threads[i], NULL, calculate, &params[i]);
        if (error_code != 0) {
            printf("Thread creation error: %s\n", strerror(error_code));
            count_created_threads = i;
            break;
        }
    }

    for (int j = 0; j < count_created_threads; ++j) {
        error_code = pthread_join(threads[j], NULL);
        if (error_code != 0) {
            printf("Thread join error: %s", strerror(error_code));
            return EXIT_FAILURE;
        }
        pi += params[j].sum;
    }

    pi = pi * 4.0;
    printf("pi done - %.15g \n", pi);
    pthread_barrier_destroy(&barrier);
    return (EXIT_SUCCESS);
}

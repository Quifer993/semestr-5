#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <malloc.h>

const long int NUM_STEPS = 3100100100;
#define COUNT_THREAD 32500//32768
#define OFFSET 3

struct ThreadParams {
    long int startPoint;
    long int finishPoint;
    double sum;
} ThreadParams;

void* calculate(void* param){
    long int start = (long int)(((struct ThreadParams*)param)[0].startPoint);
    long int finish = (long int)(((struct ThreadParams*)param)[0].finishPoint);
    double sum = 0;
    for (long int i = start; i < finish; i++) {
         sum += 1.0/(i*4.0 + 1.0);
         sum -= 1.0/(i*4.0 + 3.0);
    }
    ((struct ThreadParams*)param)[0].sum = sum;
    printf("%d\n", gettid());
    pthread_exit(NULL);
}


int main(int argc, char** argv) {
    long int countIteration = NUM_STEPS;
    printf("%d\n", COUNT_THREAD);
    if(argc != 1){
    	countIteration = strtol(argv[0], NULL,10);
    }
    printf("%ld\n", countIteration);
    printf("%ld ", countIteration);
    pthread_t threads[COUNT_THREAD];
    double arrayBorder[COUNT_THREAD * OFFSET];
    struct ThreadParams params[COUNT_THREAD];
    int error_code = 0;
    int count_created_threads = COUNT_THREAD;
    double pi = 0;


    for(int i = 0; i < COUNT_THREAD; i++){
	params[i].startPoint = countIteration * i / COUNT_THREAD;
	params[i].finishPoint = countIteration * (i + 1) / COUNT_THREAD;
	params[i].sum = 0;
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
    return (EXIT_SUCCESS);
}

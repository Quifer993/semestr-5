#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define SECONDS_WAITING 5 * 1000000
#define SECONDS_STEP 2 * 1000000
#define TRUE 1
#define ARG_IN_POP 0

void cancelCleanFunc(void* param){
    char* string = (char*)param;
    printf("Thread : %s\n", string);
}

int val = nullptr;
pthread_cleanup_push(free, val)
val = malloc()
kbalkbjlakjblk
pthread_cleanup_pop(1);

void* printLines(void* param) {
    pthread_cleanup_push(cancelCleanFunc, "Pthread was cancelled!");

    while (TRUE) {
        printf("Hello! I'm a %s\n", (char*)param);
	usleep(SECONDS_STEP);
    }

    pthread_cleanup_pop(ARG_IN_POP);
}

int main() {
    pthread_t thread;
    int errorCode = pthread_create(&thread, NULL, printLines, "Child");
    if (errorCode != 0) {
        printf("Thread creation error: %s", strerror(errorCode));
        return EXIT_FAILURE;
    }

    usleep(SECONDS_WAITING);
    printf("Parent: Trying to cancel child thread\n");

    errorCode = pthread_cancel(thread);
    if (errorCode != 0) {
        printf("Thread cancel error: %s", strerror(errorCode));
        return EXIT_FAILURE;
    }

    errorCode = pthread_join(thread, NULL);
    if(errorCode != 0){
        printf("Error if joining thread: %s", strerror(errorCode));
        return EXIT_FAILURE;
    }

    printf("Parent: Cancelled child thread\n");
    return 0;
}

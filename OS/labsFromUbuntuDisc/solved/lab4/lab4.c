#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#define MICROSEC_IN_SEC 1000000
#define SECONDS_WAITING 5 * MICROSEC_IN_SEC
#define SECONDS_STEP 4 * MICROSEC_IN_SEC
#define TRUE 1

void* printLines(void* param) {
    while (TRUE) {
        printf("Hello! I'm a %s\n", (char*)param);
	usleep(SECONDS_STEP);
    }
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

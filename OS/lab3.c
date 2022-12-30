#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define THREADS_NUMBER 4
#define ARRAY_LENGTH 4

void *printLines(void* strings) {
    char* lines = *(char **) strings;
    write(STDOUT_FILENO, lines, strlen(lines));
    return NULL;
}

int main() {
    int errorCode;
    pthread_attr_t tattr;
    errorCode = pthread_attr_init(&tattr);
        if (0 != errorCode) {
            printf("Attr not init: %d", errorCode);
	}

    errorCode = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
	if (0 != errorCode) {
            printf("Thread not set detachstate: %d", errorCode);
	}
    pthread_t threads[THREADS_NUMBER];
    char* numbers[ARRAY_LENGTH] = {"Child 1\n", "Child 2\n", "Child 3\n", "Child 4\n"};

    //int countCreatedThreads = THREADS_NUMBER;
    for (int i = 0; i < THREADS_NUMBER; ++i) {
        errorCode = pthread_create(&threads[i], &tattr, printLines, &numbers[i]);
        if (0 != errorCode) {
            printf("Thread not created: %d", errorCode);
            //countCreatedThreads = i;
            break;
        }
    }
    //for (int j = 0; j < count_created_threads; ++j) {
    //    errorCode = pthread_join(threads[j], NULL);
    //    if (0 != errorCode) {
    //        printf("Thread not joined: %d", errorCode);
    //        return EXIT_FAILURE;
    //    }
    //}
    printf("%s\n", "Parent Thread");
    pthread_exit(NULL);
 //   return EXIT_SUCCESS;
}

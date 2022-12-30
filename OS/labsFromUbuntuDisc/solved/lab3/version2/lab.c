#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#define THREADS_NUMBER 4
#define ARRAY_LENGTH 4

void *print_lines(void* strings) {
    char* lines = *(char **) strings;
    write(STDOUT_FILENO, lines, strlen(lines));
    return NULL;
}

int main() {
    pthread_t threads[THREADS_NUMBER];
    pthread_attr_t tattr;
    char* numbers[ARRAY_LENGTH] = {"Child 1\n", "Child 2\n", "Child 3\n", "Child 4\n"};
    int error_code;
    for (int i = 0; i < THREADS_NUMBER; ++i) {
        error_code = pthread_create(&threads[i], NULL, print_lines, &numbers[i]);
        if (0 != error_code) {
            printf("Thread not created: %d", error_code);
            break;
        }
    }
//    for (int j = 0; j < count_created_threads; ++j) {
//        error_code = pthread_join(threads[j], NULL);
//        if (0 != error_code) {
//            printf("Thread not joined: %d", error_code);
//            return EXIT_FAILURE;
//        }
//    }
    printf("%s\n", "Parent Thread");
    pthread_exit(NULL);
    return EXIT_SUCCESS;
}

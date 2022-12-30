#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <stdbool.h>

#include "list/list.h"

#define THREADS_NUMBER 4
#define SIZE_STRING 0
#define ARRAY_LENGTH 4
#define MAX_LINES 100
#define BUFFER_SIZE 100
#define PROPORTIONALITY_FACTOR 12000
#define TIMER 5*1000*1000


typedef struct pthreadParameters {
    pthread_mutex_t* mutex;
    List* list;
    char* str;
}pthreadParameters;


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


void lockMutex(pthread_mutex_t* mutex) {
    if (mutex == NULL) {
        fprintf(stderr, "lock_mutex: mutex was NULL\n");
        return;
    }
    int errorCode = pthread_mutex_lock(mutex);
    if (errorCode != 0) {
        printError("Unable to lock mutex: ", errorCode);
    }
}

void unlockMutex(pthread_mutex_t* mutex) {
    if (mutex == NULL) {
        fprintf(stderr, "lock_mutex: mutex was NULL\n");
        return;
    }
    int errorCode = pthread_mutex_unlock(mutex);
    if (errorCode != 0) {
        printError("Unable to unlock mutex", errorCode);
    }
}


int init_mutex(pthread_mutex_t* mutex) {
    pthread_mutexattr_t mutexAttrs;
    int errorCode = pthread_mutexattr_init(&mutexAttrs);
    if (errorCode != 0) {
        printError("Unable to init mutex attrs", errorCode);
        return EXIT_FAILURE;
    }

    errorCode = pthread_mutexattr_settype(&mutexAttrs, PTHREAD_MUTEX_ERRORCHECK);
    if (errorCode != 0) {
        printError("Unable to init mutex attrs type", errorCode);
        pthread_mutexattr_destroy(&mutexAttrs);
        return EXIT_FAILURE;
    }

    errorCode = pthread_mutex_init(mutex, &mutexAttrs);
    if (errorCode != 0) {
        pthread_mutexattr_destroy(&mutexAttrs);
        pthread_mutex_destroy(mutex);
        return EXIT_FAILURE;
    }

    pthread_mutexattr_destroy(&mutexAttrs);
    return EXIT_SUCCESS;
}



void* printLines(void* paramVoid) {
    pthreadParameters* params = (pthreadParameters*)paramVoid;
    char* line = params->str;
    pthread_mutex_t* mutex = params->mutex;
    List* list = params->list;
    usleep(PROPORTIONALITY_FACTOR * strlen(line));
    lockMutex(mutex);

    if (list->head == NULL) {
        insert_head(list, line);
        list->finishedNode = list->head;
    }
    else {
        insert_after(list->finishedNode, line);
        list->finishedNode = list->finishedNode->next;
    }

    unlockMutex(mutex);
    pthread_exit(NULL); 
}

//111222333444555666777888999101010111111121212131313141414151515161616171717181818191919202020212121222222232323
//12345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345

int main() {
    pthread_mutex_t mutex;
    if (init_mutex(&mutex) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    List list;
    list.head = NULL;

    int errorCode;
    char* lines[MAX_LINES];
    int countOfLines = 0;

    for (int i = 0; i < MAX_LINES; i++) {
        bool isNotEndString = true;
        int counter = SIZE_STRING;
        char* lineBefore;
        while (isNotEndString) {
            if (counter == SIZE_STRING) {
                lines[i] = (char*)realloc(NULL, BUFFER_SIZE + 1);
            }
            else {
                lineBefore = realloc(NULL, counter * BUFFER_SIZE);
                strncpy(lineBefore, lines[i], counter * BUFFER_SIZE);
                free(lines[i]);

                lines[i] = (char*)realloc(NULL, BUFFER_SIZE * (counter + 1) + 1);
                strncpy(lines[i], lineBefore, counter * BUFFER_SIZE);
                free(lineBefore);
                //printf("\n%s\n", lines[i]); 
            }

            char* fef = fgets(lines[i] + counter * BUFFER_SIZE, BUFFER_SIZE + 1, stdin);
            if (fef == NULL) {
                printf("Error fgets!\n");
            }
            //printf("%lu -length string\n", strlen(lines[i]));
            //printf("string - %s\n", lines[i]);
            if (strlen(lines[i]) == (BUFFER_SIZE) * (counter + 1) && lines[i][BUFFER_SIZE * (counter + 1) - 1] != '\n') {
                isNotEndString = true;
                counter++;
            }
            else {
                isNotEndString = false;
                counter = SIZE_STRING;
            }
            //printf("counter - %i\n", counter);
        }
        //printf("%lu - length string\n", strlen(lines[i]));

        if (!strcmp(lines[i], "\n")) {
            break;
        }
        countOfLines++;
    }

    pthread_t* threads = (pthread_t*)malloc(sizeof(pthread_t) * countOfLines);

    pthreadParameters* params = (pthreadParameters*)malloc(sizeof(pthreadParameters) * countOfLines);
    for (int i = 0; i < countOfLines; i++) {
        (params + i)->list = &list;
        (params + i)->str = lines[i];
        (params + i)->mutex = &mutex;
    }


    for (int i = 0; i < countOfLines; ++i) {
        errorCode = pthread_create(&threads[i], NULL, printLines, params + i);
        if (errorCode != 0) {
            printf("Thread not created: %d", errorCode);
            break;
        }
    }

    for (int j = 0; j < countOfLines; ++j) {
        errorCode = pthread_join(threads[j], NULL);
        if (errorCode != 0) {
            printf("Thread join error: %s", strerror(errorCode));
            lockMutex(&mutex);
            free_memory(&list);
            free(threads);
            free(params);
            unlockMutex(&mutex);
            return EXIT_FAILURE;
        }
    }
    pthread_mutex_destroy(&mutex);

    Node* node = list.head;

    while (node != NULL) {
        printf("%s", node->value);
        node = node->next;
    }
    free_memory(&list);
    free(threads);
    free(params);
    printf("\nParent Thread closed\n");
    pthread_exit(NULL);
}

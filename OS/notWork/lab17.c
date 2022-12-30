#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>
#include <assert.h>

#include "list.h"

#define THREADS_NUMBER 4
#define SMALL_STRING 0
#define ARRAY_LENGTH 4
#define MAX_LINES 100
#define BUFFER_SIZE 100
#define PROPORTIONALITY_FACTOR 10000
#define TIMER 3*1000*1000


typedef struct pthreadParameters {
    pthread_mutex_t* mutex;
    List* list;
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


void* sortLines(void* param) {
    struct pthreadParameters* parameters = (pthreadParameters*)param;
    List* list = parameters->list;
    pthread_mutex_t* mutex = parameters->mutex;

    while (true) {
        usleep(TIMER);
        lockMutex(mutex);
        Node* counterNode = list->head;
        int count = 0;
        while (counterNode != NULL) {
            count++;
            counterNode = counterNode->next;
        }
        //printf("%d - count of lines\n", count);

        if (count > 1) {
            Node* newhead = list->head;
            for (int i = 1; i < count; i++) {
                //Node* nodeBefore = list->head;
                //Node* nodeLast = nodeBefore->next;
                //Node* nodeNext = nodeLast->next;
                Node* node = (Node*)malloc(sizeof(Node));
                Node* saveEmptyNode = node;
                node->next = newhead;
                for (int j = 1; j < count- i + 1; j++) {
                    //printf("%i\n", j);
                    //printf("node:  %i\t", (int)node);
                    //printf("node 1 :  %i\t", (int)newhead);
                    //printf("node 2 :  %i\t", (int)newhead->next);
                    //printf("node 3 :  %i\t\n", (int)newhead->next->next);
                    //printf("node 3 :  %i\t", (int)newhead->next->next->next);
                    
                    //strcmp();
                    if (strlen(node->next->value) > strlen(node->next->next->value)) {
                        swap(node);//ок

                        //char* str = nodeLast->value;
                        //nodeLast->value = nodeBefore->value;
                        //nodeBefore->value = str;
                    }
                    //printf("node:  %i\t", (int)node);
                    //printf("node 1 :  %i\t", (int)newhead);
                    //printf("node 2 :  %i\t", (int)newhead->next);
                    //printf("node 3 :  %i\t\n", (int)newhead->next->next);
                    //nodeBefore = nodeBefore->next;
                    //nodeLast = nodeLast->next;
                    //nodeNext = nodeNext->next;
                    if (j == 1) {
                        newhead = node->next;
                        saveEmptyNode = node;
                    }

                    //else {
                        node = node->next;
                    //}
                }
                list->head = newhead;
                //printf("%s%s ++\n", list->head->value, newhead->value);
                //printf("%s%s --\n", list->head->next->value, newhead->next->value);
                //printf("%s%s ==\n", list->head->next->next->value, newhead->next->next->value);
                free(saveEmptyNode);
            }
            
        }
        //else if (count == 2) {
        //    node->next = list->head;

        //    if (strlen(list->head->value) > strlen(list->head->next->value)) {
        //        //printf("1\n");
        //        swap(node);
        //        //printf("1\n");
        //        //Node* headBefore = list->head;
        //        //list->head = list->head->next;
        //        //list->head->next = headBefore;
        //        list->head = node->next;
        //        //printf("%s , %s\n", list->head->value, list->head->next->value);
        //    }
        //}
        //printf("%s ++++\n", list->head->value);
        //printf("%s ----\n", list->head->next->value);
        //printf("%s ====\n", list->head->next->next->value);
        

        unlockMutex(mutex);
    }
    //free(node);


    pthread_exit(NULL);
}


int createSortingThread(pthreadParameters* params) {
    pthread_attr_t tattr;
    int errorCode = pthread_attr_init(&tattr);
    if (errorCode != 0) {
        printf("Attr not init: %d", errorCode);
        return EXIT_FAILURE;
    }

    errorCode = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    if (errorCode != 0) {
        printf("Thread not set detachstate: %d", errorCode);
        pthread_attr_destroy(&tattr);
        return EXIT_FAILURE;
    }
    pthread_t thread;

    errorCode = pthread_create(&thread, &tattr, sortLines, params);
    if (errorCode != 0) {
        printf("Thread not created: %d", errorCode);
        pthread_attr_destroy(&tattr);
        return EXIT_FAILURE;
    }
    pthread_attr_destroy(&tattr);
    return EXIT_SUCCESS;
}

//111222333444555666777888999101010111111121212131313141414151515161616171717181818191919202020212121222222232323
//12345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345
int main() {
    //check_error();
    pthread_mutex_t mutex;
    if (init_mutex(&mutex) == EXIT_FAILURE) {
        return EXIT_FAILURE;
    }
    List list;
    char* firstStringEmpty = (char*)realloc(NULL, sizeof(char) * 4);
    strncpy(firstStringEmpty, "\n", 2);
    //firstStringEmpty = "\n";
    insert_head(&list, firstStringEmpty);
    //printf("%s", list.head->value);

    int errorCode;
    //char* lines[MAX_LINES];
    pthreadParameters params;
    params.list = &list;
    params.mutex = &mutex;



    if (createSortingThread(&params) == EXIT_FAILURE) {
        free_memory(&list);
        return EXIT_FAILURE;
    }

    char* lines;
    for (int i = 0; i < MAX_LINES; i++) {
        bool isNotEndString = true;
        int counter = SMALL_STRING;
        char* lineBefore;
        while (isNotEndString) {
            if (counter == SMALL_STRING) {
                lines = (char*)realloc(NULL, BUFFER_SIZE + 1);
            }
            else {
                lineBefore = realloc(NULL, counter * BUFFER_SIZE);
                strncpy(lineBefore, lines, counter * BUFFER_SIZE);
                free(lines);

                lines = (char*)realloc(NULL, BUFFER_SIZE * (counter + 1) + 1);
                strncpy(lines, lineBefore, counter * BUFFER_SIZE);
                free(lineBefore);
                //printf("\n%s\n", lines[i]); 
            }

            char* fef = fgets(lines + counter * BUFFER_SIZE, BUFFER_SIZE + 1, stdin);
            if (fef == NULL) {
                printf("Error fgets!\n");
                pthread_mutex_destroy(&mutex);
                free_memory(&list);
                return EXIT_FAILURE;
            }
            //printf("%lu -length string\n", strlen(lines[i]));
            //printf("string - %s\n", lines[i]);
            if (strlen(lines) == (BUFFER_SIZE) * (counter + 1) && lines[BUFFER_SIZE * (counter + 1) - 1] != '\n') {
                isNotEndString = true;
                counter++;
            }
            else {
                isNotEndString = false;
                counter = SMALL_STRING;
            }
        }

        if (!strcmp(lines, "\n")) {
            Node* lastNode = list.head;
            lockMutex(&mutex);
            while (lastNode != NULL) {
                printf("%s", lastNode->value);
                lastNode = lastNode->next;
            }
            //print all strings in nodes
            free(lines);
            unlockMutex(&mutex);
            continue;
        }
        if (!strcmp(lines, "q\n")) {
            break;
        }
        lockMutex(&mutex);
        insert_after(list.head, lines);
        unlockMutex(&mutex);
    }

    lockMutex(&mutex);
    free_memory(&list);
    unlockMutex(&mutex);

    pthread_mutex_destroy(&mutex);


    return 0;
}

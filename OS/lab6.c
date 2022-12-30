#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <stdbool.h>

#define THREADS_NUMBER 4
#define SIZE_STRING 0
#define ARRAY_LENGTH 4
#define MAX_LINES 100
#define BUFFER_SIZE 100
#define PROPORTIONALITY_FACTOR 10000


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


void* printLines(void* strings) {
    char* lines = *(char**)strings;
    usleep(PROPORTIONALITY_FACTOR * strlen(lines));
    write(STDOUT_FILENO, lines, strlen(lines));
    free(lines);
    return NULL;
}
//111222333444555666777888999101010111111121212131313141414151515161616171717181818191919202020212121222222232323
//12345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345123451234512345
int main() {
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
            if (strlen(lines[i]) == (BUFFER_SIZE) * (counter+1) && lines[i][BUFFER_SIZE * (counter + 1) - 1] != '\n') {
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

    pthread_attr_t tattr;
    errorCode = pthread_attr_init(&tattr);
    if (errorCode != 0) {
        printf("Attr not init: %d", errorCode);
    }

    errorCode = pthread_attr_setdetachstate(&tattr, PTHREAD_CREATE_DETACHED);
    if (errorCode != 0) {
        printf("Thread not set detachstate: %d", errorCode);
    }
    pthread_t threads[MAX_LINES];

    for (int i = 0; i < countOfLines; ++i) {
        errorCode = pthread_create(&threads[i], &tattr, printLines, &lines[i]);
        //usleep(100);
        if (errorCode != 0) {
            printf("Thread not created: %d", errorCode);
            //countCreatedThreads = i;
            break;
        }
    }

    printf("%s\n", "Parent Thread closed\n");
    pthread_exit(NULL);
}

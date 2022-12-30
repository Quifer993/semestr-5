#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <malloc.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>


#define THREADS_NUMBER 4
#define SIZE_STRING 0
#define ARRAY_LENGTH 4
#define MAX_LINES 100
#define BUFFER_SIZE 100
#define PROPORTIONALITY_FACTOR 10000


typedef struct pthreadParameters {
    pthread_mutex_t* mutex;
    List* list;
}pthreadParameters;

//list
enum TypeError { OK, ERROR };


typedef struct Node {
    int value;
    struct Node* next;
}Node;

typedef struct List {
    Node* head;
}List;


int insert_head(List* list, int value) {
    Node* new_head = (Node*)malloc(sizeof(Node));
    if (new_head == NULL) {
        exit(ERROR);
    }

    new_head->next = list->head;
    list->head = new_head;

    new_head->value = value;
}


void remove_head(List* list) {
    Node* ptr = list->head->next;

    free(list->head);
    list->head = ptr;
}


void insert_after(Node* point, int value) {
    Node* new_el = (Node*)malloc(sizeof(Node));
    if (new_el == NULL) {
        exit(ERROR);
    }

    new_el->value = value;

    new_el->next = point->next;
    point->next = new_el;

}


void remove_after(Node* point) {
    Node* ptr = point->next->next;
    free(point->next);
    point->next = ptr;
}


void insert_list(Node* point, List* list) {
    if (list == NULL) {
        return;
    }

    Node* before = list->head;
    while (before->next != NULL) {
        before = before->next;
    }

    before->next = point->next;
    point->next = list->head;

    list->head = NULL;
}

void free_memory(List* list) {
    while (list->head != NULL) {
        remove_head(list);
    }

    free(list->head);
}

void check_error() {
    List list;
    list.head = NULL;

    //check fun insert_head
    insert_head(&list, 8);
    assert(list.head->value == 8);
    assert(list.head->next == NULL);

    insert_head(&list, 9);

    assert(list.head->value == 9);
    assert(list.head->next->next == NULL);

    //check fun remove_head
    remove_head(&list);

    assert(list.head->value == 8);
    assert(list.head->next == NULL);

    //check fun insert_after
    insert_after(list.head, 10);

    assert(list.head->value == 8);
    assert(list.head->next->value == 10);
    assert(list.head->next->next == NULL);

    //check fun remove_after
    remove_after(list.head);

    assert(list.head->value == 8);
    assert(list.head->next == NULL);

    //check fun insert_list
    insert_after(list.head, 10);

    Node* check_point = list.head->next;

    List list2;
    list2.head = NULL;

    insert_head(&list2, 1);
    insert_after(list2.head, 3);
    insert_after(list2.head, 2);

    insert_list(list.head->next, &list2);

    assert(list2.head == NULL);
    assert(check_point->value == 10);
    assert(check_point->next->value == 1);

    free_memory(&list);
}
///////mutex


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


void destroyMutex(struct pthreadParameters* parameters, int count) {
    for (int i = 0; i < count; i++) {
        pthread_mutex_destroy(&parameters->mutexes[i]);
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

    errorCode = pthread_mutex_init(&mutex, &mutexAttrs);
    if (errorCode != 0) {
        pthread_mutexattr_destroy(&mutexAttrs);
        pthread_mutex_destroy(&mutex);
        return EXIT_FAILURE;
    }

    pthread_mutexattr_destroy(&mutexAttrs);
    return EXIT_SUCCESS;
}

//////////////lines

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

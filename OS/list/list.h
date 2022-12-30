#include <malloc.h>
#include <stdlib.h>

enum TypeError { OK, ERROR };

typedef struct Node {
    char* value;
    struct Node* next;
}Node;

typedef struct List {
    Node* head;
    Node* finishedNode;
}List;


int insert_head(List* list, char* value);

void remove_head(List* list);

void insert_after(Node* point, char* value);

void remove_after(Node* point);

void insert_list(Node* point, List* list);

void free_memory(List* list);

void swap(Node* node);

#include "list.h"

int insert_head(List* list, char* value) {
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
    free(list->head->value);
    free(list->head);
    list->head = ptr;
}

void insert_after(Node* point, char* value) {
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
    free(point->value);
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

    //free(list->head);
}

void swap(Node* node) {
    Node* nodeNext = node->next;
    node->next = node->next->next;
    nodeNext->next = node->next->next;
    node->next->next = nodeNext;
}

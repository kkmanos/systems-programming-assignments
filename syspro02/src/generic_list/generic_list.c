#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "generic_list.h"

list_t *list_create(int (*data_cmp)(void *, void *), void (*data_print)(FILE *, void *)) {
    list_t *l = (list_t *) malloc(sizeof(list_t));
    l->head = NULL;
    l->data_cmp = data_cmp;
    l->data_print = data_print;
    l->size = 0;
    return l;
}

// Add a node in the beginning
list_node_t *list_insert(list_t *list, void *key, void *value) {
    if (key == NULL || value == NULL) {
        printf("==Error: There is no key or value to insert in list\n");
        return NULL;
    }
    /* Allocate node and initialize node */

    list_node_t *new_node = (list_node_t *) malloc(sizeof(list_node_t));
    // memcpy(new_node->key, new_data, data_size);
    new_node->key = key; // just copy the pointers
    new_node->value = value;
    /* since we are adding at the begining, prev is always NULL */
    new_node->prev = NULL;
    /* link the old list off the new node */
    new_node->next = list->head;
    /* change prev of head node to new node */
    if(list->head != NULL) {
        list->head->prev = new_node;
    }
    /* move the head to point to the new node */
    list->head = new_node;
    list->size++;
    return new_node;
}



void list_delete_node(list_t *list, list_node_t *del) { // just free the node (not the contents)
    /* base case */
    if(list->head == NULL || del == NULL)
        return;
    /* If node to be deleted is head node */
    if(list->head == del)
        list->head = del->next;
    /* Change next only if node to be deleted is NOT the last node */
    if(del->next != NULL)
        del->next->prev = del->prev;
    /* Change prev only if node to be deleted is NOT the first node */
    if(del->prev != NULL)
        del->prev->next = del->next;
    /* Finally, free the memory occupied by del*/
    free(del);
    list->size--;
}

list_node_t *list_search(list_t *list, void *searching_data) {
    list_node_t *temp = list->head;
    while(temp != NULL) {
        if(list->data_cmp(temp->key, searching_data) == 0 ) {
            return temp;
        } else {
            temp = temp->next;
        }
    }
    return NULL;
}


list_node_t *list_search_custom(list_t *list, void *searching_data, int (*eq)(void *, void *)) {
    list_node_t *temp = list->head;
    while(temp != NULL) {
        if(eq(temp->key, searching_data) == 0) {
            return temp;
        } else {
            temp = temp->next;
        }
    }
    return NULL;
}

void list_print(list_t *list, FILE *out) {
    if(list == NULL) return;

    list_node_t *temp = list->head;
    while(temp != NULL) {
        list->data_print(out, temp->value);
        temp = temp->next;
    }

}

void list_delete_list(list_t *list) {

    if(list == NULL) return;

    list_node_t *current = list->head;
    list_node_t *next;

    while(current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }
    list->head = NULL;

    free(list);
}


list_node_t *get_next(list_node_t* n) { return n->next; }


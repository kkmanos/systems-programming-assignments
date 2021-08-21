#ifndef GENERIC_LIST
#define GENERIC_LIST


#include <stdint.h>
#include <stdio.h>
#include "../record/record.h"
typedef uint8_t byte_t;



typedef struct list_node {
    struct list_node *prev;
    struct list_node *next;
    void* key; // pointer to hashed data. Its usually a member of the content.
                // This data only will be compared in cmp calls
    void* value; // pointer to full data a list node carries
} list_node_t;

typedef struct list {
    list_node_t *head;
    int size;
    int (*data_cmp)(void *, void *);
    void (*data_print)(FILE *, void *);
} list_t;

list_t *list_create(int (*data_cmp)(void *, void *), void (*data_print)(FILE *, void *)); 
list_node_t *list_insert(list_t *, void *, void *);
void list_delete_list(list_t *);
void list_delete_node(list_t *, list_node_t *);
void list_print(list_t *, FILE *out);
list_node_t *list_search(list_t *, void *);
list_node_t* get_next(list_node_t* n);

#endif

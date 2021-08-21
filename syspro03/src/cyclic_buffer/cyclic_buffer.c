#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "../mystring/mystring.h"
#include "cyclic_buffer.h"

cyclic_buf_t *cyclic_buf_create(int size) {
    cyclic_buf_t *cb = (cyclic_buf_t *) malloc(sizeof(cyclic_buf_t));
    cb->sz = size;
    cb->num_elements = 0;
    cb->front = cb->back = -1;
    cb->array = (char **) malloc(sizeof(char *) * size); // alloc an array of size pointers
    memset(cb->array, 0, cb->sz); // initialize all array elements with NULL
    return cb;
}

// returns 0 if cyclic_bufer is full
// return 1 if insertion is successfull
int cyclic_buf_insert(cyclic_buf_t *cb, char *elem) {
    if (cb->sz == cb->num_elements) 
        return 0;
    if (cb->front == -1) { 
        cb->front = cb->back = 0;
        cb->num_elements++;
        cb->array[cb->back] = string_create(elem);
        return 1;
    }
    if (cb->front != 0 && cb->back == cb->sz - 1) {
        cb->back = 0;
        cb->num_elements++;
        cb->array[cb->back] = string_create(elem);
        return 1;
    }
    cb->num_elements++;
    cb->back++;
    cb->array[cb->back] = string_create(elem);
    return 1;
}

// returns NULL if cyclic_buf is empty and therefore the removal failed
// returns pointer to element if succeded
char *cyclic_buf_pop(cyclic_buf_t *cb) {
    if (cb->front == -1) {
        return NULL;
    }

    char *topop = cb->array[cb->front];
    cb->array[cb->front] = NULL;
    if (cb->front == cb->back)
        cb->back = cb->front = -1;
    else if (cb->front == cb->sz - 1)
        cb->front = 0;
    else cb->front++;

    cb->num_elements--;
    return topop;
}

void cyclic_buf_destroy(cyclic_buf_t *cb) {
    char *elem;
    while ( (elem = cyclic_buf_pop(cb)) != NULL )
        free(elem);

    free(cb->array);
    free(cb);
}

int cyclic_buf_is_empty(cyclic_buf_t *cb) {
    return (cb->num_elements == 0);
}

int cyclic_buf_is_full(cyclic_buf_t *cb) {
    return (cb->num_elements == cb->sz);
}
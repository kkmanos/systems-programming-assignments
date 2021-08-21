
#ifndef CYCLIC_BUF_H
#define CYCLIC_BUF_H

typedef struct cyclic_buf_t {
	int sz;   // does not change
    int num_elements;
	int front;
	int back;
	char **array;  // the actual buffer holding the strings
} cyclic_buf_t;


cyclic_buf_t *cyclic_buf_create(int size);
int cyclic_buf_insert(cyclic_buf_t *cb, char *elem);
char *cyclic_buf_pop(cyclic_buf_t *cb);
void cyclic_buf_destroy(cyclic_buf_t *cb);
int cyclic_buf_is_empty(cyclic_buf_t *cb);
int cyclic_buf_is_full(cyclic_buf_t *cb);
#endif
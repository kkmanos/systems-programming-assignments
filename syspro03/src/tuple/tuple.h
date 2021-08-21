#ifndef _TUPLE_H
#define _TUPLE_H

// (x, y)
typedef struct tuple { // this struct is used as a key for the R hashtable
	char* x;
	char* y;
} tuple_t;

tuple_t* tuple_create(char* x, char* y);

void tuple_destroy(tuple_t* t);
int tuple_cmp(void *a, void *b);
#endif
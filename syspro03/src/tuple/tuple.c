#include <stdlib.h>
#include <string.h>
#include "tuple.h"
tuple_t* tuple_create(char* x, char* y) {
	tuple_t* t = (tuple_t*) malloc(sizeof(tuple_t));
	t->x = x;
	t->y = y;
	return t;
}

void tuple_destroy(tuple_t* t) { free(t); }

int tuple_cmp(void *a, void *b) { // comparing y keys
	char* a_x = ((tuple_t*)a)->x;
	char* b_x = ((tuple_t*)b)->x;
	char* a_y = ((tuple_t*)a)->y;
	char* b_y = ((tuple_t*)b)->y;

	// if x of the tuples has the field "y" equal to NULL
	// then only compare the "x" field
	if (a_y == NULL || b_y == NULL) // then compare only the id
		return strcmp(a_x, b_x);
	// if at least x of them are different then return != 0
	return (strcmp(a_x, b_x) || strcmp(a_y, b_y));
}
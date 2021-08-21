
#include <string.h>
unsigned int SOCK_BUFFER_SZ;
unsigned int BLOOM_SZ; 
unsigned int CYCLIC_BUFFER_SZ;
unsigned int NUM_THREADS;

void delete_new_line(char* str) { // delete new line character
	char* n_ptr = strstr(str, "\n");
	if (n_ptr != NULL) *n_ptr = '\0';
}

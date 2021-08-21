
unsigned int BUFFER_SZ;
unsigned int BLOOM_SZ;
#include <string.h>

void delete_new_line(char* str) { // delete new line character
	char* n_ptr = strstr(str, "\n");
	if (n_ptr != NULL) *n_ptr = '\0';
}
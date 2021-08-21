#ifndef _BLOOMF_H_
#define _BLOOMF_H_
#include <stdint.h>

typedef struct bf {
		int size;  // bloom filter size in Bytes
		uint8_t* bitarray;
		int K;
} bf_t;

bf_t* bf_create(int sz/* in bytes */);
bf_t* bf_copy(uint8_t *bitarr, unsigned int sz);
void bf_insert(bf_t* bloomfilter, char* element);
int bf_test(bf_t* bloomfilter, char* element);
void bf_destroy(bf_t* bloomfilter);
#endif
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "../hash_functions/hash_functions.h"
#include "bloom_filter.h"

// The struct must have been allocated before
// calling bf_initialize()
bf_t* bf_create(int sz/* in bytes */) {
	bf_t* bloomfilter = malloc(sizeof(bf_t));
	bloomfilter->bitarray = (uint8_t*)calloc(sz, sizeof(uint8_t));
	bloomfilter->size = sz;
	bloomfilter->K = 1;
	return bloomfilter;
}

bf_t* bf_copy(uint8_t *bitarr, unsigned int sz) {
	bf_t* bloomfilter = malloc(sizeof(bf_t));
	bloomfilter->bitarray = (uint8_t*)calloc(sz, sizeof(uint8_t));

	memcpy(bloomfilter->bitarray, bitarr, sz);
	bloomfilter->size = sz;
	bloomfilter->K = 1;
	return bloomfilter;
}

// sets the bit at the i-th position in arr
void set_bit(uint8_t* arr, int i) { arr[i/8] = arr[i/8] | (1 << (i % 8)); }

int test_bit(uint8_t* arr, int i) { return ((arr[i/8] & (1 << (i % 8) )) != 0); }

void bf_insert(bf_t* bloomfilter, char* element) {
	for (int i = 0; i < bloomfilter->K; i++)
		set_bit(bloomfilter->bitarray, hash_i((unsigned char*)element, i) % bloomfilter->size);
}

int bf_test(bf_t* bloomfilter, char* element) {
	// if there is at least one bit equal to 0, then doesnt exist
	for (int i = 0; i < bloomfilter->K; i++) { 
		if (!test_bit(bloomfilter->bitarray, hash_i((unsigned char*)element, i) % bloomfilter->size)) {
			//printf("Doesnt exist!\n");
			return 0;
		}
	}
	//printf("probably Exists!\n");
	return 1;
}


void bf_destroy(bf_t* bloomfilter) { // free the bitarray
	free(bloomfilter->bitarray);
	free(bloomfilter);
}


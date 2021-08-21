#ifndef _VIRUS_H
#define _VIRUS_H

#include "../bloom_filter/bloom_filter.h"

typedef struct virus {
    char *name;
    bf_t *bloom;
} virus_t;


virus_t *virus_create(char *name);
void virus_destroy(virus_t *v);
#endif
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "virus.h"
#include "../globals.h"

virus_t *virus_create(char *name) {
    virus_t *v = malloc(sizeof(virus_t));
    v->name = malloc(sizeof(char) * (strlen(name)+1));
    strcpy(v->name, name);
    v->bloom = bf_create(BLOOM_SZ);
    return v;
}

void virus_destroy(virus_t *v) {
    free(v->name);
    bf_destroy(v->bloom);
    free(v);
}
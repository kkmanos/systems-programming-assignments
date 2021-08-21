#ifndef _COUNTRY_H
#define _COUNTRY_H

#include "../bloom_filter/bloom_filter.h"
#include "../generic_list/generic_list.h"

extern unsigned int BLOOM_SZ;
typedef struct country {
    char   *name;
    list_t *viruseslist;
    list_t *R; // {key: (id, virus),  value: record_t* } hols all records for this country
} country_t;

country_t* country_create(char *name);

void country_insert_record(country_t *c, record_t *r);
void country_destroy(country_t *c);

#endif
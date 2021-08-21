
#include <stdlib.h>
#include <string.h>
#include "../bloom_filter/bloom_filter.h"
#include "country.h"
#include "../generic_list/generic_list.h"
#include "../virus/virus.h"
#include "../tuple/tuple.h"
#include "../monitor/monitor.h"
#include "../record/record.h"
#include "../globals.h"


int viruses_cmp(void * v1, void *v2) {
    return strcmp(v1, v2);
}

void virus_print(FILE * f, void *val) {
    virus_t *v = (virus_t *) val;
    fprintf(f, "virus name: %s\n", v->name);
}





void rec_print(FILE *out, void *v) {
	record_print(v);
}


country_t* country_create(char *name) {
    country_t *c = (country_t *) malloc(sizeof(country_t));
    c->name = (char *) malloc(sizeof(char) * (strlen(name)+1));
    strcpy(c->name, name);
    c->viruseslist = list_create(viruses_cmp, virus_print);
    c->R = list_create(tuple_cmp, rec_print);
    return c;
}

void country_insert_record(country_t *c, record_t *r) {
    tuple_t *t = tuple_create(r->citizen_id, r->virus_name);
    // insert at R list
    if (list_search(c->R, t) == NULL) { // if does not exist in R list
        list_insert(c->R, t, r); // insert {(id, name) : r}

        list_node_t *n = list_search(c->viruseslist, r->virus_name);
        if (n == NULL) { // if virus does not exist, create a virus node 
            n = list_insert(c->viruseslist, r->virus_name, virus_create(r->virus_name));
        }
        virus_t *v = (virus_t *) n->value;
        if (r->yN) // if is vaccinated
            bf_insert(v->bloom, r->citizen_id); // insert at bloom
    }
    else { // if already exists, delete the record
        record_destroy(r);
        tuple_destroy(t);
        //printf("Destroying record\n");
    }

}



void country_destroy(country_t *c) {
    free(c->name); // country is responsible for freeing the memory of name
    list_delete_list(c->viruseslist);

    for (list_node_t *rnode = c->R->head; rnode; rnode = rnode->next) {
        record_destroy(((record_t *)rnode->value));
    }
    list_delete_list(c->R);
    free(c);
}
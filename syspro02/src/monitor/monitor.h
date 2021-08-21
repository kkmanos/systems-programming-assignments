#ifndef MONITOR_H
#define MONITOR_H

#include "../generic_list/generic_list.h"
typedef struct monitor {
    list_t *countrieslist; // a list of {key: countryname, value: country_t* } 
    unsigned int accepted;
    unsigned int rejected;
    int readfd;
    int writefd;
} monitor_t;



monitor_t *monitor_create();

void monitor_insert_record(monitor_t *m, char *country, record_t *r);

char *monitor_get_date_vaccinated(monitor_t *m, char *id, char *virus, char *country);
int countries_cmp(void * c1, void *c2);
void country_print(FILE * f, void *val);

int monitor_search_vaccination_status(monitor_t *m, char *citizen_id);


void monitor_update_blooms(monitor_t *m, list_t *filelist, char *input_dir);


#endif
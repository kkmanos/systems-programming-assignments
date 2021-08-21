#ifndef MONITOR_H
#define MONITOR_H
#include <semaphore.h>
#include "../cyclic_buffer/cyclic_buffer.h"
#include "../generic_list/generic_list.h"
typedef struct monitor {
    list_t *countrieslist; // a list of {key: countryname, value: country_t* } 
    unsigned int accepted;
    unsigned int rejected;
    int readfd;
    int writefd;
} monitor_t;


record_t* parse_line(char *line, char *country);

monitor_t *monitor_create();

void monitor_insert_record(monitor_t *m, char *country, record_t *r);

char *monitor_get_date_vaccinated(monitor_t *m, char *id, char *virus, char *country);
int countries_cmp(void * c1, void *c2);
void country_print(FILE * f, void *val);

int monitor_search_vaccination_status(monitor_t *m, char *citizen_id);


void monitor_update_blooms(monitor_t *m, list_t *filelist, sem_t *empty, sem_t *mutex
                                        , sem_t *full, cyclic_buf_t *cb);

void monitor_set_socket(monitor_t *m, int sock);


int countries_cmp(void * c1, void *c2);

void country_print(FILE * f, void *val);

int filenames_cmp(void *f1, void *f2);

void int_to_str(char* buf, int num);  // result returned in buf

void reset_all_sig_flags();

#endif
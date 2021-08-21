#ifndef _REQUEST_H
#define _REQUEST_H

#include <stdint.h>

typedef struct request {
    unsigned int request_id;
    char *citizen_id;
    char *date;
    char *from; // country from
    char *to;
    char *virus;
    uint8_t accepted; // boolean for accepted or not
} request_t;



request_t *request_create(unsigned int r_id, char *id, char *date, char *from
                            , char *to, char *virus, uint8_t accepted);
void request_destroy(request_t *req);
int requests_cmp(void *v1, void *v2);




int in_last_six_months(char *end, char *date);
int in_date(char* date, char* date1, char* date2);
void date_reverse(char* src);

#endif
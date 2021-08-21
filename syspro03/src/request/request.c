#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "request.h"
int requests_cmp(void *v1, void *v2) {
    int intv1 = *((int *)v1);
    int intv2 = *((int *)v2);
    return intv1-intv2;
}


request_t *request_create(unsigned int r_id, char *id, char *date, char *from
                            , char *to, char *virus, uint8_t accepted) {
    request_t *r = malloc(sizeof(request_t));
    //r->request_id = malloc(sizeof(char)* (strlen(r_id) + 1));
    r->request_id = r_id;
    //strcpy(r->request_id, r_id);

    r->citizen_id = malloc(sizeof(char)* (strlen(id) + 1));
    strcpy(r->citizen_id, id);

    r->date = malloc(sizeof(char)* (strlen(date) + 1));
    strcpy(r->date, date);

    r->from = malloc(sizeof(char)* (strlen(from) + 1));
    strcpy(r->from, from);

    r->to = malloc(sizeof(char)* (strlen(to) + 1));
    strcpy(r->to, to);

    r->virus = malloc(sizeof(char)* (strlen(virus) + 1));
    strcpy(r->virus, virus);

    r->accepted = accepted;
    return r;
}

void request_destroy(request_t *req) {
    //free(req->request_id);
    free(req->citizen_id);
    free(req->date);
    free(req->from);
    free(req->to);
    free(req->virus);
    free(req);
}


void date_reverse(char* src) {
	char s[12];
    char* tokens[3];
    for (int i = 0; i < 3; i++)
        tokens[i] = malloc(sizeof(char)*6);
    int i = 0;
    for (char *p = strtok(src,"-"); p != NULL; p = strtok(NULL, "-"))
        strcpy(tokens[i++], p);
    strcpy(s, "");
    for (int i = 2; i >= 0; i--) {
        strcat(s, tokens[i]);
        if (i != 0) strcat(s, "-");
    }
	strcpy(src, s);

    for (int i = 0; i < 3; i++) free(tokens[i]);
}

int in_date(char* date, char* date1, char* date2) { // if date in [date1...date2]
	// converting DD-MM-YYYY format, into YYYY-MM-DD (in temporary strings)
	// which is easier to process
	char d1[12], d2[12], d[12];
	strcpy(d1, date1);
	strcpy(d2, date2);
	strcpy(d, date);
	date_reverse(d1);
	date_reverse(d2);
	date_reverse(d);
	return (strcmp(d2, d) >= 0 && strcmp(d, d1) >= 0); 
}

int in_last_six_months(char *end, char *date) {
	char day[3];
	char month[3];
	char year[5];
	memcpy(&day, end, 2);
	day[3] = '\0';

	int endD = atoi(day);

	memcpy(&month, end+3, 2);
	month[3] = '\0';
	int endM = atoi(month);


	memcpy(&year, end + 6, 5);
	int endY = atoi(year);	
	
	int startM = endM-6;
	char start[11];
	if (startM >= 1) {
		sprintf(start, "%02d-%02d-%4d", endD, startM, endY);
	}
	else {
		startM = startM + 12;
		sprintf(start, "%02d-%02d-%4d", endD, startM, endY-1);
	}
	return in_date(date, start, end);
}
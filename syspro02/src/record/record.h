#ifndef _REC_H_
#define _REC_H_
#include <stdint.h>

typedef struct record_t {
	char* citizen_id;
	char* first_name;
	char* last_name;
	char* country;
	uint8_t age;
	char* virus_name;
	uint8_t yN; // yes(1) or no(0) for vaccination
	char* date_vaccinated;
} record_t;


record_t* record_create(const char* citizen_id, const char* first_name, 
										const char* last_name,
										const char* country,
										uint8_t age,
										const char* virus_name,
										uint8_t yN,
										const char* date_vaccinated);
void record_print(record_t* rec);
void record_destroy(record_t* rec);
int isdate(char* str);

#endif
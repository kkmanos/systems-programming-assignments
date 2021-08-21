#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include "record.h"


record_t* record_create(const char* citizen_id, const char* first_name, 
										const char* last_name,
										const char* country,
										uint8_t age,
										const char* virus_name,
										uint8_t yN,
										const char* date_vaccinated) {

	// if NOT vaccinated, and a date appears, then cancel this record
	if (yN == 0 && date_vaccinated != NULL) {
		//printf("\n==ERROR IN RECORD %s\n", citizen_id);
		return NULL;
	}
	if (date_vaccinated != NULL && !isdate((char*)date_vaccinated)) // if this is not a correct date,
		return NULL; 			// then return error
	
	
	record_t* rec = malloc(sizeof(record_t));
	rec->citizen_id = malloc(sizeof(char) * (strlen(citizen_id) + 1));
	rec->first_name = malloc(sizeof(char) * (strlen(first_name) + 1));
	rec->last_name = malloc(sizeof(char) * (strlen(last_name) + 1));
	rec->country = malloc(sizeof(char) * (strlen(country) + 1));
	rec->virus_name = malloc(sizeof(char) * (strlen(virus_name) + 1));
	

	strcpy(rec->citizen_id, citizen_id);
	strcpy(rec->first_name, first_name);
	strcpy(rec->last_name, last_name);
	strcpy(rec->country, country);
	rec->age = age;
	strcpy(rec->virus_name, virus_name);
	rec->yN = yN;

	if (yN) { // if vaccinated, then
		rec->date_vaccinated = malloc(sizeof(char) * (strlen(date_vaccinated) + 1));
		strcpy(rec->date_vaccinated, date_vaccinated);

	}
	else rec->date_vaccinated = NULL;

	return rec;
}








void record_print(record_t* rec) {
	if (rec == NULL) return;
	if (rec->yN) {
		printf("%s %s %s %s %d", rec->citizen_id, rec->first_name,
										rec->last_name, rec->country, rec->age);
		printf(" %s YES %s\n", rec->virus_name, rec->date_vaccinated);
	}
	else {
		printf("%s %s %s %s %d", rec->citizen_id, rec->first_name,
										rec->last_name, rec->country, rec->age);
		printf(" %s NO\n", rec->virus_name);
	}
}


void record_destroy(record_t* rec) {
	if (rec == NULL) return;
	free(rec->citizen_id);
	free(rec->first_name);
	free(rec->last_name);
	free(rec->country);
	free(rec->virus_name);
	if (rec->yN == 1) // if vaccinated	
		free(rec->date_vaccinated);
	free(rec);
}

int isdate(char* str) { // simple function to define if str is a date
	for (int i = 0; i < strlen(str); i++)
		if (i != 2 && i != 5 && !isdigit(str[i])) // if not a dash and is not digit
			return 0;
	return (strlen(str) == 10 && str[2] == '-' && str[5] == '-');
}
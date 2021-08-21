#ifndef GLOBALS_H
#define GLOBALS_H
extern unsigned int BUFFER_SZ;
extern unsigned int BLOOM_SZ;

// monitor's receiving messages
#define REQUESTING_DATE_VACCINATED -10
#define REQUESTING_SEARCH_VACCINATION_STATUS -20
#define SHUT_DOWN -30


void delete_new_line(char* str);
#endif
#ifndef GLOBALS_H
#define GLOBALS_H
extern unsigned int SOCK_BUFFER_SZ;
extern unsigned int BLOOM_SZ;
extern unsigned int CYCLIC_BUFFER_SZ;
extern unsigned int NUM_THREADS;

// monitor's receiving messages
#define REQUESTING_DATE_VACCINATED -10
#define REQUESTING_SEARCH_VACCINATION_STATUS -20
#define REQUESTING_STATS -25
#define SHUT_DOWN -30
#define TERM -40
#define TERMINATE_SERVER -50
#define REQUESTING_BLOOMS_UPDATE -60



void delete_new_line(char* str);
#endif
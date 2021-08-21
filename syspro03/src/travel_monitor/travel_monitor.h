#ifndef TRAVEL_MON_H
#define TRAVEL_MON_H

#include <stdint.h>
#include "../generic_list/generic_list.h"
#include "../sockets/sockets.h"





typedef struct travel_monitor {
    list_t *blooms; // { key: (virus, country) , value: bf_t* }
    list_t *requests; // { key: request_id , value: request_t*  }
    connection_t **sockets_array;
    unsigned int sockets_array_sz;
} travel_monitor_t;


travel_monitor_t *travel_monitor_create(connection_t **sockets_array, unsigned int sockets_array_sz);
void travel_monitor_insert_bloom(travel_monitor_t *tm, char *virus, char *country, uint8_t *bitarray);

void travel_monitor_travel_request(travel_monitor_t *tm, char *id, char *date, char *from
                            , char *to, char *virus);
void travel_monitor_destroy(travel_monitor_t *tm);

void travel_monitor_travel_stats(travel_monitor_t *tm, char *virus_name, char *date1, char *date2, char *country);

void travel_monitor_search_vaccination_status(travel_monitor_t *tm, char *citizen_id);


void travel_monitor_update_blooms(travel_monitor_t *tm, char *countryname, connection_t *con);

void broadcast_string(travel_monitor_t *tm, char *msg);

void broadcast_int(travel_monitor_t *tm, int integer);




#endif
OBJS	= cyclic_buffer/cyclic_buffer.o mystring/mystring.o monitor/monitor.o sockets/sockets.o request/request.o globals.o record/record.o  virus/virus.o tuple/tuple.o generic_list/generic_list.o country/country.o  hash_functions/hash_functions.o bloom_filter/bloom_filter.o  dir_traversal/dir_traversal.o 
SOURCE	= cyclic_buffer/cyclic_buffer.c mystring/mystring.c monitor/monitor.c sockets/sockets.c request/request.c  globals.c record/record.c virus/virus.c tuple/tuple.c generic_list/generic_list.c country/country.c  hash_functions/hash_functions.c bloom_filter/bloom_filter.c  dir_traversal/dir_traversal.o

OBJS_TRAVEL = travelMonitorClient.o travel_monitor/travel_monitor.o $(OBJS)
SRC_TRAVEL = travelMonitorClient.c travel_monitor/travel_monitor.c $(SOURCE)

OBJS_MON = monitor/monitorServer.o $(OBJS)
SRC_MON	= monitor/monitorServer.c $(SOURCE)

HEADER = 

OUT	= travelMonitorClient monitorServer
LFLAGS	 = -g -lpthread
CC = gcc

all: travelMonitorClient monitorServer

travelMonitorClient: $(OBJS_TRAVEL)
	$(CC)  -o $@ $^ $(LFLAGS)

%.o: %.c $(HEADER)
	$(CC)  -c -o $@ $< $(LFLAGS)


monitorServer: $(OBJS_MON)
	$(CC) -o $@ $^ $(LFLAGS)


clean:
	rm -f $(OBJS_TRAVEL) $(OUT) $(OBJS_MON) log_file.* 

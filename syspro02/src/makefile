OBJS	= request/request.o globals.o record/record.o  virus/virus.o tuple/tuple.o generic_list/generic_list.o country/country.o  hash_functions/hash_functions.o bloom_filter/bloom_filter.o namedpipes/named_pipes.o dir_traversal/dir_traversal.o 
SOURCE	= request/request.c  globals.c record/record.c virus/virus.c tuple/tuple.c generic_list/generic_list.c country/country.c  hash_functions/hash_functions.c bloom_filter/bloom_filter.c namedpipes/named_pipes.c  dir_traversal/dir_traversal.o

OBJS_TRAVEL = travelMonitor.o travel_monitor/travel_monitor.o $(OBJS)
SRC_TRAVEL = travelMonitor.c travel_monitor/travel_monitor.c $(SOURCE)

OBJS_MON = monitor/monitor.o $(OBJS)
SRC_MON	= monitor/monitor.c $(SOURCE)

HEADER = 

OUT	= travelMonitor Monitor
LFLAGS	 = -g
CC = gcc

all: travelMonitor Monitor

travelMonitor: $(OBJS_TRAVEL)
	$(CC)  -o $@ $^ $(LFLAGS)

%.o: %.c $(HEADER)
	$(CC)  -c -o $@ $< $(LFLAGS)


Monitor: $(OBJS_MON)
	$(CC) -o $@ $^ $(LFLAGS)


clean:
	rm -f $(OBJS_TRAVEL) $(OUT) $(OBJS_MON) log_file.* /tmp/w.* /tmp/r.*
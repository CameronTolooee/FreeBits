#~#############################################~#
# Cameron Tolooee                               #
#~#############################################~#
CC=g++
CFLAGS=-c -std=c++11 -Wall #Wextra  #-Wpedantic #<---- deter hates these for some reason...
LDFLAGS=-lboost_regex 
SOURCES=conf_file_parser.cc manager.cc tracker.cc node.cc
EXECUTABLES=proj1 node tracker
all: $(EXECUTABLES) cleanup

no-cleanup: $(EXECUTABLES)

proj1: manager.o conf_file_parser.o connection.o wireformats.h
	$(CC) manager.o conf_file_parser.o connection.o -o $@ $(LDFLAGS)

node: node.o connection.o node_timer.o timers.o tools.o 
	$(CC) node.o connection.o node_timer.o timers.o tools.o -o $@ $(LDFLAGS)

tracker: tracker.o connection.o
	$(CC) tracker.o connection.o -o $@ $(LDFLAGS)

manager.o: manager.cc manager.h wireformats.h
	$(CC) $(CFLAGS) manager.cc 

node.o: node.cc node.h wireformats.h node_timer.o
	$(CC) $(CFLAGS) node.cc  

tracker.o: tracker.cc tracker.h wireformats.h
	$(CC) $(CFLAGS) tracker.cc

conf_file_parser.o: conf_file_parser.cc conf_file_parser.h
	$(CC) $(CFLAGS) conf_file_parser.cc

connection.o: connection.cc connection.h wireformats.h
	$(CC) $(CFLAGS) connection.cc

node_timer.o: node_timer.cc node_timer.h tools.o timers.o wireformats.h
	$(CC) $(CFLAGS) node_timer.cc 

clean: cleanup
	rm -rf *.o $(EXECUTABLES)

cleanup:
	rm -f *.out *-_csu_cs556_*.txt





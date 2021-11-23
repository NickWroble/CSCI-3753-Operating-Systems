# CS3753 - PA4

CC = gcc
CFLAGS = -c -g -Wall -Wextra
LFLAGS = -g -Wall -Wextra
SUBMITFILES = pager-lru.c pager-predict.c # add additional files for submission here

.PHONY: all clean

all: test-basic test-lru test-predict test-api

test-basic: simulator.o pager-basic.o
	$(CC) $(LFLAGS) $^ -o $@

test-lru: simulator.o pager-lru.o
	$(CC) $(LFLAGS) $^ -o $@

test-predict: simulator.o pager-predict.o
	$(CC) $(LFLAGS) $^ -o $@

test-api: simulator.o api-test.o
	$(CC) $(LFLAGS) $^ -o $@

simulator.o: simulator.c programs.c simulator.h
	$(CC) $(CFLAGS) $<

pager-basic.o: pager-basic.c simulator.h 
	$(CC) $(CFLAGS) $<

pager-lru.o: pager-lru.c simulator.h 
	$(CC) $(CFLAGS) $<

pager-predict.o: pager-predict.c simulator.h 
	$(CC) $(CFLAGS) $<

api-test.o:  api-test.c simulator.h
	$(CC) $(CFLAGS) $<

clean:
	rm -f test-basic test-lru test-predict test-api
	rm -f *.o
	rm -f *~
	rm -f *.csv
	rm -f *.pdf

submit: 
	@read -r -p "please enter your identikey username: " username; \
	tar -cvf PA4-$$username.txt $(SUBMITFILES)

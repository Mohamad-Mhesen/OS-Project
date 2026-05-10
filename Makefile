CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LIBS = -lraylib -lm -lpthread -ldl -lrt

.PHONY: all milestone1 milestone2 clean

all: milestone1 milestone2

milestone1: main.c
	$(CC) $(CFLAGS) -o dijkstra main.c $(LIBS)

milestone2: main.c
	$(CC) $(CFLAGS) -DMILESTONE2 -o sim main.c $(LIBS)

milestone3: main.c
	$(CC) $(CFLAGS) -DMILESTONE3 -o sim main.c $(LIBS)

clean:
	rm -f dijkstra sim

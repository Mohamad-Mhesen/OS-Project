CC = gcc
CFLAGS = -Wall -Wextra -std=c11
LIBS = -lraylib -lm -lpthread -ldl -lrt

.PHONY: all milestone1 milestone2 milestone3 milestone4 milestone5 clean

all: milestone1 milestone2 milestone3 milestone4 milestone5

milestone1: main.c
	$(CC) $(CFLAGS) -o dijkstra main.c $(LIBS)

milestone2: main.c
	$(CC) $(CFLAGS) -DMILESTONE2 -o sim main.c $(LIBS)

milestone3: main.c
	$(CC) $(CFLAGS) -DMILESTONE3 -o sim main.c $(LIBS)

milestone4: main.c
	$(CC) $(CFLAGS) -DMILESTONE4 -o sim main.c $(LIBS)

milestone5: main.c
	$(CC) $(CFLAGS) -DMILESTONE5 -o sim main.c $(LIBS)

milestone6: main.c
	$(CC) $(CFLAGS) -DMILESTONE6 -o sim main.c $(LIBS)
clean:
	rm -f dijkstra sim

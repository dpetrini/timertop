#Very simple Makefile for the beginning of the fun

CC = gcc
#CFLAGS = -g -Wall
CFLAGS = -Os -Wall
LIBS = -lncurses

timertop: list.o timertop.o
	$(CC) $(CFLAGS) -o timertop timertop.o list.o $(LIBS)

timertop.o: timertop.c timertop.h
	$(CC) $(CFLAGS) -c timertop.c

list.o: list.c list.h
	$(CC) $(CFLAGS) -c list.c

clean:
	rm -rf *.o core timertop

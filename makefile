#
# "makefile" for the CPU scheduler simulation.
#

CC=gcc
CFLAGS=-c -Wall -g

all: gentasks cpusched

gentasks.o: gentasks.c
	$(CC) $(CFLAGS) gentasks.c

cpusched.o: cpusched.c
	$(CC) $(CFLAGS) cpusched.c

gentasks: gentasks.o
	$(CC) gentasks.o -o gentasks

cpusched: cpusched.o
	$(CC) cpusched.o -o cpusched

clean:
	rm -rf *.o gentasks cpusched

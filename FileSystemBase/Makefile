CC=gcc
CFLAGS= -c --std=gnu99 -Wall -Wpedantic

all: simpleFS
	$(CC) main.o simpleFS.o -o simpleFS

simpleFS: main.c simpleFS_new.c
	$(CC) $(CFLAGS) main.c simpleFS_new.c

clean:
	rm *.o simpleFS

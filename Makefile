CC=gcc
CFLAGS=-c -g -Wall -Werror
LDFLAGS=-ljpeg -lm -pthread

all: mandel mandelmovie

mandel: mandel.o jpegrw.o
	$(CC) -o mandel mandel.o jpegrw.o $(LDFLAGS)
mandelmovie: mandelmovie.o 
	$(CC) -o mandelmovie mandelmovie.o -lm -pthread

clean:
	rm -f *.d *.o mandel mandelmovie *.jpg

%.c%.o: 
	$(CC) $(CFLAGS) $< -o $@
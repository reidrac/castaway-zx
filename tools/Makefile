BIN=bin2tap bas2tap ucl

all: $(BIN)

CC=gcc
CFLAGS=-s -O3 -Wall

%.o: %.c
	$(CC) $(CFLAGS) $< -o $@ -c

bin2tap: bin2tap.o
	$(CC) $< -o $@

bas2tap: bas2tap.o
	$(CC) -lm $< -o $@

ucl: ucl_src/ucl.c
	make -C ucl_src

clean:
	make -C clean ucl_src
	rm -f $(BIN) *.o


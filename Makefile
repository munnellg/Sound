OBJS = $(wildcard src/*)

CC = gcc

CFLAGS = -Wall -Wextra -Wpedantic

LDFLAGS = -D_REENTRANT -lpulse-simple -lpulse

BIN = sound

all : $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(BIN) $(LDFLAGS)
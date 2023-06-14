CC = gcc
CFLAGS = -Wall -Wextra -O3
LDFLAGS = -lraylib

all: main.exe

main.exe: main.c
	$(CC) $(CFLAGS) $(LDFLAGS) -o main.exe main.c

clean:
	del main.exe

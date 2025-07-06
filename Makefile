CC=gcc
CFLAGS=-Wall -Wextra -std=c11 -I. -L. -lm -lraylib -g

mzoom: main.c
	$(CC) -o $@ $< $(CFLAGS) 

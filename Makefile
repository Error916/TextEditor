CFLAGS=-Wall -Wextra -std=c11 -pedantic -ggdb #-O3
LIBS=-lSDL2 -lm
SRC=src/main.c src/la.c src/editor.c
CC=gcc

texteditor: $(SRC)
	$(CC) $(CFLAGS) -o texteditor $(SRC) $(LIBS)

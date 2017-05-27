CURSEBASE = $$HOME/ncurses

CC = gcc
CFLAGS = -std=c89 -I. -O0

DEPS = c51.h

OBJ = c51_s.o c51_c.o

c51_s : c51_s.c c51.h
	$(CC) $(CFLAGS) -o $@ c51_s.c

c51_c : c51_c.c c51.h
	$(CC) $(CFLAGS) -o $@ c51_c.c

.PHONY: clean

clean:
	rm -f c51_s
	rm -f c51_c
	rm -f *.o


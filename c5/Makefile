CURSEBASE = $$HOME/ncurses

CC = gcc
CFLAGS = -O3 -g -Werror -Wall -Wextra -std=c89 -pedantic

DEPS = c51.h

OBJ = c51_s.o c51_c.o

all : c51_s c51_c c54 c55 c56 c58_s c58_c


c51_s : c51_s.c c51.h errors.h
	$(CC) $(CFLAGS) -o $@ c51_s.c

c51_c : c51_c.c c51.h errors.h
	$(CC) $(CFLAGS) -o $@ c51_c.c

c54 : c54.c errors.h
	$(CC) $(CFLAGS) -lrt -lpthread -o $@ c54.c

c55 : c55.c errors.h
	$(CC) $(CFLAGS) -lrt -lpthread -o $@ c55.c

c56 : c56.c errors.h
	$(CC) $(CFLAGS) -lrt -lpthread -o $@ c56.c

c58_s : c58_s.c c51.h errors.h
	$(CC) $(CFLAGS) -lnsl -lsocket -o $@ c58_s.c

c58_c : c58_c.c c51.h errors.h
	$(CC) $(CFLAGS) -lnsl -lsocket -o $@ c58_c.c

.PHONY: clean

clean:
	rm -f c51_s c51_c c54 c55 c56 c58_s c58_c *.o 



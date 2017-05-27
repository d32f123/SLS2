CURSEBASE = $$HOME/ncurses

CC = gcc
CFLAGS = -std=c89 -I. -O0

DYNAMIC = -Bdynamic -fPIC
CURSEINC = -I"$(CURSEBASE)/include"
CURSES = -L"$(CURSEBASE)/lib" -lncursesw

DEPS = multibyte.h errors.h work_file.h cat.h

OBJ = multibyte.o work_file.o cat.c

less : less.o $(OBJ)
	$(CC) $(CFLAGS) $(DYNAMIC) -o $@ $^ $(CURSEINC) $(CURSES)

%.o : %.c $(DEPS)
	$(CC) $(CFLAGS) -c -o $@ $< $(CURSEINC)

.PHONY: clean

clean:
	rm -f less
	rm -f *.o


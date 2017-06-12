#ifndef _MULTIBYTE_H
#define _MULTIBYTE_H

#include <stdint.h>
#include <ncursesw/ncurses.h>
#include <wchar.h>

#define MAXBYTES 4

static union UTFchar_t 
{
    uint32_t fullChar;
    uint8_t partChar[MAXBYTES];
};

int readUTFchar(char * src, wint_t * dest);
int is_corrupt_UTF(char * src, int available_size);

#endif
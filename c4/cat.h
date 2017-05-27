#ifndef _WORK_FILE_H
#define _WORK_FILE_H

extern int files_opened;
extern int * fd_array;

#define CHUNK_BUFFER_SIZE (16*1024)
static char chunk_buffer[CHUNK_BUFFER_SIZE];

char * read_chunk(int fd, int * read_length);

#endif
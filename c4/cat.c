#include <stdlib.h>
#include <unistd.h>

#include "cat.h"
#include "errors.h"

char * read_chunk(int fd, int * read_length)
{
    ssize_t bytes_read = read(fd, chunk_buffer, CHUNK_BUFFER_SIZE);
    ssize_t prev_total_bytes = bytes_read;
    char istty = isatty(fd);
    /* TODO: error checking */
    if (bytes_read == -1)
    {
        panic_exit(fd_array, files_opened);
        perror("Read call failed");
        _exit(READ_FAILCODE);
    }
    if (!bytes_read) /* met eol, insert newline */
    {
        *read_length = bytes_read;
        return NULL;
    }
    if (istty && chunk_buffer[bytes_read - 1] == '\x04')
    {
        *read_length = bytes_read;
        return NULL;
    }
    if (istty && chunk_buffer[bytes_read - 1] != '\n')
    {
        while (chunk_buffer[prev_total_bytes - 1] != '\n' && chunk_buffer[prev_total_bytes - 1] != '\r' && chunk_buffer[prev_total_bytes - 1] != '\x04')
        {
            bytes_read = read(fd, chunk_buffer + prev_total_bytes, CHUNK_BUFFER_SIZE - prev_total_bytes);
            if (bytes_read == -1)
            {
                panic_exit(fd_array, files_opened);
                perror("Read call failed");
                _exit(READ_FAILCODE);
            }
            if (!bytes_read || (bytes_read == 1 && chunk_buffer[prev_total_bytes + bytes_read - 1] == '\x04')) /* met eol, insert newline */
            {
                *read_length = prev_total_bytes;
                return NULL;
            }
            prev_total_bytes += bytes_read;
        }
    }
    *read_length = prev_total_bytes;
    return &chunk_buffer[0];
}
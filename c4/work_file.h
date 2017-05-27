#ifndef _WORK_FILE_H
#define _WORK_FILE_H

#include "multibyte.h"
#include <wchar.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

#define INIT_BUFFER_CAP 8
#define INIT_LINE_META_CAP 8
#define DECODE_BUFFER_SIZE (16*1024)
#define BUFFER_SIZE DECODE_BUFFER_SIZE * 4

#define REGULAR_FILE 1
#define TTY_FILE 2
#define PIPE_FILE 4

#define TEXT_FILE 8
#define NONTEXT_FILE 16

extern int files_opened;
extern int * fd_array;

extern WINDOW * main_win;
extern WINDOW * cmd_win;
extern WINDOW * eof_win;

char decode_buffer[DECODE_BUFFER_SIZE];

struct line_meta
{
    wint_t * buffer;
    size_t buffer_offset;
    /* need to be careful about the end of buffer (i.e. buffer ended, but line did not, need to go to next buffer to finish the line) */
};

struct work_file
{
    wint_t ** buffer_pointer_arr;
    size_t buffer_pointer_arr_size;
    size_t buffer_pointer_arr_cap;

    struct line_meta * line_offset_map; /* line -> starting address */
    size_t line_offset_map_size;
    size_t line_offset_map_cap;

    struct line_meta * last_line;
    wint_t * iter;

    ssize_t current_line;
    char met_EOF;

    int fd;
    size_t file_offset;     /*  offset in bytes in the file */
    int file_type;

    ssize_t horizontal_scroll;


};

struct work_file * create_work_file(int fd);

int read_work_file(struct work_file * file, char to_EOL, wint_t tty_missed_char);

void print_work_file(struct work_file * file);

void gotoline_work_file(struct work_file * file, ssize_t line_number);

void readtoendof_work_file(struct work_file * file);
void gotoendof_work_file(struct work_file * file);


static void enable_control_char_output_mode();
static void disable_control_char_output_mode();

#endif
#include <ncursesw/ncurses.h>
#include "work_file.h"
#include "errors.h"

#define CHECK_BUFFER_SPACE() \
if (iter - buf_start  >= (BUFFER_SIZE) / sizeof(wint_t) - 1)\
{   /* no space left, make room for one more */\
    add_work_file_buffer(file);\
    *iter = L'\0';\
    iter = buf_start = file->buffer_pointer_arr[file->buffer_pointer_arr_size - 1];\
}

#define APPEND_LINE() \
if (file->line_offset_map_size >= file->line_offset_map_cap)\
{\
    add_work_file_line(file);\
}\
file->last_line = line = &(file->line_offset_map[file->line_offset_map_size++]);\
line->buffer = buf_start;\
line->buffer_offset = iter - buf_start;\
file->iter = iter;\
*iter = L'\0'; /* just in case newline is the last symbol */

void add_work_file_buffer(struct work_file * file)
{
    wint_t * buffer_array;
    wint_t ** buffer_pointer_arr = file->buffer_pointer_arr;
    if (file->buffer_pointer_arr_size >= file->buffer_pointer_arr_cap)
    {
        /* allocate more memory for array of pointers */
        buffer_pointer_arr = (wint_t **)realloc(file->buffer_pointer_arr, sizeof(wint_t *) * file->buffer_pointer_arr_cap * 2);
        if (!buffer_pointer_arr)
        {
            panic_exit(fd_array, files_opened);
            perror("Realloc call failed");
            _exit(OUTOFMEMORY_FAILCODE);
        }
        file->buffer_pointer_arr_cap *= 2;
    }
    buffer_array = (wint_t *)malloc(BUFFER_SIZE); /* allocate memory for buffer */
    if (!buffer_array)
    {
        panic_exit(fd_array, files_opened);
        perror("Malloc call failed");
        _exit(OUTOFMEMORY_FAILCODE);
    }
    buffer_pointer_arr[file->buffer_pointer_arr_size++] = buffer_array;
    file->buffer_pointer_arr = buffer_pointer_arr;
}

void add_work_file_line(struct work_file * file)
{
    struct line_meta * new_lines_array = (struct line_meta *)realloc(file->line_offset_map, sizeof(struct line_meta) * file->line_offset_map_cap * 2);
    if (!new_lines_array)
    {
        panic_exit(fd_array, files_opened);
        perror("Malloc call failed");
        _exit(OUTOFMEMORY_FAILCODE);
    }
    file->line_offset_map = new_lines_array;
    file->line_offset_map_cap *= 2;
}

struct work_file * create_work_file(int fd)
{
    struct work_file * file = (struct work_file *)malloc(sizeof(struct work_file));
    wint_t ** buffer_pointer_arr, * buffer_arr_temp;
    struct line_meta * line_offset_map;
    struct stat file_stat;
    int i;

    file->fd = fd;
    file->file_offset = NULL;
    
    buffer_arr_temp = (wint_t *)malloc(BUFFER_SIZE); /*64 KB buffer*/
    if (!buffer_arr_temp)
    {
        free(file);
        panic_exit(fd_array, files_opened);
        perror("Malloc call failed");
        _exit(OUTOFMEMORY_FAILCODE);
    }

    buffer_pointer_arr = (wint_t **)malloc(sizeof(wint_t *) * INIT_BUFFER_CAP);
    if (!buffer_pointer_arr)
    {
        free(buffer_arr_temp);
        free(file);
        panic_exit(fd_array, files_opened);
        perror("Malloc call failed");
        _exit(OUTOFMEMORY_FAILCODE);
    }

    buffer_pointer_arr[0] = buffer_arr_temp;
    for (i = 1; i < INIT_BUFFER_CAP; ++i)
    {
        buffer_pointer_arr[i] = NULL;
    }

    file->buffer_pointer_arr = buffer_pointer_arr;
    file->buffer_pointer_arr_cap = INIT_BUFFER_CAP;
    file->buffer_pointer_arr_size = 1;

    line_offset_map = (struct line_meta *)malloc(sizeof(struct line_meta) * INIT_LINE_META_CAP);
    if (!line_offset_map)
    {
        free(buffer_arr_temp);
        free(buffer_pointer_arr);
        free(file);
        panic_exit(fd_array, files_opened);
        perror("Malloc call failed");
        _exit(OUTOFMEMORY_FAILCODE);
    }
    
    file->line_offset_map = line_offset_map;
    file->line_offset_map_cap = INIT_LINE_META_CAP;
    file->line_offset_map_size = 0;

    if (fstat(fd, &file_stat) == -1)
    {
        free(buffer_arr_temp);
        free(buffer_pointer_arr);
        free(file);
        free(line_offset_map);
        panic_exit(fd_array, files_opened);
        perror("fstat call failed");
        _exit(FSTAT_FAILCODE);
    }
    if (isatty(fd))
        file->file_type = TTY_FILE | TEXT_FILE;
    else if (S_ISFIFO(file_stat.st_mode))
        file->file_type = PIPE_FILE;
    else
        file->file_type = REGULAR_FILE;

    if (file->file_type & REGULAR_FILE)
    {
        file->file_type |= get_work_file_format(file);
    }
    file->current_line = 0;
    file->met_EOF = 0;

    file->horizontal_scroll = 0;
    
    return file;
}

int get_work_file_format(struct work_file * file)
{
    char buffer[512];
    int i;
    wint_t ch;
    ssize_t bytes_read = pread(file->fd, buffer, 512, 0); /* read chars into decoding buffer and check for errors */
    if (bytes_read == -1)
    {
        panic_exit(fd_array, files_opened);
        perror("Read call failed");
        _exit(READ_FAILCODE);
    }
    if (!bytes_read)
    {
        return NONTEXT_FILE;
    }
    
    i = 0;
    while (i < bytes_read)
    {
        int char_size;
        if (i > 512 - 4)
        {
            int byte_leak;
            if ((byte_leak = is_corrupt_UTF(buffer + i, 512 - i)) > 0)
            {
                if (byte_leak == 512 - i)
                    return TEXT_FILE;
                return NONTEXT_FILE;
            }
        }
        char_size = readUTFchar(buffer + i, &ch);
        /* if an error occured while decoding */
        if (char_size <= 0)
            return NONTEXT_FILE;
        /* if at the end of buffer and some symbol which might be decoded, but not enough info */
        if (ch < 32 && ch >= 0 && ch != '\n' && ch != '\r')
            return NONTEXT_FILE;
        i += char_size;
    }
    return TEXT_FILE;
}

/* return codes:
    -1: something bad happened
     0: could not find EOL, file pointer was moved appropriately
     1: EOL found, file pointer was moved appropriately */
int read_to_end_of_line(struct work_file * file)
{
    off_t curr_offset;
    ssize_t bytes_read;
    ssize_t total_bytes = bytes_read;
    ssize_t to_eol = 0;
    wint_t * buf_start;
    wint_t * iter;
    struct line_meta * line;
    char edge_buffer[4] = { 0, 0, 0, 0 };    
    char canonical = (file->file_type & TTY_FILE) && (file->file_type & TEXT_FILE);
    char is_pipe = file->file_type & PIPE_FILE;
    int i = 0;
    

    if (file->line_offset_map_size == 0)
    {
        /* initiate line mapping */
        file->line_offset_map[0].buffer = file->buffer_pointer_arr[0];
        file->line_offset_map[0].buffer_offset = 0;
        file->line_offset_map_size = 1;
        file->last_line = line = &(file->line_offset_map[0]);
        buf_start = line->buffer;
        file->iter = iter = line->buffer + line->buffer_offset;
    } else
    {
        line = file->last_line;
        buf_start = line->buffer;
        iter = file->iter;
    }

    if (canonical)
    {
        noraw();
        nocbreak();
        bytes_read = read(file->fd, decode_buffer, DECODE_BUFFER_SIZE);
        cbreak();
        raw();
    }
    else if (!is_pipe)
    {
        curr_offset = lseek(file->fd, 0, SEEK_CUR);
        bytes_read = pread(file->fd, decode_buffer, DECODE_BUFFER_SIZE, curr_offset);
    }
    else
    {
        while (1)
        {
            wint_t ch = L'\0';
            int j = 1, char_size;
            bytes_read = read(file->fd, &edge_buffer[0], 1);

            if (bytes_read == -1)
            {
                panic_exit(fd_array, files_opened);
                perror("Read call failed");
                _exit(READ_FAILCODE);
            }
            if (!bytes_read) /* met eol, insert newline */
            {
                *(iter++) = L'\0';
                APPEND_LINE()
                return -1;
            }

            while (mbrtowc(&ch, edge_buffer, 4, NULL) == -2 && j < 4)
            {
                read(file->fd, &edge_buffer[j++], 1);
                if (bytes_read == -1)
                {
                    panic_exit(fd_array, files_opened);
                    perror("Read call failed");
                    _exit(READ_FAILCODE);
                }
                if (!bytes_read) /* met eol, insert newline */
                {
                    *(iter++) = L'\0';
                    APPEND_LINE()
                    return -1;
                }
            }
            char_size = readUTFchar(decode_buffer + i, &ch);
            switch(ch)
            {
                case L'\x0A':
                    *(iter++) = L'\0';
                    APPEND_LINE()
                    i += char_size;
                    if (!canonical)
                        lseek(file->fd, i, SEEK_CUR);
                    return 1;
                case L'\r':
                    i += char_size;
                    continue;
            }
            *(iter++) = ch;
            i += char_size;
        }
    }

    if (bytes_read == -1)
    {
        panic_exit(fd_array, files_opened);
        perror("Read call failed");
        _exit(READ_FAILCODE);
    }
    if (!bytes_read) /* met eol, insert newline */
    {
        *(iter++) = L'\0';
        APPEND_LINE()
        return -1;
    }

    while (i < bytes_read) /* decoding symbols (from UTF-8 to wint utf-8) */
    {
        int char_size;
        wint_t ch;
        
        CHECK_BUFFER_SPACE()

        /* check if we are on the edge of the decode bufer */
        if (i > DECODE_BUFFER_SIZE - 4)
        {
            int byte_leak;
            if ((byte_leak = is_corrupt_UTF(decode_buffer + i, DECODE_BUFFER_SIZE - i)) > 0)
            {
                int j, k = 0;
                for (j = i; j < DECODE_BUFFER_SIZE; ++j)
                    edge_buffer[k++] = decode_buffer[j];
                if (!canonical && !is_pipe)
                    total_bytes += pread(file->fd, edge_buffer + (DECODE_BUFFER_SIZE - i), byte_leak, curr_offset + bytes_read);
                else
                    total_bytes = read(file->fd, edge_buffer + (DECODE_BUFFER_SIZE - i), byte_leak);
                if (total_bytes - bytes_read == -1)
                {
                    panic_exit(fd_array, files_opened);
                    perror("Read call failed");
                    _exit(READ_FAILCODE);
                }
                if (total_bytes == bytes_read)
                {
                    if (!canonical && !is_pipe)
                        lseek(file->fd, total_bytes, SEEK_CUR);
                    return -1;
                }
                char_size = readUTFchar(edge_buffer, &ch);
            }
            else
                char_size = readUTFchar(decode_buffer + i, &ch);
        }
        else if (file->file_type & TEXT_FILE)
            char_size = readUTFchar(decode_buffer + i, &ch);
        else
        {
            char_size = 1;
            ch = (wint_t)decode_buffer[i];
        }
        if (!char_size)
            ++char_size;
        switch(ch)
        {
            case L'\x0A':
                *(iter++) = L'\0';
                APPEND_LINE()
                i += char_size;
                if (!canonical)
                    lseek(file->fd, i, SEEK_CUR);
                return 1;
            case L'\r':
                i += char_size;
                continue;
        }
        *(iter++) = ch;
        i += char_size;
    }
    *iter = L'\0';
    file->iter = iter;
    return 0;
}

int read_work_file(struct work_file * file, char to_EOL, wint_t tty_missed_char)
{
    int i; /* characters until eol */
    ssize_t bytes_read = read(file->fd, decode_buffer, DECODE_BUFFER_SIZE); /* read chars into decoding buffer and check for errors */
    wint_t * buf_start;
    wint_t * iter;
    struct line_meta * line;
    char EOL_is_last_char = 0;
    char edge_buffer[4] = { 0, 0, 0, 0 };    
                            /* 
                             * when on the edge of a decode buffer
                             * it is possible that we have a corrupted char (we did not read it fully)
                             * so read it to the end and put it here
                             */

    if (bytes_read == -1)
    {
        panic_exit(fd_array, files_opened);
        perror("Read call failed");
        _exit(READ_FAILCODE);
    }
    

    if (file->line_offset_map_size == 0)
    {
        /* initiate line mapping */
        file->line_offset_map[0].buffer = file->buffer_pointer_arr[0];
        file->line_offset_map[0].buffer_offset = 0;
        file->line_offset_map_size = 1;
        file->last_line = line = &(file->line_offset_map[0]);
        buf_start = line->buffer;
        file->iter = iter = line->buffer + line->buffer_offset;
    } else
    {
        line = file->last_line;
        buf_start = line->buffer;
        iter = file->iter;
    }

    i = 0;

    CHECK_BUFFER_SPACE()

    if (!bytes_read) /* met eol, insert newline */
    {
        if (tty_missed_char)
        {
            *(iter++) = tty_missed_char;
            CHECK_BUFFER_SPACE()
        }
        *(iter++) = L'\0';
        EOL_is_last_char = 1;
        APPEND_LINE()
        return -1;
    }

    if (tty_missed_char)
    {
        *(iter++) = tty_missed_char;
    }

    while (i < bytes_read) /* decoding symbols (from UTF-8 to wint utf-8) */
    {
        int char_size;
        wint_t ch;
        
        CHECK_BUFFER_SPACE()

        /* check if we are on the edge of the decode bufer */
        if ((file->file_type & TEXT_FILE) && i > DECODE_BUFFER_SIZE - 4)
        {
            int byte_leak;
            if ((byte_leak = is_corrupt_UTF(decode_buffer + i, DECODE_BUFFER_SIZE - i)) > 0)
            {
                int j, k = 0;
                for (j = i; j < DECODE_BUFFER_SIZE; ++j)
                    edge_buffer[k++] = decode_buffer[j];
                read(file->fd, edge_buffer + (DECODE_BUFFER_SIZE - i), byte_leak);
                if (bytes_read == -1)
                {
                    panic_exit(fd_array, files_opened);
                    perror("Read call failed");
                    _exit(READ_FAILCODE);
                }
                if (!bytes_read)
                {
                    /* corrupted symbol ? */
                    return -1;
                }
                char_size = readUTFchar(edge_buffer, &ch);
            }
            else
                char_size = readUTFchar(decode_buffer + i, &ch);
        }
        else if (file->file_type & TEXT_FILE)
            char_size = readUTFchar(decode_buffer + i, &ch);
        else
        {
            char_size = 1;
            ch = (wint_t)decode_buffer[i];
        }
        if (!char_size)
            ++char_size;
        if (file->file_type & TTY_FILE && ch == erasechar())
        {
            if (iter > line->buffer + line->buffer_offset)
                --iter;
            i += char_size;
            continue;
        }
        switch(ch)
        {
            case L'\x03': /* ^C */
            case L'\x04': /* ^D */
                if (file->file_type & TTY_FILE)
                {
                    file->met_EOF = 1;
                    *(iter++) = L'\0';
                    APPEND_LINE()
                    return -1;
                }
                break;
            case L'\x0A':
                *(iter++) = L'\0';
                EOL_is_last_char = 1;
                APPEND_LINE()
                i += char_size;
                if (to_EOL)
                    return 0;
                continue;
            case L'\r':
                i += char_size;
                continue;
            default:
                EOL_is_last_char = 0;
        }
        *(iter++) = ch;
        i += char_size;
    }
    if (!EOL_is_last_char && (file->file_type & NONTEXT_FILE))
    {
        APPEND_LINE();
    }
    else if (!EOL_is_last_char)
    {
        int ret;
        /* we need to read till eol */
        file->iter = iter;
        do
        {
            ret = read_to_end_of_line(file);
        } while(ret == 0);
    }
    return 0;
}

void print_work_file(struct work_file * file)
{
    int cols, lines;
    int current_line;
    int i;
    wint_t cc = L'^';

    if (file->current_line < 0)
        file->current_line = 0;
    current_line = file->current_line;
    if (file->horizontal_scroll < 0)
        file->horizontal_scroll = 0;

    getmaxyx(main_win, lines, cols);
    
    wclear(main_win);
    wclear(eof_win);

    if (!file->met_EOF && !(file->file_type & TTY_FILE) && current_line + lines >= file->line_offset_map_size - 1)
    {
        if (read_work_file(file, 0, NULL))
            file->met_EOF = 1;
    }
    if (current_line >= file->line_offset_map_size - 2) /* if we met EOF, we cannot fill all the terminal */
    {
        current_line = file->line_offset_map_size - 3;
        if (current_line < 0)
            current_line = 0;
        file->current_line = current_line;
        if (file->met_EOF)
        {
            wattr_on(eof_win, A_STANDOUT, NULL);
            mvwaddwstr(eof_win, 0, 0, L"EOF");
            wattr_off(eof_win, A_STANDOUT, NULL);
        }
    }

    wrefresh(eof_win);

    for (i = 0; i < lines; ++i)
    {
        if (current_line + i >= file->line_offset_map_size - 2)
        {
            wattr_on(main_win, A_BOLD, NULL);
            mvwaddwstr(main_win, i, 0, L"~");
            wattr_off(main_win, A_BOLD, NULL);
        }
        else
        {
            struct line_meta line = file->line_offset_map[current_line + i];
            struct line_meta nextline = file->line_offset_map[current_line + i + 1]; /* to find the border of the prev one */
            char line_is_divided = line.buffer != nextline.buffer && nextline.buffer_offset != 0; /* if line spans two buffers */
            ssize_t curr_char_pos = file->horizontal_scroll, curr_win_col = 0; 
            ssize_t line_length = (line_is_divided ? BUFFER_SIZE / sizeof(wint_t) - line.buffer_offset - 1: nextline.buffer_offset - line.buffer_offset - 1);

            while (curr_win_col < cols && curr_char_pos < line_length)
            {
                wchar_t curr_char = line.buffer[line.buffer_offset + (curr_char_pos++)];
                if (curr_char >= 0 && curr_char < 32)
                {
                    
                    enable_control_char_output_mode();
                    mvwaddnwstr(main_win, i, curr_win_col++, &cc, 1);
                    curr_char += L'A' - 1;
                    mvwaddnwstr(main_win, i, curr_win_col++, &curr_char, 1);
                    disable_control_char_output_mode();
                }
                else
                {
                    mvwaddnwstr(main_win, i, curr_win_col++, &curr_char, 1);
                }
            }
            if (line_is_divided)
            {
                int k = 0;
                line_length = nextline.buffer_offset - 1;
                while (curr_win_col < cols && k < line_length)
                {
                    wint_t curr_char = nextline.buffer[k++];
                    if (curr_char >= 0 && curr_char < 32)
                    {
                        enable_control_char_output_mode();
                        mvwaddnwstr(main_win, i, curr_win_col++, &cc, 1);
                        curr_char += L'A';
                        mvwaddnwstr(main_win, i, curr_win_col++, &curr_char, 1);
                        disable_control_char_output_mode();
                    }
                    else
                    {
                        mvwaddnwstr(main_win, i, curr_win_col++, &curr_char, 1);
                    }
                }
            }
        }
    }
    wrefresh(main_win);
    wrefresh(cmd_win);
}

void enable_control_char_output_mode()
{
    if(has_colors() == FALSE)
    {
        wattr_on(main_win, A_BLINK, NULL);
    }
    else
    {
        wattr_on(main_win, COLOR_PAIR(1), NULL);
    }
}

void disable_control_char_output_mode()
{
    if(has_colors() == FALSE)
    {
        wattr_off(main_win, A_BLINK, NULL);
    }
    else
    {
        wattr_off(main_win, COLOR_PAIR(1), NULL);
    }
}

void gotoline_work_file(struct work_file * file, ssize_t line_number)
{
    if (line_number < 0)
        line_number = 0;
    while (!file->met_EOF && line_number >= file->line_offset_map_size)
    {
        if (read_work_file(file, 0, NULL))
        {
            file->met_EOF = 1;
            break;
        }
    }
    file->current_line = line_number;
    print_work_file(file);
}

void readtoendof_work_file(struct work_file * file)
{
    if (file->met_EOF)
        return;
    while (read_work_file(file, 0, NULL) != -1);
    file->met_EOF = 1;
}

void gotoendof_work_file(struct work_file * file)
{
    readtoendof_work_file(file);
    file->current_line = file->line_offset_map_size - 1;
    
    print_work_file(file);
}
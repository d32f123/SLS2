#include <ncursesw/ncurses.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <locale.h>
#include <termios.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "multibyte.h"
#include "errors.h"
#include "work_file.h"
#include "cat.h"

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

int files_opened;
int * fd_array;

struct termios sh_attr;     /* shell terminal attributes (default) */
struct termios prog_attr;

SCREEN * term;

WINDOW * main_win;
WINDOW * cmd_win;
WINDOW * eof_win;

int stdin_temp;
int stdout_temp;

char * is_stdin;
char simulate_cat = 0; /* if stdout is not a tty, then just simulate cat (no interactive element) */

size_t scroll_window_default;
size_t scroll_line_default = 1;
size_t scroll_half_screen_default;
size_t horizontal_scroll_default;

size_t current_horizontal_offset = 0;

void panic_exit(int * fds, int count)
{
    int i;
    for (i = 0; i < count; ++i)
    {
        close(fds[i]);
    }
    set_terminal_attributes(STDIN_FILENO, &sh_attr);
}

void clear_cmd_win()
{
    wclear(cmd_win);
    mvwaddwstr(cmd_win, 0, 0, L":");
    wrefresh(cmd_win);
}

int * process_cmd_line(int argc, char * argv[], int * count, char stdin_was_pipe) /* returns an array of FD's to opened files */
{
    int i;
    int * fd, tempcount = 0;
    if (argc < 2)
    {
        /* 
         * if there are no arguments, check if there is a pipe connected to us
         * if not, exit, else go for stdin as input
         */
         if (isatty(stdin_temp))
         {
             perror("Missing filename!");
             panic_exit(fd, 0);
             _exit(NOFILE_FAILCODE);
         }

         fd = malloc(sizeof(int));
         is_stdin = malloc(sizeof(char));
         if (fd == NULL || is_stdin == NULL)
         {
             perror("Not enough memory, malloc call failed");
             panic_exit(fd , 0);
             _exit(OUTOFMEMORY_FAILCODE);
         }
         *fd = stdin_temp;
         *count = 1;
         *is_stdin = 0;
         return fd;
    }
    fd = malloc(sizeof(int) * (argc - (stdin_was_pipe ? 0 : 1)));
    is_stdin = malloc(sizeof(char) * (argc - (stdin_was_pipe ? 0 : 1)));

    if (stdin_was_pipe)
    {
        fd[0] = stdin_temp;
        is_stdin[0] = 0;
        ++tempcount;
    }
    for (i = 1; i < argc; ++i)
    {
        if (argv[i][0] == '-' && argv[i][1] == '\0') /* if stdin */
        {
            fd[i - (stdin_was_pipe ? 0 : 1)] = STDIN_FILENO;
            is_stdin[i - (stdin_was_pipe ? 0 : 1)] = 1;
            ++tempcount;
            continue;
        }
        if ((fd[i - (stdin_was_pipe ? 0 : 1)] = open(argv[i], O_RDONLY)) == -1)
        {
            panic_exit(fd, tempcount);
            perror(argv[i]);
            _exit(OPENFILE_FAILCODE);
        }
        is_stdin[i - (stdin_was_pipe ? 0 : 1)] = 0;
        ++tempcount;
    }
    *count = tempcount;
    return fd;
}

int get_terminal_attributes(int fd, struct termios * dest)
{
    if (tcgetattr(fd, dest)) /* get shell attributes */
    {
        perror("Failed to get terminal attributes\n");
        _exit(GETATTR_FAILCODE);
    } 
    return 0;
}

int set_terminal_attributes(int fd, struct termios * src)
{
    if (tcsetattr(fd, TCSANOW, src)) /* apply changes */
    {
        perror("Failed to set terminal attributes\n");
        _exit(SETATTR_FAILCODE);
    }
    return 0;
}

void enable_ncurses()
{
    int total_lines, total_cols;

    initscr();

    if (has_colors() == TRUE)                   /* enable color pair for control character output */
    {
        start_color();
        init_pair(1, COLOR_WHITE, COLOR_BLUE);
    }

    curs_set(0);

    noecho();
    raw();

    getmaxyx(stdscr, total_lines, total_cols);

    scroll_half_screen_default =  (scroll_window_default = total_lines - 1) / 2;
    horizontal_scroll_default = (total_cols / 2);

    main_win = newwin(total_lines - 1, total_cols, 0, 0);
    cmd_win = newwin(1, total_cols - 3, total_lines - 1, 0);
    eof_win = newwin(1, 3, total_lines - 1, total_cols - 3);
}

void free_ncurses()
{
    delwin(main_win);
    delwin(cmd_win);
    delwin(eof_win);

    endwin();
    delscreen(term);
}

void command_loop(int istty, char interactive)
{
    int current_file = 0;
    struct work_file ** files;
    int i, loop = 1;
    wint_t ch;
    int do_default_size = 1;
    size_t arg = 0;

    files = (struct work_file **)malloc(sizeof(struct work_file *) * files_opened);
    if (!files)
    {
        panic_exit(fd_array, files_opened);
        perror("Malloc call failed");
        _exit(OUTOFMEMORY_FAILCODE);
    }
    
    for (i = 0; i < files_opened; ++i)
        files[i] = NULL;
    
    cbreak();
    raw();
    if (!interactive)
    {
        for (i = 0; i < files_opened; ++i)
        {
            int prev_line_total = 0, j, total_bytes_read;
            char * p;
            while (p = read_chunk(fd_array[i], &total_bytes_read))
                write(STDOUT_FILENO, p, total_bytes_read);
            /* TODO: error check */

        }
    }
    else
    {
        files[0] = create_work_file(fd_array[0]);
        read_work_file(files[0], 0, NULL);
        print_work_file(files[0]);
        wnoutrefresh(main_win);
        wnoutrefresh(cmd_win);
        doupdate();
        clear_cmd_win();
        raw();
        keypad(cmd_win, TRUE);
        /* main loop */
        while (loop)
        {
sw:     wget_wch(cmd_win, &ch);
            switch(ch)
            {
                case L'z':
                    if (!do_default_size)
                        scroll_window_default = arg;
                case L'\x20': /* space bar */
                case L'\x16': /* ^V */
                case L'f':
                case L'\x6':  /* ^F */
                /* move forward a screenful */
                    gotoline_work_file(files[current_file], files[current_file]->current_line + (do_default_size ? scroll_window_default : arg));
                    do_default_size = 1;
                    clear_cmd_win();
                    break;
                case L'\r':
                case L'\n':
                case L'\xE': /* ^N */
                case L'e':
                case L'j': 
                /* down 1 line */
                    gotoline_work_file(files[current_file], files[current_file]->current_line + (do_default_size ? scroll_line_default : arg));
                    do_default_size = 1;
                    clear_cmd_win();
                    break;
                case L'd': 
                case L'\x4': /* ^D */
                /* forward half a screen */
                    if (!do_default_size)
                        scroll_half_screen_default = arg;
                    gotoline_work_file(files[current_file], files[current_file]->current_line + scroll_half_screen_default);
                    do_default_size = 1;
                    clear_cmd_win();
                    break;
                case L'w':
                    if (!do_default_size)
                        scroll_window_default = arg;
                case L'b':
                case L'\x2': /* ^B */
                /* move backwards a screenful */
                    gotoline_work_file(files[current_file], files[current_file]->current_line - (do_default_size ? scroll_window_default : arg));
                    do_default_size = 1;
                    clear_cmd_win();
                    break;
                case L'y':
                case L'\x19': /* ^Y */
                case L'\x10': /* ^P */
                case L'\xB': /* ^K */
                case L'k': 
                /* up 1 line */
                    gotoline_work_file(files[current_file], files[current_file]->current_line - (do_default_size ? scroll_line_default : arg));
                    do_default_size = 1;
                    clear_cmd_win();
                    break;
                case L'u':
                case L'\x15': /* ^U */
                /* backward half a screen */
                    if (!do_default_size)
                        scroll_half_screen_default = arg;
                    gotoline_work_file(files[current_file], files[current_file]->current_line - scroll_half_screen_default);
                    do_default_size = 1;
                    clear_cmd_win();
                    break;
                case L'R':
                /* repaint and read */
                    read_work_file(files[current_file], 0, NULL);
                    print_work_file(files[current_file]);
                    clear_cmd_win();
                case L'r':
                case L'\x12': /* ^R */
                case L'\xC': /* ^L */
                /* repaint */
                    print_work_file(files[current_file]);
                    do_default_size = 1;
                    clear_cmd_win();
                    break;
                case L'g':
                case L'<':
                /* goto line */
                    gotoline_work_file(files[current_file], do_default_size ? 0 : arg - 1);
                    do_default_size = 1;
                    clear_cmd_win();
                    break;
                case L'G':
                case L'>':
                /* goto line or eof */
                    if (do_default_size)
                        gotoendof_work_file(files[current_file]);
                    else
                        gotoline_work_file(files[current_file], arg - 1);
                    do_default_size = 1;
                    clear_cmd_win();
                    break;
                case KEY_LEFT:
                    if (!do_default_size)
                    {
                        horizontal_scroll_default = arg;
                    }
                    files[current_file]->horizontal_scroll -= horizontal_scroll_default;
                    print_work_file(files[current_file]);
                    do_default_size = 1;
                    break;
                case KEY_RIGHT:
                    if (!do_default_size)
                        horizontal_scroll_default = arg;
                    files[current_file]->horizontal_scroll += horizontal_scroll_default;
                    print_work_file(files[current_file]);
                    do_default_size = 1;
                    break;
                case L'p':
                case L'%':
                    clear_cmd_win();
                    readtoendof_work_file(files[current_file]);
                    if (do_default_size)
                        arg = 0;
                    gotoline_work_file(files[current_file], (files[current_file]->line_offset_map_size - 1) * (arg / 100));
                    print_work_file(files[current_file]);
                    do_default_size = 1;
                    break;
                case L'q': /* exit */
                    loop = 0;
                    do_default_size = 1;
                    break;
                case L'0': case L'1': case L'2': case L'3':
                case L'4': case L'5': case L'6': case L'7':
                case L'8': case L'9':
                    waddnwstr(cmd_win, &ch, 1);
                    if (do_default_size)
                    {
                        do_default_size = arg = 0;
                    }
                    arg = arg * 10 + ch - L'0';
                    break;
                case L':': /* command */
                    mvwaddwstr(cmd_win, 0, 0, L":");
                    wrefresh(cmd_win);
                    wget_wch(cmd_win, &ch);
                    switch(ch)
                    {
                        case 'n':
                            current_file++;
                            break;
                        case 'p':
                            current_file--;
                            break;
                        case 'x':
                            current_file = do_default_size ? 0 : arg - 1;
                            break;
                        case 'f':
                            mvwprintw(cmd_win, 0, 0, "%d: %d", current_file + 1, files[current_file]->current_line);
                            wrefresh(cmd_win);
                            goto sw;
                            break;
                        default:
                            goto sw;
                            break;
                    }
                    if (current_file < 0)
                        current_file = 0;
                    if (current_file >= files_opened)
                        current_file = files_opened - 1;
                    if (files[current_file] == NULL)
                    {
                        files[current_file] = create_work_file(fd_array[current_file]);
                        read_work_file(files[current_file], 0, NULL);
                    }
                    print_work_file(files[current_file]);
                    wclear(cmd_win);
                    wrefresh(cmd_win);
                    wnoutrefresh(main_win);
                    wnoutrefresh(cmd_win);
                    doupdate();
                    do_default_size = 1;
                    break;
                case L'\033':   /* escape sequence */
                    wclear(cmd_win);
                    mvwaddwstr(cmd_win, 0, 1, L"ESC");
                    wrefresh(cmd_win);
                    wget_wch(cmd_win, &ch);
                    switch(ch)
                    {
                        case L')':
                            if (!do_default_size)
                                horizontal_scroll_default = arg;
                            files[current_file]->horizontal_scroll += horizontal_scroll_default;
                            print_work_file(files[current_file]);
                            clear_cmd_win();
                            break;
                        case L'(':
                            if (!do_default_size)
                                horizontal_scroll_default = arg;
                            files[current_file]->horizontal_scroll -= horizontal_scroll_default;
                            print_work_file(files[current_file]);
                            clear_cmd_win();
                            break;
                        case L'<':
                            gotoline_work_file(files[current_file], do_default_size ? 0 : arg - 1);
                            do_default_size = 1;
                            clear_cmd_win();
                            break;
                        case L'>':
                            if (do_default_size)
                                gotoendof_work_file(files[current_file]);
                            else
                                gotoline_work_file(files[current_file], arg - 1);
                            do_default_size = 1;
                            clear_cmd_win();
                            break;
                        default:
                            clear_cmd_win();
                            break;
                    }
                    break;
                default:
                    if (is_stdin[current_file] && !files[current_file]->met_EOF)
                    {
                        read_work_file(files[current_file], 0, ch);
                        print_work_file(files[current_file]);
                    }
                    else
                    {
                        clear_cmd_win();
                    }
                    break;
            }
        }
    }
    free(files);
}


/* TODO: пройтись по аргументам, и если у нас там есть tty, то istty = true */
int main(int argc, char * argv[])
{    
    int istty = isatty(STDIN_FILENO);
    int i;

    simulate_cat = !isatty(STDOUT_FILENO);
    
    setlocale(LC_ALL, "");

    if (!istty)
    {
        int fin;
        fin = dup(STDIN_FILENO);
        if (fin == -1)
        {
            perror("/dev/tty: failed to open file");
            _exit(OPENFILE_FAILCODE);
        }

        stdin_temp = fin;
        stdout_temp = 1;

        close(STDIN_FILENO);

        if (open("/dev/tty", O_RDONLY) == -1)
        {
            perror("/dev/tty: failed to open file");
            _exit(OPENFILE_FAILCODE);
        }
    }

    get_terminal_attributes(STDIN_FILENO, &sh_attr);

    memcpy(&prog_attr, &sh_attr, sizeof(struct termios)); /* copy shell attrs for later */

    prog_attr.c_lflag = prog_attr.c_lflag & !ICANON & !ISIG & !ECHO | IGNCR | INLCR; /* set noncanonical mode */


    fd_array = process_cmd_line(argc, argv, &files_opened, !istty);

    for (i = 0; i < files_opened; ++i)
    {
        if (is_stdin[i])
            set_terminal_attributes(fd_array[i], &prog_attr);
    }

    if (simulate_cat)
    {
        command_loop(istty, 0);
    }
    else {

        enable_ncurses();

        set_terminal_attributes(STDIN_FILENO, &prog_attr);
        
        command_loop(istty, 1);

        free_ncurses();

        set_terminal_attributes(STDIN_FILENO, &sh_attr);

    }

    set_terminal_attributes(STDIN_FILENO, &sh_attr);

    free(fd_array);
    free(is_stdin);

    return 0;
}
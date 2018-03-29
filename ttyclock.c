/*
 *      TTY-CLOCK Main file.
 *      Copyright © 2009-2018 tty-clock contributors
 *      Copyright © 2008 Martin Duquesnoy <xorg62@gmail.com>
 *      All rights reserved.
 *
 *      Redistribution and use in source and binary forms, with or without
 *      modification, are permitted provided that the following conditions are
 *      met:
 *
 *      * Redistributions of source code must retain the above copyright
 *        notice, this list of conditions and the following disclaimer.
 *      * Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following disclaimer
 *        in the documentation and/or other materials provided with the
 *        distribution.
 *      * Neither the name of the  nor the names of its
 *        contributors may be used to endorse or promote products derived from
 *        this software without specific prior written permission.
 *
 *      THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *      "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *      LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *      A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *      OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *      SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *      LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *      DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *      THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *      (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *      OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "ttyclock.h"

void
init(void)
{
    struct sigaction sig;
    ttyclock->bg = COLOR_BLACK;

    /* Init ncurses */
    if (ttyclock->tty) {
        FILE *ftty = fopen(ttyclock->tty, "r+");
        if (!ftty) {
            fprintf(stderr, "tty-clock: error: '%s' couldn't be opened: %s.\n",
            ttyclock->tty, strerror(errno));
            exit(EXIT_FAILURE);
        }
        ttyclock->ttyscr = newterm(NULL, ftty, ftty);
        assert(ttyclock->ttyscr != NULL);
        set_term(ttyclock->ttyscr);
    } else
        initscr();

    cbreak();
    noecho();
    keypad(stdscr, True);
    start_color();
    curs_set(False);
    clear();

    /* Init default terminal color */
    if(use_default_colors() == OK)
        ttyclock->bg = -1;

    /* Init color pair */
    init_pair(0, ttyclock->bg, ttyclock->bg);
    init_pair(1, ttyclock->bg, ttyclock->option.color);
    init_pair(2, ttyclock->option.color, ttyclock->bg);
//     init_pair(0, ttyclock->bg, ttyclock->bg);
//     init_pair(1, ttyclock->bg, ttyclock->option.color);
//     init_pair(2, ttyclock->option.color, ttyclock->bg);
    refresh();

    /* Init signal handler */
    sig.sa_handler = signal_handler;
    sig.sa_flags   = 0;
    sigaction(SIGWINCH, &sig, NULL);
    sigaction(SIGTERM,  &sig, NULL);
    sigaction(SIGINT,   &sig, NULL);
    sigaction(SIGSEGV,  &sig, NULL);

    /* Init global struct */
    ttyclock->running = True;
    if(!ttyclock->geo.x)
        ttyclock->geo.x = 0;
    if(!ttyclock->geo.y)
        ttyclock->geo.y = 0;
    if(!ttyclock->geo.a)
        ttyclock->geo.a = 1;
    if(!ttyclock->geo.b)
        ttyclock->geo.b = 1;
    ttyclock->geo.w = (ttyclock->option.second) ? SECFRAMEW : NORMFRAMEW;
    ttyclock->geo.h = 7;
    ttyclock->tm = localtime(&(ttyclock->lt));
    if(ttyclock->option.utc) {
        ttyclock->tm = gmtime(&(ttyclock->lt));
    }
    ttyclock->lt = time(NULL);
    update_hour();

    /* Create clock win */
    ttyclock->framewin = newwin(ttyclock->geo.h,
                                ttyclock->geo.w,
                                ttyclock->geo.x,
                                ttyclock->geo.y);
    if(ttyclock->option.box) {
         box(ttyclock->framewin, 0, 0);
    }

    if (ttyclock->option.bold)
    {
        wattron(ttyclock->framewin, A_BLINK);
    }

    /* Create the date win */
    ttyclock->datewin = newwin(DATEWINH, strlen(ttyclock->date.datestr) + 2,
                               ttyclock->geo.x + ttyclock->geo.h - 1,
                               ttyclock->geo.y + (ttyclock->geo.w / 2) -
                               (strlen(ttyclock->date.datestr) / 2) - 1);
    if(ttyclock->option.box && ttyclock->option.date) {
        box(ttyclock->datewin, 0, 0);
    }
    clearok(ttyclock->datewin, True);

    if (ttyclock->option.center)
        set_center(ttyclock->option.center);
    else
        if (ttyclock->option.position)
            set_position(ttyclock->option.position);

    nodelay(stdscr, True);

    if (ttyclock->option.date)
    {
        wrefresh(ttyclock->datewin);
    }

    wrefresh(ttyclock->framewin);
}

void
signal_handler(int signal)
{
    switch(signal)
    {
    case SIGWINCH:
        endwin();
        init();
        break;
        /* Interruption signal */
    case SIGINT:
    case SIGTERM:
        ttyclock->running = False;
        /* Segmentation fault signal */
        break;
    case SIGSEGV:
        endwin();
        fprintf(stderr, "Segmentation fault.\n");
        exit(EXIT_FAILURE);
        break;
    }
}

void
cleanup(void)
{
    if (ttyclock->ttyscr)
        delscreen(ttyclock->ttyscr);

    if (ttyclock && ttyclock->tty)
        free(ttyclock->tty);
    if (ttyclock && ttyclock->option.format)
        free(ttyclock->option.format);
    if (ttyclock)
        free(ttyclock);
}

void
update_hour(void)
{
    int ihour;
    char tmpstr[128];

    ttyclock->lt = time(NULL);
    ttyclock->tm = localtime(&(ttyclock->lt));
    if(ttyclock->option.utc) {
        ttyclock->tm = gmtime(&(ttyclock->lt));
    }

    ihour = ttyclock->tm->tm_hour;

    if(ttyclock->option.twelve)
        ttyclock->meridiem = ((ihour >= 12) ? PMSIGN : AMSIGN);
    else
        ttyclock->meridiem = "\0";

    /* Manage hour for twelve mode */
    ihour = ((ttyclock->option.twelve && ihour > 12)  ? (ihour - 12) : ihour);
    ihour = ((ttyclock->option.twelve && !ihour) ? 12 : ihour);

    /* Set hour */
    ttyclock->date.hour[0] = ihour / 10;
    ttyclock->date.hour[1] = ihour % 10;

    /* Set minutes */
    ttyclock->date.minute[0] = ttyclock->tm->tm_min / 10;
    ttyclock->date.minute[1] = ttyclock->tm->tm_min % 10;

    /* Set date string */
    strftime(tmpstr,
             sizeof(tmpstr),
             ttyclock->option.format,
             ttyclock->tm);
    sprintf(ttyclock->date.datestr, "%s%s", tmpstr, ttyclock->meridiem);

    /* Set seconds */
    ttyclock->date.second[0] = ttyclock->tm->tm_sec / 10;
    ttyclock->date.second[1] = ttyclock->tm->tm_sec % 10;
}

void
draw_number(int n, int x, int y)
{
    int i, sy = y;

    for(i = 0; i < 30; ++i, ++sy)
    {
        if(sy == y + 6)
        {
            sy = y;
            ++x;
        }

        if (ttyclock->option.bold)
            wattron(ttyclock->framewin, A_BLINK);
        else
            wattroff(ttyclock->framewin, A_BLINK);

        wbkgdset(ttyclock->framewin, COLOR_PAIR(number[n][i/2]));
        mvwaddch(ttyclock->framewin, x, sy, ' ');
    }
    wrefresh(ttyclock->framewin);
}

void
draw_clock(void)
{
    /* Draw hour numbers */
    draw_number(ttyclock->date.hour[0], 1, 1);
    draw_number(ttyclock->date.hour[1], 1, 8);
    chtype dotcolor = COLOR_PAIR(1);
    if (ttyclock->option.blink && time(NULL) % 2 == 0)
        dotcolor = COLOR_PAIR(2);

    /* 2 dot for number separation */
    wbkgdset(ttyclock->framewin, dotcolor);
    mvwaddstr(ttyclock->framewin, 2, 16, "  ");
    mvwaddstr(ttyclock->framewin, 4, 16, "  ");

    /* Draw minute numbers */
    draw_number(ttyclock->date.minute[0], 1, 20);
    draw_number(ttyclock->date.minute[1], 1, 27);

    /* Draw the date */
    if (ttyclock->option.bold)
        wattron(ttyclock->datewin, A_BOLD);
    else
        wattroff(ttyclock->datewin, A_BOLD);

    if (ttyclock->option.date)
    {
        wbkgdset(ttyclock->datewin, (COLOR_PAIR(2)));
        mvwprintw(ttyclock->datewin, (DATEWINH / 2), 1, ttyclock->date.datestr);
        wrefresh(ttyclock->datewin);
    }

    /* Draw second if the option is enable */
    if(ttyclock->option.second)
    {
        /* Again 2 dot for number separation */
        wbkgdset(ttyclock->framewin, dotcolor);
        mvwaddstr(ttyclock->framewin, 2, NORMFRAMEW, "  ");
        mvwaddstr(ttyclock->framewin, 4, NORMFRAMEW, "  ");

        /* Draw second numbers */
        draw_number(ttyclock->date.second[0], 1, 39);
        draw_number(ttyclock->date.second[1], 1, 46);
    }
}

void
clock_move(int x, int y, int w, int h)
{
    /* Erase border for a clean move */
    wbkgdset(ttyclock->framewin, COLOR_PAIR(0));
    wborder(ttyclock->framewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    werase(ttyclock->framewin);
    wrefresh(ttyclock->framewin);

    if (ttyclock->option.date)
    {
        wbkgdset(ttyclock->datewin, COLOR_PAIR(0));
        wborder(ttyclock->datewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        werase(ttyclock->datewin);
        wrefresh(ttyclock->datewin);
    }

    /* Frame win move */
    mvwin(ttyclock->framewin, (ttyclock->geo.x = x), (ttyclock->geo.y = y));
    wresize(ttyclock->framewin, (ttyclock->geo.h = h), (ttyclock->geo.w = w));

    /* Date win move */
    if (ttyclock->option.date)
    {
        mvwin(ttyclock->datewin,
              ttyclock->geo.x + ttyclock->geo.h - 1,
              ttyclock->geo.y + (ttyclock->geo.w / 2) - (strlen(ttyclock->date.datestr) / 2) - 1);
        wresize(ttyclock->datewin, DATEWINH, strlen(ttyclock->date.datestr) + 2);

        if (ttyclock->option.box) {
            box(ttyclock->datewin,  0, 0);
        }
    }

    if (ttyclock->option.box)
    {
        box(ttyclock->framewin, 0, 0);
    }

    wrefresh(ttyclock->framewin);
    wrefresh(ttyclock->datewin);
}

/* Useless but fun :) */
void
clock_rebound(void)
{
    if(!ttyclock->option.rebound)
        return;

    if(ttyclock->geo.x < 1)
        ttyclock->geo.a = 1;
    if(ttyclock->geo.x > (LINES - ttyclock->geo.h - DATEWINH))
        ttyclock->geo.a = -1;
    if(ttyclock->geo.y < 1)
        ttyclock->geo.b = 1;
    if(ttyclock->geo.y > (COLS - ttyclock->geo.w - 1))
        ttyclock->geo.b = -1;

    clock_move(ttyclock->geo.x + ttyclock->geo.a,
               ttyclock->geo.y + ttyclock->geo.b,
               ttyclock->geo.w,
               ttyclock->geo.h);
}

void
set_second(void)
{
    int new_w = (((ttyclock->option.second = !ttyclock->option.second)) ? SECFRAMEW : NORMFRAMEW);
    int y_adj;

    for(y_adj = 0; (ttyclock->geo.y - y_adj) > (COLS - new_w - 1); ++y_adj);

    clock_move(ttyclock->geo.x, (ttyclock->geo.y - y_adj), new_w, ttyclock->geo.h);

    if (ttyclock->option.center)
        set_center(ttyclock->option.center);
    else
        if (ttyclock->option.position)
}

void
set_center(Bool b)
{
    if((ttyclock->option.center = b))
    {
        ttyclock->option.rebound = False;

        clock_move((LINES / 2 - (ttyclock->geo.h / 2)),
                   (COLS  / 2 - (ttyclock->geo.w / 2)),
                   ttyclock->geo.w,
                   ttyclock->geo.h);
    }
}

void
set_position(int p)
{
    int LINE, COL;

    ttyclock->option.rebound = False;

    if (p >= 1 && p <= 3)
        LINE = 0;
    if (p >= 4 && p <= 6)
        LINE = (LINES / 2) - (ttyclock->geo.h / 2);
    if (p >= 7 && p <= 9)
        LINE = LINES - ttyclock->geo.h - 2;

    if (p == 1 || p == 4 || p == 7)
        COL = 0;
    if (p == 2 || p == 5 || p == 8)
        COL = (COLS  / 2) - (ttyclock->geo.w / 2);
    if (p == 3 || p == 6 || p == 9)
        COL = COLS - ttyclock->geo.w;

    clock_move(LINE,
               COL,
               ttyclock->geo.w,
               ttyclock->geo.h);

}

void
set_box(Bool b)
{
    ttyclock->option.box = b;

    wbkgdset(ttyclock->framewin, COLOR_PAIR(0));
    wbkgdset(ttyclock->datewin, COLOR_PAIR(0));

    if(ttyclock->option.box) {
        wbkgdset(ttyclock->framewin, COLOR_PAIR(0));
        wbkgdset(ttyclock->datewin, COLOR_PAIR(0));
        box(ttyclock->framewin, 0, 0);
        box(ttyclock->datewin,  0, 0);
    }
    else {
        wborder(ttyclock->framewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
        wborder(ttyclock->datewin, ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ');
    }

    wrefresh(ttyclock->datewin);
    wrefresh(ttyclock->framewin);
}

void
key_event(void)
{
    int i, c;

    struct timespec length = { ttyclock->option.delay, ttyclock->option.nsdelay };

    if (ttyclock->option.screensaver)
    {
        c = wgetch(stdscr);
        if(c != ERR && ttyclock->option.noquit == False)
        {
            ttyclock->running = False;
        }
        else
        {
            nanosleep(&length, NULL);
            for(i = 0; i < 8; ++i)
                if(c == (i + '0'))
                {
                    ttyclock->option.color = i;
                    init_pair(1, ttyclock->bg, i);
                    init_pair(2, i, ttyclock->bg);
                }
        }
        return;
    }


    switch(c = wgetch(stdscr))
    {
    case KEY_UP:
    case 'k':
    case 'K':
        if(ttyclock->geo.x >= 1
          && !ttyclock->option.center)
            clock_move(ttyclock->geo.x - 1, ttyclock->geo.y, ttyclock->geo.w, ttyclock->geo.h);
        break;

    case KEY_DOWN:
    case 'j':
    case 'J':
        if(ttyclock->geo.x <= (LINES - ttyclock->geo.h - DATEWINH)
            && !ttyclock->option.center)
                clock_move(ttyclock->geo.x + 1,
                           ttyclock->geo.y,
                           ttyclock->geo.w,
                           ttyclock->geo.h);
        break;

    case KEY_LEFT:
    case 'h':
    case 'H':
        if(ttyclock->geo.y >= 1
            && !ttyclock->option.center)
                clock_move(ttyclock->geo.x,
                           ttyclock->geo.y - 1,
                           ttyclock->geo.w,
                           ttyclock->geo.h);
        break;

    case KEY_RIGHT:
    case 'l':
    case 'L':
        if(ttyclock->geo.y <= (COLS - ttyclock->geo.w - 1)
            && !ttyclock->option.center)
                clock_move(ttyclock->geo.x,
                        ttyclock->geo.y + 1,
                        ttyclock->geo.w,
                        ttyclock->geo.h);
        break;

    case 'q':
    case 'Q':
        if (ttyclock->option.noquit == False)
        ttyclock->running = False;
        break;

    case 's':
    case 'S':
        set_second();
        break;

    case 't':
    case 'T':
        ttyclock->option.twelve = !ttyclock->option.twelve;
        /* Set the new ttyclock->date.datestr to resize date window */
        update_hour();
        clock_move(ttyclock->geo.x, ttyclock->geo.y, ttyclock->geo.w, ttyclock->geo.h);
        break;

    case 'c':
    case 'C':
        set_center(!ttyclock->option.center);
        break;

    case 'b':
    case 'B':
        ttyclock->option.bold = !ttyclock->option.bold;
        break;

    case 'r':
    case 'R':
        ttyclock->option.rebound = !ttyclock->option.rebound;
        if(ttyclock->option.rebound && ttyclock->option.center)
            ttyclock->option.center = False;
        break;

    case 'x':
    case 'X':
        set_box(!ttyclock->option.box);
        break;

    default:
        nanosleep(&length, NULL);
        for(i = 0; i < 8; ++i)
            if(c == (i + '0'))
            {
                ttyclock->option.color = i;
                init_pair(1, ttyclock->bg, i);
                init_pair(2, i, ttyclock->bg);
            }
        break;
    }
}

int
main(int argc, char **argv)
{
    int c;

    /* Alloc ttyclock */
    ttyclock = malloc(sizeof(ttyclock_t));
    assert(ttyclock != NULL);
    memset(ttyclock, 0, sizeof(ttyclock_t));

    ttyclock->option.date = True;

    /* Date format */
    ttyclock->option.format = malloc(sizeof(char) * 100);
    /* Default date format */
    strncpy(ttyclock->option.format, "%F", 100);
    /* Default color */
    ttyclock->option.color = COLOR_GREEN; /* COLOR_GREEN = 2 */
    /* Default delay */
    ttyclock->option.delay = 1; /* 1FPS */
    ttyclock->option.nsdelay = 0; /* -0FPS */
    ttyclock->option.blink = False;

    atexit(cleanup);

    while ((c = getopt(argc, argv, "iuvsScbtrhBxnDC:f:d:T:a:p:")) != -1)
    {
        switch(c)
        {
        case 'h':
        default:
            printf("usage : tty-clock [-iuvsScbtrahDBxn] [-C [0-7]] [-f format] [-d delay] [-a nsdelay] [-T tty] [-p [1-9]] \n"
                   "    -s            Show seconds                                   \n"
                   "    -S            Screensaver mode                               \n"
                   "    -x            Show box                                       \n"
                   "    -c            Set the clock at the center of the terminal    \n"
                   "    -C [0-7]      Set the clock color                            \n"
                   "    -b            Use bold colors                                \n"
                   "    -t            Set the hour in 12h format                     \n"
                   "    -u            Use UTC time                                   \n"
                   "    -T tty        Display the clock on the specified terminal    \n"
                   "    -r            Do rebound the clock                           \n"
                   "    -f format     Set the date format                            \n"
                   "    -n            Don't quit on keypress                         \n"
                   "    -v            Show tty-clock version                         \n"
                   "    -i            Show some info about tty-clock                 \n"
                   "    -h            Show this page                                 \n"
                   "    -D            Hide date                                      \n"
                   "    -B            Enable blinking colon                          \n"
                   "    -d delay      Set the delay between two redraws of the clock. Default 1s. \n"
                   "    -a nsdelay    Additional delay between two redraws in nanoseconds. Default 0ns.\n"
                   "    -p [1-9]      Position of clock in terminal: 1-9 starting top-left and ending bottom-right:\n\n"
                   "                  123\n"
                   "                  456\n"
                   "                  789\n");
            exit(EXIT_SUCCESS);
            break;
        case 'i':
            puts("TTY-Clock 2 © by Martin Duquesnoy (xorg62@gmail.com), Grey (grey@greytheory.net)");
            exit(EXIT_SUCCESS);
            break;
        case 'u':
            ttyclock->option.utc = True;
            break;
        case 'v':
            puts("TTY-Clock 2 © devel version");
            exit(EXIT_SUCCESS);
            break;
        case 's':
            ttyclock->option.second = True;
            break;
        case 'S':
            ttyclock->option.screensaver = True;
            break;
        case 'c':
            ttyclock->option.center = True;
            break;
        case 'b':
            ttyclock->option.bold = True;
            break;
        case 'C':
            if(atoi(optarg) >= 0 && atoi(optarg) < 8)
                ttyclock->option.color = atoi(optarg);
            break;
        case 't':
            ttyclock->option.twelve = True;
            break;
        case 'r':
            ttyclock->option.rebound = True;
            break;
        case 'f':
            strncpy(ttyclock->option.format, optarg, 100);
            break;
        case 'd':
            if(atol(optarg) >= 0 && atol(optarg) < 100)
                ttyclock->option.delay = atol(optarg);
            break;
        case 'D':
            ttyclock->option.date = False;
            break;
        case 'B':
            ttyclock->option.blink = True;
            break;
        case 'a':
            if(atol(optarg) >= 0 && atol(optarg) < 1000000000)
                ttyclock->option.nsdelay = atol(optarg);
             break;
        case 'x':
            ttyclock->option.box = True;
            break;
        case 'p':
            if(atol(optarg) > 0 && atol(optarg) < 10)
                ttyclock->option.position = atol(optarg);
            break;
     case 'T': {
         struct stat sbuf;
         if (stat(optarg, &sbuf) == -1) {
            fprintf(stderr, "tty-clock: error: couldn't stat '%s': %s.\n",
                optarg, strerror(errno));
            exit(EXIT_FAILURE);
         } else if (!S_ISCHR(sbuf.st_mode)) {
                    fprintf(stderr, "tty-clock: error: '%s' doesn't appear to be a character device.\n",
                optarg);
                exit(EXIT_FAILURE);
         } else {
            if (ttyclock->tty)
                free(ttyclock->tty);
            ttyclock->tty = strdup(optarg);
         }}
         break;
     case 'n':
         ttyclock->option.noquit = True;
         break;
        }
    }

    init();
    attron(A_BLINK);
    while(ttyclock->running)
    {
        clock_rebound();
        update_hour();
        draw_clock();
        key_event();
    }

    endwin();
}

// vim: expandtab tabstop=4 softtabstop=4 shiftwidth=4

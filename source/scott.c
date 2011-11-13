/* $Id: scott.c,v 1.1.1.1 2001/01/08 07:29:19 edwards Exp $ */
 
/*
 * File:	scott.c
 * Programmer:	Scott Kilau.
 * Date:	February 2000.
 *
 * Description:	My "Browser" interface into the "search" and "browse" lists.
 * 		I have tried to duplicate a cross between "pico" and
 *		"lynx" for the interface. Fully uses curses calls to
 *		implement its windowing.
 *
 * ToDo:	Ensure the block mode works correctly.
 */

/* Our include files */
#include "teknap.h"
#include "struct.h"
#include "commands.h"
#include "input.h"
#include "list.h"
#include "output.h"
#include "napster.h"
#include "server.h"
#include "status.h"
#include "window.h"
#include "vars.h"
#include "scott.h"
#include "scott2.h"
#include "tgtk.h"


/* Our 1 global variable we export out to the world */
int in_browser = 0;
extern Server *server_list;

#if defined(GUI) && (defined(__EMX__) || defined(WIN32_GUI))

#include "scottpm.c"

#else

void update_browser(void) { }

#if defined(SCOTT) 
#if defined(USING_NCURSES) || defined(USING_CURSES)

typedef struct
{
	int fg;
	int bg;
} CPAIR;

/* 128 is more than enuf, I believe ncurses is 64, curses is 32. */
static CPAIR cpairs[128];
static int cpair_count = 0;

/* Our static strings */
static char copyright[] = "---Sheiker Browser 1.0---";
static char help1[] = "Up/Down: arrow keys.      ^V Next Page.       ^Y Prev Page";
static char help2[] = "^O Download CMDS          ^A Home             ^E End            Q/q Quit.";
static char help3[] = "Enter: Get File          Space: Toggle File.     ^^ Toggle block mode.";
static char help4[] = "^O Movement CMDS         ^W Write Block.         ^D Download Toggled Files";

/* Our static flags */
static int block_mode = 0;
static int help_toggle = 0;

/* External global linked lists, we need to be able to access */
extern FileStruct *file_browse;
extern char *n_speed(int);

/* Our static pointers */
static NickStruct *curr_nick = NULL;
static FileStruct *top = NULL;
static FileStruct *bottom = NULL;
static FileStruct *true_top = NULL;
static FileStruct *true_bottom = NULL;
static WINDOW *header_win;
static WINDOW *main_win;
static WINDOW *help_win;
static WINDOW *tree_win;
static int in_tree = 0;

#if 0
static WINDOW *status_win;
#endif
/* Finally, our static functions declarations. Most are inlined */
static int display_output(WINDOW *, int);
static void toggle_file(WINDOW *, int, int);
static inline void do_down(WINDOW *);
static inline void do_up(WINDOW *);
static inline void do_page_down(WINDOW *);
static inline void do_page_up(WINDOW *);
static inline void do_home(WINDOW *);
static inline void find_true_bottom(void);
static inline void set_to_hit_bottom(WINDOW *, int);
static inline void do_end(WINDOW *);
static inline void create_header(WINDOW *);
static inline void create_help(WINDOW *, int);
static inline void refresh_all_windows(void);
static inline int get_window_len(WINDOW *);
static inline int get_window_width(WINDOW *);
static inline void fill_with_spaces(char *, int);
static inline FileStruct *correlate_line_to_file(int);
static inline void turn_off_all_block_toggle(void);
static inline void toggle_specific_file(FileStruct *, int);
static inline void do_block_toggle(WINDOW *);
static inline void do_page_block_down(int, int);
static inline void do_page_block_up(int, int);
static inline void convert_block_to_toggle(WINDOW *);
static inline void cleanup(void);
static inline void do_download_of_toggles(void);
static inline void do_actual_get(FileStruct *);
static inline void do_download_of_one_file(WINDOW *);
static int make_attr(int, int, int);
static void prep_search_globals(void);


/* Our command interface in the client */
BUILT_IN_COMMAND(scott)
{
	/* init vars to 0 */
	static int first = 1;
	NickStruct *n;
	char *nick = NULL;
	
	top = NULL;
	bottom = NULL;
	true_top = NULL;
	true_bottom = NULL;
	block_mode = 0;
	help_toggle = 0;
	in_tree = 0;

#ifdef GTK
	if (get_int_var(GTK_VAR)) {
		if (tgtk_okay()) {
			scott2_show(ON);
			return;
		}
		else if (!tgtk_failed()) {
			say("GTK is not ready yet... Please wait a bit and try again");
			return;
		}
		say("GTK failed to start. Make sure you are on an X console!");
		say("To get rid of this message, run /set GTK OFF");
	}
#endif
	/* Since its possible 3 commands could come here. We need
	 * to check which one of the 3 the user wants.
	 */
	if(command && *command) {
		/* First command is sbrowse. It automagically goes to the browse list */
		if (!my_strnicmp(command, "sbrowse", 3)) {
			if (!(nick = next_arg(args, &args))) {
				say("Please specify a nick or -search!");
				return;
			}
		}
		/* 2nd command is sbrowse. It automagically goes to the search list */
		else if (!my_strnicmp(command, "ssearch", 3)) {
			prep_search_globals();
			goto start;
		}
		/* 3rd command is scott. It does the old checking for -search in the nick */
		else if (!my_strnicmp(command, "scott", 3)) {
			if (!(nick = next_arg(args, &args))) {
				say("Please specify a nick or -search!");
				return;
			}
			if (!my_strnicmp(nick, "-search", 7)) {
				prep_search_globals();
				goto start;
			}
		}
		/* Unknown command, return without doing anything */
		else {
			return;
		}
	}


	if ((n = (NickStruct *)find_in_list((List **)&server_list[from_server].users, nick, 0)))
	{
		if (n->flag & BROWSE_IN_PROGRESS)
		{
			say("Can't do that while a browse is in progress");
			return;
		}
		true_top = top = n->file_browse;
		/* Save a pointer to the Nick struct */
		curr_nick = n;
	}
start:
	/* No list, bail out */
	if (!true_top) {
		say("File Browser error. No files in list!");
		curr_nick = NULL;
		return;
	}
	/* Set global flag, telling client that we are in the file browser */
	in_browser = 1;
	/* init screen, which will give us our "main" window */
	initscr();
	nonl();
	/* Set up usual curses stuff */
	noecho();
	cbreak();
	intrflush(stdscr, FALSE);
	/* Start up color support */
	if (first == 1) {
		/* Why do this, you ask? It appears I have found
		 * a difference between curses and ncurses.
		 * ncurses only allows us to start_color() once,
		 * on the other hand, curses DEPENDS upon us
		 * doing a start_color() each time... Goofy.
		 */
#ifdef USING_CURSES
		first = 1;
#else
		first = 0;
#endif
		start_color();
	}
	/* I know pana removed this before, but I have to have this clear(),
	 * Or else my Win95 Securecrt -> Linux -> "screen" (TERM=ansi)
	 * does not clear the non-curses text correctly. -Sheiker
	 */
	clear();
	/* Create and move main into place */
	main_win = newwin(LINES - 4, COLS, 1, 0);
	/* Create the rest of the windows */
	header_win = newwin(1, COLS, 0, 0);
	create_header(header_win);
	help_win = newwin(2, COLS - 1, LINES - 3, 0);
	create_help(help_win, SWITCH_HELP);
	/* Walk the list, setting true_bottom to the end of list */
	find_true_bottom();
	tree_win = NULL;
	if (curr_nick && curr_nick->top)
		top = curr_nick->top;
	else
		top = true_top;
	/* Dump out our list to the main window */
	display_output(main_win, 0);
	/* Set cursor to top line */
	wmove(main_win, 0, MY_INDENT);
	wrefresh(main_win);
	/* Turn keypad on for stdscr AND main_win */
	keypad(stdscr, TRUE);
	keypad(main_win, TRUE);
}


static void prep_search_globals(void)
{
	FileStruct **f;
	f = get_search_head(from_server);
	true_top = top = *f;
	curr_nick = NULL;
}

/* This is our input parser, to check certain keypresses */
void
browser_main_loop(void)
{
	unsigned int cinput = 0;

	switch (cinput = wgetch(main_win)) {
	/* UP */
	case KEY_UP:
	case 'k':
		do_up(main_win);
		break;
	/* DOWN */
	case KEY_DOWN:
	case 'j':
		do_down(main_win);
		break;
	/* LEFT */
	case KEY_LEFT:
		break;
	/* RIGHT */
	case KEY_RIGHT:
		break;
	/* HOME */
	case CTRL_A:
 	case KEY_HOME:
		do_home(main_win);
		break;
	/* END */
	case CTRL_E:
	case KEY_END:
		do_end(main_win);
		break;
	/* NEXT PAGE */
	case CTRL_F:
	case KEY_NPAGE:
	case CTRL_V:
		do_page_down(main_win);
		break;
	/* PREV PAGE */
	case CTRL_B:
	case KEY_PPAGE:
	case CTRL_Y:
		do_page_up(main_win);
		break;
	/* ENTER */
	case KEY_ENTER:
	case 13:
		do_download_of_one_file(main_win);
		refresh_browser();
		break;
	/* CTRL_D : Actually do the "Download" of the batch files */
        case CTRL_D: 
                do_download_of_toggles();
		refresh_browser();
		break;
	/* CTRL-L, standard for doing screen refresh */
	case CTRL_L:
		refresh_browser();
		refresh_all_windows();
		break;
	/* CTRL-^, turns block toggle mode on or off  */
	case BLOCK_KEY:
		do_block_toggle(main_win);
		break;
	/* CTRL-W, saves all blocks mode into real toggles  */
	case CTRL_W:
		convert_block_to_toggle(main_win);
		do_block_toggle(main_win);
		break;
	case CTRL_T:
		in_tree ^= 1;
		display_output(main_win, 0);
		wrefresh(main_win);
		break;
	/* SPACE */
	case SPACEBAR:
		toggle_file(main_win, BROWSE_TOGGLE, DONT_FORCE);
		do_down(main_win);
		break;
	/* CTRl-O, Switch help screen */
	case CTRL_O:
		create_help(help_win, SWITCH_HELP);
		break;
	/* QUIT */
	case 'Q':
	case 'q':
		/* Might be a search, not a browse */
		if (curr_nick) {
			curr_nick->top = top;
			curr_nick->bottom = bottom;		
			curr_nick = NULL;
		}
		cleanup();
		return;
	default:
#if 0
	{
		char buf[20];
		sprintf(buf, "%d   ", cinput);
		mvwaddstr(main_win, 1, 0, buf);
	}
#endif
		break;
	}
#if 0
	wrefresh(main_win);
#endif
}

/* Respond to the KEY_DOWN keypress */
static inline void
do_down(WINDOW *win)
{
	int c_x = 0, c_y = 0, i = 0, len = 0, ratio, flag = 0;

	if (!top || !bottom)
		return;

	getyx(main_win, c_y, c_x);
	ratio = get_int_var(SCROLLBACK_RATIO_VAR);
	if (ratio < 10 ) 
		ratio = 10;
	if (ratio > 100) 
		ratio = 100;
	len = get_window_len(win);

	/* If we hit the bottom of screen,
	 * Shift screen down by "ratio" amount.
	 */
	if (c_y >= len) {
		for (i = 0; i <= len * ratio / 100; i++) {
			if (top->next) {
				c_y--;
				top = top->next;
			}
			else {
				set_to_hit_bottom(win, -1);
				display_output(win, 0);
				wmove(win, 0, 0);
				wrefresh(win);
				return;
			}
		}
	}
	i = len;
	/* If this is true, we are nearing the bottom of the list */
	if (bottom == true_bottom) {
		FileStruct *tmp = NULL;
		i = 0;
		for (tmp = top; tmp; tmp = tmp->next, i++)
			;
		i--;
		flag = 1;
	}
	if (block_mode) {
		wmove(win, c_y, c_x);
		if (!flag)
			toggle_file(win, BLOCK_TOGGLE, DONT_FORCE);
		else if (c_y == i)
			toggle_file(win, BLOCK_TOGGLE, FORCE_ON);
		else
			toggle_file(win, BLOCK_TOGGLE, DONT_FORCE);
	}
	c_y++;
	if (c_y > i)
		c_y = i;
	display_output(win, 0);
	wmove(win, c_y, c_x);
	wrefresh(win);
}


/* Respond to the KEY_UP keypress */
static inline void
do_up(WINDOW *win)
{
	int c_x = 0, c_y = 0, i = 0, len = 0, ratio;

	if (!top)
		return;

	getyx(win, c_y, c_x);
	ratio = get_int_var(SCROLLBACK_RATIO_VAR);
	if (ratio < 10 ) 
		ratio = 10;
	if (ratio > 100) 
		ratio = 100;
	len = get_window_len(win);

	/* If we hit the top of screen,
	 * Shift screen up by "ratio" amount.
	 */
	if (c_y <= 0) {
		for (i = 0; i <= len * ratio / 100; i++) {
			if (top->prev) {
				c_y++;
				top = top->prev;
			}
			else {
				do_home(win);	
				return;
			}
		}
	}
	if (block_mode) {
		wmove(win, c_y - 1, c_x);
		toggle_file(win, BLOCK_TOGGLE, DONT_FORCE);
	}
	c_y--;
	display_output(win, 0);
	wmove(win, c_y, c_x);
	wrefresh(win);
}

static inline void
do_page_down(WINDOW *win)
{
	FileStruct *tmp = NULL;
	int c_x = 0, c_y = 0, i = 0, len = 0, last_y = 0;

	if (!top || !bottom)
		return;

	getyx(win, c_y, c_x);
	len = get_window_len(win);
	do_page_block_down(c_y, len);
	for (tmp = top; tmp; tmp = tmp->next, i++) {
		if (i >= len - 1)
			break;
	}
	if (tmp)
		top = tmp;
	else
		set_to_hit_bottom(win, i);
	last_y = display_output(win, 0);
	if (bottom == true_bottom)
		c_y = last_y - 1;
	wmove(win, c_y, MY_INDENT);
	wrefresh(win);
}

static inline void
do_page_up(WINDOW *win)
{
	FileStruct *tmp = NULL;
	int c_x = 0, c_y = 0, i = 0, len = 0;

	if (!top)
		return;

	getyx(win, c_y, c_x);
	len = get_window_len(win);
	do_page_block_up(c_y, len);
	for (tmp = top; tmp; tmp = tmp->prev, i++) {
		if (i >= len - 1) {
			top = tmp;
			display_output(win, 0);
			wmove(win, c_y, c_x);
			wrefresh(win);
			return;
		}
	}
	/* Move to top */
	do_home(win);
}

static inline void
do_home(WINDOW *win)
{
	top = true_top;
	display_output(win, 0);
	wmove(win, 0, MY_INDENT);
	wrefresh(win);
}

static inline void
do_end(WINDOW *win)
{
	set_to_hit_bottom(win, -1);
	display_output(win, 0);
	wmove(win, 0, 0);
	wrefresh(win);
}

/*
 * Anything in TekNap can call this function, to cause us to regenerate all our
 * windows, and the output thats in each window.
 */
void
refresh_browser(void)
{
	int c_x = 0, c_y = 0;

	getyx(main_win, c_y, c_x);
	display_output(main_win, 0);
	wmove(main_win, c_y, c_x);
	wrefresh(main_win);
}	

/* Our main output function to the main window */
static int
display_output(WINDOW *win, int all)
{
	FileStruct *tmp = NULL, *prev = NULL;
	char buff[300 + 1], format[300 + 1];
	char format1[300+1], format2[300 + 1], format3[300 + 1];
#ifdef PANA_TREE_STUFF
	char buff2[500], *p, *q;
	int newdir = 0;
#endif
	int slen = 0, len = 0, width = 0, c_x = 0, c_y = 0;

	wmove(win, 0, MY_INDENT);
	getyx(win, c_y, c_x);
	width = get_window_width(win);
	len = get_window_len(win);

	/* Set up all our different formats */
	sprintf(format, "%%-%d.%ds  %%-5.5s  %%-3.3d  %%-8ld  %%-10.10s %%-5.5s",
	width - 42 - MY_INDENT,
	width - 42 - MY_INDENT);
	sprintf(format1, "%%-%d.%ds %%-5.5s", width - 41 - MY_INDENT, width - 41 - MY_INDENT);
	sprintf(format2, "%%s %%-%d.%ds", width - 30 - MY_INDENT, width - 24 - MY_INDENT);
	sprintf(format3, "%%s %%-%d.%ds  %%d%%s", width - 30 - MY_INDENT,
	   width - 30 - MY_INDENT);
	*buff = 0;
	/* Start at the top of window, and start creating output strings */
	for (tmp = top; tmp; tmp = tmp->next) {
#ifdef PANA_TREE_STUFF
		newdir = 0;
		if ((p = strrchr(tmp->filename, '\\')))
		{
			p--;
			q = p;
			while (q != tmp->filename && *q != '\\')
				q--;
			{
				q++;
				if (!*buff2 || strncmp(buff2, q, p - q))
				{
					strncpy(buff2, q, p - q);
					newdir = 1;
				}
			}
		}
#endif
		/* We finished downloading this file */
#ifdef PANA_TREE_STUFF
		if (in_tree)
		{
			if (newdir)
				sprintf(buff, format1, buff2, tmp->nick, n_speed(tmp->speed));
			else
				continue;
		}
		else 
#endif
		if (tmp->flags & DOWNLOADED_FILE) {
			sprintf(buff, format2, "-> Finished Getting ",
			    base_name(tmp->filename));
		}
		/* We are currently not dl'ing this file, use format */
		else if (!tmp->getfile) {
			sprintf(buff, format,
			    base_name(tmp->filename), mp3_time(tmp->seconds),
			    tmp->bitrate, tmp->filesize, tmp->nick, n_speed(tmp->speed));
		}
		/* We are currently dl'ing this file, use format3 */
		else {
			int perc = 0;
			if (tmp->getfile->filesize)
                		perc = (100.0 * (((double)
				    (tmp->getfile->received + tmp->getfile->resume)) /
				    (double) tmp->getfile->filesize));
			sprintf(buff, format3, "-> Getting ",
			    base_name(tmp->filename), perc,
			    "% Done.");
		}
		slen = strlen(buff);
		if (slen > width - c_x)
			slen = width - c_x - 1;
		wmove(win, c_y, c_x);
		if (tmp->getfile)
			wattron(win, make_attr(A_NORMAL, COLOR_MAGENTA, COLOR_BLACK));
		else if (tmp->flags & DOWNLOADED_FILE)
			wattron(win, make_attr(A_NORMAL, COLOR_CYAN, COLOR_BLACK));
		else if (tmp->flags & (BROWSE_TOGGLE|BLOCK_TOGGLE))
			wattron(win, make_attr(A_BOLD, COLOR_CYAN, COLOR_BLACK));
		else;
		waddnstr(win, buff, slen);
		wclrtoeol(win);
		if (tmp->getfile)
			wattroff(win, make_attr(A_NORMAL, COLOR_MAGENTA, COLOR_BLACK));
		else if (tmp->flags & DOWNLOADED_FILE)
			wattroff(win, make_attr(A_NORMAL, COLOR_CYAN, COLOR_BLACK));
		else if (tmp->flags & (BROWSE_TOGGLE|BLOCK_TOGGLE))
			wattroff(win, make_attr(A_BOLD, COLOR_CYAN, COLOR_BLACK));
		else;
		c_y++;
		c_x = MY_INDENT;
		if (c_y > len) {
			bottom = tmp;
			break;
		}
		prev = tmp;
	}
	bottom = prev;
	/* Clear to bottom of screen */
	wclrtobot(win);
	/* return last line we drew, before clearing to bottom */
	return c_y;
}

/* Walk linked list, getting the true bottom link */
static inline void
find_true_bottom(void)
{
	FileStruct *tmp = NULL, *prev = NULL;

	for (tmp = true_top; tmp; tmp = tmp->next)
		prev = tmp;
	true_bottom = prev;
}

static inline void
set_to_hit_bottom(WINDOW *win, int lines_needed)
{
	FileStruct *tmp = NULL, *prev = NULL;
	int i = 0;

	if (lines_needed < 0)
		i = get_window_len(win) - 1;
	else
		i = lines_needed;
	for (tmp = true_bottom; i > 0 && tmp; tmp = tmp->prev, i--) {
		if (block_mode)
			toggle_file(main_win, BLOCK_TOGGLE, FORCE_ON);
		prev = tmp;
	}
	/* Hit top of list, set top to "prev" */
	top = prev;
}	


static inline void
refresh_all_windows(void)
{
	wrefresh(main_win);
	wrefresh(header_win);
	wrefresh(help_win);
}


static inline void
create_header(WINDOW *win)
{
	int width = 0;

	width = get_window_width(win);
	wattron(win, make_attr(A_NORMAL, COLOR_CYAN, COLOR_BLACK));
	/* Add copyright to header window */
	mvwaddstr(win, 0, MY_CENTER(width, strlen(copyright)), copyright);
	wattroff(win, make_attr(A_NORMAL, COLOR_CYAN, COLOR_BLACK));
	wrefresh(win);
}

static inline void
create_help(WINDOW *win, int flag)
{
	char buf[COLS];
	char *ptr;
	int width = 0, len = 0;

	ptr = buf;
	width = get_window_width(win);
	wattron(win, make_attr(A_NORMAL, COLOR_BLUE, COLOR_WHITE));
	if (help_toggle)
		strcpy(buf, help3);
	else
		strcpy(buf, help1);
	len = strlen(buf);
	fill_with_spaces(ptr + len, width - len);
	mvwaddnstr(win, 0, 0, ptr, width);
	if (help_toggle)
		strcpy(buf, help4);
	else
		strcpy(buf, help2);
	if (flag)
		help_toggle = ~help_toggle;
	len = strlen(buf);
	fill_with_spaces(ptr + len, width - len);
	mvwaddnstr(win, 1, 0, ptr, width);
	wattroff(win, make_attr(A_NORMAL, COLOR_BLUE, COLOR_WHITE));
	wrefresh(win);
}

static inline int
get_window_len(WINDOW *win)
{
	int w_x1 = 0, w_y1 = 0;
	int w_x2 = 0, w_y2 = 0;

	getbegyx(win, w_y1, w_x1);
	getmaxyx(win, w_y2, w_x2);
	return (w_y2 - w_y1);
}

static inline int
get_window_width(WINDOW *win)
{
	int w_x1 = 0, w_y1 = 0;
	int w_x2 = 0, w_y2 = 0;

	getbegyx(win, w_y1, w_x1);
	getmaxyx(win, w_y2, w_x2);
	return (w_x2 - w_x1);
}

static inline void
fill_with_spaces(char *ptr, int count)
{
	for (; count > 0; ptr++, count--)
		*ptr = ' ';
}

/* This function takes the line passed in, and correlates it to the screen, and which
 * FileStruct it points to.
 */
static inline FileStruct *
correlate_line_to_file(int line)
{
	FileStruct *tmp;
	for (tmp = top; line > 0 && tmp; tmp = tmp->next, line--)
		;
	return tmp;
}

static inline void
toggle_specific_file(FileStruct *tmp, int type)
{
	if (tmp->flags & type)
		tmp->flags &= ~type;
	else
		tmp->flags |= type;
}

static void
toggle_file(WINDOW *win, int type, int force)
{
	FileStruct *tmp;
	int c_x = 0, c_y = 0;

	getyx(win, c_y, c_x);
	if (!(tmp = correlate_line_to_file(c_y)))
		return;
	/* If its already been downloaded, don't allow it to be toggled */
	if (tmp->flags & DOWNLOADED_FILE)
		return;
	if (tmp->flags & type) {
		if (force != FORCE_ON)
			tmp->flags &= ~type;
	}
	else {
		if (force != FORCE_OFF)
			tmp->flags |= type;
	}
	display_output(win, 0);
	wmove(win, c_y, c_x);
	wrefresh(win);
}

static inline void
convert_block_to_toggle(WINDOW *win)
{
	int c_x = 0, c_y = 0;
	FileStruct *tmp;

	getyx(win, c_y, c_x);
	for (tmp = top; tmp; tmp = tmp->next) {
		if (tmp->flags & BLOCK_TOGGLE) {
			tmp->flags &= ~BLOCK_TOGGLE;
			tmp->flags |= BROWSE_TOGGLE;
		}
	}
	display_output(win, 0);
	wmove(win, c_y, c_x);
	wrefresh(win);
}

static inline void
turn_off_all_block_toggle(void)
{
	FileStruct *tmp;
	for (tmp = top; tmp; tmp = tmp->next)
		tmp->flags &= ~BLOCK_TOGGLE;
}

static inline void
do_actual_get(FileStruct *tmp)
{
	create_and_do_get(tmp, 0, 0);
}


static inline void
do_download_of_one_file(WINDOW *win)
{
	int c_x = 0, c_y = 0;
	FileStruct *tmp;

	getyx(win, c_y, c_x);
	if (!(tmp = correlate_line_to_file(c_y)) || tmp->getfile)
		return;
	do_actual_get(tmp);
}

static inline void
do_download_of_toggles(void)
{
	FileStruct *tmp;
	for (tmp = true_top; tmp; tmp = tmp->next) {
		if (tmp->flags & BROWSE_TOGGLE && !tmp->getfile) {
			tmp->flags &= ~BROWSE_TOGGLE;
			do_actual_get(tmp);
		}
	}
}

static inline void
do_block_toggle(WINDOW *win)
{
	int c_x = 0, c_y = 0;

	if (block_mode == 1) {
		getyx(win, c_y, c_x);
		block_mode = 0;
		turn_off_all_block_toggle();
		display_output(win, 0);
		wmove(win, c_y, c_x);
		wrefresh(win);
	}
	else
		block_mode = 1;
}

static inline void
do_page_block_down(int c_y, int len)
{
	FileStruct *tmp;
	int i = 0;
	for (tmp = top; tmp; tmp = tmp->next, i++) {
		if (i == c_y - 1)
			break;
	}
	i = 0;
	for (; tmp; tmp = tmp->next, i++) {
		if (block_mode)
			toggle_specific_file(tmp, BLOCK_TOGGLE);
		if (i == len - 1)
			break;
	}
}	

static inline void
do_page_block_up(int c_y, int len)
{
	FileStruct *tmp;
	int i = 0;
	for (tmp = top; tmp; tmp = tmp->next, i++) {
		if (i == c_y)
			break;
	}
	i = 0;
	for (; tmp; tmp = tmp->prev, i++) {
		if (block_mode)
			toggle_specific_file(tmp, BLOCK_TOGGLE);
		if (i == len - 2)
			break;
	}
}	

static inline void
cleanup(void)
{
	delwin(header_win);
	delwin(help_win);
	delwin(main_win);
	if (tree_win)
		delwin(tree_win);
#if 0
	wclear(main_win);
#endif
	endwin();
	cpair_count = 0;
	in_browser = 0;
	refresh_screen(0, NULL);
	cursor_to_input();
#ifdef WINNT
	reset_console_mode();
#endif
}

int
make_attr(int att, int fg, int bg)
{
	int i;

	/* Short circuit if term doesn't support colors */
	if (!has_colors()) {
#ifdef MY_DEBUG
		say("Has no colors!");
#endif
		return att;
	}
	/* Look for the pair */
	for (i = 0; i < cpair_count; i++) {
		if (cpairs[i].fg == fg && cpairs[i].bg == bg)
			break;
	}
	/* Do we need to add a new pair? */
	if (i >= cpair_count) {
		if (cpair_count <  COLOR_PAIRS) {
			cpairs[i].fg = fg;
			cpairs[i].bg = bg;
			init_pair(i + 1, fg, bg);
			cpair_count++;
		}
	}
	i++;
#ifdef SOLARIS
	return ((att & ~A_ALTCHARSET) | COLOR_PAIR(i));
#else
	return (att | COLOR_PAIR(i));
#endif
}
#else
BUILT_IN_COMMAND(scott)
{
}
#endif

#else
#if 0

#include "ircterm.h"
#include "screen.h"
#include "window.h"


extern char *n_speed(int);

Window *main_win = NULL;
Window *old_window = NULL;

static NickStruct *curr_nick = NULL;
static FileStruct *top = NULL;
static FileStruct *bottom = NULL;
static FileStruct *true_top = NULL;
static FileStruct *true_bottom = NULL;
static FileStruct *cursor = NULL;

static int help_toggle = 0;
extern int need_redraw;

static char help1[] = "Up/Down: arrow keys.      ^V Next Page.       ^Y Prev Page";
static char help2[] = "^O Download CMDS          ^A Home             ^E End            Q/q Quit.";
static char help3[] = "Enter: Get File          Space: Toggle File.     ^^ Toggle block mode.";
static char help4[] = "^O Movement CMDS         ^W Write Block.         ^D Download Toggled Files";

static void prep_search_globals(void)
{
	FileStruct **f;
	f = get_search_head(from_server);
	cursor = true_top = top = *f;
	curr_nick = NULL;
}

/* Walk linked list, getting the true bottom link */
static inline void find_true_bottom(void)
{
	FileStruct *tmp = NULL, *prev = NULL;

	for (tmp = true_top; tmp; tmp = tmp->next)
		prev = tmp;
	true_bottom = prev;
}

void refresh_browser(void)
{
	refresh_screen(0, NULL);
}	

int display_output(Window *win, int all)
{
FileStruct *tmp = NULL, *prev = NULL;
char buff[BIG_BUFFER_SIZE + 1], format[400 + 1];
char format1[400+1], format2[400 + 1], format3[400 + 1];
char cursor_color[10], *cur_color;
int slen = 0, len = 0, width = 0, c_x = 0, c_y = 0;

	width = win->columns;
	len = win->bottom - win->top;

	/* Set up all our different formats */
	sprintf(format,"%%s%*s%%-%d.%ds  %%-5.5s  %%-3.3d  %%-8lu  %%-10.10s %%-5.5s%%s",
			MY_INDENT, MY_INDENT ? " " : empty_string,
			width - 42 - MY_INDENT,
			width - 42 - MY_INDENT);
	sprintf(format1, "%%s%*s%%-%d.%ds %%-5.5s%%s", 
			MY_INDENT, MY_INDENT ? " " : empty_string,
			width - 41 - MY_INDENT,	width - 41 - MY_INDENT);
	sprintf(format2, "%%s%*s%%s %%-%d.%ds%%s", 
			MY_INDENT, MY_INDENT ? " " : empty_string,
			width - 30 - MY_INDENT, width - 24 - MY_INDENT);
	sprintf(format3, "%%s%*s%%s %%-%d.%ds  %%d%%s%%s", 
			MY_INDENT, MY_INDENT ? " " : empty_string,
			width - 30 - MY_INDENT,	width - 30 - MY_INDENT);

	*buff = 0;
	cur_color = cursor_color;
	*cur_color = 0;

	/* Start at the top of window, and start creating output strings */
	for (tmp = top; tmp; tmp = tmp->next) {
		if (tmp == cursor)
			strcpy(cur_color, REV_TOG_STR);
		else
			*cur_color = 0;
		/* We finished downloading this file */

		if (tmp->flags & DOWNLOADED_FILE) {
			sprintf(buff, format2, cur_color, "-> Finished Getting ",
			    base_name(tmp->filename), cur_color);
		}
		/* We are currently not dl'ing this file, use format */
		else if (!tmp->getfile) {
			sprintf(buff, format, cur_color,
			    base_name(tmp->filename), mp3_time(tmp->seconds),
			    tmp->bitrate, tmp->filesize, tmp->nick, n_speed(tmp->speed), cur_color);
		}
		/* We are currently dl'ing this file, use format3 */
		else {
			int perc = 0;
			if (tmp->getfile->filesize)
                		perc = (100.0 * (((double)
				    (tmp->getfile->received + tmp->getfile->resume)) /
				    (double) tmp->getfile->filesize));
			sprintf(buff, format3, cur_color, "-> Getting ",
			    base_name(tmp->filename), perc,
			    "% Done.", cur_color);
		}

		add_to_display_list(main_win, buff, tmp);
		prev = tmp;
	}
	bottom = prev;
	/* return last line we drew, before clearing to bottom */
	return c_y;
}

void redraw_line(Window *win, Display *next, FileStruct *cursor)
{
char buff[BIG_BUFFER_SIZE + 1], format[400 + 1];
char format1[400+1], format2[400 + 1], format3[400 + 1];
char cursor_color[10], *cur_color;
FileStruct *tmp;
int width, len;

	width = win->columns;
	len = win->bottom - win->top;

	tmp = next->data;

	/* Set up all our different formats */
	sprintf(format,"%%s%*s%%-%d.%ds  %%-5.5s  %%-3.3d  %%-8lu  %%-10.10s %%-5.5s%%s",
			MY_INDENT, MY_INDENT ? " " : empty_string,
			width - 42 - MY_INDENT,
			width - 42 - MY_INDENT);
	sprintf(format1, "%%s%*s%%-%d.%ds %%-5.5s%%s", 
			MY_INDENT, MY_INDENT ? " " : empty_string,
			width - 41 - MY_INDENT,	width - 41 - MY_INDENT);
	sprintf(format2, "%%s%*s%%s %%-%d.%ds%%s", 
			MY_INDENT, MY_INDENT ? " " : empty_string,
			width - 30 - MY_INDENT, width - 24 - MY_INDENT);
	sprintf(format3, "%%s%*s%%s %%-%d.%ds  %%d%%s%%s", 
			MY_INDENT, MY_INDENT ? " " : empty_string,
			width - 30 - MY_INDENT,	width - 30 - MY_INDENT);

	*buff = 0;
	cur_color = cursor_color;
	*cur_color = 0;

	if (tmp == cursor)
		strcpy(cur_color, REV_TOG_STR);
	else
		*cur_color = 0;
	/* We finished downloading this file */

	if (tmp->flags & DOWNLOADED_FILE) {
		sprintf(buff, format2, cur_color, "-> Finished Getting ",
		    base_name(tmp->filename), cur_color);
	}
	/* We are currently not dl'ing this file, use format */
	else if (!tmp->getfile) {
		sprintf(buff, format, cur_color,
		    base_name(tmp->filename), mp3_time(tmp->seconds),
		    tmp->bitrate, tmp->filesize, tmp->nick, n_speed(tmp->speed), cur_color);
	}
	/* We are currently dl'ing this file, use format3 */
	else {
		int perc = 0;
		if (tmp->getfile->filesize)
               		perc = (100.0 * (((double)
			    (tmp->getfile->received + tmp->getfile->resume)) /
			    (double) tmp->getfile->filesize));
		sprintf(buff, format3, cur_color, "-> Getting ",
		    base_name(tmp->filename), perc,
		    "% Done.", cur_color);
	}
	strcpy(next->line, buff);
/*	output_with_count(next->line, 1, 1);*/
}

void do_up(Window *win)
{
Display *next;
int count = 0;
	if (!cursor->prev)
		return;
	for (next = win->top_of_scrollback; next; next = next->next, count++)
	{
		if (next->data == cursor)
		{
			cursor = cursor->prev;
			term_move_cursor(0, count);
			redraw_line(win, next, cursor);
			if (next->next)
			{
				term_move_cursor(0, count+1);
				redraw_line(win, next->next, cursor);
			}
			scrollback_backwards_lines(1);
			return;
		}
	}
}


void do_down(Window *win)
{
Display *next;
int count = 0;
	if (!cursor->next)
		return;
	for (next = win->top_of_scrollback; next; next = next->next, count++)
	{
		if (next->data == cursor)
		{
			cursor = cursor->next;
			term_move_cursor(0, count);
			redraw_line(win, next, cursor);
			if (next->prev)
			{
				term_move_cursor(0, count-1);
				redraw_line(win, next->prev, cursor);
			}
/*			scrollback_forwards_lines(1);*/
			repaint_window(win, count - 1, count+1);
			return;
		}
	}
}

void do_home(Window *win)
{
	scrollback_start(0, NULL);
	cursor = true_top;
}

void do_end(Window *win)
{
	scrollback_end(0, NULL);
	cursor = true_bottom;
}

void do_page_up(Window *win)
{
	scrollback_backwards(0, NULL);
	cursor = win->top_of_display->data;
}

void do_page_down(Window *win)
{
	scrollback_forwards(0, NULL);
	cursor = win->top_of_display->data;
}

void update_browse_cursor(Window *window)
{
Display *next;
	while (window->top_of_scrollback)
	{
		next = window->top_of_scrollback->next;
		new_free(&window->top_of_scrollback->line);
		new_free((char **)&window->top_of_scrollback);
		window->display_buffer_size--;
		window->top_of_scrollback = next;
	}
	window->display_ip = NULL;
	window->top_of_display = main_win->top_of_scrollback = NULL;
	window->ceiling_of_display = window->scrollback_point = 
		main_win->display_ip = NULL;
	window->lines_scrolled_back = window->display_buffer_size = 
		window->distance_from_display = 0;
	resize_window_display(window);
/*	display_output(window, 1);
	resize_window_display(window);*/
}

void browser_main_loop(void)
{
unsigned int cinput = 0;
	cinput = getch();
	switch (cinput)
	{
		case KEY_UP:
		case 'k':
			do_up(main_win);
/*			update_browse_cursor(main_win);*/
			break;
		/* DOWN */
		case KEY_DOWN:
		case 'j':
			do_down(main_win);
/*			update_browse_cursor(main_win);*/
			break;
		/* LEFT */
		case KEY_LEFT:
			break;
		/* RIGHT */
		case KEY_RIGHT:
			break;
		/* HOME */
		case CTRL_A:
	 	case KEY_HOME:
			do_home(main_win);
			update_browse_cursor(main_win);
			break;
		/* END */
		case CTRL_E:
		case KEY_END:
			do_end(main_win);
			update_browse_cursor(main_win);
			break;
		/* NEXT PAGE */
		case CTRL_F:
		case KEY_NPAGE:
		case CTRL_V:
			do_page_down(main_win);
			update_browse_cursor(main_win);
			break;
		/* PREV PAGE */
		case CTRL_B:
		case KEY_PPAGE:
		case CTRL_Y:
			do_page_up(main_win);
			update_browse_cursor(main_win);
			break;
		case 'Q':
		case 'q':
			if (curr_nick) 
			{
				curr_nick->top = top;
				curr_nick->bottom = bottom;		
				curr_nick = NULL;
			}
			in_browser = 0;
			delete_window(main_win);
			main_win = NULL;
			make_window_current(old_window);
			old_window = NULL;
			endwin();
		default:
			/* put_it("%d", cinput);*/
			break;
	}
/*	refresh_screen(0, NULL);*/
	return;
}

BUILT_IN_COMMAND(scott)
{
	/* init vars to 0 */
	NickStruct *n;
	Window *owd = current_window;
	int old_wd = window_display;
		
	char *nick = NULL;

	old_window = current_window;
		
	top = NULL;
	bottom = NULL;
	true_top = NULL;
	true_bottom = NULL;
	help_toggle = 0;
#if 0
	block_mode = 0;
	in_tree = 0;
#endif
	/* Since its possible 3 commands could come here. We need
	 * to check which one of the 3 the user wants.
	 */
	if(command && *command) {
		/* First command is sbrowse. It automagically goes to the browse list */
		if (!my_strnicmp(command, "sbrowse", 3)) {
			if (!(nick = next_arg(args, &args))) {
				say("Please specify a nick or -search!");
				return;
			}
		}
		/* 2nd command is sbrowse. It automagically goes to the search list */
		else if (!my_strnicmp(command, "ssearch", 3)) {
			prep_search_globals();
			goto start;
		}
		/* 3rd command is scott. It does the old checking for -search in the nick */
		else if (!my_strnicmp(command, "scott", 3)) {
			if (!(nick = next_arg(args, &args))) {
				say("Please specify a nick or -search!");
				return;
			}
			if (!my_strnicmp(nick, "-search", 7)) {
				prep_search_globals();
				goto start;
			}
		}
		/* Unknown command, return without doing anything */
		else {
			return;
		}
	}


	if ((n = (NickStruct *)find_in_list((List **)&server_list[from_server].users, nick, 0)))
	{
		if (n->flag & BROWSE_IN_PROGRESS)
		{
			say("Can't do that while a browse is in progress");
			return;
		}
		true_top = top = n->file_browse;
		/* Save a pointer to the Nick struct */
		curr_nick = n;
	}
start:
	/* No list, bail out */
	if (!true_top) {
		say("File Browser error. No files in list!");
		curr_nick = NULL;
		return;
	}
	/* Set global flag, telling client that we are in the file browser */
	in_browser = 1;

	initscr();
	nonl();
	/* Set up usual curses stuff */
	noecho();
	cbreak();
	keypad(stdscr, TRUE);

	/* Create and move main into place. Already current*/
	if (!(main_win = new_window(NULL)))
		return;
			
	malloc_strcpy(&main_win->status.line[0].raw, help1);
	malloc_strcpy(&main_win->status.line[1].raw, help2);
	malloc_strcpy(&main_win->status.line[2].raw, help3);
	main_win->status.double_status = 1;
	main_win->display_size--;
	build_status_format(&(main_win->status), 0);
	build_status_format(&(main_win->status), 1);
	build_status_format(&(main_win->status), 2);

	swap_window(owd, main_win);
	recalculate_windows(main_win->screen);
	update_all_status(current_window, NULL, 0);

	/* Walk the list, setting true_bottom to the end of list */
	find_true_bottom();

	if (curr_nick && curr_nick->top)
		top = curr_nick->top;
	else
		top = true_top;
	/* Dump out our list to the main window */

	display_output(main_win, 1);
	scrollback_start(0, NULL);
}
#else
void browser_main_loop(void)
{
}
void refresh_browser(void)
{
}

BUILT_IN_COMMAND(scott)
{
}
#endif
#endif /* SCOTT */

#endif /* GUI && __EMX__ */

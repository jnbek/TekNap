/*
 * output.c: handles a variety of tasks dealing with the output from the irc
 * program 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 * $Id: output.c,v 1.1.1.1 2001/01/31 17:53:39 edwards Exp $
 */

#include "teknap.h"
#include "struct.h"              
#include <sys/stat.h>
#include <stdarg.h>

#include "log.h"
#include "output.h"
#include "vars.h"
#include "input.h"
#include "ircaux.h"
#include "ircterm.h"
#include "napster.h"
#include "window.h"

/* make this buffer *much* bigger than needed */
#define LARGE_BIG_BUFFER_SIZE BIG_BUFFER_SIZE * 3

static	char	putbuf[LARGE_BIG_BUFFER_SIZE + 1];
char three_stars[4] = "***";
char *thing_ansi = NULL;
extern int current_numeric;;




/* unflash: sends a ^[c to the screen */
/* Must be defined to be useful, cause some vt100s really *do* reset when
   sent this command. >;-) */

/* functions which switch the character set on the console */
/* ibmpc is not available on the xterm */

void charset_ibmpc (void)
{
	fwrite("\033(U", 3, 1, current_ftarget);	/* switch to IBM code page 437 */
}

void charset_lat1 (void)
{
	fwrite("\033(B", 3, 1, current_ftarget);	/* switch to Latin-1 (ISO 8859-1) */
}

void charset_cst(void)
{
	fwrite("\033(K", 3, 1, current_ftarget); /* switch too user-defined */
}

/* currently not used. */



/*
 * refresh_screen: Whenever the REFRESH_SCREEN function is activated, this
 * swoops into effect 
 */
void refresh_screen (unsigned char dumb, char *dumber)
{
extern int need_redraw;

	term_clear_screen();

	if (term_resize())
		recalculate_windows(current_window->screen);
	else
		redraw_all_windows();
	if (need_redraw)
		need_redraw = 0;
	update_all_windows();
		
	update_input(UPDATE_ALL);
}

/*
 * refresh_window_screen: Updates and redraws only the window's 
 * screen that was passed as a parameter. 
 */

/* init_windows:  */
int init_screen (void)
{
extern int term_initialized;
#if 0
	if (!putbuf)
		putbuf = new_malloc(LARGE_BIG_BUFFER_SIZE+1);
#endif
	term_initialized = 1;
	term_clear_screen();
	term_resize();
	create_new_screen();
	new_window(main_screen);
	update_all_windows();
	term_move_cursor(0, 0);
	return 1;
}

void put_echo (char *str)
{
	add_to_log(naplog_fp, -1, str, 0);
	add_to_screen(str);
}


/*
 * put_it: the irc display routine.  Use this routine to display anything to
 * the main irc window.  It handles sending text to the display or stdout as
 * needed, add stuff to the lastlog and log file, etc.  Things NOT to do:
 * Dont send any text that contains \n, very unpredictable.  Tabs will also
 * screw things up.  The calling routing is responsible for not overwriting
 * the 1K buffer allocated.  
 *
 * For Ultrix machines, you can't call put_it() with floating point arguements.
 * It just doesn't work.  - phone, jan 1993.
 */
void put_it(const char *format, ...)
{
	if (window_display && format)
	{
		va_list args;
		memset(putbuf, 0, 200);
		va_start (args, format);
		vsnprintf(putbuf, LARGE_BIG_BUFFER_SIZE, format, args);
		va_end(args);
		if (*putbuf)
			put_echo(putbuf);
	}
}

/* This is an alternative form of put_it which writes three asterisks
 * before actually putting things out.
 */
void say (const char *format, ...)
{
int len = 0;
	if (window_display && format)
	{
		va_list args;
		va_start (args, format);
		len = strlen(numeric_banner(-current_numeric));
#if 0
		if (thing_ansi)
			len = strlen(thing_ansi);
		else
			len = 3;
#endif
		vsnprintf(&(putbuf[len+1]), LARGE_BIG_BUFFER_SIZE, format, args);
		va_end(args);
		strcpy(putbuf, numeric_banner(-current_numeric));
		putbuf[len] = ' ';
		put_echo(putbuf);
	}
}

void bitchsay (const char *format, ...)
{
int len;
	if (window_display && format)
	{
		va_list args;
		va_start (args, format);
		sprintf(putbuf, "%s \002%s\002: ", thing_ansi?thing_ansi:three_stars, version);
		len = strlen(putbuf);
		vsnprintf(&(putbuf[len]), LARGE_BIG_BUFFER_SIZE, format, args);
		va_end(args);
		put_echo(putbuf);
	}
}

void	yell(const char *format, ...)
{
	if (format)
	{
		va_list args;
		va_start (args, format);
		*putbuf = 0;
		vsnprintf(putbuf, LARGE_BIG_BUFFER_SIZE, format, args);
		va_end(args);
		if (*putbuf)
			put_echo(putbuf);
	}
}


/*
 * Error is exactly like yell, except that if the error occured while
 * you were loading a script, it tells you where it happened.
 */
void 	naperror (const char *format, ...)
{
	if (format)
	{
		va_list args;
		va_start (args, format);
		vsnprintf(putbuf, LARGE_BIG_BUFFER_SIZE, format, args);
		va_end(args);
		put_echo(putbuf);
	}
}

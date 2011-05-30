/*
 * screen.c
 *
 * Written By Matthew Green, based on window.c by Michael Sandrof
 * Copyright(c) 1993 Matthew Green
 * Significant modifications by Jeremy Nelson
 * Copyright 1997 EPIC Software Labs,
 * Significant additions by J. Kean Johnston
 * Copyright 1998 J. Kean Johnston, used with permission
 * Modifications Copyright Colten Edwards 1997-1998
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 * $Id: screen.c,v 1.1.1.1 2001/01/31 17:53:49 edwards Exp $
 */


#include "teknap.h"
#include "struct.h"
#include "log.h"
#include "hook.h"
#include "screen.h"
#include "server.h"
#include "status.h"
#include "window.h"
#include "output.h"
#include "vars.h"
#include "list.h"
#include "ircterm.h"
#include "ircaux.h"
#include "input.h"
#include "newio.h"

	Screen	*main_screen = NULL;
	Screen	*last_input_screen = NULL;
	Screen	*output_screen = NULL;
	Window	*debugging_window = NULL;

	
	int	extended_handled = 0;
extern	int	foreground;
	
/* Our list of screens */
Screen	*screen_list = NULL;

	int	strip_ansi_never_xlate = 0;
	
	u_char	**prepare_display	(const u_char *, int, int *, int);
	int	add_to_display_list	(Window *, const unsigned char *, void *);
	Display	*new_display_line	(Display *);
	void	add_to_window		(Window *, const unsigned char *);

static	char	display_standout	(int flag);
static	char	display_bold		(int flag);
static	char	display_blink		(int flag);
static	char	display_underline	(int flag);
static	int	display_color		(long color1, long color2);
static	void	display_normal		(void);
static	char	display_altcharset	(int flag);

static	char	*replace_color		(int, int);

const	u_char	*skip_ctl_c_seq		(const u_char *start, int *lhs, int *rhs, int proper);



unsigned char *skip_incoming_mirc(unsigned char *buffer)
{
int lhs = -1, rhs = -1;
register unsigned char *p = buffer;
register unsigned char *end;
	while (*p)
	{
		if (*p == 3)
		{
			unsigned char *save_p = p;
			lhs = rhs = -1;
			save_p = end = (char *)skip_ctl_c_seq (p, &lhs, &rhs, 1);
			while(*end)
				*p++ = *end++;
			*p = 0;
			p = save_p;
		}
		else
			p++;
	}
	*p = 0;
	return buffer;
}

/*
 * add_to_screen: This adds the given null terminated buffer to the screen.
 * That is, it determines which window the information should go to, which
 * lastlog the information should be added to, which log the information
 * should be sent to, etc 
 */
void add_to_screen(unsigned char *buffer)
{
char *out = NULL;

	out = buffer;		

	if (in_window_command)
	{
		in_window_command = 0;
		update_all_windows();
		in_window_command = 1;
	}

	if (!current_window)
	{
		puts(out);
		return;
	}
	if (target_window)
		target_window->output_func(target_window, out);
	else
		current_window->output_func(current_window, out);

}


/*
 * add_to_window: adds the given string to the display.  No kidding. This
 * routine handles the whole ball of wax.  It keeps track of what's on the
 * screen, does the scrolling, everything... well, not quite everything...
 * The CONTINUED_LINE idea thanks to Jan L. Peterson (jlp@hamblin.byu.edu)  
 *
 * At least it used to. Now most of this is done by split_up_line, and this
 * function just dumps it onto the screen. This is because the scrollback
 * functions need to be able to figure out how to split things up too.
 */
void add_to_window(Window *window, const unsigned char *str)
{

	if (do_hook(WINDOW_LIST, "%u %s", window->refnum, str))
	{
		unsigned char   **lines;
		int	cols;
		int	must_free = 0;
		char *pend;
		
		if ((pend = get_string_var(OUTPUT_REWRITE_VAR)))
		{
			char	*prepend_exp;
			char	argstuff[10241];
			int	args_flag;

			/* First, create the $* list for the expando */
			snprintf(argstuff, 10240, "%u %s", 
					window->refnum, str);

			/* Now expand the expando with the above $* */
			prepend_exp = expand_alias(pend, argstuff,
						   &args_flag, NULL);

			str = prepend_exp;
			must_free = 1;
		}

		add_to_log(window->log_fp, window->refnum, str, window->mangler);
		add_to_lastlog(window, str);
		display_standout(OFF);
		display_bold(OFF);

		if (window->screen)
			cols = window->screen->co-1;
		else
			cols = window->columns-1;

		for (lines = split_up_line(str, cols); *lines; lines++)
		{
			if (add_to_display_list(window, *lines, NULL))
				rite(window, *lines);
		}

		term_flush();
		term_all_off();

		/*
		 * This used to be in rite(), but since rite() is a general
		 * purpose function, and this stuff really is only intended
		 * to hook on "new" output, it really makes more sense to do
		 * this here.  This also avoids the terrible problem of 
		 * recursive calls to split_up_line, which are bad.
		 */
		if (!window->visible)
		{
			/*
			 * This is for archon -- he wanted a way to have 
			 * a hidden window always beep, even if BEEP is off.
			 */
			if (window->beep_always && strchr(str, '\007'))
			{
				Window *old_target_window = target_window;
				target_window = current_window;
				term_beep();
				say("Beep in window %d", window->refnum);
				target_window = old_target_window;
			}

			/*
			 * Tell some visible window about our problems 
			 * if the user wants us to.
			 */
			if (!(window->miscflags & WINDOW_NOTIFIED)/* &&
				who_level & window->notify_level*/)
			{
				window->miscflags |= WINDOW_NOTIFIED;
				if (window->miscflags & WINDOW_NOTIFY)
				{
					Window	*old_target_window = target_window;
					int	lastlog_level;

					lastlog_level = set_lastlog_msg_level(LOG_CRAP);
					target_window = current_window;
					say("Activity in window %d", 
						window->refnum);
					target_window = old_target_window;
					set_lastlog_msg_level(lastlog_level);
				}
				update_all_status(current_window, NULL, 0);
			}
		}
		if (must_free)
			new_free(&str);
		cursor_in_display(window);
	}
}


u_char **split_up_line (const unsigned char *str, int max_cols)
{
	int nl = 0;
	return prepare_display(str, max_cols, &nl, 0);
}


/*
 * Given an input string, prepare it for display. This involves several
 * phases. We parse through the original string and prepare data in
 * a list of strings, which are returned as this functions return value.
 * During the parsing of the string, we adhere to the maximum width of
 * the string, and to the user variables CONTINUED_LINE and INDENT.
 * When calculating the width of the string, we do not take into account
 * "invisible" characters, such as color and attribute setting characters.
 *
 * If we encounter an ANSI escape sequence starting, we decide how to
 * handle it. If COLOR_MODE is 0, we silently drop all ANSI characters.
 * if it is 1, we convert ANSI escape sequences which set color to the
 * sequences required by the current terminal. If mode is 2, we leave these
 * escape sequences alone and pass them through.
 *
 * Next, if we encounter an upper ASCII character, or a lower ASCII character
 * that is not a format-specific character, we check the value of the user
 * variable DISPLAY_PC_CHARACTERS. This variable is used as follows:
 * 0 - ignore the character completely.
 * 1 - if the terminal has a specific escape sequence for displaying PC
 *     ROM characters, use that sequence for displaying the character.
 *     Otherwise, display the character in inverse. If the character is
 *     >= 128, display it in bold inverse.
 * 2 - if the terminal has a specific escape sequence for displaying
 *     PC ROM characters, display the character with that sequence, otherwise
 *     strip the character from the output string completely.
 * 3 - insert the character exactly as is, and reply on the users terminal
 *     displaying it correctly as a graphics character (pass-through mode).
 *
 * If we detect the beginning of a Ctrl-C color sequence, we pick up the
 * color values, and then examine the user variable COLOR_CONTROL_C.
 * If this value is 0, we ignore the sequences completely, stripping them
 * from the output string. If it is 1, and our terminal supports color,
 * replace the color sequence with the terminal's color escape sequence.
 * If the terminal does not support color, strip them from the output
 * string. If the value is 2, convert these escape sequences explicitly
 * into ANSI escape sequences.
 *
 * While we are preparing the output strings, we take special care not to
 * extend the maximum column width of the output display. We will return
 * as many strings as we need to in order to display the string, and we
 * will wrap as intelligently as we know how.
 *
 * This function can also be run in several ways, and the mode parameter
 * controls this. In the default mode (mode 0) we preserve ^B, ^C, ^F,
 * ^I, ^V and ^_ characters in the string. This assumes that some other
 * function will be doing the output interpretation of the strings.
 * In this mode, depending on the conversion behaviour defined above,
 * we will always convert ANSI color escape sequences to ^C sequences,
 * and we will introduce a new sequence, ^R, for displaying PC ROM
 * characters. ^R is always followed by 3 decimal digits, ALWAYS.
 * Of course, if pass-through is in effect for graphics characters,
 * the raw characters are displayed. To support all of this lot, we
 * provide another function, display_final, which will actually insert
 * all of the right codes for the colors, highlights etc.
 *
 * In mode 1, we prepare a string which is capable of being sent directly
 * to the display, with all formatting in place. Thus the string can be
 * sent with a single fputs(), which is much quicker than the looping
 * one which itteratively does a term_putchar().
 *
 * It may be a good idea to allow users to control this behaviour, possibly
 * by introducing a new variable, DISPLAY_MODE. 0 is the traditional way
 * and 1 is the quick way. Anyway, we dont care at this level, as we are
 * simply preparing the strings here.
 *
 * Errm ... oh yes, and since this caught me by surprise and caused many a
 * minutes worth of hair pulling, dont forget we need to keep count of
 * PRINTED Characters not buffer positions :-)
 */
#define SPLIT_EXTENT 40
unsigned char **prepare_display(const unsigned char *orig_str,
                                int max_cols,
                                int *lused,
                                int flags)
{
	int	gchar_mode;
static 	int 	recursion = 0, 
		output_size = 0;
	int 	pos = 0,            /* Current position in "buffer" */
		col = 0,            /* Current column in display    */
		word_break = 0,     /* Last end of word             */
		indent = 0,         /* Start of second word         */
		firstwb = 0,
		beep_cnt = 0,       /* Number of beeps              */
		beep_max,           /* Maximum number of beeps      */
		tab_cnt = 0,        /* TAB counter                  */
		tab_max,            /* Maximum number of tabs       */
		nds_count = 0,
		nds_max,
		line = 0,           /* Current pos in "output"      */
		len, i,             /* Used for counting tabs       */
		do_indent,          /* Use indent or continued line? */
		in_rev = 0,         /* Are we in reverse mode?      */
		newline = 0;        /* Number of newlines           */
static	u_char 	**output = NULL;
const 	u_char	*ptr = NULL;
	u_char 	buffer[BIG_BUFFER_SIZE + 1],
		*cont_ptr = NULL,
		*cont = empty_string,
		c,
		*words = NULL,
		*str = NULL;

	if (recursion)
		nappanic("prepare_display() called recursively");
	recursion++;

	gchar_mode = get_int_var(DISPLAY_PC_CHARACTERS_VAR);
	beep_max = get_int_var(BEEP_VAR)? get_int_var(BEEP_MAX_VAR) : -1;
	tab_max = get_int_var(TAB_VAR) ? get_int_var(TAB_MAX_VAR) : -1;
	nds_max = get_int_var(ND_SPACE_MAX_VAR);
	do_indent = get_int_var(INDENT_VAR);
	words = (char *)get_string_var(WORD_BREAK_VAR);

	if (!words)
		words = ", ";
	if (!(cont_ptr = (char *)get_string_var(CONTINUED_LINE_VAR)))
		cont_ptr = (char *)empty_string;

	buffer[0] = 0;

	/* Handle blank or non-existant lines */
	if (!orig_str || !orig_str[0])
		orig_str = space;

	if (!output_size)
	{
		int 	new_i = SPLIT_EXTENT;
		RESIZE(output, char *, new_i);
		while (output_size < new_i)
			output[output_size++] = 0;
	}

  /*
   * Before we do anything else, we convert the string to a normalized
   * string. Until such time as I can do this all properly, this is
   * the best effort we can make. The ultimate goal is to have the
   * ANSI stripper / converter split everything into multiple lines,
   * which will decrease the amounT of time spent calculating split
   * lines. For now though, we simply "normalize" the line before we
   * start working with it. This will convert ANSI escape sequences
   * into easy-to-work-with Ctrl-C sequences, or IRCI-II formatting
   * characters.
   * The stripper is a separate function for right now to make
   * debugging easier. This code will be improved, this is proof-of-concept
   * code only, right now.
   */
	str = strip_ansi (orig_str);

	/*
	 * Start walking through the entire string.
	 */
	for (ptr = str; *ptr && (pos < BIG_BUFFER_SIZE - 8); ptr++)
	{
		switch (*ptr)
		{
			case '\007':      /* bell */
			{
				beep_cnt++;
				if ((beep_max == -1) || (beep_cnt > beep_max))
				{
					if (!in_rev)
						buffer[pos++] = REV_TOG;
					buffer[pos++] = (*ptr & 127) | 64;
					if (!in_rev)
						buffer[pos++] = REV_TOG;
					col++;
				}
				else
					buffer[pos++] = *ptr;

				break; /* case '\a' */
			}
			case '\011':    /* TAB */
			{
				tab_cnt++;
				if ((tab_max > 0) && (tab_cnt > tab_max))
				{
					if (!in_rev)
						buffer[pos++] = REV_TOG;
					buffer[pos++] = (*ptr & 0x7f) | 64;
					if (!in_rev)
						buffer[pos++] = REV_TOG;
					col++;
				}
				else
				{
					if (indent == 0)
						indent = -1;
					word_break = pos;

				/* Only go as far as the edge of the screen */
					len = 8 - (col % 8);
					for (i = 0; i < len; i++)
					{
						buffer[pos++] = ' ';
						if (col++ >= max_cols)
							break;
					}
				}
				break; /* case '\011' */
			}
			case ND_SPACE:
			{
				nds_count++;
				/*
				 * Just swallop up any ND's over the max
				 */
				if ((nds_max > 0) && (nds_count > nds_max))
					;
				else
					buffer[pos++] = ND_SPACE;
				col++;
				break;
			}
			case '\n':      /* Forced newline */
			{
				newline = 1;
				if (indent == 0)
					indent = -1;
				word_break = pos;
				break; /* case '\n' */
			}
			case '\003':
			{
				int lhs = 0, rhs = 0;
				const u_char *end = skip_ctl_c_seq(ptr, &lhs, &rhs, 0);
				while (ptr < end)
					buffer[pos++] = *ptr++;
				ptr = end - 1;
				break; /* case '\003' */
			}
			case ROM_CHAR:
			{
				/*
				 * Copy the \r and the first three digits that
				 * are after it.  Careful!  There may not be
				 * three digits after it!  If there arent 3
				 * chars, we fake it with zeros.  This is the
				 * wrong thing, but its better than crashing.
				 */
				buffer[pos++] = *ptr++;  /* Copy the \R ...  */
				if (*ptr)
					buffer[pos++] = *ptr++;
				else
					buffer[pos++] = '0';
				if (*ptr)
					buffer[pos++] = *ptr++;
				else
					buffer[pos++] = '0';
				if (*ptr)
					buffer[pos++] = *ptr;
				else
					buffer[pos++] = '0';

				col++;   /* This is a printable   */
				break; /* case ROM_CHAR */
			}

			case UND_TOG:
			case ALL_OFF:
			case REV_TOG:
			case BOLD_TOG:
			case BLINK_TOG:
			case ALT_TOG:
			{
				buffer[pos++] = *ptr;
				if (*ptr == ALL_OFF)
					in_rev = 0;
				else if (*ptr == REV_TOG)
					in_rev = !in_rev;
				break;
			}

			default:
			{
				if (*ptr == ' ' || strchr(words, *ptr))
				{
					if (indent == 0)
					{
						indent = -1;
						firstwb = pos;
					}
					if (*ptr == ' ')
						word_break = pos;
					if (*ptr != ' ' && ptr[1] &&
					    (col + 1 < max_cols))
						word_break = pos + 1;
					buffer[pos++] = *ptr;
				}
				else
				{
					if (indent == -1)
						indent = col;
					buffer[pos++] = *ptr;
				}
				col++;
				break;
			}
		} /* End of switch (*ptr) */

		/*
		 * Must check for cols >= maxcols+1 becuase we can have a 
		 * character on the extreme screen edge, and we would still 
		 * want to treat this exactly as 1 line, and cols has already 
		 * been incremented.
		 */
		if ((col >= max_cols) || newline)
		{
			unsigned char *pos_copy;
			
			if (col > max_cols)
				col--;
			if (ptr[1] == 0)
				break;
			if (!word_break || (flags & PREPARE_NOWRAP))
				word_break = pos /*pos - 1*/;
			else if (col > max_cols)
				word_break = pos - 1;
								
			/*
			 * XXXX Massive hackwork here.
			 *
			 * Due to some ... interesting design considerations,
			 * if you have /set indent on and your first line has
			 * exactly one word seperation in it, then obviously
			 * there is a really long "word" to the right of the 
			 * first word.  Normally, we would just break the 
			 * line after the first word and then plop the really 
			 * big word down to the second line.  Only problem is 
			 * that the (now) second line needs to be broken right 
			 * there, and we chew up (and lose) a character going 
			 * through the parsing loop before we notice this.  
			 * Not good.  It seems that in this very rare case, 
			 * people would rather not have the really long word 
			 * be sent to the second line, but rather included on 
			 * the first line (it does look better this way), 
			 * and so we can detect this condition here, without 
			 * losing a char but this really is just a hack when 
			 * it all comes down to it.  Good thing its cheap. ;-)
			 */
			if (!*cont && (firstwb == word_break) && do_indent) 
				word_break = pos;

			/*
			 * If we are approaching the number of lines that
			 * we have space for, then resize the master line
			 * buffer so we dont run out.
			 */

			if (line >= output_size - 3)
			{
				int new_i = output_size + SPLIT_EXTENT;
				RESIZE(output, char *, new_i);
				while (output_size < new_i)
					output[output_size++] = 0;
			}

			if (!*cont)
			{
				int	lhs_count = 0;
				int	continued_count = 0;

				if (do_indent && (indent < (max_cols / 3)) &&
						(strlen(cont_ptr) < indent))
				{
					cont = alloca(indent+10); /* sb pana */
					sprintf(cont, "%-*s", indent, cont_ptr);
				}

				/*
				 * Otherwise, we just use /set continued_line, 
				 * whatever it is.
				 */
				else if (!*cont && *cont_ptr)
					cont = cont_ptr;


				/*
				 * XXXX "line wrap bug" fix.  If we are here,
				 * then we are between the first and second
				 * lines, and we might have a word that does
				 * not fit on the first line that also does
				 * not fit on the second line!  We check for
				 * that right here, and if it won't fit on
				 * the next line, we revert "word_break" to
				 * the current position.
				 *
				 * THIS IS UNFORTUNATELY VERY EXPENSIVE! :(
				 */
				c = buffer[word_break];
				buffer[word_break] = 0;
				lhs_count = output_with_count(buffer, 0, 0);
				buffer[word_break] = c;
				continued_count = output_with_count(cont, 0, 0);

				/* 
				 * Chop the line right here if it will
				 * overflow the next line.
				 */
				if (lhs_count <= continued_count)
					word_break = pos;

				/*
				 * XXXX End of nasty nasty hack.
				 */
			}
			c = buffer[word_break];
			buffer[word_break] = 0;
			malloc_strcpy((char **)&(output[line++]), buffer);
			buffer[word_break] = c;

			while (buffer[word_break] == ' ' && word_break < pos)
				word_break++;
			buffer[pos] = 0;

			pos_copy = LOCAL_COPY(buffer+word_break);
			
			strlcpy (buffer, cont, BIG_BUFFER_SIZE/2);
			strlcat (buffer, pos_copy, BIG_BUFFER_SIZE/2);
			pos = strlen(buffer);
			col = output_with_count(buffer, 0, 0);

			word_break = 0;
			newline = 0;

			/* Only allow N lines... */
			if (*lused && line >= *lused)
			{
				*buffer = 0;
				break;
			}
		}
	} /* End of (ptr = str; *ptr && (pos < BIG_BUFFER_SIZE - 8); ptr++) */

	buffer[pos++] = ALL_OFF;
	buffer[pos] = 0;
	if (*buffer)
		malloc_strcpy((char **)&(output[line++]),buffer);

	recursion--;
	new_free(&output[line]);
	*lused = line-1;
	new_free(&str);
	return output;
}


/*
 * This puts the given string into a scratch window.  It ALWAYS suppresses
 * any further action (by returning a FAIL, so rite() is not called).
 */
static	int	add_to_scratch_window_display_list (Window *window, const unsigned char *str, void *data)
{
	Display *my_line, *my_line_prev;
	int cnt;

	if (window->scratch_line == -1)
		nappanic("This is not a scratch window.");
	if (!window->display_size)
		return 0;

	if (window->scratch_line >= window->display_size)
		nappanic("scratch_line is too big.");

	my_line = window->top_of_display;
	my_line_prev = NULL;

	for (cnt = 0; cnt < window->scratch_line; cnt++)
	{
		if (my_line == window->display_ip)
		{
			if (my_line_prev)
			{
				my_line_prev->next = new_display_line(my_line_prev);
				my_line = my_line_prev->next;
				my_line->next = window->display_ip;
				window->display_ip->prev = my_line;
				window->display_buffer_size++;
			}
			else
			{
				my_line = new_display_line(NULL);
				window->top_of_display = my_line;
				window->top_of_scrollback = my_line;
				my_line->next = window->display_ip;
				window->display_ip->prev = my_line;
			}
		}

		my_line_prev = my_line;
		my_line = my_line->next;
	}
	/* XXXX DO this right!!!! XXXX */
	if (my_line == window->display_ip)
	{
		my_line = new_display_line(window->display_ip->prev);
		if (window->display_ip->prev)
			window->display_ip->prev->next = my_line;
		my_line->next = window->display_ip;
		window->display_ip->prev = my_line;
		window->display_buffer_size++;
	}

	malloc_strcpy(&my_line->line, str);
	window->cursor = window->scratch_line;

	window->scratch_line++;
	if (window->scratch_line >= window->display_size)
		window->scratch_line = 0;	/* Just go back to top */
	if (window->noscroll)
	{
		if (window->scratch_line == 0)
			my_line = window->top_of_display;
		else if (my_line->next != window->display_ip)
			my_line = my_line->next;
		else
			my_line = NULL;

		if (my_line)
			malloc_strcpy(&my_line->line, empty_string);
	}


	/*
	 * Make sure the cursor ends up in the right spot
	 */
	if (window->visible)
	{
		window->screen->cursor_window = window;
		term_move_cursor(0, window->top + window->cursor);
		term_clear_to_eol();
		output_line(str);
		display_standout(OFF);
		if (window->noscroll)
		{
		    term_move_cursor(0, window->top + window->scratch_line);
		    term_clear_to_eol();
		}
	}

	window->cursor = window->scratch_line;
	return 0;		/* Express a failure */
}

 
/*
 * This function adds an item to the window's display list.  If the item
 * should be displayed on the screen, then 1 is returned.  If the item is
 * not to be displayed, then 0 is returned.  This function handles all
 * the hold_mode stuff.
 */
int 	add_to_display_list (Window *window, const unsigned char *str, void *data)
{
	if (window->scratch_line != -1)
		return add_to_scratch_window_display_list(window, str, data);

	/* Add to the display list */
	window->display_ip->next = new_display_line(window->display_ip);
	malloc_strcpy(&window->display_ip->line, str);
	window->display_ip->data = data;
	window->display_ip = window->display_ip->next;
	window->display_buffer_size++;
	window->distance_from_display++;
	
	/*
	 * Only output the line if hold mode isnt activated
	 */
	if (((window->distance_from_display > window->display_size) &&
						window->scrollback_point) ||
		(window->hold_mode &&
  			(++window->held_displayed > window->display_size)) ||
  		(window->holding_something && window->lines_held))
	{
		if (!window->holding_something)
			window->holding_something = 1;
			
		window->lines_held++;
		if (window->lines_held >= window->last_lines_held + 10)
		{
			update_window_status(window, 0);
			window->last_lines_held = window->lines_held;
		}
		return 0;
	}

	if (window->in_more && window->hold_mode && window->last_lines_held && 
		!window->lines_held && !window->holding_something && 
		window->held_displayed)
	{
		/* if we get here, it should mean that no more lastlog more
		 * is possible, reset the values.
		 */
		 reset_hold_mode(window);
	}

	/*
	 * Before outputting the line, we snip back the top of the
	 * display buffer.  (The fact that we do this only when we get
	 * to here is what keeps whats currently on the window always
	 * active -- once we get out of hold mode or scrollback mode, then
	 * we truncate the display buffer at that point.)
	 */
	while (window->display_buffer_size > window->display_buffer_max)
	{
		Display *next = window->top_of_scrollback->next;
		delete_display_line(window->top_of_scrollback);
		window->top_of_scrollback = next;
		window->display_buffer_size--;
	}

	/*
	 * And tell them its ok to output this line.
	 */
	return 1;
}



/*
 * rite: this routine displays a line to the screen adding bold facing when
 * specified by ^Bs, etc.  It also does handle scrolling and paging, if
 * SCROLL is on, and HOLD_MODE is on, etc.  This routine assumes that str
 * already fits on one screen line.  If show is true, str is displayed
 * regardless of the hold mode state.  If redraw is true, it is assumed we a
 * redrawing the screen from the display_ip list, and we should not add what
 * we are displaying back to the display_ip list again. 
 *
 * Note that rite sets display_highlight() to what it was at then end of the
 * last rite().  Also, before returning, it sets display_highlight() to OFF.
 * This way, between susequent rites(), you can be assured that the state of
 * bold face will remain the same and the it won't interfere with anything
 * else (i.e. status line, input line). 
 */
int rite(Window *window, const unsigned char *str)
{
static  int     high = OFF;
static  int     bold = OFF;
static  int     undl = OFF;
static  int     blink = OFF;
static	int	altc = OFF;

	output_screen = window->screen;
	scroll_window(window);

	if (window->visible && foreground && window->display_size)
	{
		display_standout(high);
		display_bold(bold);
		display_blink(blink);
		display_underline(undl);
		display_altcharset(altc);
		tputs_x(replace_color(-2, -2));		/* Reinstate color */

		output_line(str);

		high = display_standout(OFF);
		bold = display_bold(OFF);
		undl = display_underline(OFF);
		blink = display_blink(OFF);
		altc = display_altcharset(OFF);

		term_newline();
		term_cr();
	}

	window->cursor++;
	return 0;
}



/*
 * A temporary wrapper function for backwards compatibility.
 */
int output_line(const unsigned char *str)
{
	output_with_count(str,1, 1);
	return 0;
}

/*
 * NOTE: When we output colors, we explicitly turn off bold and reverse,
 * as they can affect the display of the colors. We turn them back on
 * afterwards, though. We dont need to worry about blinking or underline
 * as they dont affect the colors. But reverse and bold do, so we need to
 * make sure that the color sequence has preference, rather than the previous
 * IRC-II formatting colors.
 *
 * OKAY ... just how pedantic do you wanna get? We have a problem with
 * colorization, and its mostly to do with how ANSI is interpreted by
 * different systems. Both Linux and SCO consoles do the right thing,
 * but termcap lacks the ability to turn off blinking and underline
 * as individual sequences, so the "normal" sequence is sent. This causes
 * things such as reverse video, bold, underline, blinking etc to be turned
 * off. So, we have to keep track of our current set of attributes always.
 * Yes, this adds a little bit of processing to the loop, but this has already
 * been optimized to be faster than the previous mechanism, and the cycles
 * are so few, it hardly matters. I think it is more important to get the
 * correct rendering of colors than to get just one more nanosecond of
 * display speed.
 */
int output_with_count(const unsigned char *str, int clreol, int output)
{
const 	u_char 	*ptr = str;
	int 	beep = 0, 
		out = 0;
	int 	val1, 
		val2;
	char 	old_bold = 0, 
		old_rev = 0, 
		old_blink = 0, 
		old_undl = 0, 
		old_altc = 0,
		in_color = 0;

	if (output)
	{
		old_bold  = display_bold(999);
		old_rev   = display_standout(999);
		old_blink = display_blink(999);
		old_undl  = display_underline(999);
		old_altc  = display_altcharset(999);
		in_color  = display_color(999,999);
	}

	while (ptr && *ptr)
	{
	    switch (*ptr)
	    {
		case REV_TOG:
		{
			if (output)
			{
				display_standout(TOGGLE);
				if (!in_color)
				{
					if (!(old_rev = !old_rev))
					{
						if (old_bold)
							term_bold_on();
						if (old_blink)
							term_blink_on();
						if (old_undl)
							term_underline_on();
						if (old_altc)
							term_altcharset_on();
					}
				}
			}
			break;
		}
		case UND_TOG:
		{
			if (output)
			{
				display_underline(TOGGLE);
				if (!in_color)
				{
					if (!(old_undl = !old_undl))
					{
						if (old_bold)
							term_bold_on();
						if (old_blink)
							term_blink_on();
						if (old_rev)
							term_standout_on();
						if (old_altc)
							term_altcharset_on();
					}
				}
			}
			break;
		}
		case BOLD_TOG:
		{
			if (output)
			{
				display_bold(TOGGLE);
				if (!in_color)
				{
					if (!(old_bold = !old_bold))
					{
						if (old_undl)
							term_underline_on();
						if (old_blink)
							term_blink_on();
						if (old_rev)
							term_standout_on();
						if (old_altc)
							term_altcharset_on();
					}
				}
			}
			break;
		}
		case BLINK_TOG:
		{
			if (output)
			{
				display_blink(TOGGLE);
				if (!in_color)
				{
					if (!(old_blink = !old_blink))
					{
						if (old_undl)
							term_underline_on();
						if (old_bold)
							term_bold_on();
						if (old_rev)
							term_standout_on();
						if (old_altc)
							term_altcharset_on();
					}
				}
			}
			break;
		}
		case ALT_TOG:
		{
			if (output)
			{
				display_altcharset(TOGGLE);
				if (!in_color)
				{
					if (!(old_altc = !old_altc))
					{
						if (old_undl)
							term_underline_on();
						if (old_bold)
							term_bold_on();
						if (old_rev)
							term_standout_on();
						if (old_blink)
							term_blink_on();
					}
				}
			}
			break;
		}
		case ALL_OFF:
		{
			if (output)
			{
				display_underline(OFF);
				display_bold(OFF);
				display_standout(OFF);
				display_blink(OFF);
				display_altcharset(OFF);
				display_color(-1,-1);
				in_color = 0;
				old_bold = old_rev = old_blink = old_undl = 0;
				old_altc = 0;
				if (!ptr[1])
					display_normal();
			}
			break;
		}
		case '\007':
		{
			beep++;
			break;
		}
		/* Dont ask */
		case '\f':
		{
			if (output)
			{
				display_standout(TOGGLE);
				putchar_x('f');
				display_standout(TOGGLE);
			}
			out++;
			break;
		}
		case ROM_CHAR:
		{
			/* Sanity... */
			if (!ptr[1] || !ptr[2] || !ptr[3])
				break;

			val1 = ((ptr[1] -'0') * 100) + 
				((ptr[2] -'0') * 10) + 
				 (ptr[3] - '0');

			if (output)
				term_putgchar(val1);

			ptr += 3;
			out++;
			break;
		}
		case '\003':
		{
			/*
			 * By the time we get here, we know we need to 
			 * display these as colors, and they have been 
			 * conveniently "rationalized" for us to provide 
			 * for easy color insertion.
			 */
			val1 = val2 = -1;
			ptr = skip_ctl_c_seq(ptr, &val1, &val2, 0);
			if (output)
			{
				if ((val1 > -1) || (val2 > -1))
				{
					in_color = 1;
					term_standout_off();
				}
				if (display_bold(999) && 
						(val1 >= 30 && val1 <= 37))
					val1 += 20;
				if (display_blink(999) && 
						(val2 >= 40 && val2 <= 47))
					val2 += 10;
				display_color (val1, val2);

				if ((val1 == -1) && (val2 == -1))
				{
					display_normal();
					if (old_bold == ON)
						term_bold_on();
					if (old_rev == ON)
						term_standout_on();
					if (old_blink == ON)
						term_blink_on();
					if (old_undl == ON)
						term_underline_on();
					if (old_altc == ON)
						term_altcharset_on();
					in_color = 0;
				}
			}
			ptr--;
			break;
		}
		case ND_SPACE:
		{
			term_right(1);
			out++;
			break;
		}

		default:
		{
		/*
		 * JKJ TESTME: here I believe it is safe to use putchar_x,
		 * which is MUCH faster than all the weird processing in
		 * term_putchar. The reason for this is we can safely
		 * assume that the stripper has handled control sequences
		 * correctly before we ever get to this point.
		 * term_putchar() can still be left there, for other places
		 * that may want to call it, but at this level, I do not
		 * see any benefit of using it. using putchar_x makes things
		 * a LOT faster, as we have already done the processing once
		 * to make output "terminal friendly".
		 */
			if (output)
				putchar_x(*ptr);
			out++;
		}
	    }
	    ptr++;
	}

	if (output)
	{
		if (beep)
			term_beep();
		if (clreol)
			term_clear_to_eol();
	}

	return out;
}

/*
 * scroll_window: Given a pointer to a window, this determines if that window
 * should be scrolled, or the cursor moved to the top of the screen again, or
 * if it should be left alone. 
 */
void scroll_window(Window *window)
{

	if (window->cursor == window->display_size)
	{
		int	scroll,	i;
		if (window->holding_something || window->scrollback_point)
			nappanic("Cant scroll this window!");

		if ((scroll = get_int_var(SCROLL_LINES_VAR)) <= 0)
			scroll = 1;


		for (i = 0; i < scroll; i++)
		{
			window->top_of_display = window->top_of_display->next;
			window->distance_from_display--;
		}


		if (window->visible && foreground && window->display_size)
		{
			if (window->status.status_split)
				term_scroll(window->top+window->status.status_lines,
					window->top + window->status.status_lines +
					window->cursor-1, scroll);
			else
				term_scroll(window->top,
					window->top - window->status.status_lines +
					window->cursor, scroll);
		}
		window->cursor -= scroll;
	}
	if (window->visible && window->display_size)
	{
		window->screen->cursor_window = window;
		if (window->status.status_split)
			term_move_cursor(0, window->top + window->status.status_lines + window->cursor);
		else
			term_move_cursor(0, window->top + window->cursor);
		term_clear_to_eol();
	}
}

#define ATTRIBUTE_HANDLER(x, y)					\
static	char	display_ ## x (int flag)			\
{								\
	static	int	x	= OFF;				\
								\
	if (flag == 999)					\
		return (x);					\
	if (flag == (x))					\
		return flag;					\
								\
	switch (flag)						\
	{							\
		case ON:					\
		{						\
			x = ON;					\
			term_ ## x ## _on();			\
			return OFF;				\
		}						\
		case OFF:					\
		{						\
			x = OFF;				\
			term_ ## x ## _off();			\
			return ON;				\
		}						\
		case TOGGLE:					\
		{						\
			if (x == ON)				\
			{					\
				x = OFF;			\
				term_ ## x ## _off();		\
				return ON;			\
			}					\
			else					\
			{					\
				x = ON;				\
				term_ ## x ## _on();		\
				return OFF;			\
			}					\
		}						\
	}							\
	return OFF;						\
}

ATTRIBUTE_HANDLER(underline, UNDERLINE_VIDEO_VAR)
ATTRIBUTE_HANDLER(bold, BOLD_VIDEO_VAR)
ATTRIBUTE_HANDLER(blink, BLINK_VIDEO_VAR)
ATTRIBUTE_HANDLER(standout, INVERSE_VIDEO_VAR)
ATTRIBUTE_HANDLER(altcharset, ALT_CHARSET_VAR)

static	void	display_normal (void)
{
	tputs_x(current_term->TI_sgrstrs[TERM_SGR_NORMAL-1]);
}

static int	display_color (long color1, long color2)
{
	static	int	doing_color = 0;

	if ((color1 == 999) && (color2 == 999))
		return doing_color;

	if ((color1 == -1) && (color2 == -1))
		doing_color = 0;
	else
		doing_color = 1;

	tputs_x(replace_color(color1, color2));
	return doing_color;
}

/*
 * cursor_not_in_display: This forces the cursor out of the display by
 * setting the cursor window to null.  This doesn't actually change the
 * physical position of the cursor, but it will force rite() to reset the
 * cursor upon its next call 
 */
void cursor_not_in_display (Screen *s)
{
	if (!s)
		s = output_screen;
	if (s->cursor_window)
		s->cursor_window = NULL;
}

/*
 * cursor_in_display: this forces the cursor_window to be the
 * output_screen->current_window. 
 * It is actually only used in hold.c to trick the system into thinking the
 * cursor is in a window, thus letting the input updating routines move the
 * cursor down to the input line.  Dumb dumb dumb 
 */
void cursor_in_display (Window *w)
{
	if (!w)
		w = current_window;
	if (w->screen)
		w->screen->cursor_window = w;
}

/*
 * is_cursor_in_display: returns true if the cursor is in one of the windows
 * (cursor_window is not null), false otherwise 
 */
int is_cursor_in_display(Screen *screen)
{
	if (!screen && current_window->screen)
		screen = current_window->screen;
	return (screen->cursor_window ? 1 : 0);
}

/* * * * * * * SCREEN UDPATING AND RESIZING * * * * * * * * */
/*
 * repaint_window: redraw the "m"th through the "n"th part of the window.
 * If end_line is -1, then that means clear the display if any of it appears
 * after the end of the scrollback buffer.
 */
void 	repaint_window (Window *w, int start_line, int end_line)
{
	Window *window = (Window *)w;
	Display *curr_line;
	int count;
	int clean_display = 0;

	if (!window)
		window = current_window;

	if (end_line == -1)
	{
		end_line = window->display_size;
		clean_display = 1;
	}

	curr_line = window->top_of_display;
	for (count = 0; count < start_line; count++)
		curr_line = curr_line->next;
                        
	window->cursor = start_line;
	for (count = start_line; count < end_line; count++)
	{
		if (!curr_line) break;
		rite(window, curr_line->line);

		if (curr_line == window->display_ip)
			break;

		curr_line = curr_line->next;
	}

	/*
	 * If "end_line" is -1, then we're supposed to clear off the rest
	 * of the display, if possible.  Do that here.
	 */
	if (clean_display)
	{
		for (; count < end_line - 1; count++)
		{
			term_clear_to_eol();
			term_newline();
		}
/*		update_window_status(window, 1);*/
	}

	recalculate_window_cursor(window);	/* XXXX */
}

/*
 * create_new_screen creates a new screen structure. with the help of
 * this structure we maintain ircII windows that cross screen window
 * boundaries.
 */
Screen	* create_new_screen(void)
{
	Screen	*new = NULL, *list;
	static	int	refnumber = 0;
	
	for (list = screen_list; list; list = list->next)
	{
		if (!list->alive)
		{
			new = list;
			break;
		}

		if (!list->next)
			break;
	}

	if (!new)
	{
		new = (Screen *) new_malloc(sizeof(Screen));
		new->screennum = ++refnumber;
		new->next = NULL;
		if (list)
			list->next = new;
		else
			screen_list = new;
	}
	new->last_window_refnum = 1;


	new->fpout = stdout;
	new->fdout = fileno(stdout);
	if (use_input)
		new_open(0);
	
	new->fpin = stdin;
	new->fdin = fileno(stdin);

	new->alive = 1;
	new->li = current_term->TI_lines;
	new->co = current_term->TI_cols;
	new->old_li = 0;
	new->old_co = 0;
	new->buffer_pos = new->buffer_min_pos = 0;
	new->input_buffer[0] = '\0';
	new->input_cursor = 0;
	new->input_visible = 0;
	new->input_start_zone = 0;
	new->input_end_zone = 0;
	new->input_prompt = NULL;
	new->input_prompt_len = 0;
	new->input_prompt_malloc = 0;
	new->input_line = 23;

	new->redirect_server = -1;

	last_input_screen = new;

	if (!main_screen)
		main_screen = new;
	init_input();
	return new;
}


void set_screens (fd_set *rd, fd_set *wd)
{
Screen *screen;
	if (use_input)
		for (screen = screen_list; screen; screen = screen->next)
			if (screen->alive)
				FD_SET(screen->fdin, rd);
}




void do_screens (fd_set *rd)
{
	Screen *screen;
	if (!use_input)
		return;

	for (screen = screen_list; screen; screen = screen->next)
	{
		if (!screen->alive)
			continue;
		if (FD_ISSET(screen->fdin, rd))
		{
			/*
			 * This section of code handles all in put from the terminal(s).
			 * connected to ircII.  Perhaps the idle time *shouldn't* be 
			 * reset unless its not a screen-fd that was closed..
			 */
			time(&idle_time);
			if (in_browser)
			{
				browser_main_loop();
				return;
			}
			if (cpu_saver)
			{
				cpu_saver = 0;
				update_all_status(current_window, NULL, 0);
			}

			{
				unsigned char loc_buffer[BIG_BUFFER_SIZE + 1];
				int	n, i;
				int	server = from_server;
				last_input_screen = screen;
				output_screen = screen;
				make_window_current(screen->current_window);
				target_window = NULL;
				from_server = current_window->server;
				if ((n = read(screen->fdin, loc_buffer, BIG_BUFFER_SIZE)) > 0)
				{
					extended_handled = 0;
					for (i = 0; i < n; i++)
						if (!extended_handled)
							edit_char(loc_buffer[i]);
				}
				else
					nap_exit(1, "My damn controlling terminal disappeared!", NULL);
				from_server = server;
			} 
		}
	} 
} 


/*
 * Change xterm's title to reflect current connection status.
 */
void xterm_settitle(void)
{
	char titlestring[BIG_BUFFER_SIZE+1];
	char *c;

	if (!(c = get_current_channel_by_refnum(0)))
		c = "(none)";

	if ((getenv("STY") || getenv("DISPLAY")) && get_int_var(XTERM_SHOW_TITLE_VAR))
	{
		snprintf(titlestring, BIG_BUFFER_SIZE, 
			"\033]2;%s:  %s on %s", 
				nap_version, nickname, c);
        	tputs_x(titlestring);
		term_flush();
	}
}

/* 
 * add_wait_prompt:  Given a prompt string, a function to call when
 * the prompt is entered.. some other data to pass to the function,
 * and the type of prompt..  either for a line, or a key, we add 
 * this to the prompt_list for the current screen..  and set the
 * input prompt accordingly.
 */
void add_wait_prompt(char *prompt, void (*func)(char *, char *), char *data, int type, int echo)
{
	WaitPrompt **AddLoc,
		   *New;

	if (!current_window->screen)
		return;
	New = (WaitPrompt *) new_malloc(sizeof(WaitPrompt));
	New->prompt = m_strdup(prompt);
	New->data = m_strdup(data);
	New->type = type;
	New->func = func;
	New->next = NULL;
	New->echo = echo;
	for (AddLoc = &current_window->screen->promptlist; *AddLoc;
			AddLoc = &(*AddLoc)->next);
	*AddLoc = New;
	if (AddLoc == &current_window->screen->promptlist)
		change_input_prompt(1);
}

/* * * * * * * * * * * * * * * * * COLOR SUPPORT * * * * * * * * * * */
/*
 * This parses out a ^C control sequence.  Note that it is not acceptable
 * to simply slurp up all digits after a ^C sequence (either by calling
 * strtol(), or while (isdigit())), because people put ^C sequences right
 * before legit output with numbers (like the time in your status bar.)
 * Se we have to actually slurp up only those digits that comprise a legal
 * ^C code.
 */
const u_char *skip_ctl_c_seq (const u_char *start, int *lhs, int *rhs, int proper)
{
const 	u_char 	*after = start;
	u_char	c1, c2;
	int *	val;
	int	lv1, rv1;

	/*
	 * For our sanity, just use a placeholder if the caller doesnt
	 * care where the end of the ^C code is.
	 */
	if (!lhs)
		lhs = &lv1;
	if (!rhs)
		rhs = &rv1;

	*lhs = *rhs = -1;

	/*
	 * If we're passed a non ^C code, dont do anything.
	 */
	if (*after != '\003')
		return after;
                                                  
	/*
	 * This is a one-or-two-time-through loop.  We find the  maximum 
	 * span that can compose a legit ^C sequence, then if the first 
	 * nonvalid character is a comma, we grab the rhs of the code.  
	 */
	val = lhs;
	for (;;)
	{
		/*
		 * If its just a lonely old ^C, then its probably a terminator.
		 * Just skip over it and go on.
		 */
		after++;
		if (*after == 0)
			return after;

		/*
		 * Check for the very special case of a definite terminator.
		 * If the argument to ^C is -1, then we absolutely know that
		 * this ends the code without starting a new one
		 */
		if (after[0] == '-' && after[1] == '1')
			return after + 2;

		/*
		 * Further checks against a lonely old naked ^C.
		 */
		if (!isdigit(after[0]) && after[0] != ',')
			return after;


		/*
		 * Code certainly cant have moRe than two chars in it
		 */
		c1 = after[0];
		c2 = after[1];

		/*
		 * Our action depends on the char immediately after the ^C.
		 */
		switch (c1)
		{
			/* 
			 * 0X -> 0X <stop> for all numeric X
			 */
			case '0':
				after++;
				*val = c1 - '0';
				if (c2 >= '0' && c2 <= '9')
				{
					after++;
					*val = *val * 10 + (c2 - '0');
				}
				break;

			/* 
			 * 1X -> 1 <stop> X if X >= '6' 
			 * 1X -> 1X <stop>  if X < '6'
			 */
			case '1':
				after++;
				*val = c1 - '0';
				if (c2 >= '0' && c2 < '7')
				{
					after++;
					*val = *val * 10 + (c2 - '0');
				}
				else if (proper && c2 > '5' && c2 <= '9')
				{
					after++;
					*val = *val * 10 + (c2 - '0');
					*val = *val - 16;
				}
				break;

			/*
			 * 3X -> 3 <stop> X if X >= '8'
			 * 3X -> 3X <stop>  if X < '8'
			 * (Same for 4X and 5X)
			 */
			case '3':
			case '4':
				after++;
				*val = c1 - '0';
				if (c2 >= '0' && c2 < '8')
				{
					after++;
					*val = *val * 10 + (c2 - '0');
				}
				break;

			case '5':
				after++;
				*val = c1 - '0';
				if (c2 >= '0' && c2 < '9')
				{
					after++;
					*val = *val * 10 + (c2 - '0');
				}
				break;

			/*
			 * Y -> Y <stop> for any other numeric Y.
			 */
			case '2':
			case '6':
			case '7':
			case '8':
			case '9':
				*val = (c1 - '0');
				after++;
				break;

			/*
			 * Y -> <stop> Y for any other nonnumeric Y
			 */
			default:
				break;
		}

		if (val == lhs)
		{
			val = rhs;
			if (*after == ',')
				continue;
		}
		break;
	}

	return after;
}

/*
 * Used as a translation table when we cant display graphics characters
 * or we have been told to do translation.  A no-brainer, with little attempt
 * at being smart.
 * (JKJ: perhaps we should allow a user to /set this?)
 */
static	u_char	gcxlate[256] = {
  '*', '*', '*', '*', '*', '*', '*', '*',
  '#', '*', '#', '*', '*', '*', '*', '*',
  '>', '<', '|', '!', '|', '$', '_', '|',
  '^', 'v', '>', '<', '*', '=', '^', 'v',
  ' ', '!', '"', '#', '$', '%', '&', '\'',
  '(', ')', '*', '+', ',', '_', '.', '/',
  '0', '1', '2', '3', '4', '5', '6', '7',
  '8', '9', ':', ';', '<', '=', '>', '?',
  '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G',
  'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
  'Z', 'Y', 'X', '[', '\\', ']', '^', '_',
  '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g',
  'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
  'x', 'y', 'z', '{', '|', '}', '~', '?',
  'C', 'u', 'e', 'a', 'a', 'a', 'a', 'c',
  'e', 'e', 'e', 'i', 'i', 'i', 'A', 'A',
  'e', 'e', 'e', 'o', 'o', 'o', 'u', 'u',
  'y', 'O', 'U', 'C', '#', 'Y', 'P', 'f',
  'a', 'i', 'o', 'u', 'n', 'N', '^', '^',
  '?', '<', '>', '2', '4', '!', '<', '>',
  '#', '#', '#', '|', '|', '|', '|', '+',
  '+', '+', '+', '|', '+', '+', '+', '+',
  '+', '+', '+', '+', '-', '+', '+', '+',
  '+', '+', '+', '+', '+', '=', '+', '+',
  '+', '+', '+', '+', '+', '+', '+', '+',
  '+', '+', '+', '#', '-', '|', '|', '-',
  'a', 'b', 'P', 'p', 'Z', 'o', 'u', 't',
  '#', 'O', '0', 'O', '-', 'o', 'e', 'U',
  '*', '+', '>', '<', '|', '|', '/', '=',
  '*', '*', '*', '*', 'n', '2', '*', '*'
};

/*
 * State 0 is "normal, printable character"
 * State 1 is "nonprintable character"
 * State 2 is "Escape character" (033)
 * State 3 is "Color code" (003)
 * State 4 is "Special highlight character"
 * State 5 is "ROM character" (022)
 * State 6 is a character that should never be printed
 */
static	u_char	ansi_state[256] = {
/*	^@	^A	^B	^C	^D	^E	^F	^G */
	6,	6,	4,	3,	6,	4,	4,	4,  /* 000 */
/*	^H	^I	^J	^K	^D	^M	^N	^O */
	6,	4,	4,	6,	0,	6,	6,	4,  /* 010 */
/*	^P	^Q	^R	^S	^T	^U	^V	^W */
	6,	6,	5,	4,	4,	6,	4,	6,  /* 020 */
/*	^X	^Y	^Z	^[	^\	^]	^^	^_ */
	6,	6,	6,	2,	6,	6,	6,	4,  /* 030 */
	0,	0,	0,	0,	0,	0,	0,	0,  /* 040 */
	0,	0,	0,	0,	0,	0,	0,	0,  /* 050 */
	0,	0,	0,	0,	0,	0,	0,	0,  /* 060 */
	0,	0,	0,	0,	0,	0,	0,	0,  /* 070 */
	0,	0,	0,	0,	0,	0,	0,	0,  /* 100 */
	0,	0,	0,	0,	0,	0,	0,	0,  /* 110 */
	0,	0,	0,	0,	0,	0,	0,	0,  /* 120 */
	0,	0,	0,	0,	0,	0,	0,	0,  /* 130 */
	0,	0,	0,	0,	0,	0,	0,	0,  /* 140 */
	0,	0,	0,	0,	0,	0,	0,	0,  /* 150 */
	0,	0,	0,	0,	0,	0,	0,	0,  /* 160 */
	0,	0,	0,	0,	0,	0,	0,	0,  /* 170 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 200 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 210 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 220 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 230 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 240 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 250 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 260 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 270 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 300 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 310 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 320 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 330 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 340 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 350 */
	1,	1,	1,	1,	1,	1,	1,	1,  /* 360 */
	1,	1,	1,	1,	1,	1,	1,	1   /* 370 */
};

/*
 * This started off as a general ansi parser, and it worked for stuff that
 * was going out to the display, but it couldnt deal properly with ansi codes,
 * and so when i tried to use it for the status bar, it just all fell to 
 * pieces.  After working it over, i came up with this.  What this does is,
 * (believe it or not) walk through and strip oUt all the ansi codes in the
 * target string.  Any codes that we recognize as being safe (pretty much
 * just ^[[<number-list>m), are converted back into their logical characters
 * (eg, ^B, ^R, ^_, etc), and everything else is completely blown away.
 */

/*
 * These macros help keep 8 bit chars from sneaking into the output stream
 * where they might be stripped out.
 */
#if 1
#define this_char() (eightbit ? *str : (*str) & 0x7f)
#define next_char() (eightbit ? *str++ : (*str++) & 0x7f)
#define put_back() (str--)
#else
#define this_char() (*str)
#define next_char() (*str++)
#define put_back() (str--)
#endif


unsigned char *strip_ansi (const unsigned char *str)
{
	u_char	*output;
	u_char	chr, chr1;
	int 	pos, maxpos;
	int 	args[10], nargs, i;

	int	bold = 0;
	int	underline = 0;
	int	reverse = 0;
	int	blink = 0;
	int	alt_mode = 0;
	int     fg_color = 0;
	int     bg_color = 1;
                
	int	ansi = get_int_var(DISPLAY_ANSI_VAR);
	int	gcmode = get_int_var(DISPLAY_PC_CHARACTERS_VAR);
	int 	n;
	int	eightbit = term_eight_bit();

	/* 
	 * The output string has a few extra chars on the end just 
	 * in case you need to tack something else onto it.
	 */
	maxpos = strlen(str);
	output = (u_char *)new_malloc(maxpos + 64);
	pos = 0;

	while ((chr = next_char()))
	{
	    chr1 = *(str - 1);
	    if (pos > maxpos)
	    {
		maxpos += 64; /* Extend 64 chars at a time */
		RESIZE(output, unsigned char, maxpos + 64);
	    }

	    switch (ansi_state[chr1])
	    {
		/*
		 * State 0 is a normal, printable ascii character
		 */
		case 0:
			output[pos++] = chr;
			break;

		/*
		 * State 1 is an unprintable character that may need
		 * to be translated first.
		 */
		case 1:
		case 6:
		{
			int my_gcmode = gcmode;
			/*
			 * This is a very paranoid check to make sure that
			 * the 8-bit escape code doesnt elude us.
			 */
			if (chr == 27 + 128)
			{
				output[pos++] = REV_TOG;
				output[pos++] = '[';
				output[pos++] = REV_TOG;
				continue;
			}
			if (ansi_state[chr] == 6)
				my_gcmode = 1;
			if (strip_ansi_never_xlate)
				my_gcmode = 4;

			switch (my_gcmode)
			{
				/*
				 * In gcmode 5, translate all characters
				 */
				case 5:
				{
					output[pos++] = gcxlate[chr];
					break;
				}

				/*
				 * In gcmode 4, accept all characters
				 */
				case 4:
				{
					output[pos++] = chr;
					break;
				}

				/*
				 * In gcmode 3, accept or translate chars
				 */
				case 3:
				{
					if (termfeatures & TERM_CAN_GCHAR)
						output[pos++] = chr;
					else
						output[pos++] = gcxlate[chr];
					break;
				}

				/*
				 * In gcmode 2, accept or highlight xlate
				 */
				case 2:
				{
					if (termfeatures & TERM_CAN_GCHAR)
						output[pos++] = chr;
					else
					{
						output[pos++] = REV_TOG;
						output[pos++] = gcxlate[chr];
						output[pos++] = REV_TOG;
					}
					break;
				}

				/*
				 * gcmode 1 is "accept or reverse mangle"
				 * If youre doing 8-bit, it accepts eight
				 * bit characters.  If youre not doing 8 bit
				 * then it converts the char into something
				 * printable and then reverses it.
				 */
				case 1:
				{
					if (termfeatures & TERM_CAN_GCHAR)
						output[pos++] = chr;
					else if ((chr & 0x80) && eightbit)
						output[pos++] = chr;
					else
					{
						output[pos++] = REV_TOG;
						output[pos++] = 
						(chr | 0x40) & 0x7f;
						output[pos++] = REV_TOG;
					}
					break;
				}

				/*
				 * gcmode 0 is "always strip out"
				 */
				case 0:
					break;
			}
			break;
		}


		/*
		 * State 2 is the escape character
		 */
		case 2:
		{
		    /*
		     * The next thing we do is dependant on what the character
		     * is after the escape.  Be very conservative in what we
		     * allow.  In general, escape sequences shouldnt be very
		     * complex at this point.
		     */
		    if (!ansi && this_char() == 0)
		    {
#if 0
			output[pos++] = REV_TOG;
			output[pos++] = '[';
			output[pos++] = REV_TOG;
#endif
			continue;
		    }

		    switch ((chr = next_char()))
		    {
			/*
			 * All these codes we just skip over.  We're not
			 * interested in them.
			 */

			/*
			 * These are two-character commands.  The second
			 * char is the argument.
			 */
			case ('#') : case ('(') : case (')') :
			case ('*') : case ('+') : case ('$') :
			case ('@') :
			{
				chr = next_char();	/* Skip the argument */
				if (chr == 0)
					put_back();
				break;
			}

			/*
			 * These are just single-character commands.
			 */
			case ('7') : case ('8') : case ('=') :
			case ('>') : case ('D') : case ('E') :
			case ('F') : case ('H') : case ('M') :
			case ('N') : case ('O') : case ('Z') :
			case ('l') : case ('m') : case ('n') :
/* { */			case ('o') : case ('|') : case ('}') :
			case ('~') : case ('c') :
			{
				break;		/* Dont do anything */
			}

			/*
			 * Swallow up graphics sequences...
			 */
			case ('G'):
			{
				while (((chr = next_char()) != 0) && chr != ':')
					;
				if (chr == 0)
					put_back();
				break;
			}

			/*
			 * Not sure what this is, its unimplemented in 
			 * rxvt, but its supposed to end with an ESCape.
			 */
			case ('P') :
			{
				while (((chr = next_char()) != 0) && chr != 033)
					;
				if (chr == 0)
					put_back();
				break;
			}

			/*
			 * Anything else, we just munch the escape and 
			 * leave it at that.
			 */
			default:
				put_back();
				break;

			/*
			 * Strip out Xterm sequences
			 */
			case (']') :
			{
				while (((chr = next_char()) != 0) && chr != 7)
					;
				if (chr == 0)
					put_back();
				break;
			}

			/*
			 * Now these we're interested in....
			 * (CSI sequences)
			 */
			case ('[') :
			{
				/*
				 * Set up the arguments list
				 */
				nargs = 0;
				args[0] = args[1] = args[2] = args[3] = 0;
				args[4] = args[5] = args[6] = args[7] = 0;
				args[8] = args[9] = 0;

				/*
				 * This stuff was taken/modified/inspired by
				 * rxvt.  We do it this way in order to trap
				 * an esc sequence that is embedded in another
				 * (blah).  We're being really really really
				 * paranoid doing this, but it is for the best.
				 */

				/*
				 * Check to see if the stuff after the
				 * command is a "private" modifier.  If it is,
				 * then we definitely arent interested.
				 *   '<' , '=' , '>' , '?'
				 */
				chr = this_char();
				if (chr >= '<' && chr <= '?')
					next_char();	/* skip it */


				/*
				 * Now pull the arguments off one at a time.
				 * Keep pulling them off until we find a
				 * character that is not a number or a semi-
				 * colon.  Skip everything else.
				 */
				for (nargs = 0; nargs < 10; str++)
				{
					n = 0;
					for (n = 0;
					     isdigit(this_char());
					     next_char())
					{
					    n = n * 10 + (this_char() - '0');
					}

					args[nargs++] = n;

					/*
					 * If we run out of code here, 
					 * then we're totaly confused.
					 * just back out with whatever
					 * we have...
					 */
					if (!this_char())
					{
						output[pos] = output[pos+1] = 0;
						return output;
					}
					if (this_char() != ';')
						break;
				}

				/*
				 * If we find a new ansi char, start all
				 * over from the top and strip it out too
				 */
				if (this_char() == 033)
					continue;

				/*
				 * Support "spaces" (cursor right) code
				 */
				if (this_char() == 'a' || this_char() == 'C')
				{
					next_char();
					if (nargs >= 1)
					{
					    /*
					     * Keep this within reality.
					     */
					    if (args[0] > 256)
						args[0] = 256;

					    /* This is just sanity */
					    if (pos + args[0] > maxpos)
					    {
						maxpos += args[0]; 
						RESIZE(output, u_char, maxpos + 64);
					    }
					    while (args[0]-- > 0)
						output[pos++] = ND_SPACE;
					}
					break;
				}

				/*
				 * The 'm' command is the only one that
				 * we honor.  All others are dumped.
				 */
				if (next_char() != 'm')
					break;

				if (!ansi)
					break;
				/*
				 * Walk all of the numeric arguments,
				 * plonking the appropriate commands into
				 * the output...
				 */
				for (i = 0; i < nargs; i++)
				{
					if (args[i] == 0)
					{
						output[pos++] = ALL_OFF;
						bold = reverse = blink = 0;
						underline = 0;
						fg_color = bg_color = 0;
					}
					else if (args[i] == 1 && !bold)
					{
						output[pos++] = BOLD_TOG;
						if (fg_color >= 30 && 
						    fg_color <= 37)
						{
							fg_color += 20;
							output[pos++] = 3;
							output[pos++] = '5';
							output[pos++] = '0' +
								fg_color % 10;
						}
						bold = 1;
					}
					else if (args[i] == 4 && !underline)
					{
						output[pos++] = UND_TOG;
						underline = 1;
					}
					else if ((args[i] == 5 || args[i] == 26 )
						&& !blink)
					{
						output[pos++] = BLINK_TOG;
						if (bg_color >= 40 && 
						    bg_color <= 47)
						{
							bg_color += 10;
							output[pos++] = 3;
							output[pos++] = ',';
							output[pos++] = '5';
							output[pos++] = '0' +
								bg_color % 10;
						}
						blink = 1;
					}
					else if ((args[i] == 6 || args[i] == 25)
							&& blink)
					{
						output[pos++] = BLINK_TOG;
						if (bg_color >= 50 && 
						    bg_color <= 57)
						{
							bg_color -= 10;
							output[pos++] = 3;
							output[pos++] = ',';
							output[pos++] = '4';
							output[pos++] = '0' +
								bg_color % 10;
						}
						blink = 0;
					}
					else if (args[i] == 7 && !reverse)
					{
						output[pos++] = REV_TOG;
						reverse = 1;
					}
					else if (args[i] == 21 && bold)
					{
						output[pos++] = BOLD_TOG;
						if (fg_color >= 50 && 
						    fg_color <= 57)
						{
							fg_color -= 20;
							output[pos++] = 3;
							output[pos++] = '3';
							output[pos++] = '0' +
								fg_color % 10;
						}
						bold = 0;
					}
					else if (args[i] == 24 && underline)
					{
						output[pos++] = UND_TOG;
						underline = 0;
					}
					else if (args[i] == 27 && reverse)
					{
						output[pos++] = REV_TOG;
						reverse = 0;
					}
					else if (args[i] >= 30 && args[i] <= 37)
					{
						output[pos++] = 003;
						if (bold)
							output[pos++] = '5';
						else
							output[pos++] = '3';
						output[pos++] = args[i] - 30 + '0';
						if (blink)
						{
							output[pos++] = ',';
							output[pos++] = '5';
							output[pos++] = '8';
						}
						fg_color = args[i];
					}
					else if (args[i] >= 40 && args[i] <= 47)
					{
						output[pos++] = 003;
						output[pos++] = ',';
						if (blink)
							output[pos++] = '5';
						else
							output[pos++] = '4';
						output[pos++] = args[i] - 40 + '0';
						bg_color = args[i];
					}

				} /* End of for (handling esc-[...m) */
			} /* End of escape-[ code handling */
		    } /* End of ESC handling */
		    break;
	        } /* End of case 27 handling */


	        /*
	         * Skip over ^C codes, theyre already normalized
	         * well, thats not totaly true.  We do some mangling
	         * in order to make it work better
	         */
		case 3:
		{
			const u_char 	*end;
			int		lhs, rhs;

			put_back();
			end = skip_ctl_c_seq (str, &lhs, &rhs, 0);

			/*
			 * XXX - This is a short-term hack to prevent an 
			 * infinite loop.  I need to come back and fix
			 * this the right way in the future.
			 *
			 * The infinite loop can happen when a character
			 * 131 is encountered when eight bit chars is OFF.
			 * We see a character 3 (131 with the 8th bit off)
			 * and so we ask skip_ctl_c_seq where the end of 
			 * that sequence is.  But since it isnt a ^c sequence
			 * it just shrugs its shoulders and returns the
			 * pointer as-is.  So we sit asking it where the end
			 * is and it says "its right here".  So there is a 
			 * need to check the retval of skip_ctl_c_seq to 
			 * actually see if there is a sequence here.  If there
			 * is not, then we just mangle this character.  For
			 * the record, char 131 is a reverse block, so that
			 * seems the most appropriate thing to put here.
			 */
			if (end == str)
			{
				/* Turn on reverse if neccesary */
				if (reverse == 0)
					output[pos++] = REV_TOG;
				output[pos++] = ' ';
				if (reverse == 0)
					output[pos++] = REV_TOG;
				next_char();	/* Munch it */
				break;
			}


			/*
			 * If the user specifies an absolute value ^C
			 * code (eg, not an mirc order one), then we turn
			 * off bold or blink so that their color is rendered
			 * exactly like they specify.
			 */
			/*
			 * Turn off BOLD if the user specifies a lhs
			 */
			if (lhs != -1 && bold)
			{
				output[pos++] = BOLD_TOG;
				bold = 0;
			}

			/*
			 * Ditto with BLINK for rhs
			 */
			if (rhs != -1 && blink)
			{
				output[pos++] = BLINK_TOG;
				blink = 0;
			}

			while (str < end)
				output[pos++] = next_char();

			fg_color = lhs;
			bg_color = rhs;
			break;
		}

		/*
		 * State 4 is for the special highlight characters
		 */
		case 4:
		{
			put_back();
			switch (this_char())
			{
				case REV_TOG:   reverse = !reverse;     break;
				case BOLD_TOG:  bold = !bold;           break;
				case BLINK_TOG: blink = !blink;         break;
				case UND_TOG:   underline = !underline; break;
				case ALT_TOG:	alt_mode = !alt_mode;   break;
				case ALL_OFF:
				{
					reverse = bold = blink = underline = 0;
					alt_mode = 0;
					break;
				}
				default:   break;
			}

			output[pos++] = next_char();
			break;
		}

		case 5:
		{
			put_back();
			if (str[0] && str[1] && str[2] && str[3])
			{
				output[pos++] = next_char();
				output[pos++] = next_char();
				output[pos++] = next_char();
				output[pos++] = next_char();
			}
			else 
				output[pos++] = next_char();
			break;
		}

		default:
		{
			nappanic("Unknown strip_ansi mode");
			break;
		}
	    }
	}
	output[pos] = output[pos + 1] = 0;
	return output;
}

/*
 * Meant as an argument to strip_ansi(). This version will replace ANSI
 * color changes with terminal specific color changes.
 */
char *replace_color(int fore, int back)
{
static 	char 	retbuf[512];
	char 	*ptr = retbuf;
	static	int	last_fore = -2, last_back = -2;

	static int fore_conv[] = {
		15,  0,  4,  2,  1,  3,  5,  9,		/*  0-7  */
		11, 10,  6, 14, 12, 13,  8,  7,		/*  8-15 */
		15,  0,  0,  0,  0,  0,  0,  0, 	/* 16-23 */
		 0,  0,  0,  0,  0,  0,  0,  1, 	/* 24-31 */
		 2,  3,  4,  5,  6,  7,  0,  0,		/* 32-39 */
		 0,  0,  0,  0,  0,  0,  0,  0,		/* 40-47 */
		 0,  0,  8,  9, 10, 11, 12, 13, 	/* 48-55 */
		14, 15 					/* 56-57 */
	};
	static int back_conv[] = {
		 7,  0,  4,  2,  1,  3,  5,  1,
		 3,  2,  6,  6,  4,  5,  0,  0,
		 7,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,
		 0,  0,  0,  0,  0,  0,  0,  0,
		 0,  1,  2,  3,  4,  5,  6,  7, 
		 0,  0,  8,  9, 10, 11, 12, 13,
		14, 15 
	};

	*ptr = '\0';

	if (!get_int_var(COLOR_VAR))
		return retbuf;

	if (fore == -2 && back == -2)
	{
		back = fore = -1;
	}
	else if (fore == -1 && back == -1)
	{
		last_fore = last_back = -2;
		return (current_term->TI_sgrstrs[TERM_SGR_NORMAL-1]);
	}

	if (fore == -1)
		fore = last_fore;
	if (back == -1)
		back = last_back;

	if (fore > -1 && fore_conv[fore] < 8)
		strcat(retbuf, current_term->TI_sgrstrs[TERM_SGR_BOLD_OFF - 1]);
	if (back > -1 && back < 58)
		if (fore_conv[back] < 8)
			strcat(retbuf, current_term->TI_sgrstrs[TERM_SGR_BLINK_OFF - 1]);

	if (back == 58)
		strcat(retbuf, current_term->TI_sgrstrs[TERM_SGR_BLINK_ON - 1]);
	if (fore > -1)
		strcat(retbuf, current_term->TI_forecolors[fore_conv[fore]]);
	if (back > -1 && back < 58)
		strcat(retbuf, current_term->TI_backcolors[back_conv[back]]);

	last_fore = fore;
	last_back = back;

	return retbuf;
}

/*
 * input.c: does the actual input line stuff... keeps the appropriate stuff
 * on the input line, handles insert/delete of characters/words... the whole
 * ball o wax 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 * $Id: input.c,v 1.1.1.1 2001/01/17 21:19:21 edwards Exp $
 */

#include "teknap.h"
#include <pwd.h>
#include "struct.h"

#include "commands.h"
#include "exec.h"
#include "history.h"
#include "bsdglob.h"
#include "hook.h"
#include "input.h"
#include "ircaux.h"
#include "keys.h"
#include "screen.h"
#include "server.h"
#include "ircterm.h"
#include "list.h"
#include "vars.h"
#include "screen.h"
#include "output.h"
#include "window.h"
#include "status.h"
#include "napster.h"

#include <sys/ioctl.h>

void get_history (int);

static char new_nick[NICKNAME_LEN+1] = "";
static char *input_lastmsg = NULL;

extern int extended_handled;
extern int foreground;

const int WIDTH = 10;

/* input_prompt: contains the current, unexpanded input prompt */
static	char	*input_prompt = NULL;

enum I_STATE { 
	STATE_NORMAL = 0, 
	STATE_COMPLETE, 
	STATE_TABKEY, 
	STATE_TABKEYNEXT, 
	STATE_CNICK, 
	STATE_CNICKNEXT 
} in_completion = STATE_NORMAL;

/* These are sanity macros.  The file was completely unreadable before 
 * i put these in here.  I make no apologies for them.
 */
#define current_screen		last_input_screen
#define INPUT_CURSOR 		current_screen->input_cursor
#define INPUT_BUFFER 		current_screen->input_buffer
#define MIN_POS 		current_screen->buffer_min_pos
#define THIS_POS 		current_screen->buffer_pos
#define THIS_CHAR 		INPUT_BUFFER[THIS_POS]
#define MIN_CHAR 		INPUT_BUFFER[MIN_POS]
#define PREV_CHAR 		INPUT_BUFFER[THIS_POS-1]
#define NEXT_CHAR 		INPUT_BUFFER[THIS_POS+1]
#define ADD_TO_INPUT(x) 	strmcat(INPUT_BUFFER, (x), INPUT_BUFFER_SIZE);
#define INPUT_ONSCREEN 		current_screen->input_visible
#define INPUT_VISIBLE 		INPUT_BUFFER[INPUT_ONSCREEN]
#define ZONE			current_screen->input_zone_len
#define START_ZONE 		current_screen->input_start_zone
#define END_ZONE 		current_screen->input_end_zone
#define INPUT_PROMPT 		current_screen->input_prompt
#define INPUT_PROMPT_LEN 	current_screen->input_prompt_len
#define INPUT_LINE 		current_screen->input_line
#define BUILT_IN_KEYBINDING(x) void x (char key, char *string)


#define HOLDLAST		current_screen->current_window->screen_hold
  
Display *get_screen_hold(Window *win)
{
	return win->screen_hold;
}

static int safe_puts (register char *str, int len, int echo) 
{
	int i = 0;
	while (*str && i < len)
	{
		term_putchar(*str);
		str++; i++;
	}
	return i;
}

/* cursor_to_input: move the cursor to the input line, if not there already */
extern void cursor_to_input (void)
{
	Screen *oldscreen = last_input_screen;
	Screen *screen;
	
	if (!foreground)
		return;

	for (screen = screen_list; screen; screen = screen->next)
	{
		if (screen->alive && is_cursor_in_display(screen))
		{
			output_screen = screen;
			last_input_screen = screen;
			term_move_cursor(INPUT_CURSOR, INPUT_LINE);
			cursor_not_in_display(screen);
			term_flush();
		}
	}
	output_screen = last_input_screen = oldscreen;
}

/*
 * update_input: does varying amount of updating on the input line depending
 * upon the position of the cursor and the update flag.  If the cursor has
 * move toward one of the edge boundaries on the screen, update_cursor()
 * flips the input line to the next (previous) line of text. The update flag
 * may be: 
 *
 * NO_UPDATE - only do the above bounds checking. 
 *
 * UPDATE_JUST_CURSOR - do bounds checking and position cursor where is should
 * be. 
 *
 * UPDATE_FROM_CURSOR - does all of the above, and makes sure everything from
 * the cursor to the right edge of the screen is current (by redrawing it). 
 *
 * UPDATE_ALL - redraws the entire line 
 */
extern void	update_input (int update)
{
	int	old_zone;
	char	*ptr, *ptr_free;
	int	len,
		echo = 1,
		free_it = 0,
		max;

	char	*prompt;
	Screen	*os = last_input_screen;
	Screen	*ns;
	Window	*saved_current_window = current_window;


	if (!foreground)
		return;
	for (ns = screen_list; ns; ns = ns->next)
	{
		if (!ns->alive)
			continue;
		last_input_screen = ns;
		current_window = ns->current_window;

		cursor_to_input();

		if (last_input_screen->promptlist)
			prompt = last_input_screen->promptlist->prompt;
		else
			prompt = input_prompt;
				
		if (prompt && update != NO_UPDATE)
		{
			int	af;

			/*
			 * If the current window is query'ing an exec'd process,
			 * then we just get the current prompt for that process.
			 * Note that it is not malloced.
			 */
			if (is_valid_process(get_target_by_refnum(0)) != -1)
				ptr = get_prompt_by_refnum(0);

			/*
			 * Otherwise, we just expand the prompt as normal.
			 */
			else
			{
				ptr = expand_alias(prompt, empty_string, &af, NULL);
				free_it = 1;
			}

			/*
			 * If we're in an add_wait_prompt(), we see whether or not
			 * this is an "invisible" prompt.  If it is, we turn off the
			 * echo so what the user types doesnt show up.
			 */
			if (last_input_screen->promptlist)
				term_echo(last_input_screen->promptlist->echo);

			/*
			 * Mangle out any ansi chars or so forth.
			 */
			ptr_free = ptr;
			ptr = strip_ansi(ptr);
			strcat(ptr, ALL_OFF_STR);	/* Yes, we can do this */
			if (free_it)
				new_free(&ptr_free);
			free_it = 1;

			/*
			 * If the prompt has changed, or if there is no prompt...
			 */
			if (	(ptr && !INPUT_PROMPT) ||
				(!ptr && INPUT_PROMPT) ||
				strcmp(ptr, INPUT_PROMPT)	)
			{
				if (last_input_screen->input_prompt_malloc)
					new_free(&INPUT_PROMPT);

				last_input_screen->input_prompt_malloc = free_it;
				INPUT_PROMPT = ptr;
				INPUT_PROMPT_LEN = output_with_count(INPUT_PROMPT, 0, 0);
				update = UPDATE_ALL;
			}
			/*
			 * Prompt didnt change, so clean up our mess
			 */
			else
			{
				if (free_it)
					new_free(&ptr);
			}
		}
		/*
		 * 
		 * HAS THE SCREEN CHANGED SIZE SINCE THE LAST TIME?
		 *
		 */

		/*
		 * If the screen has resized, then we need to re-compute the
		 * side-to-side scrolling effect.
		 */
		if ((last_input_screen->li != last_input_screen->old_li) || 
		    (last_input_screen->co != last_input_screen->old_co))
		{
			/*
			 * The input line is always the bottom line
			 */

			INPUT_LINE = last_input_screen->li - 1;

			/*
			 * The "zone" is the range in which when you type, the
			 * input line does not scroll.  Its WIDTH chars in from
			 * either side.
			 */
			ZONE = last_input_screen->co - (WIDTH * 2);
			if (ZONE < 10)
				ZONE = 10;		/* Take that! */
	
			START_ZONE = WIDTH;
			END_ZONE = last_input_screen->co - WIDTH;
	
			last_input_screen->old_co = last_input_screen->co;
			last_input_screen->old_li = last_input_screen->li;
		}
		/*
		 * About zones:
		 * The input line is divided into "zones".  A "zone" is set above,
		 * and is the width of the screen minus 20 (by default).  The input
		 * line, as displayed, is therefore composed of the current "zone",
		 * plus 10 characters from the previous zone, plus 10 characters 
		 * from the next zone.  When the cursor moves to an adjacent zone,
		 * (by going into column 9 from the right or left of the edge), the
		 * input line is redrawn.  There is one catch.  The first "zone"
		 * includes the first ten characters of the input line.
		 */
		old_zone = START_ZONE;

		/*
		 * The BEGINNING of the current "zone" is a calculated value:
		 *	The number of characters since the origin of the input buffer
		 *	is the number of printable chars in the input prompt plus the
		 *	current position in the input buffer.  We subtract from that
		 * 	the WIDTH delta to take off the first delta, which doesnt
		 *	count towards the width of the zone.  Then we divide that by
		 * 	the size of the zone, to get an integer, then we multiply it
		 * 	back.  This gives us the first character on the screen.  We
		 *	add WIDTH to the result in order to get the start of the zone
		 *	itself.
		 * The END of the current "zone" is just the beginning plus the width.
		 * If we have moved to an adjacent "zone" since last time, we want to
		 * 	completely redraw the input line.
		 */
		START_ZONE = ((INPUT_PROMPT_LEN + THIS_POS - WIDTH) / ZONE) * ZONE + WIDTH;
		END_ZONE = START_ZONE + ZONE;

		if (old_zone != START_ZONE)
			update = UPDATE_ALL;

		/*
		 * Now that we know where the "zone" is in the input buffer, we can
		 * easily calculate where where we want to start displaying stuff
		 * from the INPUT_BUFFER.  If we're in the first "zone", then we will
		 * output from the beginning of the buffer.  If we're not in the first
		 * "zone", then we will begin to output from 10 characters to the
		 * left of the zone, after adjusting for the length of the prompt.
		 */
		if (START_ZONE == WIDTH)
			INPUT_ONSCREEN = 0;
		else
			INPUT_ONSCREEN = START_ZONE - WIDTH - INPUT_PROMPT_LEN;

		/*
		 * And the cursor is simply how many characters away THIS_POS is
		 * from the first column on the screen.
		 */
		if (INPUT_ONSCREEN == 0)
			INPUT_CURSOR = INPUT_PROMPT_LEN + THIS_POS;
		else
			INPUT_CURSOR = THIS_POS - INPUT_ONSCREEN;

		/*
		 * If the cursor moved, or if we're supposed to do a full update,
		 * then redrwa the entire input line.
		 */
		if (update == UPDATE_ALL)
		{
			term_move_cursor(0, INPUT_LINE);

			/*
			 * If the input line is NOT empty, and we're starting the
			 * display at the beginning of the input buffer, then we
			 * output the prompt first.
			 */
			if (INPUT_ONSCREEN == 0 && INPUT_PROMPT && *INPUT_PROMPT)
			{
				/*
				 * Forcibly turn on echo.
				 */
				int	echo = term_echo(1);
	
				/*
				 * Crop back the input prompt so it does not extend
				 * past the end of the zone.
				 */
				if (INPUT_PROMPT_LEN > (last_input_screen->co - WIDTH))
					INPUT_PROMPT_LEN = last_input_screen->co - WIDTH - 1;

				/*
				 * Output the prompt.
				 */
				output_with_count(INPUT_PROMPT, 0, 1);

				/*
				 * Turn the echo back to what it was before,
				 * and output the rest of the input buffer.
				 */
				term_echo(echo);
				safe_puts(INPUT_BUFFER, last_input_screen->co - INPUT_PROMPT_LEN, echo);
			}

			/*
			 * Otherwise we just output whatever we have.
			 */
			else if (echo)
				safe_puts(&(INPUT_VISIBLE), last_input_screen->co, echo);

			/*
			 * Clear the rest of the input line and reset the cursor
			 * to the current input position.
			 */
			term_clear_to_eol();
		}
		else if (update == UPDATE_FROM_CURSOR)
		{
			/*
			 * Move the cursor to where its supposed to be,
			 * Figure out how much we can output from here,
			 * and then output it.
			 */
#ifdef __EMXPM__
			int echo = term_echo(1);
#endif
			term_move_cursor(INPUT_CURSOR, INPUT_LINE);
			max = last_input_screen->co - (THIS_POS - INPUT_ONSCREEN);
			if (INPUT_ONSCREEN == 0 && INPUT_PROMPT && *INPUT_PROMPT)
				max -= INPUT_PROMPT_LEN;
	
			if ((len = strlen(&(THIS_CHAR))) > max)
				len = max;
			safe_puts(&(THIS_CHAR), len, echo);
			term_clear_to_eol();
		}
		term_move_cursor(INPUT_CURSOR, INPUT_LINE);
		term_echo(1);
		term_flush();
	}
	last_input_screen = os;
	current_window = saved_current_window;
}

void change_input_prompt (int direction)
{
	if (!last_input_screen->promptlist)
	{
		strcpy(INPUT_BUFFER, last_input_screen->saved_input_buffer);
		THIS_POS = last_input_screen->saved_buffer_pos;
		MIN_POS = last_input_screen->saved_min_buffer_pos;
		*last_input_screen->saved_input_buffer = '\0';
		last_input_screen->saved_buffer_pos = 0;
		last_input_screen->saved_min_buffer_pos = 0;
		update_input(UPDATE_ALL);
	}

	else if (direction == -1)
		update_input(UPDATE_ALL);

	else if (!last_input_screen->promptlist->next)
	{
		strcpy(last_input_screen->saved_input_buffer, INPUT_BUFFER);
		last_input_screen->saved_buffer_pos = THIS_POS;
		last_input_screen->saved_min_buffer_pos = MIN_POS;
		*INPUT_BUFFER = '\0';
		THIS_POS = MIN_POS = 0;
		update_input(UPDATE_ALL);
	}
}

/* input_move_cursor: moves the cursor left or right... got it? */
extern void	input_move_cursor (int dir)
{
	cursor_to_input();
	if (dir)
	{
		if (THIS_CHAR)
		{
			THIS_POS++;
			term_cursor_right();
		}
	}
	else
	{
		if (THIS_POS > MIN_POS)
		{
			THIS_POS--;
			term_cursor_left();
		}
	}
	update_input(NO_UPDATE);
}

/*
 * set_input: sets the input buffer to the given string, discarding whatever
 * was in the input buffer before 
 */
void	set_input (char *str)
{
	strmcpy(INPUT_BUFFER + MIN_POS, str, INPUT_BUFFER_SIZE - MIN_POS);
	THIS_POS = strlen(INPUT_BUFFER);
}

/*
 * get_input: returns a pointer to the input buffer.  Changing this will
 * actually change the input buffer.  This is a bad way to change the input
 * buffer tho, cause no bounds checking won't be done 
 */
char	*get_input (void)
{
	return (&(MIN_CHAR));
}

/* init_input: initialized the input buffer by clearing it out */
extern void init_input (void)
{
	*INPUT_BUFFER = (char) 0;
	THIS_POS = MIN_POS;
}

/* get_input_prompt: returns the current input_prompt */
extern char	*get_input_prompt (void)
{ 
	return input_prompt; 
}

/*
 * set_input_prompt: sets a prompt that will be displayed in the input
 * buffer.  This prompt cannot be backspaced over, etc.  It's a prompt.
 * Setting the prompt to null uses no prompt 
 */
void	set_input_prompt (Window *win, char *prompt, int unused)
{
	if (prompt)
		malloc_strcpy(&input_prompt, prompt);
	else if (input_prompt)
		malloc_strcpy(&input_prompt, empty_string);
	else
		return;

	update_input(UPDATE_ALL);
}


/* 
 * Why did i put these in this file?  I dunno.  But i do know that the ones 
 * in edit.c didnt have to be here, and i knew the ones that were here DID 
 * have to be here, so i just moved them all to here, so that they would all
 * be in the same place.  Easy enough. (jfn, june 1995)
 */

/*
 * input_forward_word: move the input cursor forward one word in the input
 * line 
 */
BUILT_IN_KEYBINDING(input_forward_word)
{
	cursor_to_input();

	while ((my_isspace(THIS_CHAR) || ispunct(THIS_CHAR)) && (THIS_CHAR))
			THIS_POS++;
	while (!(ispunct(THIS_CHAR) || my_isspace(THIS_CHAR)) && (THIS_CHAR))
			THIS_POS++;
	update_input(UPDATE_JUST_CURSOR);
}

/* input_backward_word: move the cursor left on word in the input line */
BUILT_IN_KEYBINDING(input_backward_word)
{
	cursor_to_input();
	while ((THIS_POS > MIN_POS) && (my_isspace(PREV_CHAR) || ispunct(PREV_CHAR)))
		THIS_POS--;
	while ((THIS_POS > MIN_POS) && !(ispunct(PREV_CHAR) || my_isspace(PREV_CHAR)))
		THIS_POS--;

	update_input(UPDATE_JUST_CURSOR);
}

/* inPut_delete_character: deletes a character from the input line */
BUILT_IN_KEYBINDING(input_delete_character)
{
int	pos;
	cursor_to_input();
	in_completion = STATE_NORMAL;
	if (!THIS_CHAR)
		return;	
	ov_strcpy(&THIS_CHAR, &NEXT_CHAR);
	if (!(termfeatures & TERM_CAN_DELETE))
		update_input(UPDATE_FROM_CURSOR);
	else
	{
		term_delete(1);
		if (INPUT_ONSCREEN == 0)		/* UGH! */
			pos = last_input_screen->co - INPUT_PROMPT_LEN - 1;
		else
			pos = INPUT_ONSCREEN + last_input_screen->co - 1;

		if (pos < strlen(INPUT_BUFFER))
		{
			term_move_cursor(last_input_screen->co - 1, INPUT_LINE);
			term_putchar(INPUT_BUFFER[pos]);
			term_move_cursor(INPUT_CURSOR, INPUT_LINE);
		}
		update_input(NO_UPDATE);
	}
}

/* input_backspace: does a backspace in the input buffer */
BUILT_IN_KEYBINDING(input_backspace)
{
	cursor_to_input();
	if (THIS_POS > MIN_POS)
	{
		char	*ptr = NULL;
		int	pos;

		ptr = LOCAL_COPY(&THIS_CHAR);
		strcpy(&(PREV_CHAR), ptr);
		THIS_POS--;
		term_cursor_left();
		if (THIS_CHAR)
		{
			if (!(termfeatures & TERM_CAN_DELETE))
				update_input(UPDATE_FROM_CURSOR);
			else
			{
				term_delete(1);
				pos = INPUT_ONSCREEN + last_input_screen->co - 1;
				/*
				 * If the prompt is visible right now,
				 * then we have to cope with that.
				 */
				if (START_ZONE == WIDTH)
					pos -= INPUT_PROMPT_LEN;
				if (pos < strlen(INPUT_BUFFER))
				{
					term_move_cursor(last_input_screen->co - 1, INPUT_LINE);
					term_putchar(INPUT_BUFFER[pos]);
				}
				update_input(UPDATE_JUST_CURSOR);
			}
		}
		else
		{
			term_putchar(' ');
#ifndef __EMX__
			term_cursor_left();
			update_input(NO_UPDATE);
#else
			update_input(UPDATE_FROM_CURSOR);
#endif
		}
	}
	if (THIS_POS == MIN_POS)
		HOLDLAST = NULL;
	in_completion = STATE_NORMAL;
	*new_nick = 0;
}

/*
 * input_beginning_of_line: moves the input cursor to the first character in
 * the input buffer 
 */
BUILT_IN_KEYBINDING(input_beginning_of_line)
{
	cursor_to_input();
	THIS_POS = MIN_POS;
	update_input(UPDATE_JUST_CURSOR);
}

/*
 * input_beginning_of_line: moves the input cursor to the first character in
 * the input buffer 
 */
BUILT_IN_KEYBINDING(new_input_beginning_of_line)
{
	cursor_to_input();
	THIS_POS = MIN_POS;
	update_input(UPDATE_JUST_CURSOR);
	extended_handled = 1;
}

/*
 * input_end_of_line: moves the input cursor to the last character in the
 * input buffer 
 */
BUILT_IN_KEYBINDING(input_end_of_line)
{
	cursor_to_input();
	THIS_POS = strlen(INPUT_BUFFER);
	update_input(UPDATE_JUST_CURSOR);
}

BUILT_IN_KEYBINDING(input_delete_to_previous_space)
{
	int	old_pos;
	char	c;

	cursor_to_input();
	old_pos = THIS_POS;
	c = THIS_CHAR;

	while (!my_isspace(THIS_CHAR) && THIS_POS >= MIN_POS)
		THIS_POS--;

	if (THIS_POS < old_pos)
	{
		strcpy(&(NEXT_CHAR), &(INPUT_BUFFER[old_pos]));
		THIS_POS++;
	}

	update_input(UPDATE_FROM_CURSOR);
}


/*
 * input_delete_previous_word: deletes from the cursor backwards to the next
 * space character. 
 */
BUILT_IN_KEYBINDING(input_delete_previous_word)
{
	int	old_pos;
	char	c;

	cursor_to_input();
	old_pos = THIS_POS;
	while ((THIS_POS > MIN_POS) && (my_isspace(PREV_CHAR) || ispunct(PREV_CHAR)))
		THIS_POS--;
	while ((THIS_POS > MIN_POS) && !(ispunct(PREV_CHAR) || my_isspace(PREV_CHAR)))
		THIS_POS--;
	c = INPUT_BUFFER[old_pos];
	INPUT_BUFFER[old_pos] = (char) 0;
	malloc_strcpy(&cut_buffer, &THIS_CHAR);
	INPUT_BUFFER[old_pos] = c;
	strcpy(&(THIS_CHAR), &(INPUT_BUFFER[old_pos]));
	update_input(UPDATE_FROM_CURSOR);
}

/*
 * input_delete_next_word: deletes from the cursor to the end of the next
 * word 
 */
BUILT_IN_KEYBINDING(input_delete_next_word)
{
	int	pos;
	char	*ptr = NULL,
		c;

	cursor_to_input();
	pos = THIS_POS;
	while ((my_isspace(INPUT_BUFFER[pos]) || ispunct(INPUT_BUFFER[pos])) && INPUT_BUFFER[pos])
		pos++;
	while (!(ispunct(INPUT_BUFFER[pos]) || my_isspace(INPUT_BUFFER[pos])) && INPUT_BUFFER[pos])
		pos++;
	c = INPUT_BUFFER[pos];
	INPUT_BUFFER[pos] = (char) 0;
	malloc_strcpy(&cut_buffer, &(THIS_CHAR));
	INPUT_BUFFER[pos] = c;
	malloc_strcpy(&ptr, &(INPUT_BUFFER[pos]));
	strcpy(&(THIS_CHAR), ptr);
	new_free(&ptr);
	update_input(UPDATE_FROM_CURSOR);
}

/*
 * input_add_character: adds the character c to the input buffer, repecting
 * the current overwrite/insert mode status, etc 
 */
BUILT_IN_KEYBINDING(input_add_character)
{
	int	display_flag = NO_UPDATE;

	if (last_input_screen->promptlist)
		term_echo(last_input_screen->promptlist->echo);
		
	cursor_to_input();
        if (THIS_POS >= INPUT_BUFFER_SIZE)
	{
		term_echo(1);
		return;
	}
                                                        
	if (get_int_var(INSERT_MODE_VAR))
	{
		if (THIS_CHAR)
		{
			char	*ptr = NULL;

			ptr = alloca(strlen(&(THIS_CHAR)) + 1);
			strcpy(ptr, &(THIS_CHAR));
			THIS_CHAR = key;
			NEXT_CHAR = 0;
			ADD_TO_INPUT(ptr);
			if (termfeatures & TERM_CAN_INSERT)
				term_insert(key);
			else
			{
				term_putchar(key);
				if (NEXT_CHAR)
				    display_flag = UPDATE_FROM_CURSOR;
				else
				    display_flag = NO_UPDATE;
			}
		}
		else
		{
			THIS_CHAR = key;
			NEXT_CHAR = 0;
			term_putchar(key);
		}
	}
	else
	{
		if (THIS_CHAR == 0)
			NEXT_CHAR = 0;
		THIS_CHAR = key;
		term_putchar(key);
	}
	
	if (!THIS_POS)
		HOLDLAST = current_window->display_ip;
	THIS_POS++;
#ifdef GUI1
        gui_flush();
#endif
	update_input(display_flag);
	if (in_completion == STATE_COMPLETE && key == ' ' && input_lastmsg)
	{
		new_free(&input_lastmsg);
		*new_nick = 0;
		in_completion = STATE_NORMAL;
	}
	term_echo(1);
}

/* input_clear_to_eol: erases from the cursor to the end of the input buffer */
BUILT_IN_KEYBINDING(input_clear_to_eol)
{
	cursor_to_input();
	malloc_strcpy(&cut_buffer, &(THIS_CHAR));
	THIS_CHAR = 0;
	term_clear_to_eol();
	update_input(NO_UPDATE);

}

/*
 * input_clear_to_bol: clears from the cursor to the beginning of the input
 * buffer 
 */
BUILT_IN_KEYBINDING(input_clear_to_bol)
{
	char	*ptr = NULL;
	cursor_to_input();
	malloc_strcpy(&cut_buffer, &(MIN_CHAR));
	cut_buffer[THIS_POS - MIN_POS] = (char) 0;
	malloc_strcpy(&ptr, &(THIS_CHAR));
	MIN_CHAR = (char) 0;
	ADD_TO_INPUT(ptr);
	new_free(&ptr);
	THIS_POS = MIN_POS;
	term_move_cursor(INPUT_PROMPT_LEN, INPUT_LINE);
	term_clear_to_eol();
	update_input(UPDATE_FROM_CURSOR);
}

/*
 * input_clear_line: clears entire input line
 */
BUILT_IN_KEYBINDING(input_clear_line)
{
	cursor_to_input();
	malloc_strcpy(&cut_buffer, INPUT_BUFFER + MIN_POS);
	MIN_CHAR = (char) 0;
	THIS_POS = MIN_POS;
	term_move_cursor(INPUT_PROMPT_LEN, INPUT_LINE);
	term_clear_to_eol();
	update_input(NO_UPDATE);
	if (get_int_var(HISTORY_CIRCLEQ_VAR))
		abort_history_browsing(0);
}

/*
 * input_transpose_characters: swaps the positions of the two characters
 * before the cursor position 
 */
BUILT_IN_KEYBINDING(input_transpose_characters)
{
	cursor_to_input();
	if (last_input_screen->buffer_pos > MIN_POS)
	{
		u_char	c1[3] = { 0 };
		int	pos, end_of_line = 0;

		if (THIS_CHAR)
			pos = THIS_POS;
		else if (strlen(get_input()) > MIN_POS + 2)
		{
			pos = THIS_POS - 1;
			end_of_line = 1;
		}
		else
			return;

		c1[0] = INPUT_BUFFER[pos];
		c1[1] = INPUT_BUFFER[pos] = INPUT_BUFFER[pos - 1];
		INPUT_BUFFER[pos - 1] = c1[0];
		term_cursor_left();
		if (end_of_line)
			term_cursor_left();

		term_putchar(c1[0]);
		term_putchar(c1[1]);
		if (!end_of_line)
			term_cursor_left();
		update_input(NO_UPDATE);
	}
}


BUILT_IN_KEYBINDING(refresh_inputline)
{
	update_input(UPDATE_ALL);
}

/*
 * input_yank_cut_buffer: takes the contents of the cut buffer and inserts it
 * into the input line 
 */
BUILT_IN_KEYBINDING(input_yank_cut_buffer)
{
	char	*ptr = NULL;

	if (cut_buffer)
	{
		malloc_strcpy(&ptr, &(THIS_CHAR));
		/* Ooops... */
		THIS_CHAR = 0;
		ADD_TO_INPUT(cut_buffer);
		ADD_TO_INPUT(ptr);
		new_free(&ptr);
		update_input(UPDATE_FROM_CURSOR);
		THIS_POS += strlen(cut_buffer);
		if (THIS_POS > INPUT_BUFFER_SIZE)
			THIS_POS = INPUT_BUFFER_SIZE;
		update_input(UPDATE_JUST_CURSOR);
	}
}


/* used with input_move_cursor */
#define RIGHT 1
#define LEFT 0

/* BIND functions: */
BUILT_IN_KEYBINDING(forward_character)
{
	input_move_cursor(RIGHT);
}

BUILT_IN_KEYBINDING(backward_character)
{
	input_move_cursor(LEFT);
}

BUILT_IN_KEYBINDING(backward_history)
{
	get_history(PREV);
}

BUILT_IN_KEYBINDING(forward_history)
{
	get_history(NEXT);
}

BUILT_IN_KEYBINDING(toggle_insert_mode)
{
	int tog = get_int_var(INSERT_MODE_VAR);
	tog ^= 1;
	set_int_var(INSERT_MODE_VAR, tog);
}


BUILT_IN_KEYBINDING(send_line)
{
	int	ofs = from_server;
	WaitPrompt	*OldPrompt;
	char	*line;

	from_server = get_window_server(0);

	line = LOCAL_COPY(get_input());	
	unhold_a_window(current_window);
	MIN_CHAR = 0;
	THIS_POS = MIN_POS;
	
	if (last_input_screen->promptlist && last_input_screen->promptlist->type == WAIT_PROMPT_LINE)
	{
		OldPrompt = last_input_screen->promptlist;
		last_input_screen->promptlist = OldPrompt->next;
		(*OldPrompt->func)(OldPrompt->data, line);
		set_input(empty_string);
		new_free(&OldPrompt->data);
		new_free(&OldPrompt->prompt);
		new_free((char **)&OldPrompt);
		change_input_prompt(-1);
	}
	else
	{
		update_input(UPDATE_ALL);
		if (do_hook(INPUT_LIST, "%s", line))
		{
			if (get_int_var(INPUT_ALIASES_VAR))
				parse_line(NULL, line, empty_string, 1, 0, 1);
			else
				parse_line(NULL, line, NULL, 1, 0, 1);
		}
	}
	new_free(&input_lastmsg);
	*new_nick = 0;
	in_completion = STATE_NORMAL;
	HOLDLAST = NULL;
	from_server = ofs;
}

#define METAX(x) \
	BUILT_IN_KEYBINDING( meta ## x ## _char ) \
	{ last_input_screen->meta_hit = (x); }
METAX(39) METAX(38) METAX(37) METAX(36) METAX(35) 
METAX(34) METAX(33) METAX(32) METAX(31) METAX(30)
METAX(29) METAX(28) METAX(27) METAX(26) METAX(25) 
METAX(24) METAX(23) METAX(22) METAX(21) METAX(20)
METAX(19) METAX(18) METAX(17) METAX(16) METAX(15) 
METAX(14) METAX(13) METAX(12) METAX(11) METAX(10)
METAX(9)  METAX(8)  METAX(7)  METAX(6)  METAX(5)
METAX(3)  METAX(2)  METAX(1)
                

BUILT_IN_KEYBINDING(meta4_char)
{
	if (last_input_screen->meta_hit == 4)
		last_input_screen->meta_hit = 0;
	else
		last_input_screen->meta_hit = 4;
}

BUILT_IN_KEYBINDING(quote_char)
{
	last_input_screen->quote_hit = 1;
}

/* These four functions are boomerang functions, which allow the highlight
 * characters to be bound by simply having these functions put in the
 * appropriate characters when you press any key to which you have bound
 * that highlight character. >;-)
 */
BUILT_IN_KEYBINDING(insert_bold)
{
	input_add_character (BOLD_TOG, string);
}

BUILT_IN_KEYBINDING(insert_reverse)
{
	input_add_character (REV_TOG, string);
}

BUILT_IN_KEYBINDING(insert_underline)
{
	input_add_character (UND_TOG, string);
}

BUILT_IN_KEYBINDING(highlight_off)
{
	input_add_character (ALL_OFF, string);
}

BUILT_IN_KEYBINDING(insert_blink)
{
	input_add_character (BLINK_TOG, string);
}

BUILT_IN_KEYBINDING(insert_altcharset)
{
	input_add_character (ALT_TOG, string);
}

/* type_text: the BIND function TYPE_TEXT */
BUILT_IN_KEYBINDING(type_text)
{
	if (!string)
		return;
	for (; *string; string++)
		input_add_character(*string, empty_string);
}

/*
 * clear_screen: the CLEAR_SCREEN function for BIND.  Clears the screen and
 * starts it if it is held 
 */
BUILT_IN_KEYBINDING(clear_screen)
{
	hold_mode(NULL, OFF, 1);
	clear_window_by_refnum(0);
}

/* parse_text: the bindable function that executes its string */
BUILT_IN_KEYBINDING(parse_text)
{
	parse_line(NULL, string, empty_string, 0, 0, 1);
}

/*
 * edit_char: handles each character for an input stream.  Not too difficult
 * to work out.
 */
void	edit_char (u_char key)
{
	void		(*func) (char, char *) = NULL;
	char		*ptr = NULL;
	u_char		extended_key;
	WaitPrompt	*oldprompt;
	int 		xxx_return = 0;

	/* were we waiting for a keypress? */
	if (last_input_screen->promptlist && last_input_screen->promptlist->type == WAIT_PROMPT_KEY)
	{
		unsigned char key_[2] = "\0";
		key_[0] = key;
		oldprompt = last_input_screen->promptlist;
		last_input_screen->promptlist = oldprompt->next;
		(*oldprompt->func)(oldprompt->data, key_);
		new_free(&oldprompt->data);
		new_free(&oldprompt->prompt);
		new_free((char **)&oldprompt);
		set_input(empty_string);
		change_input_prompt(-1);
		xxx_return = 1;
	}
	if (last_input_screen->promptlist && 
		last_input_screen->promptlist->type == WAIT_PROMPT_DUMMY)
	{
		oldprompt = last_input_screen->promptlist;
		last_input_screen->promptlist = oldprompt->next;
		(*oldprompt->func)(oldprompt->data, NULL);
		new_free(&oldprompt->data);
		new_free(&oldprompt->prompt);
		new_free((char **)&oldprompt);
	}

	if (xxx_return)
		return;

#if __bsdi__
	if (key & 0x80)
	{
		if (meta_mode)
		{
			edit_char('\033');
			key &= ~0x80;
		}
		else if (!term_eight_bit())
			key &= ~0x80;
	}
#endif
                                                                                                                                                                
	extended_key = key;


#ifdef __EMX__
	if (key == 0)
		key = 27;
#endif

	/* did we just hit the quote character? */
	if (last_input_screen->quote_hit)
	{
		last_input_screen->quote_hit = 0;
		input_add_character(extended_key, empty_string);
	}
	else
	{
		int	m = last_input_screen->meta_hit;
		int	i;

		if ((i = get_binding(m, key, &func, &ptr)))
		{
			if (m == 4 && i == 4)
				last_input_screen->meta_hit = 0;
			else
				last_input_screen->meta_hit = i;
		}
		else if (last_input_screen->meta_hit != 4)
			last_input_screen->meta_hit = 0;
		if (func)
			func(extended_key, SAFE(ptr));
	}
}

BUILT_IN_KEYBINDING(my_scrollback)
{
	scrollback_backwards(key, string);
	extended_handled = 1;
}

BUILT_IN_KEYBINDING(my_scrollforward)
{
	scrollback_forwards(key, string);
	extended_handled = 1;
}

BUILT_IN_KEYBINDING(my_scrollend)
{
	scrollback_end(key, string);
	extended_handled = 1;
}

static int oiwc;
static int osu;

BUILT_IN_KEYBINDING(window_key_balance)
{
	oiwc = in_window_command;
	osu = status_update_flag;
	in_window_command = 1;
	set_display_target(NULL, LOG_CURRENT);
	rebalance_windows(current_window->screen);
	in_window_command = oiwc;
	status_update_flag = osu;
	update_all_windows();
	update_all_status(current_window, NULL, 0);
	reset_display_target();
}

BUILT_IN_KEYBINDING(window_grow_one)
{
	oiwc = in_window_command;
	osu = status_update_flag;
	in_window_command = 1;
	set_display_target(NULL, LOG_CURRENT);
	resize_window(1, current_window, 1);
	in_window_command = oiwc;
	status_update_flag = osu;
	update_all_windows();
	update_all_status(current_window, NULL, 0);
	reset_display_target();
}

BUILT_IN_KEYBINDING(window_key_hide)
{
	oiwc = in_window_command;
	osu = status_update_flag;
	in_window_command = 1;
	set_display_target(NULL, LOG_CURRENT);
	hide_window(current_window);
	in_window_command = oiwc;
	status_update_flag = osu;
	update_all_windows();
	update_all_status(current_window, NULL, 0);
	reset_display_target();
}

BUILT_IN_KEYBINDING(window_key_kill)
{
	oiwc = in_window_command;
	osu = status_update_flag;
	in_window_command = 1;
	set_display_target(NULL, LOG_CURRENT);
	delete_window(current_window);
	in_window_command = oiwc;
	status_update_flag = osu;
	update_all_windows();
	update_all_status(current_window, NULL, 0);
	reset_display_target();
}

BUILT_IN_KEYBINDING(window_key_list)
{
	oiwc = in_window_command;
	osu = status_update_flag;
	in_window_command = 1;
	set_display_target(NULL, LOG_CURRENT);
	window_list(current_window, NULL, NULL);
	in_window_command = oiwc;
	status_update_flag = osu;
	reset_display_target();
}

BUILT_IN_KEYBINDING(window_key_move)
{
	oiwc = in_window_command;
	osu = status_update_flag;
	in_window_command = 1;
	set_display_target(NULL, LOG_CURRENT);
	move_window(current_window, 1);
	in_window_command = oiwc;
	status_update_flag = osu;
	update_all_windows();
	update_all_status(current_window, NULL, 0);
	reset_display_target();
}

BUILT_IN_KEYBINDING(window_shrink_one)
{
	oiwc = in_window_command;
	osu = status_update_flag;
	in_window_command = 1;
	set_display_target(NULL, LOG_CURRENT);
	resize_window(1, current_window, -1);
	in_window_command = oiwc;
	status_update_flag = osu;
	update_all_windows();
	update_all_status(current_window, NULL, 0);
	reset_display_target();
}

extern int in_window_command;

static void handle_swap(int windownum)
{
char *p = NULL;
	malloc_sprintf(&p, "SWAP %d", windownum);
	windowcmd(NULL, p, NULL, NULL, 0);
	new_free(&p);
	set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
	target_window = NULL;
	update_input(UPDATE_ALL);
	update_all_windows();
}

BUILT_IN_KEYBINDING(window_swap1)
{
	handle_swap(1);
	extended_handled = 1;
}

BUILT_IN_KEYBINDING(window_swap2)
{
	handle_swap(2);
	extended_handled = 1;
}

BUILT_IN_KEYBINDING(window_swap3)
{
	handle_swap(3);
	extended_handled = 1;
}

BUILT_IN_KEYBINDING(window_swap4)
{
	handle_swap(4);
	extended_handled = 1;
}

BUILT_IN_KEYBINDING(window_swap5)
{
	handle_swap(5);
	extended_handled = 1;
}

BUILT_IN_KEYBINDING(window_swap6)
{
	handle_swap(6);
	extended_handled = 1;
}

BUILT_IN_KEYBINDING(window_swap7)
{
	handle_swap(7);
	extended_handled = 1;
}
BUILT_IN_KEYBINDING(window_swap8)
{
	handle_swap(8);
	extended_handled = 1;
}
BUILT_IN_KEYBINDING(window_swap9)
{
	handle_swap(9);
	extended_handled = 1;
}
BUILT_IN_KEYBINDING(window_swap10)
{
	handle_swap(10);
	extended_handled = 1;
}


BUILT_IN_KEYBINDING(cpu_saver_on)
{
	cpu_saver = 1;
	update_all_status(current_window, NULL, 0);
}

BUILT_IN_KEYBINDING(input_unclear_screen)
{
	hold_mode(NULL, OFF, 1);
	unclear_window_by_refnum(0);
}


/*
 * type: The TYPE command.  This parses the given string and treats each
 * character as though it were typed in by the user.  Thus key bindings 
 * are used for each character parsed.  Special case characters are control 
 * character sequences, specified by a ^ follow by a legal control key.  
 * Thus doing "/TYPE ^B" will be as tho ^B were hit at the keyboard, 
 * probably moving the cursor backward one character.
 *
 * This was moved from keys.c, because it certainly does not belong there,
 * and this seemed a reasonable place for it to go for now.
 */
BUILT_IN_COMMAND(typecmd)
{
	int	c;
	char	key;

	while (*args)
	{
		if (*args == '^')
		{
			switch (*(++args))
			{
			    case '?':
			    {
				key = '\177';
				args++;
				break;
			    }
			    default:
			    {
				c = *(args++);
				if (islower(c))
					c = toupper(c);
				if (c < 64)
				{
					say("Invalid key sequence: ^%c", c);
					return;
				}
				key = c - 64;
				break;
			    }
			}
		}
		else if (*args == '\\')
		{
			key = *++args;
			args++;
		}
		else
			key = *(args++);

		edit_char(key);
	}
}


#define TABDEBUG

struct _name_type {
	char name[10];
	int len;
};

 struct _name_type 
 	name_type[] = {	
			{"MP3",		2},
			{"EXEC",	4}, 
			{"LS",		2}, 
			{"LOAD",	4}, 
			{"SERVER",	6}, 
 			{"MSG",		1}, 
			{"CD",		2},
			{"SEND",	2},
			{"SHARE",	2},
			{ "",		0}};

enum tab_types { MP3_FILE, EXEC, LS, LOAD, SERVER, IMSG, CHDIR, CDCC, SHARE};

char *get_home_dir(char *possible, int *count)
{
#ifdef HAVE_GETPWENT
struct passwd *pw;
char *booya = NULL;
int len = 0;
char *q;
char str[BIG_BUFFER_SIZE+1];
	(*count) = 0;
	if (possible)
	{
		possible++;
		len = strlen(possible);
	}
	while ((pw = getpwent()))
	{
		*str = 0;
		if ((q = strrchr(pw->pw_dir, '/')))
			q++;
		if (possible && *possible && q && strncmp(possible, q, len))
			continue;
		if (q && *q)
		{
			strmopencat(str, BIG_BUFFER_SIZE, "~", q, NULL);
			m_s3cat(&booya, space, str);
		}
		else
			m_s3cat(&booya, space, pw->pw_dir);
		(*count)++;
	}
	endpwent();
	if (*count)
		return booya;
#endif
	return NULL;
}

char *get_completions(enum completion type, char *possible, int *count, char **suggested)
{
char *booya = NULL;
char *path = NULL;
char *path2, *freeme;
glob_t globbers;
int numglobs = 0, i;
int globtype = GLOB_MARK;

#if defined(__EMX__) || defined(WINNT)
	if (possible && *possible)
		convert_unix(possible);
#endif

	switch(type)
	{
		case SERVER_COMPLETION:
		{
			for (i = 0; i < server_list_size(); i++)
			{
				if (possible && my_strnicmp(possible, get_server_name(i), strlen(possible)))
					continue;
				m_s3cat(&booya, space, get_server_name(i));
				(*count)++;
			}
			return booya;
		}
		case TABKEY_COMPLETION:
		{
			Msgs *n;
			*count = 0;
			if (possible)
			{
				if (*possible == '#' && *(possible+1))
				{
					ChannelStruct *chan;
					possible++;
					for (chan = current_window->nchannels; chan; chan = chan->next)
					{
						if (my_strnicmp(possible, chan->channel, strlen(possible)))
							continue;
						m_s3cat(&booya, space, chan->channel);
						(*count)++;
					}
				}
				else
				{
					List *nicks = NULL, *n1, *last = NULL;
					
					for (n = gettabkey(NULL, 0); n; n = n->next)
					{
						if (nicks && find_in_list((List **)&nicks, n->nick, 0))
							continue;
						n1 = (List *)new_malloc(sizeof(List));
						n1->name = m_strdup(n->nick);
						n1->next = nicks;
						nicks = n1;
					}
					for (n1 = nicks; n1; n1 = n1->next)
					{
						if (possible && my_strnicmp(possible, n1->name, strlen(possible)))
							continue;
						m_s3cat(&booya, space, n1->name);
						(*count)++;
					}
					while ((n1 = nicks))
					{
						last = n1->next;
						new_free(&n1->name);
						new_free(&n1);
						nicks = last;
					}
					if (*count == 1 && !strcmp(possible, booya))
					{
						new_free(&booya);				
						(*count) = 0;
						possible = NULL;
					}
				}
			}
			if (((*count) == 0) || ((*count) == 1))
			{
				/* 
				 * nothing specified for a match. 
				 * so grab the first one, if available.
				 * or, only one match was available.
				 */
				if (possible && count == 0)
					possible = NULL;
				if ((n = gettabkey(possible, 1)) && !booya)
				{
					(*count) = 1;
					booya = m_strdup(n->nick);
#if 0
					if (suggested)
						*suggested = n->type;
#endif
				}
			}
			return booya;
		}
		case COM_COMPLETION:
			command_completion(0, NULL);
			return NULL;
		case CHAN_COMPLETION:
		{
			NickStruct *cnick = NULL;
			ChannelStruct *chan = NULL;
			if (current_window->current_channel)
				chan = (ChannelStruct *)find_in_list((List **)&current_window->nchannels, current_window->current_channel, 0);
			if (possible && *possible == '#')
				possible++;
			if (chan)
			{
				cnick = chan->nicks;
				for (cnick = chan->nicks; cnick; cnick = cnick->next)
				{
					if (possible && my_strnicmp(cnick->nick, possible, strlen(possible)))
						continue;
					(*count)++;
					m_s3cat(&booya, space, cnick->nick);					
				}
			}
			if (!booya)
			{
				for (chan = current_window->nchannels; chan; chan = chan->next)
				{
					for (cnick = chan->nicks; cnick; cnick = cnick->next)
					{
						if (possible && my_strnicmp(cnick->nick, possible, strlen(possible)))
							continue;
						(*count)++;
						m_s3cat(&booya, space, cnick->nick);					
					}
				}
			}
			return booya;
		}
		case LOAD_COMPLETION:
		{
			if (!possible)
#ifdef WINNT
				path = m_sprintf("~/TekNap/");
#else
				path = m_sprintf("~/.TekNap/");
#endif
			else
			{
				if (*possible == '/' || *possible == '~' || *possible == '.')
					path = m_sprintf("%s*", possible);
				else
					path = m_sprintf("~/%s*", possible);
			}
			break;
		}
		case EXEC_COMPLETION:
		{
			if (!possible)
#if !defined(__EMX__) && !defined(WINNT)
				path = m_sprintf("/usr/bin/*");
#elif defined(__EMX__)
				path = m_sprintf("C:/OS2/*.EXE");
#elif defined(WINNT) /* Might need something for WinNT itself C:/WINNT/SYSTEM32/ *.EXE */
				path = m_sprintf("C:/WINDOWS/COMMAND/*.EXE");
#endif
			else
			{			
				if (*possible == '~' && (*(possible+1) != '/'))
				{
					if ((booya = get_home_dir(possible, count)))
						return booya;
				}
#if defined(__EMX__) || defined(WINNT) 
				if (*possible == '/' || *possible == '~' || *possible == '.' || (strlen(possible) > 3 && *(possible+1) == ':' && *(possible+2) == '/'))
					path = m_sprintf("%s*.EXE", possible);
				else
					path = m_sprintf("~/%s*.EXE", possible);
#else
				if (*possible == '/' || *possible == '~' || *possible == '.')
					path = m_sprintf("%s*", possible);
				else
					path = m_sprintf("~/%s*", possible);
#endif
			}
			break;
		}
		case CDCC_COMPLETION:
		{
			char *dl = get_string_var(DOWNLOAD_DIRECTORY_VAR);
			if (!dl)
				dl = "~";
			if (!possible)
				path = m_sprintf("%s%s%s", dl, strrchr(dl, '/')?empty_string:"/", "*");
			else
			{
				if (*possible == '~' && (*(possible+1) != '/'))
				{
					if ((booya = get_home_dir(possible, count)))
						return booya;
				}
#if defined(__EMX__) || defined(WINNT)
				if (*possible == '/' || *possible == '~' || *possible == '.' || (strlen(possible) > 3 && *(possible+1) == ':' && *(possible+2) == '/'))
#else
				if (*possible == '/' || *possible == '~' || *possible == '.')
#endif
					path = m_sprintf("%s*", possible);
				else
					path = m_sprintf("~/%s*", possible); 
			}
			break;
		}
		case FILE_COMPLETION:
		{
			globtype |= GLOB_TILDE;
			if (!possible)
				path = m_sprintf("~/*");
			else if (*possible == '~' && (*(possible+1) != '/'))
			{
				if ((booya = get_home_dir(possible, count)))
					return booya;
				path = m_strdup(possible);
			}
			else if (*possible == '/' || *possible == '~' || *possible == '.')
				path = m_sprintf("%s*", possible);
			else
				path = m_sprintf("~/%s*", possible); 
#if 0
				path = m_sprintf("%s%s%s", possible, strrchr(possible, '/') ? empty_string: "/", strchr(possible, '*')?empty_string:"*");
#endif
			break;
		}
		default:
			return NULL;
	}

	freeme = path2 = expand_twiddle(path);
	if (!path2)
		path2 = path;
	memset(&globbers, 0, sizeof(glob_t));
	numglobs = bsd_glob(path2, globtype, NULL, &globbers);
	for (i = 0; i < globbers.gl_pathc; i++)
	{
		if (strchr(globbers.gl_pathv[i], ' '))
		{
			int len = strlen(globbers.gl_pathv[i])+4;
			char *b = alloca(len+1);
			*b = 0;
			strmopencat(b, len, "\"", globbers.gl_pathv[i], "\"", NULL);
			m_s3cat(&booya, space, b);
		}
		else
			m_s3cat(&booya, space, globbers.gl_pathv[i]);
		(*count)++;
	}
	bsd_globfree(&globbers);
	new_free(&freeme);
	new_free(&path);
	return booya;
}

/*
 * this is an oddball routine that attempts to figure out where 
 * the last char is in the completes array, that is equal in all 
 * the array conditions. when found it sets the input prompt to the
 * command [inp] and the greatest amount of chars found common in all
 * array elements.
 */
void set_input_best(enum completion type, char *inp, char *old, int count, char **completes)
{
int	i, 
	c,
	j = 0, 
	match = 0;
char	buffer[BIG_BUFFER_SIZE+1];

	match = strlen(old);
	if (count == 2)
	{
#if !defined(__EMX__) && !defined(WINNT)
		if (type == CHAN_COMPLETION || type == NICK_COMPLETION || type == TABKEY_COMPLETION)
			i = strieq(completes[0], completes[1]);
		else
#endif
			i = streq(completes[0], completes[1]);
		completes[0][i] = 0;
	}
	else
	{
		int getout;
		i = match - 1;
		while (1)
		{
			getout = 0;
			c = 1;
			for (j = 1; j < count; j++)
			{
				if (!completes[0][i] || !completes[j][i])
					break;
#if defined(__EMX__) || defined(WINNT)
				if (toupper(completes[0][i]) == toupper(completes[j][i]))
					c++;
#else
				if (type == CHAN_COMPLETION || type == NICK_COMPLETION || type == TABKEY_COMPLETION)
				{
					if (toupper(completes[0][i]) == toupper(completes[j][i]))
						c++;
					else
					{
						getout = 1;
						break;
					}
				}
				else
				{
					if (completes[0][i] == completes[j][i])
						c++;
					else
					{
						getout = 1;
						break;
					}
				}
#endif
			}
			if ((c != count) || getout)
				break;
			i++;
		}
		if (i)
			completes[0][i] = 0;
	}
	if (old && *old)
	{
		if (strchr(completes[0], ' '))
			sprintf(buffer, "%s%s\"%s\"", inp, space, i ? completes[0] : old);
		else
			sprintf(buffer, "%s%s%s", inp, space, i ? completes[0] : old);
	}
	else
		strcpy(buffer, completes[0]);
	if (strcmp(buffer, get_input()))
		set_input(buffer);
	if (strchr(completes[0], ' '))
		THIS_POS = strlen(INPUT_BUFFER) - 1;
}

BUILT_IN_KEYBINDING(tab_completion)
{
int	count = 0, 
	wcount = 0;
enum completion type = NO_COMPLETION;
char *inp = NULL;
char *possible = NULL, *old_pos = NULL;
char *cmdchar;
char *suggested = NULL;
int got_space = 0;
int got_comm = 0;
char *get = NULL;

	/* 
	 * is this the != second word, then just complete from the 
	 * channel nicks. if it is the second word, grab the first word, and 
	 * this is used to determine what type of nick completion will be 
	 * done.
	 */
	inp = alloca(strlen(get_input())+2);
	strcpy(inp, get_input() ? get_input() : empty_string);

	if (*inp && inp[strlen(inp)-1] == ' ')
		got_space = 1;
	wcount = word_count(inp);
	if (!(cmdchar = get_string_var(CMDCHARS_VAR)))
		cmdchar = "/";
	if (*inp && (*inp == *cmdchar))
		got_comm = 1;

	switch(wcount)
	{
		case 0:
		{
			type = TABKEY_COMPLETION;
			break;
		}
		case 1:
		{
			if (*inp != *cmdchar)
			{
				type = CHAN_COMPLETION;
				possible = m_strdup(get_input());
				break;
			}
			else if (!got_space)
			{
				type = COM_COMPLETION;
				break;
			}
		}
		default:
		{
			char *p, *old_p;
			int i;

			old_p = p = extract(inp, 0, 0);
			if (wcount > 1)
				old_pos = possible = extract(inp, wcount-1, EOS);
			if ((*p == *cmdchar))
				p++;
			if (possible && (*possible == '"'))
			{
				possible++;
				if (*possible)
					chop(possible, 1);
				else
					possible = NULL;
			}
			if (type == NO_COMPLETION)
				type = CHAN_COMPLETION;
			for (i = 0; *p && name_type[i].len; i++)
			{
				if (!my_strnicmp(p, name_type[i].name, name_type[i].len/*strlen(p)*/))
				{
					switch(i)
					{
						case IMSG:
							type = TABKEY_COMPLETION;
							break;
						case EXEC:
							type = EXEC_COMPLETION;
							break;
						case MP3_FILE:
						case CHDIR:
						case LS:
						case SHARE:
							type = FILE_COMPLETION;
							break;
						case LOAD:
							type = LOAD_COMPLETION;
							break;
						case SERVER:
							type = SERVER_COMPLETION;
							break;
						case CDCC:
							if (wcount == 1 || wcount == 3)
								type = CDCC_COMPLETION;
							break;
					}
					break;
				}
			}
			new_free(&old_p);
			break;
		}
	}
#ifdef TABDEBUG1
	put_it("type = %s", type == TABKEY_COMPLETION ? "TABKEY": 
			    type == CHAN_COMPLETION? "CHAN" : 
			    type == COM_COMPLETION? "COMMAND":
			    type == EXEC_COMPLETION? "EXEC":
			    type == FILE_COMPLETION? "FILE":
			    type == DCC_COMPLETION? "DCC":
			    "NO_COMPLETION");
#endif
do_more_tab:
	count = 0;
	if ((get = get_completions(type, possible, &count, &suggested)))
	{
		char buffer[BIG_BUFFER_SIZE+1];
		char *p = NULL;
		char *old = NULL;
		*buffer = 0;
		if (count == 1)
		{
			if (wcount > 1)
				p = extract(get_input(), 0, wcount - 2);
			else if (suggested && *suggested)
				p = m_3dup("/", suggested, "");
			if (type == TABKEY_COMPLETION)
				snprintf(buffer, BIG_BUFFER_SIZE, "%s %s%s%s ", (p && *p == '/') ? p : "/m", get, (p && (*p != '/'))?space:empty_string, (p && (*p != '/'))?p:empty_string);
			else
			{
				if (wcount == 1 && got_space)
					snprintf(buffer, BIG_BUFFER_SIZE, "%s %s ", get_input(), get);
				else
					snprintf(buffer, BIG_BUFFER_SIZE, "%s%s%s ", p ? p : get, p ? space : empty_string, p ? get : empty_string);
			}
			if ((type == CDCC_COMPLETION || type == LOAD_COMPLETION || 
				type == FILE_COMPLETION) || ((type == EXEC_COMPLETION) && 
					(get[strlen(get)-1] == '/')))
				chop(buffer, 1);
			set_input(buffer);
			if (strchr(get, ' '))
				THIS_POS = strlen(INPUT_BUFFER) - 1;
			new_free(&p);
		}
		else
		{
			char **completes = NULL;
			int c = 0, matches = count;
			completes = RESIZE(completes, char *, count+1);
			if (wcount > 1)
			{
				if (!got_space)
				{
					old = inp;
					old = last_arg(&inp);
					if ((*old == '"'))
					{
						old++;
						chop(old, 1);
					}
				}
			}
			switch(type)
			{
				case LOAD_COMPLETION:
				case FILE_COMPLETION:
				case CDCC_COMPLETION:
				{
					char *n, *use = get, *p, *q = NULL;
					say("Found %d files/dirs", count);
					n = new_next_arg(use, &use);
					while (n && *n)
					{
						q = NULL;
						p = base_name(n);
						if (p && !*p)
						{
							q = LOCAL_COPY(n);
							q[strlen(q)-1] = 0;
							p = base_name(q);
							strcat(p, "/");
						}
						put_it("%s", p && *p ? p : n);
						completes[c++] = n;
						n = new_next_arg(use, &use);
					}
					break;
				}
				case EXEC_COMPLETION:
				{
					char *n, *q, *use = get;
					char path[BIG_BUFFER_SIZE+1];
					int buf_len = 0;
					*path = 0;
					memset(path, 0, sizeof(path));
					n = new_next_arg(use, &use);
					if ((q = strrchr(n, '/')) && *(q+1))
					{
						strncpy(path, n, q - n + 1);
						bitchsay("path = %s", path);
					}
					count = 0;
					*buffer = 0;
					while (n && *n)
					{
						if ((q = strrchr(n, '/')) && *(q+1))
							q++;
						else 
							q = n;
						buf_len = strlen(buffer);
						sprintf(buffer+buf_len, "%15s", q);
						if (++count == 4)
						{
							put_it("%s", buffer);
							count = 0;
							*buffer = 0;
						} 
						completes[c++] = n;
						n = new_next_arg(use, &use);
					}
					if (count)
						put_it("%s", buffer);
					break;
				}
				case SERVER_COMPLETION:
				case TABKEY_COMPLETION:
				case CHAN_COMPLETION:
				{
					char *n, *use = get;
					int buf_len = 0;
					n = new_next_arg(use, &use);
					count = 0;
					*buffer = 0;
					while (n && *n)
					{
						buf_len = strlen(buffer);
						sprintf(buffer+buf_len, "%20s", n);
						if (++count == 3)
						{
							put_it("%s", buffer);
							count = 0;
							*buffer = 0;
						} 
						completes[c++] = n;
						n = new_next_arg(use, &use);
					}
					if (count)
						put_it("%s", buffer);
					break;
				}
				case NO_COMPLETION:
					break;
				default:
					put_it("get = %s[%d]", get, count);
		
			}
			if (old && c)
				set_input_best(type, inp, old, matches, completes);
			new_free((char **)&completes);
		}
		update_input(UPDATE_ALL);
	}
	else if (type == TABKEY_COMPLETION)
	{
		if (get_current_channel_by_refnum(0))
		{
			type = CHAN_COMPLETION;
			goto do_more_tab;
		}
	}
	new_free(&get);
	new_free(&old_pos);
	return;
}

void join_lastinvite(char unused, char *unused1)
{
	if (last_invited)
		send_ncommand(CMDS_JOIN, "%s", last_invited);
	new_free(&last_invited);
}

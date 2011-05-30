/*
 * status.c: handles the status line updating, etc for IRCII 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 * $Id: status.c,v 1.1.1.1 2001/01/17 21:20:21 edwards Exp $
 */


#include "teknap.h"
#include "struct.h"

#include "ircterm.h"
#include "list.h"
#include "server.h"
#include "status.h"
#include "vars.h"
#include "input.h"
#include "window.h"
#include "screen.h"
#include "output.h"
#include "ircaux.h"
#include "napster.h"

#define MY_BUFFER 120

extern char *ltoa (long);

#ifdef Char
#undef Char
#endif
#define Char const char

/*
 * Maximum number of "%" expressions in a status line format.  If you change
 * this number, you must manually change the sprintf() in make_status 
 */
#define STATUS_FUNCTION(x) Char * x (Window *window, int map, int key)
//#define STATUS_FUNCTION(x) static Char * ## x (Window *window, int map, int key)
#define MAX_FUNCTIONS 40
#define MAX_STATUS_USER 19

Status  main_status;
int	main_status_init = 0;

STATUS_FUNCTION(status_nickname);
STATUS_FUNCTION(status_query_nick);
STATUS_FUNCTION(status_right_justify);
STATUS_FUNCTION(status_channel);
STATUS_FUNCTION(status_server);
STATUS_FUNCTION(status_insert_mode);
STATUS_FUNCTION(status_overwrite_mode);
STATUS_FUNCTION(status_user);
STATUS_FUNCTION(status_version);
STATUS_FUNCTION(status_clock);
STATUS_FUNCTION(status_hold_lines);
STATUS_FUNCTION(status_window);
STATUS_FUNCTION(status_refnum);
STATUS_FUNCTION(status_null_function);
STATUS_FUNCTION(status_notify_windows);
STATUS_FUNCTION(status_cpu_saver_mode);
STATUS_FUNCTION(status_position);
STATUS_FUNCTION(status_scrollback);
STATUS_FUNCTION(status_percent);
STATUS_FUNCTION(status_hold);
STATUS_FUNCTION(status_server_stats);
STATUS_FUNCTION(napster_updown);
STATUS_FUNCTION(napster_shared);
STATUS_FUNCTION(napster_download);
STATUS_FUNCTION(napster_load_share);
STATUS_FUNCTION(status_eta);
STATUS_FUNCTION(status_usercount);
STATUS_FUNCTION(status_cloak);
STATUS_FUNCTION(status_lag);
STATUS_FUNCTION(status_topic);
STATUS_FUNCTION(status_sharedir);
STATUS_FUNCTION(status_windowspec);
STATUS_FUNCTION(status_scroll_info);

/*
 * This is how we store status line expandos.
 */
struct status_formats {
	int	map;
	char 	key;
	Char	*(*callback_function)(Window *, int, int);
	char	**format_var;
	int	format_set;
};


char	
	*hold_lines_format = NULL,
	*channel_format = NULL,
	*notify_format = NULL,
	*cpu_saver_format = NULL,
	*nick_format = NULL,
	*query_format = NULL,
	*server_format = NULL,
	*lag_format = NULL,
	*clock_format = NULL,
	*stats_format = NULL,
	*mode_format = NULL; /* not used yet */
	
/*
 * This is the list of possible expandos.  Note that you should not use
 * the '{' character, as it would be confusing.  It is already used for 
 * specifying the map.
 */
struct status_formats status_expandos[] = {
{ 0, 'A', napster_updown,	NULL,			-1 },
{ 0, 'B', status_hold_lines,     &hold_lines_format,	STATUS_HOLD_LINES_VAR },
{ 0, 'C', status_channel,        &channel_format,	STATUS_CHANNEL_VAR },
{ 0, 'D', napster_shared,	NULL,			-1 },
{ 0, 'E', status_scrollback,     NULL, 			-1 },
{ 0, 'F', status_notify_windows, &notify_format,	STATUS_NOTIFY_VAR },
{ 0, 'G', napster_load_share,	NULL,			-1 },
{ 0, 'H', status_hold,		 NULL,			-1 },
{ 0, 'I', status_insert_mode,    NULL,			-1 },
{ 0, 'J', status_cpu_saver_mode, &cpu_saver_format,	STATUS_CPU_SAVER_VAR },
{ 0, 'L', status_lag,		 NULL,			-1 },
{ 0, 'N', status_nickname,	 &nick_format,		STATUS_NICK_VAR },
{ 0, 'O', status_overwrite_mode, NULL,			-1 },
{ 0, 'P', status_position,       NULL,			-1 },
{ 0, 'Q', status_query_nick,     &query_format,		STATUS_QUERY_VAR },
{ 0, 'R', status_refnum,         NULL, 			-1 },
{ 0, 'S', status_server,         &server_format,     	STATUS_SERVER_VAR },
{ 0, 'T', status_clock,          &clock_format,      	STATUS_CLOCK_VAR },
{ 0, 'd', napster_download,	NULL,			-1 },
{ 0, 'c', status_usercount,	NULL,			-1 },
{ 0, 'e', status_eta,		NULL,			-1 },
{ 0, 'h', status_cloak,		NULL,			-1 },
{ 0, 's', status_server_stats,	 &stats_format,		STATUS_STATS_VAR },
{ 0, 't', status_topic,		NULL,			-1 },
{ 0, 'u', status_sharedir,	NULL,			-1 },
{ 0, 'U', status_user,		 NULL, 			-1 },
{ 0, 'V', status_version,	 NULL, 			-1 },
{ 0, 'W', status_window,	 NULL, 			-1 },
{ 0, 'X', status_user,		 NULL, 			-1 },
{ 0, 'Y', status_user,		 NULL, 			-1 },
{ 0, 'Z', status_user,		 NULL, 			-1 },
{ 0, '%', status_percent,	 NULL, 			-1 },
{ 0, '>', status_right_justify,	 NULL, 			-1 },
{ 0, '0', status_user,		 NULL, 			-1 },
{ 0, '1', status_user,		 NULL, 			-1 },
{ 0, '2', status_user,		 NULL, 			-1 },
{ 0, '3', status_user,		 NULL, 			-1 },
{ 0, '4', status_user,		 NULL, 			-1 },
{ 0, '5', status_user,		 NULL, 			-1 },
{ 0, '6', status_user,		 NULL, 			-1 },
{ 0, '7', status_user,		 NULL, 			-1 },
{ 0, '8', status_user,		 NULL, 			-1 },
{ 0, '9', status_user,		 NULL, 			-1 },
{ 0, '.', status_windowspec,	 NULL, 			-1 },
{ 1, '0', status_user,	 NULL, 			-1 },
{ 1, '1', status_user,	 NULL, 			-1 },
{ 1, '2', status_user,	 NULL, 			-1 },
{ 1, '3', status_user,	 NULL, 			-1 },
{ 1, '4', status_user,	 NULL, 			-1 },
{ 1, '5', status_user,	 NULL, 			-1 },
{ 1, '6', status_user,	 NULL, 			-1 },
{ 1, '7', status_user,	 NULL, 			-1 },
{ 1, '8', status_user,	 NULL, 			-1 },
{ 1, '9', status_user,	 NULL, 			-1 },
{ 1, 'F', status_notify_windows,&notify_format,		STATUS_NOTIFY_VAR },
{ 1, 'K', status_scroll_info,	NULL,			-1 },
{ 1, 'S', status_server,        &server_format,     	STATUS_SERVER_VAR },
{ 2, '0', status_user,	 	NULL, 			-1 },
{ 2, '1', status_user,	 	NULL, 			-1 },
{ 2, '2', status_user,	 	NULL, 			-1 },
{ 2, '3', status_user,	 	NULL, 			-1 },
{ 2, '4', status_user,		NULL, 			-1 },
{ 2, '5', status_user,	 	NULL, 			-1 },
{ 2, '6', status_user,	 	NULL, 			-1 },
{ 2, '7', status_user,	 	NULL,			-1 },
{ 2, '8', status_user,	 	NULL, 			-1 },
{ 2, '9', status_user,	 	NULL, 			-1 },
{ 2, 'S', status_server,        &server_format,     	STATUS_SERVER_VAR },
{ 3, '0', status_user,	 	NULL, 			-1 },
{ 3, '1', status_user,	 	NULL, 			-1 },
{ 3, '2', status_user,	 	NULL, 			-1 },
{ 3, '3', status_user,	 	NULL, 			-1 },
{ 3, '4', status_user,	 	NULL, 			-1 },
{ 3, '5', status_user,	 	NULL, 			-1 },
{ 3, '6', status_user,	 	NULL, 			-1 },
{ 3, '7', status_user,	 	NULL, 			-1 },
{ 3, '8', status_user,	 	NULL, 			-1 },
{ 3, '9', status_user,	 	NULL, 			-1 }
};

#define NUMBER_OF_EXPANDOS (sizeof(status_expandos) / sizeof(struct status_formats))


/*
 * convert_sub_format: This is used to convert the formats of the
 * sub-portions of the status line to a format statement specially designed
 * for that sub-portions.  convert_sub_format looks for a single occurence of
 * %c (where c is passed to the function). When found, it is replaced by "%s"
 * for use is a sprintf.  All other occurences of % followed by any other
 * character are left unchanged.  Only the first occurence of %c is
 * converted, all subsequence occurences are left unchanged.  This routine
 * mallocs the returned string. 
 */
#if 0
static	char	* convert_sub_format(char *format, char c, char *padded)
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	static	char	bletch[] = "%% ";
	char	*ptr = NULL;
	int	dont_got_it = 1;
	
	if (format == NULL)
		return (NULL);
	*buffer = (char) 0;
	memset(buffer, 0, sizeof(buffer));
	while (format)
	{
#if 0
/wset status_mode +#%+%
#endif
		if ((ptr = (char *) strchr(format, '%')) != NULL)
		{
			*ptr = (char) 0;
			strmcat(buffer, format, BIG_BUFFER_SIZE);
			*(ptr++) = '%';
			if ((*ptr == c)/* && dont_got_it*/)
			{
				dont_got_it = 0;
				if (*padded)
				{
					strmcat(buffer, "%", BIG_BUFFER_SIZE);
					strmcat(buffer, padded, BIG_BUFFER_SIZE);
					strmcat(buffer, "s", BIG_BUFFER_SIZE);
				}
				else
					strmcat(buffer, "%s", BIG_BUFFER_SIZE);
			}
			else if (*ptr == '<')
			{
				char *s = ptr + 1;
				while(*ptr && *ptr != '>') ptr++;				
				if (*ptr)
				{
					ptr++;
					if (!*ptr)
						continue;
					else if (*ptr == c)
					{

						strmcat(buffer, "%", BIG_BUFFER_SIZE);
						strmcat(buffer, s, ptr - s);
						strmcat(buffer, "s", BIG_BUFFER_SIZE);
					}
				}
			
			}
			else
			{
				bletch[2] = *ptr;
				strmcat(buffer, bletch, BIG_BUFFER_SIZE);
				if (!*ptr)
				{
					format = ptr;
					continue;
				}
			}
			ptr++;
		}
		else
			strmcat(buffer, format, BIG_BUFFER_SIZE);
		format = ptr;
	}
	malloc_strcpy(&ptr, buffer);
	return (ptr);
}
#else
char	*convert_sub_format (const char *format, char c)
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	int	pos = 0;
	int	dont_got_it = 1;

	if (!format)
		return NULL;		/* NULL in, NULL out */

	while (*format && pos < BIG_BUFFER_SIZE - 4)
	{
		if (*format != '%')
		{
			buffer[pos++] = *format++;
			continue;
		}

		format++;
		if (*format == c/* && dont_got_it*/)
		{
			dont_got_it = 0;
			buffer[pos++] = '%';
			buffer[pos++] = 's';
			format++;
		}
		else if (*format != '%')
		{
			buffer[pos++] = '%';
			buffer[pos++] = '%';
			buffer[pos++] = *format;
			format++;
		}
		else
		{
			buffer[pos++] = '%';
			buffer[pos++] = '%';
		}
	}

	buffer[pos] = 0;
	return m_strdup(buffer);
}


#endif

#if 0
static	char	*convert_format(Window *win, char *format, int k)
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	char	padded[BIG_BUFFER_SIZE + 1];
	int	pos = 0;
	int	*cp;
	int	map;
	char	key;
	int	i;

	cp = &win->func_cnt[k];
	while (*format && pos < BIG_BUFFER_SIZE - 4)
	{
		*padded = 0;
		if (*format != '%')
		{
			buffer[pos++] = *format++;
			continue;
		}

		/* Its a % */
		map = 0;

		/* Find the map, if neccesary */
		if (*++format == '{')
		{
			char	*endptr;

			format++;
			map = strtoul(format, &endptr, 10);
			if (*endptr != '}')
			{
				/* Unrecognized */
				continue;
			}
			format = endptr + 1;
		}
		else if (*format == '<')
		{
			char *p = padded;
			format++;
			while(*format && *format != '>') 
				*p++ = *format++;
			*p = 0;
			format++;
		}
		key = *format++;

		/* Choke once we get to the maximum number of expandos */
		if (*cp >= MAX_FUNCTIONS)
			continue;

		for (i = 0; i < NUMBER_OF_EXPANDOS; i++)
		{
			if (status_expandos[i].map != map ||
			    status_expandos[i].key != key)
				continue;

			if (status_expandos[i].format_var)
				new_free(status_expandos[i].format_var);
			if (status_expandos[i].format_set != -1)
				*(status_expandos[i].format_var) =
				     convert_sub_format(get_string_var(status_expandos[i].format_set), key, padded);
			buffer[pos++] = '%';
			buffer[pos++] = 's';

			win->status_func[k][(*cp)++] = 
				status_expandos[i].callback_function;
			break;
		}
	}

	win->func_cnt[k] = *cp;
	buffer[pos] = 0;
	return m_strdup(buffer);
}
#else

/*
 * This walks a raw format string and parses out any expandos that it finds.
 * An expando is handled by pre-fetching any string variable that is used
 * by the callback function, the callback function is registered, and a
 * %s format is put in the sprintf()-able return value (stored in buffer).
 * All other characters are copied as-is to the return value.
 */
void	build_status_format (Status *s, int k)
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	int	cp;
	int	map;
	char	key;
	int	i;
	Char	*raw = s->line[k].raw;
	char	*format = buffer;

	cp = s->line[k].count = 0;
	while (raw && *raw && (format - buffer < BIG_BUFFER_SIZE - 4))
	{
		if (*raw != '%')
		{
			*format++ = *raw++;
			continue;
		}

		/* It is a % */
		map = 0;

		/* Find the map, if neccesary */
		if (*++raw == '{')
		{
			char	*endptr;

			raw++;
			map = strtoul(raw, &endptr, 10);
			if (*endptr != '}')
			{
				/* Unrecognized */
				continue;
			}
			raw = endptr + 1;
		}

		key = *raw++;

		/* Choke once we get to the maximum number of expandos */
		if (cp >= MAX_FUNCTIONS)
			continue;

		for (i = 0; i < NUMBER_OF_EXPANDOS; i++)
		{
			if (status_expandos[i].map != map ||
			    status_expandos[i].key != key)
				continue;

			if (status_expandos[i].format_var)
				new_free(status_expandos[i].format_var);
			if (status_expandos[i].format_set != -1)
				*(status_expandos[i].format_var) = 
					convert_sub_format(get_string_var(status_expandos[i].format_set), key);

			*format++ = '%';
			*format++ = 's';

			s->line[k].func[cp] = 
				status_expandos[i].callback_function;
			s->line[k].map[cp] = map;
			s->line[k].key[cp] = key;
			cp++;
			break;
		}
	}

	s->line[k].count = cp;
	*format = 0;
	malloc_strcpy(&(s->line[k].format), buffer);
	while (cp < MAX_FUNCTIONS)
	{
		s->line[k].func[cp] = status_null_function;
		s->line[k].map[cp] = 0;
		s->line[k].key[cp] = 0;
		cp++;
	}
}


#endif

char    *alias_special_char (char **, char *, char *, char *, int *);

#if 0
void fix_status_buffer(Window *win, char *buffer, int in_status)
{
unsigned char	rhs_buffer[3*BIG_BUFFER_SIZE + 1];
unsigned char	lhs_buffer[3*BIG_BUFFER_SIZE + 1];
unsigned char	lhs_fillchar[6],
		rhs_fillchar[6],
		*fillchar = lhs_fillchar,
		*lhp = lhs_buffer,
		*rhp = rhs_buffer,
		*cp,
		*start_rhs = 0,
		*str = NULL, *ptr = NULL;
int		in_rhs = 0,
		pr_lhs = 0,
		pr_rhs = 0,
		*prc = &pr_lhs;

	lhs_buffer[0] = 0;
	rhs_buffer[0] = 0;
	if (!buffer || !*buffer)
		return;
	/*
	 * This converts away any ansi codes in the status line
	 * in accordance with the current settings.  This leaves us
	 * with nothing but logical characters, which are then easy
	 * to count. :-)
	 */
	str = strip_ansi(buffer);

	/*
	 * Count out the characters.
	 * Walk the entire string, looking for nonprintable
	 * characters.  We count all the printable characters
	 * on both sides of the %> tag.
	 */
	ptr = str;
	cp = lhp;
	lhs_buffer[0] = rhs_buffer[0] = 0;
	while (*ptr)
	{
		/*
		 * The FIRST such %> tag is used.
		 * Using multiple %>s is bogus.
		 */
		if (*ptr == '\f' && start_rhs == NULL)
		{
			ptr++;
			start_rhs = ptr;
			fillchar = rhs_fillchar;
			in_rhs = 1;
			*cp = 0;
			cp = rhp;
			prc = &pr_rhs;
		}
		/*
		 * Skip over color attributes if we're not
		 * doing color.
		 */
		else if (*ptr == '\003')
		{
			const u_char *end = skip_ctl_c_seq(ptr, NULL, NULL, 0);
			while (ptr < end)
				*cp++ = *ptr++;
		}
		/*
		 * If we have a ROM character code here, we need to
		 * copy it to the output buffer, as well as the fill
		 * character, and increment the printable counter by
		 * only 1.
		 */
		else if (*ptr == ROM_CHAR)
		{
			fillchar[0] = *cp++ = *ptr++;
			fillchar[1] = *cp++ = *ptr++;
			fillchar[2] = *cp++ = *ptr++;
			fillchar[3] = *cp++ = *ptr++;
			fillchar[4] = 0;
			*prc += 1;
		}
		/*
		 * Is it NOT a printable character?
		 */
		else if ((*ptr == REV_TOG) || (*ptr == UND_TOG) ||
			 (*ptr == ALL_OFF) || (*ptr == BOLD_TOG) ||
			 (*ptr == BLINK_TOG))
				*cp++ = *ptr++;
		/*
		 * So it is a printable character.
		 * Or maybe its a tab. ;-)
		 */
		else
		{
			*prc += 1;
			fillchar[0] = *cp++ = *ptr++;
			fillchar[1] = 0;
		}
		/*
		 * Dont allow more than CO printable characters
		 */
		if (pr_lhs + pr_rhs >= win->screen->co)
		{
			*cp = 0;
			break;
		}
	}
	*cp = 0;
	strcpy(buffer, lhs_buffer);
	strmcat(buffer, rhs_buffer, BIG_BUFFER_SIZE);
	new_free(&str);
}

void build_status(Window *win, char *format, int use_format)
{
	int	i, k;
	char	*f = NULL;
	if (!win)
		return;
	for (k = 0; k < 2; k++) 
	{
		new_free(&win->status_format[k]);
		win->func_cnt[k] = 0;
		if (use_format)
			f = format;
		else
		{
			switch(k)
			{
				case 0:
					f = get_string_var(STATUS_FORMAT1_VAR);
					break;
				case 1:
					f = get_string_var(STATUS_FORMAT2_VAR);
					break;
			}			
		}
		if (f)
			win->status_format[k] = convert_format(win, f, k);

		for (i = win->func_cnt[k]; i < MAX_FUNCTIONS; i++)
			win->status_func[k][i] = status_null_function;
		if (use_format)
			break;
	}
	update_all_status(win, NULL, 0);
}

void make_status(Window *win)
{
	u_char	buffer	    [BIG_BUFFER_SIZE + 1];
	u_char	lhs_buffer  [BIG_BUFFER_SIZE + 1];
	u_char	rhs_buffer  [BIG_BUFFER_SIZE + 1];
	char	*func_value[MAX_FUNCTIONS+10] = {NULL};
	u_char	*ptr;
	
	int	len = 1,
		status_line;

	/* The second status line is only displayed in the bottom window
	 * and should always be displayed, no matter what SHOW_STATUS_ALL
	 * is set to - krys
	 */
	for (status_line = 0 ; status_line < 1+win->double_status + win->status_lines; status_line++)
	{
		u_char	lhs_fillchar[6],
			rhs_fillchar[6],
			*fillchar = lhs_fillchar,
			*lhp = lhs_buffer,
			*rhp = rhs_buffer,
			*cp,
			*start_rhs = 0,
			*str;
		int	in_rhs = 0,
			pr_lhs = 0,
			pr_rhs = 0,
			line = status_line,
			*prc = &pr_lhs, 
			i;

		fillchar[0] = fillchar[1] = 0;

		if (!win->status_format[line])
			continue;
			
		for (i = 0; i < MAX_FUNCTIONS; i++)
			func_value[i] = (win->status_func[line][i]) (win);
		len = 1;
		
		if (get_int_var(REVERSE_STATUS_VAR))
			buffer[0] = get_int_var(REVERSE_STATUS_VAR) ? REV_TOG : '\0';
		else
			len = 0;
		str = &buffer[len];
		snprintf(str, BIG_BUFFER_SIZE - 1, 
			win->status_format[line],
			func_value[0], func_value[1], func_value[2],
			func_value[3], func_value[4], func_value[5],
			func_value[6], func_value[7], func_value[8],
			func_value[9], func_value[10], func_value[11],
			func_value[12], func_value[13], func_value[14],
			func_value[15], func_value[16], func_value[17],
			func_value[18], func_value[19], func_value[20],
			func_value[21], func_value[22], func_value[23],
			func_value[24], func_value[25], func_value[26],
			func_value[27], func_value[28], func_value[29],
			func_value[30], func_value[31],func_value[32],
			func_value[33], func_value[34],func_value[35],
			func_value[36], func_value[37],func_value[38]);

			/*  Patched 26-Mar-93 by Aiken
			 *  make_window now right-justifies everything 
			 *  after a %>
			 *  it's also more efficient.
			 */

		/*
		 * This converts away any ansi codes in the status line
		 * in accordance with the current settings.  This leaves us
		 * with nothing but logical characters, which are then easy
		 * to count. :-)
		 */
		str = strip_ansi(buffer);

		/*
		 * Count out the characters.
		 * Walk the entire string, looking for nonprintable
		 * characters.  We count all the printable characters
		 * on both sides of the %> tag.
		 */
		ptr = str;
		cp = lhp;
		lhs_buffer[0] = rhs_buffer[0] = 0;

		while (*ptr)
		{
			/*
			 * The FIRST such %> tag is used.
			 * Using multiple %>s is bogus.
			 */
			if (*ptr == '\f' && start_rhs == NULL)
			{
				ptr++;
				start_rhs = ptr;
				fillchar = rhs_fillchar;
				in_rhs = 1;
				*cp = 0;
				cp = rhp;
				prc = &pr_rhs;
			}

			/*
			 * Skip over color attributes if we're not
			 * doing color.
			 */
			else if (*ptr == '\003')
			{
				const u_char *end = skip_ctl_c_seq(ptr, NULL, NULL, 0);
				while (ptr < end)
					*cp++ = *ptr++;
			}

			/*
			 * If we have a ROM character code here, we need to
			 * copy it to the output buffer, as well as the fill
			 * character, and increment the printable counter by
			 * only 1.
			 */
			else if (*ptr == ROM_CHAR)
			{
				fillchar[0] = *cp++ = *ptr++;
				fillchar[1] = *cp++ = *ptr++;
				fillchar[2] = *cp++ = *ptr++;
				fillchar[3] = *cp++ = *ptr++;
				fillchar[4] = 0;
				*prc += 1;
			}

			/*
			 * Is it NOT a printable character?
			 */
			else if ((*ptr == REV_TOG) || (*ptr == UND_TOG) ||
				 (*ptr == ALL_OFF) || (*ptr == BOLD_TOG) ||
				 (*ptr == BLINK_TOG))
					*cp++ = *ptr++;
			/*
			 * So it is a printable character.
			 * Or maybe its a tab. ;-)
			 */
			else
			{
				*prc += 1;
				fillchar[0] = *cp++ = *ptr++;
				fillchar[1] = 0;
			}

			/*
			 * Dont allow more than CO printable characters
			 */
			if (pr_lhs + pr_rhs >= win->screen->co)
			{
				*cp = 0;
				break;
			}
		}
		*cp = 0;

		/* What will we be filling with? */
		if (get_int_var(STATUS_NO_REPEAT_VAR))
		{
			lhs_fillchar[0] = ' ';
			lhs_fillchar[1] = 0;
			rhs_fillchar[0] = ' ';
			rhs_fillchar[1] = 0;
		}

		/*
		 * Now if we have a rhs, then we have to adjust it.
		 */
		if (start_rhs)
		{
			int numf = 0;

			numf = win->screen->co - pr_lhs - pr_rhs  -1;
			while (numf-- >= 0)
				strmcat(lhs_buffer, lhs_fillchar, 
						BIG_BUFFER_SIZE);
		}

		/*
		 * No rhs?  If the user wants us to pad it out, do so.
		 */
		else if (get_int_var(FULL_STATUS_LINE_VAR))
		{
			int chars = win->screen->co - pr_lhs - 1;

			while (chars-- >= 0)
				strmcat(lhs_buffer, lhs_fillchar, 
						BIG_BUFFER_SIZE);
		}

		strcpy(buffer, lhs_buffer);
		strmcat(buffer, rhs_buffer, BIG_BUFFER_SIZE);
		strmcat(buffer, ALL_OFF_STR, BIG_BUFFER_SIZE);
		new_free(&str);

		if (!win->status_line[status_line] ||
			strcmp(buffer, win->status_line[status_line]))

		{
			malloc_strcpy(&win->status_line[status_line], buffer);
			output_screen = win->screen;

			if (win->status_lines && (line == win->double_status+win->status_lines) && win->status_split)
				term_move_cursor(0,win->top);
			else if (win->status_lines && !win->status_split)
				term_move_cursor(0,win->bottom+status_line-win->status_lines);
			else
				term_move_cursor(0,win->bottom+status_line);

			output_line(buffer);
#if 0
#if defined(SCO) || defined(HPUX) || defined(SOLARIS)
			term_set_foreground(7);    term_set_background(0);
#endif
#endif
			cursor_in_display(win);
			term_all_off();
		} 
	}
	cursor_to_input();
}
#else

/*
 * This function does two things:
 * 1) Rebuilds the status_func[] tables for each of the three possible
 *    status_format's (single, lower double, upper double)
 * 2) Causes the status bars to be redrawn immediately.
 */
void	rebuild_a_status (Window *w)
{
	int 	i,
		k;
	Status	*s;

	if (w)
		s = &w->status;
	else
		s = &main_status;

	for (k = 0; k < 3; k++)
	{
		new_free((char **)&s->line[k].format);
		s->line[k].count = 0;

		/*
		 * If we have an overriding status_format, then we parse
		 * that out.
		 */
		if (w && s->line[k].raw)
			build_status_format(s, k);

		/*
		 * Otherwise, If this is for a window, just copy the essential
		 * information over from the main status lines.
		 */
		else if (w)
		{
			s->line[k].format = m_strdup(main_status.line[k].format);
			for (i = 0; i < MAX_FUNCTIONS; i++)
			{
			    s->line[k].func[i] = main_status.line[k].func[i];
			    s->line[k].map[i] = main_status.line[k].map[i];
			    s->line[k].key[i] = main_status.line[k].key[i];
			}
			s->line[k].count = main_status.line[k].count;
		}

		/*
		 * Otherwise, this *is* the main status lines we are generating
		 * and we need to do all the normal shenanigans.
		 */
		else
		{
			if (k == 0) /* XXX should be STATUS_FORMAT_VAR */
			   s->line[k].raw = get_string_var(STATUS_FORMAT1_VAR);
			else if (k == 1)
			   s->line[k].raw = get_string_var(STATUS_FORMAT1_VAR);
			else  /* (k == 2) */
			   s->line[k].raw = get_string_var(STATUS_FORMAT2_VAR);

			build_status_format(s, k);
		}
	}
}

void	init_status	(void)
{
	int	i, k;

	main_status.double_status = 0;
	main_status.special = 0;
	for (i = 0; i < 3; i++)
	{
		main_status.line[i].raw = NULL;
		main_status.line[i].format = NULL;
		main_status.line[i].count = 0;
		main_status.line[i].result = NULL;
		for (k = 0; k < MAX_FUNCTIONS; k++)
		{
			main_status.line[i].func[k] = NULL;
			main_status.line[i].map[k] = 0;
			main_status.line[i].key[k] = 0;
		}
	}
	main_status_init = 1;
}

void	build_status	(Window *win, char *unused, int unused1)
{
	Window 	*w = NULL;

	if (!unused && !main_status_init)
		init_status();

	rebuild_a_status(w);
	while (traverse_all_windows(&w))
		rebuild_a_status(w);

	update_all_status(win, NULL, 0);
}


/*
 * This just sucked beyond words.  I was always planning on rewriting this,
 * but the crecendo of complaints with regards to this just got to be too 
 * irritating, so i fixed it early.
 */
void	make_status (Window *window)
{
	int	status_line;
	u_char	buffer	    [BIG_BUFFER_SIZE + 1];
	u_char	lhs_buffer  [BIG_BUFFER_SIZE + 1];
	u_char	rhs_buffer  [BIG_BUFFER_SIZE + 1];
	Char	*func_value [MAX_FUNCTIONS];
	u_char	*ptr;

	for (status_line = 0; status_line < window->status.double_status + 1 + window->status.status_lines; status_line++)
	{
		u_char	lhs_fillchar[6],
			rhs_fillchar[6],
			*fillchar = lhs_fillchar,
			*lhp = lhs_buffer,
			*rhp = rhs_buffer,
			*cp,
			*start_rhs = 0,
			*str;
		int	in_rhs = 0,
			pr_lhs = 0,
			pr_rhs = 0,
			line,
			*prc = &pr_lhs, 
			i;

		fillchar[0] = fillchar[1] = 0;

		/*
		 * If status line gets to one, then that means that
		 * window->double_status is not zero.  That means that
		 * the status line we're working on is STATUS2.
		 */
		if (status_line)
			line = 2;

		/*
		 * If status_line is zero, and window->double_status is
		 * not zero (double status line is on) then we're working
		 * on STATUS1.
		 */
		else if (window->status.double_status)
			line = 1;

		/*
		 * So status_line is zero and window->double_status is zero.
		 * So we're working on STATUS (0).
		 */
		else
			line = 0;


		/*
		 * Sanity check:  If the status format doesnt exist, dont do
		 * anything for it.
		 */
		if (!window->status.line[line].format)
			continue;

		/*
		 * Run each of the status-generating functions from the the
		 * status list.  Note that the retval of the functions is no
		 * longer malloc()ed.  This saves 40-some odd malloc/free sets
		 * each time the status bar is updated, which is non-trivial.
		 */
		for (i = 0; i < MAX_FUNCTIONS; i++)
		{
			if (window->status.line[line].func[i] == NULL)
				nappanic("status callback null.  Window [%d], line [%d], function [%d]", window->refnum, line, i);
			func_value[i] = window->status.line[line].func[i]
				(window, window->status.line[line].map[i],
				 window->status.line[line].key[i]);
		}

		/*
		 * If the REVERSE_STATUS_LINE var is on, then put a reverse
		 * character in the first position (itll get translated to
		 * the tcap code in the output code.
		 */
		if (get_int_var(REVERSE_STATUS_VAR))
			*buffer = REV_TOG , str = buffer + 1;
		else
			str = buffer;

		/*
		 * Now press the status line into "buffer".  The magic about
		 * setting status_format is handled elsewhere.
		 */
		snprintf(str, BIG_BUFFER_SIZE - 1, window->status.line[line].format,
		    func_value[0], func_value[1], func_value[2], 
		    func_value[3], func_value[4], func_value[5],
		    func_value[6], func_value[7], func_value[8], func_value[9],
		    func_value[10], func_value[11], func_value[12],
		    func_value[13], func_value[14], func_value[15],
		    func_value[16], func_value[17], func_value[18],
		    func_value[19], func_value[20], func_value[21],
		    func_value[22], func_value[23], func_value[24],
		    func_value[25], func_value[26], func_value[27],
		    func_value[28], func_value[29], func_value[30],
		    func_value[31], func_value[32], func_value[33],
		    func_value[34], func_value[35], func_value[36], 
		    func_value[37], func_value[38], func_value[39]); 

		/*
		 * If the user wants us to, pass the status bar through the
		 * expander to pick any variables/function calls.
		 * This is horribly expensive, but what do i care if you
		 * want to waste cpu cycles? ;-)
		 */
#if 0
		if (get_int_var(STATUS_DOES_EXPANDOS_VAR))
		{
			int  af = 0;
			Window *old = current_window;

			current_window = window;
			str = expand_alias(buffer, empty_string, &af, NULL);
			current_window = old;
			strmcpy(buffer, str, BIG_BUFFER_SIZE);
			new_free(&str);
		}
#endif

		/*
		 * This converts away any ansi codes in the status line
		 * in accordance with the currenet settings.  This leaves us
		 * with nothing but logical characters, which are then easy
		 * to count. :-)
		 */
		str = strip_ansi(buffer);

		/*
		 * Count out the characters.
		 * Walk the entire string, looking for nonprintable
		 * characters.  We count all the printable characters
		 * on both sides of the %> tag.
		 */
		ptr = str;
		cp = lhp;
		lhs_buffer[0] = rhs_buffer[0] = 0;

		while (*ptr)
		{
			/*
			 * The FIRST such %> tag is used.
			 * Using multiple %>s is bogus.
			 */
			if (*ptr == '\f' && start_rhs == NULL)
			{
				ptr++;
				start_rhs = ptr;
				fillchar = rhs_fillchar;
				in_rhs = 1;
				*cp = 0;
				cp = rhp;
				prc = &pr_rhs;
			}

			/*
			 * Skip over color attributes if we're not
			 * doing color.
			 */
			else if (*ptr == '\003')
			{
				const u_char *end = skip_ctl_c_seq(ptr, NULL, NULL, 0);
				while (ptr < end)
					*cp++ = *ptr++;
			}

			/*
			 * If we have a ROM character code here, we need to
			 * copy it to the output buffer, as well as the fill
			 * character, and increment the printable counter by
			 * only 1.
			 */
			else if (*ptr == ROM_CHAR)
			{
				fillchar[0] = *cp++ = *ptr++;
				fillchar[1] = *cp++ = *ptr++;
				fillchar[2] = *cp++ = *ptr++;
				fillchar[3] = *cp++ = *ptr++;
				fillchar[4] = 0;
				*prc += 1;
			}

			/*
			 * Is it NOT a printable character?
			 */
			else if ((*ptr == REV_TOG) || (*ptr == UND_TOG) ||
				 (*ptr == ALL_OFF) || (*ptr == BOLD_TOG) ||
				 (*ptr == BLINK_TOG))
					*cp++ = *ptr++;

			/*
			 * XXXXX This is a bletcherous hack.
			 * If i knew what was good for me id not do this.
			 */
			else if (*ptr == 9)	/* TAB */
			{
				fillchar[0] = ' ';
				fillchar[1] = 0;
				do
					*cp++ = ' ';
				while (++(*prc) % 8);
				ptr++;
			}

			/*
			 * So it is a printable character.
			 */
			else
			{
				*prc += 1;
				fillchar[0] = *cp++ = *ptr++;
				fillchar[1] = 0;
			}

			/*
			 * Dont allow more than CO printable characters
			 */
			if (pr_lhs + pr_rhs >= window->screen->co)
			{
				*cp = 0;
				break;
			}
		}
		*cp = 0;

		/* What will we be filling with? */
		if (get_int_var(STATUS_NO_REPEAT_VAR))
		{
			lhs_fillchar[0] = ' ';
			lhs_fillchar[1] = 0;
			rhs_fillchar[0] = ' ';
			rhs_fillchar[1] = 0;
		}

		/*
		 * Now if we have a rhs, then we have to adjust it.
		 */
		if (start_rhs)
		{
			int numf = 0;

			numf = window->screen->co - pr_lhs - pr_rhs -1;
			while (numf-- >= 0)
				strmcat(lhs_buffer, lhs_fillchar, 
						BIG_BUFFER_SIZE);
		}

		/*
		 * No rhs?  If the user wants us to pad it out, do so.
		 */
		else if (get_int_var(FULL_STATUS_LINE_VAR))
		{
			int chars = window->screen->co - pr_lhs - 1;

			while (chars-- >= 0)
				strmcat(lhs_buffer, lhs_fillchar, 
						BIG_BUFFER_SIZE);
		}

		strlcpy(buffer, lhs_buffer, BIG_BUFFER_SIZE);
		strlcat(buffer, rhs_buffer, BIG_BUFFER_SIZE);
		strlcat(buffer, ALL_OFF_STR, BIG_BUFFER_SIZE);
		new_free(&str);

		/*
		 * Update the status line on the screen.
		 * First check to see if it has changed
		 */
		if (!window->status.line[status_line].result ||
			strcmp(buffer, window->status.line[status_line].result))
		{
			/*
			 * Roll the new back onto the old
			 */
			malloc_strcpy(&window->status.line[status_line].result,
					buffer);

			/*
			 * Output the status line to the screen
			 */
			output_screen = window->screen;
			if (window->status.status_lines && (line == 2) && window->status.status_split)
				term_move_cursor(0,window->top);
			else if (window->status.status_lines && !window->status.status_split)
				term_move_cursor(0,window->bottom+status_line-window->status.status_lines);
			else
				term_move_cursor(0,window->bottom+status_line);
#if 0
			term_move_cursor(0, window->bottom + status_line);
#endif
			output_line(buffer);
			cursor_in_display(window);
			term_all_off();
		}
	}

	cursor_to_input();
}

#endif



/* Some useful macros */
/*
 * This is used to get the current window on a window's screen
 */
#define CURRENT_WINDOW window->screen->current_window

/*
 * This tests to see if the window IS the current window on its screen
 */
#define IS_CURRENT_WINDOW (window->screen->current_window == window)

/*
 * This tests to see if all expandoes are to appear in all status bars
 */
#define SHOW_ALL_WINDOWS (get_int_var(SHOW_STATUS_ALL_VAR))

/*
 * "Current-type" window expandoes occur only on the current window for a 
 * screen.  However, if /set show_status_all is on, then ALL windows act as
 * "Current-type" windows.
 */
#define DISPLAY_ON_WINDOW (IS_CURRENT_WINDOW || SHOW_ALL_WINDOWS)

#define RETURN_EMPTY  return empty_string


STATUS_FUNCTION(status_nickname)
{
static char my_buffer[MY_BUFFER/2+1];
	snprintf(my_buffer, MY_BUFFER/2, nick_format, 
		get_server_nickname(window->server) ? 
		get_server_nickname(window->server) : 
		get_string_var(DEFAULT_NICKNAME_VAR));
	return my_buffer;
}

/*
 * This displays the server that the window is connected to.
 */
STATUS_FUNCTION(status_server)
{
	char	*rest;
const	char	*n;
	char	*name;
	char	*next;
static	char	my_buffer[64];
	size_t	len;

#ifdef OLD_STATUS_S_EXPANDO_BEHAVIOR
	/*
	 * If there is only one server, dont bother telling the user
	 * what it is.
	 */
	if (connected_to_server == 1 && map == 0)
		return empty_string;
#endif

	/*
	 * If this window isnt connected to a server, say so.
	 */
	if (window->server == -1)
		return "No Server";

	/*
	 * If the user doesnt want this expando, dont force it.
	 */
	if (!server_format)
		return empty_string;

	/* Figure out what server this window is on */
	n = get_server_name(window->server);
	if (map == 2)
	{
		snprintf(my_buffer, 63, server_format, n);
		return my_buffer;
	}

	name = LOCAL_COPY(n);

	/*
	 * If the first segment before the first dot is a number,
	 * then its an ip address, and use the whole thing.
	 */
	if (strtoul(name, &next, 10) && *next == '.')
	{
		snprintf(my_buffer, 63, server_format, name);
		return my_buffer;
	}

	/*
	 * Get the stuff to the left of the first dot.
	 */
	if (!(rest = strchr(name, '.')))
	{
		snprintf(my_buffer, 63, server_format, name);
		return my_buffer;
	}

	/*
	 * If the first segment is 'irc', thats not terribly
	 * helpful, so get the next segment.
	 */
	if (!strncmp(name, "irc", 3) || !strncmp(name, "opennap", 7))
	{
		name = rest + 1;
		if (!(rest = strchr(name + 1, '.')))
			rest = name + strlen(name);
	}

	/*
	 * If the name of the server is > 60 chars, crop it back to 60.
	 */
	if ((len = rest - name) > 60)
		len = 60;

	/*
	 * Plop the server into the server_format and return it.
	 */
	name[len] = 0;
	snprintf(my_buffer, 63, server_format, name);
	return my_buffer;
}

#if 0
STATUS_FUNCTION(status_server)
{
	char	*name;
static	char	my_buffer[MY_BUFFER+1];
	if (window->server != -1)
	{
		if (server_format)
		{
			if (!(name = get_server_itsname(window->server)))
				name = get_server_name(window->server);
			snprintf(my_buffer, MY_BUFFER, server_format, name);
			return my_buffer;
		}
		else
			RETURN_EMPTY;
	}
	RETURN_EMPTY;
}
#endif

STATUS_FUNCTION(status_query_nick)
{
static	char my_buffer[BIG_BUFFER_SIZE+1];

	if (window->query_nick && query_format)
	{
		snprintf(my_buffer, BIG_BUFFER_SIZE, query_format, window->query_nick);
		return my_buffer;
	}
	else
		RETURN_EMPTY;
}

STATUS_FUNCTION(status_right_justify)
{
static	char	my_buffer[] = "\f";

	return my_buffer;
}

STATUS_FUNCTION(status_notify_windows)
{
	int	doneone = 0;
	char	buf2[MY_BUFFER+2];
static	char	my_buffer[MY_BUFFER/2+1];
	if (get_int_var(SHOW_STATUS_ALL_VAR) || window == window->screen->current_window)
	{
		*buf2='\0';
		window = NULL;
		while ((traverse_all_windows(&window)))
		{
			if (window->miscflags & WINDOW_NOTIFIED)
			{
				if (doneone++)
					strmcat(buf2, ",", MY_BUFFER/2);
				strmcat(buf2, (map == 1 && window->name) ?
					window->name : ltoa(window->refnum), 
					MY_BUFFER/2);
			}
		}
	}
	if (doneone && notify_format)
	{
		snprintf(my_buffer, MY_BUFFER/2, notify_format, buf2);
		return (my_buffer);
	}
	RETURN_EMPTY;
}

STATUS_FUNCTION(status_clock)
{
static	char	my_buf[MY_BUFFER+1];

	if ((get_int_var(CLOCK_VAR) && clock_format)  &&
	    (get_int_var(SHOW_STATUS_ALL_VAR) ||
	    (window == window->screen->current_window)))
		snprintf(my_buf, MY_BUFFER, clock_format, update_clock(GET_TIME));
	else
		RETURN_EMPTY;
	return my_buf;
}

STATUS_FUNCTION(status_channel)
{
	char	channel[IRCD_BUFFER_SIZE + 1];
static	char	my_buffer[IRCD_BUFFER_SIZE + 1];

	if (window->current_channel)
	{
		int num;
		strmcpy(channel, window->current_channel, IRCD_BUFFER_SIZE);

		if ((num = get_int_var(CHANNEL_NAME_WIDTH_VAR)) &&
		    ((int) strlen(channel) > num))
			channel[num] = (char) 0;
		snprintf(my_buffer, IRCD_BUFFER_SIZE, channel_format, channel);
		return my_buffer;
	}
	RETURN_EMPTY;
}

STATUS_FUNCTION(status_insert_mode)
{
char	*text;

	if (get_int_var(INSERT_MODE_VAR) && (get_int_var(SHOW_STATUS_ALL_VAR)
	    || (window->screen->current_window == window)))
		if ((text = get_string_var(STATUS_INSERT_VAR)))
			return text;
	RETURN_EMPTY;
}

STATUS_FUNCTION(status_overwrite_mode)
{
char	*text;

	if (!get_int_var(INSERT_MODE_VAR) && (get_int_var(SHOW_STATUS_ALL_VAR)
	    || (window->screen->current_window == window)))
	{
	    if ((text = get_string_var(STATUS_OVERWRITE_VAR)))
		return text;
	}
	RETURN_EMPTY;
}


STATUS_FUNCTION(status_hold)
{
char	*text;

	if (window->holding_something && (text = get_string_var(STATUS_HOLD_VAR)))
		return(text);
	RETURN_EMPTY;
}

STATUS_FUNCTION(status_window)
{
char	*text;
	if ((number_of_windows_on_screen(window) > 1) && (window->screen->current_window == window) &&
	    (text = get_string_var(STATUS_WINDOW_VAR)))
		return(text);
	RETURN_EMPTY;
}

STATUS_FUNCTION(status_refnum)
{
static char my_buffer[MY_BUFFER/3+1];
	strmcpy(my_buffer, window->name ? window->name : ltoa(window->refnum), MY_BUFFER/3);
	return (my_buffer);
}

STATUS_FUNCTION(status_version)
{
	if (!get_int_var(SHOW_STATUS_ALL_VAR) && (window->screen->current_window != window))
		return(empty_string);
	return ((char *)nap_version);
}

STATUS_FUNCTION(status_cpu_saver_mode)
{
static char my_buffer[MY_BUFFER/2+1];
	if (cpu_saver && cpu_saver_format)
	{
		snprintf(my_buffer, MY_BUFFER/2, cpu_saver_format, "cpu");
		return my_buffer;
	}

	RETURN_EMPTY;
}


STATUS_FUNCTION(status_null_function)
{
	RETURN_EMPTY;
}


STATUS_FUNCTION(status_position)
{
static char my_buffer[MY_BUFFER/2+1];

	snprintf(my_buffer, MY_BUFFER/2, "(%d-%d)", window->lines_scrolled_back,
					window->distance_from_display);
	return my_buffer;
}
 
STATUS_FUNCTION(status_scrollback)
{
char *stuff;
	if (window->scrollback_point &&
	    (stuff = get_string_var(STATUS_SCROLLBACK_VAR)))
		return stuff;
	else
		RETURN_EMPTY;
}

STATUS_FUNCTION(status_percent)
{
	static	char	percent[] = "%";
	return	percent;
}

STATUS_FUNCTION(status_server_stats)
{
static char my_buffer[MY_BUFFER+1];
N_STATS *stats;
	if ((stats = get_server_stats(window->server)) && stats_format)
	{
		int c;
		char gigs[20], songs[20], shares[20];
		c = charcount(stats_format, '%');
		*my_buffer = 0;
		strcpy(gigs, ltoa(stats->gigs));
		strcpy(songs, ltoa(stats->songs));
		strcpy(shares, ltoa(stats->libraries));
		switch(c)
		{
			case 0:
				snprintf(my_buffer, MY_BUFFER, stats_format);
				break;
			case 1:
				snprintf(my_buffer, MY_BUFFER, stats_format, shares);
				break;
			case 2:
				snprintf(my_buffer, MY_BUFFER, stats_format, shares, songs);
				break;
			case 3:
				snprintf(my_buffer, MY_BUFFER, stats_format, shares, songs, gigs);
				break;
		}
		return my_buffer;
	}
	RETURN_EMPTY;
}

STATUS_FUNCTION(napster_shared)
{
static char my_buffer[MY_BUFFER+1];
	if (!shared_stats.shared_files)
		RETURN_EMPTY;
	snprintf(my_buffer, MY_BUFFER, "[Sh:%lu/%3.2f%s] ", 
		shared_stats.shared_files, _GMKv(shared_stats.shared_filesize), 
		_GMKs(shared_stats.shared_filesize));
	return my_buffer;
}

STATUS_FUNCTION(napster_updown)
{
static char my_buffer[MY_BUFFER+1];
int upload = 0, download = 0, queued = 0;
GetFile *sg;
NickStruct *n;
extern Server *server_list;
	if (!transfer_struct || window->server < 0)
		RETURN_EMPTY;
	for (sg = transfer_struct; sg; sg = sg->next)
		if ((sg->flags & NAP_DOWNLOAD) == NAP_DOWNLOAD)
			download++;
		else if ((sg->flags & NAP_UPLOAD) == NAP_UPLOAD)
			upload++;
	for (n = server_list[window->server].users; n; n = n->next)
		for (sg = n->Queued; sg; sg = sg->next)
			queued++;
	snprintf(my_buffer, MY_BUFFER, " [U:%d/D:%d", upload, download);
	if (queued)
		snprintf(my_buffer+strlen(my_buffer), MY_BUFFER, "/Q:%d", queued);
	strcat(my_buffer, "]");
	return my_buffer;
}

STATUS_FUNCTION(status_eta)
{
static char buff[MY_BUFFER+1];
GetFile *tmp;
	*buff = 0;
	for (tmp = transfer_struct; tmp; tmp = tmp->next)
	{
		if (!tmp->filesize)
			continue;
		if (*buff)
			strlcat(buff, ",", MY_BUFFER);
		strlcat(buff, calc_eta(tmp), MY_BUFFER);
		
	}
	return buff;
}

STATUS_FUNCTION(napster_download)
{
int upload = 0;
int download = 0;
GetFile *tmp;
static char buff[MY_BUFFER+1];
char upbuffer[BIG_BUFFER_SIZE+1];
char dnbuffer[BIG_BUFFER_SIZE];
char tmpbuff[80];
double perc = 0.0;
	*upbuffer = 0;
	*dnbuffer = 0;
	for (tmp = transfer_struct; tmp; tmp = tmp->next)
	{
		if (!tmp->filesize)
			continue;
                perc = (100.0 * (((double)(tmp->received + tmp->resume)) / (double)tmp->filesize));
		sprintf(tmpbuff, "%4.1f%%", perc);
		if ((tmp->flags & NAP_DOWNLOAD) == NAP_DOWNLOAD)
		{
			if (download)
				strlcat(dnbuffer, ",", BIG_BUFFER_SIZE);
			else
				strlcat(dnbuffer, " [G:", BIG_BUFFER_SIZE);
			strlcat(dnbuffer, tmpbuff, BIG_BUFFER_SIZE);
			download++;
		}
		else if ((tmp->flags & NAP_UPLOAD) == NAP_UPLOAD)
		{
			if (upload)
				strlcat(upbuffer, ",", BIG_BUFFER_SIZE);
			else
				strlcat(upbuffer, " [S:", BIG_BUFFER_SIZE);
			strlcat(upbuffer, tmpbuff, BIG_BUFFER_SIZE);
			upload++;
		}
	}
	if (download)
		strlcat(dnbuffer, "]", BIG_BUFFER_SIZE);
	if (upload)
		strlcat(upbuffer, "]", BIG_BUFFER_SIZE);
	strlcpy(buff, dnbuffer, MY_BUFFER-1);
	strlcat(buff, upbuffer, MY_BUFFER-1);
	return buff;
}


STATUS_FUNCTION(napster_load_share)
{
#ifdef WANT_THREAD
static char my_buffer[MY_BUFFER];
extern int in_load;
	if (in_load)
	{
#if !defined(WINNT)
		if (pthread_mutex_trylock(&shared_count_mutex) == EBUSY)
			RETURN_EMPTY;
#endif
		if (shared_count)
		{
			sprintf(my_buffer, "%lu", shared_count);
#if !defined(WINNT)
			pthread_mutex_unlock(&shared_count_mutex);
#endif
			return my_buffer;
		}
#if !defined(WINNT)
		pthread_mutex_unlock(&shared_count_mutex);
#endif
	}
#endif
	RETURN_EMPTY;
}

STATUS_FUNCTION(status_usercount)
{
ChannelStruct *ch;
NickStruct *n;
int count = 0;
static char buff[30];

	if (!window || !window->current_channel)
		RETURN_EMPTY;
	if (!(ch = (ChannelStruct *)find_in_list((List **)&window->nchannels, window->current_channel, 0)))
		RETURN_EMPTY;
	for (n = ch->nicks; n; n = n->next)
		count++;
	sprintf(buff, "%u", count);
	return buff;
}

STATUS_FUNCTION(status_cloak)
{
static char buff[] = "*";
	if (window->server > -1)
		if (get_server_cloak(window->server))
			return buff;
	RETURN_EMPTY;
}

STATUS_FUNCTION(status_lag)
{
static  char	my_buffer[MY_BUFFER/2+1];
	if (get_server_admin(window->server) > USER_USER)
	{
		struct timeval td;		
		td = get_server_lag(window->server);
		snprintf(my_buffer, MY_BUFFER/2-1, "%1.3f", ((double)td.tv_sec + ((double)td.tv_usec / 1000000.0))/2);
		return(my_buffer);
	}
	RETURN_EMPTY;
}

STATUS_FUNCTION(status_topic)
{
static char	my_buffer[MY_BUFFER+1];
	if (window->current_channel)
	{
		ChannelStruct *ch;
		if ((ch = (ChannelStruct *)find_in_list((List **)&window->nchannels, window->current_channel, 0)))
		{
			strncpy(my_buffer, ch->topic, MY_BUFFER);
			return my_buffer;
		}
	}
	RETURN_EMPTY;
}

STATUS_FUNCTION(status_sharedir)
{
static char 	my_buffer[MY_BUFFER+1];
char *p;
extern char *return_current_share_dir();
	p = return_current_share_dir();
	if (*p)
		strncpy(my_buffer, p, MY_BUFFER);
	else
		*my_buffer = 0;
	return my_buffer;
}

STATUS_FUNCTION(status_windowspec)
{
static char my_buffer[81];
	if (window->status.special)
		strmcpy(my_buffer, window->status.special, 80);
	else
		*my_buffer = 0;

	return my_buffer;
}

STATUS_FUNCTION(status_user)
{
	char *	text;
	int	i;

	/* XXX Ick.  Oh well. */
	struct dummystruct {
		int		map;
		char		key;
		enum VAR_TYPES	var;
	} lookup[] = {
	{ 0, 'U', STATUS_USER0_VAR }, { 0, 'X', STATUS_USER1_VAR },
	{ 0, 'Y', STATUS_USER2_VAR }, { 0, 'Z', STATUS_USER3_VAR },
	{ 0, '0', STATUS_USER0_VAR }, { 0, '1', STATUS_USER1_VAR },
	{ 0, '2', STATUS_USER2_VAR }, { 0, '3', STATUS_USER3_VAR },
	{ 0, '4', STATUS_USER4_VAR }, { 0, '5', STATUS_USER5_VAR },
	{ 0, '6', STATUS_USER6_VAR }, { 0, '7', STATUS_USER7_VAR },
	{ 0, '8', STATUS_USER8_VAR }, { 0, '9', STATUS_USER9_VAR },
	{ 1, '0', STATUS_USER10_VAR }, { 1, '1', STATUS_USER11_VAR },
	{ 1, '2', STATUS_USER12_VAR }, { 1, '3', STATUS_USER13_VAR },
	{ 1, '4', STATUS_USER14_VAR }, { 1, '5', STATUS_USER15_VAR },
	{ 1, '6', STATUS_USER16_VAR }, { 1, '7', STATUS_USER17_VAR },
	{ 1, '8', STATUS_USER18_VAR }, { 1, '9', STATUS_USER19_VAR },
	{ 2, '0', STATUS_USER20_VAR }, { 2, '1', STATUS_USER21_VAR },
	{ 2, '2', STATUS_USER22_VAR }, { 2, '3', STATUS_USER23_VAR },
	{ 2, '4', STATUS_USER24_VAR }, { 2, '5', STATUS_USER25_VAR },
	{ 2, '6', STATUS_USER26_VAR }, { 2, '7', STATUS_USER27_VAR },
	{ 2, '8', STATUS_USER28_VAR }, { 2, '9', STATUS_USER29_VAR },
	{ 3, '0', STATUS_USER30_VAR }, { 3, '1', STATUS_USER31_VAR },
	{ 3, '2', STATUS_USER32_VAR }, { 3, '3', STATUS_USER33_VAR },
	{ 3, '4', STATUS_USER34_VAR }, { 3, '5', STATUS_USER35_VAR },
	{ 3, '6', STATUS_USER36_VAR }, { 3, '7', STATUS_USER37_VAR },
	{ 3, '8', STATUS_USER38_VAR }, { 3, '9', STATUS_USER39_VAR },
	{ 0, 0, 0 },
	};

	text = NULL;
	for (i = 0; lookup[i].var; i++)
	{
		if (map == lookup[i].map && key == lookup[i].key)
		{
			text = get_string_var(lookup[i].var);
			break;
		}
	}

	if (text && (DISPLAY_ON_WINDOW || map > 1))
		return text;

	return empty_string;
}

STATUS_FUNCTION(status_scroll_info)
{
	static char my_buffer[81];

	if (window->scrollback_point)
	{
		snprintf(my_buffer, 80, " (Scroll: %d of %d)", 
				window->distance_from_display,
				window->display_buffer_size - 1);
	}
	else
		*my_buffer = 0;

	return my_buffer;
}

/*
 * Figures out how many lines are being "held" (never been displayed, usually
 * because of hold_mode or scrollback being on) for this window.
 */
STATUS_FUNCTION(status_hold_lines)
{
	int	num;
static	char	my_buffer[81];
	int	interval = window->hold_interval;

	if (interval == 0)
		interval = 1;		/* XXX WHAT-ever */

	if ((num = (window->lines_held / interval) * interval))
	{
		snprintf(my_buffer, 80, hold_lines_format, ltoa(num));
		return my_buffer;
	}

	return empty_string;
}


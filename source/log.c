/*
 * log.c: handles the irc session logging functions 
 *
 * $Id: log.c,v 1.2 2001/07/13 20:45:30 edwards Exp $
 */


#include "teknap.h"
#include "struct.h"

#include <sys/stat.h>

#include "log.h"
#include "vars.h"
#include "screen.h"
#include "output.h"
#include "ircaux.h"

FILE	*naplog_fp = NULL;

void do_log(int flag, char *logfile, FILE **fp)
{
	time_t	t = now;

	if (flag)
	{
		if (*fp)
			say("Logging is already on");
		else
		{
			if (!logfile)
				return;
			if (!(logfile = expand_twiddle(logfile)))
			{
				say("SET LOGFILE: No such user");
				return;
			}

			if ((*fp = fopen(logfile, get_int_var(APPEND_LOG_VAR)?"a":"w")) != NULL)
			{
				say("Starting logfile %s", logfile);
				chmod(logfile, S_IREAD | S_IWRITE);
#ifdef WINNT
				fprintf(*fp, "NAP log started %.24s\r\n", ctime(&t));
#else
				fprintf(*fp, "NAP log started %.24s\n", ctime(&t));
#endif
				fflush(*fp);
			}
			else
			{
				say("Couldn't open logfile %s: %s", logfile, strerror(errno));
				*fp = NULL;
			}
			new_free(&logfile);
		}
	}
	else
	{
		if (*fp)
		{
#ifdef WINNT
			fprintf(*fp, "NAP log ended %.24s\r\n", ctime(&t));
#else
			fprintf(*fp, "NAP log ended %.24s\n", ctime(&t));
#endif
			fflush(*fp);
			fclose(*fp);
			*fp = NULL;
			say("Logfile ended");
		}
	}
}

/* logger: if flag is 0, logging is turned off, else it's turned on */
void logger(Window *win, char *unused, int flag)
{
	char	*logfile;
	if ((logfile = get_string_var(LOGFILE_VAR)) == NULL)
	{
		say("You must set the LOGFILE variable first!");
		set_int_var(LOG_VAR, 0);
		return;
	}
	do_log(flag, logfile, &naplog_fp);
	if (!naplog_fp && flag)
		set_int_var(LOG_VAR, 0);
}

/*
 * set_log_file: sets the log file name.  If logging is on already, this
 * closes the last log file and reopens it with the new name.  This is called
 * automatically when you SET LOGFILE. 
 */
void set_log_file(Window *win, char *filename, int unused)
{
	char	*expanded;

	if (filename)
	{
		if (strcmp(filename, get_string_var(LOGFILE_VAR)))
			expanded = expand_twiddle(filename);
		else
			expanded = expand_twiddle(get_string_var(LOGFILE_VAR));
		set_string_var(LOGFILE_VAR, expanded);
		new_free(&expanded);
		if (naplog_fp)
		{
			logger(current_window, NULL, 0);
			logger(current_window, NULL, 1);
		}
	}
}

/*
 * add_to_log: add the given line to the log file.  If no log file is open
 * this function does nothing. 
 */
void add_to_log(FILE *fp, unsigned int winref, const char *line, int mangler)
{
int must_free = 0;

	if (fp && !inhibit_logging)
	{
		char *local_line;
		char *pend;
		
		int len = strlen(line) * 3 + 1;
		local_line = alloca(len);
		strcpy(local_line, line);

		/* Do this first */
		if (mangler)
			mangle_line(local_line, mangler, len);

                if ((pend = get_string_var(LOG_REWRITE_VAR)))
                {
                        char    *prepend_exp;
                        char    argstuff[10240];
                        int     args_flag;

                        /* First, create the $* list for the expando */
                        snprintf(argstuff, 10240, "%u %s", winref, local_line);

                        /* Now expand the expando with the above $* */
                        prepend_exp = expand_alias(pend, argstuff,
                                                   &args_flag, NULL);

                        local_line = prepend_exp;
                        must_free = 1;
                }

		
		
#ifdef WINNT
		fprintf(fp, "%s\r\n", local_line);
#else
		fprintf(fp, "%s\n", local_line);
#endif
		fflush(fp);
		if (must_free)
			new_free(&local_line);
	}
}

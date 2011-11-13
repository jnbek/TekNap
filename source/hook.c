/*
 * hook.c: Does those naughty hook functions. 
 *
 * Copyright 1990 Michael Sandrof
 * Copyright 1994 Matthew Green
 * Copyright 1997 EPIC Software Labs
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 * $Id: hook.c,v 1.1.1.1 2001/01/18 15:04:17 edwards Exp $
 */

#include "teknap.h"
#include "struct.h"
#include "alias.h"
#include "hook.h"
#include "ircaux.h"
#include "window.h"
#include "output.h"
#include "stack.h"

/*
 * The various ON levels: SILENT means the DISPLAY will be OFF and it will
 * suppress the default action of the event, QUIET means the display will be
 * OFF but the default action will still take place, NORMAL means you will be
 * notified when an action takes place and the default action still occurs,
 * NOISY means you are notified when an action occur plus you see the action
 * in the display and the default actions still occurs 
 */
static	char	*noise_level[] = { "UNKNOWN", "SILENT", "QUIET", "NORMAL", "NOISY" };

#define HF_NORECURSE	0x0001
#define HF_DEBUG	0x0010

/* Hook: The structure of the entries of the hook functions lists */

/*
 * NumericList: a special list type to dynamically handle numeric hook
 * requests 
 */
typedef struct numericlist_stru
{
struct	numericlist_stru *next;
	int	numeric;
	char	name[6];
	Hook	*list;
	char	*filename;
} NumericList;

NumericList *numeric_list 	= NULL;

#define MAXHOOK 11000

extern char *next_expr(char **, char);
extern int will_catch_return_exceptions;
extern int return_exception;
void parse_line(char *, char *, const char *, int, int, int);

/* A list of all the hook functions available */
struct
{
	char	*name;			/* The name of the hook type */
	Hook	*list;			/* The list of events for type */
	int	params;			/* Number of parameters expected */
	int	mark;			/* Hook type is currently active */
	unsigned flags;			/* Anything else needed */
} 
hook_functions[] =
{
	{ "ACTION",		(Hook *) 0,	3,	0,	0 },
	{ "BROWSE_BEGIN",	(Hook *) 0,	1,	0,	0 },
	{ "BROWSE_END",		(Hook *) 0,	1,	0,	0 },
	{ "CHANNEL_JOIN",	(Hook *) 0,	1,	0,	0 },
	{ "CONNECT",		(Hook *) 0,	2,	0,	0 },
	{ "DEBUG",		(Hook *) 0,	1,	0,	0 },
	{ "DISCONNECT",		(Hook *) 0,	1,	0,	0 },
	{ "ERROR",		(Hook *) 0,	0,	0,	0 },
	{ "EXEC",		(Hook *) 0,	2,	0,	0 },
	{ "EXEC_ERRORS",	(Hook *) 0,	2,	0,	0 },
	{ "EXEC_EXIT",		(Hook *) 0,	1,	0,	0 },
	{ "EXEC_PROMPT",	(Hook *) 0,	2,	0,	0 },
	{ "EXIT",		(Hook *) 0,	1,	0,	0 },
	{ "FLOOD",		(Hook *) 0,	2,	0,	0 },
	{ "GLIST",		(Hook *) 0,	2,	0,	0 },
	{ "HELP",		(Hook *) 0,	2,	0,	0 },
	{ "HOOK",		(Hook *) 0,	1,	0,	0 },
	{ "HOTLIST",		(Hook *) 0,	2,	0,	0 },
	{ "IDLE",		(Hook *) 0,	1,	0,	0 },
	{ "INPUT",		(Hook *) 0,	1,	0,	HF_NORECURSE },
	{ "JOIN",		(Hook *) 0,	4,	0,	0 },
	{ "KILL",		(Hook *) 0,	5,	0,	0 },
	{ "LASTLOG",		(Hook *) 0,	2,	0,	HF_NORECURSE },
	{ "LEAVE",		(Hook *) 0,	2,	0,	0 },
	{ "LIST",		(Hook *) 0,	3,	0,	0 },
	{ "MP3",		(Hook *) 0,	1,	0,	0 },
	{ "MSG",		(Hook *) 0,	2,	0,	0 },
	{ "NAMES",		(Hook *) 0,	1,	0,	0 },
	{ "NAP",		(Hook *) 0,	1,	0,	0 },
	{ "NAPERROR",		(Hook *) 0,	1,	0,	0 },
	{ "NAPFINISH",		(Hook *) 0,	1,	0, 	0 },
	{ "NAPGET",		(Hook *) 0,	1,	0,	0 },
	{ "NAPREQUEST",		(Hook *) 0,	1,	0,	0 },
	{ "NAPSEND",		(Hook *) 0,	1,	0,	0 },
	{ "PASTE",		(Hook *) 0,	2,	0,	0 },
	{ "PONG",		(Hook *) 0,	1,	0,	0 },
	{ "PUBLIC",		(Hook *) 0,	3,	0,	0 },
	{ "PUBLIC_OTHER",	(Hook *) 0,	3,	0,	0 },
	{ "RAW_NAP",		(Hook *) 0,	1,	0,	0 },
	{ "SEARCH_BEGIN",	(Hook *) 0,	1,	0,	0 },
	{ "SEARCH_END",		(Hook *) 0,	1,	0,	0 },
	{ "SEND_ACTION",	(Hook *) 0,	2,	0,	0 },
	{ "SEND_MSG",		(Hook *) 0,	2,	0,	HF_NORECURSE },
	{ "SEND_OPS",		(Hook *) 0,	1,	0,	0 },
	{ "SEND_PUBLIC",	(Hook *) 0,	2,	0,	HF_NORECURSE },
	{ "SERVERMSG",		(Hook *) 0,	1,	0,	0 },
	{ "SIGUSR1",		(Hook *) 0,	1,	0,	HF_NORECURSE },
	{ "STATUS_UPDATE",	(Hook *) 0,	3,	0,	0 },
	{ "SWITCH_CHANNELS",	(Hook *) 0,	3,	0,	0 },
	{ "SWITCH_WINDOWS",	(Hook *) 0,	4,	0,	0 },
	{ "TIMER",		(Hook *) 0,	1,	0,	0 },
	{ "TIMER_HOUR",		(Hook *) 0,	1,	0,	0 },
	{ "TOPIC",		(Hook *) 0,	2,	0,	0 },
	{ "WALL",		(Hook *) 0,	1,	0,	0 },
	{ "WHO",		(Hook *) 0,	6,	0,	0 },
	{ "WINDOW",		(Hook *) 0,	2,	0,	HF_NORECURSE },
	{ "WINDOW_CREATE",	(Hook *) 0,	1, 	0,	0 },
	{ "WINDOW_KILL",	(Hook *) 0,	1,	0,	0 },
};

static void 	    add_to_list 	(Hook **list, Hook *item);
static Hook *	    remove_from_list 	(Hook **list, char *item, int sernum);
static void 	    add_numeric_list 	(NumericList *item);
static NumericList *find_numeric_list 	(int numeric);
static NumericList *remove_numeric_list (int numeric);


/*
 * This converts a user-specified string of unknown composition and
 * returns a string that contains at minimum "params" number of words 
 * in it.  For any words that are forcibly added on, the last word will
 * be a * (so it can match any number of words), and any previous words
 * will be a % (so it forces the correct number of words to be caught.)
 */
static char *	fill_it_out (char *str, int params)
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	char	*arg,
		*ptr;
	int	i = 0;

	ptr = LOCAL_COPY(str);
	*buffer = 0;

	while ((arg = next_arg(ptr, &ptr)) != NULL)
	{
		if (*buffer)
			strmcat(buffer, " ", BIG_BUFFER_SIZE);
		strmcat(buffer, arg, BIG_BUFFER_SIZE);
		if (++i == params)
			break;
	}

	for (; i < params; i++)
		strmcat(buffer, (i < params-1) ? " %" : " *", BIG_BUFFER_SIZE);

	if (*ptr)
	{
		strmcat(buffer, " ", BIG_BUFFER_SIZE);
		strmcat(buffer, ptr, BIG_BUFFER_SIZE);
	}
	return m_strdup(buffer);
}


#define INVALID_HOOKNUM -1001

/*
 * find_hook: returns the numerical value for a specified hook name
 */
static int 	find_hook (char *name, int *first)
{
	int 	which = INVALID_HOOKNUM, i, cnt;
	size_t	len;

	if (first)
		*first = -1;

	if (!name || !(len = strlen(name)))
	{
		say("You must specify an event type!");
		return INVALID_HOOKNUM;
	}

	upper(name);

	for (cnt = 0, i = 0; i < NUMBER_OF_LISTS; i++)
	{
		if (!strncmp(name, hook_functions[i].name, len))
		{
			if (first && *first == -1)
				*first = i;

			if (strlen(hook_functions[i].name) == len)
			{
				cnt = 1;
				which = i;
				break;
			}
			else
			{
				cnt++;
				which = i;
			}
		}
		else if (cnt)
			break;
	}

	if (cnt == 0)
	{
		if (is_number(name))
		{
			which = atol(name);

			if ((which < 0) || (which > MAXHOOK))
			{
				say("Numerics must be between 001 and %d", MAXHOOK);
				return INVALID_HOOKNUM;
			}
			which = -which;
		}
		else
		{
			say("No such ON function: %s", name);
			return INVALID_HOOKNUM;
		}
	}
	else if (cnt > 1)
	{
		say("Ambiguous ON function: %s", name);
		return INVALID_HOOKNUM;
	}

	return which;
}




/* * * * * ADDING A HOOK * * * * * */
/*
 * This adds a numeric hook to the numeric hook list.  The composition of that
 * list is a linked list of "numeric holders" which simply indicate the 
 * numeric of that list, and a pointer to the actual list itself.  The actual
 * list itself is just a linked list of events sorted by serial number, then
 * by "nick".
 */
static void add_numeric_hook (int numeric, char *nick, char *stuff, int noisy, int not, int sernum, int flexible)
{
	NumericList *entry;
	Hook	*new_h;

	if (!(entry = find_numeric_list(numeric)))
	{
		entry = (NumericList *) new_malloc(sizeof(NumericList));
		entry->numeric = numeric;
		sprintf(entry->name, "%4.4u", numeric);
		entry->next = NULL;
		entry->list = NULL;
		add_numeric_list(entry);
	}

	if (!(new_h = remove_from_list(&entry->list, nick, sernum)))
	{
		new_h = (Hook *)new_malloc(sizeof(Hook));
		new_h->nick = NULL;
		new_h->stuff = NULL;
	}

	malloc_strcpy(&new_h->nick, nick);
	malloc_strcpy(&new_h->stuff, stuff);
	new_h->noisy = noisy;
	new_h->not = not;
	new_h->sernum = sernum;
	new_h->flexible = flexible;
	new_h->next = NULL;

	upper(new_h->nick);
	add_to_list(&entry->list, new_h);
}

/*
 * add_hook: Given an index into the hook_functions array, this adds a new
 * entry to the list as specified by the rest of the parameters.  The new
 * entry is added in alphabetical order (by nick). 
 */
static void add_hook (int which, char *nick, char *stuff, int noisy, int not, int sernum, int flexible)
{
	Hook	*new_h;

	if (which < 0)
	{
		add_numeric_hook(-which, nick, stuff, noisy, 
					not, sernum, flexible);
		return;
	}

	if (!(new_h = remove_from_list(&hook_functions[which].list, nick, sernum)))
	{
		new_h = (Hook *)new_malloc(sizeof(Hook));
		new_h->nick = NULL;
		new_h->stuff = NULL;
	}

	malloc_strcpy(&new_h->nick, nick);
	malloc_strcpy(&new_h->stuff, stuff);
	new_h->noisy = noisy;
	new_h->not = not;
	new_h->sernum = sernum;
	new_h->flexible = flexible;
	new_h->next = NULL;

	upper(new_h->nick);
	new_h->debug = (hook_functions[which].flags & HF_DEBUG) ? HF_DEBUG : 0;
	add_to_list(&hook_functions[which].list, new_h);
}




/* * * * * * REMOVING A HOOK * * * * * * * */
static void remove_numeric_hook (int numeric, char *nick, int sernum, int quiet)
{
	NumericList *hook;
	Hook	*tmp,
		*next;

	if ((hook = find_numeric_list(numeric)))
	{
		if (nick)
		{
			if ((tmp = remove_from_list(&hook->list, nick, sernum)))
			{
				if (!quiet)
				{
					say("%c%s%c removed from %d list",
					    (tmp->flexible?'\'':'"'), nick, 
					    (tmp->flexible?'\'':'"'), numeric);
				}
				new_free(&(tmp->nick));
				new_free(&(tmp->stuff));
				new_free(&(tmp->filename));
				new_free((char **)&tmp);

				if (!hook->list)
				{
					if ((hook = remove_numeric_list(numeric)))
						new_free((char **)&hook);
				}
				return;
			}
		}
		else
		{
			remove_numeric_list(numeric);
			for (tmp = hook->list; tmp; tmp = next)
			{
				next = tmp->next;
				tmp->not = 1;
				new_free(&(tmp->nick));
				new_free(&(tmp->stuff));
				new_free(&(tmp->filename));
				new_free((char **)&tmp);
			}
			hook->list = (Hook *) 0;
			new_free((char **)&hook);
			if (!quiet)
				say("The %d list is empty", numeric);
			return;
		}
	}

	if (quiet)
		return;

	if (nick)
		say("\"%s\" is not on the %d list", nick, numeric);
	else
		say("The %d list is empty", numeric);
}
 

static void remove_hook (int which, char *nick, int sernum, int quiet)
{
	Hook	*tmp,
		*next;

	if (which < 0)
	{
		remove_numeric_hook(-which, nick, sernum, quiet);
		return;
	}
	if (nick)
	{
		if ((tmp = remove_from_list(&hook_functions[which].list, nick, sernum)))
		{
			if (!quiet)
				say("%c%s%c removed from %s list", 
					(tmp->flexible?'\'':'"'), nick,
					(tmp->flexible?'\'':'"'),
					hook_functions[which].name);

			new_free(&(tmp->nick));
			new_free(&(tmp->stuff));
			new_free(&(tmp->filename));
			tmp->next = NULL;
			new_free((char **)&tmp); /* XXX why? */
		}
		else if (!quiet)
			say("\"%s\" is not on the %s list", nick,
					hook_functions[which].name);
	}
	else
	{
		Hook *prev = NULL;
		Hook *top = NULL;

		top = hook_functions[which].list;
		for (tmp = top; tmp; tmp = next)
		{
			next = tmp->next;

			/* 
			 * If given a non-zero sernum, then we clean out
			 * only those hooks that are at that level.
			 */
			if (sernum && tmp->sernum != sernum)
			{
				prev = tmp;
				continue;
			}

			if (prev)
				prev->next = tmp->next;
			else
				top = tmp->next;
			tmp->not = 1;
			new_free(&(tmp->nick));
			new_free(&(tmp->stuff));
			new_free(&(tmp->filename));
			tmp->next = NULL;
			new_free((char **)&tmp);
		}
		hook_functions[which].list = top;
		if (!quiet)
		{
			if (sernum)
				say("The %s <%d> list is empty", hook_functions[which].name, sernum);
			else
				say("The %s list is empty", hook_functions[which].name);
		}
	}
}

/* Used to bulk-erase all of the currently scheduled ONs */
void    flush_on_hooks (void)
{
        int x;
        int old_display = window_display;
        
        window_display = 0;
        for (x = 1; x < MAXHOOK; x++)
		remove_numeric_hook(x, NULL, x, 1);
        for (x = 0; x < NUMBER_OF_LISTS; x++)
		remove_hook(x, NULL, 0, 1); 
        window_display = old_display;
}

/* * * * * * SHOWING A HOOK * * * * * * */
/* show_hook shows a single hook */
static void 	show_hook (Hook *list, char *name)
{
	say("[*] On %s from %c%s%c do %s [%s] <%d>",
	    name,
	    (list->flexible?'\'':'"'), list->nick, (list->flexible?'\'':'"'), 
	    (list->not ? "nothing" : list->stuff),
	    noise_level[list->noisy],
	    list->sernum);
}

/*
 * show_numeric_list: If numeric is 0, then all numeric lists are displayed.
 * If numeric is non-zero, then that particular list is displayed.  The total
 * number of entries displayed is returned 
 */
static int show_numeric_list (int numeric)
{
	NumericList *tmp;
	Hook	*list;
	char	buf[6];
	int	cnt = 0;

	if (numeric)
	{
		sprintf(buf, "%4.4u", numeric);
		if ((tmp = find_numeric_list(numeric)))
		{
			for (list = tmp->list; list; list = list->next, cnt++)
				show_hook(list, tmp->name);
		}
	}
	else
	{
		for (tmp = numeric_list; tmp; tmp = tmp->next)
		{
			for (list = tmp->list; list; list = list->next, cnt++)
				show_hook(list, tmp->name);
		}
	}
	return (cnt);
}

/*
 * show_list: Displays the contents of the list specified by the index into
 * the hook_functions array.  This function returns the number of entries in
 * the list displayed 
 */
static int show_list (int which)
{
	Hook	*list;
	int	cnt = 0;

	/* Less garbage when issueing /on without args. (lynx) */
	for (list = hook_functions[which].list; list; list = list->next, cnt++)
		show_hook(list, hook_functions[which].name);
	return (cnt);
}



/* * * * * * * * EXECUTING A HOOK * * * * * * */
#define NO_ACTION_TAKEN		-1
#define SUPPRESS_DEFAULT	 0
#define DONT_SUPPRESS_DEFAULT	 1
#define RESULT_PENDING		 2
#define RESULT_NEXT		 3

/*
 * do_hook: This is what gets called whenever a MSG, INVITES, WALL, (you get
 * the idea) occurs.  The nick is looked up in the appropriate list. If a
 * match is found, the stuff field from that entry in the list is treated as
 * if it were a command. First it gets expanded as though it were an alias
 * (with the args parameter used as the arguments to the alias).  After it
 * gets expanded, it gets parsed as a command.  This will return as its value
 * the value of the noisy field of the found entry, or -1 if not found. 
 */
/* huh-huh.. this sucks.. im going to re-write it so that it works */
int 	do_hook (int which, char *format, ...)
{
	Hook		*tmp, *next = NULL, 
			**list;
	char		buffer		[BIG_BUFFER_SIZE * 10 + 1],
			*name 		= NULL;
	int		retval 		= DONT_SUPPRESS_DEFAULT;
	unsigned	display		= window_display;
	int		i;
	Hook		*hook_array	[2048];
	int		old_debug_count = debug_count;
	int		hook_num = 0;

	/*
	 * Figure out where the hooks are for the event type were asserting
	 */
	/* Numeric list */
	if (which < 0)
	{
		NumericList *hook;

		if ((hook = find_numeric_list(-which)))
		{
			name = hook->name;
			list = &hook->list;
		}
		else
			list = (Hook **) 0;
	}

	/* Named list */
	else
	{
		/*
		 * If we're already executing the type, and we're
		 * specifically not supposed to allow recursion, then
		 * dont allow recursion. ;-)
		 */
		if (hook_functions[which].mark && 
		    (hook_functions[which].flags & HF_NORECURSE))
			list = NULL;
		else
		{
			list = &(hook_functions[which].list);
			name = hook_functions[which].name;
		}
	}

	/*
	 * No hooks to look at?  No problem.  Drop out.
	 */
	if (!list)
		return NO_ACTION_TAKEN;


	/*
	 * Press the buffer using the specified format string and args
	 * We do this here so that we dont waste time doing the vsnprintf
	 * if we're not going to do any matching.  So for types where the
	 * user has no hooks, its a cheapie call.
	 */
	if (format)
	{
		va_list args;
		va_start (args, format);
		vsnprintf(buffer, BIG_BUFFER_SIZE * 10, format, args);
		va_end(args);
	}
	else
		nappanic("do_hook: format is NULL");


	/*
	 * Mark the event as being executed.  This is used to suppress
 	 * unwanted recursion in some /on's.
	 */
	if (which >= 0)
		hook_functions[which].mark++;


	/* not attached, so dont "fix" it */
	{
		int 	currser 	= 0,
			oldser 		= INT_MIN,
			currmatch 	= 0, 
			oldmatch 	= 0;
		Hook *	bestmatch 	= NULL;

		/*
		 * Walk the list of hooks for this event
		 */
		for (tmp = *list; tmp; tmp = tmp->next)
		{
			char *	tmpnick = NULL;
			int 	sa;

			/*
			 * save the current serial number
			 */
			currser = tmp->sernum;

			/*
			 * Is this a different serial number than the
			 * last hook?  If it is, then we save the previous
			 * serial number's best hook to the hook_array.
			 */
			if (currser != oldser)
			{
				/*
				 * This would happen if the list is out
				 * of order.  This probably should not be
				 * fatal, but it is probably irrecoverable.
				 * It needs to be trapped and fixed.
				 */
				if (currser < oldser)
					nappanic("currser [%d] is less than oldser [%d]", currser, oldser);

				oldser = currser;
				currmatch = oldmatch = 0;
				if (bestmatch)
					hook_array[hook_num++] = bestmatch;
				bestmatch = NULL;
			}

			/*
			 * If this is a flexible hook, expand the nick stuff
			 */
			if (tmp->flexible)
				/* 
				 * XXX Something ought to be done here
				 * with current_window, but ick, how?
				 */
				tmpnick = expand_alias(tmp->nick, empty_string,
							 &sa, NULL);
			else
				tmpnick = tmp->nick;


			/*
			 * Check to see if the pattern matches the text
			 */
			currmatch = wild_match(tmpnick, buffer);

			/*
			 * If it is the "best match" so far, then we mark
			 * its "value" and save a pointer to it.
			 */
			if (currmatch > oldmatch)
			{
				oldmatch = currmatch;
				bestmatch = tmp;
			}

			/*
			 * Clean up after flexible hooks
			 */
			if (tmp->flexible)
				new_free(&tmpnick);
		}

		/*
		 * Ok. we've walked the list.  If the last hook had a best
		 * match, use that one too. =)
		 */
		if (bestmatch)
			hook_array[hook_num++] = bestmatch;
	}


	/*
	 * Now we walk the list of collected hook events that are to be run
	 */
	for (i = 0; i < hook_num; i++)
	{
		char *		name_copy;
		char *		stuff_copy;
		char *		result = NULL;
		char *		result1 = NULL;
	const	char *		saved_who_from = NULL;
		unsigned long	saved_who_level = 0;
		int		old_alias_debug = alias_debug;
				
		/*
		 * This should never happen.
		 */
		if (!(tmp = hook_array[i]))
			nappanic("hook_array[%d] is null", i);

		/* 
		 * Check to see if this hook is supposed to supress the
		 * default action for the event.
		 */
hook_next:
		if (tmp->noisy == SILENT && tmp->sernum == 0)
			retval = SUPPRESS_DEFAULT;
		else if (tmp->noisy == UNKNOWN && tmp->sernum == 0)
			retval = RESULT_PENDING;

		/*
		 * If this is a negated event, or there isnt anything to be
		 * executed, then we dont bother.  Just go on to the next one
		 */
		if (tmp->not || !tmp->stuff || !*tmp->stuff)
			continue;

		/*
		 * If this is a NORMAL or NOISY hook, then we tell the user
		 * that we're going to execute the hook.
		 */
		if (tmp->noisy > QUIET)
			say("%s activated by %c%s%c", 
				name, tmp->flexible ? '\'' : '"',
				buffer, tmp->flexible ? '\'' : '"');


		if ((tmp->debug & HF_DEBUG) && (internal_debug & DEBUG_HOOK) && !in_debug_yell)
		{
			debugyell("%3d ON %s activated [%s]", debug_count++, name, buffer);
			alias_debug++;
		}

		/*
		 * Save some information that may be reset in the 
		 * execution, turn off the display if the user specified.
		 */
		save_display_target(&saved_who_from, &saved_who_level);
		if (tmp->noisy < NOISY)
			window_display = 0;
		
		name_copy = LOCAL_COPY(name);
		stuff_copy = LOCAL_COPY(tmp->stuff);

		if (tmp->noisy == UNKNOWN)
			result = parse_line_with_return(name_copy, stuff_copy, buffer, 0, 0);
		else
		{
			/*
			 * Ok.  Go and run the code.  It is imperitive to note
			 * that "tmp" may be deleted by the code executed here,
			 * so it is absolutely forbidden to reference "tmp" after
			 * this point.
			 */
			next = tmp->next;
			will_catch_return_exceptions++;
			result1 = parse_line_with_return(name_copy, stuff_copy, buffer, 0, 0);
			will_catch_return_exceptions--;
			return_exception = 0;
		}

		if (retval == RESULT_PENDING)
		{
			if (result && atol(result))
				retval = SUPPRESS_DEFAULT;
			else
				retval = DONT_SUPPRESS_DEFAULT;
		}

		if (result1 && *result1 && !my_stricmp(result1, "next"))
		{
			new_free(&result1);
			{
				if ((tmp = next))
					goto hook_next;
				else
					retval = RESULT_NEXT;
			}
		}
		new_free(&result1);
		/*
		 * Clean up the stuff that may have been mangled by the
		 * execution.
		 */
		restore_display_target(saved_who_from, saved_who_level);
		window_display = display;
		alias_debug = old_alias_debug;
		new_free(&result);
	}

	/*
	 * Mark the event as not currently being done here.
	 */
	if (which >= 0)
		hook_functions[which].mark--;

	if (old_debug_count == 1)
		debug_count = 1;
	/*
	 * And return the user-specified suppression level
	 */
	return retval;
}

/* * * * * * SCHEDULING AN EVENT * * * * * * * */
/*
 * The ON command:
 * Format:		/ON [#][+-^]TYPE ['] [SERNUM] NICK ['] [{] STUFF [}]
 *
 * The "ON" command mainly takes three arguments.  The first argument
 * is the "type" of callback that you want to schedule.  This is either
 * a three digit number, of it is one of the strings so enumerated at the
 * top of this file in hook_list.  The second argument is the "nick" or
 * "pattern" that is to be used to match against future events.  If the
 * "nick" matches the text that is later passed to do_hook() with the given
 * "type", then the commands in "stuff" will be executed.
 *
 * If "nick" is enclosed in single quotes ('), then it is a "flexible" 
 * pattern, and will be expanded before it is matched against the text
 * in do_hook.  Otherwise, the string so specified is "static", and is
 * used as-is in do_hook().
 *
 * Within each type, there are at least 65,535 different "serial numbers",
 * (there actually are MAX_INT of them, but by convention, only 16 bit
 * serial numbers are used, from -32,768 to 32,767) which may be used to 
 * schedule any number of events at the given serial number.
 *
 * Each time an assertion occurs for a given "type", at most one of the
 * scheduled events is executed for each of the distinct serial numbers that
 * are in use for that event.  The event to be executed is the one at a 
 * given serial number that "best" matches the text passed to do_hook().
 * While in theory, up to MAX_INT events could be executed for a given single
 * assertion, in practice, a hard limit of 2048 events per assertion is 
 * enforced.
 * 
 * The runtime behavior of the event being scheduled can be modified by
 * specifying a character at the beginning of the "type" argument.  If you
 * want to schedule an event at a serial number, then the first character
 * must be a hash (#).  The argument immediately FOLLOWING the "type"
 * argument, and immediately PRECEEDING the "nick" argument must be an 
 * integer number, and is used for the serial number for this event.
 *
 * The "verbosity" of the event may also be modified by specifying at most
 * one of the following characters:  
 *	A caret (^) is the SILENT level, and indicates that the event is to
 *		be executed with no output (window_display is turned off),
 *		and the "default action" (whatever that is) for the event is
 *		to be suppressed.  The default action is actually only 
 *		suppressed if the SILENT level is specified for serial number
 *		zero.  This is the most common level used for overriding the
 *		output of most /on's.
 *	A minus (-) is the QUIET level, and is the same as the SILENT level,
 *		except that the default action (whatever that is) is not to
 *		be suppressed.
 *	No character is the "normal" case, and is the same as the "minus"
 *		level, with the addition that the client will inform you that
 *		the event was executed.  This is useful for debugging.
 *	A plus (+) is the same as the "normal" (no character specified),
 *		except that the output is not suppressed (window_display is 
 *		not changed.)
 */
BUILT_IN_COMMAND(oncmd)
{
	char	*func,
		*nick,
		*serial;
	Noise	noisy		= NORMAL;
	int	not		= 0,
		sernum		= 0,
		remove		= 0,
		which		= INVALID_HOOKNUM;
	int	flex		= 0;
	char	type;
	int	first;

	/*
	 * Get the type of event to be scheduled
	 */
	if ((func = next_arg(args, &args)) != NULL)
	{
		/*
		 * Check to see if this has a serial number.
		 */
		if (*func == '#')
		{
			if (!(serial = next_arg(args, &args)))
			{
				say("No serial number specified");
				return;
			}
			sernum = atol(serial);
			func++;
		}

		/*
		 * Get the verbosity level, if any.
		 */
		switch (*func)
		{
			case '?':
				noisy = UNKNOWN;
				func++;
				break;
			case '-':
				noisy = QUIET;
				func++;
				break;
			case '^':
				noisy = SILENT;
				func++;
				break;
			case '+':
				noisy = NOISY;
				func++;
				break;
			default:
				noisy = NORMAL;
				break;
		}

		
		/*
		 * Check to see if the event type is valid
		 */
		if ((which = find_hook(func, &first)) == INVALID_HOOKNUM)
		{
			/*
			 * Ok.  So either the user specified an invalid type
			 * or they specified an ambiguous type.  Either way,
			 * we're not going to be going anywhere.  So we have
			 * free reign to mangle 'args' at this point.
			 */

			int len;

			/*
			 * If first is -1, then it was an unknown type.
			 * An error has already been output, just return here
			 */
			if (first == -1)
				return;

			/*
			 * Otherwise, its an ambiguous type.  If they were 
			 * trying to register the hook, then they've already
			 * gotten the error message, just return;
			 */
			if (new_new_next_arg(args, &args, &type))
				return;

			/*
			 * Ok.  So they probably want a listing.
			 */
			len = strlen(func);
			while (!my_strnicmp(func, hook_functions[first].name, len))
			{
			    if (!show_list(first))
				say("The %s list is empty.", 
					hook_functions[first].name);
			    first++;
			    if (!hook_functions[first].name)
			    	break;
			}
			return;
		}

		/*
		 * Check to see if this is a removal event or if this
		 * is a negated event.
		 */
		switch (*args)
		{
			case '-':
				remove = 1;
				args++;
				break;
			case '^':
				not = 1;
				args++;
				break;
		}

		/*
		 * Grab the "nick"
		 */
		if ((nick = new_new_next_arg(args, &args, &type)))
		{
			char *exp;

			/*
			 * Pad it to the appropriate number of args
			 */
			if (which < 0)
				nick = fill_it_out(nick, 1);
			else
				nick = fill_it_out(nick, hook_functions[which].params);

			/*
			 * If nick is empty, something is very wrong.
			 */
			if (!*nick)
			{
				say("No expression specified");
				new_free(&nick);
				return;
			}

			/*
			 * If we're doing a removal, do the deed.
			 */
			if (remove)
			{
				remove_hook(which, nick, sernum, 0);
				new_free(&nick);
				return;
			}

			/*
			 * Take a note if its flexible or not.
			 */
			if (type == '\'')
				flex = 1;
			else
				flex = 0;

			
			/*
			 * If this is a negative event, then we dont want
			 * to take any action for it.
			 */
			if (not)
				args = empty_string;


			/*
			 * Slurp up any whitespace after the nick
			 */
			while (my_isspace(*args))
				args++;

			/*
			 * Then slurp up the body ("text")
			 */
			if (*args == '{') /* } */
			{
				if (!(exp = next_expr(&args, '{'))) /* } */
				{
					say("Unmatched brace in ON");
					new_free(&nick);
					return;
				}
			}
			else
				exp = args;

			/*
			 * Schedule the event
			 */
			add_hook(which, nick, exp, noisy, not, sernum, flex);

			/*
			 * Tell the user that we're done.
			 */
			if (which < 0)
				say("On %4.4u from %c%s%c do %s [%s] <%d>",
				    -which, type, nick, type, 
				    (not ? "nothing" : exp),
				    noise_level[noisy], sernum);
			else
				say("On %s from %c%s%c do %s [%s] <%d>",
					hook_functions[which].name, 
					type, nick, type,
					(not ? "nothing" : exp),
					noise_level[noisy], sernum);

			/*
			 * Clean up after the nick
			 */
			new_free(&nick);
		}

		/*
		 * No "nick" argument was specified.  That means the user
		 * either is deleting all of the events of a type, or it
		 * wants to list all the events of a type.
		 */
		else
		{
			/*
			 * if its a removal, do the deed
			 */
			if (remove)
			{
				remove_hook(which, (char *) 0, sernum, 0);
				return;
			}
	
			/*
			 * The help files say that an "/on 0" shows all
			 * of the numeric ONs.  Since the ACTION hook is
			 * number 0, we have to check to see if the first
			 * character of "func" is a zero or not.  If it is,
			 * we output all of the numeric functions.
			 */
			if (*func == '0')
			{
				if (!show_numeric_list(0))
				    say("All numeric ON lists are empty.");
			}
			else if (which < 0)
			{
				if (!show_numeric_list(-which))
				    say("The %4.4u list is empty.", -which);
			}
			else if (!show_list(which))
				say("The %s list is empty.", 
					hook_functions[which].name);
		}
	}

	/*
	 * No "Type" argument was specified.  That means the user wants to
	 * list all of the ONs currently scheduled.
	 */
	else
	{
		int	total = 0;

		say("ON listings:");

		/*
		 * Show the named events
		 */
		for (which = 0; which < NUMBER_OF_LISTS; which++)
			total += show_list(which);

		/*
		 * Show the numeric events
		 */
		total += show_numeric_list(0);

		if (!total)
			say("All ON lists are empty.");
	}
}


/* * * * * * * * * * SAVING A HOOK * * * * * * * * * * */
static	void	write_hook (FILE *fp, Hook *hook, char *name)
{
	char	*stuff = (char *) 0;
	char	flexi = '"';

	if (hook->flexible)
		flexi = '\'';

	switch (hook->noisy)
	{
		case SILENT:
			stuff = "^";
			break;
		case QUIET:
			stuff = "-";
			break;
		case NORMAL:
			stuff = empty_string;
			break;
		case NOISY:
			stuff = "+";
			break;
		case UNKNOWN:
			stuff = "?";
			break;
	}

	if (hook->sernum)
		fprintf(fp, "ON #%s%s %d", stuff, name, hook->sernum);
	else
		fprintf(fp, "ON %s%s", stuff, name);

	fprintf(fp, " %c%s%c {%s}\n", flexi, hook->nick, flexi, hook->stuff);
}

/*
 * save_hooks: for use by the SAVE command to write the hooks to a file so it
 * can be interpreted by the LOAD command 
 */
void	save_hooks (FILE *fp, int do_all)
{
	Hook	*list;
	NumericList *numeric;
	int	which;

	for (which = 0; which < NUMBER_OF_LISTS; which++)
	{
		for (list = hook_functions[which].list; list; list = list->next)
			if (!list->global || do_all)
				write_hook(fp,list, hook_functions[which].name);
	}
	for (numeric = numeric_list; numeric; numeric = numeric->next)
	{
		for (list = numeric->list; list; list = list->next)
			if (!list->global)
				write_hook(fp, list, numeric->name);
	}
}


/* * * * * * * * * * STACKING A HOOK * * * * * * * * */
typedef struct  onstacklist
{
	int     which;
	Hook    *list;
	struct onstacklist *next;
}       OnStack;

static	OnStack	*	on_stack = NULL;

void	do_stack_on (int type, char *args)
{
	int	which;
	Hook	*list;
	NumericList *nhook = NULL, *nptr;

	if (!on_stack && (type == STACK_POP || type == STACK_LIST))
	{
		say("ON stack is empty!");
		return;
	}
	if (!args || !*args)
	{
		say("Missing event type for STACK ON");
		return;
	}

	if ((which = find_hook(args, NULL)) == INVALID_HOOKNUM)
		return;		/* Error message already outputted */

	if (which < 0)
	{
		if ((nhook = find_numeric_list(-which)))
			list = nhook->list;
		else
			list = NULL;
	}
	else
		list = hook_functions[which].list;


	if (type == STACK_PUSH)
	{
		OnStack	*new_os;
		new_os = (OnStack *) new_malloc(sizeof(OnStack));
		new_os->which = which;
		new_os->list = list;
		new_os->next = on_stack;
		on_stack = new_os;

		if (which < 0)
		{
			if (nhook && numeric_list)
				remove_numeric_list(-which);
		}
		else
			hook_functions[which].list = NULL;

		return;
	}

	else if (type == STACK_POP)
	{
		OnStack	*p, *tmp = NULL;

		for (p = on_stack; p; tmp = p, p = p->next)
		{
			if (p->which == which)
			{
				if (p == on_stack)
					on_stack = p->next;
				else
					tmp->next = p->next;
				break;
			}
		}
		if (!p)
		{
			say("No %s on the stack", args);
			return;
		}

		if ((which < 0 && nhook) || 
		    (which >= 0 && hook_functions[which].list))
			remove_hook(which, NULL, 0, 1);	/* free hooks */

		if (which < 0)
		{
			/* look -- do we have any hooks already for this numeric? */
			if (!(nptr = find_numeric_list(-which)))
			{
				if (p->list)	/* not just a placeholder? */
				{
					/* No. make a new list and put the stack on it */
					nptr = (NumericList *) new_malloc(sizeof(NumericList));
					nptr->list = p->list;
					nptr->next = NULL;
					nptr->numeric = -which;
					sprintf(nptr->name, "%3.3u", -which);
					add_numeric_list(nptr);
				}
			}
			else
			{
				remove_hook(which, NULL, 0, 1);
				nptr->list = p->list;
			}
		}
		else
			hook_functions[which].list = p->list;

		new_free((char **)&p);
		return;
	}

	else if (type == STACK_LIST)
	{
		int	slevel = 0;
		OnStack	*osptr;

		for (osptr = on_stack; osptr; osptr = osptr->next)
		{
			if (osptr->which == which)
			{
				Hook	*hptr;

				slevel++;
				say("Level %d stack", slevel);
				for (hptr = osptr->list; hptr; hptr = hptr->next)
					show_hook(hptr, args);
			}
		}

		if (!slevel)
			say("The STACK ON %s list is empty", args);
		return;
	}
	say("Unknown STACK ON type ??");
}


/* List manips especially for on's. */
static void 	add_to_list (Hook **list, Hook *item)
{
	Hook *tmp, *last = NULL;

	for (tmp = *list; tmp; last = tmp, tmp = tmp->next)
	{
		if (tmp->sernum < item->sernum)
			continue;
		else if ((tmp->sernum == item->sernum) && (my_stricmp(tmp->nick, item->nick) < 0))
			continue;
		else
			break;
	}

	if (last)
	{
		item->next = last->next;
		last->next = item;
	}
	else
	{
		item->next = *list;
		*list = item;
	}
}


static Hook *remove_from_list (Hook **list, char *item, int sernum)
{
	Hook *tmp, *last = NULL;

	for (tmp = *list; tmp; last = tmp, tmp = tmp->next)
	{
		if (tmp->sernum == sernum && !my_stricmp(tmp->nick, item))
		{
			if (last)
				last->next = tmp->next;
			else
				*list = tmp->next;
			return tmp;
		}
	}
	return NULL;
}


static void add_numeric_list (NumericList *item)
{
	NumericList *tmp, *last = NULL;

	for (tmp = numeric_list; tmp; last = tmp, tmp = tmp->next)
	{
		if (tmp->numeric > item->numeric)
			break;
	}

	if (last)
	{
		last->next = item;
		item->next = tmp;
	}
	else
	{
		item->next = numeric_list;
		numeric_list = item;
	}
}

static NumericList *find_numeric_list (int numeric)
{
	NumericList *tmp, *last = NULL;

	for (tmp = numeric_list; tmp; last = tmp, tmp = tmp->next)
	{
		if (tmp->numeric == numeric)
			return tmp;
	}

	return NULL;
}

static NumericList *remove_numeric_list (int numeric)
{
	NumericList *tmp, *last = NULL;

	for (tmp = numeric_list; tmp; last = tmp, tmp = tmp->next)
	{
		if (tmp->numeric == numeric)
			break;
	}

	if (last)
		last->next = tmp->next;
	else
		numeric_list = numeric_list->next;

	return tmp;
}

void debug_hook(char *name, int x)
{
int cnt = 0, i;

	for (cnt = 0, i = 0; i < NUMBER_OF_LISTS; i++)
	{
		if (!strcmp(name, hook_functions[i].name))
		{
			Hook *tmp;
			if (x)
				hook_functions[i].flags |= HF_DEBUG;
			else
				hook_functions[i].flags &= ~(HF_DEBUG);
			for (tmp = hook_functions[i].list; tmp; tmp = tmp->next)
			{
				if (x)
					tmp->debug |= HF_DEBUG;
				else
					tmp->debug &= ~(HF_DEBUG);
				cnt++;
			}
			break;
		}
	}

	if (cnt == 0)
	{
		int which;
		if (is_number(name))
		{
			NumericList *tmp;
			which = atol(name);

			if ((which < 0) || (which > MAXHOOK))
				return;
			
			if ((tmp = find_numeric_list(which)))
			{
				Hook *list;
				for (list = tmp->list; list; list = list->next, cnt++)
				{
					if (x)
						list->debug |= HF_DEBUG;
					else
						list->debug &= ~(HF_DEBUG);
					cnt++;
				}
			}
		}
	}
}


/* Copyright (C) 2000 drscholl@users.sourceforge.net
   This is free software distributed under the terms of the
   GNU Public License.  See the file COPYING for details.

   $Id: debug.c,v 1.1.1.1 2001/01/13 11:30:17 edwards Exp $ */

/* This is a very simple memory management debugger.  It's useful for detecting
   memory leaks, references to uninitialzed memory, bad pointers, buffer
   overflow and getting an idea of how much memory is used by a program. */

#include "teknap.h"
#include "struct.h"
#include "hook.h"
#include "input.h"
#include "output.h"
#include "lastlog.h"
#include "debug.h"
#include "screen.h"
#include "status.h"
#include "window.h"
#include "vars.h"

#ifdef DEBUG_MEM

#define MIN(a,b) ((a<b)?a:b)

/* tunable parameter.  if you have a large amount of memory allocated, setting
   this value higher will result in faster validation of pointers */
#define SIZE 4099

typedef struct _block
{
    void *val;
    int len;
    const char *file;
    int line;
    struct _block *next;
}
BLOCK;

static BLOCK *Allocation[SIZE];
static int Memory_Usage = 0;
static int Memory_debug = 0;

struct _file_manager
{
	int init;
	int count;
	int max_fd;
	char *files[FD_SETSIZE+1];
} file_manager =  {0, 0, -1, {NULL}};

void
debug_init (void)
{
	memset (Allocation, 0, sizeof (Allocation));
	file_manager.init = 1;
}

void
debug_start(void)
{
	Memory_debug = 1;
}

#if SIZEOF_LONG == 8
#define SHIFT 3
#else
#define SHIFT 2
#endif

/* hash the pointer value for insertion into the table */
static int
debug_hash (void *ptr)
{
    int hash;

    /* pointers are allocated on either 4 or 8 bytes boundaries, so we want
       to ignore those values.  this will cause consecutive pointers to hash
       to the next bin */
    hash = ((unsigned long) ptr) >> SHIFT;
    return ((hash & 0x7fffffff) % SIZE);
}

static int
debug_overflow (BLOCK *block, const char *func)
{
    if (*((unsigned char *) block->val + block->len) != END_BYTE)
    {
	fprintf(stderr,
		"debug_%s: buffer overflow detected in data allocated at %s:%d\n",
		func, block->file, block->line);
	return 1;
    }
    return 0;
}

static void
debug_exhausted (const char *file, int line)
{
    yell( "debug_malloc(): memory exhausted at %s:%d (%d bytes allocated)",
	    file, line, Memory_Usage);
}

void *
debug_malloc (int bytes, const char *file, int line)
{
    BLOCK *block, **ptr;
    int offset;

    if (bytes == 0)
	return 0;

#if 0
    if (!Memory_debug)
    {
    	Memory_Usage += bytes;
    	return calloc(1, bytes + 1);
    }
#endif
    block = malloc (sizeof (BLOCK));
    if (!block)
    {
	debug_exhausted (file, line);
	return 0;
    }
    block->val = calloc (1, bytes + 1);
    if (!block->val)
    {
	debug_exhausted (__FILE__, __LINE__);
	free (block);
	return 0;
    }
    Memory_Usage += bytes;
    block->len = bytes;
    block->file = file;
    block->line = line;
/*    memset (block->val, 0, bytes);*/
    *((unsigned char *) block->val + bytes) = END_BYTE;

    offset = debug_hash (block->val);
    for (ptr = &Allocation[offset]; *ptr; ptr = &(*ptr)->next)
    {
	if (block->val < (*ptr)->val)
	    break;
    }
    block->next = *ptr;
    *ptr = block;
    return block->val;
}

void *
debug_calloc (int count, int bytes, const char *file, int line)
{
    void *ptr = debug_malloc (count * bytes, file, line);
    if (!ptr)
	return 0;
    memset (ptr, 0, count * bytes);
    return ptr;
}

static BLOCK *
find_block (void *ptr)
{
    int offset = debug_hash (ptr);
    BLOCK *block;

    for (block = Allocation[offset]; block && ptr > block->val; block = block->next);
    return ((block != 0 && ptr == block->val) ? block : 0);
}

void *
debug_realloc (void **ptr, int bytes, const char *file, int line)
{
    void *newptr;
    BLOCK *block = 0;

    if (bytes == 0)
    {
	debug_free (ptr, file, line);
	return 0;
    }
    if (*ptr)
    {
	block = find_block (*ptr);
	if (!block)
	{
	    yell(
		     "debug_realloc(): invalid pointer at %s:%d", file,
		     line);
	    return 0;
	}
	debug_overflow (block, "realloc");
    }
    newptr = debug_malloc (bytes, file, line);
    if (!newptr)
	return 0;
    if (*ptr)
    {
	memcpy (newptr, *ptr, MIN (bytes, block->len));
	debug_free (ptr, file, line);
    }
    *ptr = newptr;
    return newptr;
}

void
debug_free (void **ptr, const char *file, int line)
{
    BLOCK **list, *block = 0;
    int offset;

    if (!*ptr)
	return;

    offset = debug_hash (*ptr);
    if (!Allocation[offset])
    {
	yell(
		 "debug_free: attempt to free bogus pointer at %s:%d",
		 file, line);
	return;
    }
    for (list = &Allocation[offset]; *list; list = &(*list)->next)
    {
	if ((*list)->val == *ptr)
	    break;
    }
    if (!*list)
    {
	yell(
		 "debug_free: attempt to free bogus pointer at %s:%d",
		 file, line);
	return;
    }
    block = *list;
    /* remove the block from the list */
    *list = (*list)->next;

    debug_overflow (block, "free");
    memset (block->val, FREE_BYTE, block->len);
    free (block->val);
    Memory_Usage -= block->len;
    if (ptr)
        *ptr = NULL;
    free (block); 
}

#if 0
/* display the contents of an allocated block */
static void
debug_dump (BLOCK *block)
{
    int i;

    fputc('\t', stderr);
    for (i = 0; i < block->len && i < 8; i++)
	fprintf(stderr, "%02x ", *((unsigned char*)block->val+i));
    fputc('\t', stderr);
    for (i = 0; i < block->len && i < 8; i++)
	fprintf(stderr, "%c", isprint(*((unsigned char*)block->val+i))?*((unsigned char*)block->val+i):'.');
    fputc('\n',stderr);
}
#endif

void
debug_cleanup (void)
{
    int i;
    BLOCK *block;

    for (i = 0; i < SIZE; i++)
    {
	for (block = Allocation[i]; block; block = block->next)
	{
	    debug_overflow (block, "cleanup");
	    fprintf(stderr, "debug_cleanup: %d bytes allocated at %s:%d\n",
		    block->len, block->file, block->line);
#if 0
	    debug_dump (block);
#endif
	}
    }
    if (Memory_Usage)
	fprintf(stderr, "debug_cleanup: %d bytes total\n", Memory_Usage);
}

char *
debug_strdup (const char *s, const char *file, int line)
{
    char *r;
    r = debug_malloc (strlen (s ? s : "") + 1, file, line);
    if (!r)
	return 0;
    strcpy (r, s ? s : "");
    return r;
}

/* check to see if a pointer is valid */
int
debug_valid (void *ptr, int len)
{
    BLOCK * block = find_block (ptr);

    if (!block)
    {
	yell( "debug_valid: invalid pointer\n");
	return 0; /* not found */
    }
    if (debug_overflow (block, "valid"))
	return 0;
    /* ensure that there are at least `len' bytes available */
    return ((len <= block->len));
}

int
debug_usage (void)
{
    return Memory_Usage;
}

void open_file(int fd, char *filename, const char *file, const int line)
{
	if (fd < 0)
		return;
	file_manager.files[fd] = debug_strdup(filename, file, line);
	file_manager.count++;
}

void close_file(int fd, const char *file, const int line)
{
	if (fd < 0)
		return;
	debug_free((void **)&file_manager.files[fd], file, line);
	file_manager.count--;
}

void print_open(void)
{
int i;
	for (i = 0; i < FD_SETSIZE; i++)
	{
		if (file_manager.files[i])
			put_it("fd %d -> %s", i, file_manager.files[i]);
	}
}

#else

void debug_free (void **ptr, const char *file, int line)
{
    if (!*ptr)
	return;

    free (*ptr);
    *ptr = NULL;
}


void *debug_malloc (int bytes, const char *file, int line)
{
	return calloc(1, bytes);
}
void *debug_calloc (int count, int bytes, const char *file, int line)
{
	void *ptr = debug_malloc (count * bytes, file, line);
	if (!ptr)
		return NULL;
	return ptr;
}

void *debug_realloc (void **ptr, int bytes, const char *file, int line)
{
    void *newptr;

    if (bytes == 0)
    {
	debug_free (ptr, file, line);
	return NULL;
    }
    newptr = realloc(*ptr, bytes);
    if (!newptr)
	return NULL;
    *ptr = newptr;
    return newptr;
}

char *debug_strdup (const char *s, const char *file, int line)
{
    char *r;

    r = debug_malloc (strlen (s ? s : "") + 1, file, line);
    if (!r)
	return NULL;
    strcpy (r, s ? s : "");
    return r;
}
void debug_cleanup (void)
{
}

void debug_init (void)
{
}

void open_file(int fd, char *filename, const char *file, const int line)
{
}

void close_file(int fd, const char *file, const int line)
{
}

void print_open(void)
{
}


#endif /* DEBUG */

unsigned long internal_debug = 0;
unsigned long alias_debug = 0;
unsigned int debug_count = 1;
int in_debug_yell = 0;

void	debugyell(const char *format, ...)
{
const char *save_from;
unsigned long save_level;
int refnum = 0;
unsigned long old_alias_debug = alias_debug;

	if (target_window)
		refnum = target_window->refnum;

	alias_debug = 0;
	save_display_target(&save_from, &save_level);
	set_display_target(NULL, LOG_DEBUG);
	if (format)
	{
		char debugbuf[BIG_BUFFER_SIZE+1];
		va_list args;
		va_start (args, format);
		*debugbuf = 0;
		vsnprintf(debugbuf, BIG_BUFFER_SIZE, format, args);
		va_end(args);
		in_debug_yell = 1;
		if (*debugbuf && do_hook(DEBUG_LIST, "%s", debugbuf))
			put_echo(debugbuf);
		in_debug_yell = 0;
	}
	alias_debug = old_alias_debug;
	reset_display_target();
	restore_display_target(save_from, save_level);
	set_display_target_by_winref(refnum);
}

int parse_debug(char *value, int nvalue, char **rv)
{
	char	*str1, *str2;
	char	 *copy;
	char	*nv = NULL;

	if (rv)
		*rv = NULL;

	if  (!value)
		return 0;

	copy = alloca(strlen(value) + 1);
	strcpy(copy, value);
	
	while ((str1 = new_next_arg(copy, &copy)))
	{
		while (*str1 && (str2 = next_in_comma_list(str1, &str1)))
		{
			if (!my_strnicmp(str2, "ALL", 3))
				nvalue = (0x7F);
			else if (!my_strnicmp(str2, "-ALL", 4))
				nvalue = 0;
			else if (!my_strnicmp(str2, "COMMANDS", 4))
				nvalue |= DEBUG_COMMANDS;
			else if (!my_strnicmp(str2, "-COMMANDS", 4))
				nvalue &= ~(DEBUG_COMMANDS);
			else if (!my_strnicmp(str2, "EXPANSIONS", 4))
				nvalue |= DEBUG_EXPANSIONS;
			else if (!my_strnicmp(str2, "-EXPANSIONS", 4))
				nvalue &= ~(DEBUG_EXPANSIONS);
			else if (!my_strnicmp(str2, "ALIAS", 3))
				nvalue |= DEBUG_CMDALIAS;
			else if (!my_strnicmp(str2, "-ALIAS", 3))
				nvalue &= ~(DEBUG_CMDALIAS);
			else if (!my_strnicmp(str2, "HOOK", 3))
				nvalue |= DEBUG_HOOK;
			else if (!my_strnicmp(str2, "-HOOK", 3))
				nvalue &= ~(DEBUG_HOOK);
			else if (!my_strnicmp(str2, "VARIABLES", 3))
				nvalue |= DEBUG_VARIABLE;
			else if (!my_strnicmp(str2, "-VARIABLES", 3))
				nvalue &= ~(DEBUG_VARIABLE);
			else if (!my_strnicmp(str2, "FUNCTIONS", 3))
				nvalue |= DEBUG_FUNC;
			else if (!my_strnicmp(str2, "-FUNCTIONS", 3))
				nvalue &= ~(DEBUG_FUNC);
			else if (!my_strnicmp(str2, "SERVER", 3))
				nvalue |= DEBUG_SERVER;
			else if (!my_strnicmp(str2, "-SERVER", 3))
				nvalue &= ~(DEBUG_SERVER);
		}
	}
	if (rv)
	{
		if (nvalue & DEBUG_COMMANDS)
			m_s3cat(&nv, comma, "COMMANDS");
		if (nvalue & DEBUG_EXPANSIONS)
			m_s3cat(&nv, comma, "EXPANSIONS");
		if (nvalue & DEBUG_CMDALIAS)
			m_s3cat(&nv, comma, "ALIAS");
		if (nvalue & DEBUG_HOOK)
			m_s3cat(&nv, comma, "HOOK");
		if (nvalue & DEBUG_VARIABLE)
			m_s3cat(&nv, comma, "VARIABLES");
		if (nvalue & DEBUG_FUNC)
			m_s3cat(&nv, comma, "FUNCTIONS");
		if (nvalue & DEBUG_SERVER)
			m_s3cat(&nv, comma, "SERVER");
		*rv = nv;
	}
	return nvalue;
}

void debug_window(Window *win, char *value, int unused)
{
	Window	*old_win = win;
	char	*nv = NULL;

	internal_debug = parse_debug(value, internal_debug, &nv);

	if (internal_debug)
	{
		Window *tmp = NULL;
		if (!get_window_by_name("debug") && (tmp = new_window(win->screen)))
		{
			int i;
			malloc_strcpy(&tmp->name, "debug");
			tmp->status.double_status = 0;
			hide_window(tmp);
			tmp->window_level = LOG_DEBUG;
			tmp->absolute_size = 1;
			tmp->skip = 1;
			debugging_window = tmp;
			for (i = 0; i < 3; i++)
			{
				new_free(&tmp->status.line[i].format);
				new_free(&tmp->status.line[i].raw);
				new_free(&tmp->status.line[i].result);
			}
			malloc_strcpy(&tmp->status.line[0].raw, "58,06DEBUG");
			build_status(tmp, "58,06DEBUG", 1);
			update_all_windows();
			set_input_prompt(win, get_string_var(INPUT_PROMPT_VAR), 0);
			cursor_to_input();
			set_screens_current_window(old_win->screen, old_win);
			set_string_var(DEBUG_VAR, nv);
		}
		else
			set_string_var(DEBUG_VAR, nv);
	}
	else
	{
		if ((old_win = get_window_by_name("debug")))
		{
			delete_window(old_win);
			debugging_window = NULL;
			update_all_windows();
			set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
			cursor_to_input();
		}
		set_string_var(DEBUG_VAR, nv);
	}
	new_free(&nv);
}


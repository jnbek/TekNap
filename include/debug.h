/* Copyright (C) 2000 drscholl@users.sourceforge.net
   This is free software distributed under the terms of the
   GNU Public License.  See the file COPYING for details.

   $Id: debug.h,v 1.1.1.1 2001/01/13 11:30:14 edwards Exp $ */

#ifndef debug_h
#define debug_h

#define DEBUG 1
#if DEBUG

#define ALLOC_BYTE 0xAA		/* allocated memory is filled with this value */
#define END_BYTE 0xEE		/* written at the end of each block to detect
				   buffer overrun */
#define FREE_BYTE 0xFF		/* memory is filled with this prior to free */

#include <stdio.h>
#define ASSERT(x) {if(!(x)){printf("assertion failed in %s, line %d: %s\n",__FILE__,__LINE__,#x);}}
#define ASSERT_RETURN_IF_FAIL(x,r) {if(!(x)){printf("assertion failed in %s, line %d: %s\n",__FILE__,__LINE__,#x);return(r);}}

#include <sys/types.h>

#define INIT debug_init
#define FREE(p) debug_free(p,__FILE__,__LINE__)
#define MALLOC(s) debug_malloc(s,__FILE__,__LINE__)
#define REALLOC(p,s) debug_realloc(p,s,__FILE__,__LINE__)
#define CALLOC(n,s) debug_calloc(n,s,__FILE__,__LINE__)
#define STRDUP(s) debug_strdup(s,__FILE__,__LINE__)
#define CLEANUP debug_cleanup
#define VALID(p) debug_valid(p,1)
#define VALID_LEN debug_valid
#define OPENFILE(fd, f) open_file(fd, f, __FILE__,__LINE__)
#define CLOSEFILE(fd) close_file(fd, __FILE__, __LINE__)
#define MEMORY_USED debug_usage()

/* internal functions, DO NOT CALL DIRECTLY -- use the above macros */
void debug_init (void);
void debug_free (void **, const char *, int);
void *debug_malloc (int, const char *, int);
void *debug_calloc (int, int, const char *, int);
void *debug_realloc (void **, int, const char *, int);
char *debug_strdup (const char *, const char *, int);
void debug_cleanup (void);
int debug_valid (void *, int);
int debug_usage (void);

#else

#define INIT()
#define FREE free
#define MALLOC malloc
#define CALLOC calloc
#define REALLOC realloc
#define STRDUP strdup
#define CLEANUP()
#define VALID(p)
#define VALID_LEN(p,l)
#define ASSERT(p)
#define MEMORY_USED -1

#endif /* DEBUG */

void open_file(int, char *, const char *, const int);
void close_file(int, const char *, const int);

extern  unsigned long x_debug;
extern  unsigned long internal_debug;
extern  unsigned long alias_debug;
extern  unsigned int debug_count;
extern  int in_debug_yell;

void    debugyell       (const char *, ...);
void    debug_alias     (char *, int);
void    debug_hook      (char *, int);

#define DEBUG_COMMANDS          0x0001
#define DEBUG_EXPANSIONS        0x0002
#define DEBUG_FUNCTIONS         0x0004
#define DEBUG_CMDALIAS          0x0008
#define DEBUG_HOOK              0x0010
#define DEBUG_VARIABLE          0x0020
#define DEBUG_FUNC              0x0040
#define DEBUG_SERVER		0x0080

#define DEBUG_ALL (DEBUG_COMMANDS|DEBUG_EXPANSIONS|DEBUG_FUNCTIONS\
		  |DEBUG_CMDALIAS|DEBUG_HOOK|DEBUG_VARIABLE|DEBUG_FUNC)


#endif /* debug_h */

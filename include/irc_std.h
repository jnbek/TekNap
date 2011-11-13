/*
 * irc_std.h: header to define things used in all the programs ircii
 * comes with
 *
 * hacked together from various other files by matthew green
 * copyright(c) 1993 
 *
 * See the copyright file, or do a help ircii copyright 
 *
 * @(#)$Id: irc_std.h,v 1.1.1.1 2000/10/16 01:39:24 edwards Exp $
 */

#ifndef __irc_std_h
#define __irc_std_h
#include "defs.h"

/*
 * Everybody needs these ANSI headers...
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>

/*
 * Everybody needs these POSIX headers...
 */
#include <unistd.h>
#include <signal.h>
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/param.h>

/*
 * Everybody needs these INET headers...
 */
#include <sys/stat.h>
#include <sys/file.h>
#include <sys/ioctl.h>
#include <sys/resource.h>

#include <sys/socket.h>
#ifdef WINNT
#include <cygwin/in.h>
#else
#include <netinet/in.h>
#endif
#include <arpa/inet.h>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif


#if defined(HAVE_ENDIAN_H)
#include <endian.h>
#elif defined(HAVE_SYS_ENDIAN_H)
#include <sys/endian.h>
#elif defined(HAVE_MACHINE_ENDIAN_H)
#include <machine/endian.h>
#endif

/*
 * Some systems define tputs, etc in this header
 */
#ifdef HAVE_TERMCAP_H
#include <termcap.h>
#endif

#ifdef USING_CURSES
#ifdef HPUX
#define _XOPEN_SOURCE_EXTENDED
#endif
#include <curses.h>
#endif

#ifdef USING_NCURSES
#include <ncurses.h>
#endif


#if !defined(HAVE_TERMCAP_H) && !defined(USING_CURSES) && !defined(HAVE_TERMINFO) && !defined(HAVE_TERM_H)
int tputs(const unsigned char *, int, int (*)(int));
#endif
/*
 * Deal with brokenness in <time.h> and <sys/time.h>
 */
#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

/*
 * Deal with brokenness in <fcntl.h> and <sys/fcntl.h>
 */
#ifdef HAVE_SYS_FCNTL_H
# include <sys/fcntl.h>
#else
# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# endif
#endif

/*
 * Deal with brokenness figuring out struct direct
 */
#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif


/*
 * First try to figure out if we can use GNU CC special features...
 */
#ifndef __GNUC__
# define __inline		/* delete gcc keyword */
# define __A(x)
# define __N
# define __inline__
#else
# if (__GNUC__ >= 2) && (__GNUC_MINOR__ >= 7)
#  define __A(x) __attribute__ ((format (printf, x, x + 1)))
#  define __N    __attribute__ ((noreturn))
# else
#  define __A(x)
#  define __N
#  define __inline
# endif
#endif

/*
 * Figure out how to make alloca work
 * I took this from the autoconf documentation
 */
#if defined(__GNUC__) && !defined(HAVE_ALLOCA_H)
# define alloca __builtin_alloca
#else
# if HAVE_ALLOCA_H
#  include <alloca.h>
# else
#  ifdef _AIX
 #pragma alloca
#  else
#   ifndef alloca
char *alloca();
#   endif
#  endif
# endif
#endif


# include <errno.h>
extern	int	errno;

/*
 * Some systems (AIX) have sys/select.h, but dont include it from sys/types.h
 * Some systems (Solaris) have sys/select.h, but include it from sys/types.h
 * and dont want you to do it again.  Some systems dont have sys/select.h
 * Configure has this all figured out for us already.
 */
#if defined(HAVE_SYS_SELECT_H) && defined(NEED_SYS_SELECT_H)
#include <sys/select.h>
#endif

#ifdef SCO
#include <fcntl.h>
#include <strings.h>
#endif

#ifndef WINNT

#ifndef NBBY
# define NBBY	8		/* number of bits in a byte */
#endif /* NBBY */

#ifndef NFDBITS
# define NFDBITS	(sizeof(long) * NBBY)	/* bits per mask */
#endif /* NFDBITS */

#ifndef	FD_SETSIZE
#define FD_SETSIZE	256
#endif

#endif

#ifndef howmany
#define howmany(x, y)   (((x) + ((y) - 1)) / (y))
#endif

#ifdef HAVE_SYS_SYSLIMITS_H
# include <sys/syslimits.h>
#endif
   
#include <limits.h>
   
typedef RETSIGTYPE sigfunc (int);
sigfunc *my_signal (int, sigfunc *, int);

#define SIGNAL_HANDLER(x) \
	RETSIGTYPE x (int unused)


#include <stdlib.h>
#define index strchr



#ifndef MAXPATHLEN
#ifndef PATHSIZE
#define PATHSIZE 1024
#endif
#define MAXPATHLEN  PATHSIZE
#endif

/*
 * Dont trust anyone else's NULLs.
 */
#ifdef NULL
#undef NULL
#endif
#define NULL (void *) 0



#ifndef HAVE_STRERROR
#ifndef SYS_ERRLIST_DECLARED
extern  char    *sys_errlist[];
#endif
#define strerror(x) (char *)sys_errlist[x]
#endif

#if !defined(HAVE_GETTIMEOFDAY) && defined(HAVE_SYS_TIME_H)
extern	int	gettimeofday(struct timeval *tv, struct timezone *tz);
#endif

/* we need an unsigned 32 bit integer for dcc, how lame */

#ifdef UNSIGNED_LONG32

typedef		unsigned long		u_32int_t;

#else
# ifdef UNSIGNED_INT32

typedef		unsigned int		u_32int_t;

# else

typedef		unsigned long		u_32int_t;

# endif /* UNSIGNED_INT32 */
#endif /* UNSIGNED_LONG32 */

#define BUILT_IN_COMMAND(x) \
	void x (char *command, char *args, char *subargs, char *helparg, unsigned int numeric)

#define BUILT_IN_FUNCTION(x) \
	char * x (char *fn, char *input)

#if defined(_AIX)
int getpeername (int s, struct sockaddr *, int *);
int getsockname (int s, struct sockaddr *, int *);
int socket (int, int, int);
int bind (int, struct sockaddr *, int);
int listen (int, int);
int accept (int, struct sockaddr *, int *);
int recv (int, void *, int, unsigned int);
int send (int, void *, int, unsigned int);
int gettimeofday (struct timeval *, struct timezone *);
int gethostname (char *, int);
int setsockopt (int, int, int, void *, int);
int setitimer (int, struct itimerval *, struct itimerval *);
int ioctl (int, int, ...);

#endif


#ifdef __EMX__
#define strcasecmp stricmp
#define strncasecmp strnicmp
#endif

#ifndef HAVE_MEMMOVE
void *memmove(char *, const char *, register size_t);
#endif

#if defined(_SYS_SIGLIST_DECLARED) && !defined(SYS_SIGLIST_DECLARED)
#define sys_siglist _sys_siglist
#endif

#ifndef PATH_MAX
#define PATH_MAX 255
#endif

#ifdef THREAD
#include <pthread.h>

#if defined(MUTEX_INITIALIZER) && !defined(PTHREAD_MUTEX_INITIALIZER)
#define PTHREAD_MUTEX_INITIALIZER MUTEX_INITIALIZER
#endif

#endif

#if !defined(__BYTE_ORDER) 
#if defined(BYTE_ORDER)
#define __BYTE_ORDER BYTE_ORDER
#else 
#if defined(WORDS_BIGENDIAN)
#define __BYTE_ORDER 1
#else
#define __BYTE_ORDER 0
#endif
#endif
#endif

#if !defined(__BIG_ENDIAN) 
#if defined(BIG_ENDIAN)
#define __BIG_ENDIAN BIG_ENDIAN
#else 
#define __BIG_ENDIAN 1
#endif
#endif

#if defined(WORDS_BIGENDIAN)
#ifndef __BIG_ENDIAN
#define __BIG_ENDIAN 1
#endif
#ifndef __BYTE_ORDER
#define __BYTE_ORDER 1
#endif
#endif

#if __BYTE_ORDER == __BIG_ENDIAN
#define BSWAP16(c) (((c & 0xff) << 8) | ((c >> 8) & 0xff))
#define BSWAP32(c) (((c >> 24) & 0xff) | ((c & 0xff0000) >> 8) | ((c & 0xff00) << 8) | (c << 24))
#else
#define BSWAP16(c) c
#define BSWAP32(c) c
#endif

#ifndef O_BINARY
#define O_BINARY 0 
#endif
#ifndef O_TEXT
#define O_TEXT 1
#endif

#endif /* __irc_std_h */


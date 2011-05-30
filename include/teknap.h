 /* $Id: teknap.h,v 1.1.1.1 2001/01/31 17:53:23 edwards Exp $ */
 
/*
 * teknap.h: header file.
 *
 */

#ifndef __irc_h
#define __irc_h

#define TEKNAP 1

#if defined(WINNT) || defined(__EMX__)
#define IRCRC_NAME "/teknap.rc"
#else 
#define IRCRC_NAME "/.teknaprc"
#endif

extern const char nap_version[];
extern const char internal_version[];
extern char	*thing_ansi;
extern char	thing_star[4];


#define QUOTE_CHAR '\\'

#include "defs.h"
#include "config.h"
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>

#include <signal.h>
#include <sys/param.h>

#ifdef __EMX__
#ifdef __EMXPM__
#define AVIO_BUFFER 2048
#define INCL_GPI
#define INCL_AVIO
#define INCL_DOS
#endif
#define INCL_WIN       /* Window Manager Functions     */
#define INCL_BASE
#define INCL_VIO
#include <os2.h>
#elif defined(GTK)
#include <gtk/gtk.h>
#include <gtk/gtkmenu.h>
#endif

#ifdef WINNT
#include <sys/socket.h>
#include <cygwin/in.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
# ifdef HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif /* HAVE_SYS_TIME_H */
#endif /* TIME_WITH_SYS_TIME */

#ifdef HAVE_SYS_FCNTL_H
# include <sys/fcntl.h>
#else
  #ifdef HAVE_FCNTL_H
  #include <fcntl.h> 
  #endif /* HAVE_FCNTL_H */
#endif

#include <stdarg.h>
#include <unistd.h>

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

#ifdef HAVE_TERMCAP_H
#include <termcap.h>
#endif

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif


#include "bsdglob.h"


#include "irc_std.h"

/* these define what characters do, inverse, underline, bold and all off */
#define REV_TOG		'\026'		/* ^V */
#define REV_TOG_STR	"\026"
#define UND_TOG		'\037'		/* ^_ */
#define UND_TOG_STR	"\037"
#define BOLD_TOG	'\002'		/* ^B */
#define BOLD_TOG_STR	"\002"
#define ALL_OFF		'\017'		/* ^O */
#define ALL_OFF_STR	"\017"
#define BLINK_TOG	'\006'		/* ^F (think flash) */
#define BLINK_TOG_STR	"\006"
#define ROM_CHAR        '\022'          /* ^R */
#define ROM_CHAR_STR    "\022"
#define ALT_TOG		'\005'		/* ^E (think Extended) */
#define ALT_TOG_STR	"\005"
#define ND_SPACE	'\023'		/* ^S */
#define ND_SPACE_STR	"\023"

#define IRCD_BUFFER_SIZE	512
#define BIG_BUFFER_SIZE		(4 * IRCD_BUFFER_SIZE)
#define MAX_PROTOCOL_SIZE	(IRCD_BUFFER_SIZE - 2)

#ifndef INPUT_BUFFER_SIZE
#define INPUT_BUFFER_SIZE	(IRCD_BUFFER_SIZE - 20)
#endif


#define REFNUM_MAX 10

#ifndef RAND_MAX
#define RAND_MAX 2147483647
#endif

#define NICKNAME_LEN 30
#define NAME_LEN 80
#ifndef PATH_LEN
#define PATH_LEN 1024
#endif

#ifndef MIN
#define MIN(a,b) ((a < b) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) ((a > b) ? (a) : (b))
#endif

/*
 * declared in irc.c 
 */
extern	char	*cut_buffer;
extern	int	irc_port;
extern	char	empty_string[];
extern	char	zero[];
extern	char	one[];
extern	char	on[];
extern	char	off[];
extern	char	space[];
extern	char	space_plus[];
extern	char	space_minus[];
extern	char	dot[];
extern	char	star[];
extern	char	comma[];
extern	char	nickname[];
extern	char	password[];
extern	char	*bircrc_file;
extern	char	hostname[];
extern	char	*my_path;
extern	char	*irc_path;
extern	char	*irc_lib;
extern	char	*args_str;
extern	int	use_input;
extern	time_t	idle_time;
extern	time_t	now;
extern  time_t  start_time;
extern	char	**environ;
extern	int	quick_startup;
extern 	fd_set	readables, writables;
extern	int	inhibit_logging;
extern	int	cpu_saver;
extern	int	nap_port;
extern	char	version[];
extern	int	timeout_select;

int	is_channel (char *);
void	nap_exit (int, char *, char *, ...);
void	beep_em (int);
void	nap_quit (char, char *);
char	get_a_char (void);
void	load_scripts (void);

void	get_line_return (char, char *);
void	get_line (char *, int, void (*)(char, char *));
void	io (const char *);
char *expand_alias(const char *, const char *, int *, char **);
void	check_serverlag(int);

extern	int logfile_line_mangler;

#endif /* __irc_h */

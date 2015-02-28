 /* $Id: ircaux.h,v 1.1.1.1 2000/12/21 06:08:46 edwards Exp $ */
 
/*
 * ircaux.h: header file for ircaux.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 */

#ifndef _IRCAUX_H_
#define _IRCAUX_H_

#include "teknap.h"
#include "irc_std.h"
#include <stdio.h>
#ifdef WANT_TCL
#undef USE_TCLALLOC
#include <tcl.h>
#endif
/*#define DEBUG 1 */
#include "debug.h"

typedef int comp_len_func (char *, char *, int);
typedef int comp_func (char *, char *);

extern unsigned char stricmp_table[];


char *	check_nickname 		(char *);
char *	next_arg 		(char *, char **);
char *	new_next_arg 		(char *, char **);
char *	new_new_next_arg 	(char *, char **, char *);
char *	last_arg 		(char **);
char *	expand_twiddle 		(char *);
char *	upper 			(char *);
char *	lower 			(char *);
char *	sindex			(register char *, char *);
char *	rsindex 		(register char *, char *, char *, int);
char *	path_search 		(char *, char *);
char *	double_quote 		(const char *, const char *, char *);


char *	m_s3cat_s 		(char **, const char *, const char *);
char *	m_s3cat 		(char **, const char *, const char *);
char *	m_3cat 			(char **, const char *, const char *);
char *	m_e3cat 		(char **, const char *, const char *);
char *	m_2dup 			(const char *, const char *);
char *	m_3dup 			(const char *, const char *, const char *);
char *	m_opendup 		(const char *, ...);
char *	malloc_sprintf 		(char **, const char *, ...);
char *	m_sprintf 		(const char *, ...);
int	is_number 		(const char *);
int	is_real_number		(const char *);
char *	my_ctime 		(time_t);

#if 0
#define my_stricmp(x, y) strcasecmp(x, y) /* unable to use these for reasons of case sensitivity and finish */
#define my_strnicmp(x, y, n) strncasecmp(x, y, n)
#else
int	my_stricmp 	(const unsigned char *, const unsigned char *);
int	my_strnicmp	(const unsigned char *, const unsigned char *, size_t);
#endif

int	my_strnstr 		(const unsigned char *, const unsigned char *, size_t);
int	scanstr 		(char *, char *);
void	really_free 		(int);
char *	chop 			(char *, int);
char *	strmcpy 		(char *, const char *, int);
char *	strmcat 		(char *, const char *, int);
char *	strmcat_ue 		(char *, const char *, int);
char *	stristr 		(const char *, const char *);
char *	rstristr 		(char *, char *);

FILE *	uzfopen 		(char **, char *, int, int *);
char *	uzfopen_error		(int);

#define ERR_NOERROR		0
#define ERR_NOGUNZIP		1
#define ERR_NOUNCOMPRESS	2
#define ERR_NOBUNZIP2		3
#define ERR_NOFILE		4
#define ERR_DIRECTORY		5
#define ERR_EXECUTABLE		6
#define ERR_NOUNCOMPRESSOR	7
#define ERR_CANNOTOPEN		8


int	end_strcmp 		(const char *, const char *, int);
void	nappanic		(char *, ...);

int	fw_strcmp 		(comp_len_func *, char *, char *);
int	lw_strcmp 		(comp_func *, char *, char *);
int	open_to 		(char *, int, off_t);
struct timeval get_time 	(struct timeval *);
double 	time_diff 		(struct timeval, struct timeval);
char *	plural 			(int);
int	time_to_next_minute 	(void);
char *	remove_trailing_spaces 	(char *);
char *	ltoa			(long);

int	count_trailing_spaces	(const char*);

char *	strformat 		(char *, const char *, int, char);
char *	chop_word 		(char *);
int	splitw 			(char *, char ***);
char *	unsplitw 		(char ***, int);
int	check_val 		(char *);
char *	strextend 		(char *, char, int);
char *	strext			(const char *, const char *);
char *	pullstr 		(char *, char *);
int 	empty 			(const char *);
char *	safe_new_next_arg 	(char *, char **);
char *	MatchingBracket 	(register char *, register char, register char);
int	word_count 		(const char *);
int	parse_number 		(char **);
char *	remove_brackets 	(const char *, const char *, int *);
u_long	hashpjw 		(char *, u_long);
char *	m_dupchar 		(int);
char *	strmccat		(char *, char, int);
off_t	file_size		(char *);
int	is_root			(char *, char *, int);
size_t	streq			(const char *, const char *);
size_t	strieq			(const char *, const char *);
char *	on_off 			(int);
char *	rfgets			(char *, int, FILE *);
char *  strmopencat             (char *, int, ...);
long 	my_atol			(const char *);
unsigned long 	my_atoul	(const char *);
char *	s_next_arg		(char **);
char *	next_in_comma_list	(char *, char **);
void	strip_control		(const char *, char *);
int	figure_out_address	(char *, char **, char **, char **, char **, int *);
int	count_char		(const unsigned char *, const unsigned char);
char *	strnrchr		(char *, char, int);
void	mask_digits		(char **);
const char *strfill		(char, int);
char *	ov_strcpy		(char *, const char *);
char *	strpcat			(char *, const char *, ...);
char *  strmpcat		(char *, size_t, const char *, ...);
char *	chomp			(char *);
size_t	ccspan			(const char *, int);
u_char *strcpy_nocolorcodes	(u_char *, const u_char *);

u_long	random_number		(u_long);
char *	get_userhost		(void);
const char *	find_backward_quote (const char *, const char *);
const char *	find_forward_quote (const char *, const char *);


/* From words.c */
#define SOS -32767
#define EOS 32767
#if 0
char	*search			(register char *, char **, char *, int);
char	*move_to_abs_word	(const register char *, char **, int, char);
char	*move_word_rel		(const register char *, char **, int, char);
char	*extract		(char *, int, int);
char	*extract2		(const char *, int, int);
#endif
char *		search 			(char *, char **, char *, int);
const char *	real_move_to_abs_word 	(const char *, const char **, int, int);
char *		real_extract 		(char *, int, int, int);
char *		real_extract2 		(const char *, int, int, int);
#define move_to_abs_word(a, b, c)	real_move_to_abs_word(a, b, c, 0);
#define extract(a, b, c)		real_extract(a, b, c, 0)
#define extract2(a, b, c)		real_extract2(a, b, c, 0)
#define extractw(a, b, c)		real_extract(a, b, c, 1)
#define extractw2(a, b, c)		real_extract2(a, b, c, 1)

int	wild_match		(register const unsigned char *, register const unsigned char *);

/* Used for connect_by_number */
#define SERVICE_SERVER 0
#define SERVICE_CLIENT 1
#define PROTOCOL_TCP 0
#define PROTOCOL_UDP 1
#define PROTOCOL_ICMP 2

/* Used from network.c */
int			connect_by_number (char *, unsigned short *, int, int, int);
struct hostent *	resolv (const char *);
struct hostent *	lookup_host (const char *);
struct hostent *	lookup_ip (const char *);
char *			host_to_ip (const char *);
char *			ip_to_host (const char *);
char *			one_to_another (const char *);
int			set_blocking (int);
int			set_non_blocking (int);
int			my_accept (int, struct sockaddr *, int *);
int			lame_resolv (const char *, struct in_addr *);
void			do_nslookup(char *, char *, char *);
int			open_icmp(void);
void			send_ping(int, unsigned long);


#define my_isspace(x) \
	((x) == 9 || (x) == 10 || (x) == 11 || (x) == 12 || (x) == 13 || (x) == 32)
  
#define my_isdigit(x) \
(*x >= '0' && *x <= '9') || \
((*x == '-'  || *x == '+') && (x[1] >= '0' && x[1] <= '9'))

#define LOCAL_COPY(y) strcpy(alloca(strlen((y)) + 1), y)


#define	_1KB	((double) 1000)
#define _1MEG	(_1KB * _1KB)
#define _1GIG	(_1KB * _1KB * _1KB)
#define _1TER	(_1KB * _1KB * _1KB * _1KB)
#define _1ETA	(_1KB * _1KB * _1KB * _1KB * _1KB)

#if 0
#define	_1MEG	(1024.0*1024.0)
#define	_1GIG	(1024.0*1024.0*1024.0)
#define	_1TER	(1024.0*1024.0*1024.0*1024.0)
#define	_1ETA	(1024.0*1024.0*1024.0*1024.0*1024.0)
#endif

#define	_GMKs(x)	( ((double)x > _1ETA) ? "eb" : \
			(((double)x > _1TER) ? "tb" : (((double)x > _1GIG) ? "gb" : \
			(((double)x > _1MEG) ? "mb" : (((double)x > _1KB)? "kb" : "bytes")))))

#define	_GMKv(x)	(((double)x > _1ETA) ? \
			((double)x/_1ETA) : (((double)x > _1TER) ? \
			((double)x/_1TER) : (((double)x > _1GIG) ? \
			((double)x/_1GIG) : (((double)x > _1MEG) ? \
			((double)x/_1MEG) : (((double)x > _1KB) ? \
			((double)x/_1KB): (double)x)))) )


#if 0

void	*n_malloc 	(size_t, const char *, const char *, const int);
void	*n_realloc	(void **, size_t, const char *, const char *, const int);

void	*n_free 	(void **, const char *, const char *, const int);

#define MODULENAME NULL

#define new_malloc(x) n_malloc(x, MODULENAME, __FILE__, __LINE__)
#define new_free(x) n_free((void **)(x), MODULENAME, __FILE__, __LINE__)

#define RESIZE(x, y, z) n_realloc     ((void **)& (x), sizeof(y) * (z), MODULENAME, __FILE__, __LINE__)
#define malloc_strcpy(x, y) n_malloc_strcpy((char **)x, (char *)y, MODULENAME, __FILE__, __LINE__)
#define malloc_strcat(x, y) n_malloc_strcat((char **)x, (char *)y, MODULENAME, __FILE__, __LINE__)
#define m_strdup(x) n_m_strdup(x, MODULENAME, __FILE__, __LINE__)
#define m_strcat_ues(x, y, z) n_m_strcat_ues(x, y, z, MODULENAME, __FILE__, __LINE__)
#define m_strndup(x, y) n_m_strndup(x, y, MODULENAME, __FILE__, __LINE__)

#else

#define new_malloc(x) debug_malloc(x, __FILE__,__LINE__)
#define new_free(x) debug_free((void **)x,__FILE__,__LINE__)
#define RESIZE(x, y, z) n_realloc ((void **)& (x), sizeof(y) * (z), __FILE__, __LINE__)
#define m_strdup(x) debug_strdup(x, __FILE__, __LINE__)

#define malloc_strcpy(x, y) n_malloc_strcpy((char **)x, (char *)y, __FILE__, __LINE__)
#define malloc_strcat(x, y) n_malloc_strcat((char **)x, (char *)y, __FILE__, __LINE__)
#define m_strcat_ues(x, y, z) n_m_strcat_ues(x, y, z, __FILE__, __LINE__)
#define m_strndup(x, y) n_m_strndup(x, y, __FILE__, __LINE__)
#define n_realloc(x, y, z, a) debug_realloc(x, y, z, a)

char *	n_m_strndup		(const char *, const int, const char *, const int);
char *	n_m_strcat_ues(char **, char *, int, const char *, const int);
char *	n_malloc_strcpy		(char **, const char *, const char *, const int);
char *	n_malloc_strcat 	(char **, const char *, const char *, const int);
char *	n_m_strdup 		(const char *, const char *, const int);
#endif



char	*encode			(const char *, int);
char	*decode			(const char *);
char	*cryptit		(const char *);
int	checkpass		(const char *, const char *);


/* Used for the inbound mangling stuff */

#define MANGLE_ESCAPES		1 << 0
#define MANGLE_ANSI_CODES	1 << 1
#define STRIP_COLOR		1 << 2
#define STRIP_REVERSE		1 << 3
#define STRIP_UNDERLINE		1 << 4
#define STRIP_BOLD		1 << 5
#define STRIP_BLINK		1 << 6
#define STRIP_ROM_CHAR          1 << 7
#define STRIP_ND_SPACE          1 << 8
#define STRIP_ALL_OFF		1 << 9
#define STRIP_ALT_CHAR		1 << 10
#define PRE_MANGLE		1 << 11

extern	int     outbound_line_mangler;
extern	int     inbound_line_mangler;
extern	int	logfile_line_mangler;
extern	int	operlog_line_mangler;

void	mangle_line		(char *, int, int);
int	charcount		(const char *, char);
char	*stripdev		(char *);
char	*convert_dos		(char *);
char	*convert_unix		(char *);
int	is_dos			(char *);
void	strip_chars		(char *, char *, char);
char	*longcomma		(long);
char	*ulongcomma		(unsigned long);
char	*slongcomma		(char *);

void compute_soundex (char *, int, const char *);


#define SAFE(x) (((x) && *(x)) ? (x) : empty_string)

/* Used in compat.c */
#ifndef HAVE_TPARM
	char 	*tparm1 (const char *, ...);
#endif

#ifndef HAVE_STRTOUL
	unsigned long 	strtoul (const char *, char **, int);
#endif

	char *	bsd_getenv (const char *);
	int	bsd_putenv (const char *);
	int	bsd_setenv (const char *, const char *, int);
	void	bsd_unsetenv (const char *);

#ifndef HAVE_INET_ATON
#if !defined(__EMX__) && !defined(WINNT)
	int	inet_aton (const char *, struct in_addr *);
#endif
#endif

#ifndef HAVE_STRLCPY
	size_t	strlcpy (char *, const char *, size_t);
#endif

#ifndef HAVE_STRLCAT
	size_t	strlcat (char *, const char *, size_t);
#endif

#ifndef HAVE_VSNPRINTF
	int	vsnprintf (char *, size_t, const char *, va_list);
#endif

#ifndef HAVE_SNPRINTF
	int	snprintf (char *, size_t, const char *, ...);
#endif

#ifndef HAVE_SETSID
	int	setsid (void);
#endif

int matchnumber(char *, int);
char *off_on(int);

void my_decrypt(char *, int, char *);
void my_encrypt (char *, int, char *);

#endif /* _IRCAUX_H_ */

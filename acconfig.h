/* Fix broken pthread.h
#undef __USE_EXTERN_INLINES
/* Define short ircii version here */
#undef _VERSION_

/* Define long ircii version here */
#undef VERSION

/* Define this if your compiler has "const" */
#undef HAVE_CONST

/* Define this for DEC 3.2 threads */
#undef HAVE_DEC_3_2_THREADS

/* Define this for DEC threads */
#undef HAVE_DEC_THREADS

/* Define this for Linux threads */
#undef HAVE_LINUXTHREADS

/* define this if pthread has pthread_testcancel */
#undef HAVE_PTHREAD_TESTCANCEL

/* define this is pthread has pthread_cancel */
#undef HAVE_PTHREAD_CANCEL

/* Define this if you have hpux version 7 */
#undef HPUX7

/* Define this if you have hpux version 8 */
#undef HPUX8

/* Define this if you have an unknown hpux version (pre ver 7) */
#undef HPUXUNKNOWN

/* Define this for a SCO system */
#undef SCO

/* Define this if you have POSIX.1 */
#undef POSIX

/* Define this if you are using -lcurses */
#undef USING_CURSES  

/* Define this if you are using -lncurses */
#undef USING_NCURSES

/* Define this for panel lib */
#undef HAVE_PANELLIB

/* Define this for Unixware 7 */
#undef HAVE_UNIXWARE7_POSIX
 
/* Define this for Unixware 7 threads */
#undef HAVE_UNIXWARE7_THREADS
 
/* Define this for IPv6 */
#undef IPV6

/* Define this if termcap.h defines tputs(3) */
#undef HAVE_TERMCAP_H

/* Define this if term.h defines tputs(3) */
#undef HAVE_TERM_H

/* Define this if you have tparm() */
#undef HAVE_TPARM

/* Define this if you have strtoul(3) */
#undef HAVE_STRTOUL

/* Define this if you have inet_aton(2) */
#undef HAVE_INET_ATON

/* Define this if you have strlcpy(3) */
#undef HAVE_STRLCPY

/* Define this if you have strlcat(3) */
#undef HAVE_STRLCAT

/* Define this if you have vsnprintf(3) */
#undef HAVE_VSNPRINTF

/* Define this if you have snprintf(3) */
#undef HAVE_SNPRINTF

/* Define this if you have setsid(2) */
#undef HAVE_SETSID

/* Define this if you have the terminfo database */
#undef HAVE_TERMINFO

/* Define this if you have strerror(3) */
#undef HAVE_STRERROR

/* Define this if you have memmove(3) */
#undef HAVE_MEMMOVE

/* Define this if you have gettimeofday(2) */
#undef HAVE_GETTIMEOFDAY

/* Define this if you have uname(2) */
#undef HAVE_UNAME

/* Define this if you have getrusage(2) */
#undef HAVE_GETRUSAGE

/* Define this if you have scandir(5) */
#undef HAVE_SCANDIR

/* Define this if you have sysconf(1) */
#undef HAVE_SYSCONF

/* Define this if you have getpgid(1) */
#undef HAVE_GETPGID

/* Define this if you have killpg(2) */
#undef HAVE_KILLPG

/* Define this if you have getlogin(0) */
#undef HAVE_GETLOGIN

/* Define this if you have realpath(2) */
#undef HAVE_REALPATH

/* Define this if you have fchdir(1) */
#undef HAVE_FCHDIR

/* Define this if you have setvbuf(4) */
#undef HAVE_SETVBUF

/* Define this if you have getpass(1) */
#undef HAVE_GETPASS

/* Define this if you have fpathconf(2) */
#undef HAVE_FPATHCONF

/* Define this if you have mlock(2) */
#undef HAVE_MLOCK

/* Define this if you are using the MPEG audio decoder library (libmad) */
#undef HAVE_LIBMAD

/* define this if you are on svr3/twg */
#undef WINS

/* Define this if you need sys/select.h */
#undef NEED_SYS_SELECT_H

/* Define this if you don't have struct linger */
#undef NO_STRUCT_LINGER

/* Define this if you have SUN_LEN in <sys/un.h> */
#undef HAVE_SUN_LEN

/* Define this if your getpgrp() is broken */
#undef GETPGRP_VOID

/* Define this if you are using BSD wait union things */
#undef BSDWAIT

/* Define this if waitpid() is unavailable */
#undef NEED_WAITPID

/* Define this if you have non-blocking POSIX signals */
#undef NBLOCK_POSIX

/* Define this if you have non-blocking BSD signals */
#undef NBLOCK_BSD

/* Define this if you have non-blocking SYSV signals */
#undef NBLOCK_SYSV

/* Define if sys_siglist is declared */
#undef SYS_SIGLIST_DECLARED

/* Define this if you are using sigaction() instead of signal() */
#undef USE_SIGACTION

/* Define this if you are using sigset() instead of signal() */
#undef USE_SIGSET

/* Define this if you are using system V (unreliable) signals */
#undef SYSVSIGNALS

/* Define this if wait3() is declared */
#undef WAIT3_DECLARED

/* Define this if waitpid() is declared */
#undef WAITPID_DECLARED

/* Define this if errno is declared */
#undef ERRNO_DECLARED

/* Define this if an unsigned long is 32 bits */
#undef UNSIGNED_LONG32

/* Define this if an unsigned int is 32 bits */
#undef UNSIGNED_INT32

/* Define this if you are unsure what is 32 bits */
#undef UNKNOWN_32INT

/* Define a list of default servers here */
#undef DEFAULT_SERVER

/* Define this for MP3 player support */
#undef WANT_MP3PLAYER

/* Define this for thread support */
#undef WANT_THREAD
 
/* Define this for XMMS support */
#undef WANT_XMMS

/* Define this for Win32 GUI support */
#undef WIN32_GUI

/* Define this for <sys/siglist.h> */
#undef _SYS_SIGLIST_DECLARED

/* Define this if compiling with SOCKS (the firewall traversal library).
   Also, you must define connect, getsockname, bind, accept, listen, and
   select to their R-versions. */
#undef SOCKS
#undef SOCKS4
#undef SOCKS5
#undef connect
#undef getsockname
#undef bind
#undef accept
#undef listen
#undef select
#undef dup
#undef dup2
#undef fclose
#undef gethostbyname
#undef getpeername
#undef read
#undef recv
#undef recvfrom
#undef rresvport
#undef send
#undef sendto
#undef shutdown
#undef write

/*
 * Are we doing non-blocking connects?  Note:  SOCKS support precludes
 * us from using this feature.
 */
#if (defined(NBLOCK_POSIX) || defined(NBLOCK_BSD) || defined(NBLOCK_SYSV)) && !defined(SOCKS)
#define NON_BLOCKING_CONNECTS
#endif

/* Define this if you want CD-ROM support */
#undef WANT_CD

/* Define this if you want GUI support (PM or GTK) */
#undef GUI

/* Define this is you want OS/2 PM support */
#undef __EMXPM__

/* Define this if you want imlib support */
#undef USE_IMLIB

/* Define this if you want GNOME support */
#undef USE_GNOME

/* Define this if your ZVT is newer than 1.0.10 */
#undef HAVE_NEW_ZVT

/* Define this if you want ZVT support */
#undef USE_ZVT

/* Define this if you want GTK support */
#undef GTK

/* Define this is you want sound support */
#undef SOUND

/* Define this is you have getpwent(2) */
#undef HAVE_GETPWENT


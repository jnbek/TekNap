/*
 * Various config options for the package TekNap.
 * Copyright Colten Edwards Feb 14/2000
 */
 /* $Id: config.h,v 1.1.1.1 2000/12/27 03:43:27 edwards Exp $ */
 
#ifndef __config_h_
#define __config_h_


#define ON 1
#define OFF 0

/*
 * Set your favorite default server list here.  This list should be a
 * whitespace separated hostname:portnum:password list (with portnums and
 * passwords optional).  This IS NOT an optional definition. Please set this
 * to your nearest servers.  However if you use a seperate 'ircII.servers'
 * file and the ircII can find it, this setting is overridden.
 */

#ifndef DEFAULT_SERVER
/* 
 * some caution is required here. the \ is a continuation char and is required
 * on any servers you add into this list. also the very last server should not 
 * have a continuation char.
 */
#define DEFAULT_SERVER  "bitchx.dimension6.com:8875:::1 "\
			"server.napster.com:8875:::1"
#endif

/*
 * You must always define this. If you can't compile glob.c, let us know.
 */
#define NEED_GLOB

#define INCLUDE_GLOB_FUNCTION

#define NAP_PORT 8875

/* 
 * If you define ONLY_STD_CHARS, only "normal" characters will displayed.
 * This is recommended when you want to start BitchX in an xterm without
 * the usage of the special "vga"-font. 
 */
#undef ONLY_STD_CHARS

#define DEFAULT_MSGLOG ON


#define DEFAULT_ALT_CHARSET ON
#define DEFAULT_ALWAYS_SPLIT_BIGGEST ON
#define DEFAULT_BANTIME 600
#define DEFAULT_BEEP ON
#define DEFAULT_BEEP_MAX 3
#define DEFAULT_BEEP_WHEN_AWAY OFF
#define DEFAULT_BOLD_VIDEO ON
#define DEFAULT_BLINK_VIDEO ON
#define DEFAULT_CHANNEL_NAME_WIDTH 15
#define DEFAULT_CLOCK ON
#define DEFAULT_CLOCK_24HOUR OFF
#define DEFAULT_COMMAND_MODE OFF
#define DEFAULT_DISPLAY ON
#define DEFAULT_EIGHT_BIT_CHARACTERS ON
#define DEFAULT_FULL_STATUS_LINE ON
#define DEFAULT_HIGH_BIT_ESCAPE OFF
#define DEFAULT_HISTORY 100
#define DEFAULT_HOLD_MODE OFF
#define DEFAULT_HOLD_MODE_MAX 0
#define DEFAULT_INDENT ON
#define DEFAULT_INPUT_ALIASES OFF
#define DEFAULT_INSERT_MODE ON
#define DEFAULT_INVERSE_VIDEO ON
#define DEFAULT_LASTLOG 1000

#define DEFAULT_LOG OFF
#define DEFAULT_SCROLL_LINES ON
#define DEFAULT_SHELL_LIMIT 0
#define DEFAULT_SHOW_AWAY_ONCE ON
#define DEFAULT_SHOW_CHANNEL_NAMES ON
#define DEFAULT_SHOW_END_OF_MSGS OFF
#define DEFAULT_SHOW_NUMERICS OFF
#define DEFAULT_SHOW_STATUS_ALL OFF
#define DEFAULT_SHOW_WHO_HOPCOUNT OFF


#define DEFAULT_META_STATES 5
#define DEFAULT_IGNORE_TIME 600
#define DEFAULT_AUTO_RECONNECT ON

#define DEFAULT_DOUBLE_STATUS_LINE ON

#define DEFAULT_NICK_COMPLETION ON
#define DEFAULT_NICK_COMPLETION_LEN 2
#define DEFAULT_NICK_COMPLETION_TYPE 0  /* 0 1 2 */

#define DEFAULT_SUPPRESS_SERVER_MOTD ON
#define DEFAULT_TAB ON
#define DEFAULT_TAB_MAX 8

#define DEFAULT_UNDERLINE_VIDEO ON

#define DEFAULT_DISPLAY_ANSI ON
#define DEFAULT_DISPLAY_PC_CHARACTERS 4

#define DEFAULT_CONNECT_TIMEOUT 30
#define DEFAULT_STATUS_NO_REPEAT ON
#define DEFAULT_STATUS_DOES_EXPANDOS OFF
#define DEFAULT_SCROLLBACK_LINES 512
#define DEFAULT_SCROLLBACK_RATIO 50

#define DEFAULT_ND_SPACE_MAX 160

#define DEFAULT_CPU_SAVER_AFTER 0
#define DEFAULT_CPU_SAVER_EVERY 0

/* #define HUMBLE	ON */		/* define this for a hades look */



#define DEFAULT_BEEP_ON_MSG "MSGS"
#define DEFAULT_CMDCHARS "/"
#define DEFAULT_CONTINUED_LINE "          "
#define DEFAULT_HIGHLIGHT_CHAR "INVERSE"
#define DEFAULT_LASTLOG_LEVEL "ALL"
#define DEFAULT_MSGLOG_LEVEL "MSGS NOTICES SEND_MSG"
#define DEFAULT_LOGFILE "NapLog"
#define DEFAULT_SHELL "/bin/sh"
#define DEFAULT_SHELL_FLAGS "-c"

#define DEFAULT_PAD_CHAR ' '
#define DEFAULT_WORD_BREAK ",; \t"
#define DEFAULT_DOWNLOAD_DIRECTORY "~"
#define DEFAULT_ILLEGAL_CHARS "|<>"

#define DEFAULT_HELP_WINDOW OFF
#define DEFAULT_HELP_PAGER ON
#define DEFAULT_HELP_PROMPT ON

#define DEFAULT_RANDOM_LOCAL_PORTS 0
#define DEFAULT_RANDOM_SOURCE 0
#define DEFAULT_TERM_DOES_BRIGHT_BLINK 0

#define DEFAULT_FLOOD_AFTER	5
#define DEFAULT_FLOOD_RATE	4
#define DEFAULT_FLOOD_USERS	10
#define DEFAULT_PING_SYSTEM	1


#define WANT_NSLOOKUP

#define WANT_PTEST		/* does a port test on Notification. */
#define WANT_FASTSHARE		/* threaded sharing code */

#ifndef WINNT
#define SCOTT			/* This enables a file browser for the /browse
				   and /search commands */
#define DEFAULT_GTK ON

#else
#undef SCOTT
#define DEFAULT_GTK OFF
#undef WANT_NSLOOKUP
#undef WANT_PTEST
#endif

#if !defined(HAVE_LINUXTHREADS) && !defined(HAVE_DEC_THREADS) &&\
	!defined(HAVE_DEC_3_3_THREADS) && !defined(HAVE_UNIXWARE7_THREADS) &&\
		!defined(HAVE_UNIXWARE7_POSIX) && !defined(WANT_THREAD)
#	if defined(THREAD)
		#undef THREAD
#	endif
#else
#	define THREAD
#endif

#undef OFF
#undef ON

#include "color.h"		/* all color options here. */

#endif /* __config_h_ */

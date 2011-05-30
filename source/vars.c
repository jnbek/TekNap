/*
 * vars.c: All the dealing of the irc variables are handled here. 
 *
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 * $Id: vars.c,v 1.2 2001/07/08 21:33:57 edwards Exp $
 */


#include "teknap.h"
#include "struct.h"

#include "debug.h"
#include "status.h"
#include "window.h"
#include "lastlog.h"
#include "history.h"
#include "napster.h"
#include "log.h"
#include "vars.h"
#include "input.h"
#include "ircaux.h"
#include "ircterm.h"
#include "output.h"
#include "screen.h"
#include "server.h"
#include "stack.h"
#include "list.h"
#include "window.h"
#include "alist.h"
#include "ptest.h"


#ifdef WANT_CD
#include "cdrom.h"
#endif

#if defined(WINNT)
#include <windows.h>
extern DWORD gdwPlatform;
#endif


char	*var_settings[] =
{
	"OFF", "ON", "TOGGLE"
};


extern	char	*auto_str;

extern  Screen	*screen_list;

int	loading_global = 0;


enum VAR_TYPES	find_variable (char *, int *);
static	void	eight_bit_characters (Window *, char *, int);
static	void	set_numeric_string (Window *, char *, int);
static	void	reinit_screen (Window *, char *, int);
	void	reinit_status (Window *, char *, int);
static	void	set_clock_format (Window *, char *, int);
	void	toggle_reverse_status(Window *, char *, int);
	void	set_scrollback_size (Window *, char *, int);
	void	set_cd_device(Window *, char *, int);
static	void	set_hold_interval (Window *, char *, int);

static	void	set_speed(Window *, char *, int);
static	void	set_all_dataport(Window *, char *, int);
static	void	set_ptest_timeout(Window *, char *, int);
static	void	set_ptest_connect(Window *, char *, int);
static	void	set_email_address(Window *, char *, int);
static	void	set_mangle_logfiles(Window *, char *, int);

extern	void	debug_window(Window *, char *, int);
extern	void	save_ignore(FILE *);

char *_n_speed[] = 
	{"?", "14.4", "28.8", "33.6", "56k", "64k ISDN", "128k ISDN", "Cable", "DSL", "T1", "T3 >", 0};
		
/*
 * irc_variable: all the irc variables used.  Note that the integer and
 * boolean defaults are set here, which the string default value are set in
 * the init_variables() procedure 
 */

static	IrcVariable irc_variable[] =
{
	{ "ALLOW_DCC",0,		BOOL_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "ALLOW_DCC_OVERWRITE",0,	BOOL_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "ALT_CHARSET", 0,		BOOL_TYPE_VAR,	DEFAULT_ALT_CHARSET, NULL, NULL, 0, VIF_BXNAP },
	{ "ALWAYS_SPLIT_BIGGEST",0,	BOOL_TYPE_VAR,	DEFAULT_ALWAYS_SPLIT_BIGGEST, NULL, NULL, 0, 0 },
	{ "APPEND_LOG",0,		BOOL_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "AUTO_RECONNECT",0,		BOOL_TYPE_VAR,	DEFAULT_AUTO_RECONNECT, NULL, NULL, 0, VIF_BXNAP },
	{ "AUTO_REJOIN",0,		BOOL_TYPE_VAR,   1, NULL, NULL, 0, VIF_BXNAP  },
	{ "AUTO_SHARE", 0,		BOOL_TYPE_VAR,	1, NULL, NULL, 0, VIF_BXNAP },
	{ "BEEP",0,			BOOL_TYPE_VAR,	DEFAULT_BEEP, NULL, NULL, 0, VIF_BXNAP },
	{ "BEEP_ALWAYS",0,		BOOL_TYPE_VAR,	0, NULL, NULL, 0, VIF_BXNAP },
	{ "BEEP_MAX",0,			INT_TYPE_VAR,	DEFAULT_BEEP_MAX, NULL, NULL, 0, 0 },
	{ "BEEP_ON_MSG",0,		STR_TYPE_VAR,	0, NULL, set_beep_on_msg, 0, VIF_BXNAP },
	{ "BEEP_WHEN_AWAY",0,		INT_TYPE_VAR,	DEFAULT_BEEP_WHEN_AWAY, NULL, NULL, 0, VIF_BXNAP },
	{ "BLINK_VIDEO",0,		BOOL_TYPE_VAR,	DEFAULT_BLINK_VIDEO, NULL, NULL, 0, 0 },
	{ "BOLD_VIDEO",0,		BOOL_TYPE_VAR,	DEFAULT_BOLD_VIDEO, NULL, NULL, 0, 0 },
	{ "CD_DEVICE",0,		STR_TYPE_VAR,	0, NULL, set_cd_device, 0, VIF_BXNAP },
	{ "CHANNEL_NAME_WIDTH",0,	INT_TYPE_VAR,	DEFAULT_CHANNEL_NAME_WIDTH, NULL, update_all_status, 0, 0 },
	{ "CLOCK",0,			BOOL_TYPE_VAR,	DEFAULT_CLOCK, NULL, update_all_status, 0, 0 },
	{ "CLOCK_24HOUR",0,		BOOL_TYPE_VAR,	DEFAULT_CLOCK_24HOUR, NULL, reset_clock, 0, 0 },
	{ "CLOCK_FORMAT",0,		STR_TYPE_VAR,	0, NULL, set_clock_format, 0, 0 },
	{ "CMDCHARS",0,			STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "COLOR",0,			BOOL_TYPE_VAR,	1, NULL, NULL, 0, 0 },
	{ "COMMAND_MODE",0,		BOOL_TYPE_VAR,	DEFAULT_COMMAND_MODE, NULL, NULL, 0, 0 },
	{ "CONNECT_TIMEOUT",0,		INT_TYPE_VAR,	DEFAULT_CONNECT_TIMEOUT, NULL, NULL, 0, 0 },
	{ "CONTINUED_LINE",0,		STR_TYPE_VAR,	0, NULL, set_continued_lines, 0, 0 },
	{ "CPU_SAVER_AFTER",0,		INT_TYPE_VAR,	DEFAULT_CPU_SAVER_AFTER, NULL, NULL, 0, VIF_BXNAP },
	{ "CPU_SAVER_EVERY",0,		INT_TYPE_VAR,	DEFAULT_CPU_SAVER_EVERY, NULL, NULL, 0, VIF_BXNAP },
	{ "DEBUG",0,			STR_TYPE_VAR,	0, NULL, debug_window, 0, 0 },
	{ "DEFAULT_DATAPORT",0,		INT_TYPE_VAR,	6699, NULL, set_all_dataport, 0, VIF_BXNAP },
	{ "DEFAULT_DIRECT_BROWSE",0,	BOOL_TYPE_VAR,	1, NULL, NULL, 0, VIF_BXNAP },
	{ "DEFAULT_EMAIL",0,		STR_TYPE_VAR,	0, NULL, set_email_address, 0, VIF_BXNAP },
	{ "DEFAULT_METASERVER",0,	INT_TYPE_VAR,	8875, NULL, NULL, 0, VIF_BXNAP },
	{ "DEFAULT_NICKNAME",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BXNAP },
	{ "DEFAULT_PASSWORD",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BXNAP },
	{ "DEFAULT_SERVER",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BXNAP },
	{ "DEFAULT_SPEED",0,		STR_TYPE_VAR,	0, NULL, set_speed, 0, 0 },
	{ "DISPLAY",0,			BOOL_TYPE_VAR,	DEFAULT_DISPLAY, NULL, NULL, 0, 0 },
	{ "DISPLAY_ANSI",0,		BOOL_TYPE_VAR,	DEFAULT_DISPLAY_ANSI, NULL, toggle_reverse_status, 0, 0 },
	{ "DISPLAY_PC_CHARACTERS",0,	INT_TYPE_VAR,	DEFAULT_DISPLAY_PC_CHARACTERS, NULL, reinit_screen, 0, VIF_BXNAP },
	{ "DOUBLE_STATUS_LINE",0,	BOOL_TYPE_VAR,	DEFAULT_DOUBLE_STATUS_LINE, NULL, reinit_status, 0, VIF_BXNAP },
	{ "DOWNLOAD_DIRECTORY",0,	STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "EIGHT_BIT_CHARACTERS",0,	BOOL_TYPE_VAR,	DEFAULT_EIGHT_BIT_CHARACTERS, NULL, eight_bit_characters, 0, VIF_BXNAP },
	{ "FLOATING_POINT",0,		INT_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "FLOOD_AFTER",0,		INT_TYPE_VAR,	DEFAULT_FLOOD_AFTER, NULL, NULL, 0, 0 },
	{ "FLOOD_RATE",0,		INT_TYPE_VAR,	DEFAULT_FLOOD_RATE, NULL, NULL, 0, 0 },
	{ "FLOOD_USERS",0,		INT_TYPE_VAR,	DEFAULT_FLOOD_USERS, NULL, NULL, 0, 0 },
	{ "FLOOD_WARNING",0,		BOOL_TYPE_VAR,	1, NULL, NULL, 0, 0 },
	{ "FORMAT_DIRECTORY",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "FORMAT_FILENAME",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "FULL_STATUS_LINE",0,		BOOL_TYPE_VAR,	DEFAULT_FULL_STATUS_LINE, NULL, update_all_status, 0, 0 },
	{ "GTK",0,			BOOL_TYPE_VAR,	DEFAULT_GTK, NULL, NULL, 0, 0 },
	{ "HELP_PAGER",0,		BOOL_TYPE_VAR,	DEFAULT_HELP_PAGER, NULL, NULL, 0, 0 },
	{ "HELP_PATH",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "HELP_PROMPT",0,		BOOL_TYPE_VAR,	DEFAULT_HELP_PROMPT, NULL, NULL, 0, 0 },
	{ "HELP_WINDOW",0,		BOOL_TYPE_VAR,	DEFAULT_HELP_WINDOW, NULL, NULL, 0, 0 },
	{ "HIGHLIGHT_CHAR",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "HIGH_BIT_ESCAPE",0,		INT_TYPE_VAR,	DEFAULT_HIGH_BIT_ESCAPE, NULL, set_meta_8bit, 0, 0 }, 
	{ "HISTORY",0,			INT_TYPE_VAR,	DEFAULT_HISTORY, NULL, set_history_size, 0, VF_NODAEMON },
	{ "HISTORY_CIRCLEQ",0,		BOOL_TYPE_VAR,	1, NULL, NULL, 0, VF_NODAEMON },
	{ "HOLD_INTERVAL",0,		INT_TYPE_VAR,	10, NULL, set_hold_interval, 0, 0 },
	{ "HOLD_MODE",0,		BOOL_TYPE_VAR,	DEFAULT_HOLD_MODE, NULL, reset_line_cnt, 0, 0 },
	{ "HOLD_MODE_MAX",0,		INT_TYPE_VAR,	DEFAULT_HOLD_MODE_MAX, NULL, NULL, 0, 0 },
	{ "ILLEGAL_CHARS",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BXNAP },
	{ "INDENT",0,			BOOL_TYPE_VAR,	DEFAULT_INDENT, NULL, NULL, 0, 0 },
	{ "INPUT_ALIASES",0,		BOOL_TYPE_VAR,	DEFAULT_INPUT_ALIASES, NULL, NULL, 0, 0 },
	{ "INPUT_GLOB",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BXNAP },
	{ "INPUT_PROMPT",0,		STR_TYPE_VAR,	0, NULL, set_input_prompt, 0, 0 },
	{ "INSERT_MODE",0,		BOOL_TYPE_VAR,	DEFAULT_INSERT_MODE, NULL, update_all_status, 0, 0 },
	{ "INVERSE_VIDEO",0,		BOOL_TYPE_VAR,	DEFAULT_INVERSE_VIDEO, NULL, NULL, 0, 0 },
	{ "LASTLOG",0,			INT_TYPE_VAR,	DEFAULT_LASTLOG, NULL, set_lastlog_size, 0, 0 },
	{ "LASTLOG_LEVEL",0,		STR_TYPE_VAR,	0, NULL, set_lastlog_level, 0, 0 },
	{ "LASTLOG_TIMEFORMAT",0,	STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "LOAD_PATH",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "LOG",0,			BOOL_TYPE_VAR,	DEFAULT_LOG, NULL, logger, 0, 0 },
	{ "LOGFILE",0,			STR_TYPE_VAR,	0, NULL, set_log_file, 0, VF_NODAEMON },
	{ "LOG_REWRITE",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "MANGLE_LOGFILES",0,		STR_TYPE_VAR,	0, NULL, set_mangle_logfiles, 0, 0 },
	{ "MAX_RELM",0,			INT_TYPE_VAR,	10,NULL, NULL, 0, 0 },
	{ "MAX_REQUESTS_NICK",0,	INT_TYPE_VAR,	0, NULL, NULL, 0 },
	{ "MAX_RESULTS",0,		INT_TYPE_VAR,	100, NULL, 0, VIF_BXNAP },
	{ "MAX_SENDS_NICK",0,		INT_TYPE_VAR,	4, NULL, NULL, 0, 0 },
	{ "MAX_SERVER_RECONNECT",0,	INT_TYPE_VAR,	2, NULL, 0, VIF_BXNAP },
	{ "MAX_URLS",0,			INT_TYPE_VAR,	50,NULL, 0, VIF_BXNAP },
	{ "META_STATES",0,		INT_TYPE_VAR,	DEFAULT_META_STATES, NULL, NULL, 0, 0 },
	{ "MOVE_INCOMPLETE",0,		BOOL_TYPE_VAR,	1, NULL, NULL, 0, 0 },
	{ "NAMES_COLUMNS",0,		INT_TYPE_VAR,	4, NULL, NULL, 0, VIF_BXNAP },
	{ "NAMES_NICKCOLOR",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "ND_SPACE_MAX",0,		INT_TYPE_VAR,	DEFAULT_ND_SPACE_MAX, NULL, NULL, 0, 0 },
	{ "OUTPUT_REWRITE",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "PAD_CHAR",0,			CHAR_TYPE_VAR,	DEFAULT_PAD_CHAR, NULL, NULL, 0, 0 },
	{ "PING",0,			BOOL_TYPE_VAR,  DEFAULT_PING_SYSTEM, NULL, NULL, 0, 0 },
	{ "PTEST",0,			BOOL_TYPE_VAR,	0, NULL, set_ptest_timeout, 0, 0 },
	{ "PTEST_CHANGE_PORT",0,	STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "PTEST_CONNECT_TIMEOUT",0,	INT_TYPE_VAR,	15, NULL, set_ptest_connect, 0, 0 },
	{ "QUEUE_SENDS",0,		INT_TYPE_VAR,	2, NULL, NULL, 0, VIF_BXNAP },
	{ "QUEUE_SENDS_TIMEOUT",0,	INT_TYPE_VAR,	600, NULL, NULL, 0, VIF_BXNAP },
	{ "QUIET_SENDS",0,		BOOL_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "RANDOM_SOURCE",0,		INT_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "RESUME_DOWNLOAD",0,		BOOL_TYPE_VAR,	1, NULL, NULL, 0, 0 },
	{ "REVERSE_STATUS",0,		BOOL_TYPE_VAR,	0, NULL, reinit_status, 0, 0 },
	{ "SCROLLBACK",0,		INT_TYPE_VAR,	DEFAULT_SCROLLBACK_LINES, NULL, set_scrollback_size, 0, 0 },
	{ "SCROLLBACK_RATIO",0,		INT_TYPE_VAR,	DEFAULT_SCROLLBACK_RATIO, NULL, NULL, 0, 0 },
	{ "SCROLL_LINES",0,		INT_TYPE_VAR,	DEFAULT_SCROLL_LINES, NULL, set_scroll_lines, 0, 0 },
	{ "SEARCH_FORMAT",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ "SEND_LIMIT",0,		INT_TYPE_VAR,	5, NULL, NULL, 0, 0 },
	{ "SHARE",0,			BOOL_TYPE_VAR,	1, NULL, NULL, 0, 0 },
	{ "SHARE_LINKS",0,		BOOL_TYPE_VAR,	1, NULL, NULL, 0, 0 },
	{ "SHELL",0,			STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "SHELL_FLAGS",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VF_NODAEMON },
	{ "SHELL_LIMIT",0,		INT_TYPE_VAR,	DEFAULT_SHELL_LIMIT, NULL, NULL, 0, VF_NODAEMON },
	{ "SHOW_CHANNEL_NAMES",0,	BOOL_TYPE_VAR,	DEFAULT_SHOW_CHANNEL_NAMES, NULL, NULL, 0, 0 },
	{ "SHOW_END_OF_MSGS",0,		BOOL_TYPE_VAR,	DEFAULT_SHOW_END_OF_MSGS, NULL, NULL, 0, 0 },
	{ "SHOW_NUMERICS",0,		BOOL_TYPE_VAR,	DEFAULT_SHOW_NUMERICS, NULL, NULL, 0, 0 },
	{ "SHOW_NUMERICS_STR",0,	STR_TYPE_VAR,	0, NULL, set_numeric_string, 0, 0 },
	{ "SHOW_STATUS_ALL",0,		BOOL_TYPE_VAR,	DEFAULT_SHOW_STATUS_ALL, NULL, update_all_status, 0, 0 },
	{ "STATUS_CHANNEL",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_CLOCK",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_CPU_SAVER",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_FORMAT1",0,           STR_TYPE_VAR,   0, NULL, build_status, 0, 0 },
	{ "STATUS_FORMAT2",0,           STR_TYPE_VAR,   0, NULL, build_status, 0, 0 },
	{ "STATUS_HOLD",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_HOLD_LINES",0,	STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_INSERT",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_NICK",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_NOTIFY",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_NO_REPEAT",0,		BOOL_TYPE_VAR,	DEFAULT_STATUS_NO_REPEAT, NULL, NULL, 0, 0 },
	{ "STATUS_OVERWRITE",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_QUERY",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_SCROLLBACK",0,	STR_TYPE_VAR,	0, NULL, build_status, 0, 0 }, 
	{ "STATUS_SERVER",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_STATS",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER0",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER1",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER10",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER11",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER12",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER13",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER14",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER15",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER16",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER17",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER18",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER19",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER2",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER20",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER21",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER22",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER23",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER24",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER25",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER26",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER27",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER28",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER29",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER3",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER30",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER31",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER32",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER33",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER34",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER35",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER36",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER37",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER38",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER39",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER4",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER5",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER6",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER7",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER8",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_USER9",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "STATUS_WINDOW",0,		STR_TYPE_VAR,	0, NULL, build_status, 0, 0 },
	{ "SUPPRESS_SERVER_MOTD",0,	BOOL_TYPE_VAR,	DEFAULT_SUPPRESS_SERVER_MOTD, NULL, NULL, 0, VF_NODAEMON },
	{ "TAB",0,			BOOL_TYPE_VAR,	DEFAULT_TAB, NULL, NULL, 0, 0 },
	{ "TAB_MAX",0,			INT_TYPE_VAR,	DEFAULT_TAB_MAX, NULL, NULL, 0, 0 },
	{ "TRANSFER_TIMEOUT",0,		INT_TYPE_VAR,	600, NULL, NULL, 0, 0 },
	{ "UNDERLINE_VIDEO",0,		BOOL_TYPE_VAR,	DEFAULT_UNDERLINE_VIDEO, NULL, NULL, 0, 0 },
	{ "URL_GRAB",0,			BOOL_TYPE_VAR,  1, NULL, NULL, 0, VIF_BXNAP },
	{ "VERSION",0,			STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BXNAP },
	{ "WINDOW_QUIET",0,		BOOL_TYPE_VAR,	0, NULL, NULL, 0, VIF_BXNAP },
	{ "WORD_BREAK",0,		STR_TYPE_VAR,	0, NULL, NULL, 0, VIF_BXNAP },
	{ "XTERM_SHOW_TITLE",0,		BOOL_TYPE_VAR,	0, NULL, NULL, 0, 0 },
	{ NULL, 0,			0, 		0, NULL, NULL, 0, 0 }
};

/*
 * get_string_var: returns the value of the string variable given as an index
 * into the variable table.  Does no checking of variable types, etc 
 */
char	*	get_string_var(enum VAR_TYPES var)
{
	return (irc_variable[var].string);
}

/*
 * get_int_var: returns the value of the integer string given as an index
 * into the variable table.  Does no checking of variable types, etc 
 */
int get_int_var(enum VAR_TYPES var)
{
	return (irc_variable[var].integer);
}

/*
 * set_string_var: sets the string variable given as an index into the
 * variable table to the given string.  If string is null, the current value
 * of the string variable is freed and set to null 
 */
void set_string_var(enum VAR_TYPES var, char *string)
{
	if (string)
		malloc_strcpy(&(irc_variable[var].string), string);
	else
		new_free(&(irc_variable[var].string));
}

/*
 * set_int_var: sets the integer value of the variable given as an index into
 * the variable table to the given value 
 */
void set_int_var(enum VAR_TYPES var, unsigned int value)
{
	irc_variable[var].integer = value;
}

/*
 * init_variables: initializes the string variables that can't really be
 * initialized properly above 
 */
void init_variables()
{
#if defined(WINNT) || defined(__EMX__)
	char *shell;
#endif
	int old_display = window_display;
 	int i;
	char *s;
		
	for (i = 1; i < NUMBER_OF_VARIABLES - 1; i++)
		if (strcmp(irc_variable[i-1].name, irc_variable[i].name) >= 0)
			nappanic("Variable [%d] (%s) is out of order.", i, irc_variable[i].name);

	window_display = 0;

	set_string_var(DEFAULT_EMAIL_VAR, "blah@opennap");
	set_string_var(DEFAULT_SERVER_VAR, "bitchx.dimension6.com");
	if (*password)
		set_string_var(DEFAULT_PASSWORD_VAR, password);
	else
		set_string_var(DEFAULT_PASSWORD_VAR, "blah");
	if (*nickname)
		set_string_var(DEFAULT_NICKNAME_VAR,	nickname);
	else
		set_string_var(DEFAULT_NICKNAME_VAR,	"blah");

	set_string_var(DEFAULT_SPEED_VAR, "?");
	set_string_var(PTEST_CHANGE_PORT_VAR, "8899 0");

	s = m_strdup(irc_lib);
	malloc_strcat(&s, "/help");
	set_string_var(HELP_PATH_VAR, s);
	new_free(&s);
	set_string_var(LOGFILE_VAR, DEFAULT_LOGFILE);

	set_string_var(FORMAT_DIRECTORY_VAR, "%d");
	set_string_var(FORMAT_FILENAME_VAR, "%g4.1L %b %t %f");
	set_string_var(SEARCH_FORMAT_VAR, "%-40f %b %3.1H %t %g4.1L %N %1.3p");
	
	set_string_var(NAMES_NICKCOLOR_VAR, FORMAT_NICKCOLOR);
	set_string_var(DOWNLOAD_DIRECTORY_VAR, DEFAULT_DOWNLOAD_DIRECTORY);
	
	set_string_var(SHOW_NUMERICS_STR_VAR, DEFAULT_SHOW_NUMERICS_STR);
	set_numeric_string(current_window, DEFAULT_SHOW_NUMERICS_STR, 0);

	set_string_var(CMDCHARS_VAR, DEFAULT_CMDCHARS);
	set_string_var(LOGFILE_VAR, DEFAULT_LOGFILE);
	set_string_var(WORD_BREAK_VAR, DEFAULT_WORD_BREAK);

#if defined(__EMX__)
	if ((shell = getenv("SHELL")))
	{
		set_string_var(SHELL_VAR, path_search(shell, getenv("PATH")));
        	if (getenv("SHELL_FLAGS"))
        		set_string_var(SHELL_FLAGS_VAR, getenv("SHELL_FLAGS"));
	}
	else
	{
	        shell = "cmd.exe";
        	set_string_var(SHELL_FLAGS_VAR, "/c");
		set_string_var(SHELL_VAR, convert_dos(path_search(shell, getenv("PATH"))));
	}
#elif WINNT
	if ((shell = getenv("SHELL")))
	{
		set_string_var(SHELL_VAR, path_search(shell, getenv("PATH")));
        	if (getenv("SHELL_FLAGS"))
        		set_string_var(SHELL_FLAGS_VAR, getenv("SHELL_FLAGS"));
	}
 	else if (gdwPlatform == VER_PLATFORM_WIN32_WINDOWS)
	{
		shell = "command.com";
        	set_string_var(SHELL_FLAGS_VAR, "/c");
		set_string_var(SHELL_VAR, convert_dos(path_search(shell, getenv("PATH"))));
	}
	else
	{
	        shell = "cmd.exe";
        	set_string_var(SHELL_FLAGS_VAR, "/c");
		set_string_var(SHELL_VAR, convert_dos(path_search(shell, getenv("PATH"))));
	}
#else
	set_string_var(SHELL_VAR, DEFAULT_SHELL);
	set_string_var(SHELL_FLAGS_VAR, DEFAULT_SHELL_FLAGS);
#endif


	set_string_var(CONTINUED_LINE_VAR, DEFAULT_CONTINUED_LINE);
	set_string_var(INPUT_PROMPT_VAR, DEFAULT_INPUT_PROMPT);
	set_string_var(HIGHLIGHT_CHAR_VAR, DEFAULT_HIGHLIGHT_CHAR);
	set_string_var(LASTLOG_LEVEL_VAR, DEFAULT_LASTLOG_LEVEL);

	set_string_var(STATUS_FORMAT1_VAR, DEFAULT_STATUS_FORMAT1);
	set_string_var(STATUS_FORMAT2_VAR, DEFAULT_STATUS_FORMAT2);

	set_string_var(STATUS_CHANNEL_VAR, DEFAULT_STATUS_CHANNEL);
	set_string_var(STATUS_CLOCK_VAR, DEFAULT_STATUS_CLOCK);
	set_string_var(STATUS_CPU_SAVER_VAR, DEFAULT_STATUS_CPU_SAVER);
	set_string_var(STATUS_HOLD_VAR, DEFAULT_STATUS_HOLD);
	set_string_var(STATUS_HOLD_LINES_VAR, DEFAULT_STATUS_HOLD_LINES);
	set_string_var(STATUS_INSERT_VAR, DEFAULT_STATUS_INSERT);
	set_string_var(STATUS_NICK_VAR, DEFAULT_STATUS_NICK);
	set_string_var(STATUS_NOTIFY_VAR, DEFAULT_STATUS_NOTIFY);
	set_string_var(STATUS_OVERWRITE_VAR, DEFAULT_STATUS_OVERWRITE);
	set_string_var(STATUS_QUERY_VAR, DEFAULT_STATUS_QUERY);
	set_string_var(STATUS_SERVER_VAR, DEFAULT_STATUS_SERVER);
	set_string_var(STATUS_STATS_VAR, DEFAULT_STATUS_STATS);
	set_string_var(STATUS_USER0_VAR, DEFAULT_STATUS_USER);
	set_string_var(STATUS_USER1_VAR, DEFAULT_STATUS_USER1);
	set_string_var(STATUS_USER2_VAR, DEFAULT_STATUS_USER2);
	set_string_var(STATUS_USER3_VAR, DEFAULT_STATUS_USER3);
	set_string_var(STATUS_USER4_VAR, DEFAULT_STATUS_USER4);
	set_string_var(STATUS_USER5_VAR, DEFAULT_STATUS_USER5);
	set_string_var(STATUS_USER6_VAR, DEFAULT_STATUS_USER6);
	set_string_var(STATUS_USER7_VAR, DEFAULT_STATUS_USER7);
	set_string_var(STATUS_USER8_VAR, DEFAULT_STATUS_USER8);
	set_string_var(STATUS_USER9_VAR, DEFAULT_STATUS_USER9);
	set_string_var(STATUS_USER10_VAR, DEFAULT_STATUS_USER10);
	set_string_var(STATUS_USER11_VAR, DEFAULT_STATUS_USER11);
	set_string_var(STATUS_USER12_VAR, DEFAULT_STATUS_USER12);
	set_string_var(STATUS_USER13_VAR, DEFAULT_STATUS_USER13);
	set_string_var(STATUS_USER14_VAR, DEFAULT_STATUS_USER14);
	set_string_var(STATUS_USER15_VAR, DEFAULT_STATUS_USER15);
	set_string_var(STATUS_USER16_VAR, DEFAULT_STATUS_USER16);
	set_string_var(STATUS_USER17_VAR, DEFAULT_STATUS_USER17);
	set_string_var(STATUS_USER18_VAR, DEFAULT_STATUS_USER18);
	set_string_var(STATUS_USER19_VAR, DEFAULT_STATUS_USER19);
	set_string_var(STATUS_USER20_VAR, DEFAULT_STATUS_USER20);
	set_string_var(STATUS_USER21_VAR, DEFAULT_STATUS_USER21);
	set_string_var(STATUS_USER22_VAR, DEFAULT_STATUS_USER22);
	set_string_var(STATUS_USER23_VAR, DEFAULT_STATUS_USER23);
	set_string_var(STATUS_USER24_VAR, DEFAULT_STATUS_USER24);
	set_string_var(STATUS_USER25_VAR, DEFAULT_STATUS_USER25);
	set_string_var(STATUS_USER26_VAR, DEFAULT_STATUS_USER26);
	set_string_var(STATUS_USER27_VAR, DEFAULT_STATUS_USER27);
	set_string_var(STATUS_USER28_VAR, DEFAULT_STATUS_USER28);
	set_string_var(STATUS_USER29_VAR, DEFAULT_STATUS_USER29);
	set_string_var(STATUS_USER30_VAR, DEFAULT_STATUS_USER30);
	set_string_var(STATUS_USER31_VAR, DEFAULT_STATUS_USER31);
	set_string_var(STATUS_USER32_VAR, DEFAULT_STATUS_USER32);
	set_string_var(STATUS_USER33_VAR, DEFAULT_STATUS_USER33);
	set_string_var(STATUS_USER34_VAR, DEFAULT_STATUS_USER34);
	set_string_var(STATUS_USER35_VAR, DEFAULT_STATUS_USER35);
	set_string_var(STATUS_USER36_VAR, DEFAULT_STATUS_USER36);
	set_string_var(STATUS_USER37_VAR, DEFAULT_STATUS_USER37);
	set_string_var(STATUS_USER38_VAR, DEFAULT_STATUS_USER38);
	set_string_var(STATUS_USER39_VAR, DEFAULT_STATUS_USER39);
	set_string_var(STATUS_WINDOW_VAR, DEFAULT_STATUS_WINDOW);
	set_string_var(STATUS_SCROLLBACK_VAR, DEFAULT_STATUS_SCROLLBACK);
	
	set_string_var(ILLEGAL_CHARS_VAR, DEFAULT_ILLEGAL_CHARS);
		
	set_beep_on_msg(current_window, DEFAULT_BEEP_ON_MSG, 0);

	set_cd_device(current_window, "/dev/cdrom", 0);

	set_lastlog_size(current_window, NULL, irc_variable[LASTLOG_VAR].integer);
	set_history_size(current_window, NULL, irc_variable[HISTORY_VAR].integer);

	set_lastlog_level(current_window, irc_variable[LASTLOG_LEVEL_VAR].string, 0);

	set_input_prompt(current_window, DEFAULT_INPUT_PROMPT, 0);
	build_status(current_window, NULL, 0);
	window_display = old_display;
}


/*
 * do_boolean: just a handy thing.  Returns 1 if the str is not ON, OFF, or
 * TOGGLE 
 */
int do_boolean(char *str, int *value)
{
	upper(str);
	if (strcmp(str, var_settings[ON]) == 0)
		*value = 1;
	else if (strcmp(str, var_settings[OFF]) == 0)
		*value = 0;
	else if (strcmp(str, "TOGGLE") == 0)
	{
		if (*value)
			*value = 0;
		else
			*value = 1;
	}
	else
		return (1);
	return (0);
}

/*
 * set_var_value: Given the variable structure and the string representation
 * of the value, this sets the value in the most verbose and error checking
 * of manors.  It displays the results of the set and executes the function
 * defined in the var structure 
 */

void set_var_value(int var_index, char *value, IrcVariable *dll)
{
	char	*rest;
	IrcVariable *var;
	int	old;
	var = &(irc_variable[var_index]);
	switch (var->type)
	{
	case BOOL_TYPE_VAR:
		if (value && *value && (value = next_arg(value, &rest)))
		{
			old = var->integer;
			if (do_boolean(value, &(var->integer)))
			{
				say("Value must be either ON, OFF, or TOGGLE");
				break;
			}
			if (!(var->int_flags & VIF_CHANGED))
			{
				if (old != var->integer)
					var->int_flags |= VIF_CHANGED;
			}
			if (var->func)
				(var->func) (current_window, NULL, var->integer);
			say("Value of %s set to %s", var->name,
				var->integer ? var_settings[ON]
					     : var_settings[OFF]);
		}
		else
			say("Value of %s -> %s", var->name, var->integer?var_settings[ON] : var_settings[OFF]);
		break;
	case CHAR_TYPE_VAR:
		if (!value)
		{
			if (!(var->int_flags & VIF_CHANGED))
			{
				if (var->integer)
					var->int_flags |= VIF_CHANGED;
			}
			var->integer = ' ';
			if (var->func)
				(var->func) (current_window, NULL, var->integer);
			say("Value of %s set to '%c'", var->name, var->integer);
		}

		else if (value && *value && (value = next_arg(value, &rest)))
		{
			if (strlen(value) > 1)
				say("Value of %s must be a single character",
					var->name);
			else
			{
				if (!(var->int_flags & VIF_CHANGED))
				{
					if (var->integer != *value)
						var->int_flags |= VIF_CHANGED;
				}
				var->integer = *value;
				if (var->func)
					(var->func) (current_window, NULL, var->integer);
				say("Value of %s set to '%c'", var->name,
					var->integer);
			}
		}
		else
			say("Value of %s -> %c", var->name, var->integer);
		break;
	case INT_TYPE_VAR:
		if (value && *value && (value = next_arg(value, &rest)))
		{
			int	val;

			if (!is_number(value))
			{
				say("Value of %s must be numeric!", var->name);
				break;
			}
			if ((val = my_atol(value)) < 0)
			{
				say("Value of %s must be greater than 0", var->name);
				break;
			}
			if (!(var->int_flags & VIF_CHANGED))
			{
				if (var->integer != val)
					var->int_flags |= VIF_CHANGED;
			}
			var->integer = val;
			if (var->func)
				(var->func) (current_window, NULL, var->integer);
			say("Value of %s set to %d", var->name, var->integer);
		}
		else
			say("Value of %s -> %d", var->name, var->integer);
		break;
	case STR_TYPE_VAR:
		if (value)
		{
			if (*value)
			{
				char	*temp = NULL;

				if (var->flags & VF_EXPAND_PATH)
				{
					temp = expand_twiddle(value);
					if (temp)
						value = temp;
					else
						say("SET: no such user");
				}
				if ((!var->int_flags & VIF_CHANGED))
				{
					if ((var->string && ! value) ||
					    (! var->string && value) ||
					    my_stricmp(var->string, value))
						var->int_flags |= VIF_CHANGED;
				}
				malloc_strcpy(&(var->string), value);
				if (temp)
					new_free(&temp);
			}
			else
			{
				say("Value of %s -> %s", var->name, var->string ? var->string : "<null>");
				return;
			}
		}
		else
			new_free(&(var->string));
		if (var->func && !(var->int_flags & VIF_PENDING))
		{
			var->int_flags |= VIF_PENDING;
			(var->func) (current_window, var->string, 0);
			var->int_flags &= ~VIF_PENDING;
		}
		say("Value of %s set to %s", var->name, var->string ?
			var->string : "<EMPTY>");
		break;
	}
}


/*
 * set_variable: The SET command sets one of the irc variables.  The args
 * should consist of "variable-name setting", where variable name can be
 * partial, but non-ambbiguous, and setting depends on the variable being set 
 */
BUILT_IN_COMMAND(setcmd)
{
	char	*var;
	int	cnt = 0;

enum VAR_TYPES	var_index = 0;

	if ((var = next_arg(args, &args)) != NULL)
	{
		if (*var == '-')
		{
			var++;
			args = NULL;
		}
		upper(var);
		find_fixed_array_item (irc_variable, sizeof(IrcVariable), NUMBER_OF_VARIABLES, var, &cnt, (int *)&var_index);

		if (cnt == 1)
			cnt = -1;

		if (cnt < 0)
			irc_variable[var_index].int_flags |= VIF_PENDING;

		if (cnt < 0)
			irc_variable[var_index].int_flags &= ~VIF_PENDING;
		if (1)
		{
			if (cnt < 0)
				set_var_value(var_index, args, NULL);
			else if (cnt == 0)
			{
				say("No such variable \"%s\"", var);
			}
			else
			{
				say("%s is ambiguous", var);
				for (cnt += var_index; var_index < cnt; var_index++)
					set_var_value(var_index, empty_string, NULL);
			}	
		}
	}
	else
        {
		int var_index;
		for (var_index = 0; var_index < NUMBER_OF_VARIABLES; var_index++)
			set_var_value(var_index, empty_string, NULL);
	}
}


/* returns the size of the character set */
int charset_size(void)
{
	return get_int_var(EIGHT_BIT_CHARACTERS_VAR) ? 256 : 128;
}

static	void eight_bit_characters(Window *win, char *unused, int value)
{
	if (value == ON && !term_eight_bit())
		say("Warning!  Your terminal says it does not support eight bit characters");
	set_term_eight_bit(value);
}


static void set_numeric_string(Window *win, char *value, int unused)
{
	malloc_strcpy(&thing_ansi, value);
}

void clear_sets(void)
{
int i = 0;
	for(i = 0; irc_variable[i].name; i++)
		new_free(&irc_variable->string);
}

static void reinit_screen(Window *win, char *unused, int value)
{
	set_input_prompt(current_window, NULL, 0);
	set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
	update_all_windows();
	update_all_status(current_window, NULL, 0);
	update_input(UPDATE_ALL);
}

void reinit_status(Window *win, char *unused, int value)
{
	update_all_windows();
	update_all_status(current_window, NULL, 0);
	build_status(current_window, NULL, 0);
}

void toggle_reverse_status(Window *win, char *unused, int value)
{
	if (!value)
		set_int_var(REVERSE_STATUS_VAR, 1);
	else
		set_int_var(REVERSE_STATUS_VAR, 0);
#ifndef ONLY_STD_CHARS
	set_string_var(SHOW_NUMERICS_STR_VAR, value ? "[1;31mù[0m[1;37mí[1;31mù[0m" : "-:-");
	set_numeric_string(current_window, value ? "[1;31mù[0m[1;37mí[1;31mù[0m":"-:-", 0);
#endif
	reinit_status(win, unused, value);
}

void clear_variables(void)
{
int i;
	for(i = 0; irc_variable[i].name; i++)
	{
		if (irc_variable[i].string)
			new_free(&irc_variable[i].string);
	}
}

#ifndef WANT_CD
void set_cd_device (Window *win, char *value, int unused)
{

}
#endif

static void set_clock_format (Window *win, char *value, int unused)
{
	extern char *time_format; /* XXXX bogus XXXX */
	malloc_strcpy(&time_format, value);
	update_clock(RESET_TIME);
	update_all_status(current_window, NULL, 0);
}

static void set_all_dataport(Window *win, char *unused, int value)
{
int pt = 0;
	make_listen(value, &pt);
	set_int_var(DEFAULT_DATAPORT_VAR, pt);
	send_all_servers(CMDS_CHANGEDATA, "%d", pt);
}

static void set_speed(Window *win, char *value, int unused)
{
int def_speed = 0;
	if (value && *value)
	{
		for (def_speed = 0; _n_speed[def_speed]; def_speed++)
		{
			if (!my_strnicmp(value, _n_speed[def_speed], strlen(value)))
				break;
		}
		if (def_speed > MAX_SPEED)
			if (isdigit(*value))
				def_speed = my_atol(value);
		if (def_speed > MAX_SPEED)
			def_speed = 0;
	}
	send_all_servers(CMDS_CHANGESPEED, "%d", def_speed);
	set_int_var(DEFAULT_SPEED_VAR, def_speed);
	set_string_var(DEFAULT_SPEED_VAR, _n_speed[def_speed]);
	return;
}

static void set_email_address(Window *win, char *value, int unused)
{
	if (value && *value)
		send_all_servers(CMDS_CHANGEEMAIL, "%s", value);
}

static void set_ptest_connect(Window *win, char *unused, int value)
{
	if (value < 1)
		set_int_var(PTEST_CONNECT_TIMEOUT_VAR, 1);
	else if (value > 30)
		set_int_var(PTEST_CONNECT_TIMEOUT_VAR, 30);
}

static void set_ptest_timeout(Window *win, char *unused, int value)
{
#if defined(WANT_PTEST) && defined(WANT_THREAD)
	if (value == ON)
		start_ptest();
	else if (value == OFF)
		stop_ptest();
#endif
}

void save_variables(FILE *fp, int do_all)
{
	IrcVariable *var;

	for (var = irc_variable; var->name; var++)
	{
		if ((do_all == 1) || !(var->int_flags & VIF_GLOBAL))
		{
			fprintf(fp, "SET ");
			switch (var->type)
			{
			case BOOL_TYPE_VAR:
				fprintf(fp, "%s %s\n", var->name, var->integer ?
					var_settings[ON] : var_settings[OFF]);
				break;
			case CHAR_TYPE_VAR:
				fprintf(fp, "%s %c\n", var->name, var->integer);
				break;
			case INT_TYPE_VAR:
				fprintf(fp, "%s %u\n", var->name, var->integer);
				break;
			case STR_TYPE_VAR:
				if (var->string)
					fprintf(fp, "%s %s\n", var->name,
						var->string);
				else
					fprintf(fp, "-%s\n", var->name);
				break;
			}
		}
	}
}


BUILT_IN_COMMAND(savenap)
{
char buffer[BIG_BUFFER_SIZE+1];
char *p;
FILE *outfile = NULL;

#ifdef WINNT
	snprintf(buffer, BIG_BUFFER_SIZE, "~/%s/%s.sav", version, version);
#else
	snprintf(buffer, BIG_BUFFER_SIZE, "~/.%s/%s.sav", version, version);
#endif
	if ((p = expand_twiddle(buffer)))
	{
		if (!(outfile = fopen(p, "w")))
		{
			bitchsay("Cannot open file %s for saving!", buffer);
			new_free(&p);
			return;
		}
	}
	bitchsay("Saving All Your Settings to %s", buffer);
	save_variables(outfile, 1);
	save_hotlist(outfile);
	save_ignore(outfile);
	fclose(outfile);
}

char	*make_string_var(const char *var_name)
{
	int	cnt,
		msv_index;
	char	*ret = NULL;
	char	*copy;
	
	copy = LOCAL_COPY(var_name);
	upper(copy);

	if ((find_fixed_array_item (irc_variable, sizeof(IrcVariable), NUMBER_OF_VARIABLES, copy, &cnt, &msv_index) == NULL))
		return NULL;
	if (cnt >= 0)
		return NULL;
	switch (irc_variable[msv_index].type)
	{
		case STR_TYPE_VAR:
			ret = m_strdup(irc_variable[msv_index].string);
			break;
		case INT_TYPE_VAR:
			ret = m_strdup(ltoa(irc_variable[msv_index].integer));
			break;
		case BOOL_TYPE_VAR:
			ret = m_strdup(var_settings[irc_variable[msv_index].integer]);
			break;
		case CHAR_TYPE_VAR:
			ret = m_dupchar(irc_variable[msv_index].integer);
			break;
	}
	return ret;
}

typedef struct	varstacklist
{
	int 		which;
	IrcVariable 	*set;
	char		*name;
	int		var_index;
	struct varstacklist *next;
}	VarStack;

VarStack *set_stack = NULL;

void do_stack_set(int type, char *args)
{
	VarStack *aptr = set_stack;
	VarStack **aptrptr = &set_stack;

	if (!*aptrptr && (type == STACK_POP || type == STACK_LIST))
	{
		say("Set stack is empty!");
		return;
	}

	if (STACK_PUSH == type)
	{
		enum VAR_TYPES var_index;
		int cnt = 0;

		upper(args);
		find_fixed_array_item (irc_variable, sizeof(IrcVariable), NUMBER_OF_VARIABLES, args, &cnt, (int *)&var_index);

		if (cnt < 0 || cnt == 1)
		{
			aptr = (VarStack *)new_malloc(sizeof(VarStack));
			aptr->next = aptrptr ? *aptrptr : NULL;
			*aptrptr = aptr;
			aptr->set = (IrcVariable *) new_malloc(sizeof(IrcVariable));
			memcpy(aptr->set, &irc_variable[var_index], sizeof(IrcVariable));
			aptr->name = m_strdup(irc_variable[var_index].name);
			aptr->set->string = m_strdup(irc_variable[var_index].string);
			aptr->var_index = var_index;
		}
		else
			say("No such Set [%s]", args);

		return;
	}

	if (STACK_POP == type)
	{
		VarStack *prev = NULL;
		for (aptr = *aptrptr; aptr; prev = aptr, aptr = aptr->next)
		{
			/* have we found it on the stack? */
			if (!my_stricmp(args, aptr->name))
			{
				/* remove it from the list */
				if (prev == NULL)
					*aptrptr = aptr->next;
				else
					prev->next = aptr->next;

				new_free(&(irc_variable[aptr->var_index].string));
				memcpy(&irc_variable[aptr->var_index], aptr->set, sizeof(IrcVariable));
				/* free it */
				new_free((char **)&aptr->name);
				new_free((char **)&aptr->set);
				new_free((char **)&aptr);
				return;
			}
		}
		say("%s is not on the %s stack!", args, "Set");
		return;
	}
	if (STACK_LIST == type)
	{
		VarStack *prev = NULL;
		for (aptr = *aptrptr; aptr; prev = aptr, aptr = aptr->next)
		{
			switch(aptr->set->type)
			{
				case BOOL_TYPE_VAR:
					say("Variable [%s] = %s", aptr->set->name, var_settings[aptr->set->integer]);
					break;
				case INT_TYPE_VAR:
					say("Variable [%s] = %d", aptr->set->name, aptr->set->integer);
					break;
				case CHAR_TYPE_VAR:
					say("Variable [%s] = %c", aptr->set->name, aptr->set->integer);
					break;
				case STR_TYPE_VAR:
					say("Variable [%s] = %s", aptr->set->name, aptr->set->string?aptr->set->string:"<Empty String>");
					break;
				default:
					say("Error in do_stack_set: unknown set type");
			}
		}
		return;
	}
	say("Unknown STACK type ??");
}

static	void	set_hold_interval (Window *win, char *unused, int value)
{
	static int	old_value = -1;
	Window *	window = NULL;

	if (value == old_value)
		return;
	while (traverse_all_windows(&window))
	{
		if (window->hold_interval == old_value)
			window->hold_interval = value;
	}
	old_value = value;
}

int	parse_mangle (char *value, int nvalue, char **rv)
{
	char	*str1, *str2;
	char	*copy;
	char	*nv = NULL;

	if (rv)
		*rv = NULL;

	if (!value)
		return 0;

	copy = LOCAL_COPY(value);

	while ((str1 = new_next_arg(copy, &copy)))
	{
		while (*str1 && (str2 = next_in_comma_list(str1, &str1)))
		{
			     if (!my_strnicmp(str2, "ALL_OFF", 4))
				nvalue |= STRIP_ALL_OFF;
			else if (!my_strnicmp(str2, "-ALL_OFF", 5))
				nvalue &= ~(STRIP_ALL_OFF);
			else if (!my_strnicmp(str2, "ALL", 3))
				nvalue = (0x7FFFFFFF - (MANGLE_ESCAPES));
			else if (!my_strnicmp(str2, "-ALL", 4))
				nvalue = 0;
			else if (!my_strnicmp(str2, "ANSI", 2))
				nvalue |= MANGLE_ANSI_CODES;
			else if (!my_strnicmp(str2, "-ANSI", 3))
				nvalue &= ~(MANGLE_ANSI_CODES);
			else if (!my_strnicmp(str2, "BLINK", 2))
				nvalue |= STRIP_BLINK;
			else if (!my_strnicmp(str2, "-BLINK", 3))
				nvalue &= ~(STRIP_BLINK);
			else if (!my_strnicmp(str2, "BOLD", 2))
				nvalue |= STRIP_BOLD;
			else if (!my_strnicmp(str2, "-BOLD", 3))
				nvalue &= ~(STRIP_BOLD);
			else if (!my_strnicmp(str2, "COLOR", 1))
				nvalue |= STRIP_COLOR;
			else if (!my_strnicmp(str2, "-COLOR", 2))
				nvalue &= ~(STRIP_COLOR);
			else if (!my_strnicmp(str2, "ESCAPE", 1))
				nvalue |= MANGLE_ESCAPES;
			else if (!my_strnicmp(str2, "-ESCAPE", 2))
				nvalue &= ~(MANGLE_ESCAPES);
			else if (!my_strnicmp(str2, "ND_SPACE", 2))
				nvalue |= STRIP_ND_SPACE;
			else if (!my_strnicmp(str2, "-ND_SPACE", 3))
				nvalue &= ~(STRIP_ND_SPACE);
			else if (!my_strnicmp(str2, "NONE", 2))
				nvalue = 0;
			else if (!my_strnicmp(str2, "REVERSE", 2))
				nvalue |= STRIP_REVERSE;
			else if (!my_strnicmp(str2, "-REVERSE", 3))
				nvalue &= ~(STRIP_REVERSE);
			else if (!my_strnicmp(str2, "ROM_CHAR", 2))
				nvalue |= STRIP_ROM_CHAR;
			else if (!my_strnicmp(str2, "-ROM_CHAR", 3))
				nvalue &= ~(STRIP_ROM_CHAR);
			else if (!my_strnicmp(str2, "UNDERLINE", 1))
				nvalue |= STRIP_UNDERLINE;
			else if (!my_strnicmp(str2, "-UNDERLINE", 2))
				nvalue &= ~(STRIP_UNDERLINE);
		}
	}

	if (rv)
	{
		if (nvalue & MANGLE_ESCAPES)
			m_s3cat(&nv, comma, "ESCAPE");
		if (nvalue & MANGLE_ANSI_CODES)
			m_s3cat(&nv, comma, "ANSI");
		if (nvalue & STRIP_COLOR)
			m_s3cat(&nv, comma, "COLOR");
		if (nvalue & STRIP_REVERSE)
			m_s3cat(&nv, comma, "REVERSE");
		if (nvalue & STRIP_UNDERLINE)
			m_s3cat(&nv, comma, "UNDERLINE");
		if (nvalue & STRIP_BOLD)
			m_s3cat(&nv, comma, "BOLD");
		if (nvalue & STRIP_BLINK)
			m_s3cat(&nv, comma, "BLINK");
		if (nvalue & STRIP_ROM_CHAR)
			m_s3cat(&nv, comma, "ROM_CHAR");
		if (nvalue & STRIP_ND_SPACE)
			m_s3cat(&nv, comma, "ND_SPACE");
		if (nvalue & STRIP_ALL_OFF)
			m_s3cat(&nv, comma, "ALL_OFF");

		*rv = nv;
	}

	return nvalue;
}

static	void	set_mangle_logfiles (Window *win, char *value, int unused)
{
	char *nv = NULL;
	Window *tmp = NULL;
	while (traverse_all_windows(&tmp))
		tmp->mangler = parse_mangle(value, tmp->mangler, &nv);
	set_string_var(MANGLE_LOGFILES_VAR, nv);
	new_free(&nv);
}


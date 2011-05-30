
 /* $Id: commands.c,v 1.1.1.1 2001/02/19 03:23:48 edwards Exp $ */
 
#include "teknap.h"
#include "struct.h"
#include "alias.h"
#include "exec.h"
#include "if.h"
#include "history.h"
#include "hook.h"
#include "keys.h"
#include "list.h"
#include "input.h"
#include "output.h"
#include "ircterm.h"
#include "server.h"
#include "vars.h"
#include "window.h"
#include "status.h"
#include "stack.h"
#include "napster.h"
#include "commands.h"
#include "timer.h"
#include "help.h"
#include "scott2.h"
#include "tgtk.h"
#include "whois.h"
#include "ptest.h"
#include "irchandler.h"

#ifdef WANT_CD
#include "cdrom.h"
#endif

#ifdef WANT_MP3PLAYER
#include "mp3control.h"
#endif

#include <unistd.h>
#include <sys/stat.h>

#ifdef HAVE_UNAME
#include <sys/utsname.h>
#endif

int parse_command(register char *, int, char *);

#ifndef SCRIPT_PATH
#define SCRIPT_PATH "/usr/local/lib/"
#endif

BUILT_IN_COMMAND(send_comm);
BUILT_IN_COMMAND(send_1comm);
BUILT_IN_COMMAND(send_2comm);
BUILT_IN_COMMAND(exec_cmd);
BUILT_IN_COMMAND(versioncmd);
BUILT_IN_COMMAND(nap_stats);
BUILT_IN_COMMAND(evalcmd);
BUILT_IN_COMMAND(send_me);
BUILT_IN_COMMAND(query);
BUILT_IN_COMMAND(browse);
BUILT_IN_COMMAND(my_clear);
BUILT_IN_COMMAND(scott);
BUILT_IN_COMMAND(send_with_channel);
BUILT_IN_COMMAND(relmcmd);
BUILT_IN_COMMAND(queuecmd);
BUILT_IN_COMMAND(server_ignore);
BUILT_IN_COMMAND(echocmd);
BUILT_IN_COMMAND(resume);
BUILT_IN_COMMAND(beepcmd);
BUILT_IN_COMMAND(nslookup);
BUILT_IN_COMMAND(mychdir);
BUILT_IN_COMMAND(dbsearch);
BUILT_IN_COMMAND(send_allcomm);
BUILT_IN_COMMAND(send_file);
BUILT_IN_COMMAND(send_kill);
BUILT_IN_COMMAND(send_kick);
BUILT_IN_COMMAND(send_muzzle);
BUILT_IN_COMMAND(send_unmuzzle);
BUILT_IN_COMMAND(ptestcmd);
BUILT_IN_COMMAND(hookcmd);
BUILT_IN_COMMAND(inputcmd);
BUILT_IN_COMMAND(reset);
BUILT_IN_COMMAND(e_pause);

BUILT_IN_COMMAND(breakcmd);
BUILT_IN_COMMAND(continuecmd);
BUILT_IN_COMMAND(returncmd);
BUILT_IN_COMMAND(sleepcmd);
BUILT_IN_COMMAND(usleepcmd);
BUILT_IN_COMMAND(comment);
BUILT_IN_COMMAND(waitcmd);
BUILT_IN_COMMAND(spingcmd);
BUILT_IN_COMMAND(dnscmd);
BUILT_IN_COMMAND(sendlinecmd);
BUILT_IN_COMMAND(listusers);
BUILT_IN_COMMAND(send_chanlevel);
BUILT_IN_COMMAND(listop);
BUILT_IN_COMMAND(createop);
BUILT_IN_COMMAND(delop);
BUILT_IN_COMMAND(bancmd);
BUILT_IN_COMMAND(opsay_cmd);
BUILT_IN_COMMAND(mode_cmd);
BUILT_IN_COMMAND(invite_cmd);
BUILT_IN_COMMAND(setuser);
BUILT_IN_COMMAND(variablecmd);
BUILT_IN_COMMAND(url_grabber);
BUILT_IN_COMMAND(pastecmd);
BUILT_IN_COMMAND(stackcmd);
BUILT_IN_COMMAND(pop_cmd);
BUILT_IN_COMMAND(push_cmd);
BUILT_IN_COMMAND(setenvcmd);
BUILT_IN_COMMAND(xtypecmd);
BUILT_IN_COMMAND(irccmd);
BUILT_IN_COMMAND(shiftcmd);
BUILT_IN_COMMAND(unshiftcmd);
BUILT_IN_COMMAND(e_debug);
BUILT_IN_COMMAND(send_listcmd);
BUILT_IN_COMMAND(whowas);
BUILT_IN_COMMAND(playwinamp);
BUILT_IN_COMMAND(pretend_cmd);

int load_depth = -1;
int loading_savefile = 0;
int initial_load = 0;
extern int current_numeric;

PingStruct *ping_time = NULL;
PingStruct *sping_time = NULL;

static IgnoreStruct *ignores = NULL;
static IgnoreStruct *signores = NULL;
static IgnoreStruct *cignores = NULL;

void send_text(const char *, const char *, char *, int, int);

char *recv_nick = NULL;
char *sent_nick = NULL;


typedef	struct	WaitCmdstru
{
	char	*stuff;
	struct	WaitCmdstru	*next;
}	WaitCmd;

static	WaitCmd	*start_wait_list = NULL,
		*end_wait_list = NULL;

#define SERVERREQ	0x0001
#define INTERNAL_HELP	0x0010

/* Used to handle and catch breaks and continues */
	int     will_catch_break_exceptions = 0;
	int     will_catch_continue_exceptions = 0;
	int     will_catch_return_exceptions = 0;
	int     break_exception = 0;
	int     continue_exception = 0;
	int     return_exception = 0;


IrcCommand irc_command[] = {
{ "",		empty_string,	do_send_text,	NULL, SERVERREQ, 0 },
{ "#",		NULL,		comment,	NULL, 0, 0 },
{ ":",		NULL,		comment,	NULL, 0, 0 },
{ "ADMIN",	NULL,		nadmin,		HELP_ADMIN, SERVERREQ|INTERNAL_HELP, 0 },
{ "ALIAS",	NULL,		aliascmd,	HELP_ALIAS, 0, 0 },
{ "ANNOUNCE",	NULL,		send_allcomm,	HELP_ANNOUNCE, SERVERREQ, CMDR_ANNOUNCE },
{ "ASSIGN",	NULL,		assigncmd,	HELP_ASSIGN, 0, 0 },
{ "BAN",	NULL,		bancmd,		HELP_BANCMD, SERVERREQ, 0 },
{ "BEEP",	NULL,		beepcmd,	HELP_BEEP, 0, 0 },
{ "BIND",	NULL,		bindcmd,	HELP_BIND, 0, 0 },

{ "BLOCK",	NULL,		send_1comm,	HELP_BLOCK, SERVERREQ, CMDS_BLOCK },
{ "BLOCKLIST",	NULL,		send_comm,	HELP_BLOCK, SERVERREQ, CMDS_BLOCKLIST },

{ "BREAK",	NULL,		breakcmd,	HELP_BREAK, 0, 0 },
{ "BROWSE",	NULL,		browse,		HELP_BROWSE, SERVERREQ, CMDS_BROWSE },

{ "CBAN",	NULL,		send_2comm,	HELP_CBAN, SERVERREQ, CMDS_CBAN },
{ "CBANCLEAR",	NULL,		send_1comm,	HELP_CBANCLEAR, SERVERREQ, CMDS_CBANCLEAR },
{ "CBANLIST",	NULL,		send_with_channel,HELP_CBANLIST, SERVERREQ, CMDS_CBANLIST },
{ "CD",		NULL,		mychdir,	HELP_CD, 0, 0 },

#ifdef WANT_CD
{ "CDEJECT",	NULL,		cd_eject,	HELP_CDEJECT, 0, 0 },
{ "CDLIST",	NULL,		cd_list,	HELP_CDLIST, 0, 0 },
{ "CDPAUSE",	NULL,		cd_pause,	HELP_CDPAUSE, 0, 0 },
{ "CDPLAY",	NULL,		cd_play,	HELP_CDPLAY, 0, 0 },
{ "CDSTOP",	NULL,		cd_stop,	HELP_CDSTOP, 0, 0 },
{ "CDVOLUME",	NULL,		cd_volume,	HELP_CDVOLUME, 0, 0 },
#endif

{ "CHANLEVEL",	NULL,		send_chanlevel, HELP_CHANLEVEL, SERVERREQ, CMDS_SETCHANLEVEL },
{ "CHAT",	"CHAT",		send_file,	HELP_SEND, SERVERREQ, 0 },
{ "CIGNORE",	"CIGNORE",	ignore_user,	HELP_CIGNORE, 0, 0 },
{ "CLEAR",	NULL,		my_clear,	HELP_CLEAR, 0, 0 },
{ "CLOAK",	NULL,		send_comm,	HELP_CLOAK, SERVERREQ, CMDS_CLOAK },
{ "CLOSE",	NULL,		serverclose,	HELP_CLOSE, SERVERREQ, 0 },
{ "CMUZZLE",	NULL,		variablecmd,	HELP_CHANMUZZLE, SERVERREQ, CMDS_CHANNELMUZZLE },
{ "CONTINUE",	NULL,		continuecmd,	HELP_CONTINUE, 0, 0 },
{ "CUNBAN",	NULL,		send_2comm,	HELP_CUNBAN, SERVERREQ, CMDS_CUNBAN },
{ "CUNMUZZLE",	NULL,		variablecmd,	HELP_CHANUNMUZZLE, SERVERREQ, CMDS_CHANNELUNMUZZLE },
{ "DBSEARCH",	NULL,		dbsearch,	HELP_DBSEARCH, 0, 0 },
{ "DCCGET",	"DCC_GET",	request,	HELP_DCCGET, 0, 0 },

{ "DEBUG",	NULL,		e_debug,	NULL, 0, 0 },
{ "DELETE",	NULL,		nap_del,	HELP_DELETE, 0, 0 },
{ "DEOP",	NULL,		delop,		HELP_DELOP, 0, 0 },
#ifndef WINNT
{ "DF",		"df",		exec_cmd,	HELP_DF, 0, 0 },
#endif
{ "DISCONNECT",	NULL,		serverclose,	HELP_DISCONNECT, SERVERREQ, 0 },
{ "DNS",	NULL,		dnscmd,		HELP_DNS, SERVERREQ, 0 },
{ "DO",		NULL,		docmd,		HELP_DO, 0, 0 },
{ "DU",		"du",		exec_cmd,	HELP_DU, 0, 0 },
{ "DUMP",	NULL,		dumpcmd,	HELP_DUMP, 0, 0 },
{ "ECHO",	NULL,		echocmd,	HELP_ECHO, 0, 0 },
{ "EVAL",	NULL,		evalcmd,	HELP_EVAL, 0, 0 },
{ "EXEC",	NULL,		execcmd,	HELP_EXEC, 0, 0 },
{ "FE",		"FE",		fe,		HELP_FE, 0, 0 },
{ "FEC",	"FEC",		fe,		HELP_FEC, 0, 0 },
{ "FOR",	NULL,		forcmd,		HELP_FOR, 0, 0 },
{ "FOREACH",	NULL,		foreach,	HELP_FOREACH, 0, 0 },
{ "GET",	"GET",		request,	HELP_GET, SERVERREQ, 0 },
{ "GLIST",	NULL,		glist,		HELP_GLIST, 0, 0 },
{ "GUSERS",	NULL,		listusers,	HELP_GUSERS, SERVERREQ, CMDS_SHOWUSERS },
{ "HELP",	NULL,		help,		HELP_HELP, 0, 0 },
{ "HISTORY",	NULL,		history,	HELP_HISTORY, 0, 0 },
{ "HOOK",	NULL,		hookcmd,	HELP_HOOK, 0, 0 },
{ "HOTLIST",	NULL,		hotlist,	HELP_HOTLIST, 0, 0 },
{ "I",		NULL,		invite_cmd,	HELP_INVITE, SERVERREQ, 0 },
{ "IF",		"IF",		ifcmd,		HELP_IFCMD, 0, 0 },
{ "IGNORE",	NULL,		ignore_user,	HELP_IGNORE, 0, 0 },

{ "INPUT",	"INPUT",	inputcmd,	HELP_INPUT, 0, 0 },
{ "INPUT_CHAR",	"INPUT_CHAR",	inputcmd,	HELP_INPUTCHAR, 0, 0 },
{ "INVITE",	NULL,		invite_cmd,	HELP_INVITE, SERVERREQ, 0 },
{ "IRC",	NULL,		irccmd,		HELP_IRCCMD, 0, 0 },
{ "JOIN",	NULL,		joincmd,	HELP_JOIN, 0, 0 },
{ "KICK",	NULL,		send_kick,	HELP_KICK, 0, CMDS_KICK },
{ "KILL",	NULL,		send_kill,	HELP_KILL, SERVERREQ, CMDS_KILLUSER },
{ "L",		NULL,		partcmd,	HELP_L, SERVERREQ, 0 },
{ "LASTLOG",	NULL,		lastlog,	HELP_LASTLOG, 0, 0 },
{ "LEAVE",	NULL,		partcmd,	HELP_LEAVE, SERVERREQ, 0 },
{ "LIST",	"LIST",		send_listcmd,	HELP_LIST, SERVERREQ, CMDS_SHOWALLCHANNELS },
{ "LOAD",	NULL,		load,		HELP_LOAD, 0, 0 },
{ "LOCAL",	NULL,		localcmd,	HELP_LOCAL, 0, 0 },
#ifndef WINNT
{ "LS",		"ls",		exec_cmd,	HELP_LS, 0, 0 },
#endif
{ "M",		"MSG",		privmsg,	HELP_M, SERVERREQ, 0 },
{ "ME",		NULL,		send_me,	HELP_ME, 0, CMDS_SENDME },
{ "MODE",	NULL,		mode_cmd,	HELP_MODE, SERVERREQ, CMDS_CHANNELMODE },
{ "MOTD",	NULL,		send_comm,	HELP_MOTD, SERVERREQ, CMDR_MOTDS },
#if defined(WANT_MP3PLAYER) && defined(WANT_THREAD)
{ "MP3",	NULL,		mp3play,	NULL, 0, 0 },
#elif defined(WINNT)
{ "MP3",	NULL,		playwinamp,	HELP_WINAMPPLAY, 0, 0 },
#endif
{ "MSG",	"MSG",		privmsg,	HELP_MSG, SERVERREQ, 0 },
{ "MUZZLE",	NULL,		send_muzzle,	HELP_MUZZLE, SERVERREQ, CMDS_MUZZLE },
{ "NAMES",	NULL,		send_with_channel,HELP_NAMES, SERVERREQ, CMDS_NAME },
{ "NSLOOKUP",	NULL,		nslookup,	HELP_NSLOOKUP, 0, 0 },
{ "ON",		NULL,		oncmd,		HELP_ON, 0, 0 },
{ "OP",		NULL,		createop,	HELP_OP, SERVERREQ, 0 },
{ "OPDEL",	NULL,		delop,		HELP_DELOP, SERVERREQ, 0 },
{ "OPLIST",	NULL,		listop,		HELP_OPLIST, SERVERREQ, 0 },
{ "OPSAY",	NULL,		opsay_cmd,	HELP_OPSAY, SERVERREQ, CMDS_OPWALL },
{ "PARSEKEY",	NULL,		parsekeycmd,	HELP_PARSEKEY, 0, 0 },
{ "PART",	NULL,		partcmd,	HELP_PART, SERVERREQ, 0 },
{ "PASTE",	NULL,		pastecmd,	HELP_PASTE, SERVERREQ, 0 },
{ "PAUSE",	NULL,		e_pause,	HELP_PAUSE, 0, 0 },

{ "PING",	NULL,		ping,		HELP_PING, SERVERREQ, 0 },

{ "POP",	NULL,		pop_cmd,	HELP_POP, 0, 0 },
{ "PRETEND",	NULL,		pretend_cmd,	HELP_PRETEND, SERVERREQ, 0 },
{ "PRINT",	NULL,		print_napster,	HELP_PRINT, 0, 0 },
{ "PS",		"ps",		exec_cmd,	HELP_PS, 0, 0 },
{ "PTEST",	NULL,		ptestcmd,	HELP_PTEST, 0, 0 },
{ "PURGE",	NULL,		purge,		HELP_PURGE, 0, 0 },
{ "PUSH",	NULL,		push_cmd,	HELP_PUSH, 0, 0 },
{ "QUERY",	NULL,		query,		HELP_QUERY, 0, 0 },
{ "QUEUE",	NULL,		queuecmd,	HELP_QUEUE, 0, 0 },
{ "QUIT",	NULL,		napquit,	HELP_QUITNAP, 0, 0 },
{ "RAW",	NULL,		rawcmd,		HELP_RAW, SERVERREQ, 0 },
{ "RBIND",	NULL,		rbindcmd,	HELP_RBIND, 0, 0 },
{ "RELM",	NULL,		relmcmd,	HELP_RELM, 0, 0 },
{ "RELOAD",	NULL,		reload_save,	HELP_RELOAD, 0, 0 },
{ "REPEAT",	NULL,		repeatcmd,	HELP_REPEAT, 0, 0 },
{ "REQUEST",	"REQUEST",	request,	HELP_REQUEST, SERVERREQ, 0 },
{ "RESET",	NULL,		reset,		HELP_RESET, 0, 0 },
{ "RESUME",	"RESUME",	resume,		HELP_RESUME, SERVERREQ, 0 },
{ "RETURN",	NULL,		returncmd,	HELP_RETURN, 0, 0 },
{ "S",		NULL,		nap_search,	HELP_S, SERVERREQ, 0 },
{ "SAVE",	NULL,		savenap,	HELP_SAVE, 0, 0 },
{ "SAY",	empty_string,	do_send_text,	HELP_SAY, SERVERREQ, 0 },
{ "SBROWSE",	"SBROWSE",	scott,		HELP_SCOTT, SERVERREQ, 0 },
{ "SC",		NULL,		nap_scan,	HELP_SC, SERVERREQ, 0 },
{ "SCAN",	NULL,		nap_scan,	HELP_SCAN, SERVERREQ, 0 },
{ "SCOTT",	"SCOTT",	scott,		HELP_SCOTT, SERVERREQ, 0 },
{ "SEARCH",	NULL,		nap_search,	HELP_SEARCH, SERVERREQ, 0 },
{ "SEND",	NULL,		send_file,	HELP_SEND, SERVERREQ, 0 },
{ "SENDLINE",	empty_string,	sendlinecmd,	HELP_SENDLINE, 0, 0 },
{ "SERVER",	NULL,		window_serv,	HELP_SERVER, 0, 0 },
{ "SET",	NULL,		setcmd,		HELP_SET, 0, 0 },
#if 0
	{ "setpassword",CMDS_SETPASSWORD,	-1,	4, NULL },
	{ "setchanlevel",CMDS_SETCHANLEVEL,	2,	9, NULL },
	{ "setdataport",CMDR_SETDATAPORT,	2,	4, NULL }, 
	{ "setlinespeed",CMDS_SETLINESPEED,	2,	4, NULL },
	{ "setuserlevel",CMDS_SETUSERLEVEL,	2,	4, NULL },
#endif

{ "SETCHANLEVEL", NULL,		setuser,	HELP_SETCHANNELLEVEL, SERVERREQ, CMDS_SETCHANLEVEL },
{ "SETCHANLIMIT", NULL,		setuser,	HELP_SETCHANNELLIMIT, SERVERREQ, CMDS_SETCHANNELLIMIT },

{ "SETDATAPORT",  NULL,		setuser,	HELP_SETDATAPORT, SERVERREQ, CMDR_SETDATAPORT },
{ "SETENV",	  NULL,		setenvcmd,	HELP_SETENV,	0, 0 },
{ "SETLINESPEED", NULL,		setuser,	HELP_SETLINESPEED, SERVERREQ, CMDS_SETLINESPEED },
{ "SETPASSWORD",  NULL,		setuser,	HELP_SETPASSWORD, SERVERREQ, CMDS_SETPASSWORD },
{ "SETUSERLEVEL", NULL,		setuser,	HELP_SETUSERLEVEL, SERVERREQ, CMDS_SETUSERLEVEL },

{ "SHARE",	NULL,		share_napster,	HELP_SHARE, 0, 0 },
{ "SHIFT",	NULL,		shiftcmd,	HELP_SHIFT, 0, 0 },
{ "SIGNORE",	NULL,		server_ignore,	HELP_SIGNORE, 0, 0 },
{ "SLEEP",	NULL,		sleepcmd,	HELP_SLEEP, 0, 0 },
{ "SOUNDEX",	"SOUNDEX",	nap_search,	HELP_SOUNDEX, SERVERREQ, 0 },
{ "SPING",	NULL,		spingcmd,	HELP_SPING, SERVERREQ, 0 },
{ "SSEARCH",	"SSEARCH",	scott,		HELP_SCOTT, SERVERREQ, 0 },
{ "STACK",	NULL,		stackcmd,	HELP_STACKCMD, 0, 0 },
{ "STATS",	NULL,		nap_stats,	HELP_STATS, 0, 0 },
{ "STUB",	NULL,		stubcmd,	HELP_STUB, 0, 0 },
{ "SWITCH",	NULL,		switchcmd,	HELP_SWITCH, 0, 0 },
{ "TBAN",	"TBAN",		bancmd,		HELP_TBANCMD, 0, 0 },
{ "TIMER",	NULL,		timercmd,	HELP_TIMER, 0, 0 },
{ "TOPIC",	NULL,		topic,		HELP_TOPIC, SERVERREQ, 0 },
{ "TYPE",	NULL,		typecmd,	HELP_TYPE, 0, 0 },
{ "UNBLOCK",	NULL,		send_1comm,	HELP_UNBLOCK, SERVERREQ, CMDS_UNBLOCK },
{ "UNLESS",	"UNLESS",	ifcmd,		HELP_UNLESS, 0, 0 },
{ "UNMUZZLE",	NULL,		send_unmuzzle,	HELP_UNMUZZLE, SERVERREQ, CMDS_UNMUZZLE },
{ "UNSHIFT",	NULL,		unshiftcmd,	HELP_UNSHIFT, 0, 0 },
{ "UNVOICE",	NULL,		variablecmd,	HELP_UNVOICE, SERVERREQ, CMDS_CHANNELUNVOICE },
{ "URL",	NULL,		url_grabber,	HELP_URL, 0, 0 },
{ "USLEEP",	NULL,		usleepcmd,	HELP_USLEEP, 0, 0 },
{ "VERSION",	NULL,		versioncmd,	HELP_VERSION, 0, 0 },
{ "VOICE",	NULL,		variablecmd,	HELP_VOICE, SERVERREQ, CMDS_CHANNELVOICE },
{ "W",		NULL,		whois,		HELP_W, SERVERREQ, 0 },
{ "WAIT",	NULL,		waitcmd,	HELP_WAIT, 0, 0 },
{ "WALLOP",	NULL,		send_allcomm,	HELP_WALLOP, SERVERREQ, CMDS_OPSAY },
{ "WHICH",	"WHICH",	load,		HELP_WHICH, 0, 0 },
{ "WHILE",	"WHILE",	whilecmd,	HELP_WHILE, 0, 0 },
{ "WHOIS",	NULL,		whois,		HELP_WHOIS, SERVERREQ, 0 },
{ "WHOWAS",	NULL,		whowas,		NULL /*HELP_WHOWAS */, SERVERREQ, 0 },
{ "WINDOW",	NULL,		windowcmd,	HELP_WINDOW, INTERNAL_HELP, 0 },
{ "XECHO",	"XECHO",	echocmd,	HELP_XECHO, 0, 0 },
{ "XTYPE",	"XTYPE",	xtypecmd,	HELP_XTYPE, 0, 0 },
{ NULL,		NULL,		NULL,		NULL, 0, 0 }
};

#define	NUMBER_OF_COMMANDS (sizeof(irc_command) / sizeof(IrcCommand)) - 2

/* comment: does the /COMMENT command, useful in .ircrc */
BUILT_IN_COMMAND(comment)
{
	/* nothing to do... */
}



int check_nignore(char *nick)
{
	if (ignores && find_in_list((List **)&ignores, nick, 0))
		return 1;
	return 0;
}

int check_cignore(char *chan)
{
	if (cignores && find_in_list((List **)&cignores, chan, 0))
		return 1;
	return 0;
}

int check_server_ignore(char *match)
{
IgnoreStruct *new;
	if (signores)
	{
		for (new = signores; new; new = new->next)
			if (wild_match(new->nick, match))
				return 1;
	}
	return 0;
}

BUILT_IN_COMMAND(versioncmd)
{
int cmd = CMDS_SENDMSG;
char *arg;
ChannelStruct *ch;
char *version_buf = NULL;
#ifdef HAVE_UNAME
struct utsname buf;
	
	uname(&buf);
	malloc_sprintf(&version_buf, "%s %s %s %s", nap_version, internal_version, buf.sysname, buf.release?buf.release:empty_string);
#else
	malloc_sprintf(&version_buf, "%s %s", nap_version, internal_version);
#endif
	if (!(arg = next_arg(args, &args)))
		arg = current_window->current_channel, cmd = CMDS_SEND;
	else
	{
		for (ch = current_window->nchannels; ch; ch = ch->next)
		{
			if (!my_stricmp(arg, ch->channel))
			{
				cmd = CMDS_SEND;
				break;
			}
		}
	}
	if (arg)		
	{
		if (get_server_ircmode(from_server))
			send_text(arg, version_buf, command, window_display, 1);
		else
			send_ncommand(cmd, "%s %s", arg, version_buf);
	}
	else
		say("%s", version_buf);
	new_free(&version_buf);
}

BUILT_IN_COMMAND(send_comm)
{
	if (get_server_ircmode(from_server) && command)
		send_to_server("%s", command);
	else
		send_ncommand(numeric, NULL);
}

BUILT_IN_COMMAND(send_listcmd)
{
	set_server_listmode(from_server, NULL);
	if (get_server_ircmode(from_server) && command)
		send_to_server("%s", command);
	else
		send_ncommand(numeric, NULL);
	set_server_listmode(from_server, args && *args ? args : NULL);
}

BUILT_IN_COMMAND(send_1comm)
{
char *arg;
	if ((arg = next_arg(args, &args)))
		send_ncommand(numeric, "%s%s%s", arg, args && *args ? space : empty_string, args && *args ? args : empty_string);
}

BUILT_IN_COMMAND(send_2comm)
{
char *arg1, *arg2;
	if ((arg1 = next_arg(args, &args)) && (arg2 = next_arg(args, &args)))
		send_ncommand(numeric, "%s %s%s%s", arg1, arg2, args && *args ? space : empty_string, args && *args ? args : empty_string);
}

BUILT_IN_COMMAND(send_with_channel)
{
char *arg;
	if ((arg = next_arg(args, &args)))
		send_ncommand(numeric, "%s", arg);
	else if (current_window->current_channel)
		send_ncommand(numeric, "%s", current_window->current_channel);
}


BUILT_IN_COMMAND(send_me)
{
	if (args && *args && current_window->current_channel)
	{
		do_hook(SEND_ACTION_LIST, "%s %s", current_window->current_channel, args);
		if (get_server_ircmode(current_window->server))
		{
			char *target = next_arg(args, &args);
			if (args && *args)
			{
				send_to_server("PRIVMSG %s :%cACTION %s%c", target, CTCP_DELIM_CHAR, args, CTCP_DELIM_CHAR);
				put_it("50ð 53%s/%s-1 %s", get_server_nickname(from_server), target, args);
			}
		}
		else
			send_ncommand(numeric, "%s \"%s\"", current_window->current_channel, args);
	}
}

BUILT_IN_COMMAND(send_kill)
{
int mkill = 0;
char *nick, *n;
	if ((nick = next_arg(args, &args)))
	{
		if (!my_stricmp(nick, "-ip"))
		{
			mkill = 1; 
			if (!(nick = next_arg(args, &args)))
				return;
		}
		while ((n = next_in_comma_list(nick, &nick)))
		{
			if (!n || !*n)
				break;
			if (mkill)
				send_ncommand(CMDS_MKILL, "%s \"%s\"", n, args && *args ? args : empty_string);
			else
				send_ncommand(numeric, "%s \"%s\"", n, args && *args ? args : empty_string);
		}
	}
}

BUILT_IN_COMMAND(send_muzzle)
{
char *nick, *n;
	if ((nick = next_arg(args, &args)))
	{
		while ((n = next_in_comma_list(nick, &nick)))
		{
			if (!n || !*n)
				break;
			if (args && *args)
				send_ncommand(numeric, "%s \"%s\"", n, args && *args ? args : empty_string);
			else
				send_ncommand(numeric, "%s", n);
		}
	}
}

BUILT_IN_COMMAND(send_chanlevel)
{
char *nick, *n, *level;
	if ((nick = next_arg(args, &args)))
	{
		level = next_arg(args, &args);
		if (!level || !*level)
		{
			say("Need to specify a Level");
			return;
		}
		while ((n = next_in_comma_list(nick, &nick)))
		{
			if (!n || !*n)
				break;
			send_ncommand(numeric, "%s %s", n, level);
		}
	}
}

BUILT_IN_COMMAND(send_unmuzzle)
{
char *nick, *n;
	nick = next_arg(args, &args);
	while ((n = next_in_comma_list(nick, &nick)))
	{
		if (!n || !*n)
			break;

		if (args && *args)
			send_ncommand(numeric, "%s \"%s\"", n, args);
		else
			send_ncommand(numeric, "%s", n);
	}
}


BUILT_IN_COMMAND(send_kick)
{
char *nick = NULL, *n, *chan = NULL;
Window *tmp = NULL;
	if ((chan = next_arg(args, &args)))
	{
		while (traverse_all_windows(&tmp))
		{
			if (tmp->server != from_server)
				continue;
			if (find_in_list((List **)&tmp->nchannels, chan, 0))
			{
				nick = next_arg(args, &args);
				break;
			}
		}
		if (!nick)
		{
			nick = chan;
			chan = current_window->current_channel;
		}
	}
	if (chan && nick)
	{
		while ((n = next_in_comma_list(nick, &nick)))
		{
			if (!n || !*n)
				break;
			send_ncommand(numeric, "%s %s \"%s\"", chan, n, args && *args ? args : empty_string);
		}
	}
}

BUILT_IN_COMMAND(send_allcomm)
{
	if (args && *args)
		send_ncommand(numeric, "%s", args);
}

BUILT_IN_COMMAND(exec_cmd)
{
	char	buffer[BIG_BUFFER_SIZE + 1];
	
	snprintf(buffer, BIG_BUFFER_SIZE, "%s %s", command, args);
	execcmd(NULL, buffer, subargs, helparg, 0);
}

BUILT_IN_COMMAND(nap_scan)
{
ChannelStruct *ch;
char *chan = NULL, *pattern = NULL;
int sort_type = NORMAL_SORT;

	if (!args || !*args)
		chan = current_window->current_channel;
	else
	{
		char *arg;
		while ((arg = next_arg(args, &args)))
		{
			if (!my_stricmp(arg, "-sort"))
			{
				arg = next_arg(args, &args);
				if (!my_stricmp(arg, "songs"))
					sort_type = SONG_SORT;
				else if (!my_stricmp(arg, "speed"))
					sort_type = SPEED_SORT;
			}
			else if (*arg == '#')
				chan = arg;
			else 
				pattern = arg;
		}
	}
	if (!chan || !*chan)
	{
		chan = current_window->current_channel;;
		if (!chan)
			return;
	}
	if ((ch = (ChannelStruct *)find_in_list((List **)&current_window->nchannels, chan, 0)))
		name_print(chan, ch->nicks, 0, sort_type, pattern);
}

BUILT_IN_COMMAND(nap_search)
{
char buff[BIG_BUFFER_SIZE+1];
char s_buff[BIG_BUFFER_SIZE+1];
char x_buff[BIG_BUFFER_SIZE+1];
int n = 0;

unsigned int size = 0;
unsigned int duration = 0;
unsigned int bitrate = 0;
unsigned int freq = 0;
unsigned int linespeed = 0;
int bit_int = -1;
int freq_int = -1;
int line_int = -1;
int dur_int = -1;
int size_int = -1;

int do_type = 0;
int buf_len = 0;
int local = 0;
int search_flags = 0;
int exclude = 0;

char *search_param[] = { "EQUAL TO", "AT BEST", "AT LEAST", ""};
int soundex = 0;
char *sound_ex[] = { "FILENAME", "SOUNDEX", "" };
char any[] = "ANY";
char *type = NULL;
FileStruct **f;

	if (!args || !*args)
	{
		f = get_search_head(from_server);
		display_list(*f);
		return;
	}
	if (get_server_search(from_server))
	{
		if (args && !my_strnicmp(args, "-end", 4))
			set_server_search(from_server, search_flags);
		else
			say("Already a search in progress");
		return;
	}
	/* 
	 * We'd better search the search list and remove any pointers
	 * from the glist or we'll have a huge problem here.
	 */
	 
	if ((f = get_search_head(from_server)))
		remove_search_from_glist(f);
	if (command && !my_stricmp(command, "soundex"))
		soundex++;

	*x_buff = 0;

	while (args && *args == '-')
	{
		char *cmd, *val;
		unsigned int value = 0;
		cmd = next_arg(args, &args);
		if (!my_strnicmp(cmd, "-local", 3))
		{
			local = 1;
			continue;
		}
		else if (!my_strnicmp(cmd, "-any", 4))
		{
			type = any;
			do_type = 1;
			continue;
		}
		else if (!my_strnicmp(cmd, "-fullpath", 4))
		{
			search_flags |= SEARCH_FULLPATH;
			continue;
		}
		val = new_next_arg(args, &args);
		value = my_atol(val);
		
		if (!my_strnicmp(cmd, "-type", 4))
		{
			if (!(type = val))
			{
				say("You must specify a type [video/image/audio/application/text]");
				break;
			}
			do_type = 1;
			continue;
		}
		if (!my_strnicmp(cmd, "-exclude", 4) && val)
		{
			sprintf(x_buff+exclude, "%s ", val);
			exclude += strlen(x_buff);
			continue;
		}
		if (!my_strnicmp(cmd, "-maxresults", 4))
		{
			if (!args)
			{
				say("Default Max Results %d", get_int_var(MAX_RESULTS_VAR));
				return;
			}
			set_int_var(MAX_RESULTS_VAR, value);
			continue;
		}
		if (strstr(cmd, "bitrate"))
		{
			int br[] = {20, 24, 32, 48, 56, 64, 98, 112, 128, 160, 192, 256, 320, -1 };
			int o;
			for (o = 0; br[o] != -1; o++)
				if (br[o] == value)
					break;
			if (br[o] == -1)
			{
				say("Allowed Bitrates 20, 24, 32, 48, 56, 64, 98, 112, 128, 160, 192, 256, 320");
				return;
			}
			if (!my_strnicmp(cmd, "-bitrate", 4))
				bitrate = value, bit_int = 0;
			else if (!my_strnicmp(cmd, "-minbitrate", 6))
				bitrate = value, bit_int = 2;
			else if (!my_strnicmp(cmd, "-maxbitrate", 6))
				bitrate = value, bit_int = 1;
		} 
		else if (strstr(cmd, "duration"))
		{
			if (!my_strnicmp(cmd, "-duration", 4))
				duration = value, dur_int = 0;
			else if (!my_strnicmp(cmd, "-minduration", 6))
				bitrate = value, dur_int = 2;
			else if (!my_strnicmp(cmd, "-maxduration", 6))
				duration = value, dur_int = 1;
		} 
		else if (strstr(cmd, "size"))
		{
			if (!my_strnicmp(cmd, "-size", 4))
				size = value, size_int = 0;
			else if (!my_strnicmp(cmd, "-minsize", 6))
				size = value, size_int = 2;
			else if (!my_strnicmp(cmd, "-maxsize", 6))
				size = value, size_int = 1;
		} 
		else if (strstr(cmd, "freq"))
		{
			long fr[] = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000, -1};
			int o;
			for (o = 0; fr[o] != -1; o++)
				if (fr[o] == value)
					break;
			if (fr[o] == -1)
			{
				say("Allowed Freq 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000");
				return;
			}
			if (!my_strnicmp(cmd, "-maxfreq", 4))
				freq = value, freq_int = 1;
			else if (!my_strnicmp(cmd, "-minfreq", 6))
				freq = value, freq_int = 2;
			else if (!my_strnicmp(cmd, "-freq", 6))
				freq = value, freq_int = 0;
		}
		else if (strstr(cmd, "line"))
		{
			if (value < 0 || value > MAX_SPEED)
			{
				say("Allowed linespeed 0-%d", MAX_SPEED);
				return;
			}
			if (!my_strnicmp(cmd, "-maxlinespeed", 4))
				linespeed = value, line_int = 1;
			else if (!my_strnicmp(cmd, "-minlinespeed", 6))
				linespeed = value, line_int = 2;
			else if (!my_strnicmp(cmd, "-linespeed", 6))
				linespeed = value, line_int = 0;
		}
#if 0
		else if (strstr(cmd, "-end"))
		{
			set_server_search(from_server, SEARCH_FINISH);
			return;
		}
#endif
	}
	

	if (!args || !*args)
		return;

	*buff = 0;
	
	if (get_icmp_socket(from_server) > -1)
		new_close(get_icmp_socket(from_server));
	set_icmp_socket(from_server, open_icmp());
	
	clear_filelist(get_search_head(from_server));
	if (soundex)
		compute_soundex(s_buff, sizeof(s_buff), args);

	if (do_type && type)
	{
		sprintf(buff, "TYPE %s ", type);
		buf_len = strlen(buff);
	}	
	
	if ((n = get_int_var(MAX_RESULTS_VAR)))
		sprintf(buff + buf_len, "%s CONTAINS \"%s\" MAX_RESULTS %d ", sound_ex[soundex], soundex ? s_buff : lower(args), n);
	else
		sprintf(buff + buf_len, "%s CONTAINS \"%s\" ", sound_ex[soundex], soundex ? s_buff : lower(args));

	buf_len = strlen(buff);
	
	if (local)
	{
		strcat(buff, "LOCAL_ONLY ");
		buf_len = strlen(buff);
	}

	if (exclude)
	{
		sprintf(buff + buf_len, "FILENAME EXCLUDES \"%s\" ", x_buff);
		buf_len = strlen(buff);
	}
	
	if (!do_type && !type)
	{
		if (bitrate && (bit_int != -1))
			strmopencat(buff, BIG_BUFFER_SIZE, " BITRATE \"", search_param[bit_int], "\" \"", ltoa(bitrate), "\"", NULL);
		if (freq && (freq_int != -1))
			strmopencat(buff, BIG_BUFFER_SIZE, " FREQ \"", search_param[freq_int], "\" \"", ltoa(freq), "\"", NULL);
		if (linespeed && (line_int != -1))
			strmopencat(buff, BIG_BUFFER_SIZE, " LINESPEED \"", search_param[line_int], "\" ", ltoa(linespeed), NULL);
		if (duration && (dur_int != -1))
			strmopencat(buff, BIG_BUFFER_SIZE, " DURATION \"", search_param[dur_int], "\" ", ltoa(duration), NULL);
		if (size && (size_int != -1))
			strmopencat(buff, BIG_BUFFER_SIZE, " SIZE \"", search_param[size_int], "\" ", ltoa(size), NULL);
	}
	if (do_type)	
	{
		if (do_hook(SEARCH_BEGIN_LIST, "%s", buff))
			say("* Searching for type [%s] %s %s excluding [%s]", type, args, soundex ? s_buff: "", x_buff);
	}
	else
		if (do_hook(SEARCH_BEGIN_LIST, "%s", buff))
			say("* Searching for %s %s excluding [%s]", args, soundex ? s_buff: "", x_buff);
	send_ncommand(CMDS_SEARCH, "%s", buff);
	set_server_search(from_server, search_flags | SEARCH_START);
}


void save_ignore(FILE *out)
{
IgnoreStruct *tmp;
	if (!out) return;
	for (tmp = ignores; tmp; tmp = tmp->next)
#ifdef WINNT
		fprintf(out, "IGNORE %s\r\n", tmp->nick);
#else
		fprintf(out, "IGNORE %s\n", tmp->nick);
#endif
	for (tmp = cignores; tmp; tmp = tmp->next)
#ifdef WINNT
		fprintf(out, "CIGNORE %s\r\n", tmp->nick);
#else
		fprintf(out, "CIGNORE %s\n", tmp->nick);
#endif
	for (tmp = signores; tmp; tmp = tmp->next)
#ifdef WINNT
		fprintf(out, "SIGNORE \"%s\"\r\n", tmp->nick);
#else
		fprintf(out, "SIGNORE \"%s\"\n", tmp->nick);
#endif
}
BUILT_IN_COMMAND(ignore_user)
{
IgnoreStruct *new;
char *nick;
IgnoreStruct **todo = &ignores;
int c_ignore = 0;
	if (command && !my_stricmp(command, "CIGNORE"))
		todo = &cignores, c_ignore = 1;
	if (!args || !*args)
	{
		char buffer[BIG_BUFFER_SIZE+1];
		int cols = get_int_var(NAMES_COLUMNS_VAR);
		int count = 0;
		int buf_len;
		
		if (!cols)
			cols = 1;
		*buffer = 0;
		say("Ignore List:");
		for (new = *todo; new; new = new->next)
		{
			buf_len = strlen(buffer);
			sprintf(buffer+buf_len, get_string_var(NAMES_NICKCOLOR_VAR), new->nick); 
			strcat(buffer, space);
			if (count++ >= (cols - 1))
			{
				put_it("%s", buffer);
				*buffer = 0;
				count = 0;
			}
		}
		if (*buffer)
			put_it("%s", buffer);
		return;
	}
	while ((nick = new_next_arg(args, &args)))
	{
		if (!my_stricmp("-REMOVE", nick))
		{
			nick = next_arg(args, &args);;
			if (!nick || !*nick)
				continue;
			if ((new = (IgnoreStruct *)remove_from_list((List **)todo, nick)))
			{
				new_free(&new->nick);
				new_free(&new);
				say("Removed %s from ignore list", nick);
			}
			else
				say("%s not found on ignore list", nick);
		}
		else
		{
			if ((!c_ignore && !check_nignore(nick)) || (c_ignore && !check_cignore(nick)))
			{
				new = new_malloc(sizeof(IgnoreStruct));
				new->nick = m_strdup(nick);
				new->start = time(NULL);
				new->next = *todo;
				*todo = new;
				say("Added %s to ignore list", new->nick);
			}
		}
	}
	return;
}

BUILT_IN_COMMAND(server_ignore)
{
IgnoreStruct *new;
char *nick;
	if (!args || !*args)
	{
		say("Server Ignore List:");
		for (new = signores; new; new = new->next)
			put_it("\"%s\"", new->nick);
		return;
	}
	while ((nick = new_next_arg(args, &args)))
	{
		if (*nick == '-')
		{
			nick = new_next_arg(args, &args);
			if (!nick || !*nick)
				continue;
			if ((new = (IgnoreStruct *)remove_from_list((List **)&signores, nick)))
			{
				new_free(&new->nick);
				new_free(&new);
				say("Removed \"%s\" from server ignore list", nick);
			}
		}
		else
		{
			if (!check_server_ignore(nick))
			{
				new = new_malloc(sizeof(IgnoreStruct));
				new->nick = m_strdup(nick);
				new->start = time(NULL);
				new->next = signores;
				signores = new;
				say("Added \"%s\" to server ignore list", new->nick);
			}
		}
	}
	return;
}



BUILT_IN_COMMAND(window_serv)
{
	window_server(current_window, &args, NULL);
}

static void real_quit(char *dummy, char *ptr)
{
	if (ptr && *ptr && (toupper(*ptr) == 'Y'))
		nap_exit(1, "/quit", NULL);
	bitchsay("Good Choice. Not Quitting");
}

BUILT_IN_COMMAND(napquit)
{
	if (!files_in_progress(NULL, NAP_UPLOAD) && !files_in_progress(NULL, NAP_DOWNLOAD))
		nap_exit(1, "/quit", NULL);
	glist(NULL, NULL, NULL, NULL, 0);
	add_wait_prompt("Active File\'s. Really Quit [y/N] ? ", real_quit, NULL, WAIT_PROMPT_LINE, 1);
}

BUILT_IN_COMMAND(partcmd)
{
char *cmd;
ChannelStruct *ch = NULL;
char *parting = NULL;
	cmd = next_arg(args, &args);
	if (cmd)
	{
		if ((ch = (ChannelStruct *)remove_from_list((List **)&current_window->nchannels, cmd)))
			parting = cmd;
		else
		{
			Window *tmp = NULL;
			while (traverse_all_windows(&tmp))
			{
				if (tmp->server != from_server)
					continue;
				if ((ch = (ChannelStruct *)remove_from_list((List **)&tmp->nchannels, cmd)))
				{
					parting = cmd;
					break;
				}
			}
		}
	}
	else if (current_window->current_channel)
	{
		if ((ch = (ChannelStruct *)remove_from_list((List **)&current_window->nchannels, current_window->current_channel)))
			parting = current_window->current_channel;
	} else
		parting = cmd;
	if (parting)
	{
		if (get_server_ircmode(from_server))
			send_to_server("PART %s", parting);
		else
			send_ncommand(CMDS_PART, "%s", parting);
	}
	if (ch)
	{
		say("Parting %s", ch->channel);
		free_nicks(ch);
		if (current_window->current_channel && !my_stricmp(ch->channel, current_window->current_channel))
		{
			if (ch->next)
				malloc_strcpy(&current_window->current_channel, ch->next->channel);
			else if (current_window->nchannels)
				malloc_strcpy(&current_window->current_channel, current_window->nchannels->channel);
			else
				new_free(&current_window->current_channel);
		}
		new_free(&ch->channel);
		new_free(&ch->topic);
		new_free(&ch);
	}
	update_window_status(current_window, 0);
	update_input(UPDATE_ALL);	
}

BUILT_IN_COMMAND(joincmd)
{
char *buffer;
Window *tmp = NULL;
ChannelStruct *df = NULL;
char *ch;
	if ((buffer = next_arg(args, &args)))
	{
		while ((ch = next_in_comma_list(buffer, &buffer)))
		{
			if (!ch || !*ch)
				break;
			while (traverse_all_windows(&tmp))
			{
				if (tmp->server == from_server)
					if ((df = (ChannelStruct *)find_in_list((List **)&tmp->nchannels, ch, 0)))
						break;
			}
			if (!df)
			{
				if (get_server_ircmode(from_server))
					send_to_server("JOIN %s", ch);
				else
					send_ncommand(CMDS_JOIN, "%s", ch);
				add_waiting_channel(current_window, ch);
			} 
			else if (!tmp->bind_channel || my_stricmp(tmp->bind_channel, df->channel))
			{
				df = (ChannelStruct *)remove_from_list((List **)&tmp->nchannels, df->channel);
				add_to_list((List **)&current_window->nchannels, (List *)df);
				if (tmp->current_channel && !my_stricmp(df->channel, tmp->current_channel))
				{
					new_free(&tmp->current_channel);
					if (tmp->nchannels)
						malloc_strcpy(&tmp->current_channel, tmp->nchannels->channel);
				}
				malloc_strcpy(&current_window->current_channel, df->channel);
			}
			else
				say("%s is bound to another window.", ch);
		}
		update_window_status(current_window, 0);
		update_input(UPDATE_ALL);	
	}
	else
		switch_channels(0, NULL);
}




struct target_type
{
	char *nick_list;
	char *message;
	int  hook_type;
	char *format;
	unsigned long level;
	int hook;
};




void 	send_text(const char *nick_list, const char *text, char *command, int hook, int log)
{
	int	hooked, 
		old_window_display = window_display;
	char	*current_nick,
		*next_nick;
	        
struct target_type target[2] = 
{	
{NULL, NULL, CMDS_SENDMSG,	FORMAT_SENDMSG,	LOG_MSG,	SEND_MSG_LIST },
{NULL, NULL, CMDS_SEND,		FORMAT_PUBLIC,	LOG_PUBLIC,	SEND_PUBLIC_LIST },
};
	
	if (!nick_list || from_server == -1)
		return;
	next_nick = LOCAL_COPY(nick_list);
		
	while ((current_nick = next_nick))
	{
		if ((next_nick = strchr(current_nick, ',')))
			*next_nick++ = 0;

		if (!*current_nick)
			continue;

		if (*current_nick == '%')
		{
			if ((hooked = get_process_index(&current_nick)) == -1)
				say("Invalid process specification");
			else
				text_to_process(hooked, text, 1);
		}
		/*
		 * This test has to be here because it is legal to
		 * send an empty line to a process, but not legal
		 * to send an empty line anywhere else.
		 */
		else if (!text || !*text)
			;
		else if (*current_nick == '"')
		{
			if (get_server_ircmode(from_server))
				send_to_server("PRIVMSG %s :%s", current_window->current_channel, text);
			else
				send_ncommand(CMDS_SEND, "%s %s", current_window->current_channel, text);
		}
		else if (*current_nick == '=' && is_valid_dcc(current_nick+1))
		{
			dcc_chat_transmit(current_nick + 1, text, text, 1);
		}
#if 0
		else if (*current_nick == '/')
		{
			line = m_opendup(current_nick, space, text, NULL);
			parse_command(line, 0, empty_string);
			new_free(&line);
		}
#endif
		else
		{
			if ((*current_nick == '#') || (!command && find_in_list((List **)&current_window->nchannels, current_nick, 0)))
				hooked = 1;
			else
			{
				hooked = 0;
				addtabkey(current_nick, "msg", (char *)text);
			}
			if (get_server_ircmode(from_server))
				send_to_server("PRIVMSG %s :%s", current_nick, text);
			else
				send_ncommand(target[hooked].hook_type, "%s %s", current_nick, text);
			m_s3cat(&target[hooked].nick_list, ",", current_nick);
			target[hooked].message = (char *)text;
		}
	}
	
	if (target[0].nick_list && target[0].message)
	{
		const char *old_mf;
		unsigned long old_ml;
				
		save_display_target(&old_mf, &old_ml);
		set_display_target(target[0].nick_list, target[0].level);
		if (do_hook(target[0].hook, "%s %s", target[0].nick_list, target[0].message))
			put_it(target[0].format, target[0].nick_list, target[0].message);
		malloc_strcpy(&sent_nick, target[0].nick_list);
		new_free(&target[0].nick_list);
		target[0].message = NULL;
		restore_display_target(old_mf, old_ml);
	} 
	else if (!get_server_ircmode(from_server) && target[1].nick_list && target[1].message)
	{
		const char *old_mf;
		unsigned long old_ml;
		save_display_target(&old_mf, &old_ml);
		set_display_target(target[1].nick_list, target[1].level);
		do_hook(target[1].hook, "%s %s", target[1].nick_list, target[1].message);
		new_free(&target[1].nick_list);
		target[1].message = NULL;
		restore_display_target(old_mf, old_ml);
	}
	else if (get_server_ircmode(from_server) && target[1].nick_list && target[1].message)
	{
		const char *old_mf;
		unsigned long old_ml;
		int is_current;				
		save_display_target(&old_mf, &old_ml);
		set_display_target(target[1].nick_list, target[1].level);
		is_current = is_current_channel(target[1].nick_list, from_server, 0);
		if (do_hook(target[1].hook, "%s %s", target[1].nick_list, target[1].message))
			put_it(target[1].format, is_current ? get_server_nickname(from_server) : target[1].nick_list, target[1].message);
		new_free(&target[1].nick_list);
		target[1].message = NULL;
		restore_display_target(old_mf, old_ml);
	}
	window_display = old_window_display;
}

void	redirect_text (int to_server, const char *nick_list, const char *text, int hook, int log)
{
	int 	old_from_server = from_server;
	from_server = to_server;
	send_text(nick_list, text, NULL, hook, log);
	from_server = old_from_server;
}

BUILT_IN_COMMAND(privmsg)
{
char *nick;
	if ((nick = next_arg(args, &args)))
	{
		if (!strcmp(nick, ","))
		{
			if (!(nick = recv_nick))
			{
				say("You have not recieved a msg yet");
				return;
			}
		}
		else if (!strcmp(nick, "."))
		{
			if (!(nick = sent_nick))
			{
				say("You have not sent a msg yet");
				return;
			}
		}
		else if (!strcmp(nick, "*"))
		{
			if (!(nick = current_window->current_channel))
			{
				say("You are not in a current channel");
				return;
			}
		}
		send_text(nick, args, command, window_display, 1);
	}
}

BUILT_IN_COMMAND(do_send_text)
{
	char	*tmp;
	char	*cmd = NULL;
	
	if (command)
		tmp = get_current_channel_by_refnum(0);
	else
		tmp = get_target_by_refnum(0);
	if (cmd)
	{
		if (!my_stricmp(cmd, "SAY") || !my_stricmp(cmd, "MSG"))
			send_text(tmp, args, NULL, 1, 1);
		else
			send_text(tmp, args, NULL, 1, 1);					
	}
	else
		send_text(tmp, args, NULL, 1, 1);
}



IrcCommand *find_command(char *com, int *cnt)
{
	IrcCommand *retval;
	int loc;

	/*
	 * As a special case, the empty or NULL command is send_text.
	 */
	if (!com || !*com)
	{
		*cnt = -1;
		return irc_command;
	}

	retval = (IrcCommand *)find_fixed_array_item ((void *)irc_command, sizeof(IrcCommand), NUMBER_OF_COMMANDS + 1, com, cnt, &loc);
	return retval;
}

struct load_info
{
	char 	*filename;
	int	line;
	int	start_line;
} load_level[MAX_LOAD_DEPTH] = {{ NULL, 0 }};

void dump_load_stack (int onelevel)
{
	int i = load_depth;

	if (i == -1)
		return;

	yell("Right before line [%d] of file [%s]", load_level[i].line,
					  load_level[i].filename);

	if (!onelevel)
	{
		while (--i >= 0)
		{
			yell("Loaded right before line [%d] of file [%s]", 
				load_level[i].line, load_level[i].filename);
		}
	}
	return;
}

void parse_line (const char *name, char *org_line, const char *args, int hist_flag, int append_flag, int handle_local)
{
	char	*line = NULL,
			*stuff,
			*s,
			*t;
	int	args_flag = 0,
		die = 0;

	

	if (handle_local)
		make_local_stack((char *)name);

	line = LOCAL_COPY(org_line);

	if (!*org_line)
		do_send_text(NULL, empty_string, empty_string, NULL, 0);
	else if (args) do
	{
		while (*line == '{') 
               	{
                       	if (!(stuff = next_expr(&line, '{'))) 
			{
				naperror("Unmatched {");
				destroy_local_stack();
                               	return;
                        }
			if (((internal_debug & DEBUG_CMDALIAS) || (internal_debug & DEBUG_HOOK)) && alias_debug && !in_debug_yell && stuff)
				debugyell("%3d %s", debug_count++, stuff);

       	                parse_line(name, stuff, args, hist_flag, append_flag, handle_local);

			if ((will_catch_break_exceptions && break_exception) ||
			    (will_catch_return_exceptions && return_exception) ||
			    (will_catch_continue_exceptions && continue_exception))
			{
				die = 1;
				break;
			}

			while (line && *line && ((*line == ';') || (my_isspace(*line))))
				*line++ = '\0';
		}

		if (!line || !*line || die)
			break;

		stuff = expand_alias(line, args, &args_flag, &line);
		if (!line && append_flag && !args_flag && args && *args)
			m_3cat(&stuff, space, args);

		if (((internal_debug & DEBUG_CMDALIAS) || (internal_debug & DEBUG_HOOK)) && alias_debug && !in_debug_yell && stuff)
			debugyell("%3d %s", debug_count++, stuff);

		parse_command(stuff, hist_flag, (char *)args);
       	        new_free(&stuff);

		if ((will_catch_break_exceptions && break_exception) ||
		    (will_catch_return_exceptions && return_exception) ||
		    (will_catch_continue_exceptions && continue_exception))
			break;
	} while (line && *line);
        else
	{
		if (load_depth != -1) /* CDE should this be != 1 or > 0 */
		{
			if (((internal_debug & DEBUG_CMDALIAS) || (internal_debug & DEBUG_HOOK)) && alias_debug && !in_debug_yell && line)
				debugyell("%3d %s %s", debug_count++, line, args ? args : empty_string);
			parse_command(line, hist_flag, (char *)args);
		}
		else
		{
			while ((s = line))
			{
				if ((t = sindex(line, "\r\n")))
				{
					*t++ = '\0';
					line = t;
				}
				else
					line = NULL;
				if (((internal_debug & DEBUG_CMDALIAS) || (internal_debug & DEBUG_HOOK)) && alias_debug && !in_debug_yell && s)
					debugyell("%3d %s", debug_count++, s, args ? args : empty_string);
				parse_command(s, hist_flag, (char *)args);

				if ((will_catch_break_exceptions && break_exception) ||
				    (will_catch_return_exceptions && return_exception) ||
				    (will_catch_continue_exceptions && continue_exception))
					break;

			}
		}
		debug_count = 1;
	}
	if (handle_local)
		destroy_local_stack();
	return;
}

int parse_command(register char *line, int hist_flag, char *sub_args)
{
static unsigned int level =  0;
char	*cmdchars,
	*this_cmd,
	*rest,
	*com,
	*cline;
int	add_to_hist,
	display,
	noisy = 1,
	args_flag = 0,
	cmdchar_used = 0,
	old_alias_debug = alias_debug,
	old_display_var;

	if (!line || !*line) /* XXX should this be here? */
		return 0;
		
#if 0
	if (internal_debug & DEBUG_COMMANDS && !in_debug_yell)
		debugyell("%3d Executing [%d] %s (%s)", debug_count++, level, line, sub_args ? sub_args : empty_string);
#endif

	if (!(cmdchars = get_string_var(CMDCHARS_VAR)))
		cmdchars = DEFAULT_CMDCHARS;

	level++;
	
	this_cmd = LOCAL_COPY(line);

	add_to_hist = 1;
	display = window_display;
	old_display_var = (unsigned) get_int_var(DISPLAY_VAR);

	for ( ;*line;line++)
	{
		if (*line == '^' && (!hist_flag || cmdchar_used))
		{
			if (!noisy)
				break;
			noisy = 0;
		}
		else if ((!hist_flag && *line == '/') || strchr(cmdchars, *line))
		{
			cmdchar_used++;
			if (cmdchar_used > 2)
				break;
		}
		else
			break;
	}

	com = line;
	if (!noisy)
		window_display = 0;
			
	if (hist_flag && !cmdchar_used && !get_int_var(COMMAND_MODE_VAR))
	{
		send_text(get_target_by_refnum(0), line, NULL, 0, 0);
		if (hist_flag && add_to_hist)
			add_to_history(this_cmd);
	}
	else if (*com == '\'' && get_int_var(COMMAND_MODE_VAR))
	{
		send_text(NULL, line+1, NULL, 0, 0);
		if (hist_flag && add_to_hist)
			add_to_history(this_cmd);
	}
	else if ((*com == '@') || (*com == '('))
	{
		/* This kludge fixes a memory leak */
		char	*tmp;
		char	*my_line = LOCAL_COPY(line); 
		char	*l_ptr;
		/*
		 * This new "feature" masks a weakness in the underlying
		 * grammar that allowed variable names to begin with an
		 * lparen, which inhibited the expansion of $s inside its
		 * name, which caused icky messes with array subscripts.
		 *
		 * Anyhow, we now define any commands that start with an
		 * lparen as being "expressions", same as @ lines.
		 */
		if (*com == '(')
		{
			if ((l_ptr = MatchingBracket(my_line + 1, '(', ')')))
				*l_ptr++ = 0;
		}

		if ((tmp = parse_inline(my_line + 1, sub_args, &args_flag)))
			new_free(&tmp);

		if (hist_flag && add_to_hist)
			add_to_history(this_cmd);
	}
	else
	{
		char	*alias = NULL,
			*alias_name = NULL;
		int	alias_cnt = 0;
		void	*arglist = NULL;
					
		if ((rest = strchr(line, ' ')))
		{
			cline = alloca((rest - line) + 1);
			strmcpy(cline, line, rest - line);
			rest++;
		}
		else
		{	
			cline = LOCAL_COPY(line);
			rest = empty_string;
		}

		upper(cline);

		if (cmdchar_used < 2)
			alias = get_cmd_alias(cline, &alias_cnt, &alias_name, &arglist);

		if (alias && alias_cnt < 0)
		{
			if (hist_flag && add_to_hist)
				add_to_history(this_cmd);
			call_user_alias(alias_name, alias, rest, arglist);
			new_free(&alias_name);
			goto end_parse;
		}

		if (*cline == '!')
		{
			if ((cline = do_history(cline + 1, rest)) != NULL)
			{
				if (level == 1)
				{
					set_input(cline);
					update_input(UPDATE_ALL);
				}
				else
					parse_command(cline, 0, sub_args);
				new_free(&cline);
			}
			else
				set_input(empty_string);
		}
		else
		{
			char unknown[] = "Unknown command:";
			int cmd_cnt = 0;
			IrcCommand      *command = NULL;
		
			if (hist_flag && add_to_hist)
				add_to_history(this_cmd);
			command = find_command(cline, &cmd_cnt);

			if (cmd_cnt < 0 || (alias_cnt == 0 && cmd_cnt == 1))
			{
				if (rest && !my_strnicmp(rest, "-help", 3))
				{
					if ((command->flag & INTERNAL_HELP) == INTERNAL_HELP)
						command->func(command->server_func, rest, sub_args, command->help, 0);
					else
						bitchsay("Usage: /%s %s", cline, command->help? command->help:"Undocumented");
				}
				else if ((command->flag & SERVERREQ) && (from_server == -1))
					say("Try connecting to a server first");
				else if (command->func)
					command->func(command->server_func, rest, sub_args, command->help, command->numeric);
			}
			else if ((alias_cnt == 1 && cmd_cnt == 1 && (!strcmp(alias_name, command->name))) ||
				    (alias_cnt == 1 && cmd_cnt == 0))
			{
				will_catch_return_exceptions++;
				parse_line(alias_name, alias, rest, 0, 1, 1);
				will_catch_return_exceptions--;
				return_exception = 0;
			}
			else if (from_server != -1 && !my_stricmp(get_server_nickname(from_server), cline))
				send_me(NULL, rest, empty_string, NULL, 0);
			else if (cmd_cnt > 1)
				bitchsay("Ambiguous command: %s", cline);
			else if (cmd_cnt == 0)
				bitchsay("%s %s", unknown, cline);
		}	
		if (alias)
			new_free(&alias_name);
	}

end_parse:

	if (old_display_var != get_int_var(DISPLAY_VAR))
		window_display = get_int_var(DISPLAY_VAR);
	else
		window_display = display;
	level--;
	alias_debug = old_alias_debug;
	return 0;
}

void command_completion(char unused, char *not_used)
{
	int	do_aliases = 1, 
		do_functions = 1;
	int	cmd_cnt = 0,
		alias_cnt = 0,
		function_cnt = 0,
		i,
		c;

	char	**aliases = NULL;
	char	**functions = NULL;

	char	*line = NULL,
		*com,
		*cmdchars,
		*rest,
		firstcmdchar[2] = "/";
IrcCommand	*command;
	char	buffer[BIG_BUFFER_SIZE + 1];
	
	
	malloc_strcpy(&line, get_input());
	if ((com = next_arg(line, &rest)) != NULL)
	{
		if (!(cmdchars = get_string_var(CMDCHARS_VAR)))
			cmdchars = DEFAULT_CMDCHARS;
		if (*com == '/' || strchr(cmdchars, *com))
		{
			*firstcmdchar = *cmdchars;
			com++;
			if (*com && strchr(cmdchars, *com))
			{
				do_aliases = 0;
				do_functions = 0;
				alias_cnt = 0;
				function_cnt = 0;
				com++;
			}
			else if (*com && strchr("$", *com))
			{
				do_aliases = 0;
				alias_cnt = 0;
				do_functions = 1;
				com++;
			} else
				do_aliases = do_functions = 1;

			upper(com);
			if (do_aliases)
				aliases = glob_cmd_alias(com, &alias_cnt);

			if (do_functions)
				functions = get_builtins(com, &function_cnt);

			if ((command = find_command(com, &cmd_cnt)) != NULL)
			{
				if (cmd_cnt < 0)
					cmd_cnt *= -1;
				/* special case for the empty string */
				if (!*command->name)
				{
					command++;
					cmd_cnt = NUMBER_OF_COMMANDS;
				}
			}
			if ((alias_cnt == 1) && (cmd_cnt == 0))
			{
				snprintf(buffer, BIG_BUFFER_SIZE, "%s%s %s", firstcmdchar, *aliases, rest);
				set_input(buffer);
				new_free((char **)aliases);
				new_free((char **)&aliases);
				update_input(UPDATE_ALL);
			}
			else if (((cmd_cnt == 1) && (alias_cnt == 0)) ||
			    (((cmd_cnt == 1) && (alias_cnt == 1) &&
			    !strcmp(aliases[0], command[0].name))))
			{
				snprintf(buffer, BIG_BUFFER_SIZE, "%s%s %s", firstcmdchar,
					command[0].name, rest);
				set_input(buffer);
				update_input(UPDATE_ALL);
			}
			else
			{
				*buffer = 0;
				if (command)
				{
					int buf_len;
					*buffer = 0;
					c = 0;
					for (i = 0; i < cmd_cnt; i++)
					{
						if (i == 0)
							bitchsay("Commands:");
						buf_len = strlen(buffer);
						sprintf(buffer+buf_len, "%15s", command[i].name);
						if (++c == 5)
						{
							put_it("%s", buffer);
							*buffer = 0;
							c = 0;
						}
					}
					if (*buffer)
						put_it("%s", buffer);
				}
				if (aliases)
				{
					int buf_len = 0;
					*buffer = 0;
					c = 0;
					for (i = 0; i < alias_cnt; i++)
					{
						if (i == 0)
							bitchsay("Aliases:");
						buf_len = strlen(buffer);
						sprintf(buffer+buf_len, "%15s", aliases[i]);
						if (++c == 5)
						{
							put_it("%s", buffer);
							*buffer = 0;
							c = 0;
						}
						new_free(&(aliases[i]));
					}
					if (*buffer)
						put_it("%s", buffer);
					new_free((char **)&aliases);
				}
				if (functions)
				{
					int buf_len = 0;
					*buffer = 0;
					c = 0;
					for (i = 0; i < function_cnt; i++)
					{
						if (i == 0)
							bitchsay("Functions:");
						buf_len = strlen(buffer);
						sprintf(buffer+buf_len, "%15s", functions[i]);
						if (++c == 5)
						{
							put_it("%s", buffer);
							*buffer = 0;
							c = 0;
						}
						new_free(&(functions[i]));
					}
					if (*buffer)
						put_it("%s", buffer);
					new_free((char **)&functions);
				}
				if (!*buffer)
					term_beep();
			}
		}
		else
			term_beep();
	}
	else
		term_beep();
	new_free(&line);
}


BUILT_IN_COMMAND(load)
{
	FILE	*fp;
	char	*filename,
		*expanded = NULL;
	int	flag = 0;
        int     paste_level = 0;
	register char	*start;
	char	*current_row = NULL;
#define MAX_LINE_LEN 5 * BIG_BUFFER_SIZE
	char	buffer[MAX_LINE_LEN + 1];
	int	no_semicolon = 1;
	char	*irc_path;
	int	display = window_display;
        int     ack = 0;

	
	if (!(irc_path = get_string_var(LOAD_PATH_VAR)))
	{
		int owc = window_display;
		window_display = 1;
		bitchsay("LOAD_PATH has not been set");
		window_display = owc;
		return;
	}

	if (++load_depth == MAX_LOAD_DEPTH)
	{
		int owc = window_display;
		window_display = 1;
		load_depth--;
		bitchsay("No more than %d levels of LOADs allowed", MAX_LOAD_DEPTH);
		window_display = owc;
		return;
	}

	display = window_display;
	window_display = 0;
	status_update(0);

	/* 
	 * We iterate over the whole list -- if we use the -args flag, the
	 * we will make a note to exit the loop at the bottom after we've
	 * gone through it once...
	 */
	while (args && *args && (filename = next_arg(args, &args)))
	{
		int error = ERR_NOERROR;

		/* 
		   If we use the args flag, then we will get the next
		   filename (via continue) but at the bottom of the loop
		   we will exit the loop 
		 */
		if (!my_strnicmp(filename, "-args", strlen(filename)))
		{
			flag = 1;
			continue;
		}
		if (!(expanded = expand_twiddle(filename)))
		{
			int owc = window_display;
			window_display = 1;
			if (!initial_load)
				naperror("Unknown user for file %s", filename);
			window_display = owc;
			continue;
		}

		if (!(fp = uzfopen(&expanded, irc_path, 0, &error)))
		{
			int owc = window_display;
			/* uzfopen emits an error if the file
			 * is not found, so we dont have to. */
			window_display = 1;
#if 0
			if (!initial_load)
				say("Filename not found [%s]", filename);
#else
			if (!initial_load)
				say("%s", uzfopen_error(error));
#endif
			new_free(&expanded);
			window_display = owc;
			continue;
		}

		if (command && *command == 'W')
		{
			if (!window_display)
				window_display = 1;
			yell ("%s", expanded);
			if (fp)
				fclose (fp);
			new_free(&expanded);
			window_display = 0;
			continue;
		}
/* Reformatted by jfn */
/* *_NOT_* attached, so dont "fix" it */
		{
		int	in_comment 	= 0;
		int	comment_line 	= -1;
		int 	paste_line	= -1;

		current_row = NULL;
		load_level[load_depth].filename = expanded;
		load_level[load_depth].line = 1;
                                
		for (;;load_level[load_depth].line++)
		{
			int len;
			register char *ptr;
			
			if (!(fgets(buffer,MAX_LINE_LEN, fp)))
				break;

			for (start = buffer; my_isspace(*start); start++)
				;
			if (!*start || *start == '#')
				continue;

			len = strlen(start);

			/*
			 * this line from stargazer to allow \'s in scripts for continued
			 * lines <spz@specklec.mpifr-bonn.mpg.de>
			 *  If we have \\ at the end of the line, that
			 *  should indicate that we DONT want the slash to 
			 *  escape the newline (hop)
			 *  We cant just do start[len-2] because we cant say
			 *  what will happen if len = 1... (a blank line)
			 *  SO.... 
			 *  If the line ends in a newline, and
			 *  If there are at least 2 characters in the line
			 *  and the 2nd to the last one is a \ and,
			 *  If there are EITHER 2 characters on the line or
			 *  the 3rd to the last character is NOT a \ and,
			 *  If the line isnt too big yet and,
			 *  If we can read more from the file,
			 *  THEN -- adjust the length of the string
			 */

			while ( (start[len-1] == '\n') && 
				(len >= 2 && start[len-2] == '\\') &&
				(len < 3 || start[len-3] != '\\') && 
				(len < MAX_LINE_LEN) && 
				(fgets(&(start[len-2]), MAX_LINE_LEN - len, fp)))
			{
				len = strlen(start);
				load_level[load_depth].line++;
			}

			if (start[len - 1] == '\n')
				start[--len] = 0;
			if (start[len-1] == '\r')
				start[--len] = 0;

			while (start && *start)
			{
				char    *optr = start;

				/* Skip slashed brackets */
				while ((ptr = sindex(optr, "{};/")) && 
				      ptr != optr && ptr[-1] == '\\')
					optr = ptr+1;

				/* 
				 * if no_semicolon is set, we will not attempt
				 * to parse this line, but will continue
				 * grabbing text
				 */
				if (no_semicolon)
					no_semicolon = 0;
				else if ((!ptr || (ptr != start || *ptr == '/')) && current_row)
				{
					if (!paste_level)
					{
						parse_line(NULL, current_row, flag ? args :get_int_var(INPUT_ALIASES_VAR) ?empty_string: NULL, 0, 0, 1);
						new_free(&current_row);
					}
					else if (!in_comment)
						malloc_strcat(&current_row, ";");
				}

				if (ptr)
				{
					char    c = *ptr;

					*ptr = '\0';
					if (!in_comment)
						malloc_strcat(&current_row, start);
					*ptr = c;

					switch (c)
					{
		/* switch statement tabbed back */
case '/' :
{
	/* If we're in a comment, any slashes that arent preceeded by
	   a star is just ignored (cause its in a comment, after all >;) */
	if (in_comment)
	{
		/* ooops! cant do ptr[-1] if ptr == optr... doh! */
		if ((ptr > start) && (ptr[-1] == '*'))
		{
			in_comment = 0;
			comment_line = -1;
		}
		else
			break;
	}
	/* We're not in a comment... should we start one? */
	else
	{
		/* COMMENT_BREAKAGE_VAR */
		if ((ptr[1] == '*') && ptr == start)
		{
			/* Yep. its the start of a comment. */
			in_comment = 1;
			comment_line = load_level[load_depth].line;
		}
		else
		{
			/* Its NOT a comment. Tack it on the end */
			malloc_strcat(&current_row, "/");

			/* Is the / is at the EOL? */
			if (ptr[1] == '\0')
			{
				/* If we are NOT in a block alias, */
				if (!paste_level)
				{
					/* Send the line off to parse_line */
					parse_line(NULL, current_row, flag ? args : get_int_var(INPUT_ALIASES_VAR) ? empty_string : NULL, 0, 0, 1);
					new_free(&current_row);
					ack = 0; /* no semicolon.. */
				}
				else
					/* Just add a semicolon and keep going */
					ack = 1; /* tack on a semicolon */
			}
		}


	}
	no_semicolon = 1 - ack;
	ack = 0;
	break;
}
case '{' :
{
	if (in_comment)
		break;

	/* If we are opening a brand new {} pair, remember the line */
	if (!paste_level)
		paste_line = load_level[load_depth].line;
		
	paste_level++;
	if (ptr == start)
		malloc_strcat(&current_row, " {");
	else
		malloc_strcat(&current_row, "{");
	no_semicolon = 1;
	break;
}
case '}' :
{
	if (in_comment)
		break;
        if (!paste_level)
	{
		int owc = window_display;
		window_display = 1;
		naperror("Unexpected } in %s, line %d", expanded, load_level[load_depth].line);
		window_display = owc;
	}
	else 
	{
        	--paste_level;

		if (!paste_level)
			paste_line = -1;
		malloc_strcat(&current_row, "}"); /* } */
		no_semicolon = ptr[1]? 1: 0;
	}
	break;
}
case ';' :
{
	if (in_comment)
		break;
	malloc_strcat(&current_row, ";");
	if (!ptr[1] && !paste_level)
	{
		parse_line(NULL, current_row, flag ? args : get_int_var(INPUT_ALIASES_VAR) ? empty_string : NULL, 0, 0, 1);
		new_free(&current_row);
	}
	else if (!ptr[1] && paste_level)
		no_semicolon = 0;
	else
		no_semicolon = 1;
	break;
}
	/* End of reformatting */
					}
					start = ptr+1;
				}
				else
				{
					if (!in_comment)
						malloc_strcat(&current_row, start);
					start = NULL;
				}
			}
		}
		if (in_comment)
		{
			int owc = window_display;
			window_display = 1;
			naperror("File %s ended with an unterminated comment in line %d", expanded, comment_line);
			window_display = owc;
		}
		if (current_row)
		{
			if (paste_level)
			{
				int owc = window_display;
				window_display = 1;
				naperror("Unexpected EOF in %s trying to match '[' at line %d",
						expanded, paste_line);
				window_display = owc;
			}
			else
				parse_line(NULL,current_row, flag ? args: get_int_var(INPUT_ALIASES_VAR) ? empty_string : NULL,0,0, 1);
			new_free(&current_row);
		}
		}
		new_free(&expanded);
		fclose(fp);
	}	/* end of the while loop that allows you to load
		   more then one file at a time.. */

	if (get_int_var(DISPLAY_VAR))
		window_display = display;

	status_update(1);
	load_depth--;
}


BUILT_IN_COMMAND(reload_save)
{
char *p = NULL;
char buffer[BIG_BUFFER_SIZE+1];

#ifdef WINNT
	snprintf(buffer, BIG_BUFFER_SIZE, "~/%s", version);
#else	
	snprintf(buffer, BIG_BUFFER_SIZE, "~/.%s", version);
#endif
	p = expand_twiddle(buffer);
	if (p && access(p, F_OK) != 0)
	{
		yell("Created directory %s", p);
		mkdir(p, S_IWUSR|S_IRUSR|S_IXUSR);
	}
	new_free(&p);
#ifdef WINNT
	snprintf(buffer, BIG_BUFFER_SIZE, "~/%s/%s.sav", version, version);
#else
	snprintf(buffer, BIG_BUFFER_SIZE, "~/.%s/%s.sav", version, version);
#endif
	p = expand_twiddle(buffer);
	load(empty_string, p, empty_string, NULL, 0); /*CDE XXX p */
	new_free(&p);
}

void load_scripts(void)
{
	static int done = 0;
	char buffer[BIG_BUFFER_SIZE+1];
	int old_display = window_display;
	if (!done++)
	{
		window_display = 0;
		initial_load = 1;
#ifdef WINNT
		if (!access(bircrc_file, R_OK))
			load("LOAD", bircrc_file, empty_string, NULL, 0);
		else
		{
			char *p;
			strcpy(buffer, bircrc_file);
			if ((p = strrchr(buffer, '.')))
			{
				*p = '-';
				if (!access(buffer, R_OK))
					load("LOAD", buffer, empty_string, NULL, 0);
			}
		}
#else
		if (!access(bircrc_file, R_OK))
			load("LOAD", bircrc_file, empty_string, NULL, 0);
#endif
		sprintf(buffer, "%s/%s/%s", SCRIPT_PATH, version, version);
		load("LOAD", buffer, empty_string, NULL, 0);

		loading_savefile++;
		reload_save(NULL, NULL, empty_string, NULL, 0);
		loading_savefile--;
		window_display = old_display;
	}
	initial_load = 0;
}

BUILT_IN_COMMAND(whois)
{
	if (args && *args)
		whobase(args, 0);
	else if (current_window->server > -1)
		whobase(get_server_nickname(current_window->server), 0);
}

BUILT_IN_COMMAND(whowas)
{
	if (args && *args)
		whobase(args, 1);
	else if (current_window->server > -1)
		whobase(get_server_nickname(current_window->server), 1);
}

BUILT_IN_COMMAND(dnscmd)
{
	if (args && *args)
		dnsbase(args);
	else if (current_window->server > -1)
		dnsbase(get_server_nickname(current_window->server));
}


BUILT_IN_COMMAND(serverclose)
{
	window_discon(current_window, NULL, NULL);
}

BUILT_IN_COMMAND(hotlist)
{
NickStruct *new;
char *nick;
	if (!args || !*args)
	{
		say("Your Hotlist:");
		name_print(NULL, nap_hotlist, 1, 0, NULL);
		return;
	}
	while ((nick = next_arg(args, &args)))
	{
		if (!(*nick == '-'))
		{
			send_ncommand(CMDS_ADDHOTLIST, "%s", nick);
			if (!(new = (NickStruct *)find_in_list((List **)&nap_hotlist, nick, 0)))
			{
				new = new_malloc(sizeof(NickStruct));
				new->nick = m_strdup(nick);
				new->speed = -1;
				add_to_list((List **)&nap_hotlist, (List *)new);
			} else 
				say("%s is already on your Hotlist", nick);
		}
		else if ((*nick == '-'))
		{
			if (!my_stricmp(nick, "-remove"))
				nick = next_arg(args, &args);
			else
				nick++;
			if (nick && *nick && (new = (NickStruct *)remove_from_list((List **)&nap_hotlist, nick)))
			{
				send_ncommand(CMDS_HOTLISTREMOVE, "%s", nick);
				say("Removing %s from your HotList", nick);
				new_free(&new->nick);
				new_free(&new);
			}
		}
	}
}

BUILT_IN_COMMAND(topic)
{
ChannelStruct *ch;
char *cmd;
	if (!args && !current_window && !current_window->current_channel)
		return;
	cmd = next_arg(args, &args);
	ch = (ChannelStruct *)find_in_list((List **)&current_window->nchannels, cmd ? cmd : current_window->current_channel ? current_window->current_channel : empty_string, 0);
	if (ch)
	{
		if (args && *args)
			send_ncommand(CMDS_TOPIC, "%s %s", ch->channel, args);
		else
			say("Topic for %s: %s", ch->channel, ch->topic);
	}
	else
		say("No Channel found %s", cmd ? cmd : empty_string);
}


BUILT_IN_COMMAND(ping)
{
char *cmd;
	if ((cmd = next_arg(args, &args)))
	{
		PingStruct *new;
		if (!(new = (PingStruct *)find_in_list((List **)&ping_time, cmd, 0)))
		{
			new = new_malloc(sizeof(PingStruct));
			new->nick = m_strdup(cmd);
			add_to_list((List **)&ping_time, (List *)new);
		}
		get_time(&new->start);
		send_ncommand(CMDS_PING, "%s %s", cmd, args ? args : empty_string);
	}
}

BUILT_IN_COMMAND(spingcmd)
{
char *cmd;
struct timeval tv;

	get_time(&tv);
	if ((cmd = next_arg(args, &args)))
	{
		if (!my_strnicmp(cmd, "-A", 2) || (command && !my_stricmp(command, "MPING")))
		{
			send_ncommand(CMDS_SERVERMPING, NULL);
			return;
		}
		send_ncommand(CMDS_SPING, "%s %lu %lu", cmd, tv.tv_sec, tv.tv_usec);
	}
	else
		send_ncommand(CMDS_SPING, "%s %lu %lu", get_server_name(from_server), tv.tv_sec, tv.tv_usec);
}

BUILT_IN_COMMAND(rawcmd)
{
char	*cmd;
int	x = 0;
int	serv = from_server, 
	fd = -1;

	while ((cmd = next_arg(args, &args)))
	{
		if (*cmd == '-')
		{
			if (!my_strnicmp(cmd, "server", 4) && args && *args)
				serv = my_atol(next_arg(args, &args));
			else if (!my_strnicmp(cmd, "rawfd", 4) && args && *args)
				fd = my_atol(next_arg(args, &args));
		}
		else 
		{
			x = my_atol(cmd);
			break;
		}
	}
	
	if (x != 0)
	{
		int old_serv = from_server;
		from_server = serv;
		if (fd != -1)
		{
			char buffer[BIG_BUFFER_SIZE+1];
			int writelen;
			N_DATA n_data = {0};
			*buffer = 0;
			if (args && *args)
				snprintf(buffer, BIG_BUFFER_SIZE, "%s", args);
			n_data.len = writelen = strlen(buffer);
			n_data.len = BSWAP16(n_data.len);
			n_data.command = BSWAP16(x);
			write(fd, &n_data, sizeof(n_data));
			if (writelen)
				write(fd, buffer, writelen);
		}
		else
			send_ncommand(x, "%s", args);
		from_server = old_serv;
	}
}

BUILT_IN_COMMAND(nap_stats)
{
int i;
N_STATS *s_stats;
	say("Client Uptime is %s", convert_time(now - start_time));
	for (i = 0; i < server_list_size(); i++)
	{
		if ((s_stats = get_server_stats(i)) && s_stats->libraries)
			say("There are %d libraries with %d songs in %dgb on %s", s_stats->libraries, s_stats->songs, s_stats->gigs, get_server_name(i));
	}
	say("We are sharing %d for %4.2f%s", shared_stats.shared_files, _GMKv(shared_stats.shared_filesize), _GMKs(shared_stats.shared_filesize));
	say("There are %d files loaded with %4.2f%s", shared_stats.total_files, _GMKv(shared_stats.total_filesize), _GMKs(shared_stats.total_filesize));
	say("We have served %lu files and %4.2f%s", shared_stats.files_served, _GMKv(shared_stats.filesize_served), _GMKs(shared_stats.filesize_served));
	say("We have downloaded %lu files for %4.2f%s", shared_stats.files_received, _GMKv(shared_stats.filesize_received), _GMKs(shared_stats.filesize_received));
	say("The Highest download speed has been %4.2fK/s from %s", _GMKv(shared_stats.max_downloadspeed), shared_stats.max_downloadspeed_nick ? shared_stats.max_downloadspeed_nick : "N/A");
	say("The Highest upload speed has been %4.2fK/s from %s", _GMKv(shared_stats.max_uploadspeed), shared_stats.max_uploadspeed_nick ? shared_stats.max_uploadspeed_nick : "N/A");
	if (get_int_var(PTEST_VAR))
	{
		for (i = 0; i < server_list_size(); i++)
		{
			if (!is_connected(i))
				continue;
			s_stats = get_server_stats(i);
			say("%d total unreach. (%d pending / %d switched / %d zero'ed / %d bogus / %d dupes).",
			   s_stats->total_unreach_messages, s_stats->total_unreach_pending,
			   s_stats->total_unreach_real - s_stats->total_unreach_switch_to_zero,
			   s_stats->total_unreach_switch_to_zero, s_stats->total_unreach_bogus,
  			   s_stats->total_unreach_duplicates);
		}
	}
}

BUILT_IN_COMMAND(evalcmd)
{

	parse_line(NULL, args, subargs ? subargs : empty_string, 0, 0, 0);
}

BUILT_IN_COMMAND(query)
{
	window_query(current_window, &args, NULL);
	build_status(current_window, NULL, 0);
}

BUILT_IN_COMMAND(my_clear)
{
	char	*arg;
	int	all = 0,
		scrollback = 0,
		unhold = 0;

	
	while ((arg = next_arg(args, &args)) != NULL)
	{
		if (!my_strnicmp(arg+1, "A", 1))
			all = 1;
		else if (!my_strnicmp(arg+1, "U", 1))
			unhold = 1;
		else if (!my_strnicmp(arg+1, "S", 1))
			scrollback = 1;
	}
	if (all)
		clear_all_windows(unhold, scrollback);
	else
	{
		if (scrollback)
			clear_scrollback(get_window_by_refnum(0));
		if (unhold)
			hold_mode(NULL, OFF, 1);
		clear_window_by_refnum(0);
	}
	update_input(UPDATE_JUST_CURSOR);
}

BUILT_IN_COMMAND(relmcmd)
{
	if (args && *args)
	{
		if (!my_stricmp(args, "-clear"))
			clear_servermsg();
		else
			display_servermsgs();
	}
	else
		display_servermsgs();
}

BUILT_IN_COMMAND(queuecmd)
{
extern Server *server_list;
GetFile **queue = NULL, *tmp, *last;
int count = 1;
int delete = 0;
char *who = NULL;
	if (from_server == -1)
		return;
	if (server_list[from_server].users)
		queue = &server_list[from_server].users->Queued;
	while (args && *args)
	{
		who = next_arg(args, &args);
		if (!my_stricmp(who, "-remove"))
		{
			delete = 1;
			if (!(who  = next_arg(args, &args)))
				return;
		} else if (!my_stricmp(who, "-clear"))
			delete = -1;
	}
		if (queue && *queue)
		{
			say("Download Queue");
			for (tmp = *queue; tmp; tmp = last)
			{
				last = tmp->next;
				switch (delete)
				{
					case 1:
						if (!wild_match(who, tmp->nick) && !wild_match(who, tmp->filename))
							continue;
					case -1:
					{
						break_from_list((List **)queue, (List *)tmp);
						nap_finished_file(tmp->socket, PREMATURE_FINISH);
						count++;
						break;
					}
					default:
						put_it("%3d %3d %16s %s", count, now - tmp->addtime, tmp->nick, base_name(tmp->filename));
						count++;
						break;
				}
			}
		}
	if (delete && count)
		say("Removed %d entries from download queue", count);
}

BUILT_IN_COMMAND(echocmd)
{
int	owd = window_display,
	all_windows = 0,
	banner = 0,
	no_log = 0,
	more = 1,
	xtended = 0,
	old_und = 0,
	old_rev = 0,
	old_bold = 0,
	old_color = 0,
	old_blink = 0,
	old_ansi = 0;
unsigned long	lastlog_level = 0;
unsigned long	from_level = 0;
unsigned long	temp;

unsigned long	saved_level;
const char	*saved_from;
char 		*stuff = NULL,
		*flag_arg;
	

	window_display = 1;
	save_display_target(&saved_from, &saved_level);

	if (command && *command == 'X')
	{
		while (more && args && (*args == '-' || *args == '/'))
		{
			switch(toupper(args[1]))
			{
				case 'C':
				{
					next_arg(args, &args);
					target_window = current_window;
					break;
				}
				case 'L':
				{
					flag_arg = next_arg(args, &args);

					if (!(flag_arg = next_arg(args, &args)))
						break;
					if ((temp = parse_lastlog_level(flag_arg, 0)))
					{
						lastlog_level = 
							set_lastlog_msg_level(temp);
						from_level = message_from_level(temp);
					}
					break;
				}
				case 'W':
				{
					flag_arg = next_arg(args, &args);
					if (!(flag_arg = next_arg(args, &args)))
						break;
					if (isdigit(*flag_arg))
						target_window = get_window_by_refnum(my_atol(flag_arg));
					else
						target_window = get_window_by_name(flag_arg);
					break;
				}
				case 'T':
				{
					flag_arg = next_arg(args, &args);
					if (!(flag_arg = next_arg(args, &args)))
						break;
					target_window = get_window_target_by_desc(flag_arg);
					break;
				}
				case 'A':
				case '*':
				{
					next_arg(args, &args);
					all_windows = 1;
					break;
				}
				case 'X': /* X -- allow all attributes to be outputted */
				{
					next_arg(args, &args);

					old_und = get_int_var(UNDERLINE_VIDEO_VAR);
					old_rev = get_int_var(INVERSE_VIDEO_VAR);
					old_bold = get_int_var(BOLD_VIDEO_VAR);
					old_color = get_int_var(COLOR_VAR);
					old_blink = get_int_var(BLINK_VIDEO_VAR);
					old_ansi = get_int_var(DISPLAY_ANSI_VAR);

					set_int_var(UNDERLINE_VIDEO_VAR, 1);
					set_int_var(INVERSE_VIDEO_VAR, 1);
					set_int_var(BOLD_VIDEO_VAR, 1);
					set_int_var(COLOR_VAR, 1);
					set_int_var(BLINK_VIDEO_VAR, 1);
					set_int_var(DISPLAY_ANSI_VAR, 1);

					xtended = 1;
					break;
				}
				case 'B':
				{
					next_arg(args, &args);
					banner = 1;
					break;
				}
				case 'N':
				{
					next_arg(args, &args);
					no_log = 1;
					break;
				}
				case '-':
				{
					next_arg(args, &args);
					more = 0;
					break;					
				}
				default:
				{
					more = 0;
					break;
				}
			}
			if (!args)
				args = empty_string;				
		}
	}

	if (no_log)
		inhibit_logging = 1;
		 
	if (banner == 1)
	{
		malloc_strcpy(&stuff, numeric_banner(-current_numeric));
		if (*stuff)
		{
			m_3cat(&stuff, space, args);
			args = stuff;
		}
	} 
	else if (banner != 0)
		abort();
		
	if (all_windows == 1)
	{
		Window *win = NULL;
		while ((traverse_all_windows(&win)))
		{
			target_window = win;
			put_echo(args);
		}
	} 
	else if (all_windows != 0)
		abort();
	else
		put_echo(args);
	if (xtended)
	{
		set_int_var(UNDERLINE_VIDEO_VAR, old_und);
		set_int_var(INVERSE_VIDEO_VAR, old_rev);
		set_int_var(BOLD_VIDEO_VAR, old_bold);
		set_int_var(COLOR_VAR, old_color);
		set_int_var(BLINK_VIDEO_VAR, old_blink);
		set_int_var(DISPLAY_ANSI_VAR, old_ansi);
	}
	if (stuff) 
		new_free(&stuff);
	if (lastlog_level)
	{
		set_lastlog_msg_level(lastlog_level);
		message_from_level(from_level);
	}
	if (no_log)
		inhibit_logging = 0;
	set_display_target(saved_from, saved_level);
	window_display = owd;
}

BUILT_IN_COMMAND(beepcmd)
{
	term_beep();
}

BUILT_IN_COMMAND(nslookup)
{
#if defined(WANT_THREAD) && defined(WANT_NSLOOKUP)
char *host;
char *cmd = NULL;
	while ((host = next_arg(args, &args)))
	{
		if (*host == '-' || *host == '/')
		{
			host++;
			if (!my_stricmp(host, "cmd"))
			{
				if (!(cmd = next_expr(&args, '{')))
					say("Need {...} for -CMD argument");
				else
					host = next_arg(args, &args);
			}
			else
				continue;
		}
		if (!host)
			break;
		do_nslookup(NULL, host, cmd);
	}
#else
	say("This Command Disabled due to lack of Thread support");
#endif
}

BUILT_IN_COMMAND(ptestcmd)
{
#if defined(WANT_THREAD) && defined(WANT_PTEST)
char *arg = NULL, *host= NULL, *port = NULL;
char *cmd = NULL;
	if (!get_int_var(PTEST_VAR))
	{
		say("Enable PTEST before using");
		return;
	}
	while ((arg = next_arg(args, &args)))
	{
		if (*arg == '-' || *arg == '/')
		{
			arg++;
			if (!my_stricmp(arg, "cmd"))
			{
				if (!(cmd = next_expr(&args, '{')))
					say("Need {...} for -CMD argument");
				else
				{
					if (!(arg = next_arg(args, &args)))
						break;
				}
			}
			else
				continue;
		}
		if (strchr(arg, '.'))
		{
			char *p;
			host = arg;
			if ((p = strchr(host, ':')))
			{
				port = p;
				*port++ = 0;
			}	
		}
		else
			port = arg;
		if (host && port)
		{
			do_ptestcall(host, port, cmd);
			host = port = NULL;
		}
	}
#else
	say("This Command Disabled due to lack of Thread support");
#endif
}

BUILT_IN_COMMAND(mychdir)
{
	char	*arg,
		*expand;
	char buffer[BIG_BUFFER_SIZE + 1];

	
	*buffer = 0;
	if ((arg = new_next_arg(args, &args)) != NULL && *arg)
	{
		if ((expand = expand_twiddle(arg)) != NULL)
		{
			if (chdir(expand))
			{
				say("CD: %s %s", arg, strerror(errno));
				new_free(&expand);
				return;
			}
			new_free(&expand);
		}
		else
		{
			say("CD No such dir %s", arg);
			return;
		}
	}
	getcwd(buffer, BIG_BUFFER_SIZE);
	say("Current directory: %s", buffer);
}

BUILT_IN_COMMAND(hookcmd)
{
	if (*args)
		do_hook(HOOK_LIST, "%s", args);
	else
		say("Usage: /HOOK [text]");
}

/*
 * eval_inputlist:  Cute little wrapper that calls parse_line() when we
 * get an input prompt ..
 */
static void	eval_inputlist (char *args, char *line)
{
	parse_line(NULL, args, line ? line : empty_string, 0, 0, 1);
}


BUILT_IN_COMMAND(inputcmd)
{
	char	*prompt;
	int	wait_type = WAIT_PROMPT_KEY;
	char	*argument;
	int	echo = 1;

	if (!strcmp(command, "INPUT"))
		wait_type = WAIT_PROMPT_LINE;

	while (*args == '-')
	{
		argument = next_arg(args, &args);
		if (!my_stricmp(argument, "-noecho"))
			echo = 0;
	}

	if (!(prompt = new_next_arg(args, &args)))
	{
		say("Usage: %s \"prompt\" { commands }", command);
		return;
	}

	while (my_isspace(*args))
		args++;

	add_wait_prompt(prompt, eval_inputlist, args, wait_type, echo);
}

BUILT_IN_COMMAND(reset)
{
	refresh_screen(0, NULL);
}

static        int     e_pause_cb_throw = 0;
static        void    e_pause_cb (char *u1, char *u2) { e_pause_cb_throw--; }

BUILT_IN_COMMAND(e_pause)
{
	char *sec;
	long milliseconds;
struct timeval start;

	if (!(sec = next_arg(args, &args)))
	{
		int	c_level = e_pause_cb_throw;

		add_wait_prompt(empty_string, e_pause_cb, 
				NULL, WAIT_PROMPT_DUMMY, 0);
		e_pause_cb_throw++;
		while (e_pause_cb_throw > c_level)
			io("pause");
		return;
	}

	milliseconds = (long)(atof(sec) * 1000);
	get_time(&start);
	start.tv_usec += (milliseconds * 1000);
	start.tv_sec += (start.tv_usec / 1000000);
	start.tv_usec %= 1000000;

	/* 
	 * I use comment here simply becuase its not going to mess
	 * with the arguments.
	 */
	add_timer(0, empty_string, milliseconds, 1, (int (*)(void *, char *))comment, NULL, NULL, get_current_winref(), "pause");
	while (time_diff(get_time(NULL), start) > 0)
		io("e_pause");
}

BUILT_IN_COMMAND(breakcmd)
{
	if (!will_catch_break_exceptions)
		say("Cannot BREAK here.");
	else
		break_exception++;
}

BUILT_IN_COMMAND(continuecmd)
{
	if (!will_catch_continue_exceptions)
		say("Cannot CONTINUE here.");
	else
		continue_exception++;
}

BUILT_IN_COMMAND(returncmd)
{
	if (!will_catch_return_exceptions)
		say("Cannot RETURN here.");
	else
	{
		if (args && *args)
			add_local_alias("FUNCTION_RETURN", args, 0);
		return_exception++;
	}
}

BUILT_IN_COMMAND(sleepcmd)
{
	char	*arg;

	
	if ((arg = next_arg(args, &args)) != NULL)
		sleep(atoi(arg));
}

BUILT_IN_COMMAND(usleepcmd)
{
	char 		*arg;
	struct timeval 	pause;
	time_t		nms;

	if ((arg = next_arg(args, &args)))
	{
		nms = (time_t)my_atol(arg);
		pause.tv_sec = nms / 1000000;
		pause.tv_usec = nms % 1000000;
		select(0, NULL, NULL, NULL, &pause);
	}
	else
		say("Usage: USLEEP <usec>");
}

/*
   This isnt a command, its used by the wait command.  Since its extern,
   and it doesnt use anything static in this file, im sure it doesnt
   belong here.
 */
void oh_my_wait (int servnum)
{
	int w_server;

	if ((w_server = servnum) == -1)
		w_server = from_server;

	if (is_connected(w_server))
	{
		int old_from_server = from_server;
				
		inc_server_waiting_out(w_server);
		lock_stack_frame();
		send_ncommand(1000, NULL);
		while (server_waiting_in(w_server) < server_waiting_out(w_server))
			io("oh_my_wait");
		from_server = old_from_server;
	}
}

BUILT_IN_COMMAND(waitcmd)
{
	char	*ctl_arg = next_arg(args, &args);

	if (from_server == -1)
		return;
	if (ctl_arg && !my_strnicmp(ctl_arg, "-c", 2))
	{
		WaitCmd	*new_wait;

		new_wait = (WaitCmd *) new_malloc(sizeof(WaitCmd));
		new_wait->stuff = m_strdup(args);
		new_wait->next = NULL;

		if (end_wait_list)
			end_wait_list->next = new_wait;
		end_wait_list = new_wait;
		if (!start_wait_list)
			start_wait_list = new_wait;
		send_ncommand(1000, NULL);
	}

	else if (ctl_arg && !my_strnicmp(ctl_arg, "for", 3))
	{
		clear_sent_to_server(from_server);
		parse_line(NULL, args, subargs, 0, 0, 1);
		if (sent_to_server(from_server))
			oh_my_wait(from_server);
		clear_sent_to_server(from_server);
	}

	else if (ctl_arg && *ctl_arg == '%')
	{
		int	w_index = is_valid_process(ctl_arg);

		if (w_index != -1)
		{
			if (args && *args)
			{
				if (!my_strnicmp(args, "-cmd", 4))
					next_arg(args, &args);
				add_process_wait(w_index, args?args:empty_string);
			}
			else
			{
#if 0
				set_input(empty_string);
#endif
				lock_stack_frame();
				while (process_is_running(ctl_arg))
					io("wait %proc");
				unlock_stack_frame();
			}
		}
	}
	else if (ctl_arg)
		yell("Unknown argument to /WAIT");
	else
	{
		oh_my_wait(from_server);
		clear_sent_to_server(from_server);
	}
}

int check_wait_command(char *nick)
{
	if ((server_waiting_out(from_server) > server_waiting_in(from_server)) && !strcmp(nick, "Unknown command code 1000"))
	{
		inc_server_waiting_in(from_server);
		unlock_stack_frame();
	        return 1;
	}
	if (start_wait_list && !strcmp(nick, "Unknown command code 1000"))
	{
		WaitCmd *old = start_wait_list;

		start_wait_list = old->next;
		if (old->stuff)
		{
			parse_line("WAIT", old->stuff, empty_string, 0, 0, 1);
			new_free(&old->stuff);
		}
		if (end_wait_list == old)
			end_wait_list = NULL;
		new_free((char **)&old);
		return 1;
	}
	return 0;
}

/* The SENDLINE command.. */
BUILT_IN_COMMAND(sendlinecmd)
{
	int	server;
	int	display;

	server = from_server;
	display = window_display;
	window_display = 1;
	if (args && *args)
		parse_line(NULL, args, get_int_var(INPUT_ALIASES_VAR) ? empty_string : NULL, 1, 0, 1);
	update_input(UPDATE_ALL);
	window_display = display;
	from_server = server;
}

BUILT_IN_COMMAND(listusers)
{
char *server = NULL, *arg, *chan = NULL, *ip = NULL;
char *buff = NULL;
char *cmd = NULL;

	if (!args)
		return;
	
	while ((arg = next_arg(args, &args)))
	{
		if (!(*arg == '-') && !server)
			server = arg;
		else if (!my_strnicmp(arg, "-Leech", 2))
			m_s3cat(&buff, space, "l");
		else if (!my_strnicmp(arg, "-User", 2))
			m_s3cat(&buff, space, "u");
		else if (!my_strnicmp(arg, "-Moderator", 2))
			m_s3cat(&buff, space, "m");
		else if (!my_strnicmp(arg, "-Admin", 2))
			m_s3cat(&buff, space, "a");
		else if (!my_strnicmp(arg, "-Elite", 2))
			m_s3cat(&buff, space, "e");
		else if (!my_strnicmp(arg, "-Cloak", 3))
			m_s3cat(&buff, space, "c");
		else if (!my_strnicmp(arg, "-Muzzled", 2))
			m_s3cat(&buff, space, "z");
		else if (!my_strnicmp(arg, "-Ip", 2))
		{
			if ((ip = next_arg(args, &args)))
			{
				m_s3cat(&buff, space, "i");
				m_s3cat(&buff, space, ip);
			} else
				goto got_error;
		}
		else if (!my_strnicmp(arg, "-Channel", 3))
		{
			if ((chan = next_arg(args, &args)))
			{
				m_s3cat(&buff, space, "C");
				m_s3cat(&buff, space, chan);
			}
			else
				goto got_error;
		}
		else if (!my_strnicmp(arg, "-cmd", 3))
		{
			cmd = next_expr(&args, '{');
			if (!cmd)
			{
				say("Missing cmd {}");
				return;
			}
		}
		else 
			set_server_showuser(from_server, arg, -1);
	}
#if 0
	send_ncommand(CMDS_SHOWUSERS, "%s%s%s%s%s%s%s", server ? server : star, flag ? space : empty_string, flag ? flag : empty_string, (flag && chan) ? space : empty_string, (flag && chan) ? chan : empty_string, 
		(flag && ip) ? space : empty_string, (flag && ip) ? ip : empty_string);
#else

	set_server_showusercmd(from_server, cmd);
	send_ncommand(CMDS_SHOWUSERS, "%s%s%s", server ? server : star, 
						buff ? space : empty_string,
						buff ? buff : empty_string);
#endif
got_error:
	new_free(&buff);
}

BUILT_IN_COMMAND(listop)
{
char *chan;
	chan = next_arg(args, &args);
	if (!chan)
		chan = current_window->current_channel;
	if (!chan)
	{
		say("Specify a channel as you are not on a channel");
		return;
	}
	send_ncommand(CMDS_CREATEOP, "%s", chan);
}

BUILT_IN_COMMAND(createop)
{
char *chan;
	chan = next_arg(args, &args);
	if (!chan)
		chan = current_window->current_channel;
	if (chan)
		send_ncommand(CMDS_CREATEOP, "%s %s", chan, args && *args ? args : empty_string);
}

BUILT_IN_COMMAND(delop)
{
char *chan;
	chan = next_arg(args, &args);
	if (args && *args)
		send_ncommand(CMDS_DELETEOP, "%s %s", chan, args);
	else
		say("Who's your Daddy?");
}

BUILT_IN_COMMAND(variablecmd)
{
	char *chan;
	if (!(chan = next_arg(args, &args)))
		chan = current_window->current_channel;
	if (chan)
		send_ncommand(numeric, "%s %s", chan, args && *args ? args : empty_string);
}

BUILT_IN_COMMAND(nadmin)
{
int	i;
char	*comm,
	*user;

typedef struct _Nadmin {
	char	*command;
	int	cmd;
	int	arg_count;
	int	len;
	char	*helparg;
} Nadmin;

Nadmin admin_comm[] = {
	{ "killserver",	CMDS_SERVERKILL,	-1,	5, HELP_KILLSERVER },
	{ "banuser",	CMDS_BANUSER,		1,	4, HELP_BANUSER },
	{ "connect",	CMDS_SERVERLINK,	-1,	4, HELP_SCONNECT },
	{ "disconnect",	CMDS_SERVERUNLINK,	-1,	4, HELP_SDISCONNECT },
	{ "config",	CMDS_SETCONFIG,		-1,	4, HELP_CONFIG },
	{ "unnukeuser",	CMDS_UNNUKEUSER,	1,	3, HELP_UNNUKEUSER },
	{ "unbanuser",	CMDS_UNBANUSER,		1,	3, HELP_UNBANUSER },
	{ "removeserver",CMDS_SERVERREMOVE,	-1,	3, HELP_REMOVESERVER },
	{ "version",	CMDS_SERVERVERSION,	0,	1, HELP_SVERSION },
	{ "links",	CMDS_SERVERLINKS,	-1,	1, HELP_LINKS },
	{ "reload",	CMDS_RELOADCONFIG,	-1,	3, HELP_SRELOAD },
	{ "nukeuser",	CMDS_NUKEUSER,		1,	1, HELP_NUKEUSER },
	{ "banlist",	CMDS_BANLIST,		-1,	1, HELP_BANLIST },
	{ "speed",	CMDS_CHANGESPEED,	1,	2, HELP_SPEED },
	{ "password",	CMDS_CHANGEPASS,	1,	2, HELP_PASSWORD },
	{ "email",	CMDS_CHANGEEMAIL,	1,	2, HELP_EMAIL },
	{ "dataport",	CMDS_CHANGEDATA,	1,	2, HELP_DATAPORT },
	{ "clearchannel",CMDS_CLEARCHANNEL,	2,	3, HELP_CLEARCHANNEL },
	{ "stats",	CMDS_SERVERUSAGE,	-1,	2, HELP_SERVERSTATS },
	{ "register",	CMDS_ADMINREGISTER,	2,	3, HELP_REGISTER },
	{ "rehash",	CMDS_SERVERREHASH,	0,	3, HELP_REHASH },
	{ "drop",	CMDS_DROPCHANNEL,	1,	3, HELP_DROP },
	{ "cversion",   CMDS_CLIENT_VERSION,	0,	3, HELP_CVERSION },

	{ "histogram",	CMDS_HISTOGRAM,		0,	3, HELP_HISTOGRAM },
	{ "classlist",	CMDR_CLASSLIST,		0,	3, HELP_CLASSLIST },
	{ "ilinelist",	CMDR_ILINELIST,		0,	3, HELP_ILINELIST },
	{ "dlinelist",	CMDR_DLINELIST,		0,	3, HELP_DLINELIST },
	{ "elinelist",	CMDR_ELINELIST,		0,	3, HELP_ELINELIST },

	{ "addclass",	CMDR_CLASSADD,		2,	4, HELP_ADDCLASS },
	{ "addiline",	CMDR_ILINEADD,		1,	4, HELP_ADDILINE },
	{ "adddline",	CMDR_DLINEADD,		1,	4, HELP_ADDDLINE },
	{ "addeline",	CMDR_ELINEADD,		1,	4, HELP_ADDELINE },

	{ "delclass",	CMDR_CLASSDELETE,	1,	4, HELP_DELCLASS },
	{ "deliline",	CMDR_ILINEDEL,		1,	4, HELP_DELILINE },
	{ "deldline",	CMDR_DLINEDEL,		1,	4, HELP_DELDLINE },
	{ "deleline",	CMDR_ELINEDEL,		1,	4, HELP_DELELINE },

	{ "showallchannels",CMDS_SHOWALLCHANNELS, -1,	2, HELP_SHOWALLCHANNELS },
	{ NULL,		0,			-1,	0, NULL }
};
				
	if (!(comm = next_arg(args, &args)))
	{
		char buffer[BIG_BUFFER_SIZE+1];
		int buflen, count = 0;
		*buffer = 0;
		bitchsay("Admin commands:");
		for (i = 0; admin_comm[i].command; i++)
		{
			buflen = strlen(buffer);
			sprintf(buffer+buflen, "%16s ", admin_comm[i].command);
			if (count++ > 2)
			{
				put_it("%s", buffer);
				*buffer = 0;
				count = 0;
			}
		}
		if (*buffer)
			put_it("%s", buffer);
		return;
	}
	if (!my_stricmp(comm, "-help"))
	{
		bitchsay("%s", helparg);
		return;
	}
	for (i = 0; admin_comm[i].command; i++)
	{
		if (!my_strnicmp(admin_comm[i].command, comm, admin_comm[i].len))
		{
			if (args && !my_stricmp(args, "-help"))
			{
				bitchsay("Usage: /admin %s %s", comm, admin_comm[i].helparg? admin_comm[i].helparg:"Undocumented");
				bitchsay("\t /admin  to list all the admin commands");
				return;
			}
			switch(admin_comm[i].arg_count)
			{
				case 0:
				{
					send_ncommand(admin_comm[i].cmd, NULL);
					return;
				}
				case 1:
				{
					if ((user = next_arg(args, &args)))
						send_ncommand(admin_comm[i].cmd, "%s", user);
					else
						say("Nothing to send for %s", admin_comm[i].command);
					return;
				}
				case 2:
				{
					user = next_arg(args, &args);
					if (args && *args)
						send_ncommand(admin_comm[i].cmd, "%s \"%s\"", user, args);
					else
						send_ncommand(admin_comm[i].cmd, "%s", user);
					return;
				}
				case -1:
				{
					if (args && *args)
						send_ncommand(admin_comm[i].cmd, "%s", args);
					else
						send_ncommand(admin_comm[i].cmd, NULL);
					return;
				}
			}
			say("Unknown /admin command %s", comm);
			return;
		}
	}
}

BUILT_IN_COMMAND(bancmd)
{
char *nick, *n, *t = NULL;
time_t timeout = 0;

	nick = next_arg(args, &args);
	if (command && !my_stricmp(command, "TBAN"))
	{
		char ti[40];
		int count = 0;
		unsigned long mult = 0;
		if (!(t = next_arg(args, &args)))
			return;
		while (t && *t)
		{
			if (isdigit(*t))
				ti[count++] = *t++;
			else
			{
				ti[count++] = 0;
				switch(*t)
				{
					case 'd':
					case 'D':
						mult += 60 * 60 * 24 * atol(ti);
						break;
					case 'M':
					case 'm':
						mult += 60 * atol(ti);
						break;
					case 's':
					case 'S':
						mult += atol(ti);
						break;
					case 'y':
					case 'Y':
						mult += 60 * 60 * 24 * 365 * atol(ti);
						break;
					case 'h':
					case 'H':
						mult += 60 * 60 * atol(ti);
						break;
					case 'w':
					case 'W':
						mult += 7 * 60 * 60 * 24 * atol(ti);
						break;
					default:
						mult = atol(ti);
				}
				t++;
				count = 0;
			}			
		}
		if (!mult)
			timeout = atol(ti);
		else
			timeout = mult;
		if (!timeout)
		{
			say("Specify a timeout period for the ban");
			return;
		}
	}
	while ((n = next_in_comma_list(nick, &nick)))
	{
		if (!n || !*n)
			break;

		if (args && *args)
		{
			if (timeout)
				send_ncommand(CMDS_BANUSER, "%s \"%s\" %lu", n, args, timeout);
			else
				send_ncommand(CMDS_BANUSER, "%s \"%s\"", n, args);
		}
		else
		{
			if (timeout)
				send_ncommand(CMDS_BANUSER, "%s \"\" %lu", n, timeout);
			else
				send_ncommand(CMDS_BANUSER, "%s", n);
		}
	}
}

BUILT_IN_COMMAND(opsay_cmd)
{
	if (!args || !*args)
		return;
	if (current_window && current_window->current_channel)
	{
		do_hook(SEND_OPS_LIST, "%s %s", current_window->current_channel, args);
		send_ncommand(CMDS_OPWALL, "%s %s", current_window->current_channel, args);
	}
}

BUILT_IN_COMMAND(mode_cmd)
{
char *mode[] = {"MODERATED", "PRIVATE", "TOPIC", "INVITE", "REGISTERED", NULL};
char *new_mode = NULL;
char *chan = NULL;
unsigned int i;
	if (args && *args)
	{
		int plus_mode = 0;		
		char *m;
		if (get_server_ircmode(from_server))
		{
			send_to_server("MODE %s", args);
			return;
		}
		while (args && *args)
		{
			switch (*args)
			{
				case '+':
				case '-':
					if (*args == '+')
						plus_mode = 1;
					else
						plus_mode = -1;
					args++;
					m = next_arg(args, &args);
					if (m)
					{
						for (i = 0; mode[i]; i++)
						{
							if (!my_strnicmp(m, mode[i], 1))
							{
								m_s3cat(&new_mode, space, plus_mode == 1 ? "+" : "-");
								malloc_strcat(&new_mode, mode[i]);
							}
						}
					}
					break;
				default:
					chan = next_arg(args, &args);
					break;
			}
		}

		if (new_mode)
		{
			if (chan)
				send_ncommand(CMDS_CHANNELMODE, "%s %s", chan, new_mode);
			else if (current_window && current_window->current_channel)
				send_ncommand(CMDS_CHANNELMODE, "%s %s", current_window->current_channel, new_mode);
			new_free(&new_mode);
			return;
		}
	}
	if (chan || (current_window && current_window->current_channel))
	{
		if (get_server_ircmode(from_server))
			send_to_server("MODE %s", chan ? chan : current_window->current_channel);
		else
			send_ncommand(CMDS_CHANNELMODE, "%s", chan ? chan : current_window->current_channel);
	}
}

BUILT_IN_COMMAND(invite_cmd)
{
char *chan = NULL, *arg, *nick = NULL;
/*	10210 <channel> <nick> */
	if (!(arg = next_arg(args, &args)))
		return;
	if (args && *args)
	{
		chan = arg;
		nick = next_arg(args, &args);
	}
	else
	{
		nick = arg;
		chan = current_window->current_channel;
	}
	if (chan && nick)
	{
		char *n = NULL;
		say("Inviting %s to %s", nick, chan);
		while ((n = next_in_comma_list(nick, &nick)))
		{
			if (!n || !*n)
				break;
			send_ncommand(CMDS_CHANNELINVITE, "%s %s", chan, n);
		}
	}
}

BUILT_IN_COMMAND(setuser)
{
char *nick, *level, *n;
	nick = next_arg(args, &args);
	level = next_arg(args, &args);
	if (nick && level)
	{
		while ((n = next_in_comma_list(nick, &nick)))
		{
			if (!n || !*n)
				break;
			send_ncommand(numeric, "%s %s", n, level);
		}
	}
}

void get_range(char *line, int *start, int *end)
{
char *q = line, *p = line;
	while (*p && isdigit(*p))
		p++;
	if (*p)
		*p++ = 0;
	*start = my_atol(q);
	*end = *p? my_atol(p): *start;
	if (*end < *start)
		*end = *start;
}

BUILT_IN_COMMAND(pastecmd)
{
	char	*lines;
	char	*channel = NULL;
	Window	*win;
	int	winref = 0;
	int	line = 1;
	int	topic = 0;
	int	start_line = 1,
		end_line = 1,
		count;
	Display	*start_pos;

	if ((lines = next_arg(args, &args)))
		get_range(lines, &start_line, &end_line);
	if (!args || !*args)
		channel = get_current_channel_by_refnum(0);
	else
	{
		char *t;
		while (args && *args)
		{
			t = next_arg(args, &args);
			if (*t == '-')
			{
				if (!my_strnicmp(t, "-win", strlen(t)))
				{
					if (*args && isdigit(*args))
					{
						t = next_arg(args, &args);
						winref = my_atol(t);
					}
				}
				else if (!my_strnicmp(t, "-topic", strlen(t)))
				{
					topic = 1;
				}
			}
			else 
				channel = t;
		}
	}
	if (!channel && !(channel = get_current_channel_by_refnum(0)))
		return;
	win = get_window_by_refnum(winref);

	line = end_line;
	
	if (line < 1 || line > win->display_buffer_size)
	{
		say("Try a realistic number within your scrollbuffer");
		return;
	}
	start_pos = get_screen_hold(win);
	for (start_pos = start_pos ? start_pos->prev : win->display_ip->prev; line > 1; start_pos = start_pos->prev)
		line--;

	if (!start_pos)
		start_pos = win->display_ip->prev;

	count = end_line - start_line + 1;
	if (count == 1 && topic && start_pos)
	{
		send_ncommand(CMDS_TOPIC, "%s %s", channel, start_pos->line);
		return;
	}
	line = 0;
	while (count)
	{
		line++;
		if (start_pos && start_pos->line)
		{
			if (do_hook(PASTE_LIST, "%s %s", channel, start_pos->line))
			{
				char buffer[BIG_BUFFER_SIZE+1];
				snprintf(buffer, BIG_BUFFER_SIZE, "[%3d] %s", line, start_pos->line);
				send_text(channel, buffer, NULL, 1, 0);
			}
			start_pos = start_pos->next;
		}
		count--;
	}
}

BUILT_IN_COMMAND(stackcmd)
{
	char	*arg;
	int	len, type;

	if ((arg = next_arg(args, &args)) != NULL)
	{
		len = strlen(arg);
		if (!my_strnicmp(arg, "PUSH", len))
			type = STACK_PUSH;
		else if (!my_strnicmp(arg, "POP", len))
			type = STACK_POP;
		else if (!my_strnicmp(arg, "LIST", len))
			type = STACK_LIST;
		else
		{
			say("%s is unknown stack verb", arg);
			return;
		}
	}
	else
	{
		say("Need operation for STACK");
		return;
	}
	if ((arg = next_arg(args, &args)) != NULL)
	{
		len = strlen(arg);
		if (!my_strnicmp(arg, "ON", len))
			do_stack_on(type, args);
		else if (!my_strnicmp(arg, "ALIAS", len))
			do_stack_alias(type, args, STACK_DO_ALIAS);
		else if (!my_strnicmp(arg, "ASSIGN", len))
			do_stack_alias(type, args, STACK_DO_ASSIGN);
		else if (!my_strnicmp(arg, "SET", len))
			do_stack_set(type, args);
		else
		{
			say("%s is not a valid STACK type", arg);
			return;
		}
	}
	else
	{
		say("Need stack type for STACK");
		return;
	}
}

BUILT_IN_COMMAND(pop_cmd)
{
	extern char *function_pop(char *, char *);
        char *blah = function_pop(NULL, args);
        new_free(&blah);
}

BUILT_IN_COMMAND(push_cmd)
{
	extern char *function_push(char *, char *);
        char *blah = function_push(NULL, args);
        new_free(&blah);
}

BUILT_IN_COMMAND(setenvcmd)
{
	char *env_var;

	if ((env_var = next_arg(args, &args)) != NULL)
		bsd_setenv(env_var, args, 1);
	else
		say("Usage: SETENV <var-name> <value>");
}

BUILT_IN_COMMAND(xtypecmd)
{
	char	*arg;

	
	if (args && (*args == '-' || *args == '/'))
	{
		char saved = *args;
		args++;
		if ((arg = next_arg(args, &args)) != NULL)
		{
			if (!my_strnicmp(arg, "L", 1))
			{
				for (; *args; args++)
					input_add_character(*args, empty_string);
			}
			else
				say ("Unknown flag -%s to XTYPE", arg);
			return;
		}
		input_add_character(saved, empty_string);
	}
	else
		typecmd(command, args, empty_string, helparg, 0);
	return;
}

BUILT_IN_COMMAND(shiftcmd)
{
	extern char *function_shift(char *, char *);
        char *blah = function_shift(NULL, args);
        new_free(&blah);
}

BUILT_IN_COMMAND(unshiftcmd)
{
	extern char *function_unshift(char *, char *);
        char *blah = function_unshift(NULL, args);
        new_free(&blah);
}

BUILT_IN_COMMAND(e_debug)
{
	int x;
	char buffer[BIG_BUFFER_SIZE + 1];

	if (args && *args)
	{
		char *s, *copy;

		copy = LOCAL_COPY(args);
		upper(copy);
		while ((s = next_arg(copy, &copy)))
		{
			char *t, *c;
			x = 1;
			c = new_next_arg(copy, &copy);
			if (!c || !*c) 
				break;
			while ((t = next_in_comma_list(c, &c)))
			{
				if (!t || !*t) 
					break;
				if (*t == '-') 
				{ 
					t++; 
					x = 0; 
				}
				if (!my_stricmp(s, "ALIAS"))
					debug_alias(t, x);
				else if (!my_stricmp(s, "HOOK"))
					debug_hook(t, x);
			}
		}
		return;
	}
	*buffer = 0;
	for (x = 0; x < FD_SETSIZE; x++)
	{
		if (FD_ISSET(x, &readables))
		{
			strcat(buffer, space);
			strcat(buffer, ltoa(x));
		}
	}
	yell(buffer);
}

BUILT_IN_COMMAND(ftp)
{
Window *window, *tmp;
char name[40];
char *pgm = NULL;
int direct = 0;

	
	sprintf(name, "%%%s", command);
	if (command && !my_stricmp(command, "shell"))
	{
		pgm = get_string_var(SHELL_VAR);
		direct = 1;
	}
	else
		pgm = command;
		
	if (!args || !*args)
	{
		if (!is_window_name_unique(name+1))
		{
			int logic = -1;
			if ((tmp = get_window_by_name(name+1)))
			{
				delete_window(tmp);
				if ((logic = logical_to_index(name+1)) > -1)
					kill_process(logic, 15);
				else bitchsay("No such process [%s]", name+1);
			}
			recalculate_windows(tmp->screen);
			update_all_windows();
			return;
		}
	}
	if ((tmp = new_window(current_window->screen)))
	{
		int refnum;
		char *p = NULL;
		window = tmp;
		if (is_window_name_unique(name+1))
		{
			malloc_strcpy(&window->name, name+1);
			window->update |= UPDATE_STATUS;
		}

		window->screen = main_screen;
		set_screens_current_window(window->screen, window);
		recalculate_windows(tmp->screen);
		hide_window(window);
		refnum = window->refnum;
		
/*		update_all_windows();*/
		malloc_sprintf(&p, "-NAME %s %s %s", name, pgm, args);
/*		start_process(p, name+1, NULL, NULL, refnum, direct);*/
		execcmd(NULL, p, NULL, NULL, 0);
		if (is_valid_process(name))
		{
			NickStruct *tmp_nick = NULL;
			malloc_strcpy(&window->query_nick, name);
			tmp_nick = (NickStruct *)new_malloc(sizeof(NickStruct));
			malloc_strcpy(&tmp_nick->nick, name);
			add_to_list((List **)&window->nicks, (List *)tmp_nick);
		}
		new_free(&p);
	} else bitchsay("Unable to create a new window");	
}

BUILT_IN_COMMAND(pretend_cmd)
{
N_DATA cmd;
	if (args && *args)
	{
		cmd.command = my_atol(next_arg(args, &args));
		cmd.len = strlen(args ? args : empty_string);
		parse_server(&cmd, args ? args : empty_string);
	}
}

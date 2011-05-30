/*
 *
 * irc handler for TekNap. Handles a irc window.
 */

#include "teknap.h"
#include "struct.h"
#include "ircaux.h"
#include "irchandler.h"
#include "list.h"
#include "hook.h"
#include "napster.h"
#include "output.h"
#include "server.h"
#include "window.h"
#include "vars.h"

#include <sys/utsname.h>

int 	num_protocol_cmds = -1;
char	*FromUserHost = empty_string;


#define CTCP_HANDLER(x) \
	static char * x (CtcpEntry *ctcp, char *from, char *to, char *cmd)

CTCP_HANDLER(do_atmosphere);
CTCP_HANDLER(do_ping_reply);
CTCP_HANDLER(do_version);
CTCP_HANDLER(do_clientinfo);
CTCP_HANDLER(do_ping);

#define CTCP_ACTION	0
#define CTCP_VERSION	1
#define CTCP_PING	2
#define CTCP_CLIENTINFO	3
#define CTCP_CUSTOM	4

#define NUMBER_OF_CTCPS CTCP_CUSTOM

#define CTCP_SPECIAL    0       /* Limited, quiet, noreply, not expanded */
#define CTCP_REPLY      1       /* Sends a reply (not significant) */
#define CTCP_INLINE     2       /* Expands to an inline value */
#define CTCP_NOLIMIT    4       /* Limit of one per privmsg. */
#define CTCP_TELLUSER   8       /* Tell the user about it. */


#define TEKNAP_COMMENT "It's waxstatic!"

static CtcpEntry ctcp_cmd[] =
{
	{ "ACTION",	CTCP_ACTION, 	CTCP_SPECIAL | CTCP_NOLIMIT,
		"contains action descriptions for atmosphere",
		do_atmosphere, 	do_atmosphere },

	{ "VERSION",	CTCP_VERSION,	CTCP_REPLY | CTCP_TELLUSER,
		"shows client type, version and environment",
		do_version, 	NULL },

	{ "PING", 	CTCP_PING, 	CTCP_REPLY | CTCP_TELLUSER,
		"returns the arguments it receives",
		do_ping, 	do_ping_reply },

	{ "CLIENTINFO",	CTCP_CLIENTINFO,CTCP_REPLY | CTCP_TELLUSER,
		"gives information about available CTCP commands",
		do_clientinfo, 	NULL },

	{ NULL,		CTCP_CUSTOM,	CTCP_REPLY | CTCP_TELLUSER,
		NULL,
		NULL, NULL }
};

int is_channel(char *to)
{
	if (to && (*to == '*' || *to == '&'))
		return 1;
	return 0;
}

char	* PasteArgs(char **Args, int StartPoint)
{
	int	i;

	for (; StartPoint; Args++, StartPoint--)
		if (!*Args)
			return NULL;
	for (i = 0; Args[i] && Args[i+1]; i++)
		Args[i][strlen(Args[i])] = ' ';
	Args[1] = NULL;
	return Args[0];
}


static void numbered_command(char *from, int comm, char **ArgList)
{
char buff[BIG_BUFFER_SIZE];

	switch(comm)
	{
		case 1:
		case 2:
		case 3:
		case 4:
			PasteArgs(ArgList, 1);
			say("%s", ArgList[1]);
			break;
		case 250:
		case 251:
		case 252:
		case 253:
		case 254:
		case 255:
		case 265:
		case 266:
			PasteArgs(ArgList, 1);
			say("%s", ArgList[1]);		
			break;

		case 301: /* away */	
			PasteArgs(ArgList, 2);
			put_it("50³ 57a-1way     50:-1 %s", ArgList[2]);
			break;

		case 307: /* regged */
		case 308: /* admin */
		case 309: /* services */
		case 310: /* helpful */
			break;

		case 311: /* nick user host status */
			put_it("52ÚÄÄÄÄÄÄÄÄ32Ä52ÄÄ32ÄÄ52Ä32ÄÄÄÄÄÄÄÄÄ50Ä32ÄÄ50ÄÄ32Ä50ÄÄÄÄÄÄÄÄÄÄ-Ä ÄÄ  Ä   -");
			put_it("52³ 57%s 50(-1%s@%s50)", ArgList[1], ArgList[2], ArgList[3]);
			PasteArgs(ArgList, 5);
			put_it("52³ 57i-1rcname  50:-1 %s", ArgList[5]);
			break;
		case 312: /* server */
			put_it("50³ 57s-1erver   50:-1 %s (%s)", ArgList[2], ArgList[3]);
			break;
		case 313: /* oper */
			PasteArgs(ArgList, 2);
			put_it("50| 57o-1perator 50:-1 %s", ArgList[2]);
			break;
		case 314: /* whowas */
			break;
		case 317: /* idle signon */
		{
			int seconds = 0;
			int hours = 0;
			int minutes = 0;
			int secs = my_atol(ArgList[2]);
			int st = my_atol(ArgList[3]);
			hours = secs / 3600;
			minutes = (secs - (hours * 3600)) / 60;
			seconds = secs % 60;
			

			put_it("50. 57i-1dle     50:-1 %d hours %d mins %d secs (signon: %s)", hours, minutes, seconds, my_ctime(st));
/*			"50: 57s-1ignon   50:-1 $0-"*/
			break;
		}
		case 318: /* end of whois */
			break;
		case 319: /* channels */
			PasteArgs(ArgList, 2);
			put_it("32³ 57c-1hannels 50:-1 %s", ArgList[2]); 
			break;
		case 332: /* topic */
			sprintf(buff, "%s %s", ArgList[0], ArgList[1]);
			cmd_topic(332, buff);
			break;
		case 333: /* topic time */
			break;

//ùíù Numbered server stuff: "352 pana #opennap toasty bender.thirty4.com
//          irc.lightning.net Toasty_ H@ :2 Chad Boyda" (irc.core.com)
//          ùíù Numbered server stuff: "315 pana #opennap :End of /WHO list."
//                    (irc.core.com)

		case 315:
			break;
		case 352:
		{
			char *name;
			PasteArgs(ArgList, 7);
			name = ArgList[7];
			next_arg(name, &name);
			put_it("53%-11s 57%-11s 36%-3s -1%s51@-1%s 50(-1%s50)", ArgList[1], ArgList[5], ArgList[6], ArgList[2], ArgList[3], name);
			break;                    
		}
		case 353:
		{
			char *n, *args;
			int l = 0;
			PasteArgs(ArgList, 3);
			args = ArgList[3];
			while ((n = next_arg(args, &args)))
			{
				if (*n == '@')
					l = 1,n++;
				else if (*n == '+') 
					l = 2,n++;
				else if (*n == '*')
					l = 4,n++;
				else
					l = 0;
				sprintf(buff, "%s %s 0 %d", ArgList[2], n, l);
				cmd_names(comm, buff);
			}
			break;
		}
		case 366:
			cmd_endnames(comm, ArgList[1]);
			break;	
		case 372: /* motd */
		case 375: /* begin of motd */
		case 376: /* end of motd */
			PasteArgs(ArgList, 1);
			cmd_motd(comm, ArgList[1]);
			break;
			
		case 401:
			PasteArgs(ArgList, 1);
			say("%s", ArgList[1]);
			break;		
		case 433: /* nickname is in use */
		{
			char *nick = ArgList[1];
#if 0
ùíù Numbered server stuff: "433 * pana :Nickname is already in use."
          (irc.core.com)
#endif
			if (strlen(nick) < 8 && nick[strlen(nick)-1] != '_')
			{
				set_server_nickname(from_server, strcat(get_server_nickname(from_server), "_"));
				send_to_server("NICK %s", get_server_nickname(from_server));	          			
			}
			else
			{
				PasteArgs(ArgList, 2);
				say("%s %s", ArgList[1], ArgList[2]);
			}
			break;
		}
		default:
			PasteArgs(ArgList, 0);
			if (from)
				say("Numbered server stuff: \"%d %s\" (%s)", comm, ArgList[0], from);
			else
				say("Numbered server stuff: \"%d %s\"", comm, ArgList[0]);
			break;
	}
}

Window *find_channel_window(int serv, char *chan)
{
Window *tmp = NULL;
	while (traverse_all_windows(&tmp))
	{
		if (serv == tmp->server && 
		    find_in_list((List **)&tmp->nchannels, chan, 0))
			if ((!get_server_ircmode(serv) && 
			    !get_server_ircmode(tmp->server)) || 
			    (get_server_ircmode(serv) && 
			    get_server_ircmode(tmp->server)))
				return tmp;
	}
	return NULL;
}

static void rfc1459_odd (char *from, char *comm, char **ArgList)
{
	PasteArgs(ArgList, 0);
	if (from)
		say("Odd server stuff: \"%s %s\" (%s)", comm, ArgList[0], from);
	else
		say("Odd server stuff: \"%s %s\"", comm, ArgList[0]);
}

static void p_error (char *from, char **ArgList)
{
	PasteArgs(ArgList, 0);
	put_it("ERROR %s", ArgList[0]);
}

static void p_channel (char *from, char **ArgList)
{
char buff[BIG_BUFFER_SIZE];
	PasteArgs(ArgList, 0);
	if (!my_stricmp(from, get_server_nickname(from_server)))
		cmd_joined(0, ArgList[0]);
	else
	{
		sprintf(buff, "%s %s", ArgList[0], from);
		cmd_names(0, buff);
	}
}

static void p_kick (char *from, char **ArgList)
{
ChannelStruct *ch = NULL;
Window *tmp = NULL;
NickStruct *n;

/* KICK panasync   #trivia pana :Set the world Afire!!!*/

	PasteArgs(ArgList, 2);
	while(traverse_all_windows(&tmp))
	{
		if (tmp->server == from_server && (ch =(ChannelStruct *) find_in_list((List **)&tmp->nchannels, ArgList[0], 0)))
			break;
		else
			ch = NULL;
	}
	if (!my_stricmp(ArgList[1], get_server_nickname(from_server)))
	{
		if (tmp && ch)
		{
			ch = (ChannelStruct *)remove_from_list((List **)&tmp->nchannels, ArgList[0]);
			free_nicks(ch);
			new_free(&ch->topic);
			new_free(&ch->channel);
			new_free(&ch);
			if (get_int_var(AUTO_REJOIN_VAR))
			{
				send_to_server("JOIN %s", ArgList[0]);
				add_waiting_channel(tmp, ArgList[0]);
			}				
		}
		set_display_target(ArgList[0], LOG_CRAP);
		say("57You-1 have been kicked off %s by 56%s 50(-1%s-50)", ArgList[0], from, ArgList[2] ? ArgList[2] : empty_string);

		
	} 
	else
	{
		if (tmp && ch)
		{
			if ((n = (NickStruct *)remove_from_list_double((List **)&ch->nicks, ArgList[1])))
			{
				free_nickstruct(n);
				new_free(&n);
			}
			else
				say("***** ERROR ArgList[0] = %s not found in nicklist", ArgList[0]);
		}
		set_display_target(ArgList[0], LOG_CRAP);
		say("-1%s was kicked off %s by 56%s 50(-1%s50)", ArgList[1], ArgList[0], from, ArgList[2] ? ArgList[2] : empty_string);
	}	
}

static void p_mode (char *from, char **ArgList)
{
int smode = 0;
	smode = strchr(from, '.') ? 1 : 0;
	PasteArgs(ArgList, 1);
	set_display_target(ArgList[0], LOG_CRAP);
	if (is_channel(ArgList[0]))
	{
		if (smode)
			say("modehack50/36%s 50[57%s50]-1 by 57%s", ArgList[0], ArgList[1], from);
		else
			say("mode50/36%s 50[57%s50]-1 by 57%s", ArgList[0], ArgList[1], from);
	}
	else
		say("mode50/36%s 50[57%s50]-1 by 57%s", ArgList[0], ArgList[1], from);
}

static void p_nick (char *from, char **ArgList)
{
Window *tmp = NULL;
ChannelStruct *ch;
NickStruct *n;
int doit = 0;
int itsme = 0;
	itsme = !my_stricmp(from, get_server_nickname(from_server));
	while (traverse_all_windows(&tmp))
	{
		if (tmp->server == from_server && get_server_ircmode(tmp->server))
		{
			for (ch = tmp->nchannels; ch; ch = ch->next)
			{
				if ((n = (NickStruct *)remove_from_list_double((List **)ch->nicks, from)))
				{
					malloc_strcpy(&n->nick, ArgList[0]);
					doit = 1;
					add_to_list_double((List **)&ch->nicks, (List *)n);
					set_display_target(ch->channel, LOG_CRAP);
				}
			}
		}
	}	
	if (doit)
		say(itsme ? 
		"57You50(-1%s50)-1 are now known as 36%s":
		"57%s -1is now known as 36%s", from, ArgList[0]);
}

static void p_quit (char *from, char **ArgList)
{
Window *tmp = NULL;
ChannelStruct *ch;
NickStruct *n;
char *chan = NULL;
char *chan1 = NULL;
int doit = 0;

	PasteArgs(ArgList, 0);
	while (traverse_all_windows(&tmp))
	{
		if (tmp->server == from_server && get_server_ircmode(from_server))
		{
			for (ch = tmp->nchannels; ch; ch = ch->next)
			{
				if ((n = (NickStruct *)remove_from_list_double((List **)&ch->nicks, from)))
				{
					m_s3cat(&chan, ",", ch->channel);
					doit++;
					free_nickstruct(n);
					new_free(&n);
					if (tmp == current_window)
						chan1 = ch->channel;
					else if (!chan1)
						chan1 = ch->channel;
				}
			}
		}
	}
	if (doit)
	{
		set_display_target(chan1, LOG_CRAP);
		say("SignOff 57%s-1@%s: %s 50(-1%s50)", from, FromUserHost, chan, ArgList[0]);
	}
	new_free(&chan);
}

static void p_part (char *from, char **ArgList)
{
char buff[BIG_BUFFER_SIZE];
	PasteArgs(ArgList, 1);
	if (!my_stricmp(from, get_server_nickname(from_server)))
		cmd_partchannel(0, ArgList[0]);
	else
	{
		sprintf(buff, "%s %s", ArgList[0], from);
		cmd_parted(0, buff);
	}
}


static void server_notice(char *to, char *line)
{
	if (*line == '*')
		put_it("%s", line);
	else
		put_it("*** %s", line);
}

static void p_notice (char *from, char **ArgList)
{
char *to, *line;
	PasteArgs(ArgList, 1);
	to = ArgList[0];
	line = ArgList[1];	
	set_display_target(to, LOG_CRAP);
	if (!from || !*from || !my_stricmp(from, get_server_name(from_server)))
		server_notice(to, line);
	else
		put_it(FORMAT_NOTICE, from, (FromUserHost && *FromUserHost) ? FromUserHost : "*@*", line);
}

static	void p_ping(char *from, char **ArgList)
{
	PasteArgs(ArgList, 0);
	send_to_server("PONG %s", ArgList[0]);
}

void split_CTCP(char *raw_message, char *ctcp_dest, char *after_ctcp)
{
	char *ctcp_start, *ctcp_end;

	*ctcp_dest = *after_ctcp = 0;
	ctcp_start = strchr(raw_message, CTCP_DELIM_CHAR);
	if (!ctcp_start)
		return;		/* No CTCPs present. */

	*ctcp_start++ = 0;
	ctcp_end = strchr(ctcp_start, CTCP_DELIM_CHAR);
	if (!ctcp_end)
	{
		*--ctcp_start = CTCP_DELIM_CHAR;
		return;		/* Thats _not_ a CTCP. */
	}

	*ctcp_end++ = 0;
	strmcpy(ctcp_dest, ctcp_start, BIG_BUFFER_SIZE-4);
	strmcpy(after_ctcp, ctcp_end, BIG_BUFFER_SIZE-4);
	return;		/* All done! */
}

CTCP_HANDLER(do_atmosphere)
{
	const char *old_message_from;
	unsigned long old_message_level;
	
	if (!cmd || !*cmd)
		return NULL;
	
	save_display_target(&old_message_from, &old_message_level);

	if (is_channel(to))
	{
		if (is_current_channel(to, from_server, 0))
			put_it("50p 57%s50/57%s -1%s", from, to, cmd);
		else
			put_it("50p 36%s50>57%s -1%s", from, to, cmd);
	}
	else
		put_it("50ð 57%s -1%s", from, cmd);
	restore_display_target(old_message_from, old_message_level);
	return NULL;
}

CTCP_HANDLER(do_clientinfo)
{
	int	i;
	if (cmd && *cmd)
	{
		for (i = 0; i < NUMBER_OF_CTCPS; i++)
		{
			if (!my_stricmp(cmd, ctcp_cmd[i].name))
			{
				send_to_server("NOTICE %s :%cCLIENTINFO %s %s%c", 
					from, CTCP_DELIM_CHAR, ctcp_cmd[i].name, 
					ctcp_cmd[i].desc, CTCP_DELIM_CHAR);
				return NULL;
			}
		}
		send_to_server("NOTICE %s :%cERRMSG %s: %s is not a valid function%c", from, CTCP_DELIM_CHAR, ctcp_cmd[CTCP_CLIENTINFO].name, cmd, CTCP_DELIM_CHAR);
	}
	else
	{
		char buffer[BIG_BUFFER_SIZE + 1];
		*buffer = '\0';

		for (i = 0; i < NUMBER_OF_CTCPS; i++)
		{
			strmcat(buffer, ctcp_cmd[i].name, BIG_BUFFER_SIZE);
			strmcat(buffer, space, BIG_BUFFER_SIZE);
		}
		send_to_server("NOTICE %s :%cCLIENTINFO %s :Use CLIENTINFO <COMMAND> to get more specific information%c", from, CTCP_DELIM_CHAR, buffer, CTCP_DELIM_CHAR);
	}
	return NULL;
}

CTCP_HANDLER(do_version)
{
	char	*version_reply = NULL;
	struct utsname un;
	char	*the_unix,
		*the_version;

	if (uname(&un) < 0)
	{
		the_version = empty_string;
		the_unix = "unknown";
	}
	else
	{
		the_version = un.release;
		the_unix = un.sysname;
	}
	malloc_sprintf(&version_reply, "%s %s %s %s", nap_version, internal_version, the_unix, the_version);

	send_to_server("NOTICE %s :%cVERSION %s %s%c", from, CTCP_DELIM_CHAR, version_reply, TEKNAP_COMMENT, CTCP_DELIM_CHAR);
	new_free(&version_reply);
	return NULL;
}

CTCP_HANDLER(do_ping)
{
	send_to_server("NOTICE %s :%cPING %s%c", from, CTCP_DELIM_CHAR, cmd ? cmd : empty_string, CTCP_DELIM_CHAR);
	return NULL;
}

CTCP_HANDLER(do_ping_reply)
{
	char *sec, *usec = NULL;
	struct timeval t;
	time_t tsec = 0, tusec = 0, orig;

	if (!cmd || !*cmd)
		return NULL;		/* This is a fake -- cant happen. */

	orig = my_atol(cmd);
	
	get_time(&t);
	if (orig < start_time || orig > t.tv_sec)
		return NULL;
	
       /* We've already checked 'cmd' here, so its safe. */
        sec = cmd;
	tsec = t.tv_sec - my_atol(sec);
        
	if ((usec = strchr(sec, ' ')))
	{
		*usec++ = 0;
		tusec = t.tv_usec - my_atol(usec);
	}
                        
	/*
	 * 'cmd', a variable passed in from do_notice_ctcp()
	 * points to a buffer which is MUCH larger than the
	 * string 'cmd' points at.  So this is safe, even
	 * if it looks "unsafe".
	 */
	sprintf(cmd, "%5.3f seconds", (float)(tsec + (tusec / 1000000.0)));
	return NULL;
}


int check_ctcp(char *from, char *to, char *msg)
{
int	delim_char = charcount(msg, '\001'),
	i;
char	local_ctcp[BIG_BUFFER_SIZE],
	last[BIG_BUFFER_SIZE],
	the_ctcp[BIG_BUFFER_SIZE],
	*ctcp_argument,
	*ctcp_command,
	*ptr = NULL;

	if (delim_char < 2)
		return 0;             /* No CTCPs. */
	if (*to == '$')
		return 0;
		
	strmcpy(local_ctcp, msg, IRCD_BUFFER_SIZE-2);

	set_display_target(to, LOG_CRAP);
	for (;;strmcat(local_ctcp, last, IRCD_BUFFER_SIZE-2))
	{
		split_CTCP(local_ctcp, the_ctcp, last);
		if (!*the_ctcp) 
			return 1;

		ctcp_command = the_ctcp;
		ctcp_argument = strchr(the_ctcp, ' ');
		if (ctcp_argument)
			*ctcp_argument++ = 0;
		else
			ctcp_argument = empty_string;

		for (i = 0; i < NUMBER_OF_CTCPS; i++)
			if (!strcmp(ctcp_command, ctcp_cmd[i].name))
				break;
		if (i == NUMBER_OF_CTCPS)
			continue;
		ptr = ctcp_cmd[i].func(ctcp_cmd + i, from, to, ctcp_argument);

		if ((ctcp_cmd[i].flag & CTCP_TELLUSER))
			put_it("50>-1>57> 52%s 50[32%s50]32 requested %s %s from %s", from, FromUserHost, ctcp_command, ctcp_argument, to); 
		new_free(&ptr);		
	}
	if (*local_ctcp)
	{
		strcpy(msg, local_ctcp);
		return 0;
	}
	return 1;
}

static void p_privmsg (char *from, char **ArgList)
{
char buffer[BIG_BUFFER_SIZE];
	PasteArgs(ArgList, 1);
	if (check_ctcp(from, ArgList[0], ArgList[1]))
		return;
	if ((*ArgList[0] == '#') || (*ArgList[0] == '&'))
	{
		sprintf(buffer, "%s %s %s", ArgList[0], from, ArgList[1]);
		cmd_public(0, buffer);
		return;
	}
	sprintf(buffer, "%s %s", from, ArgList[1]);
	cmd_msg(0, buffer);
}


static void p_topic (char *from, char **ArgList)
{
Window *tmp;
ChannelStruct *chan;
	set_display_target(ArgList[0], LOG_CRAP);
	if ((tmp = find_channel_window(from_server, ArgList[0])))
	{
		chan = (ChannelStruct *)find_in_list((List **)&tmp->nchannels, ArgList[0], 0);
		malloc_strcpy(&chan->topic, ArgList[1]);
		say("57%s -1has changed the topic on channel 57%s-1 to50:-1 %s", from, ArgList[0], ArgList[1]);
	}
}

static void p_wallops (char *from, char **ArgList)
{
	PasteArgs(ArgList, 0);
	put_it("WALLOP %s", ArgList[0]);
}
	 
protocol_command rfc1459[] = {
{	"ADMIN",	NULL,		NULL,		0,		0, 0},
{	"AWAY",		NULL,		NULL,		0,		0, 0},
{ 	"CONNECT",	NULL,		NULL,		0,		0, 0},
{	"ERROR",	p_error,	NULL,		0,		0, 0},
{	"ERROR:",	p_error,	NULL,		0,		0, 0},
{	"INVITE",	NULL,		NULL,		0,		0, 0},
{	"INFO",		NULL,		NULL,		0,		0, 0},
{	"ISON",		NULL,		NULL,		PROTO_NOQUOTE,	0, 0},
{	"JOIN",		p_channel,	NULL,		PROTO_DEPREC,	0, 0},
{	"KICK",		p_kick,		NULL,		0,		0, 0},
{	"KILL",		NULL,		NULL,		0,		0, 0},
{	"LINKS",	NULL,		NULL,		0,		0, 0},
{	"LIST",		NULL,		NULL,		0,		0, 0},
{	"MODE",		p_mode,		NULL,		0,		0, 0},
{	"NAMES",	NULL,		NULL,		0,		0, 0},
{	"NICK",		p_nick,		NULL,		PROTO_NOQUOTE,	0, 0},
{	"NOTICE",	p_notice,	NULL,		0,		0, 0},
{	"OPER",		NULL,		NULL,		0,		0, 0},
{	"PART",		p_part,		NULL,		PROTO_DEPREC,	0, 0},
{	"PASS",		NULL,		NULL,		0, 		0, 0},
{	"PING",		p_ping,		NULL,		0,		0, 0},
{	"PONG",		NULL,		NULL,		0,		0, 0},
{	"PRIVMSG",	p_privmsg,	NULL,		0,		0, 0},
{	"QUIT",		p_quit,		NULL,		PROTO_DEPREC,	0, 0},
{	"REHASH",	NULL,		NULL,		0,		0, 0},
{	"RESTART",	NULL,		NULL,		0,		0, 0},
{	"RPONG",	NULL,		NULL,		0,		0, 0},
{	"SERVER",	NULL,		NULL,		PROTO_NOQUOTE,	0, 0},
{	"SILENCE",	NULL,		NULL,		0,		0, 0},
{	"SQUIT",	NULL,		NULL,		0,		0, 0},
{	"STATS",	NULL,		NULL,		0,		0, 0},
{	"SUMMON",	NULL,		NULL,		0,		0, 0},
{	"TIME",		NULL,		NULL,		0,		0, 0},
{	"TRACE",	NULL,		NULL,		0,		0, 0},
{	"TOPIC",	p_topic,	NULL,		0,		0, 0},
{	"USER",		NULL,		NULL,		0,		0, 0},
{	"USERHOST",	NULL,		NULL,		PROTO_NOQUOTE,	0, 0},
{	"USERS",	NULL,		NULL,		0,		0, 0},
{	"VERSION",	NULL,		NULL,		0,		0, 0},
{	"WALLOPS",	p_wallops,	NULL,		0,		0, 0},
{	"WHO",		NULL,		NULL,		PROTO_NOQUOTE,	0, 0},
{	"WHOIS",	NULL,		NULL,		0,		0, 0},
{	"WHOWAS",	NULL,		NULL,		0,		0, 0},
{	NULL,		NULL,		NULL,		0,		0, 0}
};

#define NUMBER_OF_COMMANDS (sizeof(rfc1459) / sizeof(protocol_command)) - 2;


/*
 * BreakArgs: breaks up the line from the server, in to where its from,
 * setting FromUserHost if it should be, and returns all the arguements
 * that are there.   Re-written by phone, dec 1992.
 */
int BreakArgs(char *Input, char **Sender, char **OutPut, int ig_sender)
{
	int	ArgCount = 0;

	/*
	 * The RFC describes it fully, but in a short form, a line looks like:
	 * [:sender[!user@host]] COMMAND ARGUMENT [[:]ARGUMENT]{0..14}
	 */

	/*
	 * Look to see if the optional :sender is present.
	 */
	if (!ig_sender)
	{
		if (*Input == ':')
		{
			*Sender = ++Input;
			while (*Input && *Input != *space)
				Input++;
			if (*Input == *space)
				*Input++ = 0;

		/*
		 * Look to see if the optional !user@host is present.
		 * look for @host only as services use it.
		 */
			FromUserHost = *Sender;
			while (*FromUserHost && *FromUserHost != '!' && *FromUserHost != '@')
				FromUserHost++;
			if (*FromUserHost == '!' || *FromUserHost == '@')
				*FromUserHost++ = 0;
		}
		/*
		 * No sender present.
		 */
		else
			*Sender = FromUserHost = empty_string;
	}
	/*
	 * Now we go through the argument list...
	 */
	for (;;)
	{
		while (*Input && *Input == *space)
			Input++;

		if (!*Input)
			break;

		if (*Input == ':')
		{
			OutPut[ArgCount++] = ++Input;
			break;
		}

		OutPut[ArgCount++] = Input;
		if (ArgCount >= MAXPARA)
			break;

		while (*Input && *Input != *space)
			Input++;
		if (*Input == *space)
			*Input++ = 0;
	}
	OutPut[ArgCount] = NULL;
	return ArgCount;
}


void parse_irc_server(char *line)
{
	char	*from,
		*comm,
		*end;
	int	numeric;
	int	len = 0;
	int	ofs = from_server;
	char	**ArgList;
	char	copy[BIG_BUFFER_SIZE+1];
	char	*TrueArgs[MAXPARA + 1] = {NULL};

	protocol_command *retval;
	int	loc;
	int	cnt;

	if (num_protocol_cmds == -1)
		num_protocol_cmds = NUMBER_OF_COMMANDS;

	
	if (!line || !*line)
		return;

	len = strlen(line);

	end = len + line;
	if (*--end == '\n')
		*end-- = 0;
	if (*end == '\r')
		*end-- = 0;

	if (!line || !*line)
		return;

	ArgList = TrueArgs;

	strncpy(copy, line, BIG_BUFFER_SIZE);
	BreakArgs(line, &from, ArgList, 0);

	if (!(comm = (*ArgList++)) || !from || !*ArgList)
		return;		/* Empty line from server - ByeBye */

	set_display_target(from, LOG_CRAP);
	/* Check for a numeric first */
	if ((numeric = atoi(comm)))
	{
		if (do_hook(RAW_IRC_LIST, "%s %d %s", from, numeric, line))
			numbered_command(from, numeric, ArgList);
	}
	else
	{
		retval = (protocol_command *)find_fixed_array_item(
			(void *)rfc1459, sizeof(protocol_command), 
			num_protocol_cmds + 1, comm, &cnt, &loc);

		if (cnt < 0 && rfc1459[loc].inbound_handler)
		{
			if (do_hook(RAW_IRC_LIST, "%s %s", from, line))
				rfc1459[loc].inbound_handler(from, ArgList);
		}
		else
			rfc1459_odd(from, comm, ArgList);
		rfc1459[loc].bytes += len;
		rfc1459[loc].count++;
	}
	from_server = ofs;
	FromUserHost = empty_string;
}

BUILT_IN_COMMAND(irccmd)
{
char *cmd, *host;
int ofs = from_server;

	cmd = next_arg(args, &args);
	if (!my_stricmp(cmd, "server"))
	{
		int i;
		host = next_arg(args, &args);
		i = find_server_refnum(host, &host);
		if (i > -1)
		{
			if (connect_to_irc_server(i) > 0)
			{
				from_server = i;
				current_window->server = from_server;
				login_to_ircserver(i);
			}
			return;
		}
	}
	else if (!my_stricmp(cmd, "nick"))
	{
		char *nick;
		from_server = current_window->server;
		if ((nick = next_arg(args, &args)))
		{
			if (get_server_ircmode(from_server))
			{
				set_server_nickname(from_server, nick);
				if (is_connected(from_server))
					send_to_server("NICK %s", nick);
			}
		}
	}
	else if (!my_stricmp(cmd, "WHO"))
	{
		char *target;
		from_server = current_window->server;
		if ((target = next_arg(args, &args)))
		{
			if (get_server_ircmode(current_window->server))
				send_to_server("WHO %s", target);
		} 
		else if (current_window->current_channel && get_server_ircmode(from_server))
			send_to_server("WHO %s", current_window->current_channel);
	}
	from_server = ofs;
}


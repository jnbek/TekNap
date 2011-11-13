/* $Id: napster.c,v 1.2 2001/07/08 21:33:54 edwards Exp $ */
 
#include "teknap.h"
#include "keys.h"
#include "struct.h"
#include "commands.h"
#include "flood.h"
#include "hook.h"
#include "input.h"
#include "list.h"
#include "output.h"
#include "napster.h"
#include "server.h"
#include "status.h"
#include "window.h"
#include "whois.h"
#include "vars.h"

#include "ptest.h"
#include "scott2.h"
#include "tgtk.h"
#include <string.h>

#if 0
FileStruct *file_search = NULL;
#endif

#ifdef THREAD1
pthread_mutex_t file_search_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

#if defined(GUI)
void update_browser(void);
#endif

NickStruct *nap_hotlist = NULL;

char *last_invited = NULL;
extern char *FromUserHost;

PT_STATS ptest_stats;


int channel_count = 0;
int nick_count = 0;
int current_numeric = 0;

UrlList *url_list = NULL, 
	*cur_url = NULL, 
	*prev_url = NULL, 
	*new_url = NULL;

static int url_count = 0;


NAP_COMM(generic_handler);
NAP_COMM(ignore_numeric);

int grab_http(char *, char *, char *);
void switch_channels_window(Window *);
extern char *recv_nick;


char *_speed_color[] = 
	{ "[1;30m", "[0;32m", "[0;33m", "[0;35m", "[0;31m", "[0;00m", "[1;34m", "[1;31m", "[1;35m", "[1;33m", "[1;32m", "" };
 	/* 0     1     2     3     4     5     6     7     8     9     10     11 */

extern Server *server_list;

NAP_COMMANDS nap_commands[] =
{
	{ 0			, cmd_fatalerror },
	{CMDS_UNKNOWN		, cmd_unknown },/* 1 */
	{CMDS_LOGIN		, cmd_login }, 	/* 2 */
	{CMDR_EMAILADDR		, cmd_email }, /* 3 */
	{CMDR_BASTARD		, cmd_bastard}, /* 4 */

	{CMDR_CREATED		, cmd_registerinfo }, /* 8 */

	{CMDR_CREATEERROR	, cmd_alreadyregistered }, /* 9 */
	{CMDR_ILLEGALNICK	, cmd_fatalerror }, /* 10 */
	{CMDR_LOGINERROR	, cmd_fatalerror }, /* 13 */
	{CMDR_MSTAT		, NULL }, /* 15 */
	{CMDR_REQUESTUSERSPEED	, NULL }, /* 89 */
	{CMDR_SENDFILE		, NULL }, /* 95 */
	{CMDR_GETQUEUE		, NULL }, /* 108 */
	{CMDR_MOTD		, NULL }, /* 109 */
	{CMDS_REMOVEALLFILES	, ignore_numeric }, /* 110 */
	{CMDR_ANOTHERUSER	, NULL }, /* 148 */

	{CMDR_SEARCHRESULTS	, cmd_search }, /* 201 */
	{CMDR_SEARCHRESULTSEND	, cmd_endsearch }, /* 202 */
	{CMDR_FILEREADY		, cmd_getfile }, /* 204 */
	{CMDS_SENDMSG		, cmd_msg }, /* 205 */

	{CMDR_GETERROR		, cmd_nosuchnick }, /* 206 */

	{CMDR_HOTLISTONLINE	, cmd_hotlist }, /* 209 */
	{CMDR_USEROFFLINE	, cmd_offline }, /* 210 */

	{CMDR_BROWSERESULT	, cmd_browse },
	{CMDR_BROWSEENDRESULT	, cmd_endbrowse },
	{CMDR_STATS		, cmd_stats }, /* 214 */

	{CMDR_RESUMESUCCESS	, cmd_resumerequest },
	{CMDR_RESUMEEND		, cmd_resumerequestend },

	{CMDR_HOTLISTSUCCESS	, cmd_hotlistsuccess }, /* 301 */
	{CMDR_HOTLISTERROR	, cmd_hotlisterror }, /* 302 */
	
	{CMDS_PART		, cmd_partchannel }, /* 401 */
	{CMDR_PUBLIC		, cmd_public }, /* 403 */

	{CMDR_ERRORMSG		, cmd_servermsg },  /* 404 */
	{CMDR_JOIN		, cmd_joined }, /* 405 */

	{CMDR_JOINNEW		, cmd_names }, /* 406 */
	{CMDR_PARTED		, cmd_parted }, /* 407 */
	{CMDR_NAMES		, cmd_names }, /* 408 */
	{CMDR_ENDNAMES		, cmd_endnames }, /* 409 */
	{CMDS_TOPIC		, cmd_topic }, /* 410 */

	{CMDS_CBANLIST		, cmd_endcban }, /* 420 */
	{CMDR_CBANLIST		, cmd_cbanlist }, /* 421 */
	
	{CMDR_FILEINFOFIRE	, cmd_firewall_request }, /* 501 */

	{CMDS_REQUESTLINESPEED	, NULL }, /* 600 */	
	{CMDR_LINESPEED		, cmd_getlinespeed }, /* 601 */

	{CMDS_WHOIS		, cmd_whois }, /* 603 */
	{CMDR_WHOIS		, cmd_whois }, /* 604 */
	{CMDR_WHOWAS		, cmd_whowas }, /* 605 */

	{CMDR_FILEREQUEST	, cmd_filerequest }, /* 607 */
	{CMDR_ACCEPTERROR	, cmd_accepterror }, /* 609 */

	{CMDR_SETDATAPORT	, cmd_setdataport },
	{CMDS_BANLIST		, cmd_banlistend }, /* 615 */
	{CMDR_BANLIST_IP	, cmd_banlist }, /* 616 */

	{CMDS_LISTCHANNELS	, cmd_endchannel }, /* 617 */
	{CMDR_LISTCHANNELS	, cmd_channellist }, /* 618 */

	{CMDR_SENDLIMIT		, cmd_toomanyfiles }, /* 620 */
	{CMDR_MOTDS		, cmd_motd }, /* 621 */


	{CMDR_DATAPORTERROR	, cmd_dataport }, /* 626 */
	{CMDS_OPSAY		, cmd_wallop }, /* 627 */
	{CMDR_ANNOUNCE		, cmd_announce }, /* 628 */
	{CMDR_BANLIST_NICK	, cmd_banlist }, /* 629 */

	{CMDS_BROWSE_DIRECT_REQ	, cmd_direct_browse }, /* 640 */
	{CMDR_BROWSE_DIRECT	, cmd_newbrowse }, /* 641 */
	{CMDR_BROWSE_DIRECT_ERROR, cmd_newbrowse_error }, /* 642 */

	{CMDR_NICK		, cmd_recname }, /* 825 */
	{CMDS_SPING		, cmd_sping }, /* 750 */
	{CMDS_PING		, cmd_ping }, /* 751 */
	{CMDS_PONG		, cmd_pingresponse }, /* 752 */
	{CMDS_SENDME		, cmd_me }, /* 824 */
	{CMDS_SHOWALLCHANNELS	, cmd_newendchannel }, /* 827 */
	{CMDR_ALLCHANNELS	, cmd_newchannellist }, /* 828 */
	{CMDS_NAME		, cmd_endname }, /* 830 */
	{CMDS_SHOWUSERS		, cmd_showusersend }, /* 831 */
	{CMDS_SHOWUSERSLIST	, cmd_showusers }, /* 832 */
	{CMDR_USERMODE		, generic_handler }, /* 10203 */
	{CMDS_SERVERLINKS	, cmd_links }, /* 10112 */
	{CMDS_SERVERUSAGE	, cmd_usage }, /* 10115 */
	{CMDS_SERVERPING	, cmd_sping }, /* 10116 */
	{CMDS_WHOWAS		, cmd_whowas1 }, /* 10121 */

	{CMDS_HISTOGRAM		, generic_handler }, /* 10123 */
	{CMDR_HISTOGRAM		, generic_handler }, /* 10124 */

	{CMDR_CLASSADD		, generic_handler }, /* 10250 */
	{CMDR_CLASSDELETE	, generic_handler }, /* 10251 */
	{CMDR_CLASSLIST		, generic_handler }, /* 10252 */

	{CMDR_DLINEADD		, generic_handler }, /* 10253 */
	{CMDR_DLINEDEL		, generic_handler }, /* 10254 */
	{CMDR_DLINELIST		, generic_handler }, /* 10255 */

	{CMDR_ILINEADD		, generic_handler }, /* 10256 */
	{CMDR_ILINEDEL		, generic_handler }, /* 10257 */
	{CMDR_ILINELIST		, generic_handler }, /* 10258 */

	{CMDR_ELINEADD		, generic_handler }, /* 10259 */
	{CMDR_ELINEDEL		, generic_handler }, /* 10260 */
	{CMDR_ELINELIST		, generic_handler }, /* 10261 */

	{ -1			, NULL }
};

#define NUMBER_OF_COMMANDS (sizeof(nap_commands) / sizeof(NAP_COMMANDS)) - 1


char *class[] = { "Leech", "User", "Mod", "Admin", "Elite", "" };

void clear_old_nchannels(ChannelStruct **nchannels)
{
ChannelStruct *lastch;
	while (*nchannels)
	{
		lastch = (*nchannels)->next;
		new_free(&((*nchannels)->topic));
		new_free(nchannels);
		*nchannels = lastch;
	}
	*nchannels = NULL;
}

void save_hotlist(FILE *f)
{
NickStruct *new;
	for (new = nap_hotlist; new; new = new->next)
		fprintf(f, "HOTLIST %s\n", new->nick);
}

void send_hotlist(void)
{
NickStruct *new;
ChannelStruct *ch;
Window *tmp = NULL;
extern unsigned long do_share(char *);

	for (new = nap_hotlist; new; new = new->next)
	{
		send_ncommand(CMDS_ADDHOTLISTSEQ, "%s", new->nick);
		new->speed = 0;
	}
	while (traverse_all_windows(&tmp))
	{
#if 0
		if (((tmp->server == -1) && (tmp->last_server == from_server)) || 
			(tmp->server == from_server) || 
			(tmp->last_server == -1 && tmp->server == -1))
#else
		if (tmp->server == -1 || tmp->server == from_server)
#endif
		{
			for (ch = tmp->oldchannels; ch; ch = ch->next)
			{
				if (get_server_ircmode(from_server))
					send_to_server("JOIN %s", ch->channel);
				else
					send_ncommand(CMDS_JOIN, "%s", ch->channel);
				if (!ch->next)
					malloc_strcpy(&tmp->current_channel, ch->channel);
				ch->injoin = 1;
			}
			tmp->waiting_channels = tmp->oldchannels;
			tmp->oldchannels = NULL;
		}
	}
	if (get_int_var(AUTO_SHARE_VAR) && !get_server_ircmode(from_server))
		do_share(NULL);
}

char *convert_time (time_t ltime)
{
unsigned long days = 0,hours = 0,minutes = 0,seconds = 0;
static char buffer[40];
	*buffer = '\0';
	seconds = ltime % 60;
	ltime = (ltime - seconds) / 60;
	minutes = ltime%60;
	ltime = (ltime - minutes) / 60;
	hours = ltime % 24;
	days = (ltime - hours) / 24;
	sprintf(buffer, "%2lud %2luh %2lum %2lus", days, hours, minutes, seconds);
	return(*buffer ? buffer : empty_string);
}


char	*numeric_banner(int number)
{
	static	char	thing[4];
	if (!get_int_var(SHOW_NUMERICS_VAR))
		return (thing_ansi?thing_ansi:"***");
	sprintf(thing, "%3.3u", number);
	return (thing);
}

char *speed_color(int speed)
{
	if (speed > MAX_SPEED)
		speed = MAX_SPEED;
	return _speed_color[speed];
}

extern char *_n_speed[];

char *n_speed(int speed)
{
	if (speed > MAX_SPEED)
		speed = MAX_SPEED;
	return _n_speed[speed];
}


void clear_filelist(FileStruct **f)
{
FileStruct *last_file, *f1 = *f;
	while (f1)
	{
		last_file = f1->next;
		if (f1->getfile)
			f1->getfile->filestruct = NULL;
		new_free(&f1->filename);
		new_free(&f1->nick);
		new_free(&f1->checksum);
		new_free(&f1);
		f1 = last_file;
	}
	*f = NULL;
}


void free_nickstruct(NickStruct *n)
{
GetFile *tmp, *last;
	new_free(&n->nick);
	new_free(&n->class);
	new_free(&n->version);
	new_free(&n->status);
	new_free(&n->channels);
	for (tmp = n->Queued; tmp; tmp = last)
	{
		last = tmp->next;
		new_free(&tmp->nick);
		new_free(&tmp->ip);
		new_free(&tmp->checksum);
		new_free(&tmp->filename);
		new_free(&tmp->realfile);
		if (tmp->filestruct)
			tmp->filestruct->getfile = NULL;
		new_free(&tmp);
	}
	n->Queued = NULL;
	clear_filelist(&n->file_browse);
}

void free_nicks(ChannelStruct *ch)
{
NickStruct *n, *last;
	n = ch->nicks;
	while(n)
	{
		last = n;
		n = n->next;
		free_nickstruct(last);
	}
}

void clear_nchannels(int server)
{
ChannelStruct *lastch;
Window *tmp = NULL;
	while (traverse_all_windows(&tmp))
	{
		if (server != tmp->server)
			continue;
		while (tmp->nchannels)
		{
			lastch = tmp->nchannels->next;
			free_nicks(tmp->nchannels);
			new_free(&tmp->nchannels->topic);
			new_free(&tmp->nchannels);
			tmp->nchannels = lastch;
		}
		tmp->nchannels = NULL;
	}
}

void clear_nicks(int server)
{
ChannelStruct *lastch;
Window *tmp = NULL;
	while (traverse_all_windows(&tmp))
	{
		if (server != tmp->server)
			continue;
		for (lastch = tmp->nchannels; lastch; lastch = lastch->next)
		{
			free_nicks(lastch);
			lastch->nicks = NULL;
			lastch->injoin = 0;
		}
	}
}


NAP_COMM(cmd_endchannel)
{
	if (do_hook(LIST_LIST, "End of channel") && get_int_var(SHOW_END_OF_MSGS_VAR))
		say("%d nicks in %d channels", nick_count, channel_count);
	nick_count = channel_count = 0;
	return 0;
}

NAP_COMM(cmd_channellist)
{
char	*name,
	*count;
	name = next_arg(args, &args);
	count = next_arg(args, &args);

	if (!channel_count)
		if (do_hook(LIST_LIST, "Num Channel Topic"))
			put_it("Num  Channel              Topic");
	if (do_hook(LIST_LIST, "%s %s %s", count, name, args))
		put_it("%4s %20s %s", count, name, args);
	nick_count += my_atol(count);
	channel_count++;
	return 0;
}

NAP_COMM(cmd_newendchannel)
{
	char *sp;
	sp = get_server_listmode(from_server);
	
	if ((!sp || (sp && channel_count)) && do_hook(LIST_LIST, "End of channel") && get_int_var(SHOW_END_OF_MSGS_VAR))
		say("%d nicks in %d channels", nick_count, channel_count);
	nick_count = channel_count = 0;
	set_server_listmode(from_server, NULL);
	return 0;
}

NAP_COMM(cmd_newchannellist)
{
char	*sp,
	*name,
	*unkn1,
	*level,
	*limit,
	*count;

	name = next_arg(args, &args);
	count = next_arg(args, &args);
	unkn1 = next_arg(args, &args);
	level = next_arg(args, &args);
	limit = next_arg(args, &args);

	sp = get_server_listmode(from_server);
	if (sp && !wild_match(sp, name))
		return 0;
	if (!channel_count)
		if (do_hook(LIST_LIST, "Num Limit ? Level Channel Topic"))
			put_it("Num  Limit Level Channel              Topic");
	if (do_hook(LIST_LIST, "%s %s %s %s %s %s", count, limit, unkn1, level, name, args))
		put_it("%4s %5s %5s %20s %s", count, limit, class[my_atol(level)], name, args);
	nick_count += my_atol(count);
	channel_count++;
	return 0;
}

NAP_COMM(cmd_whowas)
{
char *nick, *class, *l_ip, *l_port, *d_port, *email;
int t_up, t_down;
time_t online;
	if (!do_hook(WHO_LIST, "%s", args))
		return 0;
	nick = new_next_arg(args, &args);
	class = new_next_arg(args, &args);
	online = my_atol(new_next_arg(args, &args));
	t_down = my_atol(next_arg(args, &args));
	t_up = my_atol(next_arg(args, &args));
	l_ip = next_arg(args, &args);
	l_port = next_arg(args, &args);
	d_port = next_arg(args, &args);
	email = next_arg(args, &args);
		
	put_it("52ÚÄÄÄÄÄÄÄÄ32Ä52ÄÄ32ÄÄ52Ä32ÄÄÄÄÄÄÄÄÄ50Ä32ÄÄ50ÄÄ32Ä50ÄÄÄÄÄÄÄÄÄÄ-Ä ÄÄ  Ä   -");
	if (l_ip)
		put_it("52³57 User     :-1 %s50(-1%s50)-1 %s l57:-1%s d57:-1%s",nick, email, l_ip, l_port, d_port);
	else
		put_it("52³57 User     :-1 %s",nick);
	put_it("52|57 C-1lass    57:-1 %s", class);
	put_it("32³ 57L-1ast online 57:-1 %s", my_ctime(online));
	if (t_down || t_up)
		put_it("50:-1 Total  U 57:-1 %d D57 :-1 %d", t_up, t_down);
	who_remove_queue(from_server, nick);
	return 0;
}

NAP_COMM(cmd_me)
{
char *chan, *nick, *text;
	chan = next_arg(args, &args);
	nick = next_arg(args, &args);
	text = new_next_arg(args, &args);
	set_display_target(chan, LOG_CRAP);
	if (check_nignore(nick) || check_cignore(chan))
		return 0;
	check_flooding(nick, chan, text, PUBLIC_FLOOD);
	if (do_hook(ACTION_LIST, "%s %s %s", chan, nick, text))
		put_it("* %s/%s %s", chan, nick, text);
	return 0;
}

NAP_COMM(cmd_whois)
{
	who_result(from_server, args);
	return 0;
}

NAP_COMM(cmd_whowas1)
{
	whowas_result(from_server, args);
	return 0;
}

NAP_COMM(cmd_search)
{
FileStruct *new;
	if (!args || !*args)
		return 0;
	new = (FileStruct *)new_malloc(sizeof(FileStruct));
	new->filename = m_strdup(new_next_arg(args, &args));
	new->checksum = m_strdup(next_arg(args, &args));
	new->filesize = my_atol(next_arg(args, &args));
	new->bitrate = my_atol(next_arg(args, &args));
	new->freq = my_atol(next_arg(args, &args));
	new->seconds = my_atol(next_arg(args, &args));
	new->nick = m_strdup(next_arg(args, &args));
	new->ip = my_atol(next_arg(args, &args));
	new->speed = my_atol(next_arg(args, &args));
	new->type = AUDIO;
	if (!new->filename || !new->checksum || !new->nick || !new->filesize)
	{
		new_free(&new->filename);
		new_free(&new->checksum);
		new_free(&new->nick);
		new_free(&new);
		return 1;
	}
#ifdef THREAD1
	pthread_mutex_lock(&file_search_mutex);
#endif
#ifdef GUI
#ifdef GTK
	if (tgtk_okay() &&  get_int_var(GTK_VAR) && scott2_okay())
#endif
		scott2_add_search_file(from_server, new->nick, new->filename, new->filesize, new->bitrate, n_speed(new->speed));
#endif
#if 0
	if (get_icmp_socket(from_server) > -1)
	{
		new->icmp = 1;
		send_ping(get_icmp_socket(from_server), new->ip);
		get_time(&new->start);
	}
#endif
	add_to_list_double((List **)/*&file_search*/get_search_head(from_server), (List *)new);
#ifdef THREAD1
	pthread_mutex_unlock(&file_search_mutex);
#endif
	return 0;
}

int add_to_browse_list(NickStruct *n, char *nick, char *args)
{
FileStruct *new;

	new = (FileStruct *)new_malloc(sizeof(FileStruct));
	new->nick = m_strdup(nick);
	new->filename = m_strdup(new_next_arg(args, &args));
	new->checksum = m_strdup(next_arg(args, &args));
	new->filesize = my_atol(next_arg(args, &args));
	new->bitrate = my_atol(next_arg(args, &args));
	new->freq = my_atol(next_arg(args, &args));
	new->seconds = my_atol(next_arg(args, &args));
	new->speed = my_atol(args);
	new->type = AUDIO;
	if (!new->filename || !new->checksum || !new->nick || !new->filesize)
	{
		new_free(&new->filename);
		new_free(&new->checksum);
		new_free(&new->nick);
		new_free(&new);
		return 1;
	}
#ifdef THREAD1
	pthread_mutex_lock(&n->browse_mutex);
#endif
#ifdef GUI
#ifdef GTK
	if (tgtk_okay() &&  get_int_var(GTK_VAR) && scott2_okay())
#endif
		scott2_add_file(from_server, new->nick, new->filename, new->filesize, new->bitrate, n_speed(new->speed));
#endif

	add_to_list_double((List **)&n->file_browse, (List *)new);
#ifdef THREAD1
	pthread_mutex_unlock(&n->browse_mutex);
#endif
	return 0;
}

NAP_COMM(cmd_browse)
{
/* nick filename checksum filesize bitrate freq seconds */
char *nick;
NickStruct *n;
	if (!(nick = next_arg(args, &args)))
		return 0;
	if (!(n = (NickStruct *)find_in_list((List **)&server_list[from_server].users, nick, 0)))
		return 0;
	add_to_browse_list(n, n->nick, args);
	return 0;
}

char *base_name(char *str)
{
char *p;
	if ((p = strrchr(str, '\\')))
		p++;
	else if ((p = strrchr(str, '/')))
		p++;
	else
		p = str;
	return p;
}

char *mp3_time(time_t t)
{
static char str[40];
int seconds;
int minutes;
	minutes = t / 60;
	seconds = t % 60;
	sprintf(str, "%02d:%02d", minutes, seconds);
	return str;
}

void print_file(const FileStruct *f, int count, int maxwidth)
{
static char dir[BIG_BUFFER_SIZE];
char *fs;
int search_flags = get_server_search(from_server);

	if (!f || !f->filename)
		return;
	if (count == 1)
	{
		char fmtbuf[128];
		snprintf(fmtbuf,127,"%%3s . %%-%ds . %%3s . %%5s . %%6s . %%10s . %%4s . %%-16s . %%s",maxwidth);
		put_it(fmtbuf,"#","Song","Bit","Freq","Time","Size","Nick","Ping", "Speed");
		*dir = 0;
	}
	if ((fs = get_string_var(SEARCH_FORMAT_VAR)))
	{
		char *s;
		char *dirf;
		dirf = get_string_var(FORMAT_DIRECTORY_VAR);
		if (!(s = make_mp3_string(NULL, f, dirf, fs, dir)))
		{
			if ((s = make_mp3_string(NULL, f, dirf, fs, dir)))
				put_it("%3d %s %s", count, s, n_speed(f->speed));
		} else
			put_it("%3d %s %s", count, s, n_speed(f->speed));
	}
	else
	{
		char sizebuf[32],fmtbuf[128];
		char filename[PATH_MAX+1];
		char ping[32] = "N/A";
		strncpy(filename,f->filename,PATH_MAX);
		remove_trailing_spaces(filename);
		snprintf(fmtbuf,127,"%%3d | %%-%ds | %%3u | %%5u | %%6s | %%10s | %%4s | %%-16s |%%s",maxwidth);
		snprintf(sizebuf,31,"%4.2f%2s",(float)_GMKv(f->filesize),_GMKs(f->filesize));
		if (f->result.tv_sec != 0)
			snprintf(ping, 31, "%1.3f", time_diff(f->start, f->result)/2);
		put_it(fmtbuf, count, (search_flags == SEARCH_FULLPATH) ? filename : base_name(filename), f->bitrate, f->freq,
			mp3_time(f->seconds), sizebuf,f->nick, ping, n_speed(f->speed));
	}
}

NAP_COMM(cmd_endbrowse)
{
FileStruct *f;
int count = 0;
NickStruct *n;
char *nick;
	nick = next_arg(args, &args);
	if ((n = (NickStruct *)find_in_list((List **)&server_list[from_server].users, nick, 0)))
	{
		for (f = n->file_browse; f; f = f->next, count++)
			;
		if (n->file_browse && do_hook(BROWSE_END_LIST, "%s %d %s", nick, count, args ? args : empty_string))
#if 1 /* SCOTT */
			say("%d MP3's returned. Use /SBROWSE %s to browse interactive, or /browse %s", count, nick, nick);
#else
			say("%d MP3's returned. Use /browse %s", count, nick);
#endif
#if defined(GUI)
			update_browser();
#endif
	}
	if (n)
	{
		if (!n->file_browse)
			say("Browse finished, user %s has no MP3's available.", nick);
		n->flag &= ~BROWSE_IN_PROGRESS;	
		n->flag |= END_BROWSE;
	}
	return 0;
}

NAP_COMM(cmd_endsearch)
{
FileStruct **f, *s;
int count = 0;
	if (!(f = get_search_head(from_server)))
		return 0;
	for (s = *f ; s; s = s->next, count++)
		;
	if (f && *f)
	{
		if (do_hook(SEARCH_END_LIST, "%d", count))
#if 1 /* SCOTT */
			say("%d MP3's returned. Use /SSEARCH to browse interactive, or /search", count);
#else
			say("%d MP3's returned. Use /search", count);
#endif
#if defined(GUI)
			update_browser();
#endif
	}
	else
		say("Search finished. Server found no matches.");
	set_server_search(from_server, SEARCH_FINISH);
	return 0;
}





NAP_COMM(cmd_stats)
{
N_STATS *stats;
	do_hook(STATUS_UPDATE_LIST, "%s", args);
	if ((stats = get_server_stats(from_server)))
	{
		stats->libraries = my_atol(next_arg(args, &args));
		stats->songs = my_atol(next_arg(args, &args));
		stats->gigs = my_atol(next_arg(args, &args));
	}
	build_status(current_window, NULL, 0);
	return 0;
}

NAP_COMM(cmd_motd)
{
	if (!strncmp(args, "VERSION opennap", 15))
		set_server_version(from_server, OPENNAP_SERVER); 
	if (!strncmp(args, "SERVER ", 6)) 
	{
		char *s;
		s = LOCAL_COPY(args);
		next_arg(s, &s);
		if (s && *s)
			set_server_itsname(from_server, s);
	}
	if (!get_int_var(SUPPRESS_SERVER_MOTD_VAR))
		put_it("%s", args && *args ? args : " ");
	return 0;
}

NAP_COMM(cmd_bastard)
{
	put_it("Bastard command [%d] %s", cmd, args);
	return 0;
}

NAP_COMM(cmd_unknown)
{
	put_it("Unknown [%d] %s", cmd, args);
	return 0;
}

NAP_COMM(cmd_email)
{
	say("Email Address set to -> %s", args);
	set_string_var(DEFAULT_EMAIL_VAR, args);
	return 0;
}

NAP_COMM(cmd_login)
{
	send_ncommand(CMDS_LOGIN, "%s %s %d \"%s\" %d 5201 0",
		get_server_nickname(from_server), 
		get_server_password(from_server), 
		server_list[from_server].dataport,
		get_string_var(VERSION_VAR) ? get_string_var(VERSION_VAR) : nap_version,
		get_int_var(DEFAULT_SPEED_VAR));
	return 0;
}

NAP_COMM(cmd_joined)
{
	char *chan;
	Window *tmp = NULL;
	
	if ((chan = next_arg(args, &args)))
	{
		ChannelStruct *new;
		while (traverse_all_windows(&tmp))
		{
			if (tmp->server != from_server)
				continue;
	 		if (find_in_list((List **)&tmp->nchannels, chan, 0))
				return 0;
			if ((new = (ChannelStruct *)remove_from_list((List **)&tmp->waiting_channels, chan)))
				break;
		}
		if (new)
		{
			new_free(&new->channel);
			new_free(&new->topic);
			new_free(&new);
		}
		if (!tmp)
			tmp = current_window;
		new = (ChannelStruct *)new_malloc(sizeof(ChannelStruct));
		new->channel = m_strdup(chan);
		add_to_list((List **)&tmp->nchannels, (List *)new);
		new->injoin = 1;
		new->server = from_server;
		set_display_target(chan, LOG_CRAP);
		if (do_hook(CHANNEL_JOIN_LIST, "%s", chan))
			say("Joined channel %s", chan);
		malloc_strcpy(&tmp->current_channel, chan);
		build_status(current_window, NULL, 0);
	}
	return 0;
}

NAP_COMM(cmd_msg)
{
	char *from;
	from = next_arg(args, &args);
	if (!from || check_nignore(from))
		return 0;
	check_flooding(from, NULL, args, MSG_FLOOD);
	if (check_dcc_msg(from, args))
		return 0;
	set_display_target(from, LOG_MSG);
	if (do_hook(MSG_LIST, "%s %s", from, args))
		put_it(FORMAT_MSG, from, (FromUserHost && *FromUserHost) ? FromUserHost : "*@*", args);
	if (my_stricmp(get_server_nickname(from_server), from))
	{
		grab_http(from, get_server_nickname(from_server), args);
		malloc_strcpy(&recv_nick, from);
	}
	addtabkey(from, "msg", args);
	return 0;
}

NAP_COMM(cmd_partchannel)
{
char *chan;
ChannelStruct *new = NULL;

	if ((chan = next_arg(args, &args)))
	{
		Window *tmp = NULL;
		while (traverse_all_windows(&tmp))
		{
			if (tmp->server != from_server)
				continue;
			if ((new = (ChannelStruct *)find_in_list((List **)&tmp->nchannels, chan, 0)))
				break;
		}
		if (!new)
			return 0;
		set_display_target(chan, LOG_CRAP);
		if ((new = (ChannelStruct *)remove_from_list((List **)&tmp->nchannels, chan)))
		{
			free_nicks(new);
			new_free(&new->topic);
			new_free(&new->channel);
			new_free(&new);
		}
		if (!my_stricmp(chan, tmp->current_channel))
		{
			new_free(&tmp->current_channel);
			switch_channels_window(tmp);
		}
		if (do_hook(LEAVE_LIST, "%s %s", get_server_nickname(from_server), chan))
			say("You have parted %s", chan);
		update_window_status(current_window, 0);
		update_input(UPDATE_ALL);	
	}
	return 0;
}

NAP_COMM(cmd_parted)
{
char *chan;
ChannelStruct *new = NULL;
NickStruct *n;
	if ((chan = next_arg(args, &args)))
	{
		char *nick;
		Window *tmp = NULL;
		while (traverse_all_windows(&tmp))
		{
			if (tmp->server != from_server)
				continue;
			if ((new = (ChannelStruct *)find_in_list((List **)&tmp->nchannels, chan, 0)))
				break;
			else
				new = NULL;
		}
		if (!new)
			return 0;
		if (!(nick = next_arg(args, &args)))
			return 0;
		set_display_target(chan, LOG_CRAP);
		if ((n = (NickStruct *)remove_from_list_double((List **)&new->nicks, nick)))
		{
			int shared = 0;
			int speed = 0;
			shared = my_atol(next_arg(args, &args));
			speed = my_atol(args);
#ifdef THREAD1
			pthread_mutex_unlock(&n->browse_mutex);
#endif
			clear_filelist(&n->file_browse);
#ifdef THREAD1
			pthread_mutex_unlock(&n->browse_mutex);
#endif
			new_free(&n->nick);
			new_free(&n);
			if (do_hook(LEAVE_LIST, "%s %s %d %d", chan, nick, shared, speed) && 
				!check_cignore(chan) && !check_nignore(nick))
			{
				char part_str[200], *p;
				strcpy(part_str, "%s has parted %s [1;30m[       %d/%s[1;30m]");
				if ((p = strstr(part_str, "       ")))
					memcpy(p, speed_color(speed), 7);
				say(part_str, nick, chan, shared, n_speed(speed));
			}
		}
	}	
	return 0;
}

NAP_COMM(cmd_topic)
{
char *chan;
ChannelStruct *new = NULL;
	if ((chan = next_arg(args, &args)))
	{
		Window *tmp = NULL;
		while (traverse_all_windows(&tmp))
		{
			if (tmp->server != from_server)
				continue;
	 		if ((new = (ChannelStruct *)find_in_list((List **)&tmp->nchannels, chan, 0)))
	 			break;
			else
				new = NULL;
	 	}
	 	if (!new)
	 		return 0;
		set_display_target(chan, LOG_CRAP);
		new->topic = m_strdup(args);
		if (do_hook(TOPIC_LIST, "%s %s", chan, args))
			say("Topic for %s: %s", chan, args);
	}
	return 0;
}

NAP_COMM(cmd_names)
{
ChannelStruct *ch = NULL;
char *chan, *nick;
Window *tmp = NULL;
	chan = next_arg(args, &args);
	nick = next_arg(args, &args);
	if (!nick || !chan)
		return 0;
	set_display_target(chan, LOG_CRAP);
	while (traverse_all_windows(&tmp))
	{
		if (from_server != tmp->server)
			continue;
		if ((ch = (ChannelStruct *)find_in_list((List **)&tmp->nchannels, chan, 0)))
			break;
		else
			ch = NULL;
	}
	if (ch)
	{
		NickStruct *n;
		if (!(n = (NickStruct *)find_in_list((List **)&ch->nicks, nick, 0)))
		{
			n = (NickStruct *)new_malloc(sizeof(NickStruct));
			n->nick = m_strdup(nick);
			add_to_list_double((List **)&ch->nicks, (List *)n);
		}
		n->shared = my_atol(next_arg(args, &args));
		n->speed = my_atol(args);
		check_flooding(nick, chan, NULL, JOIN_FLOOD);
		if (!ch->injoin && do_hook(JOIN_LIST, "%s %s %lu %d", chan, nick, n->shared, n->speed) && !check_cignore(chan) && !check_nignore(nick))
		{
			char join_str[200], *p;
			if (FromUserHost && *FromUserHost)
				strcpy(join_str, "%s(%s) has joined %s [1;30m[       %d/%s[1;30m]");
			else
				strcpy(join_str, "%s%s has joined %s [1;30m[       %d/%s[1;30m]");
			p = strstr(join_str, "       ");
			memcpy(p, speed_color(n->speed), 7);
			say(join_str, nick, FromUserHost && *FromUserHost ? FromUserHost : empty_string, chan, n->shared, n_speed(n->speed));
		}
	}
	reset_display_target();
	return 0;
}

void name_print(const char *channel, const NickStruct *nicks, int hotlist, int sort, char *pattern)
{
char buffer[BIG_BUFFER_SIZE+1];
char tmp[BIG_BUFFER_SIZE];
char *p;
int cols = get_int_var(NAMES_COLUMNS_VAR);
int count = 0;
NickStruct *n1, *n2 = NULL;
int buf_len = 0;
char *name = NULL;

	if (sort == NORMAL_SORT)
		n1 = (NickStruct *)nicks;
	else
		n1 = n2;
		
	if (!cols)
		cols = 1;
	*buffer = 0;
	for (n1 = (NickStruct *)nicks; n1; n1 = n1->next)
	{
		if (pattern && !wild_match(pattern, n1->nick))
			continue;
		m_s3cat(&name, " ", n1->nick);
		m_s3cat(&name, " ", ltoa(n1->speed));
		m_s3cat(&name, " ", ltoa(n1->shared));
		count++;
	}
	if (do_hook(NAMES_LIST, "%d %s", count, name) < 1)
	{
		new_free(&name);
		return;
	}
	if (!hotlist)
		say("[Users(%s:%d)]", channel, count);
	count = 0;
	new_free(&name);
	for (n1 = (NickStruct *)nicks; n1; n1 = n1->next)
	{
		if (pattern && !wild_match(pattern, n1->nick))
			continue;
		buf_len = strlen(buffer);
		if (hotlist)
			sprintf(buffer+buf_len, "%s%16s%s%3s ", 
				(n1->speed == -1)?LIGHT_RED:LIGHT_GREEN, 
				n1->nick, LIGHT_WHITE, 
				(n1->speed == -1) ? "" : ltoa(n1->speed));
		else
		{
			strcpy(tmp, get_string_var(NAMES_NICKCOLOR_VAR));
			if ((p = strstr(tmp, "       ")))
				memcpy(p, speed_color(n1->speed), 7);
			sprintf(buffer+buf_len, tmp, n1->nick);
		}
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
}

NAP_COMM(cmd_endnames)
{
ChannelStruct *ch;
char *chan;
Window *tmp = NULL;
	if (!(chan = next_arg(args, &args)))
		return 0;
	set_display_target(chan, LOG_CRAP);
	while (traverse_all_windows(&tmp))
	{
		if (tmp->server == from_server)
		{
			if ((ch = (ChannelStruct *)find_in_list((List **)&tmp->nchannels, chan, 0)))
				break;
			else
				ch = NULL;
		}
	}
	if (!tmp)	
		tmp = current_window;
	if ((ch = (ChannelStruct *)find_in_list((List **)&tmp->nchannels, chan, 0)))
	{
		ch->injoin = 0;
		if (get_int_var(SHOW_CHANNEL_NAMES_VAR))
			name_print(chan, ch->nicks, 0, NORMAL_SORT, NULL);
	} else 
		say("Something screwy in cmd_endnames()");
	malloc_strcpy(&tmp->current_channel, chan);
	reset_display_target();
	return 0;
}

NAP_COMM(cmd_public)
{
	char *from, *chan;
	int its_me = 0;
	chan = next_arg(args, &args);
	from = next_arg(args, &args);
	if (!chan || !from || check_nignore(from) || check_cignore(chan))
		return 0;
	if (!my_stricmp(from, get_server_nickname(from_server)))
		its_me = 1;
	else
		check_flooding(from, chan, args, PUBLIC_FLOOD);
	set_display_target(chan, LOG_PUBLIC);
	grab_http(from, chan, args);
	if (is_current_channel(chan, from_server, 0))
	{
		if (do_hook(PUBLIC_LIST, "%s %s %s", from, chan, args))
			put_it(its_me ? FORMAT_SENDPUBLIC:FORMAT_PUBLIC, from, args);
	}
	else if (do_hook(PUBLIC_OTHER_LIST, "%s %s %s", from, chan, args))
		put_it(FORMAT_PUBLIC_OTHER, from, chan, args);
	reset_display_target();
	return 0;
}



NAP_COMM(cmd_error)
{
int nap_error = 0;
	if (args && !strcmp(args, "Invalid Password!"))
	{
		say("%s", args);
		nap_error = 11;
		close_server(from_server);
	}
	else if (args && !my_stricmp(args, "ping failed, "))
	{
		char *nick;
		next_arg(args, &args);
		next_arg(args, &args);
		nick = next_arg(args, &args);
		if (nick)
		{
			PingStruct *p;
			if ((p = (PingStruct *)remove_from_list((List **)ping_time, nick)))
			{
				new_free(&p->nick);
				new_free(&p);
			}
			say("Ping Failed, %s is not online", nick);
		}
	}
	else if (do_hook(ERROR_LIST, "%d %s", cmd, args ? args : empty_string))
		put_it("Received error for %d %s.", cmd, args ? args : empty_string);
	return 0;
}

void parse_server(N_DATA *packet, char *string)
{
int i;
	set_display_target(NULL, LOG_CRAP);
	current_numeric = -packet->command;
	if (!do_hook(current_numeric, "%s", string ? string : empty_string))
	{
		reset_display_target();
		return;
	}
	if (internal_debug & DEBUG_SERVER && !in_debug_yell)
		debugyell("%3d <<- %d %d %s", debug_count, packet->command, packet->len, string ? string : empty_string);
	for (i = 0; i < NUMBER_OF_COMMANDS; i++)
	{
		if (nap_commands[i].cmd == packet->command)
		{
			if (nap_commands[i].func)
				(nap_commands[i].func)(packet->command, string);
			else
				put_it("%s %s", numeric_banner(packet->command), string);
			reset_display_target();
			return;
		}
	}
	cmd_error(packet->command, string);
}


NAP_COMM(cmd_hotlist)
{
char *nick;
NickStruct *new;
	nick = next_arg(args, &args);
	set_display_target(nick, LOG_CRAP);
	if ((new = (NickStruct *)find_in_list((List **)&nap_hotlist, nick, 0)))
	{
		new->speed = my_atol(next_arg(args, &args));
		if (do_hook(HOTLIST_LIST, "%s %d ON", new->nick, new->speed))
			say("HotList User %s %s has signed on",new->nick, n_speed(new->speed));
	}
	return 0;
}

NAP_COMM(cmd_hotlistsuccess)
{
	set_display_target(args, LOG_CRAP);
	say("Adding %s to your HotList", args);
	return 0;
}

NAP_COMM(cmd_hotlisterror)
{
NickStruct *new;
	if (args && (new = (NickStruct *)remove_from_list((List **)&nap_hotlist, args)))
	{
		say("No such nick %s", args);
		new_free(&new->nick);
		new_free(&new);
	}
	return 0;
}

NAP_COMM(cmd_alreadyregistered)
{
	server_disconnect(current_window, "Already Registered");
	return 0;
}

NAP_COMM(cmd_offline)
{
NickStruct *new;
	if ((new = (NickStruct *)find_in_list((List **)&nap_hotlist, args, 0)))
	{
		if (do_hook(HOTLIST_LIST, "%s OFF", new->nick))
			say("HotList User %s has signed off",new->nick);
	}
	else
		say("User %s offline", args);
	return 0;
}

NAP_COMM(cmd_setdataport)
{
int port, pt = 0;
	port = my_atol(next_arg(args, &args));
	say("Server requested dataport set. [%d] %s", port, args ? args : "");
	make_listen(port, &pt);
	send_all_servers(CMDS_CHANGEDATA, "%d", pt);
	return 0;
}

NAP_COMM(cmd_dataport)
{
int pt = 0;
	say("* Data port misconfigured. Reconfiguring");
	make_listen(-1, &pt);
	send_all_servers(CMDS_CHANGEDATA, "%d", pt);
	return 0;
}

NAP_COMM(cmd_banlist)
{
	say("* %s", args);
	return 0;
}

NAP_COMM(cmd_registerinfo)
{
	say("Registered Username %s", get_server_nickname(from_server));
	send_ncommand(CMDS_REGISTERINFO, "%s %s %d \"%s\" %d %s", 
			get_server_nickname(from_server),
			get_server_password(from_server),
			get_int_var(DEFAULT_DATAPORT_VAR), version,
			get_int_var(DEFAULT_SPEED_VAR), 
			get_string_var(DEFAULT_EMAIL_VAR));
	return 0;
}

NAP_COMM(cmd_ping)
{
char *nick;
	if ((nick = next_arg(args, &args)))
	{
		set_display_target(nick, LOG_CRAP);
		if (do_hook(PONG_LIST, "%s %s", nick, args? args : empty_string))
		{
			say("%s has requested a ping", nick);
			send_ncommand(CMDS_PONG, "%s%s%s", nick, args ? " ":empty_string, args ? args : empty_string);
		}
	}
	return 0;
}


NAP_COMM(cmd_pingresponse)
{
char *nick;
	if ((nick = next_arg(args, &args)))
	{
		PingStruct *p;
		set_display_target(nick, LOG_CRAP);
		if ((p = (PingStruct *)remove_from_list((List **)&ping_time, nick)))
		{
			struct timeval tv;
			char t[80];
			get_time(&tv);
			snprintf(t, 70, "%1.3f", time_diff(p->start, tv));
			if (do_hook(PONG_LIST, "%s %s", nick, t))
				say("received pong from %s [%s seconds]", nick, t);
			new_free(&p->nick);
			new_free(&p);
		}
		else
			say("Unexpect pong from %s", nick);
	}
	return 0;
}

NAP_COMM(cmd_sping)
{
char *nick;
	if ((nick = next_arg(args, &args)))
	{
		char *secs;
		struct timeval start = {0};
		struct timeval tv;
		char t[80];
		get_time(&tv);
		set_display_target(nick, LOG_CRAP);
		secs = next_arg(args, &args); 
		if (secs && !my_strnicmp(secs, "LAG", 3))
		{
			start.tv_sec = (tv.tv_sec - my_atol(next_arg(args, &args)));
			start.tv_usec = (tv.tv_usec - my_atol(next_arg(args, &args)));
			set_server_lag(from_server, start);
			set_server_sping(from_server, 0);
		}
		else
		{
			start.tv_sec = my_atol(secs);
			start.tv_usec = my_atol(next_arg(args, &args));
			snprintf(t, 70, "%1.3f", time_diff(start, tv) / 2);
			if (do_hook(PONG_LIST, "%s %s", nick, t))
				say("received pong from %s [%s seconds]", nick, t);
		}
	}
	return 0;
}

NAP_COMM(cmd_endname) /* 830 */
{
	if (get_int_var(SHOW_END_OF_MSGS_VAR))
		say("End of names");
	return 0;
}

NAP_COMM(cmd_recname)
{
char *arg1, *arg2, *arg3;
int speed;
	arg1 = next_arg(args, &args);
	arg2 = next_arg(args, &args);
	arg3 = next_arg(args, &args);
	speed = my_atol(args);
	put_it("%20s %16s %8s %8s", arg1, arg2, arg3, n_speed(speed));
	return 0;
}

NAP_COMM(cmd_fatalerror)
{
	say("%s", args);
	return 0;
}

NAP_COMM(cmd_banlistend)
{
	if (get_int_var(SHOW_END_OF_MSGS_VAR))
		put_it("End of banlist");
	return 0;
}


void switch_channels_window(Window *win)
{
	if (win->nchannels)
	{
		ChannelStruct *ch;
		if (win->current_channel)
		{
			ch = (ChannelStruct *)find_in_list((List **)&win->nchannels, current_window->current_channel, 0);
			if (ch)
			{
				if (ch->next)
					malloc_strcpy(&win->current_channel, ch->next->channel);
				else
					malloc_strcpy(&win->current_channel, win->nchannels->channel);			
			}
		} else if (win->nchannels)
			malloc_strcpy(&win->current_channel, win->nchannels->channel);
		update_window_status(win, 0);
		update_input(UPDATE_ALL);
		do_hook(SWITCH_CHANNELS_LIST, "%s", win->current_channel ? win->current_channel:"none");
	}
	return;
}

void switch_channels(char unused, char *unused1)
{
	if (current_window->nchannels)
		switch_channels_window(current_window);
	return;
}

void move_window_channels(Window *win)
{
Window *tmp = NULL;
ChannelStruct *ch;
	if (win->server < 0 || !win->nchannels)
		return;
	while (traverse_all_windows(&tmp))
	{
		if (win != tmp && tmp->server == win->server)
			break;
	}
	if (tmp)
	{
		while (win->nchannels && (ch = (ChannelStruct *)remove_from_list((List **)&win->nchannels, win->nchannels->channel)))
			add_to_list((List **)&tmp->nchannels, (List *)ch);
	}
	else
	{
		for (ch = win->nchannels; ch; ch = ch->next)
			free_nicks(ch);
		clear_old_nchannels(&win->nchannels);
	}
	return;
}

NAP_COMM(cmd_toomanyfiles)
{
GetFile *sf;
char *nick, *filename;
int limit;
unsigned long filesize;
NickStruct *n;
	nick = next_arg(args, &args);
	filename = new_next_arg(args, &args);
	filesize = my_atol(next_arg(args, &args));
	limit = my_atol(args);
	if (!nick || !filename)
		return 0;
	if (!(n = (NickStruct *)find_in_list((List **)&server_list[from_server].users, nick, 0)))
		return 0;
	n->limit = limit;
	if ((sf = find_in_getfile(1, nick, NULL, filename, filesize, NAP_DOWNLOAD)))
	{
		if (sf->filestruct)
			sf->filestruct->flags |= NAP_QUEUED;
		sf->flags |= NAP_QUEUED;
		add_to_list_double((List **)&n->Queued, (List *)sf);
		say("Added %s from %s to download queue", base_name(filename), nick);
		build_status(current_window, NULL, 0);
	}
	return 0;
}

NAP_COMM(cmd_getlinespeed)
{
	say("Linespeed %s", args);
	return 0;
}

NAP_COMM(cmd_nosuchnick)
{
NickStruct *n;
char *nick;
	say("No such nick %s", args);
	nick = next_arg(args, &args);
	if (!nick || !(n = (NickStruct *)find_in_list((List **)&server_list[from_server].users, nick, 0)))
		return 0;
	if (n->prev)
		n->prev->next = n->next;
	else
	{
		server_list[from_server].users = n->next;
		if (server_list[from_server].users)
			server_list[from_server].users->prev = NULL;
	}
	if (n->next)
		n->next->prev = n->prev;	
#if defined(GTK)
	if (tgtk_okay() && get_int_var(GTK_VAR) && scott2_okay())
		scott2_remove_nick(from_server, n->nick);
#endif
	free_nickstruct(n);
	return 0;
}

NAP_COMM(cmd_announce)
{
char *nick;
	nick = next_arg(args, &args);
	set_display_target(NULL, LOG_WALLOP);
	put_it("*%s/announces*  %s", nick, args);
	return 0;	
}

NAP_COMM(cmd_wallop)
{
char *nick;
	nick = next_arg(args, &args);
	set_display_target(NULL, LOG_WALLOP);
	put_it("*%s/wallop*  %s", nick, args);
	return 0;	
}

typedef struct _nap_map_ {
	struct _nap_map_ *next;
	char *name;
	struct _nap_map_ *prev;
	char *remote;
	int hopcount;
	int localport;
	int remoteport;
} NapMap;

NapMap *map = NULL;

static int link_count = 0;

void add_to_nap_map(char *server, int localport, char *server1, int remoteport,
	int hops)
{
	NapMap *tmp, *last;

	tmp = (NapMap *) new_malloc(sizeof(NapMap));
	malloc_strcpy(&tmp->name, server);
	malloc_strcpy(&tmp->remote, server1);
	tmp->localport = localport;
	tmp->remoteport = remoteport;
	tmp->hopcount = hops;
	if (!map)
	{
		map = tmp;
		return;
	}
	for (last = map; last->next; last = last->next) 
		;
	last->next = tmp;
	tmp->prev = last;
}

void sort_nap_map_server(NapMap *start, NapMap *server)
{
	NapMap *current = start, *prev = map, *back;

	while (current)
	{
		/* If this server is linked to the current server we are checking,
		 * and the previous server in the list is not thre current server,
		 * and the previous server isn't linked to the current server we
		 * need to move this item, to be after the current server in the
		 * list.
		 */
		if (strcmp(current->name, server->remote) == 0 && prev != server && server->next != current)
		{
			back = prev;
			while (back && back->hopcount != current->hopcount)
			{
				back = back->prev;
			}
			if (!back || strcmp(back->name, server->remote) != 0)
			{
				NapMap *cprev = current->prev, *cnext = current->next;

				if (cprev)
					cprev->next = cnext;
				if (cnext)
					cnext->prev = cprev;

				current->next = server->next;
				if (server->next)
					server->next->prev = current;
				server->next = current;
				current->prev = server;
				sort_nap_map_server(start, server);
				return;
			}
		}
		prev = current;
		current = current->next;
	}
}

void sort_nap_map(void)
{
	NapMap **unsorted = new_malloc(sizeof(NapMap *) * (link_count+1));
	NapMap *start, *tmp1, *next, *prev;
	int z, max_linkcount = 0, i;

	for (tmp1 = map, z = 0; tmp1; tmp1 = tmp1->next, z++)
	{
		if (tmp1->hopcount > max_linkcount)
			max_linkcount = tmp1->hopcount;
		unsorted[z] = tmp1;
	}
	for (i = max_linkcount; i; i--)
	{
		int k;
		start = map;
		for (z = 0; unsorted[z]; z++)
		{
			if (!(start = start->next)) 
				break;
			sort_nap_map_server(start, unsorted[z]);
		}
		for (k = 0; unsorted[k]; k++)
		{
			tmp1 = unsorted[k];
			if (k)
				prev = unsorted[k - 1];
			else
				prev = NULL;
			next = unsorted[k+1];
			tmp1->next = next;
			tmp1->prev = prev;
		}
		map = unsorted[0];
	}
	new_free(&unsorted);

}

void show_server_map (void)
{
	int prevdist = 0;
	NapMap *tmp;
	char tmp1[80];
	char tmp2[BIG_BUFFER_SIZE+1];
#ifdef ONLY_STD_CHARS
	char *ascii="-> ";
#else
	char *ascii = "57ÀÄ>-1 ";
#endif			    

	if (map) 
		prevdist = map->hopcount;
	
	say("Nap Server Map");
/*	sort_nap_map();*/
	for (tmp = map; tmp; tmp = map)
	{
		int len;
		map = tmp->next;
		if (!tmp->hopcount || tmp->hopcount != prevdist)
			sprintf(tmp1, "50[52%d50]", tmp->hopcount);
		else
			*tmp1 = 0;
		len =  (prevdist!=tmp->hopcount) ? 10 + (tmp->hopcount * 4) : 4 + (tmp->hopcount * 4);
		sprintf(tmp2, "%*s%s%s %s", 
			len, 
			prevdist!=tmp->hopcount?ascii:empty_string, 
				tmp->name, tmp1, tmp->remote);
		put_it("%s", tmp2);
		prevdist = tmp->hopcount;
		new_free(&tmp->name);
		new_free(&tmp->remote);
		new_free((char **)&tmp);
	}	
}

NAP_COMM(cmd_links)
{
char *host, *host1;
int port, port1, hops;

	if (!args || !*args)
	{
		show_server_map();
		say("%d linked servers", link_count);
		link_count = 0;
		return 0;
	}
#if 0
	if (link_count == 0)
		put_it("%-3s %25s %6s %25s %6s %8s", "#", "Hostname", "Port", "Hostname", "Port", "Hops");
#endif
	link_count++;
	host = next_arg(args, &args);
	port = my_atol(next_arg(args, &args));
	host1 = next_arg(args, &args);
	port1 = my_atol(next_arg(args, &args));
	hops = my_atol(next_arg(args, &args));
	if (map)
		add_to_nap_map(host1, port1, host, port, hops);
	else
	{
		add_to_nap_map(host, port, host1, port1, hops);
		add_to_nap_map(host1, port1, host, port, hops);
	}
#if 0
	put_it("%3d %25s %6d %25s %6d %8d", link_count, host, port, host1, port1, bufsize);
#endif
	return 0;
}


NAP_COMM(cmd_usage)
{
	char *num_clients, *num_servers, *num_channels, *total_users;
	unsigned long registered;
	char buff[80], buff1[80];
	unsigned long num_files;
	unsigned long pending_searches = 0;
	int mem_usage;
	
	time_t suptime, start_t;
	time_t  days,hours,minutes,seconds;

	num_clients = next_arg(args, &args);
	num_servers = next_arg(args, &args);
	total_users = next_arg(args, &args);

	num_files = my_atoul(next_arg(args, &args));
	strcpy(buff1, next_arg(args, &args));
	num_channels = next_arg(args, &args);

	start_t = my_atoul(next_arg(args, &args));
	suptime = my_atoul(next_arg(args, &args));
	mem_usage = my_atol(next_arg(args, &args));
	registered = my_atol(next_arg(args, &args));
	
	seconds = suptime % 60;
	suptime = (suptime - seconds) / 60;
	minutes = suptime % 60;
	suptime = (suptime - minutes) / 60;
	hours = suptime % 24;
	days = (suptime - hours) / 24;

	strcpy(buff, ulongcomma(num_files));
	
	say("Server stats: Started %s", my_ctime(start_t));
	put_it("\tClients %7s\t Servers %12s\t Channels %18s", num_clients, num_servers, num_channels);
	put_it("\tShares  %7s\t Files   %12s\t Bytes    %18s", total_users, buff, slongcomma(buff1));
	if (mem_usage <= 0)
		put_it("\tMemory Usage %lu", mem_usage);
	if (args && *args)
	{
		char *in_speed, *out_speed, *searches;
		char bytes_in[80], bytes_out[80];
		double bytesin = 0, bytesout = 0;
		double total;
		int new_stats = 0;
		in_speed = next_arg(args, &args);
		out_speed = next_arg(args, &args);
		searches = next_arg(args, &args);
		if (args && *args)
		{
			bytesin = atof(next_arg(args, &args));
			bytesout = atof(next_arg(args, &args));
			new_stats = 1;
			pending_searches = my_atol(next_arg(args, &args));
		}
		put_it("\tRegistered users %lu\tSearches/second %s\t Pending %lu", registered, searches, pending_searches);
		total = atof(in_speed) + atof(out_speed);
		put_it("\tBW IN/OUT %8sKB/s %8sKB/s == %4.1fKB/s", in_speed, out_speed, total);
		if (new_stats)
		{
			sprintf(bytes_in, "%.0f", bytesin);
			sprintf(bytes_out, "%.0f", bytesout);
			strcpy(buff, slongcomma(bytes_in));
			total = bytesin + bytesout;
			put_it("\tBytes IN/OUT %s(%4.2f%s) %s(%4.2f%s) == %4.2f%s", 
				buff, _GMKv(bytesin), _GMKs(bytesin), 
				slongcomma(bytes_out), _GMKv(bytesout), _GMKs(bytesout),
				_GMKv(total), _GMKs(total));
		}
	} else
		put_it("\tRegistered users %lu", registered);
	put_it("\tUptime  %7d Days  %02d Hours  %02d Minutes  %02d Seconds", days, hours, minutes, seconds);
	return 0;
}

#if defined(WANT_PTEST) && defined(WANT_THREAD)

/* SHEIK */

void send_user_to_port(int server, char *nick, int port)
{
	int serv = 0;
	char buff[50];
	sprintf(buff, "%d", port);
	serv = from_server;
	from_server = server;
	send_ncommand(CMDR_SETDATAPORT, "%s %s", nick, buff);
	if (serv == -1)
		say("from_server == -1");
	else
		from_server = server;
}


int find_next_port(int port)
{
char *pt, *q;
	if (!(q = get_string_var(PTEST_CHANGE_PORT_VAR)))
		return port;
	pt = LOCAL_COPY(q);
	while ((q = next_arg(pt, &pt)))
	{
		if (my_atol(q) == port)
		{
			if (!(q = next_arg(pt, &pt)))
			{
				pt = LOCAL_COPY(get_string_var(PTEST_CHANGE_PORT_VAR));
				q = next_arg(pt, &pt);
			}
			return my_atol(q);	
		}
	}
	pt = LOCAL_COPY(get_string_var(PTEST_CHANGE_PORT_VAR));
	q = next_arg(pt, &pt);
	return my_atol(q);
}


void
ptest_callback(int result, int r_errno, void *data)
{
	PT_INFO *tmp = (PT_INFO *) data;
	N_STATS *stats;
	/* if there was no data, we got an error, it should always be there */
	if (!tmp || tmp->server == -1)
		return;

	stats = get_server_stats(from_server);
	stats->total_unreach_pending--;
	if (result == 0)
		stats->total_unreach_bogus++;
	else 
	{
		int next_p;
		stats->total_unreach_real++;
		next_p = find_next_port(tmp->port);
		if (!next_p)
			stats->total_unreach_switch_to_zero++;
		send_user_to_port(tmp->server, tmp->to, next_p);
	}
	free_pt_info(tmp);
}
 

/* Ugly ugly ugly. But its gotta be done, if we want to do ptest */
static void parse_servmsg(char *line)
{
	char *arg1, *from = NULL, *to = NULL, *ip = NULL, *dport = NULL;
	int port = 0;
	PT_INFO *tmp;
	N_STATS *stats;
	
/* We are looking for the following message:
 * Notification from grifdogg55: sfq (206.102.239.5) - configured data port 1000 is unreachable.
 */
	if (!(stats = get_server_stats(from_server)))
		return;
	arg1 = next_arg(line, &line);
	if (!arg1 || strcmp(arg1, "Notification"))
		return;
	arg1 = next_arg(line, &line);
	if (!arg1 || strcmp(arg1, "from"))
		return;
	from = next_arg(line, &line);
	if (!from)
		return;
	/* eat the : */
	arg1 = index(from, ':');
		if (arg1)
			*arg1 = '\0';
	to = next_arg(line, &line);
	if (!to)
		return;
	ip = next_arg(line, &line);
	if (!ip)
		return;
	/* eat the ( */
	ip++;
	/* eat the ) */
	arg1 = index(ip, ')');
		if (arg1)
			*arg1 = '\0';
	arg1 = next_arg(line, &line);
	if (!arg1 || strcmp(arg1, "-"))
		return;
	arg1 = next_arg(line, &line);
	if (!arg1 || strcmp(arg1, "configured"))
		return;
	arg1 = next_arg(line, &line);
	if (!arg1 || strcmp(arg1, "data"))
		return;
	arg1 = next_arg(line, &line);
	if (!arg1 || strcmp(arg1, "port"))
		return;
	dport = next_arg(line, &line);
	if (!dport)
		return;
	port = atoi(dport);
	if (!port)
		return;
	arg1 = next_arg(line, &line);
	if (!arg1 || strcmp(arg1, "is"))
		return;
	arg1 = next_arg(line, &line);
	if (!arg1 || strcmp(arg1, "unreachable."))
		return;

	/* Finally, if we got here, its a message for ptest */
	stats->total_unreach_messages++;

	/* If they are 1 - 1023, make them go to another port */
	if (port > 0 && port < 1024) {
		stats->total_unreach_real++;
		send_user_to_port(from_server, to, 6969);
		return;
	}
	/* Check for dupes, lets not flood the remote user. */
	if (ptest_stats.last_port == port && ptest_stats.last_ip && 
	    *ptest_stats.last_ip && !strcmp(ptest_stats.last_ip, ip)) {
		stats->total_unreach_duplicates++;
		return;
	}

	tmp = (PT_INFO *) new_malloc(sizeof(PT_INFO));
	tmp->to = tmp->from = tmp->ip = NULL;
	malloc_strcpy(&tmp->to, to);
	malloc_strcpy(&tmp->from, from);
	malloc_strcpy(&tmp->ip, ip);
	tmp->server = from_server;
	tmp->port = port;

	/* Store, for dupe checking, and yes, malloc_strcpy()
	 * new_free()'s the last_ip that was there before, ie, no mem leak here!
	 */
	malloc_strcpy(&(ptest_stats).last_ip, ip);
	ptest_stats.last_port = port;
	stats->total_unreach_pending++;	
	set_display_target(NULL, LOG_SERVER);
	add_to_ptest_queue(ip, port, ptest_callback, NULL, tmp, PTEST_NORMAL);
}
#endif

NAP_COMM(cmd_servermsg)
{
	char *copy = NULL;
#if defined(WANT_PTEST) && defined(WANT_THREAD)
	if (get_int_var(PTEST_VAR) == ON) {
		malloc_strcpy(&copy, args);
		parse_servmsg(copy);
		new_free(&copy);
	}
#endif	
	if (args && (wild_match("User % is not currently online*", args) || wild_match("user % is not a known user", args)))
	{
		char *nick, *n;
		nick = LOCAL_COPY(args);
		next_arg(nick, &nick);
		n = next_arg(nick, &nick);
		who_remove_queue(from_server, n);
		/* damn them. need to remove the whois queue for this nick */
		say("%s", args);
		return 0;
	}
	else if (args && wild_match("*set your user level to*", args))
	{
		int i = 0;
		set_server_admin(from_server, USER_USER);
		for (i = USER_LEECH; *class[i]; i++)
			if (strstr(args, class[i]))
				set_server_admin(from_server, i);
		say("%s", args);
		return 0;
	}
	else if (check_wait_command(args))
		return 0;
	else if (!strncmp("You are now cloaked", args, 19))
	{
		set_server_cloak(from_server, 1);
		build_status(current_window, NULL, 0);
	}
	else if (!strncmp("You are no longer cloaked", args, 24))
	{
		set_server_cloak(from_server, 0);
		build_status(current_window, NULL, 0);
	}
	if (wild_match("% invited you to channel *", args))
	{
		char *chan, *nick;
		char *s;
		/* qr1 invited you to channel q*/
		chan = LOCAL_COPY(args);
		nick = next_arg(chan, &chan);
		next_arg(chan, &chan);
		next_arg(chan, &chan);
		next_arg(chan, &chan);
		next_arg(chan, &chan);
		malloc_strcpy(&last_invited, chan);
		if ((s = convert_to_keystr("JOIN_LAST_INVITE")) && *s)
		{
			say("%s has invited you to %s. Press %s to join", nick, chan, s);
			return 0;
		}
	}
	if (wild_match("% [ops/%]: *", args))
	{
		if (do_hook(WALL_LIST, "%s", args))
			say("%s", args);
		return 0;
	}
	set_display_target(NULL, LOG_SERVER);
	if (do_hook(SERVERMSG_LIST, "%s", args))
	{
		if (!check_server_ignore(args))
			say("%s", args);
	}
	return 0;
}

NAP_COMM(cmd_resumerequest)
{
FileStruct *new;
ResumeFile *rf;
	new = (FileStruct *)new_malloc(sizeof(FileStruct));
	new->nick = m_strdup(next_arg(args, &args));
	new->ip = my_atol(next_arg(args, &args));
	new->port = my_atol(next_arg(args, &args));
	new->filename = m_strdup(new_next_arg(args, &args));
	new->checksum = m_strdup(next_arg(args, &args));
	new->filesize = my_atol(next_arg(args, &args));
	new->speed = my_atol(next_arg(args, &args));
	new->type = AUDIO;
	for (rf = resume_struct; rf; rf = rf->next)
		if (new->checksum && !strcmp(new->checksum, rf->checksum) && rf->filesize == new->filesize)
			break;
	if (!rf || !new->filename || !new->checksum || !new->nick || !new->filesize)
	{
		new_free(&new->filename);
		new_free(&new->checksum);
		new_free(&new->nick);
		new_free(&new);
		return 1;
	}
	add_to_list_double((List **)&rf->results, (List *)new);
	set_server_resume(from_server, get_server_resume(from_server)+1);
	return 0;
}

NAP_COMM(cmd_resumerequestend)
{
	put_it("* Resume search finished %d results %s", get_server_resume(from_server), args);
	set_server_resume(from_server, 0);
	return 0;
}

NAP_COMM(cmd_showusersend)
{
unsigned int *user_count;
char *cmdstr;
	cmdstr = get_server_showusercmd(from_server);
	get_server_showuser(from_server, &user_count);
	say("End of showusers [%d]%s%s", *user_count, cmdstr ? " Using ":empty_string, cmdstr ? cmdstr : empty_string);
	set_server_showuser(from_server, NULL, 0);
	set_server_showusercmd(from_server, NULL);
	return 0;
}

NAP_COMM(cmd_showusers)
{
	if (args && *args)
	{
		char *nick, *host, *m, *cmdstr;
		unsigned int *count;
		m = get_server_showuser(from_server, &count);
		if ((cmdstr = get_server_showusercmd(from_server)))
		{
			parse_line(NULL, cmdstr, args, 0, 0, 1);
			(*count)++;
			return 0;
		}
		nick = next_arg(args, &args);
		host = next_arg(args, &args);
		if (!m || wild_match(m, nick) || wild_match(m, host))
		{
			if (*count == 0)
				say("%-22s IP", "Nickname");
			put_it("    %-22s %s", nick, host);
			(*count)++;
		}
	}
	return 0;
}

/*
 * Rewritten by David Walluck (DavidW2) Fri Mar 26 08:13:36 EST 1999 to use
 * linked lists and add /set MAX_URLS.
 */


int grab_http(char *from, char *to, char *text) 
{
static int count = 0;
char *q = NULL;
	if (get_int_var(URL_GRAB_VAR) && (stristr(text, "HTTP:") || stristr(text, "FTP:") || stristr(text, "FTP.")))
	{
		malloc_sprintf(&q, "%s %s -- %s", from, to, text);
		for (cur_url = url_list, prev_url = NULL; cur_url; prev_url = cur_url, cur_url = cur_url->next)
		{
			if (cur_url->name && !my_stricmp(cur_url->name, q))
			{
				new_free(&q);
				return 0;
			}
		}

		while (url_count >= get_int_var(MAX_URLS_VAR))
		{
			if (!prev_url)
				url_list = NULL;
			else
				url_list = url_list->next;
			url_count--;	
		}
		url_count++;
		count++;
		new_url = (UrlList *) new_malloc(sizeof(UrlList));
		new_url->name = q;
		new_url->next = cur_url;
		if (!prev_url)
			url_list = new_url;
		else
			prev_url->next = new_url;

		say("Added HTTP/FTP grab [%d/%d]", url_count, count);
		return 1;
	}
	return 0;
}

BUILT_IN_COMMAND(url_grabber)
{
int do_display = 1;
	if (args && *args)
	{
		int i, q;
		char *p;
		while ((p = next_arg(args, &args)))
		{
			if (!my_stricmp(p, "SAVE"))
			{
				char *filename = NULL;
				FILE *file;
				char buffer[BIG_BUFFER_SIZE+1];
				char *number = "*";

#ifdef WINNT
				snprintf(buffer, BIG_BUFFER_SIZE, "~/TekNap.url");
#else
				snprintf(buffer, BIG_BUFFER_SIZE, "~/.TekNap/TekNap.url");
#endif
				filename = expand_twiddle(buffer);
				if (!filename || !(file = fopen(filename, "a")))
				{
					new_free(&filename);
					return;
				}
				if (args && *args && (isdigit(*args) || *args == '-'))
					number = next_arg(args, &args);
				for (i = 0, cur_url = url_list; cur_url; cur_url = cur_url->next, i++)
				{
					if (matchnumber(number, i) && cur_url->name)
						fprintf(file, "%s\n", cur_url->name);
				}
				new_free(&filename);
				fclose(file);
				url_count = 0;
                                prev_url = url_list;
				while (prev_url)
				{
					cur_url = prev_url;
					prev_url = cur_url->next;
					new_free(&cur_url);
				}
				url_list = NULL;
				say("Url list saved");
				do_display = 0;
			}
			else if (!my_stricmp("URL", p))
			{
				int on = get_int_var(URL_GRAB_VAR);
				char *ans = next_arg(args, &args);
				if (ans && !my_stricmp(ans, "ON"))
					on = 1;
				else
					on = 0;
				set_int_var(URL_GRAB_VAR, on);
			}
			else if (*p == '-')
  			{
                        	if (!*++p)
                        	{
                        		url_count = 0;
					prev_url = url_list;
					while (prev_url)
					{
						cur_url = prev_url;
						prev_url = cur_url->next;
						new_free(&cur_url);
					}
					url_list = NULL;	
					say("Url list cleared");
				}
				else
				{       
					q = my_atol(p);
					if (q <= 0 || q > get_int_var(MAX_URLS_VAR))
					{
						say("Url [%d] not found", q);
						break;
					}
					for (i = 1, cur_url = url_list, prev_url = NULL; cur_url && i != q; prev_url = cur_url, cur_url = cur_url->next, i++);
					if (i == q)
					{
						if (!prev_url)
							url_list = url_list->next;
						else
							prev_url->next = cur_url->next;
        					new_free(&cur_url);
						say("Cleared Url [%d]", q);
						url_count--;
					}
					else
						say("Url [%d] not found", q);
				}
				do_display = 0;
			}
			else if (!my_stricmp("LIST", p) || !my_stricmp("+", p))
			{
				if (!url_list)
				{
					say("No Urls in Url list");
					return;
				}
				say("Url list:");
				for (i = 1, cur_url = url_list; cur_url; cur_url = cur_url->next, i++)
				{
					if (!cur_url->name)
						break;
					put_it("HTTP[%3d] %s", i, cur_url->name);
				}
				do_display = 0;
			}
		}
	}
	if (do_display)
		say("HTTP grab [%3s]", on_off(get_int_var(URL_GRAB_VAR)));
}

/* napster will put the channel name first and the nick second in
 * upcoming releases of there server
 */
static int cbancount = 0; /* XXX fix me */

NAP_COMM(cmd_endcban)
{
	put_it("54*-1 There are %d bans", cbancount);
	cbancount = 0;
	return 0;
}

NAP_COMM(cmd_cbanlist)
{
	if (!cbancount)
		put_it("54*-1 Cbanlist for %s", current_window->current_channel ? current_window->current_channel : empty_string);
	put_it("54*-1 %s", args);
	cbancount++;
	return 0;
}

NAP_COMM(generic_handler)
{
	put_it("54*-1 %s", args ? args : empty_string);
	return 0;
}

NAP_COMM(ignore_numeric)
{
	return 0;
}
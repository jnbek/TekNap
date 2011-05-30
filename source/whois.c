 /* $Id: whois.c,v 1.3 2001/10/11 16:27:08 edwards Exp $ */
 
#include "teknap.h"
#include "struct.h"
#include "commands.h"
#include "hook.h"
#include "ircaux.h"
#include "if.h"
#include "list.h"
#include "napster.h"
#include "output.h"
#include "server.h"
#include "cdns.h"

extern Server *server_list;

static WhoEntry *pop_who_entry(int server, char *nick)
{
WhoEntry *tmp = NULL;
	if (server < 0 || server > server_list_size() || 
	    !(tmp = server_list[server].who_queue))
		return NULL;
	if (!nick)
	{
		server_list[server].who_queue = tmp->next;
		tmp->next = NULL;
	}
	else
		tmp = (WhoEntry *)remove_from_list((List **)&server_list[server].who_queue, nick);
	return tmp;
}

static void who_queue_add (int server, WhoEntry *item)
{
	WhoEntry *bottom = server_list[server].who_queue;
	while (bottom && bottom->next)
		bottom = bottom->next;

	if (!bottom)
		server_list[server].who_queue = item;
	else
		bottom->next = item;
	return;
}

static void who_delete_item(WhoEntry *tmp)
{
	if (!tmp)
		return;
	new_free(&tmp->who_target);
	new_free(&tmp->who_end);
	new_free(&tmp->who_mask);
	new_free(&tmp->who_result);
	new_free(&tmp);
}

void who_remove_queue(int server, char *nick)
{
WhoEntry *tmp;
	if (server < 0 || server > server_list_size() || 
	    !(tmp =server_list[server].who_queue))
		return;
	if (nick && !my_stricmp(tmp->who_target, nick))
	{
		if ((tmp = pop_who_entry(server, nick)))
			who_delete_item(tmp);
	}
	else
	{
		if ((tmp = (WhoEntry *)remove_from_list((List **)&server_list[server].who_queue, nick)))
			who_delete_item(tmp);
	}
}

static int who_queue_pop (int server)
{
	WhoEntry *save;
	int count = 0;
	while ((save = pop_who_entry(server, NULL)))
	{
		who_delete_item(save);
		count++;
	}
	return count;
}

static void who_queue_list (int server)
{
	WhoEntry *item;
	int count = 0;

	if (server < 0)
		return;
	item = server_list[server].who_queue;
	while (item)
	{
		yell("[%d] [%d] [%s] [%s] [%s]", count,
			item->who_mask,
			item->who_target ? item->who_target : empty_string,
			item->who_result ? item->who_result : empty_string,
			item->who_end ? item->who_end : empty_string);
		count++;
		item = item->next;
	}
}

static void who_queue_flush (int server)
{
	who_queue_pop(server);
	yell("Who queue for server [%d] purged of %d entries", server, who_queue_pop(server));
}



static void 
add_whois_queue(int server, char *nick, char *cmd, unsigned long flags, void (*end)(WhoEntry *, char *, char *))
{
WhoEntry *tmp;
	if (server < 0)
		return;
	tmp = (WhoEntry *)new_malloc(sizeof(WhoEntry));
	tmp->who_target = m_strdup(nick);
	if (cmd)
		tmp->who_end = m_strdup(cmd);
	tmp->flags = flags;
	tmp->end = end;
	who_queue_add(server, tmp);
	if (get_server_ircmode(server))
		send_to_server("WHOIS %s", nick);
	else
		send_ncommand(CMDS_WHOIS, "%s", nick);
}

static void 
add_whowas_queue(int server, char *nick, char *cmd, unsigned long flags, void (*end)(WhoEntry *, char *, char *))
{
WhoEntry *tmp;
	if (server < 0)
		return;
	tmp = (WhoEntry *)new_malloc(sizeof(WhoEntry));
	tmp->who_target = m_strdup(nick);
	if (cmd)
		tmp->who_end = m_strdup(cmd);
	tmp->flags = flags;
	tmp->end = end;
	who_queue_add(server, tmp);
	if (get_server_ircmode(server))
		send_to_server("WHOWAS %s", nick);
	else
		send_ncommand(CMDS_WHOWAS, "%s", nick);
}

void whobase(char *args, int whowas)
{
unsigned long flags = 0;
char *cmd = NULL, *a;
	while ((a = next_arg(args, &args)))
	{
		if (!my_stricmp(a, "-cmd"))
		{
			cmd = next_expr(&args, '{');
			if (!cmd)
			{
				say("Missing cmd {}");
				return;
			}
			continue;
		}
		else if (!my_stricmp(a, "-flush"))
		{
			who_queue_flush(from_server);
			return;
		}
		else if (!my_stricmp(a, "-list"))
		{
			who_queue_list(from_server);
			return;
		}
		if (whowas)
			add_whowas_queue(from_server, a, cmd, flags, NULL);
		else
			add_whois_queue(from_server, a, cmd, flags, NULL);
	}
}

#if defined(WANT_THREAD) && defined(WANT_NSLOOKUP)
void dns_whois_callback(DNS_QUEUE * dns)
{
	say("DNS for (%s) %s -> %s", ((WhoEntry*)dns->callinfo)->who_target, dns->in, dns->out);
}

void add_dns_whoqueue(WhoEntry *who, char *nick, char *args)
{
WhoEntry *who1;
char *ip;
char *loc;

	who1 = (WhoEntry *)new_malloc(sizeof(WhoEntry));
	who1->who_end = who->who_end;
	who1->who_result = who->who_result;
	who1->who_target = who->who_target;
	who1->who_mask = who->who_mask;
	who->who_mask = who->who_end = who->who_target = who->who_result = NULL;
	loc = LOCAL_COPY(args);
	new_next_arg(loc, &loc);
	new_next_arg(loc, &loc);
	new_next_arg(loc, &loc);
	new_next_arg(loc, &loc);			
	new_next_arg(loc, &loc);
	new_next_arg(loc, &loc);
	new_next_arg(loc, &loc);
	new_next_arg(loc, &loc);
	new_next_arg(loc, &loc);
	next_arg(loc, &loc);
	next_arg(loc, &loc);
	ip = next_arg(loc, &loc);
	add_to_dns_queue(ip, dns_whois_callback, who1->who_end, who1, DNS_URGENT);
}

void dnsbase(char *args)
{
unsigned long flags = 0;
char *cmd = NULL, *a;
	while ((a = next_arg(args, &args)))
	{
		if (!my_stricmp(a, "-cmd"))
		{
			cmd = next_expr(&args, '{');
			if (!cmd)
			{
				say("Missing cmd {}");
				return;
			}
			continue;
		}
		else if (!my_stricmp(a, "-flush"))
		{
			who_queue_flush(from_server);
			return;
		}
		else if (!my_stricmp(a, "-list"))
		{
			who_queue_list(from_server);
			return;
		}
		add_whois_queue(from_server, a, cmd, flags, add_dns_whoqueue);
	}
}
#else
void dnsbase(char *args)
{
	say("Sorry this command requires Threads");
}
#endif

void who_result(int server, char *line)
{
WhoEntry *who;
char *args, *nick;
int o_f_s = from_server;

	if (server < 0)
		return;
	args = LOCAL_COPY(line);
	nick = next_arg(args, &args);
	
	if (!(who = pop_who_entry(server, nick)))
		return;
	from_server = server;

	who->who_result = m_strdup(args);
	if (who->end)
		(who->end)(who, nick, who->who_result);
	else if (who->who_end)
		parse_line(NULL, who->who_end, line, 0, 0, 1);
	else if (do_hook(WHO_LIST, "%s %s", nick, args))
	{
		char	*class, *status, *channels, *ver, 
			*l_ip, *l_port, *d_port, *email, *server, *coder, *ip;
		int	shared, download, upload, speed, t_down, t_up;
		time_t	online;

		coder = NULL;
		class = new_next_arg(args, &args);
		online = my_atol(new_next_arg(args, &args));
		channels = new_next_arg(args, &args);
		status = new_next_arg(args, &args);			
		shared = my_atol(new_next_arg(args, &args));
		download = my_atol(new_next_arg(args, &args));
		upload = my_atol(new_next_arg(args, &args));
		speed = my_atol(new_next_arg(args, &args));
		ver = new_next_arg(args, &args);
		t_down = my_atol(next_arg(args, &args));
		t_up = my_atol(next_arg(args, &args));
		l_ip = next_arg(args, &args);
		l_port = next_arg(args, &args);
		d_port = next_arg(args, &args);		
		email = next_arg(args, &args);
		server = next_arg(args, &args);	
		ip = next_arg(args, &args);
		put_it("52ÚÄÄÄÄÄÄÄÄ32Ä52ÄÄ32ÄÄ52Ä32ÄÄÄÄÄÄÄÄÄ50Ä32ÄÄ50ÄÄ32Ä50ÄÄÄÄÄÄÄÄÄÄ-Ä ÄÄ  Ä   -");
		if (!strcasecmp(nick, "drscholl") || !strcasecmp(nick, "q") || !strcasecmp(nick, "Sheiker") || !strcasecmp(nick, "nuke"))
			coder = "Coder";
		else if (!strcasecmp(nick, "jasta"))
			coder = "Whiner";
		else if (!strcasecmp(nick, "fudd"))
			coder = "Scripter";
		else if (!strcasecmp(nick, "dusk"))
			coder = "Perl Slut";
		else if (!strcasecmp(nick, "sabina") || !strncasecmp(nick, "bina", 4))
			coder = "cyber-slut";
		else if (!strcasecmp(nick, "cosima"))
			coder = "motorcycle Mama";
		else if (!strcasecmp(nick, "Whoa_There_Chief"))
			coder = "vagrant";

		if (l_ip)
			put_it("52³57 User     :-1 %s50(-1%s50)-1 %s l57:-1%s d57:-1%s",nick, email, l_ip, l_port, d_port);
		else
			put_it("52³57 User     :-1 %s",nick);
		if (ip)
			put_it("52³57 I-1P       57:-1 %s", ip);
		if (server)
			put_it("52³57 S-1erver   57:-1 %s", server);
		if (coder)
			put_it("52|57 C-1lass    57:-1 %s 50(-1%s50)", coder, class);
		else
			put_it("52|57 C-1lass    57:-1 %s", class);
		put_it("32³ 57L-1ine     57:-1 %s", n_speed(speed));
		put_it("32³ 57T-1ime     57:-1 %s", convert_time(online));
		put_it("32³ 57C-1hannels 57:-1 %s", channels ? channels : empty_string);
		put_it("50³ 57S-1tatus   57:-1 %s", status);
		put_it("50³ 57S-1hared   57:-1 %d", shared);
		put_it("50: 57C-1lient   57:-1 %s", ver);
		put_it("50: 57U-1pload   57:-1 %d 57D-1ownload 57:-1 %d", upload, download);
		if (t_down || t_up)
			put_it("50:-1 Total  U 57:-1 %d D57 :-1 %d", t_up, t_down);
	}
	who_delete_item(who);
	from_server = o_f_s;
}

void whowas_result(int server, char *line)
{
WhoEntry *who;
char *args, *nick;
int o_f_s = from_server;

	if (server < 0)
		return;
	args = LOCAL_COPY(line);
	nick = next_arg(args, &args);
	
	if (!(who = pop_who_entry(server, nick)))
		return;
	from_server = server;
#if 0
Received error for 10121 insane 111424721 bitchx.dimension6.com 974923198.
#endif
	who->who_result = m_strdup(args);
	if (who->end)
		(who->end)(who, nick, who->who_result);
	else if (who->who_end)
		parse_line(NULL, who->who_end, line, 0, 0, 1);
	else if (do_hook(WHO_LIST, "%s %s", nick, args))
	{
		char	*server, *coder, *ip;
		struct sockaddr_in l_ip;
		time_t	online;

		coder = NULL;
		ip = new_next_arg(args, &args);
		l_ip.sin_addr.s_addr = my_atoul(ip);
		server = new_next_arg(args, &args);
		online = my_atol(new_next_arg(args, &args));

		put_it("52ÚÄÄÄÄÄÄÄÄ32Ä52ÄÄ32ÄÄ52Ä32ÄÄÄÄÄÄÄÄÄ50Ä32ÄÄ50ÄÄ32Ä50ÄÄÄÄÄÄÄÄÄÄ-Ä ÄÄ  Ä   -");
		put_it("52³57 User     :-1 %s50(-1%s50)-1",nick, inet_ntoa(l_ip.sin_addr));
		if (server)
			put_it("52³57 S-1erver   57:-1 %s", server);
		put_it("32³ 57T-1ime     57:-1 %s", my_ctime(online));
	}
	who_delete_item(who);
	from_server = o_f_s;
}


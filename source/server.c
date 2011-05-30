/* $Id: server.c,v 1.4 2001/09/19 16:57:48 edwards Exp $ */
/* Added updated napigator server listing code - Spike */ 

#include "teknap.h"
#include "struct.h"
#include "hook.h"
#include "list.h"
#include "newio.h"
#include "output.h"
#include "server.h"
#include "status.h"
#include "timer.h"
#include "vars.h"
#include "window.h"
#include "napster.h"
#include "scott2.h"
#include "tgtk.h"
#include "irchandler.h"

extern int from_server;
extern	int nick_count;
extern	int channel_count;

#ifdef WANT_THREAD
#ifdef WINNT
pthread_mutex_t send_ncommand_mutex = {0};
#else
pthread_mutex_t send_ncommand_mutex = PTHREAD_MUTEX_INITIALIZER;
#endif
#endif


int nap_data = -1;
int nap_dataport = -1;
extern int byte_order;
extern int set_capability;

static int number_of_servers = 0;

Server *server_list = NULL;

#define SERVER_CONNECTED	0x0001
#define SEARCH_IN_PROGRESS	0x0010
#define SERVER_CREATE		0x0100

NAP_COMM(cmd_login);

void clear_shared(void);
static void removetabkey(int);
int login_to_server(int server);
int add_to_server_list (char *, int, char *, char *, int, int);

struct sockaddr_in localaddr;

struct _sock_manager
{
	int init;
	int count;
	int max_fd;
	SocketList sockets[FD_SETSIZE+1];
} sock_manager =  {0, 0, -1, {{ 0, 0, 0, NULL, 0, 0, NULL, NULL, NULL }}};


#define SOCKET_TIMEOUT 120



void set_server_bits(fd_set *rd, fd_set *wr)
{
	int i;
	for (i = 0; i < number_of_servers; i++)
	{
#if TEST_NONBLOCK
		if (server_list[i].write > -1)
			FD_SET(server_list[i].write, wr);
#endif
		if (server_list[i].read > -1)
			FD_SET(server_list[i].read, rd);
	}
}

int server_list_size(void)
{
	return number_of_servers;
}

int auto_reconnect(void *arg, char *sub)
{
Window *tmp = NULL, *last = NULL;
int server = my_atol((char *)arg);
int rc = -1;
	while (traverse_all_windows(&tmp))
	{
		if (server >= number_of_servers)
			continue;
		if ((tmp->server == -1) && (tmp->last_server != -1) && (tmp->last_server == server))
		{
			last = tmp;
			if ((rc = connect_to_server_by_refnum(server, -1, (server_list[server].flags & SERVER_CREATE) ? 1 : 0)) > -1)
			{
				tmp->server = server;
				tmp->last_server = -1;
			}
			break;
		}
	}	
	if (rc == -1 && server < number_of_servers)
	{
		add_timer(0, empty_string, 15 * 1000, 1, auto_reconnect, arg, NULL, last ? last->refnum : current_window->refnum, "auto-reconnect");
		return 0;
	}

	new_free(&arg);
	tmp = NULL;
	while (traverse_all_windows(&tmp))
	{
		if (tmp->last_server != -1 && tmp->last_server == server)
		{
			tmp->server = server;
			tmp->last_server = -1;
		}
	}
	return 0; 
}

void start_autoconnect(int server)
{
Window *tmp = NULL;
	while (traverse_all_windows(&tmp))
		if (tmp->last_server != -1 && tmp->last_server == server)
			break;
	add_timer(0, empty_string, 15 * 1000, 1, auto_reconnect, m_sprintf("%d", server), NULL, tmp ? tmp->refnum : current_window->refnum, "auto-reconnect");
	say("Started Autoreconnect Timer");
}

void do_server(fd_set *rd, fd_set *wr)
{
char buffer[BIG_BUFFER_SIZE+1];
int i;
N_DATA data;

	for (i = 0; i < number_of_servers; i++)
	{
		if (server_list[i].read < 0)
			continue;
#ifdef TEST_NONBLOCK
		if (server_list[i].write != -1 && 
			FD_ISSET(server_list[i].write, wr) && 
				!(server_list[i].flags & SERVER_CONNECTED))
		{
			login_to_server(i);			
			server_list[i].write = -1;
			continue;
		}
#endif
		if (FD_ISSET(server_list[i].read, rd))
		{
			char *bufptr = &buffer[0];
			int junk;
			from_server = i;
			*bufptr = 0;
			/* irc mode check here */
			if (server_list[i].irc_mode)
				junk = new_dgets(bufptr, server_list[i].read, 1, BIG_BUFFER_SIZE);
			else
				junk = dgets(bufptr, server_list[i].read, 1, BIG_BUFFER_SIZE);
			switch (junk)
			{
				case -2:
					break;
				case -1:
				{
					close_server(i);
					if (do_hook(DISCONNECT_LIST, "Connection closed from %s: %s", server_list[i].name, (dgets_errno <= 0) ? "Remote end closed connection" : strerror(dgets_errno)))
						say("do_server() Connection closed from %s: %s", server_list[i].name, (dgets_errno <= 0) ? "Remote end closed connection" : strerror(dgets_errno));
					start_autoconnect(i);
					break;
				}					
				case 0:
					if (server_list[i].irc_mode)
						break;
					dget_data(server_list[i].read, &data, sizeof(N_DATA));
					if (data.len)
						break;
				default:
				{
					if (server_list[i].irc_mode)
						parse_irc_server(buffer);
					else
					{
						dget_data(server_list[i].read, &data, sizeof(N_DATA));
						parse_server(&data, buffer);
						dget_clear_data(server_list[i].read);
					}
					break;
				}
			}
		}
	}
}

int get_server_search(int serv)
{
	if (serv > -1 && serv < number_of_servers)
		return (server_list[serv].flags & SEARCH_IN_PROGRESS) ? 1 : 0;
	return 0;
}

void set_server_search(int serv, int offon)
{
	if (serv > -1 && serv < number_of_servers)
	{
		if (offon)
			server_list[serv].flags |= SEARCH_IN_PROGRESS;
		else if (server_list[serv].flags & SEARCH_IN_PROGRESS)
			server_list[serv].flags &= ~SEARCH_IN_PROGRESS;
	}
}

int check_socket(int s)
{
	if (s != -1 && sock_manager.count && sock_manager.sockets[s].is_read)
		return 1;
	return 0;
}


unsigned long set_socketflags(int s, unsigned long flags)
{
	if (check_socket(s))
	{
		sock_manager.sockets[s].flags = flags;
		return sock_manager.sockets[s].flags;
	}
	return 0;
}

unsigned long get_socketflags(int s)
{
	if (check_socket(s))
		return sock_manager.sockets[s].flags;
	return 0;
}

char *get_socketserver(int s)
{
	if (check_socket(s))
		return sock_manager.sockets[s].server;
	return NULL;
}

void *get_socketinfo(int s)
{
	if (check_socket(s))
		return sock_manager.sockets[s].info;
	return NULL;
}

void set_socketinfo(int s, void *info)
{
	if (check_socket(s))
		sock_manager.sockets[s].info = info;
}

int get_max_fd(void)
{
	return sock_manager.max_fd + 1;
}

void set_socket_read (fd_set *rd, fd_set *wr)
{
register int i;
static int socket_init = 0;
	if (!socket_init)
	{
		memset(&sock_manager, 0, sizeof(sock_manager));
		socket_init++;
		sock_manager.init++;
	}
	if (!sock_manager.count) return;
	for (i = 0; i < sock_manager.max_fd + 1; i++)
	{
		if (sock_manager.sockets[i].is_read > 0)
			FD_SET(sock_manager.sockets[i].is_read, rd);
		if (sock_manager.sockets[i].is_write > 0)
			FD_SET(sock_manager.sockets[i].is_write, wr);
	}
}

int add_socketread(int s, int port, unsigned long flags, char *server, void (*func_read)(int), void (*func_write)(int))
{
	if (!sock_manager.init)
	{
		fd_set rd;
		FD_ZERO(&rd);
		set_socket_read(&rd, &rd);
	}
	if (s < 0)
		return -1;
	if (s > FD_SETSIZE)
		return -1;
	if (s > sock_manager.max_fd)
		sock_manager.max_fd = s;
	sock_manager.count++;
	sock_manager.sockets[s].is_read = s;
	sock_manager.sockets[s].port = port;
	sock_manager.sockets[s].flags = flags;
	if (server)
		sock_manager.sockets[s].server = m_strdup(server);
	sock_manager.sockets[s].func_read = func_read;
	sock_manager.sockets[s].func_write = func_write;
	return new_open(s);
}

int set_socketwrite(int s)
{
	if (s > FD_SETSIZE || !check_socket(s))
		return -1;
	if (s > sock_manager.max_fd)
		sock_manager.max_fd = s;
	sock_manager.sockets[s].is_write = s;	
	new_open_write(s);
	return s;
}

void add_sockettimeout(int s, time_t timeout, void *func)
{
	if (!check_socket(s))
		return;
	if (timeout < 0)
	{
		timeout = get_int_var(CONNECT_TIMEOUT_VAR);
		if (timeout <= 0)
			timeout = SOCKET_TIMEOUT;
	}
	if (timeout)
		sock_manager.sockets[s].time = now + timeout;
	else
		sock_manager.sockets[s].time = 0;
	sock_manager.sockets[s].cleanup = func;
}
	
void close_socketread(int s)
{
	if (!sock_manager.count || !check_socket(s))
		return;
	if (sock_manager.sockets[s].is_read > -1)
	{
		new_free(&sock_manager.sockets[s].server);
		new_close(s);
		sock_manager.count--;
		if (s == sock_manager.max_fd)
		{
			int i;
			sock_manager.max_fd = -1;
			for (i = 0; i < FD_SETSIZE; i++)
				if (sock_manager.sockets[i].is_read > 0)
					sock_manager.max_fd = i;
				else if( sock_manager.sockets[i].is_write > 0)
					sock_manager.max_fd = i;
		}
	}
	memset(&sock_manager.sockets[s], 0, sizeof(SocketList));
	sock_manager.sockets[s].is_read = sock_manager.sockets[s].is_write = -1;
}

void close_all_sockets(void)
{
int i;
	if (!sock_manager.count) return;
	for (i = 0; i < sock_manager.max_fd+1; i++)
	{
		if (sock_manager.sockets[i].is_read > 0)
			close_socketread(i);
	}
}

int read_sockets(int s, unsigned char *str, int len)
{
	if (!check_socket(s))
		return -1;
	return read(s, str, len);
}


void scan_sockets(fd_set *rd, fd_set *wr)
{
register int i;
time_t t = now;
	if (!sock_manager.count) return;
	for (i = 0; i < sock_manager.max_fd+1; i++)
	{
		if (sock_manager.sockets[i].is_read > 0 && FD_ISSET(sock_manager.sockets[i].is_read, rd))
			(sock_manager.sockets[i].func_read) (sock_manager.sockets[i].is_read);
		if (sock_manager.sockets[i].func_write && sock_manager.sockets[i].is_write > 0 && FD_ISSET(sock_manager.sockets[i].is_write, wr))
			(sock_manager.sockets[i].func_write) (sock_manager.sockets[i].is_write);
		if (sock_manager.sockets[i].time && (t >= sock_manager.sockets[i].time))
		{
			if (sock_manager.sockets[i].cleanup)
				(sock_manager.sockets[i].cleanup)(i);
			else
				close_socketread(i);
		}
	}
}

SocketList *get_socket(int s)
{
	if (check_socket(s))
		return &sock_manager.sockets[s];
	return NULL;
}

int naplink_getserver(char *host, u_short port, int create, void *func)
{
	struct	in_addr	address;
	struct	hostent	*hp;
	int	nap_socket;
	if ((address.s_addr = inet_addr(host)) == -1)
	{
		if (!my_stricmp(host, "255.255.255.0") || !(hp = gethostbyname(host)))
		{
			say("NAP Unknown host: %s", host);
			return -1;
		}
		bcopy(hp->h_addr, (void *)&address, sizeof(address));
	}
#ifdef TEST_NONBLOCK
	nap_socket = connect_by_number(host, &port, SERVICE_CLIENT, PROTOCOL_TCP, 1);
#else
	nap_socket = connect_by_number(host, &port, SERVICE_CLIENT, PROTOCOL_TCP, 0);
#endif
	if (nap_socket < 0)
		return -1;
	if (func)
		add_socketread(nap_socket, from_server, create, host, func, NULL);
	say("Attempting to get host from %s:%d.", host, port);
	return nap_socket;
}

int login_to_server(int server)
{
	server_list[server].flags |= SERVER_CONNECTED;
#ifdef TEST_NONBLOCK
	server_list[server].write = -1;
#endif
	if (!get_server_password(server))
		set_server_password(server, get_string_var(DEFAULT_PASSWORD_VAR));
	if (!get_server_nickname(server))
		set_server_nickname(server, get_string_var(DEFAULT_NICKNAME_VAR));

	if (server_list[server].flags & SERVER_CREATE)
	{
		if (set_capability)
			send_ncommand(920, "%d", 62);
		send_ncommand(CMDS_CREATEUSER, "%s", server_list[server].d_nickname);
		server_list[server].flags &= ~SERVER_CREATE;
	}
	else
	{
		if (set_capability)
			send_ncommand(920, "%d", 62);
		cmd_login(CMDS_LOGIN, empty_string);
		send_hotlist();
		check_serverlag(server);
		add_files_to_whois();
	}
#if defined(GTK)
	if (tgtk_okay() && get_int_var(GTK_VAR) && scott2_okay())
		scott2_add_server(server);
#endif
	set_window_server(-1, server, 1);
	return server;
}

static int _naplink_connectserver(int server, char *tmp, int create)
{
char *s_port;
unsigned short port;
int serv = -1;
Window *win = NULL;
	if ((s_port = strchr(tmp, '\n')))
		*s_port = 0;
	if (!(s_port = strchr(tmp, ':')))
	{
		next_arg(tmp, &s_port);
		if (!s_port)
		{
			say("Error in naplink_connectserver()");
			return -1;
		}
	} else
		*s_port++ = 0;
	port = atoi(s_port);
	if ((serv = naplink_getserver(tmp, port, create, NULL)) > -1)
	{
		int len = sizeof(struct sockaddr_in);
		from_server = add_to_server_list(tmp, port, get_server_password(server), get_server_nickname(server), 0, 1);
		while (traverse_all_windows(&win))
		{
			if (win->server == server)
				win->server = from_server;
		}
		server = from_server;
		set_napster_socket(serv);
		new_open(serv);

		if (nap_data == -1)
			nap_data = make_listen(get_int_var(DEFAULT_DATAPORT_VAR), &nap_dataport);
		else
			nap_dataport = get_int_var(DEFAULT_DATAPORT_VAR);
 		server_list[from_server].dataport = nap_dataport;

		server_list[from_server].read = server_list[from_server].write = serv;
		if (create)
			server_list[from_server].flags |= SERVER_CREATE;
		login_to_server(server);
		if (do_hook(CONNECT_LIST, "%s %d", tmp, port))
			say("Connected. Attempting Login to %s:%s.", tmp, s_port);
		getsockname(serv, (struct sockaddr *)&localaddr, &len);
		return server;
	}
	return -1;
}


static void naplink_connectserver (int s)
{
char tmpstr[BIG_BUFFER_SIZE+1];
char *tmp = tmpstr;
SocketList *s1;
	s1 = get_socket(s);
	memset(tmpstr, 0, sizeof(tmpstr));
	read(s, tmp, sizeof(tmpstr)-1);
	if (*tmp)
	{
#if 1
		if (!my_strnicmp(tmp, "127.0.0.1", 3))
		{
			say("Servers are busy, try again later");
			close_socketread(s);
			return;
		}
#endif
		if ((_naplink_connectserver(s1->port, tmp, s1->flags)) != -1)
			set_server_itsname(from_server, tmp);
	}
	else
		say("Error connecting to server");
	close_socketread(s);
}


int connect_to_server(char *nick, char *pass, char *server, int port, int meta, int create)
{
char buffer[BIG_BUFFER_SIZE+1];
	if (meta)
		return naplink_getserver(server, port, create, naplink_connectserver);
	
	sprintf(buffer, "%s %d", server, port);
	return _naplink_connectserver(from_server, buffer, create);
}

int connect_to_server_by_refnum(int server, int old_serv, int create)
{
	from_server = server;
	return connect_to_server(server_list[server].d_nickname, 
		server_list[server].password, server_list[server].name, 
		server_list[server].port, server_list[server].meta, create);
}

int send_ncommand(unsigned int ncmd, char *fmt, ...)
{
char buffer[2*BIG_BUFFER_SIZE+1];
N_DATA n_data = {0};
va_list ap;
int rc;
int writelen;

	if (from_server == -1)
	{
		do_hook(DISCONNECT_LIST, "Try connecting to a server first");
		return -1;
	}
	if (get_server_ircmode(from_server))
		return -1;
	if (fmt)
	{
		va_start(ap, fmt);
		n_data.len = vsnprintf(buffer+4, 2*BIG_BUFFER_SIZE, fmt, ap);
		va_end(ap);
	}
	n_data.command = ncmd;

/*
 * This is so incredibly wrong. We can thank the shortsightness of 
 * Napster, Inc., who has never heard of byte order. 
 * This is a horrible hack required because they expect intel order.
 */
	writelen = n_data.len;
	if (set_capability)
	{
		n_data.len = htons(n_data.len);
		n_data.command = htons(n_data.command);
	}
	else
	{
		n_data.len = BSWAP16(n_data.len);
		n_data.command = BSWAP16(n_data.command);
	}
	memcpy(buffer, &n_data.len, 2);
        memcpy(buffer+2, &n_data.command, 2);
	                
#ifdef WANT_THREAD
	pthread_mutex_lock(&send_ncommand_mutex);
#endif

	if (internal_debug & DEBUG_SERVER && !in_debug_yell)
		debugyell("%3d ->> %d %d %s", debug_count, ncmd, writelen, fmt ? buffer : empty_string);

	server_list[from_server].sent = 1;

	rc = NAP_send(buffer, sizeof(n_data) + writelen);
	
	
#if 0
	rc = NAP_send(&n_data, sizeof(n_data));
	if (fmt)
	{
		rc = NAP_send(buffer, writelen);
#ifdef WANT_THREAD
		pthread_mutex_unlock(&send_ncommand_mutex);
#endif
		return rc;
	}
#endif

#ifdef WANT_THREAD
	pthread_mutex_unlock(&send_ncommand_mutex);
#endif
	return (rc != -1) ? 0 : -1;
}

char *get_server_name(int server)
{
	if (server > -1 && server < number_of_servers)
		return server_list[server].name;
	return NULL;
}

char *get_server_itsname(int server)
{
	if (server > -1 && server < number_of_servers)
		return server_list[server].itsname;
	return NULL;
}

void set_server_itsname(int server, char *val)
{
	if (server > -1 && server < number_of_servers)
	{
		if (val && *val)
			malloc_strcpy(&server_list[server].itsname, val);
		else
			new_free(&server_list[server].itsname);
	}
}

char *get_server_nickname(int server)
{
	if (server > -1 && server < number_of_servers)
		return server_list[server].d_nickname;
	return NULL;
}


void set_server_password(int server, char *val)
{
	if (server < 0 || server >= number_of_servers)
		return;
	if (val)
		malloc_strcpy(&server_list[server].password, val);
	else
		new_free(&server_list[server].password);
}

void set_server_nickname(int server, char *val)
{
	if (server < 0 || server >= number_of_servers)
		return;
	if (val)
		malloc_strcpy(&server_list[server].d_nickname, val);
	else
		new_free(&server_list[server].d_nickname);
}

char *get_server_password(int server)
{
	if (server > -1 && server < number_of_servers)
		return server_list[server].password;
	return NULL;
}

int get_server_dataport(int server)
{
	return nap_data;
}

void set_server_dataport(int server, int port)
{

}

int get_nap_socket(int server)
{
	if (server > -1 && server < number_of_servers)
		return server_list[server].read;
	return -1;
}

int get_server_motd(int server)
{
	if (server > -1 && server < number_of_servers)
		return server_list[server].motd;
	return 0;
}

void set_server_motd(int server, int val)
{
	if (server > -1 && server < number_of_servers)
		server_list[server].motd = val;
}

void close_server(int server)
{
NickStruct *new, *last;
N_STATS *stats;
Window *tmp = NULL;
ChannelStruct *ch;
int last_server = 1;

	if (server < 0 || server >= number_of_servers)
		return;
	if (server_list[server].read != -1)
		new_close(server_list[server].read);
	
	server_list[server].read = server_list[server].write = -1;
	server_list[server].cloak = server_list[server].flags = 0;
	server_list[server].sping = server_list[server].user_count = 0;
	new_free(&server_list[server].showuser);
	new_free(&server_list[server].showusercmd);
	new_free(&server_list[server].itsname);

	if (server_list[server].waiting_out > server_list[server].waiting_in)
		server_list[server].waiting_out = server_list[server].waiting_in = 0;

	nick_count = 0;
	channel_count = 0;
	clear_nicks(server);
	if (server_list[server].icmp_sock > -1)
		server_list[server].icmp_sock = new_close(server_list[server].icmp_sock);	

	clear_filelist(&server_list[server].search_results);
	clear_shared();
	new_free(&last_invited);
#if defined(GTK)
	if (tgtk_okay() && get_int_var(GTK_VAR) && scott2_okay())
		scott2_remove_server(server);
#endif
          
	for (new = server_list[server].users; new; new = last)
	{
		last = new->next;
		free_nickstruct(new);
	}	
	server_list[server].users = NULL;
	shared_stats.shared_files = 0;
	shared_stats.shared_filesize = 0;
	if ((stats = get_server_stats(server)))
	{
		stats->libraries = 0;
		stats->gigs = 0;
		stats->songs = 0;
	}
	for (new = nap_hotlist; new; new = new->next)
		new->speed = -1;
	while (traverse_all_windows(&tmp))
	{
		if (tmp->server == server)
		{
			tmp->server = -1;
			tmp->last_server = server;
			for (ch = tmp->nchannels; ch; ch = ch->next)
				ch->injoin = 1;
			tmp->oldchannels = tmp->nchannels;
			tmp->nchannels = NULL;
			new_free(&tmp->current_channel);
		}
		else if (tmp->server != -1)
			last_server = 0;
	}
	if (last_server)
	{
		clear_filelist(&server_list[server].search_results);
		clear_shared();
	}
	set_server_search(server, SEARCH_FINISH);
	update_window_status_all();
}

int is_connected(int server)
{
	if (server > -1 && server < number_of_servers)
		return server_list[server].flags & SERVER_CONNECTED ? 1 : 0;
	return -1;
}

void close_data(int server)
{
	if (nap_data != -1)
		close_socketread(nap_data);
	nap_data = -1;
}


void close_all_servers(void)
{
int i;
	for (i = 0; i < number_of_servers; i++)
	{
		clear_nicks(i);
		close_server(i);
		close_data(i);
	}
}


void server_disconnect(Window *window, char *args)
{
Window *tmp = NULL;
int serv = -1;

	if (window->server == -1)
		return;
	while (traverse_all_windows(&tmp))
	{
		if (tmp->server != -1)
			serv = tmp->server;
	}
	if (serv == -1)
		close_data(window->server);
	close_server(window->server);

	build_status(window, NULL, 0);
	if (do_hook(DISCONNECT_LIST, "Server and dataport closed %s", args ? args : "by user request"))
		say("Server and Dataport closed %s", args ? args : "by user request");

	from_server = serv;
#if 0
	tmp = NULL; serv = window->server;
	while (traverse_all_windows(&tmp))
	{
		if (tmp->server == serv)
		{
			window->last_server = window->server;
			window->server = -1;
		}
	}
	from_server = serv;
#endif
}

int make_listen(int port, int *portnum)
{
int fd;
unsigned short pt;
int count = 0;
	if (nap_data > 0)
		close_socketread(nap_data);
	nap_data = -1;
	if (port == -1)
		pt = get_int_var(DEFAULT_DATAPORT_VAR);
	else
		pt = port;
	if (pt <= 0)
	{
		*portnum = 0;
		return 0;
	}
	while ((fd = connect_by_number(NULL, &pt, SERVICE_SERVER, PROTOCOL_TCP, 1)) < 0)
	{
		pt++;
		if (count++ > 19)
			break;
	}
	if (fd <= 0)
	{
		say("Cannot setup listen port [%d-%d] %s", (port == -1) ? get_int_var(DEFAULT_DATAPORT_VAR) : port, pt, strerror(errno));
		if (portnum)
			*portnum = 0;
		return 0;
	}
	say("Setup for dataport [%d]", pt);
	if (portnum)
		*portnum = pt;
	add_socketread(fd, pt, 0, NULL, naplink_handlelink, NULL);
	nap_data = fd;
	return nap_data;
}

int	find_in_server_list (char *server, int port, int meta)
{
	int	i,
		len, hintfound = -1;
	
	len = strlen(server);

	for (i = 0; i < number_of_servers; i++)
	{
		if (port && server_list[i].port && port != server_list[i].port && port != -1)
			continue;

		/*
		 * Try to avoid unneccessary string compares. Only compare
		 * the first part of the string if there's not already a
		 * possible match set in "hintfound". This enables us to
		 * search for an exact match even if there's already a
		 * fuzzy-match, without having to compare twice.
		 */
		if ((-1 != hintfound) || !my_strnicmp(server, server_list[i].name, len))
		{
			if (!my_stricmp(server, server_list[i].name))
				return i;
			else if (-1 == hintfound)
				hintfound = i;
		}
	}
	return (hintfound);
}

void 	remove_from_server_list (int i)
{
	Window	*tmp = NULL;

	if (i < 0 || i >= number_of_servers)
		return;

	say("Deleting server [%d]", i);

	close_server(i);
	new_free(&server_list[i].name);
	new_free(&server_list[i].itsname);
	new_free(&server_list[i].password);
	new_free(&server_list[i].d_nickname);
	removetabkey(i);

	/* 
	 * this should save a coredump.  If number_of_servers drops
	 * down to zero, then trying to do a realloc ends up being
	 * a free, and accessing that is a no-no.
	 */
	if (number_of_servers == 1)
	{
		say("Sorry, the server list is empty and I just dont know what to do.");
		nap_exit(1, "NULL server list", NULL);
	}

	memmove(&server_list[i], &server_list[i + 1], (number_of_servers - i - 1) * sizeof(Server));
	number_of_servers--;
	RESIZE(server_list, Server, number_of_servers);
	removetabkey(i);
	while ((traverse_all_windows(&tmp)))
		if (tmp->server > i)
			tmp->server--;
}

/*
 * parse_server_index:  given a string, this checks if it's a number, and if
 * so checks it validity as a server index.  Otherwise -1 is returned 
 */
int	parse_server_index (char *str)
{
	int	i;

	if (is_number(str))
	{
		i = my_atol(str);
		if ((i >= 0) && (i < number_of_servers))
			return (i);
	}
	return (-1);
}

/*
 * add_to_server_list: adds the given server to the server_list.  If the
 * server is already in the server list it is not re-added... however, if the
 * overwrite flag is true, the port and passwords are updated to the values
 * passes.  If the server is not on the list, it is added to the end. In
 * either case, the server is made the current server. 
 */
int add_to_server_list (char *server, int port, char *password, char *nick, int meta, int overwrite)
{
int serv;
	if ((serv = find_in_server_list(server, port, meta)) == -1)
	{
		serv = number_of_servers++;
		RESIZE(server_list, Server, number_of_servers+1);
		memset(&server_list[serv], 0, sizeof(Server));		
		server_list[serv].name = m_strdup(server);
		server_list[serv].meta = meta;
		server_list[serv].read = -1;
		server_list[serv].write = -1;
		server_list[serv].port = port;
		server_list[serv].icmp_sock = -1;
		server_list[serv].motd = get_int_var(SUPPRESS_SERVER_MOTD_VAR);
		if (password && *password)
			malloc_strcpy(&(server_list[serv].password), password);
		else
			malloc_strcpy(&server_list[serv].password, get_string_var(DEFAULT_PASSWORD_VAR));
		if (nick && *nick)
			malloc_strcpy(&(server_list[serv].d_nickname), nick);
		else
			malloc_strcpy(&server_list[serv].d_nickname, get_string_var(DEFAULT_NICKNAME_VAR));

	}
	else
	{
		if (overwrite)
		{
			server_list[serv].port = port;
			if (password || !server_list[serv].password)
			{
				if (password && *password)
					malloc_strcpy(&server_list[serv].password, password);
				else
					malloc_strcpy(&server_list[serv].password, get_string_var(DEFAULT_PASSWORD_VAR));
			}
			if (nick || !server_list[serv].d_nickname)
			{
				if (nick && *nick)
					malloc_strcpy(&server_list[serv].d_nickname, nick);
				else
					malloc_strcpy(&server_list[serv].d_nickname, get_string_var(DEFAULT_NICKNAME_VAR));
			}
		}
		server_list[serv].motd = get_int_var(SUPPRESS_SERVER_MOTD_VAR);
		if (strlen(server) > strlen(server_list[serv].name))
			malloc_strcpy(&(server_list[serv].name), server);
	}
	return serv;
}

/*
 * servername port nick password meta
 */
void	parse_server_info (char *name, char **port, char **password, char **nick, char **meta)
{
	char *ptr, delim;

	delim = (index(name, ',')) ? ',' : ':';

	*port = *password = *nick = NULL;
	if ((ptr = (char *) strchr(name, delim)) != NULL)
	{
		*(ptr++) = (char) 0;
		if (!strlen(ptr))
			*port = NULL;
		else
		{
			*port = ptr;
			if ((ptr = (char *) strchr(ptr, delim)) != NULL)
			{
				*(ptr++) = (char) 0;
				if (strlen(ptr) == 0)
					*nick = 0;
				else
				{
					*nick = ptr;
					if ((ptr = (char *) strchr(ptr, delim))
							!= NULL)
					{
						*(ptr++) = 0;
						if (!strlen(ptr))
							*password = NULL;
						else
						{
							*password = ptr;
							if ((ptr = strchr(ptr, delim)) !=NULL)
							{
								*(ptr++) = 0;
								if  (!strlen(ptr))
									*meta = NULL;
								else
									*meta = ptr;
							}
						}
					}
				}
			}
		}
	}
}

int 	find_server_refnum (char *server, char **rest)
{
	int 	refnum;
	int	port = get_int_var(DEFAULT_METASERVER_VAR);
	int	my_meta = 0;

	char 	*cport = NULL, 
		*password = NULL,
		*nick = NULL,
		*meta = NULL;

	/*
	 * First of all, check for an existing server refnum
	 */
	if ((refnum = parse_server_index(server)) != -1)
	{
		if (!server_list[refnum].d_nickname)
			malloc_strcpy(&server_list[refnum].d_nickname, get_string_var(DEFAULT_NICKNAME_VAR));
		if (!server_list[refnum].password)
			malloc_strcpy(&server_list[refnum].password, get_string_var(DEFAULT_PASSWORD_VAR));
		return refnum;
	}
	/*
	 * Next check to see if its a "server:port:nick:password:meta"
	 */
	else if (index(server, ':') || index(server, ','))
		parse_server_info(server, &cport, &password, &nick, &meta);

	/*
	 * Next check to see if its "server port nick passwd meta"
	 */
	else if (rest && *rest)
	{
		cport = next_arg(*rest, rest);
		nick = next_arg(*rest, rest);
		password = next_arg(*rest, rest);
		meta = next_arg(*rest, rest);
	}

	if (cport && *cport)
		port = my_atol(cport);

	if (meta && isdigit(*meta))
		my_meta = my_atol(meta);
	else if (meta && my_stricmp(meta, "on"))
		my_meta = 1;

	/*
	 * Add to the server list (this will update the port
	 * and password fields).
	 */
	return add_to_server_list(server, port, password, nick, my_meta, 1);
}




/*
 * build_server_list: given a whitespace separated list of server names this
 * builds a list of those servers using add_to_server_list().  Since
 * add_to_server_list() is used to added each server specification, this can
 * be called many many times to add more servers to the server list.  Each
 * element in the server list case have one of the following forms: 
 *
 * servername 
 * servername:port 
 * servername:port:password 
 * servername::password 
 * servername:port:password:nick:servernetwork
 * Note also that this routine mucks around with the server string passed to it,
 * so make sure this is ok 
 */

int	build_server_list (char *servers)
{
	char	*host,
		*rest,
		*pass = NULL,
		*port = NULL,
		*nick = NULL,
		*meta = NULL;

	int	port_num;
	int	my_meta = 0;
	int	i = 0;
	
	if (!servers || !*servers)
		return 0;

	while (servers)
	{
		if ((rest = (char *) strchr(servers, '\n')) != NULL)
			*rest++ = 0;
		while ((host = new_next_arg(servers, &servers)) != NULL)
		{
			if (!host || !*host)
				break;
			parse_server_info(host, &port, &pass, &nick, &meta);
			if (!nick || !*nick)
				nick = nickname;
			if (!pass || !*pass)
				pass = password;
                        if (port && *port)
                        {
				if (!(port_num = my_atol(port)))
                                        port_num = nap_port;
                        }
			else
				port_num = get_int_var(DEFAULT_METASERVER_VAR);
			if (meta && isdigit(*meta))
				my_meta = my_atol(meta);
			else if (meta && my_stricmp(meta, "on"))
				my_meta = 1;

			add_to_server_list(host, port_num, pass, nick, my_meta, 1);
			i++;
		}
		servers = rest;
	}
	return i;
}

void display_server_list(void)
{
int i;
	put_it("%4s %35s %17s %10s %s", "Ref", "Server:Port", "Nick", "Password", "Meta");
	for (i = 0; i < number_of_servers; i++)
	{
		put_it("%4d %30s:%5d %16s %10s %3s %s", i, server_list[i].name,
			server_list[i].port, 
			server_list[i].d_nickname ? server_list[i].d_nickname : "(none)", 
			server_list[i].password ? server_list[i].password : "(none)", 
			off_on(server_list[i].meta), is_connected(i) ? "<--":"");
	}
}

N_STATS *get_server_stats(int server)
{
	if (server < 0 || server >= number_of_servers)
		return NULL;
	return &server_list[server].statistics;
}

void send_all_servers(int cmd, char *format, ...)
{
	va_list args;
	int i, ofs;
	char buff[BIG_BUFFER_SIZE+1];
	va_start(args, format);
	vsnprintf(buff, BIG_BUFFER_SIZE, format, args);
	va_end(args);
	ofs = from_server;
	for (i = 0; i < number_of_servers; i++)
	{
		if (is_connected(i))
		{
			from_server = i;
			send_ncommand(cmd, "%s", buff);
		}
	}
	from_server = ofs;
}

void send_to_server(const char *format, ...)
{
	va_list args;
	int len;
	char buff[BIG_BUFFER_SIZE+1];
	if (from_server < 0)
		return;
	if (!server_list[from_server].irc_mode || 
			server_list[from_server].write == -1 || !format)
		return;
	va_start(args, format);
	vsnprintf(buff, BIG_BUFFER_SIZE, format, args);
	va_end(args);
	len = strlen(buff);
	if (len > (IRCD_BUFFER_SIZE - 2) || len == -1)
		buff[IRCD_BUFFER_SIZE - 2] = (char) 0;
	strmcat(buff, "\r\n", IRCD_BUFFER_SIZE);
	write(server_list[from_server].write, buff, strlen(buff));
}

static void removetabkey(int server)
{
Msgs *new, *last;
	if (server < 0 || server >= number_of_servers)
		return;
	for (new = server_list[server].msgs; new; new = last)
	{
		last = new->next;
		new_free(&new->nick);
		new_free(&new->msg);
		new_free(&new);
	}
	server_list[server].msgs = NULL;	
}

void addtabkey(char *nick, char *cmd, char *msg)
{
Msgs *new, *last = NULL;
int count, max;
	if (from_server == -1)
		return;
	if ((max = get_int_var(MAX_RELM_VAR)))
	{
		for (count = 1, new = server_list[from_server].msgs; new; new = new->next, count++)
			if (count >= max)
				break;
			else
				last = new;
	}
	else
	{
		last = server_list[from_server].msgs;
		new = last;
	}
	if (last && new)
	{
		last->next = NULL;
		for (; new; new = last)
		{
			last = new->next;
			new_free(&new->nick);
			new_free(&new->msg);
			new_free(&new);
		}
		if (!max)
			return;
	}
	new = new_malloc(sizeof(Msgs));
	new->nick = m_strdup(nick);
	new->msg = m_strdup(msg);
	new->time = now;
	
	new->next = server_list[from_server].msgs;
	server_list[from_server].msgs = new;
}

Msgs *gettabkey(char *possible, int move)
{
Msgs *new = NULL, *last;
	if (from_server == -1)
		return NULL;
	for (new = server_list[from_server].msgs; new; new = new->next)
	{
		if (!possible || !my_strnicmp(possible, new->nick, strlen(possible)))
#if 0
			return new;
#else
			break;
#endif

	}
	if (!move)
		return new;
	if (new)
	{
		new = (Msgs *)remove_from_list((List **)&server_list[from_server].msgs, new->nick);
		last = server_list[from_server].msgs;
		while (last)
		{
			if (!last->next)
				break;
			last = last->next;
		}
		if (last)
		{
			new->next = NULL;
			last->next = new;
		}
		else
		{
			new->next = server_list[from_server].msgs;
			server_list[from_server].msgs = new;
		}
		return new;
	}
	return NULL;
}

void display_servermsgs(void)
{
int i = 1;
Msgs *s;
	if (from_server == -1)
		return;
	for (s = server_list[from_server].msgs; s; s = s->next, i++)
		put_it("%2d %s -> %s", i, s->nick, s->msg);
}

void clear_servermsg(void)
{
Msgs *new, *last;
	if (from_server == -1)
		return;
	new = server_list[from_server].msgs;
	while (new)
	{
		last = new->next;
		new_free(&new->nick);
		new_free(&new->msg);
		new_free(&new);
		new = last;
	}
	server_list[from_server].msgs = NULL;
}

GetFile *add_in_queue(char *nick, GetFile *new)
{
NickStruct *n;
	if (from_server == -1 || !(n = (NickStruct *) find_in_list((List **)&server_list[from_server].users, nick, 0)))
		return NULL;
	new->next = n->Queued;
	if (n->Queued)
		n->Queued->prev = new;
	n->Queued = new;
	return new;
}

GetFile *find_in_queue(int remove, char *nick, char *checksum, char *filename, unsigned long filesize)
{
NickStruct *n;
GetFile *tmp, *last = NULL;
	if (from_server == -1 || !(n = (NickStruct *) find_in_list((List **)&server_list[from_server].users, nick, 0)))
		return NULL;
	for (tmp = n->Queued; tmp; tmp = last)
	{
		last = tmp->next;
		if (filename && my_stricmp(filename, tmp->filename))
			continue;
		if (checksum && my_stricmp(checksum, tmp->checksum))
			continue;
		if (filesize && filesize != tmp->filesize)
			continue;
		if (remove)
			break_from_list((List **)&n->Queued, (List *)tmp);
		return tmp;
	}
	return NULL;
}

int files_in_sendqueue(char *nick, char *filename)
{
int ret = 0;
GetFile *tmp;
NickStruct *n;
	if (!nick || from_server == -1 || !(n = (NickStruct *) find_in_list((List **)&server_list[from_server].users, nick, 0)))
		return 0;
	for (tmp = n->Queued; tmp; tmp = tmp->next)
	{
		if (!my_stricmp(nick, tmp->nick))
		{
			if (filename && strcmp(filename, tmp->filename))
				continue;
			ret++;
		}
	}
	return ret;
}

int get_server_resume(int serv)
{
	if (serv < -1 || serv >= number_of_servers)
		return 0;
	return server_list[serv].resume_results;
}

void set_server_resume(int serv, int val)
{
	if (serv < -1 || serv >= number_of_servers)
		return;
	server_list[serv].resume_results = val;
}


void	clear_sent_to_server (int servnum)
{
	server_list[servnum].sent = 0;
}

int	sent_to_server (int servnum)
{
	return server_list[servnum].sent;
}

int server_waiting_in(int server)
{
	if (server < 0 || server > number_of_servers)
		return 0;
	return server_list[server].waiting_in;
}

int server_waiting_out(int server)
{
	if (server < 0 || server > number_of_servers)
		return 0;
	return server_list[server].waiting_out;
}

void	inc_server_waiting_out(int server)
{
	if (server < 0 || server > number_of_servers)
		return;
	server_list[server].waiting_out++;
}

void	inc_server_waiting_in(int server)
{
	if (server < 0 || server > number_of_servers)
		return;
	server_list[server].waiting_in++;
}

int	get_server_cloak(int server)
{
	if (server < 0 || server > number_of_servers)
		return 0;
	return server_list[server].cloak;
}

void	set_server_cloak(int server, int on_off)
{
	if (server < 0 || server > number_of_servers)
		return ;
	server_list[server].cloak = on_off;
}

void set_server_lag(int server, struct timeval tv)
{
	if (server < 0 || server > number_of_servers)
		return;
	server_list[server].lagtime.tv_sec = tv.tv_sec;
	server_list[server].lagtime.tv_usec = tv.tv_usec;
}
struct timeval get_server_lag(int server)
{
struct timeval td = {0};
	if (server < 0 || server > number_of_servers)
		return td;
	return server_list[server].lagtime;
}

void set_server_sping(int server, int value)
{
	if (server < 0 || server > number_of_servers)
		return;
	server_list[server].sping = value;
}

int get_server_sping(int server)
{
	if (server < 0 || server > number_of_servers)
		return 0;
	return server_list[server].sping;
}

void set_server_version(int server, int value)
{
	if (server < 0 || server > number_of_servers)
		return;
	server_list[server].version = value;
}

int get_server_version(int server)
{
	if (server < 0 || server > number_of_servers)
		return 0;
	return server_list[server].version;
}

void set_server_admin(int server, int value)
{
	if (server < 0 || server > number_of_servers)
		return;
	server_list[server].level = value;
}

int get_server_admin(int server)
{
	if (server < 0 || server > number_of_servers)
		return 0;
	return server_list[server].level;
}

char * get_server_showuser(int server, unsigned int **count)
{
	if (server < 0 || server > number_of_servers)
		return NULL;
	*count = &server_list[server].user_count;
	return server_list[server].showuser;
}

void set_server_showuser(int server, char *match, int count)
{
	if (server < 0 || server > number_of_servers)
		return;
	if (count != -1)
		server_list[server].user_count = count;
	if (!match)
		new_free(&server_list[server].showuser);
	else
		malloc_strcpy(&server_list[server].showuser, match);
}

char * get_server_showusercmd(int server)
{
	if (server < 0 || server > number_of_servers)
		return NULL;
	return server_list[server].showusercmd;
}

void set_server_showusercmd(int server, char *match)
{
	if (server < 0 || server > number_of_servers)
		return;
	if (!match)
		new_free(&server_list[server].showusercmd);
	else
		malloc_strcpy(&server_list[server].showusercmd, match);
}

FileStruct **get_search_head(int server)
{
	if (server < 0 || server > number_of_servers)
		return NULL;
	return &server_list[server].search_results;
}

int get_icmp_socket(int server)
{
	if (server < 0 || server > number_of_servers)
		return -1;
	return server_list[server].icmp_sock;
}

void icmp_sockets(fd_set *rd, fd_set *wr)
{
int i;
	for (i = 0; i < number_of_servers; i++)
	{
		if (server_list[i].icmp_sock > -1)
			FD_SET(server_list[i].icmp_sock, rd);
	}
}

void set_icmp_socket(int server, int val)
{
	if (server < 0 || server > number_of_servers)
		return;
	server_list[server].icmp_sock = val;
}

void add_ping(int serv, struct sockaddr_in *from)
{
FileStruct *n;
	for (n = server_list[serv].search_results; n; n = n->next)
	{
		if (n->ip != from->sin_addr.s_addr)
			continue;
		n->icmp = 1;
		get_time(&n->result);
	}	
}

void check_icmpresult(fd_set *rd, fd_set *wr)
{
int i;
char buf[80];
struct sockaddr_in from;
int fromlen = sizeof(from);

	for (i = 0; i < number_of_servers; i++)
	{
		if (server_list[i].icmp_sock > -1)
		{
			if (FD_ISSET(server_list[i].icmp_sock, rd))
			{
				recvfrom(server_list[i].icmp_sock, buf, sizeof(buf)-1, 0, (struct sockaddr *)&from, &fromlen);
				add_ping(i, &from);
			}		
		}
	}
}

int connect_to_irc_server(int serv)
{
int des = -1;
unsigned short port = 6667;
	if (serv < 0 || serv > number_of_servers)
		return -1;
	des = connect_by_number(server_list[serv].name, &port, SERVICE_CLIENT, PROTOCOL_TCP, 0);
	if (des > -1)
	{
		server_list[serv].read = server_list[serv].write = des;
		server_list[serv].irc_mode = 1;
		server_list[serv].flags |= SERVER_CONNECTED;
		new_open(des);
	}
	return des;
}

void login_to_ircserver(int serv)
{
int ofs = from_server;
	from_server = serv;
	send_to_server("NICK %s", get_server_nickname(serv));
	send_to_server("USER %s NULL NULL :%s", get_server_nickname(serv), get_server_nickname(serv));
	from_server = ofs;
}

int get_server_ircmode(int server)
{
	if (server < 0 || server > number_of_servers)
		return 0;
	return server_list[server].irc_mode;
}

void set_server_listmode(int server, char *args)
{
	if (server < 0 || server > number_of_servers)
		return;
	if (args)
		malloc_strcpy(&server_list[server].listmode, args);
	else
		new_free(&server_list[server].listmode);
}

char *get_server_listmode(int server)
{
	if (server < 0 || server > number_of_servers)
		return NULL;
	return server_list[server].listmode;
}

void save_servers(void)
{
FILE *out;
int i;
char *expand = NULL;
char buffer[BIG_BUFFER_SIZE+1];
char *home = getenv("HOME");

#ifdef WINNT
	sprintf(buffer, "NapServers");
#else
	sprintf(buffer, "%s/.napservers", home ? home : "~");
#endif
	if (!(expand = expand_twiddle(buffer)))
		return;
	if (!(out = fopen(expand, "w")))
	{
		new_free(&expand);
		return;
	}
	for (i = 0; i < number_of_servers; i++)
	{
#ifdef WINNT
		fprintf(out, "%s:%d:%s:%s:%d\r\n", 
#else
		fprintf(out, "%s:%d:%s:%s:%d\n", 
#endif
			server_list[i].name, server_list[i].port,
			server_list[i].d_nickname, server_list[i].password,
			server_list[i].meta);
	}
	fclose(out);
	new_free(&expand);
}

int read_server_list(void)
{
char *expand = NULL, *p;
char buffer[BIG_BUFFER_SIZE+1];
char *home = getenv("HOME");
FILE *out;

#ifdef WINNT
	sprintf(buffer, "NapServers");
#else
	sprintf(buffer, "%s/.napservers", home ? home : "~");
#endif
	if (!(expand = expand_twiddle(buffer)))
		return number_of_servers;
	if (!(out = uzfopen(&expand, ".", 0, NULL)))
	{
		new_free(&expand);
		return number_of_servers;
	}
	if ((p = fgets(buffer, sizeof(buffer)-1, out)))
	{
		while (!feof(out))
		{
			chomp(buffer);
			if (*buffer)
				build_server_list(buffer);
			fgets(buffer, sizeof(buffer)-1, out);
		}
	}
	fclose(out);	
	return number_of_servers;
}

/*
 * We really only need ip (or better hostname) and port for this napigator structure
 * 'nulltoken' picks up headers we don't need
 */

int parse_napigator(char *buffer)
{
char *p;
char *ip, *nulltoken;
int port;
#if 0
This was www.napigator.com/servers.php format
<SERVER IP> <SERVER PORT> <NETWORK> <USERS> <FILES> <GIGABYTES>
And then the last line, is 4 tokens and is a total of all the stats
<NUMBER OF SERVERS> <TOTAL USERS> <TOTAL FILES> <TOTAL GIGABYTES>
New client[1|2].napigator.com/servers.php format
<?> <SERVER IP> <SERVER PORT> <NETWORK> <?> <USERS> <FILES> <GIGABYTES> <SERVER HOST> <?> <?> <?> <EMAIL> <WWW>
We pick tokens 3(port) and 9(hostname)
And then the last line, is 4 tokens and is a total of all the stats
#endif
	if ((p = strchr(buffer, '\r')))
		*p = 0;
	else
		chomp(buffer);

	if (!buffer || !*buffer)
		return 0;
	if (!my_strnicmp(buffer, "HTTP/1.1 200 OK", 6) || !my_strnicmp(buffer, "Date:", 5) || !my_strnicmp(buffer, "Server:", 7))
		return 0;
	if (!my_strnicmp(buffer, "X-Powered-By:", 13) || !my_strnicmp(buffer, "Expires:", 8))
		return 0;
	if (!my_strnicmp(buffer, "Last-Modified:", 13) || !my_strnicmp(buffer, "Cache-control", 9))
		return 0;
	if (!my_strnicmp(buffer, "Pragma:", 7) || !my_strnicmp(buffer, "Content-Type:", 12))
		return 0;
	if (!my_strnicmp(buffer, "Connection:", 11))
		return 0;
	if (!my_strnicmp(buffer, "X-Cache:", 8))
		return 0;
	nulltoken = next_arg(buffer, &buffer);
	/* This next variable is actually the ip if you want it */
	nulltoken = next_arg(buffer, &buffer);
	port = my_atol(next_arg(buffer, &buffer));
	nulltoken = next_arg(buffer, &buffer);
	/* We check here for end of list */
	if (!(nulltoken = next_arg(buffer, &buffer)))
                return -2;
	nulltoken = next_arg(buffer, &buffer);
	nulltoken = next_arg(buffer, &buffer);
	nulltoken = next_arg(buffer, &buffer);
	ip = next_arg(buffer, &buffer);
	return add_to_server_list(ip, port, get_string_var(DEFAULT_PASSWORD_VAR), get_string_var(DEFAULT_NICKNAME_VAR), 0, 0);
}

int get_napigator(void)
{
unsigned short port = 80;
int s, junk, count = 0, len, rc;
fd_set rd, wd;
static struct timeval nap_timer = {0};

char buffer[BIG_BUFFER_SIZE+1], *bufptr;
/*
Old www.napigator.com/servers.php structure
GET /servers.php HTTP/1.0
client[1|2].napigator.com/servers.php structure
GET /servers.php HTTP/1.0
Host: client1.napigator.com
*/
	s = connect_by_number("client1.napigator.com", &port, SERVICE_CLIENT, PROTOCOL_TCP, 0);
	if (s == -1)
		return -1;
	strcpy(buffer, "GET /servers.php HTTP/1.0\r\nHost: client1.napigator.com\r\n\r\n\r\n");
	len = strlen(buffer);
	if (write(s, buffer, len) != len)
	{
		close(s);
		return -1;
	}
	sleep(1);
	
	bufptr = &buffer[0];
	new_open(s);
	
select_again:
	FD_ZERO(&wd);
	FD_ZERO(&rd);
	FD_SET(s, &rd);	
	nap_timer.tv_sec = 30;
	nap_timer.tv_usec = 0;

	switch ((rc = new_select(&rd, &wd, &nap_timer)))
	{
		case 0:
			goto select_again;
		case -1:
			break;
		default:
		{
			junk = new_dgets(bufptr, s, 1, BIG_BUFFER_SIZE);
			switch(junk)
			{
				case -1:
				{
					new_close(s);
					return count;
				}
				default:
				{
					junk = parse_napigator(bufptr);
					if (junk > 0)
						count++;
					else if (junk == 0)
						goto select_again;
					else
					{
						new_close(s);
						return count;
					}
				}
				case 0:
					goto select_again;
			}
		}
	}
	new_close(s);
	return -1;
}

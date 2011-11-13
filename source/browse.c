/* $Id: */
 
#include "teknap.h"
#include "struct.h"
#include "hook.h"
#include "list.h"
#include "output.h"
#include "newio.h"
#include "server.h"
#include "timer.h"
#include "napster.h"
#include "vars.h"
#include <string.h>
#if 0
<drscholl> mynick\n
<drscholl> "file1" ....\n
<drscholl> "file2" ....\n
<drscholl> \n
#endif

extern Server *server_list;

typedef struct _file_browse_ {
	char *nick;
	unsigned long ip;
	unsigned short port;
	FileStruct *location;	
	int socket;
	char *buffer;
} FileBrowse;

void browse_timeout_error(int snum)
{
SocketList *s1;
NickStruct *n;
char buffer[BIG_BUFFER_SIZE];
char *ptr;
	s1 = get_socket(snum);
	n = s1->info;
	ptr = buffer;
	if (n)
	{
		sprintf(buffer, "%s %s", n->nick, s1->server);
		cmd_endbrowse(0, ptr);
	}
	close_socketread(snum);
}

void nap_browse_read(int snum)
{
char buffer[BIG_BUFFER_SIZE+1], *ptr;
int bytes_read;
SocketList *s1;
NickStruct *n;

	s1 = get_socket(snum);
	n = s1->info;
	if (!n)
	{
		close_socketread(snum);
		return;
	}
	bytes_read = new_dgets(buffer, snum, 1, BIG_BUFFER_SIZE);
	ptr = buffer;
	switch(bytes_read)
	{
		case -1:
			sprintf(buffer, "%s %s", n->nick, s1->server);
			cmd_endbrowse(0, ptr);
			close_socketread(snum);
			break;
		case 0:
			break;
		default:
		{
			chomp(ptr);
			if (!*ptr)
			{
				sprintf(buffer, "%s %s", n->nick, s1->server);
				cmd_endbrowse(0, ptr);
				close_socketread(snum);
				return;
			}
			if (strchr(ptr, ' '))
				add_to_browse_list(n, n->nick, ptr);
			else
				add_sockettimeout(snum, 180, browse_timeout_error);
		}
	}
}

void naplink_handlebrowse(int snum)
{
unsigned char buff[80];
SocketList *s;
int rc;
	memset(buff, 0, sizeof(buff) - 1);
	if (!(s = get_socket(snum)) || !s->info)
	{
		close_socketread(snum);
		return;
	}
	switch ((rc = recv(snum, buff, 5, MSG_PEEK)))
	{

		case -1:
			say("naplink_handlebrowse [snum = %d] %s", snum, strerror(errno));
			close_socketread(snum);
			return;
		case 0:
			s->flags++;
			if (s->flags > 2)
				close_socketread(snum);
			else if (s->flags == 1)
				write(snum, "GETLIST\n", 8);
			return;
		default:
			break;
	}

	buff[rc] = 0;
	if (rc == 1 && (buff[0] == '1' || buff[0] == '\n'))
	{
		read(snum, buff, 1);
		s->func_read = nap_browse_read;
		write(snum, "GETLIST\n", 8);
	}
	else
		close_socketread(snum);
}

void browse_timeout(int snum)
{
SocketList *s1;
NickStruct *n;
	s1 = get_socket(snum);
	n = s1->info;
	if (n)
	{
		if (!s1->flags)
		{
			say("Browse failed for %s. Using fallback", n->nick);
			send_ncommand(CMDS_BROWSE, "%s", n->nick);
		}
	}
	close_socketread(snum);
}

int setup_browse_direct(char *nick, unsigned long ip, unsigned short port, int flag)
{
int s;
struct sockaddr_in socka;
		
	if ((s = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		return -1;
	socka.sin_addr.s_addr = BSWAP32(ip);
	socka.sin_family = AF_INET;
	socka.sin_port = htons(port);
	set_keepalive(s);
	alarm(get_int_var(CONNECT_TIMEOUT_VAR));
	if (connect(s, (struct sockaddr *)&socka, sizeof(struct sockaddr)) != 0) 
	{
		alarm(0);
		close(s);
		return -1;
	}
	alarm(0);
	return s;
}


int write_buffer(int s, FileBrowse *fb, int len)
{
int rc;
	rc = write(s, fb->buffer, len);
	switch (rc)
	{
		case -1:
			new_free(&fb->buffer);
			new_free(&fb->nick);
			new_free(&fb);
			close_socketread(s);
			break;
		case 0:
			break;
		default:
			if (rc == len)
				new_free(&fb->buffer);
			else
				ov_strcpy(fb->buffer, fb->buffer + rc);
	}
	return rc;
}

void direct_browse_handler(int s)
{
SocketList *s1;
FileBrowse *fb;
int len = 0;
char buffer[3 * BIG_BUFFER_SIZE+1];
char *name;

	s1 = get_socket(s);
	if (!s1 || !(fb = (FileBrowse *)s1->info))
	{
		close_socketread(s);
		return;
	}

	if (fb->buffer)
	{
		len = strlen(fb->buffer);
		write_buffer(s, fb, len);
		return;
	}

	*buffer = 0;
	len = 0;

	while (fb->location)
	{
		name = LOCAL_COPY(fb->location->filename);
		convertnap_dos(name);
		sprintf(buffer+len, "\"%s\" %s %lu %u %u %lu\n", name, 
			fb->location->checksum, fb->location->filesize, 
			fb->location->bitrate, fb->location->freq, 
			fb->location->seconds);
		len = strlen(buffer);
		fb->location = fb->location->next;
		if (len > (2 * BIG_BUFFER_SIZE))
		{
			malloc_strcat(&fb->buffer, buffer);
			write_buffer(s, fb, len);
			return;
		}
	}
	if (*buffer)
	{
		malloc_strcat(&fb->buffer, buffer);
		len = strlen(fb->buffer);
		write_buffer(s, fb, len);
	}
	else
	{
		write(s, "\n", 1);
		new_free(&fb->buffer);
		new_free(&fb->nick);
		new_free(&fb);
		close_socketread(s);
	}
}

NAP_COMM(cmd_direct_browse) /* 640 */
{
char *nick;
unsigned long ip;
unsigned short port;
char buffer[80];
	if (get_int_var(DEFAULT_DIRECT_BROWSE_VAR))
	{
		int s = 0;
		nick = next_arg(args, &args);
		/* ip port   if other side is firewalled */
		if (args && *args)
		{
			ip = my_atoul(next_arg(args, &args));
			port = my_atoul(next_arg(args, &args));
			if ((s = setup_browse_direct(nick, ip, port, 1)) != -1)
			{
				FileBrowse *new;
				struct sockaddr_in socka;
				SocketList *s1;				

				send_ncommand(CMDR_BROWSE_DIRECT, "%s", nick);
				if (write(s, "SENDLIST\n", 9) <= 0)
				{
					close(s);
					return 0;
				}
				new = (FileBrowse *) new_malloc(sizeof(FileStruct));
				new->nick = m_strdup(nick);
				new->ip = ip;
				new->port = port;
				new->location = fserv_files;
				new->socket = s;
				set_non_blocking(s);
				socka.sin_addr.s_addr = ip;
				socka.sin_addr.s_addr = BSWAP32(socka.sin_addr.s_addr);

				add_socketread(s, s, 0, inet_ntoa(socka.sin_addr), direct_browse_handler, 0);
				set_socketinfo(s, new);
				s1 = get_socket(s);
				s1->func_write = s1->func_read;
				s1->is_write = s1->is_read;
				sprintf(buffer, "%s\n", get_server_nickname(from_server));
				write(s, buffer, strlen(buffer));
				direct_browse_handler(s);
				say("%s has requested a browse from %s/%d", nick, inet_ntoa(socka.sin_addr), port);
#if 0
				send_direct_browse(s);
				close(s);		
#endif
			}
		}
		else
			send_ncommand(CMDR_BROWSE_DIRECT, "%s", nick);
	}
	return 0;
}


NAP_COMM(cmd_newbrowse)
{
char *nick;
unsigned long ip;
unsigned short port;
NickStruct *n;
struct sockaddr_in socka = {0};

	nick = next_arg(args, &args);
	ip = my_atoul(next_arg(args, &args));
	port = my_atol(next_arg(args, &args));

#if 0
	if (!fserv_files || !get_int_var(SHARE_VAR))
	{
		if (ip)
			socka.sin_addr.s_addr = BSWAP32(ip);
		socka.sin_port = htons(port);
		if (ip)
			say("WARNING %s from %s:%d is attempting to hack you!!!!", nick, inet_ntoa(socka.sin_addr), socka.sin_port);
		return 0;
	}
#endif
	if (nick && ip && port)
	{
		int s;
		socka.sin_addr.s_addr = BSWAP32(ip);
		socka.sin_port = htons(port);
		if ((s = setup_browse_direct(nick, ip, port, 0)) == -1)
		{
			say("Unable to connect to %s [%s:%d] for browse", nick, inet_ntoa(socka.sin_addr), socka.sin_port);
			send_ncommand(CMDR_DATAPORTERROR, "%s", nick);
/*			send_ncommand(CMDS_BROWSE, "%s", nick);*/
			return 0;
		}
		say("Connected to %s [%s:%d] for browse", nick, inet_ntoa(socka.sin_addr), socka.sin_port);
		add_socketread(s, port, 0, inet_ntoa(socka.sin_addr), naplink_handlebrowse, NULL);
		n = (NickStruct *)find_in_list((List **)&server_list[from_server].users, nick, 0);
		set_socketinfo(s, n);
		add_sockettimeout(s, 20, browse_timeout);
	} else
		send_ncommand(CMDR_BROWSE_DIRECT, "%s", nick);
	return 0;
}

NAP_COMM(cmd_newbrowse_error)
{
char *nick;
	nick = next_arg(args, &args);
	say ("New Browse error %s %s", nick, args);
	send_ncommand(CMDS_BROWSE, "%s", nick);
	return 0;
}

int direct_browse_timeout(void *args, char *sub)
{
char *str = (char *)args;
NickStruct *n;
int server;
char *nick;
	server = my_atol(next_arg(str, &str));	
	nick = next_arg(str, &str);
	if (nick)
	{
		n = (NickStruct *) find_in_list((List **)&server_list[server].users, nick, 0);
		if (n && !n->file_browse)
		{
			say("Direct Browse failed.. Attempting regular browse");
			send_ncommand(CMDS_BROWSE, "%s", n->nick);
		}
	}
	new_free(&args);
	return 0;
}

BUILT_IN_COMMAND(browse)
{
extern Server *server_list;
NickStruct *n;
int newbrowse = get_int_var(DEFAULT_DIRECT_BROWSE_VAR);

	if (from_server == -1)
		return;
	n = server_list[from_server].users;
	if (args && *args)
	{
		char *n1;
		while ((n1 = next_arg(args, &args)))
		{
			if (!n1 || !*n1)
				break;
			if (!my_stricmp(n1, "-clear"))
			{
				while ((n1 = next_arg(args, &args)))
				{
					if ((n = (NickStruct *)remove_from_list((List **)&server_list[from_server].users, n1)))
					{
						clear_filelist(&n->file_browse);
						n->flag = 0;
						new_free(&n);
					}
				}
				return;
			}
			else if (!my_stricmp(n1, "-direct"))
			{
				newbrowse ^= 1;
				continue;
			}
			if (!n || !(n = (NickStruct *)find_in_list((List **)&server_list[from_server].users, n1, 0)))
			{
				if (!n)
				{
					n = (NickStruct *)new_malloc(sizeof(NickStruct));
					n->nick = m_strdup(n1);
#ifdef GTK
					if (tgtk_okay() && get_int_var(GTK_VAR) && scott2_okay())
						scott2_add_nick(from_server, n->nick);
#endif	
					add_to_list_double((List **)&server_list[from_server].users, (List *)n);
				}
				do_hook(BROWSE_BEGIN_LIST, "%s", n1);

				if (newbrowse)
				{
					send_ncommand(CMDS_BROWSE_DIRECT_REQ, "%s", n1);
					add_timer(0, empty_string, 20 * 1000, 1, 
					direct_browse_timeout, 
					m_sprintf("%d %s", from_server, n1), 
					NULL, -1, "direct_browse");
				}
				else
					send_ncommand(numeric, "%s", n1);
				clear_filelist(&n->file_browse);
				n->flag |= BROWSE_IN_PROGRESS;
			} 
			else if (n->flag & BROWSE_IN_PROGRESS)
				say("Can't do that while a browse is in progress");
			else
			{
				display_list(n->file_browse);
				return;
			}
		}
	}
	else if (n)
		display_list(n->file_browse);
}

void send_direct_browse(int snum)
{
FileBrowse *new;
char buffer[BIG_BUFFER_SIZE+1];
SocketList *s1;

	if (!(s1 = get_socket(snum)))
	{
		close_socketread(snum);
		return;
	}
	if (write(snum, "SENDLIST\n", 9) < 9)
	{
		close_socketread(snum);
		return;
	}
	new = (FileBrowse *) new_malloc(sizeof(FileStruct));
	new->location = fserv_files;
	new->socket = snum;
	set_non_blocking(snum);
	
	set_socketinfo(snum, new);
	s1->func_write = s1->func_read = direct_browse_handler;
	s1->is_write = s1->is_read;
	sprintf(buffer, "%s\n", get_server_nickname(from_server));
	write(snum, buffer, strlen(buffer));
	direct_browse_handler(snum);
}

/* $Id: send.c,v 1.1.1.1 2001/01/09 22:46:47 edwards Exp $ */

#include "teknap.h"
#include "struct.h"
#include "commands.h"
#include "hook.h"
#include "list.h"
#include "output.h"
#include "server.h"
#include "vars.h"
#include "status.h"
#include "window.h"
#include "newio.h"
#include "napster.h"

#include <sys/stat.h>
#include <sys/ioctl.h>

#ifndef WINNT
#undef O_BINARY
#define O_BINARY 0
#endif


void set_napster_socket(int number)
{
int on = 32000;
	setsockopt(number, SOL_SOCKET, SO_RCVBUF, (char *)&on, sizeof(on));
	on = 60000;
	setsockopt(number, SOL_SOCKET, SO_SNDBUF, (char *)&on, sizeof(on));
}

NAP_COMM(cmd_accepterror)
{
char *nick, *filename;
	nick = next_arg(args, &args);
	filename = new_next_arg(args, &args);
	if (nick && filename)
	{
		GetFile *gf;
		if (!(gf = find_in_getfile(1, nick, NULL, filename, 0, NAP_UPLOAD)))
			return 0;
		if (do_hook(NAPERROR_LIST, "%s Accept error", nick))
			say("Removing %s from the send queue. Accept error", nick);
		if (gf->write != -1)
		{
			close(gf->write);
			gf->write = -1;
		}
		nap_finished_file(gf->socket, PREMATURE_FINISH);
	}
	return 0;
}

GetFile *create_send(char *nick, FileStruct *fs)
{
GetFile *gf;
	if (!nick)
		return NULL;
	gf = new_malloc(sizeof(GetFile));
	gf->nick = m_strdup(nick);
	gf->checksum = m_strdup(fs->checksum);
	gf->filename = m_strdup(fs->filename);
	gf->write = -1;
	gf->socket = -1;
	gf->deleted = 0;
	gf->filesize = fs->filesize;
	gf->flags = NAP_UPLOAD;
	gf->addtime = now;
	return gf;
}

void send_limit(char *nick, char *filename, int limit)
{
char buffer[BIG_BUFFER_SIZE+1];
	snprintf(buffer, BIG_BUFFER_SIZE, "%s \"%s\" %d", nick, convertnap_dos(filename),	limit);
	send_ncommand(CMDS_SENDLIMIT, "%s", buffer);
	return;
}

NAP_COMM(cmd_filerequest)
{
char *nick;
FileStruct *newf = NULL;
char *filename;
int count = 0;

	nick = next_arg(args, &args);
	filename = new_next_arg(args, &args);
	if (!nick || !filename || !*filename || check_nignore(nick))
		return 0;
	convertnap_unix(filename);
	for (newf = fserv_files; newf; newf = newf->next)
	{
		if (!strcmp(filename, newf->filename))
			break;
	}
	if (newf)
	{
		char buffer[2*BIG_BUFFER_SIZE+1];
		GetFile *gf;
		int dl_limit, dl_count;
		if ((gf = find_in_getfile(0, nick, NULL, filename, 0, NAP_UPLOAD)))  
		{
			if (do_hook(NAPERROR_LIST, "%s \"%s\" Already Queued", nick, gf->filename) && !get_int_var(QUIET_SENDS_VAR))
				put_it("* Already sending %s to %s.", base_name(gf->filename), gf->nick);
			return 0;
		}
		else
		{
			dl_limit = get_int_var(MAX_SENDS_NICK_VAR);
			dl_count = files_in_progress(nick, NAP_UPLOAD);
			count = files_in_progress(NULL, NAP_UPLOAD);
			if (!get_int_var(SHARE_VAR))
			{
				send_limit(nick, filename, 0);
				return 0;
			}
			if (get_int_var(SEND_LIMIT_VAR) && count >= get_int_var(SEND_LIMIT_VAR)) 
			{
				send_limit(nick, filename, 0);
				return 0;
			}
			if (dl_limit && (dl_count >= dl_limit))
			{
				send_limit(nick, filename, dl_limit);
				return 0;
			}
			if (do_hook(NAPREQUEST_LIST, "%s \"%s\"", nick, filename) && !get_int_var(QUIET_SENDS_VAR))
				put_it("* %s has requested [%s]", nick, base_name(filename));
		}
		snprintf(buffer, BIG_BUFFER_SIZE, "%s \"%s\"", nick, newf->filename);
		send_ncommand(CMDS_FILEINFO, "%s", convertnap_dos(buffer));
		if (!gf)
		{
			if ((gf = create_send(nick, newf)))
			{
				gf->write = open(newf->filename, O_RDONLY | O_BINARY);
				OPENFILE(gf->write, newf->filename);
				add_to_transfer_list(gf);
			}
		}
	} 
	else
		say("* %s Requested file was not found [%s]", nick, filename);
	return 0;
}


void napfile_sendfile(int snum)
{
GetFile *gf;
unsigned char buffer[3*BIG_BUFFER_SIZE+1];
int rc, numread;
int flag = NORMAL_FINISH;
SocketList *s;

	if (!(s = get_socket(snum)) || !(gf = (GetFile *)s->info))
	{
		put_it("error get_socket(%d)", snum);
		nap_finished_file(snum, PREMATURE_FINISH);
		return;
	}
	if (gf->deleted)
	{
		if (gf->write != -1)
			close(gf->write);
		gf->write = -1;
		if (gf->deleted++ > 5)
			close_socketread(snum);
		return;
	}
	numread = read(gf->write, buffer, 2*BIG_BUFFER_SIZE);
	switch(numread)
	{
		case -1:
			flag = PREMATURE_FINISH;
			if (do_hook(NAPERROR_LIST, "%s %lu %lu \"%s\"", gf->nick, gf->received+gf->resume, gf->filesize, gf->filename) && !get_int_var(QUIET_SENDS_VAR))
				put_it("* Error sending %s (%lu/%lu) to %s ", gf->nick, gf->received+gf->resume, gf->filesize, base_name(gf->filename));
			break_from_list((List **)&transfer_struct, (List *)gf);
			close(gf->write);
			gf->write = -1;
			nap_finished_file(snum, flag);
			build_status(current_window, NULL, 0);
			send_ncommand(CMDS_UPDATE_SEND, NULL);
			return;
		case 0:
		{
			break_from_list((List **)&transfer_struct, (List *)gf);
			if ((gf->received + gf->resume) >= gf->filesize)
			{
				double speed;
				char speed1[80];
				time_t length;
				if (!(length = now - gf->starttime))
					length = 1;
				shared_stats.files_served++;
				shared_stats.filesize_served += gf->received;
				speed = gf->received / 1024.0 / length;
				if (speed > shared_stats.max_uploadspeed)
				{
					shared_stats.max_uploadspeed = speed;
					malloc_strcpy(&shared_stats.max_uploadspeed_nick, gf->nick);
				}
				sprintf(speed1, "%4.2fK/s", speed);
				if (do_hook(NAPFINISH_LIST, "%s %lu \"%s\"", gf->nick, gf->filesize, gf->filename) && !get_int_var(QUIET_SENDS_VAR))
					put_it("* Finished Sending %s (%s) [%s] at %s", gf->nick, longcomma(gf->filesize), base_name(gf->filename), speed1);
			}
			else 
			{
				flag = PREMATURE_FINISH;
				if (do_hook(NAPERROR_LIST, "%s %lu %lu \"%s\"", gf->nick, gf->received+gf->resume, gf->filesize, gf->filename) && !get_int_var(QUIET_SENDS_VAR))
					put_it("* Error sending %s (%lu/%lu) to %s ", gf->nick, gf->received+gf->resume, gf->filesize, base_name(gf->filename));
			}
			close(gf->write);
			gf->write = -1;
			nap_finished_file(snum, flag);
			build_status(current_window, NULL, 0);
			send_ncommand(CMDS_UPDATE_SEND, NULL);
			return;
		}
		default:
		{
			alarm(2);
			rc = send(snum, buffer, numread, 0);
			alarm(0);
			if (rc == -1)
			{
				if (errno == EWOULDBLOCK || errno == ENOBUFS || errno == EDEADLK)
					lseek(gf->write, -numread, SEEK_CUR);
				else
				{
					break_from_list((List **)&transfer_struct, (List *)gf);
					if (do_hook(NAPERROR_LIST, "%s %lu %lu \"%s\"", gf->nick, gf->received+gf->resume, gf->filesize, gf->filename) && !get_int_var(QUIET_SENDS_VAR))
						put_it("* Error sending %s (%lu/%lu) %s", gf->nick, gf->received+gf->resume, gf->filesize, base_name(gf->filename));
					close(gf->write);
					gf->write = -1;
					nap_finished_file(snum, PREMATURE_FINISH);
					build_status(current_window, NULL, 0);
					send_ncommand(CMDS_UPDATE_SEND, NULL);
				}
				return;
			}
			if (rc != numread)
				lseek(gf->write, -(numread - rc), SEEK_CUR);
			gf->received += rc;
			if (!(gf->received % (10 * (2*BIG_BUFFER_SIZE))))
				build_status(current_window, NULL, 0);
		}
	}
}

void napfile_read(int snum)
{
GetFile *gf;
int rc;
SocketList *s;
	s = get_socket(snum);
	if (!(gf = (GetFile *)get_socketinfo(snum)))
	{
		unsigned char buff[2*BIG_BUFFER_SIZE+1];
		unsigned char fbuff[2*BIG_BUFFER_SIZE+1];
		char *nick, *filename, *args;
		
		alarm(10);
		if ((rc = read(snum, buff, 2 * BIG_BUFFER_SIZE)) < 0)
		{
			alarm(0);
			nap_finished_file(snum, PREMATURE_FINISH);
			return;
		}
		alarm(0);
		buff[rc] = 0;
		args = &buff[0];
		if (!*args || !strcmp(buff, "FILE NOT FOUND") || 
			!strcmp(buff, "INVALID REQUEST") || 
			!strcmp(buff, "FILE NOT REQUESTED"))
		{
			say("Error in napfile_read(%d/%s) %s", snum, (rc == 0) ? strerror(errno) : "", *args ? args : "unknown read");
			nap_finished_file(snum, PREMATURE_FINISH);
			return;
		}

		nick = next_arg(args, &args);
		if ((filename = new_next_arg(args, &args)) && *filename)
		{
			strcpy(fbuff, filename);
			convertnap_unix(fbuff);
		}
		if (!nick || !filename || !*filename || !args || !*args
			|| !(gf = find_in_getfile(0, nick, NULL, fbuff, 0, NAP_UPLOAD))
			|| (gf->write == -1))
		{
			memset(buff, 0, 80);
			if (!gf)
				sprintf(buff, "INVALID REQUEST");

			else
			{
				sprintf(buff, "FILE NOT FOUND");
				break_from_list((List **)&transfer_struct, (List *)gf);
				gf->socket = snum;
				if (gf->write == -1)
				{
					send_ncommand(CMDS_REMOVEFILE, "%s", fbuff);
					say("Unable to open [%s]", fbuff);
				}
				else
				{
					close(gf->write);
					gf->write = -1;
				}
			}
			write(snum, buff, strlen(buff));
			nap_finished_file(snum, PREMATURE_FINISH);
			return;
		}
		gf->resume = my_atol(next_arg(args, &args));
		if (gf->resume >= gf->filesize)
		{
			break_from_list((List **)&transfer_struct, (List *)gf);
			close(gf->write);
			gf->write = -1;
			nap_finished_file(snum, PREMATURE_FINISH);
			return;
		}
		if (gf->socket != -1)
		{
			put_it("ERROR gf->socket != -1 %d %s %s", snum, nick, filename);
			break_from_list((List **)&transfer_struct, (List *)gf);
			close(gf->write);
			gf->write = -1;
			nap_finished_file(snum, PREMATURE_FINISH);
			return;
		}
		gf->socket = snum;
		lseek(gf->write, SEEK_SET, gf->resume);
		set_socketinfo(snum, gf);
		memset(buff, 0, 80);
		sprintf(buff, "%lu", gf->filesize);
		write(snum, buff, strlen(buff));
		s->func_write = s->func_read;
		s->is_write = s->is_read;
		if (do_hook(NAPSEND_LIST, "%s %s %lu \"%s\"", gf->resume ? "RESUME":"SEND", gf->nick, gf->filesize, gf->filename) && !get_int_var(QUIET_SENDS_VAR))
			put_it("* %sing file to %s [%s] (%s)", gf->resume ? "Resum" : "Send", gf->nick, base_name(gf->filename), longcomma(gf->filesize));
		set_non_blocking(snum);
		build_status(current_window, NULL, 0);
		send_ncommand(CMDS_UPDATE_SEND1, NULL);
		return;
	} else if (!gf->starttime)
		gf->starttime = now;
	s->func_write = s->func_read = napfile_sendfile;
	napfile_sendfile(snum);
}

extern void nap_firewall_start(int);
extern void naplink_handleconnect(int);

NAP_COMM(cmd_firewall_request) /* 501 */
{
char	*nick,
	*ip,
	*filename,
	*md5;
unsigned short port = 0;
	nick = next_arg(args, &args);
	ip = next_arg(args, &args);
	port = my_atol(next_arg(args, &args));
	filename = new_next_arg(args, &args);
	convertnap_unix(filename);
	md5 = next_arg(args, &args);
	if (!port)
		say("Unable to send to a firewalled system");
	else
	{
		GetFile *gf = NULL;
		int getfd;
		struct sockaddr_in socka;
		if (!(gf = find_in_getfile(1, nick, NULL, filename, 0, NAP_UPLOAD)))
		{
			say("no such file requested %s %s cmd_firewall_request()", nick, filename);
			return 0;
		}
		gf->checksum = m_strdup(md5);
		
		getfd = socket(AF_INET, SOCK_STREAM, 0);
		socka.sin_addr.s_addr = strtoul(ip, NULL, 10);
		socka.sin_addr.s_addr = BSWAP32(socka.sin_addr.s_addr);
		socka.sin_family = AF_INET;
		socka.sin_port = htons(port);
		gf->socket = getfd;
		set_socketinfo(getfd, gf);
		set_keepalive(getfd);

		alarm(get_int_var(CONNECT_TIMEOUT_VAR));
		if (connect(getfd, (struct sockaddr *)&socka, sizeof(struct sockaddr)) != 0) 
		{
			alarm(0);
			say("ERROR connecting [%s]", strerror(errno));
			send_ncommand(CMDR_DATAPORTERROR, "%s", gf->nick);
			if (gf->write != -1)
			{
				close(gf->write);
				gf->write = -1;
			}
			nap_finished_file(getfd, 0);
			return 0;
		}
		alarm(0);

		if (transfer_struct)
			transfer_struct->prev = gf;
		gf->next = transfer_struct;
		transfer_struct = gf;

		add_socketread(getfd, getfd, 0, inet_ntoa(socka.sin_addr), naplink_handleconnect, NULL);
	}
	return 0;
}

void nap_firewall_start1(int snum)
{
GetFile *gf;
unsigned char buffer[BIG_BUFFER_SIZE+1];
SocketList *s;
int rc;
unsigned long resume = 0;
	s = get_socket(snum);
	if (!s || !(gf = (GetFile *)get_socketinfo(snum)))
	{
		close_socketread(snum);
		return;
	}
	if ((rc = read(snum, buffer, BIG_BUFFER_SIZE)) < 1)
	{
		break_from_list((List **)&transfer_struct, (List *)gf);
		if (gf->write != -1)
		{
			close(gf->write);
			gf->write = -1;
		}
		nap_finished_file(snum, PREMATURE_FINISH);
		return;
	}	
	buffer[rc+1] = 0;
	resume = my_atol(buffer);
	put_it("Maybe this is where firewall resume is? %s", buffer);
	if (lseek(gf->write, resume, SEEK_SET) != -1)
		gf->resume = resume;
	
	gf->starttime = now;
	s->is_write = s->is_read;
	s->func_write = s->func_read = napfile_sendfile;
	napfile_sendfile(snum);
}

void nap_firewall_start(int snum)
{
GetFile *gf;
unsigned char buffer[BIG_BUFFER_SIZE+1];
SocketList *s;
	s = get_socket(snum);
	if (!s || !(gf = (GetFile *)get_socketinfo(snum)))
	{
		close_socketread(snum);
		return;
	}
	if (write(snum, "SEND", 4) == -1)
	{
		nap_finished_file(snum, PREMATURE_FINISH);
		return;
	}
	snprintf(buffer, BIG_BUFFER_SIZE, "%s \"%s\" %lu", get_server_nickname(from_server), gf->filename, gf->filesize);
	convertnap_dos(buffer);
	if (write(snum, buffer, strlen(buffer)) == -1)
	{
		nap_finished_file(snum, PREMATURE_FINISH);
		return;
	}
	s->func_read = nap_firewall_start1;
}

int is_valid_dcc(char *nick)
{
	return find_in_getfile(0, nick, NULL, NULL, 0, NAP_CHAT) ? 1 : 0;
}

static void	new_dcc_message_transmit (char *user, const char *text, const char *text_display, int type, int flag)
{
GetFile  *gf = NULL;
char		tmp[BIG_BUFFER_SIZE+1];
int 		len = 0;
char		buffer[NICKNAME_LEN+10];

	*tmp = 0;

	switch(type)
	{
		case NAP_CHAT_CONNECTED:
			break;
		default:
			return;
	}
	gf = find_in_getfile(0, user, NULL, NULL, 0, type);
	if (!gf)
	{
		if ((gf = find_in_getfile(0, user, NULL, NULL, 0, NAP_CHAT)))
			say("No active DCC CHAT connection for %s", user);
		else
			say("No DCC CHAT connection for %s", user);
		return;
	}

	strmcat(tmp, text, BIG_BUFFER_SIZE-3);
	len = strlen(tmp);

	my_encrypt(tmp, len, gf->passwd);
	tmp[len++] = '\n';
	tmp[len] = 0;

	write(gf->socket, tmp, len);
	gf->received += len;

	if (flag)
		put_it(FORMAT_SENDDCC, user, gf->ip, text_display?text_display:text);
	sprintf(buffer, "=%s", user);
	addtabkey(buffer, "msg", (char *)(text_display ? text_display : text));
}

void	dcc_chat_transmit (char *user, const char *text, const char *orig, int noisy)
{
	new_dcc_message_transmit(user, text, orig, NAP_CHAT_CONNECTED, noisy);
}

#define ALLOW_DCC_COMMANDS

void nap_chat(int snum)
{
GetFile *gf;
unsigned char buffer[3*BIG_BUFFER_SIZE+1];
SocketList *s;
long bytesread;
char *tmp, *p;

	if (!(s = get_socket(snum)) || !(gf = (GetFile *)s->info))
	{
		put_it("error get_socket(%d)", snum);
		nap_finished_file(snum, PREMATURE_FINISH);
		return;
	}
	if (gf->deleted)
	{
		if (gf->write != -1)
			close(gf->write);
		gf->write = -1;
		if (gf->deleted++ > 5)
			close_socketread(snum);
		return;
	}
	bytesread = new_dgets(buffer, snum, 1, BIG_BUFFER_SIZE);
	switch (bytesread)
	{
		case -1:
			say("Lost DCC CHAT to %s [%s]", gf->nick, 
				(dgets_errno == -1) ? "Remote End Closed Connection" : 
					strerror(dgets_errno));
			break_from_list((List **)&transfer_struct, (List *)gf);
			nap_finished_file(snum, PREMATURE_FINISH);
			break;
		case 0:
			break;
		default:
			tmp = buffer;
			if ((p = strrchr(tmp, '\r')))
				*p = 0;
			if ((p = strrchr(tmp, '\n')))
				*p = 0;
			my_decrypt(tmp, strlen(tmp), gf->passwd);
			gf->received += bytesread;
#ifdef ALLOW_DCC_COMMANDS
			if ((gf->flags & NAP_DCC_COMMANDS) == NAP_DCC_COMMANDS)
			{
				if (!my_strnicmp(tmp, ".cmd ", 5) && *(tmp+6))
					parse_line("DCC", tmp+5, NULL, 0, 0, 1);
			}
#endif
			put_it(FORMAT_DCC_MSG, gf->nick, gf->ip, tmp);
			break;
	}
}

void nap_chat_start(int snum)
{
GetFile *gf;
SocketList *s;

	if (!(s = get_socket(snum)) || !(gf = (GetFile *)s->info))
	{
		put_it("error get_socket(%d)", snum);
		nap_finished_file(snum, PREMATURE_FINISH);
		return;
	}
	if (gf->deleted)
	{
		if (gf->write != -1)
			close(gf->write);
		gf->write = -1;
		if (gf->deleted++ > 5)
			close_socketread(snum);
		return;
	}
	if ((gf->flags & NAP_CHAT) == NAP_CHAT)
	{
		/* we need to accept the connection here. */
		struct  sockaddr_in     remaddr;
		int sra = sizeof(struct sockaddr_in);
		int sock = -1;
		if ((sock = my_accept(snum, (struct sockaddr *) &remaddr, &sra)) > -1)
		{
			set_keepalive(sock);
			gf->port = ntohs(remaddr.sin_port);
			add_socketread(sock, gf->port, 0, inet_ntoa(remaddr.sin_addr), nap_chat, NULL);
			set_socketinfo(sock, gf);
			s->info = NULL;
			gf->flags = NAP_CHAT_CONNECTED;
			gf->socket = sock;
			malloc_strcpy(&gf->ip, inet_ntoa(remaddr.sin_addr));
			say("DCC CHAT to %s [%s:%d] established", gf->nick, gf->ip, gf->port);
			gf->starttime = now;
		}
		close_socketread(snum);
		return;
	}
}

BUILT_IN_COMMAND(send_file)
{
char buffer[BIG_BUFFER_SIZE+1];
extern struct  sockaddr_in localaddr;
char *nick, *filename;
extern Server *server_list;
GetFile *gf;
struct stat st;
char *n, *n1;
char *new_file = NULL;
int dcc_comm = 0;

	if (!(nick = next_arg(args, &args)) || (from_server == -1))
		return;
	if (command && !my_stricmp(command, "CHAT"))
	{
		unsigned short portnum = 0;
		int s;
		if (!my_stricmp(nick, "-cmd"))
		{
			nick = next_arg(args, &args);
			dcc_comm = 1;
		}
		
		n = LOCAL_COPY(nick);
		while ((n1 = next_in_comma_list(n, &n)))
		{
			if (!n1 || !*n1)
				break;
			if (!my_stricmp(n1, get_server_nickname(from_server)))
				continue;
			if ((gf = find_in_getfile(0, n1, NULL, NULL, 0, NAP_CHAT_CONNECTED)))
			{
				say("There is already an active chat for %s", n1);
				continue;
			}
			if ((gf = find_in_getfile(0, n1, NULL, NULL, 0, NAP_CHAT)))
			{
				struct sockaddr_in socka;
				s = socket(AF_INET, SOCK_STREAM, 0);
				socka.sin_addr.s_addr = strtoul(gf->ip, NULL, 10);
				socka.sin_addr.s_addr = socka.sin_addr.s_addr;
				socka.sin_family = AF_INET;
				socka.sin_port = ntohs(gf->port);
				alarm(get_int_var(CONNECT_TIMEOUT_VAR));
				set_keepalive(s);
				gf->socket = s;
				if (connect(s, (struct sockaddr *)&socka, sizeof(struct sockaddr)) != 0) 
				{
					alarm(0);
					say("Unable to connect to chat port for %s [%s:%d]", gf->nick, inet_ntoa(socka.sin_addr), socka.sin_port);
					set_socketinfo(s, gf);
					nap_finished_file(s, 0);
					continue;
				}
				alarm(0);
				malloc_strcpy(&gf->ip, inet_ntoa(socka.sin_addr));
				add_socketread(s, gf->port, 0, gf->nick, nap_chat, NULL);
				set_socketinfo(s, gf);
				gf->addtime = now;
				gf->flags = NAP_CHAT_CONNECTED;
				if (dcc_comm)
					gf->flags |= NAP_DCC_COMMANDS;
				gf->starttime = now;
				say("DCC CHAT connected to %s [%s:%d]", gf->nick, gf->ip, gf->port);
				continue;
			}
			if ((s = connect_by_number(NULL, &portnum, SERVICE_SERVER, PROTOCOL_TCP, 1)) < 0)
			{
				portnum = 0;
				s = connect_by_number(NULL, &portnum, SERVICE_SERVER, PROTOCOL_TCP, 1);
			}
			if (s < 0)
			{
				say("DCC Unable to create connection: %s", errno ? strerror(errno) : "Unknown Host");
				continue;
			}
			gf = new_malloc(sizeof(GetFile));
			gf->nick = m_strdup(n1);
			gf->write = -1;
			gf->socket = s;
			gf->flags = NAP_CHAT;
			gf->addtime = now;
			if (args && *args)
				gf->passwd = m_strdup(args);
			snprintf(buffer, BIG_BUFFER_SIZE, "%s %cCHAT %s %lu %d%s%s%c", 
				n1, '\001', get_server_nickname(from_server), 
				(unsigned long)BSWAP32(localaddr.sin_addr.s_addr), 
				portnum, gf->passwd ? space : empty_string,
				gf->passwd ? gf->passwd : empty_string,'\001');
			send_ncommand(CMDS_SENDMSG, "%s", buffer);
			add_to_transfer_list(gf);
			add_socketread(s, gf->port, 0, gf->nick, nap_chat_start, NULL);
			set_socketinfo(s, gf);
		}
		return;
	}
	while ((filename = new_next_arg(args, &args)))
	{
		if (!filename || !*filename)
			break;
		new_file = expand_twiddle(filename);
		if (!new_file || (stat(new_file, &st) == -1))
		{
			say("Unable to open or find %s", new_file == NULL ? filename : new_file);
			return;
		}
		n = LOCAL_COPY(nick);
		while ((n1 = next_in_comma_list(n, &n)))
		{
			if (!n1 || !*n1)
				break;
			snprintf(buffer, BIG_BUFFER_SIZE, "%s %cSEND %s %lu %d \"%s\" %lu %s %d%c", 
				n1, '\001', get_server_nickname(from_server),
				(unsigned long)BSWAP32(localaddr.sin_addr.s_addr),
				server_list[from_server].dataport, base_name(new_file),
				(unsigned long)st.st_size, "checksum", 0, '\001');
			say("DCC SEND using [%s:%d]", inet_ntoa(localaddr.sin_addr), server_list[from_server].dataport);
			gf = new_malloc(sizeof(GetFile));
			gf->nick = m_strdup(n1);
			gf->checksum = m_strdup("checksum");
			gf->filename = m_strdup(base_name(new_file));
			gf->write = -1;
			gf->socket = -1;
			gf->filesize = st.st_size;
			gf->flags = NAP_UPLOAD;
			gf->addtime = now;
			send_ncommand(CMDS_SENDMSG, "%s", buffer);
			gf->write = open(new_file, O_RDONLY | O_BINARY); 
			OPENFILE(gf->write, new_file);
			add_to_transfer_list(gf);
			new_free(&new_file);
		}
	}
}	

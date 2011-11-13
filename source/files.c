/*
 * various c functions for receiving files on napster.
 * Copyright Colten Edwards Feb 2000
 * $Id: files.c,v 1.1.1.1 2001/01/24 15:21:47 edwards Exp $
 */
 
#include "teknap.h"
#include "struct.h"
#include "list.h"
#include "hook.h"
#include "server.h"
#include "commands.h"
#include "output.h"
#include "server.h"
#include "vars.h"
#include "scott.h"
#include "status.h"
#include "window.h"
#include "newio.h"
#include "napster.h"

#include <sys/stat.h>
#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#if HAVE_LIBWRAP
#include <tcpd.h>
int allow_severity;              /* for connection logging */
int deny_severity;               /* for connection logging */
int hosts_ctl (char *, char *, char *, char *);
#endif


ResumeFile *resume_struct = NULL;
int move_file_to_unfinished(GetFile *, char *);
void nap_getfile(int);
void nap_getfilestart(int);


extern Server *server_list;

void add_to_transfer_list(GetFile *ptr)
{
GetFile *last;
	ptr->next = ptr->prev = NULL;
	last = transfer_struct;
	while (last)
	{
		if (!last->next)
			break;
		last = last->next;
	}
	if (last)
	{
		last->next = ptr;
		ptr->prev = last;
	}
	else
		transfer_struct = ptr;
}

int files_in_progress(char *nick, int type)
{
int ret = 0;
GetFile *tmp;
	for (tmp = transfer_struct; tmp; tmp = tmp->next)
	{
		if (type == -1)
		{
			if (!nick || !my_stricmp(nick, tmp->nick))
				ret++;
		}
		else if ((tmp->flags & type) == type)
		{
			if (!nick || !my_stricmp(nick, tmp->nick))
				ret++;
		}
	}
	return ret;
}

void add_files_to_whois(void)
{
GetFile *tmp;
	for (tmp = transfer_struct; tmp; tmp = tmp->next)
	{
		if ((tmp->flags & NAP_DOWNLOAD) == NAP_DOWNLOAD)
			send_ncommand(CMDS_UPDATE_GET1, NULL);
		else if ((tmp->flags & NAP_UPLOAD) == NAP_UPLOAD)
			send_ncommand(CMDS_UPDATE_SEND1, NULL);
	}
}

BUILT_IN_COMMAND(nap_del)
{
int count = 0;
GetFile *tmp, *last = NULL;
char *t;
	if (!args && !*args)
	{
		glist(NULL, NULL, NULL, NULL, 0);
		return;
	}
	if (*args == '*')
	{
		say("Removing ALL file send/upload");
		for (tmp = transfer_struct; tmp; tmp = last)
		{
			count++;
			last = tmp->next;
			break_from_list((List **)&transfer_struct, (List *)tmp);
			if (tmp->filename)
				put_it("Removing %s [%s]", tmp->nick, base_name(tmp->filename));
			else
				put_it("Removing chat request from %s", tmp->nick);
			if ((tmp->flags & NAP_DOWNLOAD) == NAP_DOWNLOAD)
				send_ncommand(CMDS_UPDATE_GET, NULL);
			else if ((tmp->flags & NAP_UPLOAD) == NAP_UPLOAD)
				send_ncommand(CMDS_UPDATE_SEND, NULL);
			if (tmp->socket > 0)
				nap_finished_file(tmp->socket, PREMATURE_FINISH);
			else
			{
				if (tmp->filestruct)
				{
					tmp->filestruct->getfile = NULL;
					tmp->filestruct->flags = 0;
				}
				tmp->next = finished_struct;
				finished_struct = tmp;
			}
		}
		transfer_struct = NULL;
		build_status(current_window, NULL, 0);
		return;
	}
	while ((t = next_arg(args, &args)))
	{	
		char *name = NULL;
		count = 1;
		if (!my_atol(t))
			name = t;
		tmp = transfer_struct;
		while (tmp)
		{
			last = tmp->next;
			if (matchnumber(t, count) || (name && !my_stricmp(name, tmp->nick)))
			{
				break_from_list((List **)&transfer_struct, (List *)tmp);
				if (tmp->filename)
					say("Removing %s [%s]",tmp->nick, base_name(tmp->filename));
				else
					put_it("Removing chat request from %s", tmp->nick);
				if ((tmp->flags & NAP_DOWNLOAD) == NAP_DOWNLOAD)
					send_ncommand(CMDS_UPDATE_GET, NULL);
				else if ((tmp->flags & NAP_UPLOAD) == NAP_UPLOAD)
					send_ncommand(CMDS_UPDATE_SEND, NULL);
				if (tmp->socket > 0)
					nap_finished_file(tmp->socket, PREMATURE_FINISH);
				else
				{
					if (tmp->filestruct)
					{
						tmp->filestruct->getfile = NULL;
						tmp->filestruct->flags = 0;
					}
					tmp->next = finished_struct;
					finished_struct = tmp;
				}
				build_status(current_window, NULL, 0);
			}
			tmp = last;
			count++;
		}
	}
}

void create_and_do_get(FileStruct *sf, int flags, int gui)
{
GetFile *new;
	send_ncommand(CMDS_REQUESTFILE, "%s \"%s\"", sf->nick, sf->filename);
	new = new_malloc(sizeof(GetFile));
	new->nick = m_strdup(sf->nick);
	new->filename = m_strdup(sf->filename);
	new->filesize = sf->filesize;
	new->checksum = m_strdup(sf->checksum);
	new->flags = NAP_DOWNLOAD | flags;
	new->addtime = now;
	new->write = new->socket = -1;

	add_to_transfer_list(new);
	if (!gui)
	{
		sf->getfile = new;
		new->filestruct = sf;
	}
}

int create_and_get(int port, GetFile *gf)
{
int getfd;
struct sockaddr_in socka;
char indata[BIG_BUFFER_SIZE+1];

	getfd = socket(AF_INET, SOCK_STREAM, 0);
	socka.sin_addr.s_addr = strtoul(gf->ip, NULL, 10);
	socka.sin_addr.s_addr = BSWAP32(socka.sin_addr.s_addr);
	socka.sin_family = AF_INET;
	socka.sin_port = htons(port);
	alarm(get_int_var(CONNECT_TIMEOUT_VAR));
	set_keepalive(getfd);
	if (connect(getfd, (struct sockaddr *)&socka, sizeof(struct sockaddr)) != 0) 
	{
		alarm(0);
		send_ncommand(CMDR_DATAPORTERROR, "%s", gf->nick);
		say("Unable to connect to %s [%s:%d]", gf->nick, inet_ntoa(socka.sin_addr), ntohs(socka.sin_port));
		gf->socket = getfd;
		set_socketinfo(getfd, gf);
		nap_finished_file(getfd, 0);
		return 0;
	}
	alarm(0);
	sprintf(indata, "%lu", gf->filesize);
	gf->count = strlen(indata);
	write(getfd, "GET", 3);
       	snprintf(indata, sizeof(indata), "%s \"%s\" %lu", get_server_nickname(from_server), gf->filename, gf->resume);
	write(getfd, indata, strlen(indata));
	add_socketread(getfd, gf->port, 0, gf->nick, nap_getfilestart, NULL);
	set_socketinfo(getfd, gf);
	gf->socket = getfd;
	gf->addtime = now;
	gf->flags = NAP_DOWNLOAD;

	add_to_transfer_list(gf);
	return 1;
}


BUILT_IN_COMMAND(request)
{
char *nick, *filen, *comm;

	if (!my_stricmp(command, "dcc_get"))
	{
		GetFile *gf = NULL;		
		char *n;
		nick = next_arg(args, &args);
		if (nick)
		{
			while ((n = next_in_comma_list(nick, &nick)))
			{
				if (!n || !*n)
					break;
				if ((gf = find_in_getfile(1, n, NULL, NULL, 0, NAP_DOWNLOAD)))
					if (gf->port)
						create_and_get(gf->port, gf);
			}
		}
		return;
	}
	else if (!my_stricmp(command, "request"))
	{
		nick = next_arg(args, &args);
		filen = new_next_arg(args, &args);
		if (nick && filen && *filen)
		{
			GetFile *new;
			send_ncommand(CMDS_REQUESTFILE, "%s \"%s\"", nick, filen);
			new = new_malloc(sizeof(GetFile));
			new->nick = m_strdup(nick);
			new->filename = m_strdup(filen);
			new->flags = NAP_DOWNLOAD;
			new->addtime = now;
			new->write = new->socket = -1;
			new->next = transfer_struct;
			transfer_struct = new;
		}
	}
	else if (!my_stricmp(command, "get"))
	{
		int	count = 1;
		FileStruct *sf = NULL;
		FileStruct **f = NULL;
		if (!args || !*args)
		{
			int count = 1, maxwidth;
			f = get_search_head(from_server);
			if (!f || !(sf = *f))
			{
				say("Need to specify a nick");
				return;
			}
			maxwidth = widest_filename(sf);
			for (; sf; sf = sf->next, count++)
				print_file(sf, count, maxwidth);
			return;
		}
		while (args && *args)
		{
			int req, browse;
			FileStruct **f = NULL;
			sf = NULL;
			req = browse = 0;
			count = 1;
			comm = next_arg(args, &args);
			f = get_search_head(from_server);
			
			if (!my_strnicmp(comm, "-request", 3))
			{
				req = 1;
				comm = next_arg(args, &args);
			}
			else if (!my_strnicmp(comm, "-browse", 3))
			{
				NickStruct *n;
				char *nick = NULL;
				browse = 1;
				comm = next_arg(args, &args);
				if ((n = (NickStruct *)find_in_list((List **)&server_list[from_server].users, comm, 0)))
				{
					sf = n->file_browse;
					comm = next_arg(args, &args);
				}
				else if ((nick = next_arg(args, &args)))
				{
					if ((n = (NickStruct *)find_in_list((List **)&server_list[from_server].users, nick, 0)))
						sf = n->file_browse;
				}
			}
			if (!req && !browse)
			{
				if (f && *f)
					sf = *f;
				else
				{
					NickStruct *n;
					char *nick;
					if ((n = (NickStruct *)find_in_list((List **)&server_list[from_server].users, comm, 0)))
					{
						sf = n->file_browse;
						comm = next_arg(args, &args);
					}
					else if ((nick = next_arg(args, &args)))
					{
						if ((n = (NickStruct *)find_in_list((List **)&server_list[from_server].users, nick, 0)))
							sf = n->file_browse;
					}
				}
			} 
			else if (req && f)
				sf = *f;

			if (sf && comm)
			{
				for (; sf; sf = sf->next, count++)
				{
					if (matchnumber(comm, count))
					{
						create_and_do_get(sf, 0, 0);
						continue;
					}
				}
			} 
			else
				say("No Such number [%s] in %s", comm ? comm : zero, sf ? (sf == *f ? "search" : "browse"):"Ack");
		}
	}
}


GetFile *find_in_getfile(int remove, char *nick, char *check, char *file, unsigned long size, int flags)
{
GetFile *last, *tmp;
	last = NULL;
	if (!nick)
		return NULL;
	for (tmp = transfer_struct; tmp; tmp = last)
	{
		last = tmp->next;
		if (!my_stricmp(tmp->nick, nick))
		{
			if (check && tmp->checksum && my_stricmp(tmp->checksum, check))
				continue;
			if (file && tmp->filename && my_stricmp(tmp->filename, file))
				continue;
			if (size && (tmp->filesize != size))
				continue;
			if ((flags != -1) && ((tmp->flags & flags) != flags))
				continue;
			if (remove)
				break_from_list((List **)&transfer_struct, (List *)tmp);
			return tmp;
		}
	}
	return NULL;
}

void clean_sockets(void)
{
GetFile *gf, *last;
	while ((gf = finished_struct))
	{
		last = gf->next;
		if (gf->write > 0)
		{
			close(gf->write);
			CLOSEFILE(gf->write);
		}
		close_socketread(gf->socket);
		gf->socket = gf->write = -1;
		new_free(&gf->nick);
		new_free(&gf->filename);
		new_free(&gf->checksum);
		new_free(&gf->realfile);
		new_free(&gf->ip);
		new_free(&gf->passwd);
		new_free(&gf);
		finished_struct = last;
	}
}

void remove_search_from_glist(FileStruct **f)
{
FileStruct *f1;
	if (!f || !*f)
		return;
	for (f1 = *f; f1; f1 = f1->next)
	{
		if (f1->getfile)
			f1->getfile->filestruct = NULL;
		f1->getfile = NULL;
	}
}

int nap_finished_file(int snum, int flag)
{
GetFile *gf;
	gf = (GetFile *)get_socketinfo(snum);
	if (gf)
	{
		if (gf->deleted)
			return 0;
		gf->deleted++;
		/* Boy, does this look wrong, but its right.
		 * This breaks the link from File <---> Get
		 */
		if ((flag == PREMATURE_FINISH) && ((gf->flags & NAP_DOWNLOAD) == NAP_DOWNLOAD))
			move_file_to_unfinished(gf, base_name(gf->filename));
		if (gf->filestruct) 
		{
			if (flag != PREMATURE_FINISH)
				gf->filestruct->flags |= DOWNLOADED_FILE;
			else
				gf->filestruct->flags = 0;
			gf->filestruct->getfile = NULL;
		}
		gf->next = finished_struct;
		finished_struct = gf;
	} 
	close_socketread(snum);
	if (gf)
		gf->socket = -1;
	if (in_browser)
		refresh_browser();

	return 0;
}

void nap_getfile(int snum)
{
char indata[BIG_BUFFER_SIZE+1];
SocketList *s;
int rc;
int count = sizeof(indata) - 1;
GetFile *gf;
unsigned long nbytes = 0;
int flag = NORMAL_FINISH;

	s = get_socket(snum);
	if (!(gf = (GetFile *)get_socketinfo(snum)))
	{
		close_socketread(snum);
		send_ncommand(CMDS_UPDATE_GET, NULL);
		return;
	}
	if (gf->deleted)
		return;
	if (gf->count)
	{
		int flags = O_WRONLY | O_BINARY;
		memset(&indata, 0, 200);
		if (gf->count > 200)
			gf->count = 200;
		if ((rc = read(snum, &indata, gf->count)) != gf->count)
			return;
		if (!isdigit(*indata) || !*(indata+1) || !isdigit(*(indata+1)))
		{
			rc += read(snum, &indata[gf->count], sizeof(indata)-1);
			indata[rc] = 0;
			if (do_hook(NAP_LIST, "Error get 0 %s %s", gf->nick, indata))
				say("Invalid Reply from %s is %s", gf->nick, indata);
			break_from_list((List **)&transfer_struct, (List *)gf);
			nap_finished_file(snum, PREMATURE_FINISH);
			return;
		}
		gf->count = 0;
		set_non_blocking(snum);
		gf->starttime = time(NULL);
		if (!gf->resume)
			flags |= O_CREAT;
		if (!gf->realfile || ((gf->write = open(gf->realfile, flags, 0644)) == -1))
		{
			if (do_hook(NAP_LIST, "Error get 0 \"%s\" %s", gf->realfile, strerror(errno)))
				say("Error opening output file %s: %s\n", base_name(gf->realfile), strerror(errno));
			break_from_list((List **)&transfer_struct, (List *)gf);
			nap_finished_file(snum, PREMATURE_FINISH);
			return;
		}
		if (gf->resume)
			lseek(gf->write, gf->resume, SEEK_SET);	
		sprintf(indata, "(%lu/%s)", gf->resume, longcomma(gf->filesize));
		if (do_hook(NAP_LIST, "request get 0 %s %s %s", gf->nick, indata, gf->filename))
			say("%sing from %s %s %s", gf->resume ? "Resum":"Gett", gf->nick, indata, base_name(gf->filename));
		send_ncommand(CMDS_UPDATE_GET1, NULL);
		build_status(current_window, NULL, 0);
		return;
	} 

        if ((rc = ioctl(snum, FIONREAD, &nbytes) != -1))
	{
		if (nbytes)
		{
			count = (nbytes > count) ? count : nbytes;
			if (count + gf->received + gf->resume > gf->filesize)
				count = gf->filesize - gf->received - gf->resume;
			rc = read(snum, indata, count);
		} else
			rc = 0;
	}
	switch (rc)
	{
get_error:
		case -1:
			if (do_hook(NAP_LIST, "Error get 0 %s %s", gf->nick, gf->filename))
				say("ERROR reading file [%s]", strerror(errno));
			flag = PREMATURE_FINISH;
		case 0:
		{
			char speed1[80];
			double speed;
			time_t t;
			break_from_list((List **)&transfer_struct, (List *)gf);
			if (!(t = now - gf->starttime))
				t = 1;
			speed = gf->received / 1024.0 / t;
			sprintf(speed1, "%4.2fK/s", speed);
			if ((gf->received + gf->resume) >= gf->filesize)
			{
				if (rc != -1)
					if (do_hook(NAP_LIST, "notify get 0 %s %lu %lu %4.2f %s", gf->nick, gf->filesize, gf->received, speed, gf->filename))
						put_it("* Finished %sing %s (%s) from %s %s", gf->resume ? "Resum":"Gett", base_name(gf->filename), longcomma(gf->filesize), gf->nick, speed1);
				if (speed > shared_stats.max_downloadspeed)
				{
					shared_stats.max_downloadspeed = speed;
					malloc_strcpy(&shared_stats.max_downloadspeed_nick, gf->nick);
				}
				shared_stats.files_received++;
				shared_stats.filesize_received += gf->received;
			}
			else
			{
				flag = PREMATURE_FINISH;
				if (do_hook(NAP_LIST, "error get 0 %s %lu %lu %s", gf->nick, gf->filesize, gf->received+gf->resume, gf->filename))
					put_it("* Error %sing %s (%lu/%lu) from %s", gf->resume ? "Resum":"Gett", base_name(gf->filename), gf->received + gf->resume, gf->filesize, gf->nick);
			}
			send_ncommand(CMDS_UPDATE_GET, NULL);
			nap_finished_file(snum, flag);
			build_status(current_window, NULL, 0);
			return;
		}
		default:
			break;
	}
	if (write(gf->write, indata, rc) != rc)
		goto get_error;
		
	gf->received += rc;
	if (in_browser) {
		/* Try to only refresh, when the percent changes */
		int new_perc = gf->filesize % gf->received;
		if (new_perc != gf->last_perc) {
			gf->last_perc = new_perc;
			refresh_browser();
		}
	}
	if ((gf->received+gf->resume) >= gf->filesize)
	{
		char speed1[80];
		double speed;
		time_t t;
		break_from_list((List **)&transfer_struct, (List *)gf);
		if (!(t = now - gf->starttime))
			t = 1;
		speed = gf->received / 1024.0 / t;
		sprintf(speed1, "%4.2fK/s", speed);
		if (speed > shared_stats.max_downloadspeed)
		{
			shared_stats.max_downloadspeed = speed;
			malloc_strcpy(&shared_stats.max_downloadspeed_nick, gf->nick);
		}
		if (do_hook(NAP_LIST, "notify get 0 %s %lu %lu %4.2f %s", gf->nick, gf->filesize, gf->received, speed, gf->filename))
			put_it("* Finished %sing %s (%s) from %s %s", gf->resume ? "Resum":"Gett", base_name(gf->filename), longcomma(gf->filesize), gf->nick, speed1);
		shared_stats.files_received++;
		shared_stats.filesize_received += gf->received;
		send_ncommand(CMDS_UPDATE_GET, NULL);
		nap_finished_file(snum, NORMAL_FINISH);
		build_status(current_window, NULL, 0);
	}
}

void nap_getfilestart(int snum)
{
SocketList *s;
int rc;
char c;
GetFile *gf;
	s = get_socket(snum);
	gf = (GetFile *)get_socketinfo(snum);
	if (gf)
	{
		set_blocking(snum);
		if ((rc = read(snum, &c, 1)) != 1)
			return;
		s->func_read = nap_getfile;
		return;
	}
	close_socketread(snum);
}


void nap_firewall_get(int snum)
{
char indata[4*BIG_BUFFER_SIZE+1];
int rc;
	memset(indata, 0, sizeof(indata));
	alarm(15);
	rc = recv(snum, indata, sizeof(indata)-1, 0);
	alarm(0);
	switch(rc)
	{
		case -1:
			close_socketread(snum);
			if (do_hook(NAP_LIST, "Error get 1 %s", strerror(errno)))
				say("ERROR in nap_firewall_get %s", strerror(errno));
		case 0:
			break;
		default:
		{
			char *args, *nick, *filename;
			unsigned long filesize;
			GetFile *gf;
			SocketList *s;

			s = get_socket(snum);
			if (!strncmp(indata, "FILE NOT", 8) || !strncmp(indata, "INVALID DATA", 10))
			{
				nap_finished_file(snum, PREMATURE_FINISH);
				return;
			}
			args = &indata[0];
			if (!(nick = next_arg(args, &args)))
			{
				nap_finished_file(snum, PREMATURE_FINISH);
				return;
			}
			filename = new_next_arg(args, &args);
			filesize = my_atol(next_arg(args, &args));
			if (!filename || !*filename || !filesize)
			{
				nap_finished_file(snum, PREMATURE_FINISH);
				return;
			}
			if ((gf = find_in_getfile(0, nick, NULL, filename, 0, NAP_DOWNLOAD)))
			{
				int flags = O_WRONLY | O_BINARY;
				gf->count = 0;
				set_non_blocking(snum);
				gf->starttime = time(NULL);
				gf->socket = snum;
				gf->filesize = filesize;
				if (!gf->resume)
					flags |= O_CREAT;
				if (!gf->realfile || ((gf->write = open(gf->realfile, flags, 0644)) == -1))
				{
					if (do_hook(NAP_LIST, "Error get 1 \"%s\" %s", gf->realfile, strerror(errno)))
						say("Error opening output file %s: %s\n", base_name(gf->realfile), strerror(errno));
					gf = (GetFile *)break_from_list((List **)&transfer_struct, (List *)gf);
					nap_finished_file(snum, PREMATURE_FINISH);
					return;
				}
				if (gf->resume)
					lseek(gf->write, gf->resume, SEEK_SET);	
				sprintf(indata, "%lu", gf->resume);
				write(snum, indata, strlen(indata));
				sprintf(indata, "%4.2g%s %4.2g%s", _GMKv(gf->resume), _GMKs(gf->resume), _GMKv(gf->filesize), _GMKs(gf->filesize));
				if (do_hook(NAP_LIST, "notify get 1 %s %lu %s", gf->nick, gf->filesize, gf->filename))
					put_it("* %sing from %s %s (%s)", gf->resume?"Resum":"Gett", gf->nick, base_name(gf->filename), longcomma(gf->filesize));
				send_ncommand(CMDS_UPDATE_GET1, NULL);
				build_status(current_window, NULL, 0);
				s->func_read = nap_getfile;
				set_socketinfo(snum, gf);
			}
		}
	}
}

NAP_COMM(cmd_getfile)
{
/*
gato242 3068149784 6699 "d:\mp3\Hackers_-_07_-_Orbital_-_Halcyon_&_On_&_On.mp3"
8b451240c17fec98ea4f63e26bd42c60 7
*/
unsigned short port;
int speed;
char *nick, *file, *checksum, *ip, *dir = NULL, *s;
char *realfile = NULL;
char *newfile = NULL;
char indata[4*BIG_BUFFER_SIZE+1];
GetFile *gf = NULL;
struct stat st;
                
	nick = next_arg(args, &args);
	ip = next_arg(args, &args);
	port = my_atol(next_arg(args, &args));
	file = new_next_arg(args, &args);
	checksum = next_arg(args, &args);
	speed = my_atol(args);

	if (!nick || !ip || !file || !checksum)
	{
		if (do_hook(NAP_LIST, "error get -1 %s", "NULL"))
			say("Error in cmd_getfile(), value == NULL");
		return 0;
	}
	newfile = LOCAL_COPY(file);
	if (!strcmp(checksum, "firewallerror"))
	{
		if (do_hook(NAP_LIST, "error get 1 %s", "firewall"))
			say("You are both firewalled. Unable to complete transfer");
		return 0;
	}
	if ((s = get_string_var(ILLEGAL_CHARS_VAR)) && *s)
	{
		char *r;
		for (r = newfile; *r; r++)
			if (strchr(get_string_var(ILLEGAL_CHARS_VAR), *r))
				*r = '_';
	}
	if (!(gf = find_in_getfile(1, nick, NULL, file, 0, NAP_DOWNLOAD)))
	{
		if (!(gf = find_in_queue(1, nick, NULL, file, 0)))
		{
			if (do_hook(NAP_LIST, "error get -1 %s %s", nick, file))
				say("request %s %s not in cmd_getfile()", nick, file);
			return 0;
		}
	}
	gf->ip = m_strdup(ip);	
	gf->checksum = m_strdup(checksum);
	gf->speed = atol(args);
	gf->port = port;

	if (!(dir = get_string_var(DOWNLOAD_DIRECTORY_VAR)))
		dir = "~";
	snprintf(indata, sizeof(indata), "%s/%s", dir, base_name(newfile));

	realfile = expand_twiddle(indata);

	gf->realfile = realfile;
	if (!(stat(realfile, &st)) && (get_int_var(RESUME_DOWNLOAD_VAR) || (gf->flags & NAP_RESUME)))
		gf->resume = st.st_size;

	gf->write = -1;

	if (!port) 
	{
		/* this is a firewalled host. make a listen socket instead */
		send_ncommand(CMDS_REQUESTFILEFIRE, "%s \"%s\"", nick, file);
		if (do_hook(NAP_LIST, "request get 1 %s %lu %s", nick, gf->filesize, file))
			say("Attempting to get from a firewalled host %s", nick);
		gf->socket = -1;
		gf->addtime = now;
		add_to_transfer_list(gf);
	}
	else
		create_and_get(port, gf);
	return 0;
}

char *calc_eta(GetFile *sg)
{
static char ret[20];
int seconds, minutes;
time_t s_time;
	if (!sg->starttime)
		return strcpy(ret, "N/A");
	s_time = now - sg->starttime;
	if (s_time <= 0)
		s_time = 1;
	if (!(sg->received / s_time))
		return strcpy(ret, "N/A");
	seconds = (int) (sg->filesize - sg->resume - sg->received) / (sg->received / s_time);
	minutes = seconds / 60;
	seconds = seconds - (minutes * 60);
	if (minutes > 999) 
	{
		minutes = 999;
		seconds = 59;
	}	
	if (seconds < 0) 
		seconds = 0;
	sprintf(ret, "%3d:%02d", minutes, seconds);
	return ret;
}

BUILT_IN_COMMAND(glist)
{
int count = 0;
GetFile *sg;
time_t snow = now;
int minutes, seconds;
int do_chat;

	for (sg = transfer_struct; sg; sg = sg->next)
	{
		char buff[80];
		char buff1[80];
		char buff2[80];
		char buff3[80];
		char buff4[80];
		char ack[4];

		double perc = 0.0;
		time_t s_time;
		*buff3 = 0;		
		s_time = snow - sg->starttime;
		seconds = minutes = 0;
		do_chat = 0;
		count++;
		if (((sg->flags & NAP_CHAT) != NAP_CHAT) && ((sg->flags & NAP_CHAT_CONNECTED) != NAP_CHAT_CONNECTED))
		{
			if (sg->starttime && s_time && sg->flags)
				sprintf(buff, "%3.2f", sg->received / 1024.0 / s_time);
			else
				strcpy(buff, "N/A");
			if (sg->filesize)
				perc = (100.0 * (((double)(sg->received + sg->resume)) / (double)sg->filesize));
			sprintf(buff1, "%4.1f%%", perc);
			sprintf(buff2, "%4.2f", _GMKv(sg->filesize));
			strcpy(buff4, calc_eta(sg));
		}
		else
		{
			if ((sg->flags & NAP_DCC_COMMANDS) == NAP_DCC_COMMANDS) 
				strcpy(buff, "   cmd");
			else
				strcpy(buff, "   N/A");
			if (sg->passwd)
				strcpy(buff1, "[E]");
			else
				strcpy(buff1, "N/A");
			sprintf(buff2, "%6lu", sg->received);
			sprintf(buff4, "%7lu", sg->starttime ? now - sg->starttime : now - sg->addtime);
			do_chat = 1;
		}			
		*ack = 0;

		if (sg->flags & NAP_QUEUED)
			strcpy(ack, "Q");
		if (sg->starttime)
		{
			if ((sg->flags & NAP_DOWNLOAD) == NAP_DOWNLOAD)
				strcat(ack, "D");
			else if ((sg->flags & NAP_UPLOAD) == NAP_UPLOAD)
				strcat(ack, "U");
			else if ((sg->flags & NAP_DCC_COMMANDS) == NAP_DCC_COMMANDS)
				strcat(ack, "\002C\002");
			else
				strcat(ack, "C");
		}
		else
			strcat(ack, "W");

		if (do_hook(GLIST_LIST, "%d %s %s %s %lu %lu %s %s %s \"%s\"", count, ack, 
				sg->nick, buff2, sg->filesize,
				sg->received + sg->resume, 
				buff4, buff, buff1, 
				sg->filename ? sg->filename : sg->passwd ? sg->passwd : empty_string))
		{
			if (count == 1)
			{
				put_it(FORMAT_GLIST1);
				put_it(FORMAT_GLIST2);
			}
			put_it("%3d %s %14s %6s%s %s %s/%s %s", 
				count, ack, sg->nick, buff2, 
				do_chat ? "" : _GMKs(sg->filesize),
				buff4, buff, buff1, 
				sg->filename ? base_name(sg->filename) : sg->passwd ? sg->passwd : empty_string);
		}
	}
}

extern void napfile_read(int);
extern void nap_firewall_start(int);

void naplink_handleconnect(int snum)
{
unsigned char buff[80];
SocketList *s;
int rc;
	memset(buff, 0, sizeof(buff) - 1);
	if (!(s = get_socket(snum)))
	{
		close_socketread(snum);
		return;
	}
	switch ((rc = recv(snum, buff, 5, MSG_PEEK)))
	{
		case -1:
			say("naplink_handleconnect [snum = %d] %s", snum, strerror(errno));
			close_socketread(snum);
			return;
		case 0:
			s->flags++;
			if (s->flags > 10)
				close_socketread(snum);
			else if (s->flags == 5)
				write(snum, "1", 1);
			return;
		default:
			break;
	}

	buff[rc] = 0;
	if (rc == 1 && (buff[0] == '1' || buff[0] == '\n'))
	{
		read(snum, buff, 1);
		s->func_read = nap_firewall_start;
		nap_firewall_start(snum);
	}
	else if (!strncmp(buff, "GETL", 4))
	{
		read(snum, buff, 7);
		/* someone has requested our list of files */
		send_direct_browse(snum);
#if 0
		close_socketread(snum);
#endif
	}
	else if (!strncmp(buff, "GET", 3))
	{
	/* someone has requested a non-firewalled send */
		read(snum, buff, 3);
		set_napster_socket(snum);
		s->func_read = napfile_read;
	}
	else if (!strncmp(buff, "SENDL", 5))
	{
		/* we are firewalled, and someone has requested us to send 
		 * a filelist */
		close_socketread(snum);
	}
	else if (!strncmp(buff, "SEND", 4))
	{
	/* we have requested a file from someone who is firewalled */
		read(snum, buff, 4);
		s->func_read = nap_firewall_get;
	}
	else
		close_socketread(snum);
}

void naplink_handlelink(int snum)
{
struct  sockaddr_in     remaddr;
int sra = sizeof(struct sockaddr_in);
int sock = -1;
int rc;
	if ((sock = accept(snum, (struct sockaddr *) &remaddr, &sra)) > -1)
	{
		set_keepalive(sock);
#if HAVE_LIBWRAP
		if (!hosts_ctl (PACKAGE, STRING_UNKNOWN, inet_ntoa (remaddr.sin_addr),
			STRING_UNKNOWN))
		{
			say("tcp wrappers denied %s", inet_ntoa (rem.sin_addr));
			close(sock);
			return;
		}
#endif
		if ((rc = write(sock, "1", 1)) < 1)
		{
			close_socketread(sock); 
			return;
		}
		add_socketread(sock, snum, 0, inet_ntoa(remaddr.sin_addr), naplink_handleconnect, NULL);
	}
}

void clean_queue(int timeout)
{
GetFile *ptr, *next = NULL;
int count = 0;
NickStruct *n;
int done = 0;
	if (timeout < 1)
		goto queue_send; 
	for (ptr = transfer_struct; ptr; ptr = next)
	{
		next = ptr->next;
		if (!ptr->starttime && (now - ptr->addtime >= timeout))
		{
			/* remove from linked list */
			break_from_list((List **)&transfer_struct, (List *)ptr);
			if (ptr->socket > 0)
			{ 
				if ((ptr->flags & NAP_UPLOAD) == NAP_UPLOAD)
					send_ncommand(CMDS_UPDATE_SEND, NULL);
				else
					send_ncommand(CMDS_UPDATE_GET, NULL);
				if (ptr->write > 0)
					close(ptr->write);
				ptr->write = -1;
				nap_finished_file(ptr->socket, 0);
			}
			else
			{
				if (ptr->write > 0)
					close(ptr->write);
				ptr->write = -1;
				if (ptr->filestruct) 
				{
					ptr->filestruct->flags = 0;
					ptr->filestruct->getfile = NULL;
				}
				if (in_browser)
					refresh_browser();
				ptr->next = finished_struct;
				finished_struct = ptr;
			}
			count++;
			done++;
		} 
	}
	if (count)
		say("Cleaned queue of stale entries");
queue_send:
	if (from_server < 0)
		return;
	for (n = server_list[from_server].users; n; n = n->next)
	{
		if (!n->Queued)
			continue;
		count = files_in_progress(n->nick, NAP_DOWNLOAD);
		if (n->limit && (count < n->limit))
		{
			for (ptr = n->Queued; ptr; ptr = ptr->next)
				if (!ptr->next)
					break;
			if (ptr)
			{
				ptr = (GetFile *) break_from_list((List **)&n->Queued, (List *)ptr);
				send_ncommand(CMDS_REQUESTFILE, "%s \"%s\"", ptr->nick, ptr->filename);
				ptr->flags &= ~NAP_QUEUED;
				ptr->addtime = now;
				add_to_transfer_list(ptr);
				done++;
			}
		}
	}
	if (done)
		build_status(current_window, NULL, 0);
	return;
}

int widest_filename(const FileStruct* files) {
	int maxwidth=1;
	const FileStruct* f;
	for (f = files; f; f = f->next) 
	{
		const char* bn = base_name(f->filename);
		int len = strlen(bn) - count_trailing_spaces(bn);
		if (len > maxwidth)
			maxwidth = len;
	}
	return maxwidth;
}

int display_list(FileStruct *files)
{
	FileStruct *f;
	int count = 0, maxwidth = widest_filename(files);
	for (f = files; f; f = f->next)
		print_file(f, ++count, maxwidth);
	return count;
}

int move_file_to_unfinished(GetFile *info, char *filename)
{
char oldbuf[2 * BIG_BUFFER_SIZE+1];
char newbuf[2 * BIG_BUFFER_SIZE+1];
char *old = NULL, *new;
struct stat st;
int rc = 0;
	if (info)
	{
		FILE *fp;
		if (!find_in_list((List **)&resume_struct, info->checksum, 0))
		{
#ifdef WINNT
			sprintf(oldbuf, "~/TekNap/unfinished");
#else
			sprintf(oldbuf, "~/.TekNap/unfinished");
#endif
			new = expand_twiddle(oldbuf);
			if ((fp = fopen(new, "a+")))
			{
				fprintf(fp, "%s \"%s\" %lu %s\n", info->checksum, base_name(info->filename), info->filesize, info->nick);
				fclose(fp);
			}
			new_free(&new);
		}
	}
	if (!get_int_var(MOVE_INCOMPLETE_VAR))
		return 0;
	snprintf(oldbuf, BIG_BUFFER_SIZE*2, "%s/%s", get_string_var(DOWNLOAD_DIRECTORY_VAR), filename);
	snprintf(newbuf, BIG_BUFFER_SIZE*2, "%s/unfinished", get_string_var(DOWNLOAD_DIRECTORY_VAR));
	new = expand_twiddle(newbuf);
	rc = stat(new, &st);
	if (!rc && !(S_ISDIR(st.st_mode)))
	{
		new_free(&new);
		return -1;
	}
	else if (rc && (rc = mkdir(new, 0600)))
	{
		new_free(&new);
		return rc;
	}
	snprintf(newbuf, BIG_BUFFER_SIZE*2, "%s/%s", new, filename);
	old = expand_twiddle(oldbuf);	
	rc = -1;
	if (old && new)
		rc = rename(old, newbuf);

	new_free(&old); new_free(&new);
	return rc;
}

int read_unfinished_list(void)
{
FILE *fp;
char *new = NULL;
char oldbuf[BIG_BUFFER_SIZE+1];
ResumeFile *rf;
int count = 0;
	sprintf(oldbuf, "~/.TekNap/unfinished");
	new = expand_twiddle(oldbuf);
	if ((fp = fopen(new, "r")))
	{
		while (!feof(fp))
		{
			if ((fgets(oldbuf, BIG_BUFFER_SIZE, fp)))
			{
				char *p;
				p = &oldbuf[0];
				chop(oldbuf, 1);
				if (*oldbuf)
				{
					rf = (ResumeFile *) new_malloc(sizeof(ResumeFile));
					rf->checksum = m_strdup(next_arg(p, &p));
					if (find_in_list((List **)&resume_struct, rf->checksum, 0))
					{
						new_free(&rf->checksum);
						new_free(&rf);
						continue;
					}
					rf->filename = m_strdup(new_next_arg(p, &p));
					rf->filesize = my_atol(next_arg(p, &p));
					rf->nick = m_strdup(p);
					rf->next = resume_struct;
					resume_struct = rf;
					count++;
				}
			}
		}
		fclose(fp);
	}
	new_free(&new);
	return count;
}

int write_unfinished_list(void)
{
FILE *fp;
char *new = NULL;
char oldbuf[BIG_BUFFER_SIZE+1];
ResumeFile *rf;
int count = 0;
	if (!resume_struct)
		return 0;
	sprintf(oldbuf, "~/.TekNap/unfinished");
	new = expand_twiddle(oldbuf);
	if ((fp = fopen(new, "w")))
	{
		for (rf = resume_struct; rf; rf = rf->next, count++)
			fprintf(fp, "%s \"%s\" %lu %s\n", rf->checksum, base_name(rf->filename), rf->filesize, rf->nick);
		fclose(fp);
	}
	new_free(&new);
	return count;
}

int remove_from_resume(char *checksum)
{
ResumeFile *rf;
FileStruct *sf, *next;
	if ((rf = (ResumeFile *)find_in_list((List **)&resume_struct, checksum, 1)))
	{
		new_free(&rf->filename);
		new_free(&rf->checksum);
		new_free(&rf->nick);
		for (sf = rf->results; sf; sf = next)
		{
			if ((next = sf->next))
				next->prev = NULL;
			if (sf->getfile)
				sf->getfile->filestruct = NULL;
			new_free(&sf->filename);
			new_free(&sf->checksum);
			new_free(&sf->nick);
		}
		new_free(&rf);
		return 1;
	}
	return 0;
}

BUILT_IN_COMMAND(resume)
{
char fn[20];
ResumeFile *sf;
FileStruct *rf;
char *arg;
int count = 1;

	if (!args || !*args)
	{
		for (sf = resume_struct; sf; sf = sf->next, count++)
		{
			int i = 0;
			memset(fn, 0, sizeof(fn));
			if (count == 1)
				say("Resume file list");
			put_it("%d %lu %s", count, sf->filesize, sf->filename);
			fn[0] = 'a';
			for (rf = sf->results; rf; rf = rf->next)
			{
				put_it("\t %3s %10lu %s", fn, rf->filesize, rf->nick);
				fn[i]++;
				if (fn[i] > 'z')
				{
					fn[i] = 'a';
					i++;
					fn[i] = 'a';
				}
				if (i > 14)
					break;
			}
		}
		return;
	}
	if ((arg = next_arg(args, &args)))
	{
		int which;
		char *who;
		if ((which = my_atol(arg)))
		{
			for (sf = resume_struct; sf; sf = sf->next, count++)
				if (count == which)
					break;
			if (sf && !sf->results)
			{
				send_ncommand(CMDS_REQUESTRESUME, "%s %lu", sf->checksum, sf->filesize);
				return;
			}
			if (sf && sf->results && (who = next_arg(args, &args)))
			{
				int i = 0;
				memset(fn, 0, sizeof(fn));
				fn[0] = 'a';
				for (rf = sf->results; rf; rf = rf->next)
				{
					if (!my_stricmp(fn, who) || !my_stricmp(who, rf->nick))
					{
						break_from_list((List **)&sf->results, (List *)rf);
						break;
					}
					fn[i]++;
					if (fn[i] > 'z')
					{
						fn[i] = 'a';
						i++;
						fn[i] = 'a';
					}
					if (i > 14)
						break;
				}
				if (rf)
				{
					say("Attempting resume from %s for %s", rf->nick, base_name(rf->filename));
					create_and_do_get(rf, NAP_RESUME, 0);
					new_free(&rf->nick);
					new_free(&rf->checksum);
					new_free(&rf->filename);
					new_free(&rf);
					return;
				}
				say("There is no such resume request");
			}
		}
	}
}


#define CTCP_DELIM_CHAR '\001'

int check_dcc_msg(char *from, char *msg)
{
int delim_char = charcount(msg, CTCP_DELIM_CHAR);
char indata[BIG_BUFFER_SIZE];

	if (delim_char == 2)
	{
		if (*msg == CTCP_DELIM_CHAR && msg[strlen(msg)-1] == CTCP_DELIM_CHAR)
		{
			char *args, *cmd;
			char *nick, *ip, *file, *dir, *realfile;
			unsigned long filesize;
			int port;
			struct  sockaddr_in     remaddr;
			
			GetFile *gf;
			struct stat st = {0};

			args = LOCAL_COPY(msg);
			args++;
			args[strlen(args)-1] = 0;
			if (!(cmd = next_arg(args, &args)))
				return 0;

			nick = next_arg(args, &args);

			/* bot's-brain discovered that nick can be differant */
			if (nick && my_stricmp(nick, from))
				return 0;

			ip = next_arg(args, &args);
			remaddr.sin_addr.s_addr = my_atoul(ip);
			port = my_atol(next_arg(args, &args));
			if (port == 0 && server_list[from_server].dataport == 0)
			{
				say("Both systems are firewalled. Unable to comply");
				return 1;
			}
			if (args && *args && !strncmp(cmd, "SEND", 4))
			{
				int rc;
				file = new_next_arg(args, &args);
				filesize = my_atol(next_arg(args, &args));
				if (filesize == 0)
				{
					say("%s is sending a 0 byte %s, ignoring", from, file);
					return 1;
				}
				if (!(dir = get_string_var(DOWNLOAD_DIRECTORY_VAR)))
					dir = "~";
				snprintf(indata, sizeof(indata), "%s/%s", dir, base_name(file));
				realfile = expand_twiddle(indata);
				if (!(rc = stat(realfile, &st)))
				{
					if (st.st_size >= filesize && !get_int_var(ALLOW_DCC_OVERWRITE_VAR))
					{
						say(" %lu >= %lu size %s", st.st_size, filesize, base_name(file));
						return 1;
					}
				}
				gf = (GetFile *)new_malloc(sizeof(GetFile));
				gf->ip = m_strdup(ip);
				gf->filename = m_strdup(file);
				gf->port = port;
				gf->nick = m_strdup(from);
				gf->realfile = realfile;
				gf->filesize = filesize;			
				gf->checksum = m_strdup("checksum");
				gf->socket = -1;
				gf->addtime = now;
				gf->flags = NAP_DOWNLOAD;
							
				if (get_int_var(ALLOW_DCC_OVERWRITE_VAR))
					gf->resume = 0;
				else if ((get_int_var(RESUME_DOWNLOAD_VAR) || (gf->flags & NAP_RESUME)))
					gf->resume = st.st_size;
				gf->write = -1;
				if (!get_int_var(ALLOW_DCC_VAR) || (get_int_var(ALLOW_DCC_VAR) && get_int_var(ALLOW_DCC_OVERWRITE_VAR)))
				{
					if (get_int_var(ALLOW_DCC_VAR) && get_int_var(ALLOW_DCC_OVERWRITE_VAR))
						say("%s is attempting to send %s/%lu to you. We already have this file. /dccget %s [%s:%d]", from, file, filesize, from, inet_ntoa(remaddr.sin_addr), gf->port);
					else
						say("%s is attempting to send %s/%lu to you. /set allow_dcc on or /dccget %s [%s:%d]", from, file, filesize, from, inet_ntoa(remaddr.sin_addr), gf->port);
					add_to_transfer_list(gf);
					return 1;
				}
				if (!(create_and_get(port, gf)))
					say("Unable to connect to %s's ip/port[%s:%d]", from, inet_ntoa(remaddr.sin_addr), port);
				else
					say("Connection to %s ip/port[%s:%d]", from, inet_ntoa(remaddr.sin_addr), port);
				return 1;
			}
			else if (!strncmp(cmd, "CHAT", 4))/* can only be a chat request at this point */
			{
				gf = (GetFile *)new_malloc(sizeof(GetFile));
				gf->port = port;
				gf->nick = m_strdup(from);
				gf->ip = m_strdup(ip);
				gf->socket = -1;
				gf->addtime = now;
				gf->flags = NAP_CHAT;
				if (args && *args)
					gf->passwd = m_strdup(args);
				say("%s is attempting to %schat with you. /chat %s [%s:%d]",
					from, gf->passwd ? "encrypted " : empty_string,
					from, inet_ntoa(remaddr.sin_addr), port);
				add_to_transfer_list(gf);
				return 1;
			}
		}
	}
	return 0;
}


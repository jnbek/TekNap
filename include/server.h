 /* $Id: server.h,v 1.1.1.1 2001/01/19 15:53:12 edwards Exp $ */
 
#ifndef _server_h
#define _server_h

int 	find_server_refnum (char *, char **);
int	build_server_list (char *);
void	display_server_list(void);
void	do_server(fd_set *, fd_set *);
void	set_server_bits(fd_set *, fd_set *);
void	server_disconnect(Window *, char *);
void	close_all_servers(void);
void	close_server(int);
void	close_data(int);
int	check_server_connect(int);
void	send_all_servers(int, char *, ...);
void	remove_from_server_list(int);

int	get_server_dataport(int);
char	*get_server_nickname(int);
char	*get_server_password(int);
char	*get_server_name(int);
char	*get_server_itsname(int);

void	set_server_nickname(int, char *);
void	set_server_password(int, char *);
void	set_server_itsname(int, char *);
void	set_server_name(int, char *);
void	set_server_dataport(int, int);
int	get_server_search(int);
void	set_server_search(int, int);
int	is_connected(int);
int	get_nap_socket(int);
int	server_list_size(void);
int	get_server_resume(int);
void	set_server_resume(int, int);
N_STATS	*get_server_stats(int);
void	clear_sent_to_server(int);
int	sent_to_server(int);
int	server_waiting_in(int);
int	server_waiting_out(int);
void	inc_server_waiting_out(int);
void	inc_server_waiting_in(int);
int	get_server_cloak(int);
void	set_server_cloak(int, int);
void	set_server_lag(int, struct timeval);
struct  timeval get_server_lag(int);
void	set_server_sping(int, int);
int	get_server_sping(int);
int	get_server_version(int);
void	set_server_version(int, int);
int	get_server_admin(int);
void	set_server_admin(int, int);

int	send_ncommand		(unsigned int, char *, ...);

int	connect_to_server_by_refnum(int, int, int);

int	check_socket		(int);
unsigned long	set_socketflags	(int, unsigned long);
unsigned long	get_socketflags	(int);
char	*get_socketserver	(int);
void	*get_socketinfo		(int);
void	set_socketinfo		(int, void *);
int	get_max_fd		(void);
void	set_socket_read		(fd_set *, fd_set *);

int	add_socketread		(int, int, unsigned long, char *, void (*func_read)(int), void (*func_write)(int));
int	set_socketwrite		(int);
void	add_sockettimeout	(int, time_t, void *);
void	close_socketread	(int);
int	read_sockets		(int, unsigned char *, int);
void	scan_sockets		(fd_set *, fd_set *);
void	close_all_sockets	(void);
SocketList	*get_socket	(int);


void	naplink_handler		(int);
int	naplink_getserver	(char *, u_short, int, void *);
int	connect_to_server	(char *, char *, char *, int, int, int);
void	naplink_handlelink	(int);

extern int from_server;

#define NAP_send(s, i) write(server_list[from_server].read, s, i);


extern Stats shared_stats;

void	addtabkey		(char *, char *, char *);
Msgs	*gettabkey		(char *, int);
void	clear_servermsg		(void);
void	display_servermsgs	(void);


GetFile	*find_in_queue		(int, char *, char *, char *, unsigned long);
int	files_in_sendqueue	(char *, char *);

char	*get_server_showuser(int, unsigned int **);
void	set_server_showuser(int, char *, int);
void	set_server_showusercmd(int, char *);
char	*get_server_showusercmd(int);
int	get_icmp_socket(int);
void	set_icmp_socket(int, int);
void	icmp_sockets(fd_set *, fd_set *);
void	check_icmpresult(fd_set *, fd_set *);
FileStruct **get_search_head(int server);
void	add_ping(int, struct sockaddr_in *);
void	login_to_ircserver(int);
int	connect_to_irc_server(int);
int	get_server_ircmode(int);
int	set_server_ircmode(int, int);

/* list cmd */
void	set_server_listmode(int, char *);
char	*get_server_listmode(int);

void	save_servers(void);
int	read_server_list(void);

#endif

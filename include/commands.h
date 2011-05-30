
#ifndef _command_h
#define _command_h
 /* $Id: commands.h,v 1.1.1.1 2001/01/17 21:18:33 edwards Exp $ */
 
BUILT_IN_COMMAND(napquit);
BUILT_IN_COMMAND(window_serv);
BUILT_IN_COMMAND(windowcmd);
BUILT_IN_COMMAND(setcmd);
BUILT_IN_COMMAND(do_send_text);
BUILT_IN_COMMAND(savenap);
BUILT_IN_COMMAND(reload_save);
BUILT_IN_COMMAND(load);
BUILT_IN_COMMAND(joincmd);
BUILT_IN_COMMAND(partcmd);
BUILT_IN_COMMAND(lastlog);
BUILT_IN_COMMAND(privmsg);
BUILT_IN_COMMAND(ignore_user);
BUILT_IN_COMMAND(nap_scan);
BUILT_IN_COMMAND(nap_search);
BUILT_IN_COMMAND(browse);
BUILT_IN_COMMAND(whois);
BUILT_IN_COMMAND(list);
BUILT_IN_COMMAND(serverclose);
BUILT_IN_COMMAND(hotlist);
BUILT_IN_COMMAND(topic);
BUILT_IN_COMMAND(ping);
BUILT_IN_COMMAND(rawcmd);
BUILT_IN_COMMAND(nadmin);
BUILT_IN_COMMAND(request);
BUILT_IN_COMMAND(glist);
BUILT_IN_COMMAND(nap_del);
BUILT_IN_COMMAND(print_napster);
BUILT_IN_COMMAND(share_napster);

#define MAX_LOAD_DEPTH 5

int	check_nignore(char *);
int	check_cignore(char *);
int	check_server_ignore (char *);
void	load_scripts(void);
void	parse_line(const char *, char *, const char *, int, int, int);
void	redirect_text(int, const char *, const char *, int, int);
int	check_wait_command(char *);
void	save_ignore(FILE *);

/* Used to handle and catch breaks and continues */
extern	int     will_catch_break_exceptions;
extern	int     will_catch_continue_exceptions;
extern	int     will_catch_return_exceptions;
extern	int     break_exception;
extern	int     continue_exception;
extern	int     return_exception;


#endif

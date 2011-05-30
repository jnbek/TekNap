/*
 * lastlog.h: header for lastlog.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: lastlog.h,v 1.1.1.1 2000/09/29 09:18:39 edwards Exp $
 */

#ifndef __lastlog_h_
#define __lastlog_h_

#define LOG_NONE	0x00000000
#define LOG_CURRENT	0x00000000
#define LOG_CRAP	0x00000001
#define LOG_PUBLIC	0x00000002
#define LOG_MSG		0x00000004
#define LOG_WALLOP	0x00000008
#define	LOG_USER1	0x00000010
#define LOG_USER2	0x00000020
#define LOG_USER3	0x00000040
#define LOG_USER4	0x00000080
#define LOG_USER5	0x00000100
#define LOG_BEEP	0x00000200
#define LOG_SEND_MSG	0x00000400
#define LOG_KILL	0x00000800
#define LOG_PART	0x00001000
#define LOG_JOIN	0x00002000
#define LOG_TOPIC	0x00004000
#define LOG_NOTIFY	0x00008000
#define LOG_SERVER	0x00010000
#define LOG_DEBUG	0x00020000

#define LOG_ALL (LOG_CRAP | LOG_PUBLIC | LOG_MSG | LOG_WALLOP | LOG_USER1 |\
		LOG_USER2 | LOG_USER3 | LOG_USER4 | LOG_USER5 | LOG_BEEP |\
		LOG_SEND_MSG | LOG_PART | LOG_JOIN | LOG_TOPIC | \
		LOG_KILL | LOG_NOTIFY | LOG_SERVER)

# define LOG_DEFAULT	LOG_NONE

	void	set_lastlog_level (Window *, char *, int);
unsigned long	set_lastlog_msg_level (unsigned long);
	void	set_lastlog_size (Window *, char *, int);
	void	set_notify_level (Window *, char *, int);
	void	set_msglog_level (Window *, char *, int);
	void	set_new_server_lastlog_level(Window *, char *, int);
	
	BUILT_IN_COMMAND(lastlog);

	void	add_to_lastlog (Window *, const char *);
	char	*bits_to_lastlog_level (unsigned long);
unsigned long	real_lastlog_level (void);
unsigned long	real_notify_level (void);
unsigned long	parse_lastlog_level (char *, int);
	int	islogged (Window *);
extern	void	remove_from_lastlog (Window *);

extern unsigned long beep_on_level;
extern unsigned long new_server_lastlog_level;

	void	set_beep_on_msg(Window *, char *, int);
	Lastlog *get_lastlog_current_head(Window *);
	void	free_lastlog(Window *);
	int	logmsg(unsigned long, char *, int, char *, ...);
	void	reset_hold_mode(Window *);
	void	free_lastlog(Window *);
			  				
#endif /* __lastlog_h_ */

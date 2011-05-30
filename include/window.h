/*
 * window.h: header file for window.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: window.h,v 1.1.1.1 2000/07/16 12:58:54 edwards Exp $
 */
 /* $Id: window.h,v 1.1.1.1 2000/07/16 12:58:54 edwards Exp $ */
 
#ifndef __window_h_
#define __window_h_

#include "irc_std.h"
#include "lastlog.h"

/* used by the update flag to determine what needs updating */
#define REDRAW_DISPLAY_FULL 1
#define REDRAW_DISPLAY_FAST 2
#define UPDATE_STATUS 4
#define REDRAW_STATUS 8

#define	LT_UNLOGGED	0
#define	LT_LOGHEAD	1
#define	LT_LOGTAIL	2

/* var_settings indexes */
#define OFF 0
#define ON 1
#define TOGGLE 2

	Window 	*new_window 			(struct ScreenStru *);
	void	delete_window			(Window *);
	void	add_to_invisible_list		(Window *);
	Window	*add_to_window_list		(struct ScreenStru *, Window *);
	void	remove_from_window_from_screen	(Window *);
	void	recalculate_window_positions	(struct ScreenStru *);
	void	redraw_all_windows		(void);
	void	recalculate_windows		(struct ScreenStru *);
	void	rebalance_windows		(struct ScreenStru *);
	void	update_all_windows		(void);
	void	set_current_window		(Window *);
	void	hide_window			(Window *);
	void	swap_last_window		(char, char *);
	void	next_window			(char, char *);
	void	swap_next_window		(char, char *);
	void	previous_window			(char, char *);
	void	swap_previous_window		(char, char *);
	void	back_window			(char, char *);
	Window 	*get_window_by_refnum		(unsigned);
	Window	*get_window_by_name		(const char *);
	char	*get_refnum_by_window		(const Window *);
	int	is_window_visible		(char *);
	void	update_window_status		(Window *, int);
	void	update_all_status		(Window *, char *, int);
	void	set_prompt_by_refnum		(unsigned, char *);
	char 	*get_prompt_by_refnum		(unsigned);
	char	*get_target_by_refnum		(unsigned);
const	char	*query_nick			(void);
	void	set_query_nick			(char *, char *, char *);
	int	is_current_channel		(char *, int, int);
const	char	*set_current_channel_by_refnum		(unsigned, char *);
	char	*get_current_channel_by_refnum		(unsigned);
	int	is_bound_to_window		(const Window *, const char *);
	Window	*get_window_bound_channel	(const char *, const int);
	int	is_bound_anywhere		(const char *);
	int	is_bound			(const char *, int);
	void    unbind_channel 			(const char *, int);
	char	*get_bound_channel		(Window *);
	int	get_window_server		(unsigned);
	void	set_window_server		(int, int, int);
	void	window_check_servers		(void);
	void	set_level_by_refnum		(unsigned, unsigned long);
	void	message_to			(unsigned long);

#if 0
	void	save_message_from		(char **, unsigned long *);
	void	restore_message_from		(char *, unsigned long);
	void	message_from			(char *, unsigned long);
	int	message_from_level		(unsigned long);
#endif
	void	set_display_target		(const char *, unsigned long);
	void	set_display_target_by_winref	(unsigned int);
	void	set_display_target_by_desc	(char *);
	void	reset_display_target		(void);
	void	save_display_target		(const char **, unsigned long *);
	void	restore_display_target		(const char *, unsigned long);


	void	clear_all_windows		(int, int);
	void	clear_window_by_refnum		(unsigned);
	void	set_scroll			(Window *, char *, int);
	void	set_scroll_lines		(Window *, char *, int);
	void	set_continued_lines		(Window *, char *, int);
	unsigned current_refnum			(void);
	int	number_of_windows_on_screen	(Window *);
	void	delete_display_line		(Display *);
	Display *new_display_line		(Display *);
	void	scrollback_backwards_lines	(int);
	void	scrollback_forwards_lines	(int);
	void	scrollback_backwards		(char, char *);
	void	scrollback_forwards		(char, char *);
	void	scrollback_end			(char, char *);
	void	scrollback_start		(char, char *);
	void	hold_mode			(Window *, int, int);
	void	unstop_all_windows		(char, char *);
	void	reset_line_cnt			(Window *, char *, int);
	void	toggle_stop_screen		(char, char *);
	void	flush_everything_being_held	(Window *);
	int	unhold_a_window			(Window *);
	char *	get_target_cmd_by_refnum	(u_int);
	void	recalculate_window_cursor	(Window *);
	int	is_window_name_unique		(char *);
	int	get_visible_by_refnum		(char *);
	void	resize_window			(int, Window *, int);
	Window *window_list			(Window *, char **, char *);
	void	move_window			(Window *, int);
	void	show_window			(Window *);
	int	traverse_all_windows		(Window **);
	Window	*get_window_by_desc		(const char *);
	char	*get_nicklist_by_window		(Window *); /* XXX */
	void	set_scrollback_size		(Window *, char *, int);
	void	make_window_current		(Window *);
	Window	*window_query			(Window *, char **, char *);
	int	unhold_windows			(void);
	void	free_window			(Window *);
	Window	*get_window_target_by_desc	(char *);
	BUILT_IN_COMMAND(windowcmd);

	char *	get_status_by_refnum (unsigned , unsigned);
	void	unclear_window_by_refnum(unsigned);
	void	set_screens_current_window(Screen *, Window *);
	void	clear_scrollback(Window *);
	void 	clear_window (Window *window);
	void	repaint_window(Window *, int, int);
	void	remove_window_from_screen(Window *);
	void	set_screens_current_window (Screen *, Window *);
	void	make_window_current (Window *);
	void	make_window_current_by_refnum(int);
	void	free_formats (Window *);
	void	goto_window (Screen *, int);
	void	update_window_status_all (void);
const	char	*query_host (void);
const	char	*query_cmd (void);
	void	window_check_servers(void);
	void	window_change_server(int, int);
	void	make_window_current_by_winref(int);
	void	make_window_current_by_desc(char *);
	int	get_winref_by_desc(const char *);
	int	get_current_winref(void);
	void	make_to_window_by_desc(char *);
	void	win_create(int, int);
			
	void	change_window_server(int, int);	
	void	move_window_channels(Window *);
	void	add_waiting_channel(Window *, char *);
	void	delete_all_windows(void);
		
unsigned long	message_from_level(unsigned long);
						
extern	Window	*invisible_list;
extern	unsigned long	who_level;
extern	int	in_window_command;
extern	unsigned int	window_display;
extern	Window	*target_window;
extern	Window	*current_window;
extern	void	*default_output_function;
extern	int	status_update_flag;


#define BUILT_IN_WINDOW(x) Window *x (Window *window, char **args, char *usage) 

BUILT_IN_WINDOW(window_server);
BUILT_IN_WINDOW(window_discon);

#define WINDOW_NOTIFY	((unsigned) 0x0001)
#define WINDOW_NOTIFIED	((unsigned) 0x0002)

#endif /* __window_h_ */

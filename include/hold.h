/*
 * hold.h: header for hold.c
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: hold.h,v 1.1.1.1 2000/06/22 09:34:39 edwards Exp $
 */

#ifndef __hold_h_
#define __hold_h_

	void	remove_from_hold_list (Window *);
	void	add_to_hold_list (Window *, char *, int);
	void	hold_mode (Window *, int, int);
	int	hold_output (Window *);
	char	*hold_queue (Window *);
	void	reset_hold (Window *);
	int	hold_queue_logged (Window *);
	void	toggle_stop_screen (char, char *);

#endif /* __hold_h_ */

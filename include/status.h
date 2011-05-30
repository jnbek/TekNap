/*
 * status.h: header for status.c
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: status.h,v 1.1.1.1 2000/09/30 22:27:04 edwards Exp $
 */

#ifndef __status_h_
#define __status_h_

	void	make_status (Window *);
	void	set_alarm (Window *, char *, int);
	char	*update_clock (int);
	void	reset_clock (Window *, char *, int);
	void	build_status (Window *, char *, int);
	int	status_update (int);
	void	rebuild_a_status(Window *);
	
#define GET_TIME 1
#define RESET_TIME 2

extern	Status	main_status;

#endif /* __status_h_ */

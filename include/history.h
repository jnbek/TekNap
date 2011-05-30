/*
 * history.h: header for history.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: history.h,v 1.1.1.1 2001/01/17 21:18:33 edwards Exp $
 */

#ifndef __history_h_
#define __history_h_

	void	set_history_size (Window *, char *, int);
	void	add_to_history (char *);
	char	*get_from_history (int);
	char	*do_history (char *, char *);
BUILT_IN_COMMAND(history);
	void	shove_to_history (char, char *);
	void	abort_history_browsing(int);
		
/* used by get_from_history */
#define NEXT 0
#define PREV 1

#endif /* __history_h_ */

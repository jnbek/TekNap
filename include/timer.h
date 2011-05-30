 /* $Id: timer.h,v 1.1.1.1 2000/06/22 09:35:07 edwards Exp $ */
 
/*
 * timer.h: header for timer.c 
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 */

#ifndef _TIMER_H_
#define _TIMER_H_

/* functions that may be called by others */
BUILT_IN_COMMAND(timercmd);
extern	void	ExecuteTimers (void);
extern	char	*add_timer (int, char *, double, long, int (*) (void *, char *), char *, char *, int, char *);
extern	int	delete_timer (char *);
extern	int	get_delete_timer(char *);
extern	int	kill_timer(char *);
extern	void	delete_all_timers (void);
extern	int	timer_exists (char *ref);


extern	time_t	TimerTimeout (void);
int		timer_callback_exists(void *);

#define MAGIC_TIMEOUT 100000000

#endif /* _TIMER_H_ */

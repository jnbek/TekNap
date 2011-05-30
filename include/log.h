/*
 * log.h: header for log.c 
 */
 /* $Id: log.h,v 1.1.1.1 2001/01/31 17:53:22 edwards Exp $ */
 

#ifndef __log_h_
#define __log_h_

	void	do_log (int, char *, FILE **);
	void	logger (Window *, char *, int);
	void	set_log_file (Window *, char *, int);
	void	add_to_log (FILE *, unsigned int, const char *, int mangler);

extern	FILE	*naplog_fp;

#endif /* __log_h_ */

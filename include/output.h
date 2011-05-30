/*
 * output.h: header for output.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: output.h,v 1.1.1.1 2000/10/16 01:09:14 edwards Exp $
 */

#ifndef __output_h_
#define __output_h_

	void	put_echo (char *);
	void	put_it (const char *, ...);

	void	send_to_server (const char *, ...);
	void	my_send_to_server (int, const char *, ...);
	void	queue_send_to_server (int, const char *, ...);

	void	say (const char *, ...);
	void	bitchsay (const char *, ...);
	void	serversay (int, int, const char *, ...);
	void	yell (const char *, ...);
	void	naperror (const char *, ...);
	
	void	refresh_screen (unsigned char, char *);
	int	init_screen (void);
	void	put_file (char *);

	void	charset_ibmpc (void);
	void	charset_lat1 (void);
	void	charset_graf (void);
	void	charset_cst(void);
	
#endif /* __output_h_ */

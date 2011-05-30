 /* $Id: newio.h,v 1.1.1.1 2000/12/24 20:37:05 edwards Exp $ */
 
/*
 * newio.h -- header file for newio.c
 *
 * Copyright 1990, 1995 Michael Sandrof, Matthew Green
 * Copyright 1997 EPIC Software Labs
 */

#ifndef __newio_h__
#define __newio_h__

extern 	int 	dgets_errno;

	int 	dgets 			(char *, int, int, int);
	int 	new_dgets 		(char *, int, int, int);
	int 	new_select 		(fd_set *, fd_set *, struct timeval *);
	int	new_open		(int);
	int 	new_close 		(int);
	int	new_close_write		(int);
	int	new_open_write		(int);
	void 	set_socket_options 	(int);
	void	set_keepalive		(int);
	size_t	get_pending_bytes	(int);
	N_DATA	*dget_data		(int, N_DATA *, int);
	void	dget_clear_data		(int);
	
#define IO_BUFFER_SIZE (8192 * 4)

#endif

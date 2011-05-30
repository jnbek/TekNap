/*
 * files.h -- header file for files.c
 *
 * Direct file manipulation for irc?  Unheard of!
 * (C) 1995 Jeremy Nelson
 * See the COPYRIGHT file for copyright information
 *
 */
 /* $Id: files.h,v 1.1.1.1 2000/09/04 23:28:17 edwards Exp $ */
 
#ifndef FILES_H
#define FILES_H

#include "irc_std.h"

extern	int	open_file_for_read (char *, int);
extern	int	open_file_for_write (char *, int);
extern	int	open_internet_socket(char *, int, int);
extern	int	file_write (int, char *, int);
extern	char *	file_read (int);
extern	int	file_eof (int);
extern	int	file_close (int);
extern	int	file_valid (int);

#endif

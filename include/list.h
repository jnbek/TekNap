/*
 * list.h: header for list.c 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 *
 * @(#)$Id: list.h,v 1.1.1.1 2000/06/22 09:34:54 edwards Exp $
 */

#ifndef __list_h_
#define __list_h_

	void	add_to_list (List **, List *);
	void	add_to_list_double (List **, List *);
	List	*find_in_list (List **, char *, int);
	List	*remove_from_list (List **, char *);
	List	*remove_from_list_double (List **, char *);
	List	*removewild_from_list (List **, char *);
	List	*list_lookup_ext (List **, char *, int, int, int (*)(List *, char *));
	List	*list_lookup (List **, char *, int, int);
	List	*remove_from_list_ext (List **, char *, int (*)(List *, char *));
	void	add_to_list_ext (List **, List *, int, int (*)(List *, List *));
	List	*find_in_list_ext (List **, char *, int, int (*)(List *, char *));
	int	list_strnicmp (List *, char *);
	int	add_list_strcmp(List *, List *);
	int	add_list_stricmp(List *, List *);
	List	*break_from_list(List **, List *);

#define REMOVE_FROM_LIST 1
#define USE_WILDCARDS 1

#endif /* __list_h_ */

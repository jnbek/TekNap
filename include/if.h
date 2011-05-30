/*
 * if.h: header for if.c
 *
 * copyright(c) 1994 matthew green
 *
 * See the copyright file, or do a help ircii copyright 
 *
 * @(#)$Id: if.h,v 1.1.1.1 2000/06/22 09:34:40 edwards Exp $
 */

#ifndef __if_h
# define __if_h

	BUILT_IN_COMMAND(ifcmd);
	BUILT_IN_COMMAND(whilecmd);
	BUILT_IN_COMMAND(foreach);
	BUILT_IN_COMMAND(fe);
	BUILT_IN_COMMAND(forcmd);
	BUILT_IN_COMMAND(fec);
	BUILT_IN_COMMAND(docmd);
	BUILT_IN_COMMAND(switchcmd);
	BUILT_IN_COMMAND(repeatcmd);
	
extern	char *  next_expr       	(char **, char);
extern	char *  next_expr_failok        (char **, char);
  		
#endif /* __if_h */

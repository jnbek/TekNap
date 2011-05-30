/*
 * stack.h - header for stack.c
 *
 * Copyright 1993 Matthew Green
 * Copyright 1997 EPIC Software Labs
 * Copyright 2000 TekNap
 * See the COPYRIGHT file for license information.
 * $Id: stack.h,v 1.1.1.1 2000/07/18 15:50:20 edwards Exp $
 */

#ifndef __stack_h__
#define __stack_h__

#define STACK_POP 	0
#define STACK_PUSH 	1
#define STACK_SWAP 	2
#define STACK_LIST 	3

#define STACK_DO_ALIAS	0x0001
#define STACK_DO_ASSIGN	0x0002

void do_stack_set(int, char *);
void do_stack_alias(int, char *, int);
void do_stack_on(int, char *);


#endif /* __stack_h_ */

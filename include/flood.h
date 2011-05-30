 /* $Id: flood.h,v 1.1.1.1 2000/06/22 09:34:35 edwards Exp $ */
 
/*
 * flood.h: header file for flood.c
 *
 * Copyright 1991 Tomi Ollila
 * Copyright 1997 EPIC Software Labs
 * See the Copyright file for license information
 */

#ifndef __flood_h__
#define __flood_h__

typedef enum {
	JOIN_FLOOD,
	MSG_FLOOD,
	PUBLIC_FLOOD,
	NUMBER_OF_FLOODS
} FloodType;

	int	check_flooding 	(char *, char *, char *, FloodType);

#endif /* _FLOOD_H_ */

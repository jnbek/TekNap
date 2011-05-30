/*
 * alist.c -- resizeable arrays.
 * Written by Jeremy Nelson
 * Copyright 1997 EPIC Software Labs
 *
 * This file presumes a good deal of chicanery.  Specifically, it assumes
 * that your compiler will allocate disparate structures congruently as 
 * long as the members match as to their type and location.  This is
 * critically important for how this code works, and all hell will break
 * loose if your compiler doesnt do this.  Every compiler i know of does
 * it, which is why im assuming it, even though im not allowed to assume it.
 *
 * This file is hideous.  Ill kill each and every one of you who made
 * me do this. ;-)
 * $Id: alist.c,v 1.1.1.1 2000/06/22 09:35:35 edwards Exp $
 */

#define _cs_alist_hash_
#define _ci_alist_hash_
#include "alist.h"
#include "ircaux.h"
#include "output.h"


u_32int_t	bin_ints = 0;
u_32int_t	lin_ints = 0;
u_32int_t	bin_chars = 0;
u_32int_t	lin_chars = 0;
u_32int_t	alist_searches = 0;
u_32int_t	char_searches = 0;
 
static void check_array_size (Array *list);
void move_array_items (Array *list, int start, int end, int dir);

Array_item *add_to_array (Array *array, Array_item *item)
{
	int count;
	int location = 0;
	Array_item *ret = (void *) 0 ;
	u_32int_t       mask;    

	if (array->hash == HASH_INSENSITIVE)
		item->hash = ci_alist_hash(item->name, &mask);
	else
		item->hash = cs_alist_hash(item->name, &mask);

	check_array_size(array);
	if (array->max)
	{
		find_array_item(array, item->name, &count, &location);
		if (count < 0)
		{
			ret = ((Array_item *) (( array ) -> list [ (  location ) ])) ;
			array->max--;
		}
		else
			move_array_items(array, location, array->max, 1);
	}

	array->list[location] = item;
	array->max++;
	return ret;
}

 


Array_item *remove_from_array (Array *array, char *name)
{
	int count, location = 0;

	if (array->max)
	{
		find_array_item(array, name, &count, &location);
		if (count >= 0)
			return (void *) 0 ;

		return array_pop(array, location);
	}
	return (void *) 0 ;	 
}

 
Array_item *array_pop (Array *array, int which)
{
	Array_item *ret = (void *) 0 ;

	if (which < 0 || which >= array->max)
		return (void *) 0 ;

	ret = ((Array_item *) (( array ) -> list [ (  which ) ])) ;
	move_array_items(array, which + 1, array->max, -1);
	array->max--;
	return ret;
}
  
 


Array_item *remove_all_from_array (Array *array, char *name)
{
	int count, location = 0;
	Array_item *ret = (void *) 0 ;

	if (array->max)
	{
		find_array_item(array, name, &count, &location);
		if (count == 0)
			return (void *) 0 ;
		ret = ((Array_item *) (( array ) -> list [ (  location ) ])) ;
		move_array_items(array, location + 1, array->max, -1);
		array->max--;
		return ret;
	}
	return (void *) 0 ;	 
}

Array_item *array_lookup (Array *array, char *name, int wild, int delete)
{
	int count, location;

	if (delete)
		return remove_from_array(array, name);
	else
		return find_array_item(array, name, &count, &location);
}

static void check_array_size (Array *array)
{
	if (array->total_max == 0)
		array->total_max = 6;		 
	else if (array->max == array->total_max-1)
		array->total_max *= 2;
	else if (array->max * 3 < array->total_max)
		array->total_max /= 2;
	else
		return;
	n_realloc     ((void **)& ( array->list ), sizeof(  Array_item * ) * (  array->total_max ), __FILE__, __LINE__) ;
}

 




void move_array_items (Array *array, int start, int end, int dir)
{
	int i;

	if (dir > 0)
	{
		for (i = end; i >= start; i--)
			((( array ) -> list [ (  i + dir ) ]))  = ((Array_item *) (( array ) -> list [ (  i ) ])) ;
		for (i = dir; i > 0; i--)
			((( array ) -> list [ (  start + i - 1 ) ]))  = (void *) 0 ;
	}
	else if (dir < 0)
	{
		for (i = start; i <= end; i++)
			((( array ) -> list [ (  i + dir ) ]))  = ((Array_item *) (( array ) -> list [ (  i ) ])) ;
		for (i = end - dir + 1; i <= end; i++)
			((( array ) -> list [ (  i ) ]))  = (void *) 0 ;
	}
}

Array_item *find_array_item (Array *set, char *name, int *cnt, int *loc)
{
	size_t	len = strlen(name);
	int	c = 0, 
		pos = 0, 
		min, 
		max;
	u_32int_t mask, hash;

	if (set->hash == HASH_INSENSITIVE)
		hash = ci_alist_hash(name, &mask);
	else
		hash = cs_alist_hash(name, &mask);
	
	*cnt = 0;
	if (!set->list || !set->max)
	{
		*loc = 0;
		return (void *) 0 ;
	}

	alist_searches++;
	max = set->max - 1;
	min = 0;
	
	while (max >= min)
	{
		bin_ints++;
		pos = (max - min) / 2 + min;
		c = (hash & mask) - (((Array_item *) (( set ) -> list [ (  pos ) ])) ->hash & mask);
		if (c == 0)
			break;
		else if (c < 0)
			max = pos - 1;
		else
			min = pos + 1;
	}

	if (c != 0)
	{
		if (c > 0)
			*loc = pos + 1;
		else
			*loc = pos;
		return (void *) 0 ;
	}

	min = max = pos;
	while ((min > 0) && (hash & mask) == (((Array_item *) (( set ) -> list [ (  min ) ])) ->hash & mask))
		min--, lin_ints++;
	while ((max < set->max - 1) && (hash &mask) == (((Array_item *) (( set ) -> list [ (  max ) ])) ->hash & mask))
		max++, lin_ints++;

	char_searches++;

	while (max >= min)
	{
		bin_chars++;
		pos = (max - min) / 2 + min;
		c = set->func(name, ((Array_item *) (( set ) -> list [ (  pos ) ])) ->name, len);
		if (c == 0)
			break;
		else if (c < 0)
			max = pos - 1;
		else
			min = pos + 1;
	}


	if (c != 0)
	{
		if (c > 0)
			*loc = pos + 1;
		else
			*loc = pos;
		return (void *) 0 ;
	}
	*cnt = 1;
	min = pos - 1;
	while (min >= 0 && !set->func(name, ((Array_item *) (( set ) -> list [ (  min ) ])) ->name, len))
		(*cnt)++, min--, lin_chars++;
	min++;
	max = pos + 1;
	while (max < set->max && !set->func(name, ((Array_item *) (( set ) -> list [ (  max ) ])) ->name, len))
		(*cnt)++, max++, lin_chars++;

	if (strlen(((Array_item *) (( set ) -> list [ (  min ) ])) ->name) == len)
		*cnt *= -1;

	if (loc)
		*loc = min;

	return ((Array_item *) (( set ) -> list [ (  min ) ])) ;
}

void * find_fixed_array_item (void *list, size_t size, int howmany, char *name, int *cnt, int *loc)
{
	int	len = strlen(name),
		min = 0,
		max = howmany,
		old_pos = -1,
		pos,
		c;

	*cnt = 0;

	while (1)
	{
		pos = (max + min) / 2;
		if (pos == old_pos)
		{
			*loc = pos;
			return (void *) 0 ;
		}
		old_pos = pos;

		c = strncmp(name, (*(Array_item *) ( list  + (   pos  *   size  ))) .name, len);
		if (c == 0)
			break;
		else if (c > 0)
			min = pos;
		else
			max = pos;
	}
	*cnt = 1;

	min = pos - 1;
	while (min >= 0 && !strncmp(name, (*(Array_item *) ( list  + (   min  *   size  ))) .name, len))
		(*cnt)++, min--;
	min++;

	max = pos + 1;
	while ((max < howmany) && !strncmp(name, (*(Array_item *) ( list  + (   max  *   size  ))) .name, len))
		(*cnt)++, max++;

	if (strlen((*(Array_item *) ( list  + (   min  *   size  ))) .name) == len)
		*cnt *= -1;
	if (loc)
		*loc = min;
	return (void *)& (*(Array_item *) ( list  + (   min  *   size  ))) ;
}


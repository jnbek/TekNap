/*
 * ircaux.c: some extra routines... not specific to irc... that I needed 
 *
 * Written By Michael Sandrof
 *
 * Copyright(c) 1990, 1991 
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 * $Id: ircaux.c,v 1.1.1.1 2001/01/17 21:19:42 edwards Exp $
 */
#include "teknap.h"
#include "struct.h"

#include "vars.h"
#include "screen.h"

#include <pwd.h>

#include <sys/stat.h>

#include "ircaux.h"
#include "output.h"
#include "ircterm.h"

#ifndef MAXPATHLEN
#define MAXPATHLEN PATHLEN
#endif

#ifndef MAX_INT
#define MAX_INT 32767
#endif

/*
 * These are used by the malloc routines.  We actually ask for an int-size
 * more of memory, and in that extra int we store the malloc size.  Very
 * handy for debugging and other hackeries.
 */


/*
 * m_strcat_ues: Given two strings, concatenate the 2nd string to
 * the end of the first one, but if the "unescape" argument is 1, do
 * unescaping (like in strmcat_ue).
 * (Malloc_STRCAT_UnEscape Special, in case you were wondering. ;-))
 *
 * This uses a cheating, "not-as-efficient-as-possible" algorithm,
 * but it works with negligible cpu lossage.
 */
char *	n_m_strcat_ues(char **dest, char *src, int unescape, const char *file, const int line)
{
	int total_length;
	char *ptr, *ptr2;
	int z;

	if (!unescape)
	{
		n_malloc_strcat(dest, src, file, line);
		return *dest;
	}

	z = total_length = (*dest) ? strlen(*dest) : 0;
	total_length += strlen(src);

	n_realloc((void **)dest, sizeof(char) * (total_length + 2), file,  line);
	if (z == 0)
		**dest = 0;

	ptr2 = *dest + z;
	for (ptr = src; *ptr; ptr++)
	{
		if (*ptr == '\\')
		{
			switch (*++ptr)
			{
				case 'n': case 'p': case 'r': case '0':
					*ptr2++ = '\020';
					break;
				case (char) 0:
					*ptr2++ = '\\';
					goto end_strcat_ues;
					break;
				default:
					*ptr2++ = *ptr;
			}
		}
		else
			*ptr2++ = *ptr;
	}
end_strcat_ues:
	*ptr2 = '\0';

	return *dest;
}


char *	n_malloc_strcpy (char **ptr, const char *src, const char *file, const int line)
{
	if (!src)
	{
		debug_free((void **)ptr, file, line);
		return NULL;
	}
	if (ptr && *ptr)
	{
		if (*ptr == src)
			return *ptr;
		debug_free((void **)ptr, file, line);
	}
	*ptr = debug_malloc(strlen(src) + 1, file, line);
	return strcpy(*ptr, src);
	return *ptr;
}

char	*n_m_strdup (const char *str, const char *file, const int line)
{
	char *ptr;
	
	if (!str)
		str = empty_string;
	ptr = (char *)debug_malloc(strlen(str) + 1, file, line);
	return strcpy(ptr, str);
}


/* malloc_strcat: Yeah, right */
char *	n_malloc_strcat (char **ptr, const char *src, const char *file, const int line)
{
	size_t  msize;

	if (*ptr)
	{
		if (!src)
			return *ptr;
		msize = strlen(*ptr) + strlen(src) + 1;
		debug_realloc((void **)ptr, sizeof(char)*msize, file, line);
		return strcat(*ptr, src);
	}
	return (*ptr = n_m_strdup(src, file, line));
}

char *n_m_strndup (const char *str, const int len, const char *file, const int line)
{
	char *retval = (char *)debug_malloc(len + 1, file, line);
	return strmcpy(retval, (char *)str, len);
}


char *m_3dup (const char *str1, const char *str2, const char *str3)
{
	size_t msize = strlen(str1) + strlen(str2) + strlen(str3) + 1;
	return strcat(strcat(strcpy((char *)new_malloc(msize), str1), str2), str3);
}

char *m_opendup (const char *str1, ...)
{
	va_list args;
	int size;
	char *this_arg = NULL;
	char *retval = NULL;

	size = strlen(str1);
	va_start(args, str1);
	while ((this_arg = va_arg(args, char *)))
		size += strlen(this_arg);

	retval = (char *)new_malloc(size + 1);

	strcpy(retval, str1);
	va_start(args, str1);
	while ((this_arg = va_arg(args, char *)))
		strcat(retval, this_arg);

	va_end(args);
	return retval;
}


char	*m_s3cat (char **one, const char *maybe, const char *definitely)
{
	if (*one && **one)
		return m_3cat(one, maybe, definitely);
	return malloc_strcpy(one, definitely);
}

char *m_s3cat_s (char **one, const char *maybe, const char *ifthere)
{
	if (ifthere && *ifthere)
		return m_3cat(one, maybe, ifthere);
	return *one;
}

char	*m_3cat(char **one, const char *two, const char *three)
{
	int len = 0;
	char *str;

	if (*one)
		len = strlen(*one);
	if (two)
		len += strlen(two);
	if (three)
		len += strlen(three);
	len += 1;

	str = (char *)new_malloc(len);
	if (*one)
		strcpy(str, *one);
	if (two)
		strcat(str, two);
	if (three)
		strcat(str, three);

	new_free(one);
	return ((*one = str));
}

char	*upper (char *str)
{
register char	*ptr = NULL;

	if (str)
	{
		ptr = str;
		for (; *str; str++)
		{
			if (islower(*str))
				*str = toupper(*str);
		}
	}
	return (ptr);
}

char	*lower (char *str)
{
register char	*ptr = NULL;

	if (str)
	{
		ptr = str;
		for (; *str; str++)
		{
			if (isupper(*str))
				*str = tolower(*str);
		}
	}
	return (ptr);
}

char *malloc_sprintf (char **to, const char *pattern, ...)
{
	char booya[BIG_BUFFER_SIZE*3+1];
	*booya = 0;
	
	if (pattern)
	{
		va_list args;
		va_start (args, pattern);
		vsnprintf(booya, BIG_BUFFER_SIZE * 3, pattern, args);
		va_end(args);
	}
	malloc_strcpy(to, booya);
	return *to;
}

/* same thing, different variation */
char *m_sprintf (const char *pattern, ...)
{
	char booya[BIG_BUFFER_SIZE * 3 + 1];
	*booya = 0;
	
	if (pattern)
	{
		va_list args;
		va_start (args, pattern);
		vsnprintf(booya, BIG_BUFFER_SIZE * 3, pattern, args);
		va_end(args);
	}
	return m_strdup(booya);
}

/* case insensitive string searching */
char	*stristr (const char *source, const char *search)
{
        int     x = 0;

        if (!source || !*source || !search || !*search || strlen(source) < strlen(search))
		return NULL;

        while (*source)
        {
                if (source[x] && toupper(source[x]) == toupper(search[x]))
			x++;
                else if (search[x])
			source++, x = 0;
		else
			return (char *)source;
        }
	return NULL;
}

/* case insensitive string searching from the end */
char	*rstristr (char *source, char *search)
{
	char *ptr;
	int x = 0;

        if (!source || !*source || !search || !*search || strlen(source) < strlen(search))
		return NULL;

	ptr = source + strlen(source) - strlen(search);

	while (ptr >= source)
        {
		if (!search[x])
			return ptr;

		if (toupper(ptr[x]) == toupper(search[x]))
			x++;
		else
			ptr--, x = 0;
	}
	return NULL;
}

#define risspace(c) (c == ' ')

#if 0
/* 
 * word_count:  Efficient way to find out how many words are in
 * a given string.  Relies on isspace() not being broken.
 */
int     word_count (char *str)
{
        int cocs = 0;
        int isv = 1;
register char *foo = str;

        if (!foo)
                return 0;

        while (*foo)
        {
		if (*foo == '"' && isv)
		{
			while (*(foo+1) && *++foo != '"')
				;
			isv = 0;
			cocs++;
		}
                if (!my_isspace(*foo) != !isv)
                {
                        isv = my_isspace(*foo);
                        cocs++;
                }
                foo++;
        }
        return (cocs + 1) / 2;
}
#else
int	word_count (const char *ptr)
{
	int	count = 0;
	if (!ptr || !*ptr)
		return 0;

	/* Skip any leading whitespace */
	while (*ptr && risspace(*ptr))
		ptr++;

 	while (ptr && *ptr)
	{
		/* Always pre-count words */
		count++;

		/* 
		 * If this is an extended word, then skip everything
		 * up to the first un-backslashed double quote.
		 */
		if (*ptr == '"')
		{
			for (ptr++; *ptr; ptr++)
			{
				if (*ptr == '\\' && ptr[1])
					ptr++;
				else if (*ptr == '"')
				{
					ptr++;
					break;
				}
			}
		}

		/* 
		 * This is a regular word, skip all of the non-whitespace
		 * characters.
		 */
		else
		{
			while (*ptr && !risspace(*ptr))
				ptr++;
		}

		/* Skip any leading whitespace before the next word */
		while (*ptr && risspace(*ptr))
			ptr++;
	}

	return count;
}
#endif

#if 0
extern  int     word_scount (char *str)
{
        int cocs = 0;
        char *foo = str;
	int isv = 1;
	
        if (!foo)
                return 0;

        while (*foo)
        {
                if (my_isspace(*foo) != !isv)
                {
                        isv = my_isspace(*foo);
                        cocs++;
                }
                foo++;
        }
        return (cocs + 1) / 2;
}
#endif

char	*next_arg (char *str, char **new_ptr)
{
	char	*ptr;

	/* added by Sheik (kilau@prairie.nodak.edu) -- sanity */
	if (!str || !*str)
		return NULL;

	if ((ptr = sindex(str, "^ ")) != NULL)
	{
		if ((str = sindex(ptr, space)) != NULL)
			*str++ = (char) 0;
		else
			str = empty_string;
	}
	else
		str = empty_string;
	if (new_ptr)
		*new_ptr = str;
	return ptr;
}

extern char *remove_trailing_spaces (char *foo)
{
	char *end;
	if (!*foo)
		return foo;

	end = foo + strlen(foo) - 1;
	while (end > foo && my_isspace(*end))
		end--;
	end[1] = 0;
	return foo;
}

/* like remove_trailing_spaces but counts instead of removes */
extern int count_trailing_spaces (const char *foo)
{
       const char *end;
       const char *end2;
       if (!*foo)
               return 0;

       end = end2 = foo + strlen(foo) - 1;
       while (end > foo && my_isspace(*end))
               end--;
       return (end2 - end);
}

/*
 * yanks off the last word from 'src'
 * kinda the opposite of next_arg
 */
char *last_arg (char **src)
{
	char *ptr;

	if (!src || !*src)
		return NULL;

	remove_trailing_spaces(*src);
	ptr = *src + strlen(*src) - 1;

	if (*ptr == '"')
	{
		for (ptr--;;ptr--)
		{
			if (*ptr == '"')
			{
				if (ptr == *src)
					break;
				if (ptr[-1] == ' ')
				{
					ptr--;
					break;
				}
			}
			if (ptr == *src)
				break;
		}
	}
	else
	{
		for (;;ptr--)
		{
			if (*ptr == ' ')
				break;
			if (ptr == *src)
				break;
		}
	}

	if (ptr == *src)
	{
		ptr = *src;
		*src = empty_string;
	}
	else
	{
		*ptr++ = 0;
		remove_trailing_spaces(*src);
	}
	return ptr;

}


char	*new_next_arg (char *str, char **new_ptr)
{
	char	*ptr,
		*start;

	if (!str || !*str)
		return NULL;

	ptr = str;
	while (*ptr && risspace(*ptr))
		ptr++;

	if (*ptr == '"')
	{
		start = ++ptr;
		for (str = start; *str; str++)
		{
			if (*str == '\\' && str[1])
				str++;
			else if (*str == '"')
			{
				*(str++) = 0;
				if (risspace(*str))
					str++;
				break;
			}
		}
	}
	else if (*ptr)
	{
		str = ptr;
		while (*str && !risspace(*str))
			str++;
		if (*str)
			*str++ = 0;
	}

	if (!*str)
		str = empty_string;

	if (new_ptr)
		*new_ptr = str;

	return ptr;
}

/*
 * This function is "safe" because it doesnt ever return NULL.
 * XXXX - this is an ugly kludge that needs to go away
 */
char	*safe_new_next_arg (char *str, char **new_ptr)
{
	char	*ptr,
		*start;

	if (!str || !*str)
		return empty_string;

	if ((ptr = sindex(str, "^ \t")) != NULL)
	{
		if (*ptr == '"')
		{
			start = ++ptr;
			while ((str = sindex(ptr, "\"\\")) != NULL)
			{
				switch (*str)
				{
					case '"':
						*str++ = '\0';
						if (*str == ' ')
							str++;
						if (new_ptr)
							*new_ptr = str;
						return (start);
					case '\\':
						if (*(str + 1) == '"')
							ov_strcpy(str, str + 1);
						ptr = str + 1;
				}
			}
			str = empty_string;
		}
		else
		{
			if ((str = sindex(ptr, " \t")) != NULL)
				*str++ = '\0';
			else
				str = empty_string;
		}
	}
	else
		str = empty_string;

	if (new_ptr)
		*new_ptr = str;

	if (!ptr)
		return empty_string;

	return ptr;
}

char	*new_new_next_arg (char *str, char **new_ptr, char *type)
{
	char	*ptr,
		*start;

	if (!str || !*str)
		return NULL;

	if ((ptr = sindex(str, "^ \t")) != NULL)
	{
		if ((*ptr == '"') || (*ptr == '\''))
		{
			char blah[3];
			blah[0] = *ptr;
			blah[1] = '\\';
			blah[2] = '\0';

			*type = *ptr;
			start = ++ptr;
			while ((str = sindex(ptr, blah)) != NULL)
			{
				switch (*str)
				{
				case '\'':
				case '"':
					*str++ = '\0';
					if (*str == ' ')
						str++;
					if (new_ptr)
						*new_ptr = str;
					return (start);
				case '\\':
					if (str[1] == *type)
						ov_strcpy(str, str + 1);
					ptr = str + 1;
				}
			}
			str = empty_string;
		}
		else
		{
			*type = '\"';
			if ((str = sindex(ptr, " \t")) != NULL)
				*str++ = 0;
			else
				str = empty_string;
		}
	}
	else
		str = empty_string;
	if (new_ptr)
		*new_ptr = str;
	return ptr;
}

unsigned char stricmp_table [] = 
{
	0,	1,	2,	3,	4,	5,	6,	7,
	8,	9,	10,	11,	12,	13,	14,	15,
	16,	17,	18,	19,	20,	21,	22,	23,
	24,	25,	26,	27,	28,	29,	30,	31,
	32,	33,	34,	35,	36,	37,	38,	39,
	40,	41,	42,	43,	44,	45,	46,	47,
	48,	49,	50,	51,	52,	53,	54,	55,
	56,	57,	58,	59,	60,	61,	62,	63,
	64,	65,	66,	67,	68,	69,	70,	71,
	72,	73,	74,	75,	76,	77,	78,	79,
	80,	81,	82,	83,	84,	85,	86,	87,
	88,	89,	90,	91,	92,	93,	94,	95,
	96,	65,	66,	67,	68,	69,	70,	71,
	72,	73,	74,	75,	76,	77,	78,	79,
	80,	81,	82,	83,	84,	85,	86,	87,
	88,	89,	90,	91,	92,	93,	126,	127,

	128,	129,	130,	131,	132,	133,	134,	135,
	136,	137,	138,	139,	140,	141,	142,	143,
	144,	145,	146,	147,	148,	149,	150,	151,
	152,	153,	154,	155,	156,	157,	158,	159,
	160,	161,	162,	163,	164,	165,	166,	167,
	168,	169,	170,	171,	172,	173,	174,	175,
	176,	177,	178,	179,	180,	181,	182,	183,
	184,	185,	186,	187,	188,	189,	190,	191,
	192,	193,	194,	195,	196,	197,	198,	199,
	200,	201,	202,	203,	204,	205,	206,	207,
	208,	209,	210,	211,	212,	213,	214,	215,
	216,	217,	218,	219,	220,	221,	222,	223,
	224,	225,	226,	227,	228,	229,	230,	231,
	232,	233,	234,	235,	236,	237,	238,	239,
	240,	241,	242,	243,	244,	245,	246,	247,
	248,	249,	250,	251,	252,	253,	254,	255
};

/* my_stricmp: case insensitive version of strcmp */
int	my_stricmp (register const unsigned char *str1, register const unsigned char *str2)
{
	while (*str1 && *str2 && (stricmp_table[(unsigned short)*str1] == stricmp_table[(unsigned short)*str2]))
		str1++, str2++;
	return (stricmp_table[(unsigned short)*str1] -
		stricmp_table[(unsigned short)*str2]);

}

/* my_strnicmp: case insensitive version of strncmp */
int	my_strnicmp (register const unsigned char *str1, register const unsigned char *str2, register size_t n)
{
	while (n && *str1 && *str2 && (stricmp_table[(unsigned short)*str1] == stricmp_table[(unsigned short)*str2]))
		str1++, str2++, n--;
	return (n ?
		(stricmp_table[(unsigned short)*str1] -
		stricmp_table[(unsigned short)*str2]) : 0);
}

/* my_strnstr: case insensitive version of strstr */
int	my_strnstr (register const unsigned char *str1, register const unsigned char *str2, register size_t n)
{
	char *p = (char *)str1;
	if (!p) return 0;
	for (; *p; p++)
		if (!strncasecmp(p, str2, strlen(str2)))
			return 1;
	return 0;
}

/* chop -- chops off the last character. capiche? */
char *chop (char *stuff, int nchar)
{
	size_t sl = strlen(stuff);
	if (nchar > 0 && sl > 0 && nchar <= sl)
		stuff[sl - nchar] = 0;
	else if (nchar > sl)
		stuff[0] = 0;
	return stuff;
}

/*
 * strext: Makes a copy of the string delmited by two char pointers and
 * returns it in malloced memory.  Useful when you dont want to munge up
 * the original string with a null.  end must be one place beyond where
 * you want to copy, ie, its the first character you dont want to copy.
 */
char *strext(const char *start, const char *end)
{
	char *ptr, *retval;

	ptr = retval = (char *)new_malloc(end-start+1);
	while (start < end)
		*ptr++ = *start++;
	*ptr = 0;
	return retval;
}


/*
 * strmcpy: Well, it's like this, strncpy doesn't append a trailing null if
 * strlen(str) == maxlen.  strmcpy always makes sure there is a trailing null 
 */
char *	strmcpy (char *dest, const char *src, int maxlen)
{
	strlcpy(dest, src, maxlen + 1);
	return dest;
}

/*
 * strmcat: like strcat, but truncs the dest string to maxlen (thus the dest
 * should be able to handle maxlen+1 (for the null)) 
 */
char *	strmcat(char *dest, const char *src, int maxlen)
{
	strlcat(dest, src, maxlen + 1);
	return dest;
}

/*
 * scanstr: looks for an occurrence of str in source.  If not found, returns
 * 0.  If it is found, returns the position in source (1 being the first
 * position).  Not the best way to handle this, but what the hell 
 */
extern	int	scanstr (char *str, char *source)
{
	int	i,
		max,
		len;

	len = strlen(str);
	max = strlen(source) - len;
	for (i = 0; i <= max; i++, source++)
	{
		if (!my_strnicmp(source, str, len))
			return (i + 1);
	}
	return (0);
}

#if defined(WINNT) || defined(__EMX__)
char *convert_dos(char *str)
{
register char *p;
	for (p = str; *p; p++)
		if (*p == '/')
			*p = '\\';
	return str;
}

char *convert_unix(char *arg)
{
register char *x = arg;
	while (*x)
	{
		if (*x == '\\')
			*x = '/';
		x++;
	}
	return arg;
}

int is_dos(char *filename)
{
	if (strlen(filename) > 3 && ( (*(filename+1) == ':') && (*(filename+2) == '/' || *(filename+2) == '\\')) )
		return 1;
	else
		return 0;
}

#endif


/* expand_twiddle: expands ~ in pathnames. */
char	*expand_twiddle (char *str)
{
	char	buffer[BIG_BUFFER_SIZE/4 + 1];
	char *str2 = NULL;

#ifdef WINNT
	convert_unix(str);
#endif
	if (*str == '~')
	{
		str++;

#if defined(WINNT) || defined(__EMX__)
		if (*str == '\\' || *str == '/' || !*str)
#else
		if (*str == '/' || !*str)
#endif
		{
			strmcpy(buffer, my_path, BIG_BUFFER_SIZE/4);
			strmcat(buffer, str, BIG_BUFFER_SIZE/4);
		}
		else
		{
			char	*rest;
			struct	passwd *entry;
#if defined(WINNT) || defined(__EMX__)
			char	*p = NULL;
#endif			
			if ((rest = strchr(str, '/')) != NULL)
				*rest++ = '\0';
#if defined(WINNT) || defined(__EMX__)
			if (((entry = getpwnam(str)) != NULL) || (p = getenv("HOME")))
			{
				if (p)
					strmcpy(buffer, p, BIG_BUFFER_SIZE/4);
				else
					strmcpy(buffer, entry->pw_dir, BIG_BUFFER_SIZE/4);
#else
			if ((entry = getpwnam(str)) != NULL)
			{
				strmcpy(buffer, entry->pw_dir, BIG_BUFFER_SIZE/4);
#endif
				if (rest)
				{
					strmcat(buffer, "/", BIG_BUFFER_SIZE/4);
					strmcat(buffer, rest, BIG_BUFFER_SIZE/4);
				}
			}
			else
				return (char *) NULL;
		}
	}
	else
		strmcpy(buffer, str, BIG_BUFFER_SIZE/4);

	malloc_strcpy(&str2, buffer);
#ifdef __EMX__
	convert_unix(str2);
#endif
	return str2;
}

/* islegal: true if c is a legal nickname char anywhere but first char */
#define islegal(c) ((((c) >= 'A') && ((c) <= '}')) || \
		    (((c) >= '0') && ((c) <= '9')) || \
		     ((c) == '-') || ((c) == '_'))

char	*check_nickname (char *nick)
{
char *s = nick;
    while (*s)
    {
	if (*s & 0x80 || isspace (*s) || iscntrl (*s) || !isprint(*s) ||
		*s == ':')
	    return NULL;
	s++;
    }
    return nick;
}

/*
 * sindex: much like index(), but it looks for a match of any character in
 * the group, and returns that position.  If the first character is a ^, then
 * this will match the first occurence not in that group.
 */
char	*sindex (register char *string, char *group)
{
	char	*ptr;

	if (!string || !group)
		return (char *) NULL;
	if (*group == '^')
	{
		group++;
		for (; *string; string++)
		{
			for (ptr = group; *ptr; ptr++)
			{
				if (*ptr == *string)
					break;
			}
			if (*ptr == '\0')
				return string;
		}
	}
	else
	{
		for (; *string; string++)
		{
			for (ptr = group; *ptr; ptr++)
			{
				if (*ptr == *string)
					return string;
			}
		}
	}
	return (char *) NULL;
}

/*
 * rsindex: much like rindex(), but it looks for a match of any character in
 * the group, and returns that position.  If the first character is a ^, then
 * this will match the first occurence not in that group.
 */
char	*rsindex (register char *string, char *start, char *group, int howmany)
{
	register char	*ptr;

	if (howmany && string && start && group && start <= string)
	{
		if (*group == '^')
		{
			group++;
			for (ptr = string; (ptr >= start) && howmany; ptr--)
			{
				if (!strchr(group, *ptr))
				{
					if (--howmany == 0)
						return ptr;
				}
			}
		}
		else
		{
			for (ptr = string; (ptr >= start) && howmany; ptr--)
			{
				if (strchr(group, *ptr))
				{
					if (--howmany == 0)
						return ptr;
				}
			}
		}
	}
	return NULL;
}

/* is_number: returns true if the given string is a number, false otherwise */
int	is_number (const char *str)
{
	if (!str || !*str)
		return 0;
	while (*str == ' ')
		str++;
	if (*str == '-' || *str == '+')
		str++;
	if (*str)
	{
		for (; *str; str++)
		{
			if (!isdigit((*str)))
				return (0);
		}
		return 1;
	}
	else
		return 0;
}

/* rfgets: exactly like fgets, cept it works backwards through a file!  */
char	*rfgets (char *buffer, int size, FILE *file)
{
	char	*ptr;
	off_t	pos;

	if (fseek(file, -2L, SEEK_CUR))
		return NULL;
	do
	{
		switch (fgetc(file))
		{
		case EOF:
			return NULL;
		case '\n':
			pos = ftell(file);
			ptr = fgets(buffer, size, file);
			fseek(file, pos, 0);
			return ptr;
		}
	}
	while (fseek(file, -2L, SEEK_CUR) == 0);
	rewind(file);
	pos = 0L;
	ptr = fgets(buffer, size, file);
	fseek(file, pos, 0);
	return ptr;
}

/*
 * path_search: given a file called name, this will search each element of
 * the given path to locate the file.  If found in an element of path, the
 * full path name of the file is returned in a static string.  If not, null
 * is returned.  Path is a colon separated list of directories 
 */
char	*path_search (char *name, char *path)
{
	static	char	buffer[BIG_BUFFER_SIZE/2 + 1];
	char	*ptr,
		*free_path = NULL;

	/* A "relative" path is valid if the file exists */
	/* A "relative" path is searched in the path if the
	   filename doesnt really exist from where we are */
	if (strchr(name, '/'))
		if (!access(name, F_OK))
			return name;

	/* an absolute path is always checked, never searched */
#if defined(WINNT) || defined(__EMX__)
	if (name[0] == '/' || name[0] == '\\')
#else
	if (name[0] == '/')
#endif
		return (access(name, F_OK) ? NULL : name);
	
	if (!path)
		return NULL;

	/* This is cheating. >;-) */
	free_path = LOCAL_COPY(path);
	path = free_path;

#ifdef __EMX__
	convert_unix(path);
#endif
	while (path)
	{
#if defined(WINNT) || defined(__EMX__)
		if (((ptr = strchr(path, ';')) != NULL) || ((ptr = strchr(path, ':')) != NULL))
#else
		if ((ptr = strchr(path, ':')) != NULL)
#endif
			*ptr++ = '\0';
		*buffer = 0;
		if (path[0] == '~')
		{
			strmcat(buffer, my_path, BIG_BUFFER_SIZE/4);
			path++;
		}
		strmcat(buffer, path, BIG_BUFFER_SIZE/4);
		strmcat(buffer, "/", BIG_BUFFER_SIZE/4);
		strmcat(buffer, name, BIG_BUFFER_SIZE/4);

		if (access(buffer, F_OK) == 0)
			break;
		path = ptr;
	}

	return (path != NULL) ? buffer : NULL;
}

/*
 * double_quote: Given a str of text, this will quote any character in the
 * set stuff with the QUOTE_CHAR. It returns a malloced quoted, null
 * terminated string 
 */
char	*double_quote (const char *str, const char *stuff, char *buffer)
{
	register char	c;
	register int	pos;

	*buffer = 0;
	if (!stuff)
		return buffer;
	for (pos = 0; (c = *str); str++)
	{
		if (strchr(stuff, c))
		{
			if (c == '$')
				buffer[pos++] = '$';
			else
				buffer[pos++] = '\\';
		}
		buffer[pos++] = c;
	}
	buffer[pos] = '\0';
	return buffer;
}

void	nappanic (char *format, ...)
{
	char buffer[3 * BIG_BUFFER_SIZE + 1];
	static int recursion = 0;
	if (recursion)
		abort();

	recursion++;	
	if (format)
	{
		va_list arglist;
		va_start(arglist, format);
		vsnprintf(buffer, BIG_BUFFER_SIZE, format, arglist);
		va_end(arglist);
	}

	yell("An unrecoverable logic error has occured.");
	yell("Please email %s giving me the following message", "edwards@bitchx.dimension6.com"  );

	nap_exit(1, "TekNap panic... Bug... Bug... Bug...", NULL);
}

/* Not very complicated, but very handy function to have */
int end_strcmp (const char *one, const char *two, int bytes)
{
	if (bytes < strlen(one))
		return (strcmp(one + strlen (one) - (size_t) bytes, two));
	else
		return -1;
}

/* beep_em: Not hard to figure this one out */
void beep_em (int beeps)
{
	int	cnt,
		i;

	for (cnt = beeps, i = 0; i < cnt; i++)
		term_beep();
}



FILE *open_compression (char *executable, char *filename, int hook)
{
	FILE *file_pointer = NULL;
	int pipes[2];

	
	pipes[0] = -1;
	pipes[1] = -1;

	if (pipe (pipes) == -1)
	{
		if (hook)
			yell("Cannot start decompression: %s\n", strerror(errno));
		if (pipes[0] != -1)
		{
			close (pipes[0]);
			close (pipes[1]);
		}
		return NULL;
	}

	switch (fork ())
	{
		case -1:
		{
			if (hook)
				yell("Cannot start decompression: %s\n", strerror(errno));
			return NULL;
		}
		case 0:
		{
			int i;
#if !defined(WINNT) && !defined(__EMX__)
			setsid();
#endif			
			setuid (getuid ());
			setgid (getgid ());
			dup2 (pipes[1], 1);
			close (pipes[0]);
			for (i = 2; i < 256; i++)
				close(i);
#ifdef ZARGS
			execl (executable, executable, "-c", ZARGS, filename, NULL);
#else
			execl (executable, executable, "-c", filename, NULL);
#endif
			_exit (0);
		}
		default :
		{
			close (pipes[1]);
			if ((file_pointer = fdopen(pipes[0], "r")) == NULL)
			{
				if (hook)
					yell("Cannot start decompression: %s\n", strerror(errno));
				return NULL;
			}
#if 0
			setlinebuf(file_pointer);
			setvbuf(file_pointer, NULL, _IONBF, 0);
#endif
			break;
		}
	}
	return file_pointer;
}

char *uzfopen_error(int error)
{
	switch (error)
	{
	}
	return empty_string;
}

/* Front end to fopen() that will open ANY file, compressed or not, and
 * is relatively smart about looking for the possibilities, and even
 * searches a path for you! ;-)
 */
FILE *uzfopen (char **filename, char *path, int hook, int *err)
{
	static int	setup				= 0;
	int 		ok_to_decompress 		= 0;
	char *		filename_path;
	char 		*filename_trying;
	char		*filename_blah;
	static char 	*path_to_gunzip = NULL;
	static char	*path_to_uncompress = NULL;
	static char	*path_to_bunzip2 = NULL;
	FILE *		doh = NULL;

	filename_trying = alloca(MAXPATHLEN+1);
	
	if (!setup)
	{
		char *gzip = path_search("gunzip", getenv("PATH"));
		char *compress = NULL;
		char *bzip = NULL;
		if (!gzip)
			gzip = empty_string;
		path_to_gunzip = m_strdup(gzip);

		if (!(compress = path_search("uncompress", getenv("PATH"))))
			compress = empty_string;
		path_to_uncompress = m_strdup(compress);

		if (!(bzip = path_search("bunzip2", getenv("PATH"))))
			bzip = empty_string;
		path_to_bunzip2 = m_strdup(bzip);
		setup = 1;
	}

	/* It is allowed to pass to this function either a true filename
	   with the compression extention, or to pass it the base name of
	   the filename, and this will look to see if there is a compressed
	   file that matches the base name */

	/* Start with what we were given as an initial guess */
	if (**filename == '~')
	{
		if ((filename_blah = expand_twiddle(*filename)))
		{
			strlcpy(filename_trying, filename_blah, MAXPATHLEN);
			new_free(&filename_blah);		
		}
		else
			filename_trying = LOCAL_COPY(*filename);

	}
	else
		strlcpy(filename_trying, *filename, MAXPATHLEN);

	/* Look to see if the passed filename is a full compressed filename */
	if ((! end_strcmp (filename_trying, ".gz", 3)) ||
	    (! end_strcmp (filename_trying, ".z", 2))) 
	{
		if (path_to_gunzip)
		{	
			ok_to_decompress = 1;
			filename_path = path_search (filename_trying, path);
		}
		else
		{
			if (hook)
				yell("Cannot open file %s because gunzip was not found", filename_trying);
			if (err)
				*err = ERR_NOGUNZIP;
			new_free(filename);
			return NULL;
		}
	}
	else if (! end_strcmp (filename_trying, ".Z", 2))
	{
		if (path_to_gunzip || path_to_uncompress)
		{
			ok_to_decompress = 1;
			filename_path = path_search (filename_trying, path);
		}
		else
		{
			if (hook)
				yell("Cannot open file %s because uncompress was not found", filename_trying);
			if (err)
				*err = ERR_NOUNCOMPRESS;
			new_free(filename);
			return NULL;
		}
	}
	else if (!end_strcmp(filename_trying, ".bz2", 4))
	{
		if (*path_to_bunzip2)
		{
			ok_to_decompress = 3;
			filename_path = path_search(filename_trying, path);
		}
		else
		{
			if (hook)
				yell("Cannot open file %s because bunzip2 was not found", filename_trying);
			if (err)
				*err = ERR_NOBUNZIP2;
			new_free(filename);
			return NULL;
		}
	}
	/* Right now it doesnt look like the file is a full compressed fn */
	else
	{
		struct stat file_info;

		/* Trivially, see if the file we were passed exists */
		filename_path = path_search (filename_trying, path);

		/* Nope. it doesnt exist. */
		if (!filename_path)
		{
			/* Is there a "filename.gz"? */
			strlcpy (filename_trying, *filename, MAXPATHLEN);
			strlcat (filename_trying, ".gz", MAXPATHLEN);
			filename_path = path_search (filename_trying, path);

			/* Nope. no "filename.gz" */
			if (!filename_path)
			{
				/* Is there a "filename.Z"? */
				strlcpy (filename_trying, *filename, MAXPATHLEN);
				strlcat (filename_trying, ".Z", MAXPATHLEN);
				filename_path = path_search (filename_trying, path);
				
				/* Nope. no "filename.Z" */
				if (!filename_path)
				{
					/* Is there a "filename.z"? */
					strlcpy (filename_trying, *filename, MAXPATHLEN);
					strlcat (filename_trying, ".z", MAXPATHLEN);
					filename_path = path_search (filename_trying, path);

					if (!filename_path)
					{
						strlcpy(filename_trying, *filename, MAXPATHLEN);
						strlcat(filename_trying, ".bz2", MAXPATHLEN);
						filename_path = path_search(filename_trying, path);
						if (!filename_path)
						{
							if (hook)
								yell("File not found: %s", *filename);
							if (err)
								*err = ERR_NOFILE;
							new_free(filename);
							return NULL;
						}
						else
							/* found a bz2 */
							ok_to_decompress = 3;
					}
					/* Yep. there's a "filename.z" */
					else
						ok_to_decompress = 2;
				}
				/* Yep. there's a "filename.Z" */
				else
					ok_to_decompress = 1;
			}
			/* Yep. There's a "filename.gz" */
			else
				ok_to_decompress = 2;
		}
		/* Imagine that! the file exists as-is (no decompression) */
		else
			ok_to_decompress = 0;

		stat (filename_path, &file_info);
		if (file_info.st_mode & S_IFDIR)
		{
			if (hook)
				yell("%s is a directory", filename_trying);
			if (err)
				*err = ERR_DIRECTORY;
			new_free(filename);
			return NULL;
		}
#ifndef WINNT
		if (file_info.st_mode & 0111)
		{
			char *p;
			if ((p = strrchr(filename_path, '.')))
			{
				p++;
				if (!strcmp(p, "so"))
				{
					malloc_strcpy(filename, filename_path);
					return NULL;
				}
			}
			if (hook)
				yell("Cannot open %s -- executable file", filename_trying);
			if (err)
				*err = ERR_EXECUTABLE;
			new_free(filename);
			return NULL;
		}
#endif
	}

	malloc_strcpy (filename, filename_path);

	/* at this point, we should have a filename in the variable
	   filename_trying, and it should exist.  If ok_to_decompress
	   is one, then we can gunzip the file if guzip is available,
	   else we uncompress the file */
	if (ok_to_decompress)
	{
		if (ok_to_decompress <= 2 && *path_to_gunzip)
			return open_compression (path_to_gunzip, filename_path, hook);
		else if ((ok_to_decompress == 1) && *path_to_uncompress)
			return open_compression (path_to_uncompress, filename_path, hook);
		else if ((ok_to_decompress == 3) && *path_to_bunzip2)
			return open_compression(path_to_bunzip2, filename_path, hook);
			
		if (hook)
			yell("Cannot open compressed file %s because no uncompressor was found", filename_trying);
		if (err)
			*err = ERR_NOUNCOMPRESSOR;
		new_free(filename);
		return NULL;
	}

	/* Its not a compressed file... Try to open it regular-like. */
	if ((doh = fopen(filename_path, "r")) != NULL)
		return doh;

	/* nope.. we just cant seem to open this file... */
	if (hook)
		yell("Cannot open file %s: %s", filename_path, strerror(errno));
	if (err)
		*err = ERR_CANNOTOPEN;
	new_free(filename);
	return NULL;
}


#ifdef INCLUDE_DEADCODE
/* some more string manips by hop (june, 1995) */
extern int fw_strcmp(comp_len_func *compar, char *one, char *two)
{
	int len = 0;
	char *pos = one;

	while (!my_isspace(*pos))
		pos++, len++;

	return compar(one, two, len);
}


/* 
 * Compares the last word in 'one' to the string 'two'.  You must provide
 * the compar function.  my_stricmp is a good default.
 */
extern int lw_strcmp(comp_func *compar, char *one, char *two)
{
	char *pos = one + strlen(one) - 1;

	if (pos > one)			/* cant do pos[-1] if pos == one */
		while (!my_isspace(pos[-1]) && (pos > one))
			pos--;
	else
		pos = one;

	if (compar)
		return compar(pos, two);
	else
		return my_stricmp(pos, two);
}

/* 
 * you give it a filename, some flags, and a position, and it gives you an
 * fd with the file pointed at the 'position'th byte.
 */
extern int opento(char *filename, int flags, off_t position)
{
	int file;

	file = open(filename, flags, 777);
	lseek(file, position, SEEK_SET);
	return file;
}
#endif

/* swift and easy -- returns the size of the file */
off_t file_size (char *filename)
{
	struct stat statbuf;

	if (!stat(filename, &statbuf))
		return (off_t)(statbuf.st_size);
	else
		return -1;
}

/* Gets the time in second/usecond if you can,  second/0 if you cant. */
struct timeval get_time(struct timeval *timer)
{
	static struct timeval timer2;
#ifdef HAVE_GETTIMEOFDAY
	if (timer)
	{
		gettimeofday(timer, NULL);
		return *timer;
	}
	gettimeofday(&timer2, NULL);
	return timer2;
#else
	time_t time2 = time(NULL);

	if (timer)
	{
		timer->tv_sec = time2;
		timer->tv_usec = 0;
		return *timer;
	}
	timer2.tv_sec = time2;
	timer2.tv_usec = 0;
	return timer2;
#endif
}

/* 
 * calculates the time elapsed between 'one' and 'two' where they were
 * gotten probably with a call to get_time.  'one' should be the older
 * timer and 'two' should be the most recent timer.
 */
double time_diff (struct timeval one, struct timeval two)
{
	struct timeval td;

	td.tv_sec = two.tv_sec - one.tv_sec;
	td.tv_usec = two.tv_usec - one.tv_usec;

	return (double)td.tv_sec + ((double)td.tv_usec / 1000000.0);
}

int time_to_next_minute (void)
{
	time_t now = time(NULL);
	static int which = 0;
	
	if (which == 1)
		return 60 - now % 60;
	else
	{	
		struct tm *now_tm = gmtime(&now);

		if (!which)
		{
			if (now_tm->tm_sec == now % 60)
				which = 1;
			else
				which = 2;
		}
		return 60-now_tm->tm_sec;
	}
}

char *plural (int number)
{
	return (number != 1) ? "s" : empty_string;
}

char *my_ctime (time_t when)
{
	return chop(ctime(&when), 1);
}

char *ltoa (long foo)
{
	static char buffer[BIG_BUFFER_SIZE/8+1];
	char *pos = buffer + BIG_BUFFER_SIZE/8-1;
	unsigned long absv;
	int negative;

	absv = (foo < 0) ? (unsigned long)-foo : (unsigned long)foo;
	negative = (foo < 0) ? 1 : 0;

	buffer[BIG_BUFFER_SIZE/8] = 0;
	for (; absv > 9; absv /= 10)
		*pos-- = (absv % 10) + '0';
	*pos = (absv) + '0';

	if (negative)
		*--pos = '-';

	return pos;
}

/*
 * Formats "src" into "dest" using the given length.  If "length" is
 * negative, then the string is right-justified.  If "length" is
 * zero, nothing happens.  Sure, i cheat, but its cheaper then doing
 * two sprintf's.
 */
char *strformat (char *dest, const char *src, int length, char pad_char)
{
	char *ptr1 = dest, 
	     *ptr2 = (char *)src;
	int tmplen = length;
	int abslen;
	char padc;
		
	abslen = (length >= 0 ? length : -length);
	if (!(padc = pad_char))
		padc = ' ';

	/* Cheat by spacing out 'dest' */
	for (tmplen = abslen - 1; tmplen >= 0; tmplen--)
		dest[tmplen] = padc;
	dest[abslen] = 0;

	/* Then cheat further by deciding where the string should go. */
	if (length > 0)		/* left justified */
	{
		while ((length-- > 0) && *ptr2)
			*ptr1++ = *ptr2++;
	}
	else if (length < 0)	/* right justified */
	{
		length = -length;
		ptr1 = dest;
		ptr2 = (char *)src;
		if (strlen(src) < length)
			ptr1 += length - strlen(src);
		while ((length-- > 0) && *ptr2)
			*ptr1++ = *ptr2++;
	}
	return dest;
}


/* MatchingBracket returns the next unescaped bracket of the given type */
char	*MatchingBracket(register char *string, register char left, register char right)
{
	int	bracket_count = 1;

	if (left == '(')
	{
		for (; *string; string++)
		{
			switch (*string)
			{
				case '(':
					bracket_count++;
					break;
				case ')':
					bracket_count--;
					if (bracket_count == 0)
						return string;
					break;
				case '\\':
					if (string[1])
						string++;
					break;
			}
		}
	}
	else if (left == '[')
	{
		for (; *string; string++)
		{
			switch (*string)
	    		{
				case '[':
					bracket_count++;
					break;
				case ']':
					bracket_count--;
					if (bracket_count == 0)
						return string;
					break;
				case '\\':
					if (string[1])
						string++;
					break;
			}
		}
	}
	else		/* Fallback for everyone else */
	{
		while (*string && bracket_count)
		{
			if (*string == '\\' && string[1])
				string++;
			else if (*string == left)
				bracket_count++;
			else if (*string == right)
			{
				if (--bracket_count == 0)
					return string;
			}
			string++;
		}
	}

	return NULL;
}

/*
 * parse_number: returns the next number found in a string and moves the
 * string pointer beyond that point	in the string.  Here's some examples: 
 *
 * "123harhar"  returns 123 and str as "harhar" 
 *
 * while: 
 *
 * "hoohar"     returns -1  and str as "hoohar" 
 */
extern	int	parse_number(char **str)
{
	long ret;
	char *ptr = *str;	/* sigh */

	ret = strtol(ptr, str, 10);
	if (*str == ptr)
		ret = -1;

	return (int)ret;
}

#if 0
extern char *chop_word(char *str)
{
	char *end = str + strlen(str) - 1;

	while (my_isspace(*end) && (end > str))
		end--;
	while (!my_isspace(*end) && (end > str))
		end--;

	if (end >= str)
		*end = 0;

	return str;
}
#endif

extern int splitw (char *str, char ***to)
{
	int numwords = word_count(str);
	int counter;
	if (numwords)
	{
		*to = (char **)new_malloc(sizeof(char *) * numwords);
		for (counter = 0; counter < numwords; counter++)
			(*to)[counter] = new_next_arg(str, &str);
	}
	else
		*to = NULL;
	return numwords;
}

char * unsplitw (char ***container, int howmany)
{
	char *retval = NULL;
	char **str = *container;

	if (!str || !*str)
		return NULL;
	while (howmany)
	{
		m_s3cat(&retval, space, *str);
		str++, howmany--;
	}

	new_free((char **)container);
	return retval;
}

char *m_2dup (const char *str1, const char *str2)
{
	size_t msize = strlen(str1) + strlen(str2) + 1;
	return strcat(strcpy((char *)new_malloc(msize), str1), str2);
}

char *m_e3cat (char **one, const char *yes1, const char *yes2)
{
	if (*one && **one)
		return m_3cat(one, yes1, yes2);
	else
		*one = m_2dup(yes1, yes2);
	return *one;
}


double strtod();
extern int check_val (char *sub)
{
	long sval;
	char *endptr;

	if (!sub || !*sub)
		return 0;

	/* get the numeric value (if any). */
	sval = strtod(sub, &endptr);

	/* Its OK if:
	 *  1) the f-val is not zero.
	 *  2) the first illegal character was not a null.
	 *  3) there were no valid f-chars.
	 */
	if (sval || *endptr || (sub == endptr))
		return 1;

	return 0;
}

char *on_off(int var)
{
	if (var)
		return ("On");
	return ("Off");
}


/*
 * Appends 'num' copies of 'app' to the end of 'str'.
 */
extern char *strextend(char *str, char app, int num)
{
	char *ptr = str + strlen(str);

	for (;num;num--)
		*ptr++ = app;

	*ptr = (char) 0;
	return str;
}

const char *strfill(char c, int num)
{
static char buffer[BIG_BUFFER_SIZE/4+1];
int i = 0;
	if (num > BIG_BUFFER_SIZE/4)
		num = BIG_BUFFER_SIZE/4;
	for (i = 0; i < num; i++)
		buffer[i] = c;
	buffer[num] = 0;
	return buffer;
}

#ifdef INCLUDE_DEADCODE
/*
 * Appends the given character to the string
 */
char *strmccat(char *str, char c, int howmany)
{
	int x = strlen(str);

	if (x < howmany)
		str[x] = c;
	str[x+1] = 0;

	return str;
}

/*
 * Pull a substring out of a larger string
 * If the ending delimiter doesnt occur, then we dont pass
 * anything (by definition).  This is because we dont want
 * to introduce a back door into CTCP handlers.
 */
extern char *pullstr (char *source_string, char *dest_string)
{
	char delim = *source_string;
	char *end;

	end = strchr(source_string + 1, delim);

	/* If there is no closing delim, then we punt. */
	if (!end)
		return NULL;

	*end = 0;
	end++;

	strcpy(dest_string, source_string + 1);
	strcpy(source_string, end);
	return dest_string;
}
#endif

extern int empty (const char *str)
{
	while (str && *str && *str == ' ')
		str++;

	if (str && *str)
		return 0;

	return 1;
}

long my_atol (const char *str)
{
	if (str)
		return (long) strtol(str, NULL, 10);
	else
		return 0L;
}

unsigned long my_atoul (const char *str)
{
	if (str)
		return (unsigned long) strtoul(str, NULL, 10);
	else
		return 0L;
}

char *m_dupchar(int i)
{
	char c = (char) i;	/* blah */
	char *ret = (char *)new_malloc(2);

	ret[0] = c;
	ret[1] = 0;
	return ret;
}

#ifdef INCLUDE_DEADCODE
/*
 * This checks to see if ``root'' is a proper subname for ``var''.
 */
int is_root (char *root, char *var, int descend)
{
	int rootl, varl;

	/* ``root'' must end in a dot */
	rootl = strlen(root);
	if (root[rootl] != '.')
		return 0;

	/* ``root'' must be shorter than ``var'' */
	varl = strlen(var);
	if (varl <= rootl)
		return 0;

	/* ``var'' must contain ``root'' as a leading subset */
	if (my_strnicmp(root, var, rootl))
		return 0;

	/* 
	 * ``var'' must not contain any additional dots
	 * if we are checking for the current level only
	 */
	if (!descend && strchr(var + rootl, '.'))
		return 0;

	/* Looks like its ok */
	return 1;
}
#endif

/* Returns the number of characters they are equal at. */
size_t streq (const char *one, const char *two)
{
	size_t cnt = 0;

	while (*one && *two && (*one == *two))
		cnt++, one++, two++;

	return cnt;
}

/* Returns the number of characters they are equal at. */
size_t strieq (const char *one, const char *two)
{
	size_t cnt = 0;

	while (*one && *two && (toupper(*one) == toupper(*two)))
		cnt++, one++, two++;

	return cnt;
}

char *strmopencat (char *dest, int maxlen, ...)
{
	va_list args;
	int size;
	char *this_arg = NULL;
	int this_len;

	size = strlen(dest);
	va_start(args, maxlen);

	while (size < maxlen)
	{
		if (!(this_arg = va_arg(args, char *)))
			break;

		if (size + ((this_len = strlen(this_arg))) > maxlen)
			strncat(dest, this_arg, maxlen - size);
		else
			strcat(dest, this_arg);

		size += this_len;
	}

	va_end(args);
	return dest;
}

/*
 * An strcpy that is guaranteed to be safe for overlaps.
 */
char *ov_strcpy (char *one, const char *two)
{
	if (two > one)
	{
		while (two && *two)
			*one++ = *two++;
		*one = 0;
 	}
	return one;
}

char *next_in_comma_list (char *str, char **after)
{
	*after = str;

	while (*after && **after && **after != ',')
		(*after)++;

	if (*after && **after == ',')
	{
		**after = 0;
		(*after)++;
	}

	return str;
}

/* Dest should be big enough to hold "src" */
void	strip_control (const char *src, char *dest)
{
	for (; *src; src++)
	{
		if (isgraph(*src) || isspace(*src))
			*dest++ = *src;
	}

	*dest++ = 0;
}

char 	*strnrchr(char *start, char which, int howmany)
{
	char *ends = start + strlen(start);

	while (ends > start && howmany)
	{
		if (*--ends == which)
			howmany--;
	}
	if (ends == start)
		return NULL;
	else
		return ends;
}

/*
 * This replaces some number of numbers (1 or more) with a single asterisk.
 * We know that the final strcpy() is safe, since we never make a string that
 * is longer than the source string, always less than or equal in size.
 */
void	mask_digits (char **hostname)
{
	char	*src_ptr;
	char 	*retval, *retval_ptr;

	retval = retval_ptr = alloca(strlen(*hostname) + 1);
	src_ptr = *hostname;

	while (*src_ptr)
	{
		if (isdigit(*src_ptr))
		{
			while (*src_ptr && isdigit(*src_ptr))
				src_ptr++;

			*retval_ptr++ = '*';
		}
		else
			*retval_ptr++ = *src_ptr++;
	}

	*retval_ptr = 0;
	strcpy(*hostname, retval);
	return;
}

/*
 * Its like strcspn, except the seconD arg is NOT a string.
 */
size_t 	ccspan (const char *string, int s)
{
	size_t count = 0;
	char c = (char) s;

	while (string && *string && *string != c)
		string++, count++;

	return count;
}

int	charcount (const char *string, char what)
{
	int x = 0;
	const char *place = string;

	while (*place)
		if (*place++ == what)
			x++;

	return x;
}

char *	encode(const char *str, int len)
{
	char *retval;
	char *ptr;

	if (len == -1)
		len = strlen(str);

	ptr = retval = new_malloc(len * 2 + 1);

	while (len)
	{
		*ptr++ = (*str >> 4) + 0x41;
		*ptr++ = (*str & 0x0f) + 0x41;
		str++;
		len--;
 	}
	*ptr = 0;
	return retval;
}

char *	decode(const char *str)
{
	char *retval;
	char *ptr;
	int len = strlen(str);

	ptr = retval = new_malloc(len / 2 + 1);
	while (len >= 2)
	{
		*ptr++ = ((str[0] - 0x41) << 4) | (str[1] - 0x41);
		str += 2;
		len -= 2;
	}
	*ptr = 0;
	return retval;
}

char *	chomp (char *s)
{
	char *e = s + strlen(s);

	if (e == s)
		return s;

	while (*--e == '\n')
	{
		*e = 0;
		if (e == s)
			break;
	}

	return s;
}

char	*strpcat (char *source, const char *format, ...)
{
	va_list args;
	char	buffer[BIG_BUFFER_SIZE + 1];

	va_start(args, format);
	vsnprintf(buffer, BIG_BUFFER_SIZE, format, args);
	va_end(args);

	strcat(source, buffer);
	return source;
}

char *	strmpcat (char *source, size_t siz, const char *format, ...)
{
	va_list args;
	char	buffer[BIG_BUFFER_SIZE + 1];

	va_start(args, format);
	vsnprintf(buffer, BIG_BUFFER_SIZE, format, args);
	va_end(args);

	strmcat(source, buffer, siz);
	return source;
}



u_char	*strcpy_nocolorcodes (u_char *dest, const u_char *source)
{
	u_char	*save = dest;

	do
	{
		while (*source == 3)
			source = skip_ctl_c_seq(source, NULL, NULL, 0);
		*dest++ = *source;
	}
	while (*source++);

	return save;
}

/*
 * This mangles up 'incoming' corresponding to the current values of
 * /set mangle_inbound or /set mangle_outbound
 */
void	mangle_line	(char *incoming, int how, int len)
{
	int	stuff;
	char	*buffer;
	int	i = 0;
	char	*s;

	stuff = how;
	buffer = alloca(strlen(incoming) * 2 + 1);	/* Absurdly large */
	*buffer = 0;

	if (stuff & MANGLE_ESCAPES)
	{
		for (i = 0; incoming[i]; i++)
		{
			if (incoming[i] == 0x1b)
				incoming[i] = 0x5b;
		}
	}

	if (stuff & MANGLE_ANSI_CODES)
	{
		/* strip_ansi always shrinks the string */
		char *output;

		strip_ansi_never_xlate = 1;	/* XXXXX */
		output = strip_ansi(incoming);
		strip_ansi_never_xlate = 0;	/* XXXXX */
		strncpy(incoming, output, len);
		new_free(&output);
	}

	/*
	 * Now we mangle the individual codes
	 */
	for (i = 0, s = incoming; *s && i < len; s++)
	{
		switch (*s)
		{
			case 003:		/* color codes */
			{
				int 		lhs = 0, 
						rhs = 0;
				char 		*end;

				end = (char *)skip_ctl_c_seq(s, &lhs, &rhs, 0);
				if (!(stuff & STRIP_COLOR))
				{
					while (s < end)
						buffer[i++] = *s++;
				}
				s = end - 1;
				break;
			}
			case ALT_TOG:		/* Alternate character set */
			{
				if (!(stuff & STRIP_ALT_CHAR))
					buffer[i++] = ALT_TOG;
				break;
			}
			case REV_TOG:		/* Reverse */
			{
				if (!(stuff & STRIP_REVERSE))
					buffer[i++] = REV_TOG;
				break;
			}
			case UND_TOG:		/* Underline */
			{
				if (!(stuff & STRIP_UNDERLINE))
					buffer[i++] = UND_TOG;
				break;
			}
			case BOLD_TOG:		/* Bold */
			{
				if (!(stuff & STRIP_BOLD))
					buffer[i++] = BOLD_TOG;
				break;
			}
			case BLINK_TOG: 	/* Flashing */
			{
				if (!(stuff & STRIP_BLINK))
					buffer[i++] = BLINK_TOG;
				break;
			}
			case ROM_CHAR:		/* Special rom-chars */
			{
				if (!(stuff & STRIP_ROM_CHAR))
					buffer[i++] = ROM_CHAR;
				break;
			}
			case ND_SPACE:		/* Nondestructive spaces */
			{
				if (!(stuff & STRIP_ND_SPACE))
					buffer[i++] = ND_SPACE;
				break;
			}
			case ALL_OFF:		/* ALL OFF attribute */
			{
				if (!(stuff & STRIP_ALL_OFF))
					buffer[i++] = ALL_OFF;
  				break;
  			}
			default:
				buffer[i++] = *s;
		}
	}
	buffer[i] = 0;
	strncpy(incoming, buffer, len);
}

void strip_chars(char *buffer, char *strip, char replace)
{
char *p;
	if (!buffer || !*buffer || !strip || !*strip || !replace)
		return;
	while (*strip)
	{
		while ((p = strchr(buffer, *strip)))
			*p = replace;
		strip++;
	}
}

char *longcomma(long val)
{
char buffer[40];
static char buff[40];
char *s = buff;
int i = 0, j = 0, len;
	sprintf(buffer, "%ld", val);
	len = strlen(buffer);
	for (i = len % 3; i > 0; i--)
		*s++ = buffer[j++];	 
	if (len > 3 && len % 3)
		*s++ = ',';
	len -= (len % 3);	
	while (len --)
	{
		*s++ = buffer[j++];
		if (!(len % 3) && len)
			*s++ = ',';
	}
	*s = 0;
	return buff;
}

char *ulongcomma(unsigned long val)
{
char buffer[40];
static char buff[40];
char *s = buff;
int i = 0, j = 0, len;
	sprintf(buffer, "%lu", val);
	len = strlen(buffer);
	for (i = len % 3; i > 0; i--)
		*s++ = buffer[j++];	 
	if (len > 3 && len % 3)
		*s++ = ',';
	len -= (len % 3);	
	while (len --)
	{
		*s++ = buffer[j++];
		if (!(len % 3) && len)
			*s++ = ',';
	}
	*s = 0;
	return buff;
}

char *slongcomma(char *val)
{
static char buff[80];
char *s = buff;
int i = 0, j = 0, len;
	len = strlen(val);
	for (i = len % 3; i > 0; i--)
		*s++ = val[j++];	 
	if (len > 3 && len % 3)
		*s++ = ',';
	len -= (len % 3);	
	while (len --)
	{
		*s++ = val[j++];
		if (!(len % 3) && len)
			*s++ = ',';
	}
	*s = 0;
	return buff;
}

/* RANDOM NUMBERS */
/*
 * Random number generator #1 -- psuedo-random sequence
 * If you do not have /dev/random and do not want to use gettimeofday(), then
 * you can use the psuedo-random number generator.  Its performance varies
 * from weak to moderate.  It is a predictable mathematical sequence that
 * varies depending on the seed, and it provides very little repetition,
 * but with 4 or 5 samples, it should be trivial for an outside person to
 * find the next numbers in your sequence.
 *
 * If 'l' is not zero, then it is considered a "seed" value.  You want 
 * to call it once to set the seed.  Subsequent calls should use 'l' 
 * as 0, and it will return a value.
 */
static	unsigned long	randm (unsigned long l)
{
	/* patch from Sarayan to make $rand() better */
static	const	long	RAND_A = 16807L;
static	const	long	RAND_M = 2147483647L;
static	const	long	RAND_Q = 127773L;
static	const	int	RAND_R = 2836L;
static		u_long	z = 0;
		long	t;

	if (z == 0)
		z = (u_long) getuid();

	if (l == 0)
	{
		t = RAND_A * (z % RAND_Q) - RAND_R * (z / RAND_Q);
		if (t > 0)
			z = t;
		else
			z = t + RAND_M;
		return (z >> 8) | ((z & 255) << 23);
	}
	else
	{
		if (l < 0)
			z = (u_long) getuid();
		else
			z = l;
		return 0;
	}
}

/*
 * Random number generator #2 -- gettimeofday().
 * If you have gettimeofday(), then we could use it.  Its performance varies
 * from weak to moderate.  At best, it is a source of modest entropy, with 
 * distinct linear qualities. At worst, it is a linear sequence.  If you do
 * not have gettimeofday(), then it uses randm() instead.
 */
static unsigned long randt_2 (void)
{
	struct timeval 	tp1;
	get_time(&tp1);
	return (unsigned long) tp1.tv_usec;
}

static	unsigned long randt (unsigned long l)
{
#ifdef HAVE_GETTIMEOFDAY
	unsigned long t1, t2, t;

	if (l != 0)
		return 0;

	t1 = randt_2();
	t2 = randt_2();
	t = (t1 & 65535) * 65536 + (t2 & 65535);
	return t;
#else
	return randm(0);
#endif
}


/*
 * Random number generator #3 -- /dev/urandom.
 * If you have the /dev/urandom device, then we will use it.  Its performance
 * varies from moderate to very strong.  At best, it is a source of pretty
 * substantial unpredictable numbers.  At worst, it is mathematical psuedo-
 * random sequence (which randm() is).
 */
static unsigned long randd (unsigned long l)
{
	unsigned long	value;
static	int		random_fd = -1;

	if (l != 0)
		return 0;	/* No seeding appropriate */

	if (random_fd == -2)
		return randm(0);

	else if (random_fd == -1)
	{
		if ((random_fd = open("/dev/urandom", O_RDONLY)) == -1)
		{
			random_fd = -2;
			return randm(0);	/* Fall back to randm */
		}
	}

	read(random_fd, (void *)&value, sizeof(value));
	return value;
}


unsigned long	random_number (unsigned long l)
{
	switch (get_int_var(RANDOM_SOURCE_VAR))
	{
		case 0:
		default:
			return randd(l);
		case 1:
			return randm(l);
		case 2:
			return randt(l);
	}
}

/*
 * DrScholl gave me this for use in my client. Thanks!!
 */
void compute_soundex (char *d, int dsize, const char *s)
{
    int n = 0;

    /* if it's not big enough to hold one soundex word, quit without
       doing anything */
    if (dsize < 4)
    {
	if (dsize > 0)
	    *d = 0;
	return;
    }
    dsize--; /* save room for the terminatin nul (\0) */

    while (*s && !isalpha(*s))
	s++;
    if (!*s)
    {
	*d = 0;
	return;
    }

    *d++ = toupper (*s);
    dsize--;
    s++;

    while (*s && dsize > 0)
    {
	switch (tolower (*s))
	{
	    case 'b':
	    case 'p':
	    case 'f':
	    case 'v':
		if (n < 3)
		{
		    *d++ = '1';
		    dsize--;
		    n++;
		}
		break;
	    case 'c':
	    case 's':
	    case 'k':
	    case 'g':
	    case 'j':
	    case 'q':
	    case 'x':
	    case 'z':
		if (n < 3)
		{
		    *d++ = '2';
		    dsize--;
		    n++;
		}
		break;
	    case 'd':
	    case 't':
		if (n < 3)
		{
		    *d++ = '3';
		    dsize--;
		    n++;
		}
		break;
	    case 'l':
		if (n < 3)
		{
		    *d++ = '4';
		    dsize--;
		    n++;
		}
		break;
	    case 'm':
	    case 'n':
		if (n < 3)
		{
		    *d++ = '5';
		    dsize--;
		    n++;
		}
		break;
	    case 'r':
		if (n < 3)
		{
		    *d++ = '6';
		    dsize--;
		    n++;
		}
		break;
	    default:
		if (!isalpha (*s))
		{
		    /* pad short words with 0's */
		    while (n < 3 && dsize > 0)
		    {
			*d++ = '0';
			dsize--;
			n++;
		    }
		    n = 0; /* reset */
		    /* skip forward until we find the next word */
		    s++;
		    while (*s && !isalpha (*s))
			s++;
		    if (!*s)
		    {
			*d = 0;
			return;
		    }
		    if (dsize > 0)
		    {
			*d++ = ',';
			dsize--;
			if (dsize > 0)
			{
			    *d++ = toupper (*s);
			    dsize--;
			}
		    }
		}
		/* else it's a vowel and we ignore it */
		break;
	}
	/* skip over duplicate letters */
	while (*(s+1) == *s)
	    s++;

	/* next letter */
	s++;
    }
    /* pad short words with 0's */
    while (n < 3 && dsize > 0)
    {
	*d++ = '0';
	dsize--;
	n++;
    }
    *d = 0;
}

#if 0
/*
 * next_expr finds the next expression delimited by brackets. The type
 * of bracket expected is passed as a parameter. Returns NULL on error.
 */
char	*my_next_expr(char **args, char type, int whine)
{
	char	*ptr,
		*ptr2,
		*ptr3;

	if (!*args)
		return NULL;
	ptr2 = *args;
	if (!*ptr2)
		return 0;
	if (*ptr2 != type)
	{
		if (whine)
			say("Expression syntax");
		return 0;
	}							/* { */
	ptr = MatchingBracket(ptr2 + 1, type, (type == '(') ? ')' : '}');
	if (!ptr)
	{
		say("Unmatched '%c'", type);
		return 0;
	}
	*ptr = '\0';

	do
		ptr2++;
	while (my_isspace(*ptr2));

	ptr3 = ptr+1;
	while (my_isspace(*ptr3))
		ptr3++;
	*args = ptr3;
	if (*ptr2)
	{
		ptr--;
		while (my_isspace(*ptr))
			*ptr-- = '\0';
	}
	return ptr2;
}

extern char *next_expr_failok (char **args, char type)
{
	return my_next_expr (args, type, 0);
}

extern char *next_expr (char **args, char type)
{
	return my_next_expr (args, type, 1);
}
#endif

int matchnumber(char *str, int count)
{
char *line;
int start = 0;
int end = 0;
	line = LOCAL_COPY(str);
	if (*line == '*')
		return 1;
	while (line && *line)
	{
		start = 0;
		end = 0;
		if (*line == '-')
		{
			while (*line && !isdigit(*line))
				line++;
			start = 1;
			end = my_atol(line);
			while (*line && !isdigit(*line))
				line++;
			if (end < start)
				continue;
		}
		else if (isdigit(*line))
		{
			while (*line && !isdigit(*line))
				line++;
			start = atol(line);
			while (*line && isdigit(*line))
				line++;
			if (*line == '-')
			{
				while (*line && !isdigit(*line))
					line++;
				end = my_atol(line);
				while (*line && isdigit(*line))
					line++;
				if (end < start)
					end = MAX_INT;
			}
		} else
			line++;
		if (count == start || (count >= start && count <= end))
			return 1;
	}
	if (count == start || (count >= start && count <= end))
		return 1;
	return 0;	
}

/* makes foo[one][two] look like tmp.one.two -- got it? */
char *remove_brackets (const char *name, const char *args, int *arg_flag)
{
	char *ptr, *right, *result1, *rptr, *retval = NULL;

	/* XXXX - ugh. */
	rptr = m_strdup(name);

	while ((ptr = strchr(rptr, '[')))
	{
		*ptr++ = 0;
		right = ptr;
		if ((ptr = MatchingBracket(right, '[', ']')))
			*ptr++ = 0;

		if (args)
			result1 = expand_alias(right, args, arg_flag, NULL);
		else
			result1 = right;

		retval = m_3dup(rptr, ".", result1);
		if (ptr)
			malloc_strcat(&retval, ptr);

		if (args)
			new_free(&result1);
		if (rptr)
			new_free(&rptr);
		rptr = retval;
	}
	return upper(rptr);
}


/* is_number: returns 1 if the given string is a real number, 0 otherwise */
int	is_real_number (const char *str)
{
	int	dot = 0;

	if (!str || !*str)
		return 0;

	while (*str == ' ')
		str++;

	if (*str == '-')
		str++;

	if (!*str)
		return 0;

	for (; *str; str++)
	{
		if (isdigit((*str)))
			continue;

		if (*str == '.' && dot == 0)
		{
			dot = 1;
			continue;
		}

		return 0;
	}

	return 1;
}

/*
 * A "forward quote" is the first double quote we find that has a space
 * or the end of string after it; or the end of the string.
 */
const char *	find_forward_quote (const char *input, const char *start)
{
	/*
	 * An "extended word" is defined as:
	 *	<SPACE> <QUOTE> <ANY>* <QUOTE> <SPACE>
	 * <SPACE> := <WORD START> | <WORD END> | <" "> | <"\t"> | <"\n">
	 * <QUOTE> :- <'"'>
	 * <ANY>   := ANY ASCII CHAR 0 .. 256
	 */
	/* Make sure we are actually looking at a double-quote */
	if (*input != '"')
		return NULL;

	/* 
	 * Make sure that the character before is the start of the
	 * string or that it is a space.
	 */
	if (input > start && !risspace(input[-1]))
		return NULL;

	/*
	 * Walk the string looking for a double quote.  Yes, we do 
	 * really have to check for \'s, still, because the following
	 * still is one word:
	 *			"one\" two"
	 * Once we find a double quote, then it must be followed by 
	 * either the end of string (chr 0) or a space.  If we find 
	 * that, return the position of the double-quote.  
	 */
	for (input++; *input; input++)
	{
		if (input[0] == '\\' && input[1])
			input++;
		else if (input[0] == '"')
		{
			if (input[1] == 0)
				return input;
			else if (risspace(input[1]))
				return input;
		}
	}

	/*
	 * If we get all the way to the end of the string w/o finding 
	 * a matching double-quote, return the position of the (chr 0), 
	 * which is a special meaning to the caller. 
	 */
	return input;		/* Bumped the end of string. doh! */
}

/*
 * A "backward quote" is the first double quote we find, going backward,
 * that has a space or the start of string after it; or the start of 
 * the string.
 */
const char *	find_backward_quote (const char *input, const char *start)
{
	const char *	saved_input = input;

	/*
	 * An "extended word" is defined as:
	 *	<SPACE> <QUOTE> <ANY>* <QUOTE> <SPACE>
	 * <SPACE> := <WORD START> | <WORD END> | <" "> | <"\t"> | <"\n">
	 * <QUOTE> :- <'"'>
	 * <ANY>   := ANY ASCII CHAR 0 .. 256
	 */
	/* Make sure we are actually looking at a double-quote */
	if (input[0] != '"')
		return NULL;

	/* 
	 * Make sure that the character after is either the end of the
	 * string, or that it is a space.
	 */
	if (input[1] && !risspace(input[1]))
		return NULL;

	/*
	 * Walk the string looking for a double quote.  Yes, we do 
	 * really have to check for \'s, still, because the following
	 * still is one word:
	 *			"one\" two"
	 * Once we find a double quote, then it must be followed by 
	 * either the end of string (chr 0) or a space.  If we find 
	 * that, return the position of the double-quote.  
	 */
	for (input--; input > start; input--)
	{
		if (input[0] == '\\' && input[1])
			input++;
		else if (input[0] == '"')
		{
			if (input[1] == 0)
				return input;
			else if (risspace(input[1]))
				return input;
		}
	}

	/*
	 * If we get all the way to the start of the string w/o finding 
	 * a matching double-quote, then THIS IS NOT AN EXTENDED WORD!
	 * We need to re-do this word entirely by starting over and looking
	 * for a normal word.
	 */
	input = saved_input;
	while (input > start && !risspace(input[0]))
		input--;

	if (risspace(input[0]))
		input++;		/* Just in case we've gone too far */

	return input;		/* Wherever we are is fine. */
}

char *off_on(int val)
{
static char *onoff[] = { "off", "on" };
	if (!val)
		return onoff[val];
	return onoff[val];
}

#if 0
/* #define MEM_DEBUG 1 */

#ifdef MEM_DEBUG
#include <dmalloc.h>
#endif

#define alloc_start(ptr) ((ptr) - sizeof(void *))
#define alloc_size(ptr) (*(int *)( alloc_start((ptr)) ))

#define FREE_DEBUG 1
#define FREED_VAL -3
#define ALLOC_MAGIC 0xafbdce70

char compress_buffer[10000];

void start_memdebug(void)
{
#ifdef MEM_DEBUG
	dmalloc_debug(/*0x2202*/0x14f41d83);
#endif
}

/*
 * really_new_malloc is the general interface to the malloc(3) call.
 * It is only called by way of the ``new_malloc'' #define.
 * It wont ever return NULL.
 */

/*
 * Malloc allocator with size caching.
 */
void * n_malloc (size_t size, const char *module, const char *file, const int line)
{
	char	*ptr;

#ifdef MEM_DEBUG
	if (!(ptr = (char *)_calloc_leap(file, line, 1, size+sizeof(void *))))
#else
	if (!(ptr = (char *)calloc(1, size+sizeof(void *)+sizeof(void *))))
#endif
	{
		yell("Malloc() failed, giving up! %s %d", file, line);
		term_reset();
		exit(1);
	}
	/* Store the size of the allocation in the buffer. */
	ptr += sizeof(void *);
	alloc_size(ptr) = size;
	return ptr;
}

/*
 * new_free:  Why do this?  Why not?  Saves me a bit of trouble here and there 
 */
void *	n_free(void **ptr, const char *module, const char *file, const int line)
{
	if (*ptr)
	{
#ifdef FREE_DEBUG
		/* Check to make sure its not been freed before */
		if (alloc_size(*ptr) == FREED_VAL)
		{
			yell("free()ing a already free'd pointer, giving up! %s %d", file, line);
			term_reset();
			exit(1);
		}
#endif
		alloc_size(*ptr) = FREED_VAL;

#ifdef MEM_DEBUG
		_free_leap(file, line, (void *)alloc_start(*ptr));
#else
		free((void *)alloc_start(*ptr));
#endif
		*ptr = NULL;
	}
	return (*ptr);
}

void * n_realloc (void **ptr, size_t size, const char *module, const char *file, const int line)
{
	char *ptr2 = NULL;

	if (*ptr)
	{
		if (size)
		{
			size_t msize = alloc_size(*ptr);

			if (msize >= size)
				return *ptr;

			ptr2 = n_malloc(size, module, file, line);
			memmove(ptr2, *ptr, msize);
			n_free(ptr, module, file, line);
			return ((*ptr = ptr2));
		}
		n_free(ptr, module, file, line);
		return NULL;
	} 
	else if (size)
		ptr2 = n_malloc(size, module, file, line);
	return ((*ptr = ptr2));
}

/*
 * malloc_strcpy:  Mallocs enough space for src to be copied in to where
 * ptr points to.
 *
 * Never call this with ptr pointinng to an uninitialised string, as the
 * call to new_free() might crash the client... - phone, jan, 1993.
 */
char *	n_malloc_strcpy (char **ptr, const char *src, const char *module, const char *file, const int line)
{
	if (!src)
		return n_free((void **)ptr, module, file, line);
	if (ptr && *ptr)
	{
		if (*ptr == src)
			return *ptr;
		if (alloc_size(*ptr) > strlen(src))
			return strcpy(*ptr, src);
		n_free((void **)ptr, module, file, line);
	}
	*ptr = n_malloc(strlen(src) + 1, module, file, line);
	return strcpy(*ptr, src);
	return *ptr;
}

/* malloc_strcat: Yeah, right */
char *	n_malloc_strcat (char **ptr, const char *src, const char *module, const char *file, const int line)
{
	size_t  msize;

	if (*ptr)
	{
		if (!src)
			return *ptr;
		msize = strlen(*ptr) + strlen(src) + 1;
		n_realloc((void **)ptr, sizeof(char)*msize, file, line);
		return strcat(*ptr, src);
	}
	return (*ptr = n_m_strdup(src, module, file, line));
}

char *malloc_str2cpy(char **ptr, const char *src1, const char *src2)
{
	if (!src1 && !src2)
		return new_free(ptr);

	if (*ptr)
	{
		if (alloc_size(*ptr) > strlen(src1) + strlen(src2))
			return strcat(strcpy(*ptr, src1), src2);
		new_free(ptr);
	}

	*ptr = new_malloc(strlen(src1) + strlen(src2) + 1);
	return strcat(strcpy(*ptr, src1), src2);
}

#endif

void my_encrypt (char *str, int len, char *key)
{
	int	key_len,
		key_pos,
		i;
	char	mix,
		tmp;

	if (!key)
		return;
		
	key_len = strlen(key);
	key_pos = 0;
	mix = 0;
	for (i = 0; i < len; i++)
	{
		tmp = str[i];
		str[i] = mix ^ tmp ^ key[key_pos];
		mix ^= tmp;
		key_pos = (key_pos + 1) % key_len;
	}
	str[i] = (char) 0;
}

void my_decrypt(char *str, int len, char *key)
{
	int	key_len,
		key_pos,
		i;
	char	mix,
		tmp;

	if (!key)
		return;
		
	key_len = strlen(key);
	key_pos = 0;
	/*    mix = key[key_len-1]; */
	mix = 0;
	for (i = 0; i < len; i++)
	{
		tmp = mix ^ str[i] ^ key[key_pos];
		str[i] = tmp;
		mix ^= tmp;
		key_pos = (key_pos + 1) % key_len;
	}
	str[i] = (char) 0;
}

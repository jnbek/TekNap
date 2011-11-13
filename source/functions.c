/* $Id: functions.c,v 1.1.1.1 2001/02/19 03:23:59 edwards Exp $ */
 
#include "teknap.h"
#include "struct.h"
#include "alias.h"
#include "array.h"
#include "files.h"
#include "list.h"
#include "input.h"
#include "napster.h"
#include "newio.h"
#include "output.h"
#include "ircaux.h"
#include "ircterm.h"
#include "server.h"
#include "window.h"
#include <sys/stat.h>
#ifdef HAVE_REGEX_H
# include <regex.h>
#endif
#ifdef HAVE_UNAME
# include <sys/utsname.h>
#endif

extern char *recv_nick, *sent_nick;

BUILT_IN_FUNCTION(nap_channel);
BUILT_IN_FUNCTION(cparse);
BUILT_IN_FUNCTION(function_time);
BUILT_IN_FUNCTION(function_mp3time);
BUILT_IN_FUNCTION(function_md5);
BUILT_IN_FUNCTION(function_channelnicks);
BUILT_IN_FUNCTION(nap_nick);
BUILT_IN_FUNCTION(nap_server);
BUILT_IN_FUNCTION(function_sar);
BUILT_IN_FUNCTION(function_msar);
BUILT_IN_FUNCTION(function_match);
BUILT_IN_FUNCTION(function_rmatch);
BUILT_IN_FUNCTION(function_midw);
BUILT_IN_FUNCTION(function_rightw);
BUILT_IN_FUNCTION(function_leftw);
BUILT_IN_FUNCTION(function_mid);
BUILT_IN_FUNCTION(function_right);
BUILT_IN_FUNCTION(function_left);
BUILT_IN_FUNCTION(function_after);
BUILT_IN_FUNCTION(function_beforew);
BUILT_IN_FUNCTION(function_tow);
BUILT_IN_FUNCTION(function_before);
BUILT_IN_FUNCTION(function_chr);
BUILT_IN_FUNCTION(function_ascii);
BUILT_IN_FUNCTION(nap_fromserver);
BUILT_IN_FUNCTION(function_sort);
BUILT_IN_FUNCTION(function_numsort);
BUILT_IN_FUNCTION(function_glob);
BUILT_IN_FUNCTION(function_twiddle);
BUILT_IN_FUNCTION(function_regexec);
BUILT_IN_FUNCTION(function_regcomp);
BUILT_IN_FUNCTION(function_regerror);
BUILT_IN_FUNCTION(function_regfree);
BUILT_IN_FUNCTION(function_getopt);
BUILT_IN_FUNCTION(function_builtin);
BUILT_IN_FUNCTION(function_strchar);
BUILT_IN_FUNCTION(function_getcap);
BUILT_IN_FUNCTION(function_isdisplaying);
BUILT_IN_FUNCTION(function_connect);

BUILT_IN_FUNCTION(function_open);
BUILT_IN_FUNCTION(function_read);
BUILT_IN_FUNCTION(function_write);
BUILT_IN_FUNCTION(function_eof);
BUILT_IN_FUNCTION(function_close);

BUILT_IN_FUNCTION(function_toupper);
BUILT_IN_FUNCTION(function_tolower);

BUILT_IN_FUNCTION(function_dollar);
BUILT_IN_FUNCTION(function_strip);
BUILT_IN_FUNCTION(function_translate);
BUILT_IN_FUNCTION(function_encode);
BUILT_IN_FUNCTION(function_decode);
BUILT_IN_FUNCTION(function_strftime);
BUILT_IN_FUNCTION(function_word);
BUILT_IN_FUNCTION(function_reverse);
BUILT_IN_FUNCTION(function_revw);
BUILT_IN_FUNCTION(nap_server_version);
BUILT_IN_FUNCTION(nap_server_level);
BUILT_IN_FUNCTION(function_unlink);

BUILT_IN_FUNCTION(function_pattern);
BUILT_IN_FUNCTION(function_rpattern);
BUILT_IN_FUNCTION(function_filter);
BUILT_IN_FUNCTION(function_rfilter);

BUILT_IN_FUNCTION(function_winnum);
BUILT_IN_FUNCTION(function_winsize);
BUILT_IN_FUNCTION(function_rand);
BUILT_IN_FUNCTION(function_srand);
BUILT_IN_FUNCTION(function_strcmp);
BUILT_IN_FUNCTION(function_channels);
BUILT_IN_FUNCTION(function_fexist);
BUILT_IN_FUNCTION(function_fsize);
BUILT_IN_FUNCTION(teknap_version);
BUILT_IN_FUNCTION(teknap_version1);
BUILT_IN_FUNCTION(function_onchannel);
BUILT_IN_FUNCTION(function_xmms);
BUILT_IN_FUNCTION(function_numwords);
BUILT_IN_FUNCTION(function_strlen);
BUILT_IN_FUNCTION(function_afterw);
BUILT_IN_FUNCTION(function_lastlog);
BUILT_IN_FUNCTION(function_line);
BUILT_IN_FUNCTION(function_uname);
BUILT_IN_FUNCTION(function_pop);
BUILT_IN_FUNCTION(function_push);
BUILT_IN_FUNCTION(function_shift);
BUILT_IN_FUNCTION(function_unshift);
BUILT_IN_FUNCTION(function_jot);

BUILT_IN_FUNCTION(function_isalpha);
BUILT_IN_FUNCTION(function_isdigit);
BUILT_IN_FUNCTION(function_isalnum);
BUILT_IN_FUNCTION(function_isspace);
BUILT_IN_FUNCTION(function_isxdigit);
BUILT_IN_FUNCTION(function_findw);

BUILT_IN_FUNCTION(function_rest);
BUILT_IN_FUNCTION(function_restw);
BUILT_IN_FUNCTION(function_remw);
BUILT_IN_FUNCTION(function_insertw);
BUILT_IN_FUNCTION(function_chngw);
BUILT_IN_FUNCTION(function_common);
BUILT_IN_FUNCTION(function_diff);
BUILT_IN_FUNCTION(function_remws);
BUILT_IN_FUNCTION(function_status);
BUILT_IN_FUNCTION(alias_currdir);
BUILT_IN_FUNCTION(alias_input);
BUILT_IN_FUNCTION(function_crypt);
BUILT_IN_FUNCTION(alias_idle);
BUILT_IN_FUNCTION(alias_online);
BUILT_IN_FUNCTION(alias_sent_nick);
BUILT_IN_FUNCTION(alias_recv_nick);
BUILT_IN_FUNCTION(function_chmod);
BUILT_IN_FUNCTION(function_servernick);
BUILT_IN_FUNCTION(function_isirc);
BUILT_IN_FUNCTION(function_index);
BUILT_IN_FUNCTION(function_rindex);
BUILT_IN_FUNCTION(function_wordtoindex);
BUILT_IN_FUNCTION(function_longtoip);
BUILT_IN_FUNCTION(function_iptolong);
BUILT_IN_FUNCTION(function_fromw);
BUILT_IN_FUNCTION(function_timer);

typedef struct {
	char *name;
	char *(*func)(char *, char *);
} NapFunc;

NapFunc built_in[] = {
	{ ".",			alias_sent_nick },
	{ ",",			alias_recv_nick },
	{ "C",			nap_channel },
	{ "E",			alias_idle },
	{ "F",			nap_fromserver },
	{ "G",			alias_online },
	{ "I",			alias_input },
	{ "J",			teknap_version },
	{ "L",			nap_server_level },
	{ "N",			nap_nick },
	{ "O",			nap_server_version },
	{ "S",			nap_server },
	{ "T",			function_time },
	{ "W",			alias_currdir },
	{ "$",  		function_dollar },
	{ "v",			teknap_version1 },
	{ NULL, 		NULL }
};

NapFunc nap_func[] = {
	{ "AFTER",		function_after },
	{ "AFTERW",		function_afterw },
	{ "ALIASCTL",		aliasctl },
	{ "ASCII",		function_ascii },
	{ "BEFORE",		function_before },
	{ "BEFOREW",		function_beforew },
	{ "BUILTIN",		function_builtin },
	{ "CHANNEL",		nap_channel },
	{ "CHANNELNICKS",	function_channelnicks },
	{ "CHANUSERS",		function_onchannel },
	{ "CHMOD",		function_chmod },
	{ "CHNGW",		function_chngw },
	{ "CHR",		function_chr },
	{ "CLOSE",		function_close },
	{ "COMMON",		function_common },
	{ "CONNECT",		function_connect },
	{ "CPARSE",		cparse },
	{ "CRYPT",		function_crypt },
	{ "DECODE",		function_decode },
	{ "DELARRAY",		function_delarray },
	{ "DELITEM",		function_delitem }, 
	{ "DIFF",		function_diff },
	{ "EOF",		function_eof },
	{ "ENCODE",		function_encode },
	{ "FEXIST",		function_fexist },
	{ "FILTER",		function_filter },
	{ "FINDITEM",		function_finditem },
	{ "FINDW",		function_findw },
	{ "FROMW",		function_fromw },
	{ "FSIZE",		function_fsize },
	{ "GETARRAYS",		function_getarrays },
	{ "GETCAP",		function_getcap },
	{ "GETITEM",		function_getitem },
	{ "GETMATCHES",		function_getmatches },
	{ "GETOPT",		function_getopt },
	{ "GETRMATCHES",	function_getrmatches },
	{ "GETTMATCH",		function_gettmatch },
	{ "GLOB",		function_glob },
	{ "GLOBI",		function_glob },
	{ "IDLE",		alias_idle },
	{ "IFINDFIRST",		function_ifindfirst },
	{ "IFINDITEM",		function_ifinditem },
	{ "IGETITEM",		function_igetitem },
	{ "IGETMATCHES",	function_igetmatches },
	{ "IGETRMATCHES",	function_getrmatches },
	{ "INDEX",		function_index },
	{ "INDEXTOITEM",	function_indextoitem },
	{ "INSERTW",		function_insertw },
	{ "IPTOLONG",		function_iptolong },
	{ "ISALNUM",		function_isalnum },
	{ "ISALPHA",		function_isalpha },
	{ "ISDIGIT",		function_isdigit },
	{ "ISDISPLAYING",	function_isdisplaying },
	{ "ISIRC",		function_isirc },
	{ "ISSPACE",		function_isspace },
	{ "ISXDIGIT",		function_isxdigit },
	{ "ITEMTOINDEX",	function_itemtoindex },
	{ "JOT",		function_jot },
	{ "LASTLOG",		function_lastlog },
	{ "LEFT",		function_left },
	{ "LEFTW",		function_leftw },
	{ "LINE",		function_line },
	{ "LISTARRAY",		function_listarray },
	{ "LONGTOIP",		function_longtoip },
	{ "NUMARRAYS",		function_numarrays },
	{ "NUMITEMS",		function_numitems },
	{ "NUMSORT",		function_numsort },
	{ "NUMWORDS",		function_numwords },
	{ "MATCH",		function_match },
	{ "MATCHITEM",		function_matchitem },
	{ "MD5",		function_md5},
	{ "MID",		function_mid },
	{ "MIDW",		function_midw },
	{ "MP3TIME",		function_mp3time },
	{ "MSAR",		function_msar },
	{ "ONCHANNELS",		function_channels },
	{ "OPEN",		function_open },
	{ "PATTERN",		function_pattern },
	{ "POP",		function_pop },
	{ "PUSH",		function_push },
	{ "RAND",		function_rand },
	{ "READ",		function_read },
	{ "REGCOMP",		function_regcomp },
	{ "REGERROR",		function_regerror },
	{ "REGEXEC",		function_regexec },
	{ "REGFREE",		function_regfree },
	{ "REMW",		function_remw },
	{ "REMWS",		function_remws },
	{ "REST",		function_rest },
	{ "RESTW",		function_restw },
	{ "REVERSE",		function_reverse },
	{ "REVW",		function_revw },
	{ "RFILTER",		function_rfilter },
	{ "RIGHT",		function_right },
	{ "RIGHTW",		function_rightw },
	{ "RINDEX",		function_rindex },
	{ "RMATCH",		function_rmatch },
	{ "RMATCHITEM",		function_rmatchitem },
	{ "RPATTERN",		function_rpattern },
	{ "SAR",		function_sar },
	{ "SERVERNICK",		function_servernick },
	{ "SETITEM",		function_setitem },
	{ "SHIFT",		function_shift },
	{ "SORT",		function_sort },
	{ "SRAND",		function_srand },
	{ "STATUS",		function_status },
	{ "STRCHR",		function_strchar },
	{ "STRCMP",		function_strcmp },
	{ "STRIP",		function_strip },
	{ "STRFTIME",		function_strftime },
	{ "STRLEN",		function_strlen },
	{ "STRRCHR",		function_strchar },
	{ "TIME",		function_time },
	{ "TIMER",		function_timer },
	{ "TOLOWER",		function_tolower },
	{ "TOUPPER",		function_toupper },
	{ "TOW",		function_tow },
	{ "TR",			function_translate },
	{ "TWIDDLE",		function_twiddle },
	{ "UNAME",		function_uname },
	{ "UNLINK",		function_unlink },
	{ "UNSHIFT",		function_unshift },
	{ "UTIME",		function_time },
	{ "WINNUM",		function_winnum },
	{ "WINSIZE",		function_winsize },
	{ "WORD",		function_word },
	{ "WORDTOINDEX",	function_wordtoindex },
	{ "WRITE",		function_write },
	{ "XMMS",		function_xmms },
	{ NULL,			NULL }
};

#define	NUMBER_OF_FUNCTIONS (sizeof(nap_func) / sizeof(NapFunc)) - 1


NapFunc *find_func_alias(char *name)
{
int i = 0;
	while (nap_func[i].func && i <= NUMBER_OF_FUNCTIONS)
	{
		if (!my_stricmp(name, nap_func[i].name))
			return &nap_func[i];
		i++;
	}
	return NULL;
}


char **get_builtins(char *name, int *cnt)
{
char *last_match = NULL;
int matches_size = 5;
int i = 0;
int len;
char **matches = NULL;

	len = strlen(name);
	*cnt = 0;
	matches = RESIZE(matches, char *, matches_size);
        while (nap_func[i].func && i <= NUMBER_OF_FUNCTIONS)
	{
		if (strncmp(name, nap_func[i].name, len) == 0)
		{
			matches[*cnt] = NULL;
			malloc_strcpy(&(matches[*cnt]), nap_func[i].name);
			last_match = matches[*cnt];
			if (++(*cnt) == matches_size)
			{
				matches_size += 5;
				matches = (char	**) RESIZE(matches, char *, matches_size);
			}
		}
		else if (*cnt)
			break;
		i++;
	}
	return matches;
}


char	*built_in_alias (char c, int *returnval)
{
	NapFunc	*tmp;

	for (tmp = built_in;tmp->name;tmp++)
	{
		if (c == *(tmp->name))
		{
			if (returnval)
			{
				*returnval = 1;
				return NULL;
			}
			else
				return tmp->func(tmp->name, NULL);
		}
	}
	return NULL;
}

char	*call_function (char *name, const char *args, int *args_flag)
{
	char	*tmp;
	char	*result = NULL;
	NapFunc *funcptr = NULL;
	char	*lparen, *rparen;
	char	*debug_copy = NULL;
	
	if ((lparen = strchr(name, '(')))
	{
		if ((rparen = MatchingBracket(lparen + 1, '(', ')')))
			*rparen++ = 0;
		else
			yell("Unmatched lparen in function call [%s]", name);

		*lparen++ = 0;
 	}
	else
		lparen = empty_string;

	tmp = expand_alias(lparen, args, args_flag, NULL);

	if ((internal_debug & DEBUG_FUNC) && !in_debug_yell)
		debug_copy = LOCAL_COPY(tmp);

	upper(name);
	if ((funcptr = find_func_alias(name)))
		result = funcptr->func(name, tmp);
	else
		result = call_user_function(name, tmp);
	if (debug_copy && alias_debug)
		debugyell("%3d %s(%s) -> %s", debug_count++, name, debug_copy, result);

	new_free(&tmp);
	return result;
}

ChannelStruct *lookup_channel(char *chan, int server)
{
Window *tmp = NULL;
	while (traverse_all_windows(&tmp))
	{
		ChannelStruct *ch;
		if (server > -1 && tmp->server != server)
			continue;
		for (ch = tmp->nchannels; ch; ch = ch->next)
		{
			if (!my_stricmp(chan, ch->channel))
				return ch;
		}
	}
	return NULL;
}

BUILT_IN_FUNCTION(nap_fromserver)
{
	return m_sprintf("%d", from_server);
}

BUILT_IN_FUNCTION(nap_nick)
{
	if (from_server != -1)
		return m_strdup(get_server_nickname(from_server));
	return m_strdup(empty_string);
}

BUILT_IN_FUNCTION(alias_input)
{
	return m_strdup(get_input());
}

BUILT_IN_FUNCTION(nap_channel)
{
	if (from_server != -1)
		return m_strdup(current_window->current_channel ? current_window->current_channel : zero);
	return m_strdup(zero);
}

BUILT_IN_FUNCTION(nap_server_version)
{
	return m_sprintf("%d", get_server_version(from_server));
}

BUILT_IN_FUNCTION(nap_server_level)
{
	return m_sprintf("%d", get_server_admin(from_server));
}

BUILT_IN_FUNCTION(function_dollar)
{
	return m_strdup("$");
}

BUILT_IN_FUNCTION(teknap_version)
{
	return m_strdup(nap_version);
}

BUILT_IN_FUNCTION(teknap_version1)
{
	return m_strdup(internal_version);
}

BUILT_IN_FUNCTION(alias_currdir)
{
	char 	*tmp = (char *)new_malloc(MAXPATHLEN+1);
	return getcwd(tmp, MAXPATHLEN);
}


BUILT_IN_FUNCTION(cparse)
{
	int 	i, j, max;
	char 	*output;
	int	noappend = 0, expand = 0;
	char *	stuff;

	if (*input == '"')
	{
		stuff = new_next_arg(input, &input);
		expand = 1;
	}
	else
		stuff = input;

	output = (char *)alloca(strlen(stuff) * 3 + 5);

	for (i = 0, j = 0, max = strlen(stuff); i < max; i++)
	{
		if (stuff[i] == '%')
		{
		    i++;
		    switch (stuff[i])
		    {
			case 'k':
				output[j++] = '\003';
				output[j++] = '3';
				output[j++] = '0';
				break;
			case 'r':
				output[j++] = '\003';
				output[j++] = '3';
				output[j++] = '1';
				break;
			case 'g':
				output[j++] = '\003';
				output[j++] = '3';
				output[j++] = '2';
				break;
			case 'y':
				output[j++] = '\003';
				output[j++] = '3';
				output[j++] = '3';
				break;
			case 'b':
				output[j++] = '\003';
				output[j++] = '3';
				output[j++]   = '4';
				break;
			case 'm':
			case 'p':
				output[j++] = '\003';
				output[j++] = '3';
				output[j++]   = '5';
				break;
			case 'c':
				output[j++] = '\003';
				output[j++] = '3';
				output[j++]   = '6';
				break;
			case 'w':
				output[j++] = '\003';
				output[j++] = '3';
				output[j++]   = '7';
				break;

			case 'K':
				output[j++] = '\003';
				output[j++] = '5';
				output[j++]   = '0';
				break;
			case 'R':
				output[j++] = '\003';
				output[j++] = '5';
				output[j++]   = '1';
				break;
			case 'G':
				output[j++] = '\003';
				output[j++] = '5';
				output[j++]   = '2';
				break;
			case 'Y':
				output[j++] = '\003';
				output[j++] = '5';
				output[j++]   = '3';
				break;
			case 'B':
				output[j++] = '\003';
				output[j++] = '5';
				output[j++]   = '4';
				break;
			case 'M':
			case 'P':
				output[j++] = '\003';
				output[j++] = '5';
				output[j++]   = '5';
				break;
			case 'C':
				output[j++] = '\003';
				output[j++] = '5';
				output[j++]   = '6';
				break;
			case 'W':
				output[j++] = '\003';
				output[j++] = '5';
				output[j++]   = '7';
				break;

			case '0': case '1': case '2': case '3': 
			case '4': case '5': case '6': case '7':
				output[j++] = '\003';
				output[j++] = ',';
				output[j++] = '4';
				output[j++]   = stuff[i];
				break;

			case 'F':
				output[j++] = BLINK_TOG;
				break;
			case 'n':
				output[j++] = '\003';
				output[j++] = ALL_OFF;
				break;
			case 'N':
				noappend = 1;
				break;
			case '%':
				output[j++] = stuff[i];
				break;
			default:
				output[j++] = '%';
				output[j++] = stuff[i];
				break;
		    }
		}
		else
			output[j++] = stuff[i];
	}
	if (noappend == 0)
	{
		output[j++] = '\003';
		output[j++] = '-';
		output[j++] = '1';
	}
	output[j] = 0;

	if (expand)
	{
		int	af;
		stuff = expand_alias(output, input, &af, NULL);
		RETURN_MSTR(stuff);
	}

	return output ? m_strdup(output) : m_strdup(empty_string);
}

BUILT_IN_FUNCTION(function_time)
{
	struct  timeval         tp;
	get_time(&tp);
	if (!my_stricmp("UTIME", fn))
		return m_sprintf("%lu %lu",(unsigned long)tp.tv_sec, (unsigned long)tp.tv_usec);
	return m_sprintf("%lu", tp.tv_sec);
}

BUILT_IN_FUNCTION(function_mp3time)
{
time_t t;
	t = my_atol(new_next_arg(input, &input));
	return m_strdup(mp3_time(t));
}

BUILT_IN_FUNCTION(function_md5)
{
char *fname;
unsigned long size = 0;
int r;
	fname = next_arg(input, &input);
	size = my_atol(next_arg(input, &input));
	if (!size)
		size = 292 * 1024;
	if (fname && ((r = open(fname, O_RDONLY)) != -1))
	{
		fn = calc_md5(r, size);
		close(r);
		return fn;
	}
	return m_strdup(empty_string);
}

char *return_channelnicks(char *channel)
{
ChannelStruct *ch;
NickStruct *n;
char *buffer = NULL;
	if ((ch = lookup_channel(channel, from_server)))
	{
		for (n = ch->nicks; n; n = n->next)
			m_s3cat(&buffer, " ", n->nick);
	}
	return buffer;
}

BUILT_IN_FUNCTION(function_channelnicks)
{
char *chan;
char *ret = NULL;
	if ((chan = next_arg(input, &input)))
		ret = return_channelnicks(chan);
	else if (current_window && current_window->current_channel)
		ret = return_channelnicks(current_window->current_channel);
	return ret ? ret : m_strdup(empty_string);
}

BUILT_IN_FUNCTION(nap_server)
{
	return m_strdup(get_server_name(from_server) ? get_server_name(from_server) : get_server_itsname(from_server) ? get_server_itsname(from_server) : empty_string);
}

/* Search and replace function --
   Usage:   $sar(c/search/replace/data)
   Commands:
		r - treat data as a variable name and 
		    return the replaced data to the variable
		g - Replace all instances, not just the first one
   The delimiter may be any character that is not a command (typically /)
   The delimiter MUST be the first character after the command
   Returns emppy string on error
*/
BUILT_IN_FUNCTION(function_sar)
{
	register char    delimiter;
	register char	*pointer	= NULL;
	char    *search         = NULL;
	char    *replace        = NULL;
	char    *data		= NULL;
	char	*value		= NULL;
	char	*booya		= NULL;
	int	variable = 0,global = 0,searchlen;
	char *(*func) (const char *, const char *) = strstr;
	char 	*svalue;

	while (((*input == 'r') && (variable = 1)) || ((*input == 'g') && (global = 1)) || ((*input == 'i') && (func = stristr)))
		input++;

	RETURN_IF_EMPTY(input);

	delimiter = *input;
	search = input + 1;
	if ((replace = strchr(search, delimiter)) == 0)
		RETURN_EMPTY;

	*replace++ = 0;
	if ((data = strchr(replace,delimiter)) == 0)
		RETURN_EMPTY;

	*data++ = '\0';

	value = (variable == 1) ? get_variable(data) : m_strdup(data);

	if (!value || !*value)
	{
		new_free(&value);
		RETURN_EMPTY;
	}

	pointer = svalue = value;
	searchlen = strlen(search) - 1;
	if (searchlen < 0)
		searchlen = 0;
	if (global)
	{
		while ((pointer = func(pointer,search)) != NULL)
		{
			pointer[0] = pointer[searchlen] = 0;
			pointer += searchlen + 1;
			m_e3cat(&booya, value, replace);
			value = pointer;
			if (!*pointer)
				break;
		}
	} 
	else
	{
		if ((pointer = func(pointer,search)) != NULL)
		{
			pointer[0] = pointer[searchlen] = 0;
			pointer += searchlen + 1;
			m_e3cat(&booya, value, replace);
			value = pointer;
		}
	}

	malloc_strcat(&booya, value);
	if (variable) 
		add_var_alias(data, booya, 0);
	new_free(&svalue);
	return (booya);
}

/* Search and replace function --
   Usage:   $msar(c/search/replace/data)
   Commands:
		r - treat data as a variable name and 
		    return the replaced data to the variable
		g - Replace all instances, not just the first one
   The delimiter may be any character that is not a command (typically /)
   The delimiter MUST be the first character after the command
   Returns empty string on error
*/
BUILT_IN_FUNCTION(function_msar)
{
	char    delimiter;
	char    *pointer        = NULL;
	char    *search         = NULL;
	char    *replace        = NULL;
	char    *data           = NULL;
	char    *value          = NULL;
	char    *booya          = NULL;
	char    *p              = NULL;
	int     variable 	= 0,
		global 		= 0,
		searchlen;
	char 	*(*func) (const char *, const char *) = strstr;
	char    *svalue;

        while (((*input == 'r') && (variable = 1)) || ((*input == 'g') && (global
= 1)) || ((*input == 'i') && (func = stristr)))
                input++;

        RETURN_IF_EMPTY(input);

        delimiter = *input;
        search = input + 1;
        if (!(replace = strchr(search, delimiter)))
                RETURN_EMPTY;

        *replace++ = 0;
        if (!(data = strchr(replace,delimiter)))
                RETURN_EMPTY;

        *data++ = 0;

        if (!(p = strrchr(data, delimiter)))
                value = (variable == 1) ? get_variable(data) : m_strdup(data);
        else
        {
                *p++ = 0;
                value = (variable == 1) ? get_variable(p) : m_strdup(p);
        }

        if (!value || !*value)
        {
                new_free(&value);
                RETURN_EMPTY;
        }

        pointer = svalue = value;

        do 
        {
                searchlen = strlen(search) - 1;
                if (searchlen < 0)
                        searchlen = 0;
                if (global)
                {
                        while ((pointer = func(pointer,search)))
                        {
                                pointer[0] = pointer[searchlen] = 0;
                                pointer += searchlen + 1;
                                m_e3cat(&booya, value, replace);
                                value = pointer;
                                if (!*pointer)
                                        break;
                        }
                } 
                else
                {
                        if ((pointer = func(pointer,search)))
                        {
                                pointer[0] = pointer[searchlen] = 0;
                                pointer += searchlen + 1;
                                m_e3cat(&booya, value, replace);
                                value = pointer;
                        }
                }
                malloc_strcat(&booya, value);
                if (data && *data)
                {
			new_free(&svalue);
                        search = data;
                        if ((replace = strchr(data, delimiter)))
                        {
                                *replace++ = 0;
                                if ((data = strchr(replace, delimiter)))
                                        *data++ = 0;
                        }
			/* patch from RoboHak */
			if (!replace || !search)
			{
				pointer = value = svalue;
				break;
			}
			pointer = value = svalue = booya;
			booya = NULL;
                } else 
                        break;
        } while (1);

        if (variable) 
                add_var_alias(data, booya, 0);
        new_free(&svalue);
        return (booya);
}

/*
 * Usage: $index(characters text)
 * Returns: The number of leading characters in <text> that do not occur 
 *          anywhere in the <characters> argument.
 * Example: $index(f three fine frogs) returns 6 (the 'f' in 'fine')
 *          $index(frg three fine frogs) returns 2 (the 'r' in 'three')
 */
BUILT_IN_FUNCTION(function_index)
{
	char	*schars;
	char	*iloc;

	GET_STR_ARG(schars, input);
	iloc = sindex(input, schars);
	RETURN_INT(iloc ? iloc - input : -1);
}

/*
 * Usage: $rindex(characters text)
 * Returns: The number of leading characters in <text> that occur before the
 *          *last* occurance of any of the characters in the <characters> 
 *          argument.
 * Example: $rindex(f three fine frogs) returns 12 (the 'f' in 'frogs')
 *          $rindex(frg three fine frogs) returns 15 (the 'g' in 'froGs')
 */
BUILT_IN_FUNCTION(function_rindex)
{
	char	*chars, *last;

	/* need to find out why ^x doesnt work */
	GET_STR_ARG(chars, input);
	last = rsindex(input + strlen(input) - 1, input, chars, 1);
	RETURN_INT(last ? last - input : -1);
}


/*
 * Usage: $match(pattern list of words)
 * Returns: if no words in the list match the pattern, it returns 0.
 *	    Otherwise, it returns the number of the word that most
 *	    exactly matches the pattern (first word is numbered one)
 * Example: $match(f*bar foofum barfoo foobar) returns 3
 *	    $match(g*ant foofum barfoo foobar) returns 0
 *
 * Note: it is possible to embed spaces inside of a word or pattern simply
 *       by including the entire word or pattern in quotation marks. (")
 */
BUILT_IN_FUNCTION(function_match)
{
	char	*pattern, 	*word;
	long	current_match,	best_match = 0,	match = 0, match_index = 0;

	GET_STR_ARG(pattern, input);

	while (input && *input)
	{
		while (input && my_isspace(*input))
			input++;
		match_index++;
		GET_STR_ARG(word, input);
		if ((current_match = wild_match(pattern, word)) > best_match)
		{
			match = match_index;
			best_match = current_match;
		}
	}

	RETURN_INT(match);
}

/*
 * Usage: $rmatch(word list of patterns)
 * Returns: if no pattern in the list matches the word, it returns 0.
 *	    Otherwise, it returns the number of the pattern that most
 *	    exactly matches the word (first word is numbered one)
 * Example: $rmatch(foobar f*bar foo*ar g*ant) returns 2 
 *	    $rmatch(booya f*bar foo*ar g*ant) returns 0
 * 
 * Note: It is possible to embed spaces into a word or pattern simply by
 *       including the entire word or pattern within quotation marks (")
 */
BUILT_IN_FUNCTION(function_rmatch)
{
	char	*pattern,	*word;
	int	current_match,	best_match = 0,	match = 0, rmatch_index = 0;

	GET_STR_ARG(word, input);

	while (input && *input)
	{
		while (input && my_isspace(*input))
			input++;
		rmatch_index++;
		GET_STR_ARG(pattern, input);
		if ((current_match = wild_match(pattern, word)) > best_match)
		{
			match = rmatch_index;
			best_match = current_match;
		}
		/* WARNING WARNING HACK IN PROGRESS WARNING WARNING */
		while (input && my_isspace(*input))
			input++;
	}

	RETURN_INT(match);
}

/* $before(chars string of text)
 * returns the part of "string of text" that occurs before the
 * first instance of any character in "chars"
 * EX:  $before(! hop!jnelson@iastate.edu) returns "hop"
 */
BUILT_IN_FUNCTION(function_before)
{
	char	*pointer = NULL;
	char	*chars;
	char	*tmp;
	long	numint;

	GET_STR_ARG(tmp, input);			/* DONT DELETE TMP! */
	numint = atol(tmp);

	if (numint)
	{
		GET_STR_ARG(chars, input);
	}
	else
	{
		numint = 1;
		chars = tmp;
	}

	if (numint < 0 && strlen(input))
		pointer = input + strlen(input) - 1;

	pointer = search(input, &pointer, chars, numint);

	if (!pointer)
		RETURN_EMPTY;

	*pointer = '\0';
	RETURN_STR(input);
}

/* $beforew(pattern string of words)
 * returns the portion of "string of words" that occurs before the 
 * first word that is matched by "pattern"
 * EX: $beforew(three one two three o leary) returns "one two"
 */
BUILT_IN_FUNCTION(function_beforew)
{
	int     where;
	char	*lame = (char *) 0;
	char	*placeholder;

	lame = LOCAL_COPY(input);
	where = my_atol((placeholder = function_rmatch(NULL, input)));
	new_free(&placeholder);

	if (where < 1)
		RETURN_EMPTY;

	placeholder = extractw(lame, 1, where - 1);
	return placeholder;
}
		
/* Same as above, but includes the word being matched */
BUILT_IN_FUNCTION(function_tow)
{
	int     where;
	char	*lame = (char *) 0;
	char	*placeholder;

	lame = LOCAL_COPY(input);
	where = my_atol((placeholder = function_rmatch(NULL, input)));
	new_free(&placeholder);

	if (where < 1)
		RETURN_EMPTY;

	placeholder = extractw(lame, 1, where);
	return placeholder;
}

/* $after(chars string of text)
 * returns the part of "string of text" that occurs after the 
 * first instance of any character in "chars"
 * EX: $after(! hop!jnelson@iastate.edu)  returns "jnelson@iastate.edu"
 */
BUILT_IN_FUNCTION(function_after)
{
	char	*chars;
	char	*pointer = NULL;
	char 	*tmp;
	long	numint;

	GET_STR_ARG(tmp, input);
	numint = atol(tmp);

	if (numint)
		chars = new_next_arg(input, &input);
	else
	{
		numint = 1;
		chars = tmp;
	}

	if (numint < 0 && strlen(input))
		pointer = input + strlen(input) - 1;

	pointer = search(input, &pointer, chars, numint);

	if (!pointer || !*pointer)
		RETURN_EMPTY;

	RETURN_STR(pointer + 1);
}

/* Returns the string after the word being matched */
BUILT_IN_FUNCTION(function_afterw)
{
	int     where;
	char	*lame = NULL;
	char	*placeholder;

	lame = LOCAL_COPY(input);
	where = my_atol((placeholder = function_rmatch(NULL, input)));
	new_free(&placeholder);

	if (where < 1)
		RETURN_EMPTY;
	placeholder = extractw(lame, where + 1, EOS);
	return placeholder;
}

/* Returns the string starting with the word being matched */
BUILT_IN_FUNCTION(function_fromw)
{
	int     where;
	char 	*lame = (char *) 0;
	char	*placeholder;

	lame = m_strdup(input);
	placeholder = function_rmatch(NULL, input);
	where = my_atol(placeholder);

	new_free(&placeholder);

	if (where < 1)
	{
		new_free(&lame);
		RETURN_EMPTY;
	}

	placeholder = extractw(lame, where, EOS);
	new_free(&lame);
	return placeholder;
}

/* $leftw(num string of text)
 * returns the left "num" words in "string of text"
 * EX: $leftw(3 now is the time for) returns "now is the"
 */
BUILT_IN_FUNCTION(function_leftw)
{
	int value;
 
	GET_INT_ARG(value, input);
	if (value < 1)
		RETURN_EMPTY;

	return (extractw(input, 0, value-1));	/* DONT USE RETURN_STR HERE! */
}

/* $rightw(num string of text)
 * returns the right num words in "string of text"
 * EX: $rightw(3 now is the time for) returns "the time for"
 */
BUILT_IN_FUNCTION(function_rightw)
{
	int     value;

	GET_INT_ARG(value, input);
	if (value < 1)
		RETURN_EMPTY;
		
	return extractw2(input, -value, EOS); 
}


/* $midw(start num string of text)
 * returns "num" words starting at word "start" in the string "string of text"
 * NOTE: The first word is word #0.
 * EX: $midw(2 2 now is the time for) returns "the time"
 */
BUILT_IN_FUNCTION(function_midw)
{
	int     start, num;

	GET_INT_ARG(start, input);
	GET_INT_ARG(num, input);

	if (num < 1)
		RETURN_EMPTY;

	return extractw(input, start, (start + num - 1));
}

BUILT_IN_FUNCTION(function_chr)
{
	char aboo[BIG_BUFFER_SIZE];
	unsigned char *ack = aboo;
	char *blah;

	while ((blah = next_arg(input, &input)))
		*ack++ = (unsigned char)atoi(blah);

	*ack = '\0';
	RETURN_STR(aboo);
}

BUILT_IN_FUNCTION(function_ascii)
{
	char *aboo = NULL;
	unsigned char *w = input;
	if (!input || !*input)
		RETURN_EMPTY;

	aboo = m_strdup(ltoa((unsigned long) *w));
	while (*++w)
		m_3cat(&aboo, space, ltoa((unsigned long) *w));
		
	return (aboo);
}

static int sort_it (const void *one, const void *two)
{
	return my_stricmp(*(char **)one, *(char **)two);
}

BUILT_IN_FUNCTION(function_sort)
{
	int wordc;
	char *ret;
	char **wordl;

	wordc = splitw(input, &wordl);
	qsort((void *)wordl, wordc, sizeof(char *), sort_it);
	ret = unsplitw(&wordl, wordc);
	RETURN_MSTR(ret);	/* DONT USE RETURN_STR() HERE */
}

static int num_sort_it (const void *one, const void *two)
{
	char *oneptr = *(char **)one;
	char *twoptr = *(char **)two;
	long val1, val2;

	while (*oneptr && *twoptr)
	{
		while (*oneptr && *twoptr && !isdigit(*oneptr) && !isdigit(*twoptr))
		{
			if (*oneptr != *twoptr)
				return (*oneptr - *twoptr);
			oneptr++, twoptr++;
		}

		if (!*oneptr || !*twoptr)
			break;

		val1 = strtol(oneptr, (char **)&oneptr, 10);
		val2 = strtol(twoptr, (char **)&twoptr, 10);
		if (val1 != val2)
			return val1 - val2;
	}
	return (*oneptr - *twoptr);
}

BUILT_IN_FUNCTION(function_numsort)
{
	int wordc;
	char **wordl;
	char *ret;
	wordc = splitw(input, &wordl);
	qsort((void *)wordl, wordc, sizeof(char *), num_sort_it);
	ret = unsplitw(&wordl, wordc);
	RETURN_MSTR(ret);
}

#ifdef NEED_GLOB
#define glob bsd_glob
#define globfree bsd_globfree
#endif

BUILT_IN_FUNCTION(function_glob)
{
#if defined(INCLUDE_GLOB_FUNCTION) && !defined(PUBLIC_ACCESS)
	char	*path, 
		*path2 = NULL, 
		*retval = NULL;
	int	numglobs,
		i;
	glob_t	globbers;
	int	insensitive = 0;
	
	if (!strcmp(fn, "GLOBI"))
		insensitive = GLOB_INSENSITIVE;
	memset(&globbers, 0, sizeof(glob_t));
	while (input && *input)
	{
		GET_STR_ARG(path, input);
		path2 = expand_twiddle(path);
		if (!path2)
			path2 = m_strdup(path);

		numglobs = glob(path2, GLOB_MARK | insensitive, NULL, &globbers);
		if (numglobs < 0)
		{
			new_free(&path2);
			RETURN_INT(numglobs);
		}
		for (i = 0; i < globbers.gl_pathc; i++)
		{
			if (strchr(globbers.gl_pathv[i], ' '))
			{
				int len = strlen(globbers.gl_pathv[i])+4;
				char *b = alloca(len+1);
				*b = 0;
				strmopencat(b, len, "\"", globbers.gl_pathv[i], "\"", NULL);
				m_s3cat(&retval, space, b);
			}
			else
				m_s3cat(&retval, space, globbers.gl_pathv[i]);
		}
		globfree(&globbers);
		new_free(&path2);
	}
	return retval ? retval : m_strdup(empty_string);
#else
	RETURN_EMPTY;
#endif
}

BUILT_IN_FUNCTION(function_twiddle)
{
char *ret = NULL;
	if (input && *input)
		ret = expand_twiddle(new_next_arg(input, &input));
	RETURN_MSTR(ret);
}

#if defined(HAVE_REGEXP_H) && !defined(__EMX__) && !defined(WINNT)
/*
 * These are used as an interface to regex support.  Here's the plan:
 *
 * $regcomp(<pattern>) 
 *	will return an $encode()d string that is suitable for
 * 		assigning to a variable.  The return value of this
 *		function must at some point be passed to $regfree()!
 *
 * $regexec(<compiled> <string>)
 *	Will return "0" or "1" depending on whether or not the given string
 *		was matched by the previously compiled string.  The value
 *		for <compiled> must be a value previously returned by $regex().
 *		Failure to do this will result in undefined chaos.
 *
 * $regerror(<compiled>)
 *	Will return the error string for why the previous $regmatch() or 
 *		$regex() call failed.
 *
 * $regfree(<compiled>)
 *	Will free off any of the data that might be contained in <compiled>
 *		You MUST call $regfree() on any value that was previously
 *		returned by $regex(), or you will leak memory.  This is not
 *		my problem (tm).  It returns the FALSE value.
 */

static int last_error = 0; 		/* XXX */

BUILT_IN_FUNCTION(function_regcomp)
{
	regex_t preg = {0};
	if (input && *input)
	{
		last_error = regcomp(&preg, input, REG_EXTENDED | REG_ICASE | REG_NOSUB);
		return encode((char *)&preg, sizeof(regex_t));
	}
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_regexec)
{
	char *unsaved;
	regex_t *preg = NULL;
	int retval = -1;

	GET_STR_ARG(unsaved, input);
	preg = (regex_t *)decode(unsaved);
	retval = regexec(preg, input, 0, NULL, 0);
	new_free((char **)&preg);
	RETURN_INT(retval);		/* DONT PASS FUNC CALL TO RETURN_INT */
}

BUILT_IN_FUNCTION(function_regerror)
{
	char *unsaved;
	regex_t *preg = NULL;
	char error_buf[1024];
	int errcode;

	GET_INT_ARG(errcode, input);
	GET_STR_ARG(unsaved, input);
	*error_buf = 0;
	preg = (regex_t *)decode(unsaved);
	if (errcode == -1)
		errcode = last_error;
	regerror(errcode, preg, error_buf, 1023);
	new_free((char **)&preg);
	RETURN_STR(error_buf);
}

BUILT_IN_FUNCTION(function_regfree)
{
	char *unsaved;
	regex_t *preg;

	GET_STR_ARG(unsaved, input);
	preg = (regex_t *)decode(unsaved);
	regfree(preg);
	new_free((char **)&preg);
	RETURN_EMPTY;
}
#else
BUILT_IN_FUNCTION(function_regexec)  { RETURN_EMPTY; }
BUILT_IN_FUNCTION(function_regcomp)  { RETURN_EMPTY; }
BUILT_IN_FUNCTION(function_regerror) { RETURN_STR("no regex support"); }
BUILT_IN_FUNCTION(function_regfree)  { RETURN_EMPTY; }
#endif

/*
 * Date: Wed, 2 Sep 1998 18:20:34 -0500 (CDT)
 * From: CrackBaby <crack@feeding.frenzy.com>
 */
/* 
 * $getopt(<optopt var> <optarg var> <opt string> <argument list>)
 *
 * Processes a list of switches and args.  Returns one switch char each time
 * it's called, sets $<optopt var> to the char, and sets $<optarg var> to the
 * value of the next argument if the switch needs one.
 *
 * Syntax for <opt string> and <argument list> should be identical to
 * getopt(3).  A ':' in <opt string> is a required argument, and a "::" is an
 * optional one.  A '-' by itself in <argument list> is a null argument, and
 * switch parsing stops after a "--"
 *
 * If a switch requires an argument, but the argument is missing, $getopt()
 * returns a '-'  If a switch is given which isn't in the opt string, the
 * function returns a '!'  $<optopt var> is still set in both cases.
 *
 * Example usage:
 * while (option = getopt(optopt optarg "ab:c:" $*)) {
 *	switch ($option) {
 * 		(a) {echo * option "$optopt" used}
 * 		(b) {echo * option "$optopt" used - $optarg}
 * 		(c) {echo * option "$optopt" used - $optarg}
 * 		(!) {echo * option "$optopt" is an invalid option}
 * 		(-) {echo * option "$optopt" is missing an argument}
 *	}
 * }
 */
BUILT_IN_FUNCTION(function_getopt)
{
static 	char	switches	[INPUT_BUFFER_SIZE+1] = "",
		args		[INPUT_BUFFER_SIZE+1] = "",
		last_input	[INPUT_BUFFER_SIZE+1] = "",
		*sptr 	= switches,
		*aptr 	= args;
	char	*optopt_var, 
		*optarg_var,
		*optstr,
		*optptr,
		*tmp;
	char	extra_args	[INPUT_BUFFER_SIZE+1] = "";
	int	arg_flag = 0;

	if (strcmp(last_input, input)) 
	{
		strlcpy(last_input, input, INPUT_BUFFER_SIZE);
		
		*switches = 0;
		*args = 0;
		sptr = switches;
		aptr = args;
	}	

	GET_STR_ARG(optopt_var, input); 
	GET_STR_ARG(optarg_var, input); 
	GET_STR_ARG(optstr, input);

	if (!(optopt_var || optarg_var || optstr)) 
		RETURN_EMPTY;

	if (!*switches && !*args && strcmp(last_input, input))
	{
		/* Process each word in the input string */
		while ((tmp = next_arg(input, &input)))
		{
			/* Word is a switch or a group of switches */
			if (tmp[0] == '-' && tmp[1] && tmp[1] != '-' 
				&& !arg_flag)
			{
				/* Look at each char after the '-' */
				for (++tmp; *tmp; tmp++)
				{
					/* If the char is found in optstr
					   and doesn't need an argument,
					   it's added to the switches list.
					   switches are stored as "xy" where
					   x is the switch letter and y is
					   '_' normal switch
					   ':' switch with arg
					   '-' switch with missing arg
					   '!' unrecognized switch
					 */
					strlcat(switches, space, INPUT_BUFFER_SIZE);
					strncat(switches, tmp, 1);
					/* char is a valid switch */
					if ((optptr = strchr(optstr, *tmp)))
					{
						/* char requires an argument */
						if (*(optptr + 1) == ':')
						{
							/* -xfoo, argument is
							   "foo" */
							if (*(tmp + 1))
							{
								tmp++;
								strlcat(args, tmp, INPUT_BUFFER_SIZE);
								strlcat(args, space, INPUT_BUFFER_SIZE);
								strlcat(switches, ":", INPUT_BUFFER_SIZE);
								break;
							}
							/* Otherwise note that
							   the next word in 
							   the input is our
							   arg. */
							else if (*(optptr + 2) == ':')
								arg_flag = 2;
							else
								arg_flag = 1;
						}
						/* Switch needs no argument */
						else strlcat(switches, "_", INPUT_BUFFER_SIZE);
					}
					/* Switch is not recognized */
					else strlcat(switches, "!", INPUT_BUFFER_SIZE);
				}
			}
			else
			{
				/* Everything after a "--" is added to
				   extra_args */
				if (*tmp == '-' && *(tmp + 1) == '-')
				{
					tmp += 2;
					strlcat(extra_args, tmp, INPUT_BUFFER_SIZE);
					strlcat(extra_args, input, INPUT_BUFFER_SIZE);
					*input = 0;
				}
				/* A '-' by itself is a null arg */
				else if (*tmp == '-' && !*(tmp + 1))
				{
					if (arg_flag == 1)
						strlcat(switches, "-", INPUT_BUFFER_SIZE);
					else if (arg_flag == 2)
						strlcat(switches, "_", INPUT_BUFFER_SIZE);
					*tmp = 0;
					arg_flag = 0;
				}
			/* If the word doesn't start with a '-,' it must be
			   either the argument of a switch or just extra
			   info. */
				else if (arg_flag)
				{
				/* If arg_flag is positive, we've processes
				   a switch which requires an arg and we can
				   just tack the word onto the end of args[] */
					
					strlcat(args, tmp, INPUT_BUFFER_SIZE);
					strlcat(args, space, INPUT_BUFFER_SIZE);
					strlcat(switches, ":", INPUT_BUFFER_SIZE);
					arg_flag = 0;
				}
				else
				{
				/* If not, we'll put it aside and add it to
				   args[] after all the switches have been
				   looked at. */
					
					strlcat(extra_args, tmp, INPUT_BUFFER_SIZE);
					strlcat(extra_args, space, INPUT_BUFFER_SIZE);
				}
			}
		}
		/* If we're expecting an argument to a switch, but we've
		   reached the end of our input, the switch is missing its
		   arg. */
		if (arg_flag == 1)
			strlcat(switches, "-", INPUT_BUFFER_SIZE);
		else if (arg_flag == 2)
			strlcat(switches, "_", INPUT_BUFFER_SIZE);
		strlcat(args, extra_args, INPUT_BUFFER_SIZE);
	}

	if ((tmp = next_arg(sptr, &sptr)))
	{
		switch (*(tmp + 1))
		{
			case '_':	
				*(tmp + 1) = 0;
				
				add_var_alias(optopt_var, tmp, 0);
				add_var_alias(optarg_var, NULL, 0);
		
				RETURN_STR(tmp);
			case ':':
				*(tmp + 1) = 0;
				
				add_var_alias(optopt_var, tmp, 0);
				add_var_alias(optarg_var, next_arg(aptr, &aptr), 0);
				RETURN_STR(tmp);
			case '-':
				*(tmp + 1) = 0;
				
				add_var_alias(optopt_var, tmp, 0);
				add_var_alias(optarg_var, NULL, 0);
				RETURN_STR("-");
			case '!':
				*(tmp + 1) = 0;
			
				add_var_alias(optopt_var, tmp, 0);
				add_var_alias(optarg_var, NULL, 0);
				RETURN_STR("!");
			default:
				/* This shouldn't happen */
				RETURN_EMPTY;
		}
	}
	else
	{
		add_var_alias(optopt_var, NULL, 0);
		add_var_alias(optarg_var, aptr, 0);

		*switches = 0;
		*args = 0;
		sptr = switches;
		aptr = args;

		RETURN_EMPTY;
	}
}

BUILT_IN_FUNCTION(function_builtin)
{
	RETURN_MSTR(built_in_alias (*input, NULL));
}

BUILT_IN_FUNCTION(function_strchar)
{
char *chr, *ret = NULL;
char *(*func)(const char *, int);
	GET_STR_ARG(chr, input);
	if (!my_stricmp("STRCHR", fn))
		func = strchr;
	else
		func = strrchr;
	if (chr && *chr)
		ret = func(input, *chr);
	RETURN_STR(ret);
}

BUILT_IN_FUNCTION(function_isdisplaying)
{
	RETURN_INT(window_display);
}

BUILT_IN_FUNCTION(function_getcap)
{
	char *	type;

	GET_STR_ARG(type, input);
	if (!my_stricmp(type, "TERM"))
	{
		char *	retval;
		char *	term = NULL;
		int	querytype = 0;
		int	mangle = 1;
		
		GET_STR_ARG(term, input);
		if (*input)
			GET_INT_ARG(querytype, input);
		if (*input)
			GET_INT_ARG(mangle, input);

		if (!term)			/* This seems spurious */
			RETURN_EMPTY;
		
		if ((retval = get_term_capability(term, querytype, mangle)))
			RETURN_STR(retval);

		RETURN_EMPTY;
	}

	RETURN_EMPTY;
}

/*
 * Usage: $right(number text)
 * Returns: the <number> rightmost characters in <text>.
 * Example: $right(5 the quick brown frog) returns " frog"
 */
BUILT_IN_FUNCTION(function_right)
{
	long	count;

	GET_INT_ARG(count, input);
	RETURN_IF_EMPTY(input);
	if (count < 0)
		RETURN_EMPTY;
		
	if (strlen(input) > count)
		input += strlen(input) - count;

	RETURN_STR(input);
}

/*
 * Usage: $mid(start number text)
 * Returns: the <start>th through <start>+<number>th characters in <text>.
 * Example: $mid(3 4 the quick brown frog) returns " qui"
 *
 * Note: the first character is numbered zero.
 */
BUILT_IN_FUNCTION(function_mid)
{
	long	start, length;

	GET_INT_ARG(start, input);
	GET_INT_ARG(length, input);
	RETURN_IF_EMPTY(input);

	if (start < strlen(input))
	{
		input += start;
		if (length < 0)
			RETURN_EMPTY;
		if (length < strlen(input))
			input[length] = 0;
	}
	else
		input = EMPTY;

	RETURN_STR(input);
}

/*
 * Usage: $left(number text)
 * Returns: the <number> leftmost characters in <text>.
 * Example: $left(5 the quick brown frog) returns "the q"
 *
 * Note: the difference between $[10]foo and $left(10 foo) is that the former
 * is padded and the latter is not.
 */
BUILT_IN_FUNCTION(function_left)
{
	long	count;

	GET_INT_ARG(count, input);
	RETURN_IF_EMPTY(input);
	if (count < 0)
		RETURN_EMPTY;
		
	if (strlen(input) > count)
		input[count] = 0;

	RETURN_STR(input);
}

BUILT_IN_FUNCTION(function_connect)
{
char *host;
unsigned short port;
int snum;
	GET_STR_ARG(host, input);
	GET_INT_ARG(port, input);
	snum = connect_by_number(host, &port, SERVICE_CLIENT, PROTOCOL_TCP, 0);
	RETURN_INT(snum);
}


/*
 * Usage: $encode(text)
 * Returns: a string, uniquely identified with <text> such that the string
 *          can be used as a variable name.
 * Example: $encode(fe fi fo fum) returns "GGGFCAGGGJCAGGGPCAGGHFGN"
 *
 * Note: $encode($decode(text)) returns text (most of the time)
 *       $decode($encode(text)) also returns text.
 */
BUILT_IN_FUNCTION(function_encode)
{
	char	*result;
	int	i = 0;

	result = (char *)new_malloc(strlen(input) * 2 + 1);
	while (*input)
	{
		result[i++] = (*input >> 4) + 0x41;
		result[i++] = (*input & 0x0f) + 0x41;
		input++;
	}
	result[i] = '\0';

	return result;		/* DONT USE RETURN_STR HERE! */
}


/*
 * Usage: $decode(text)
 * Returns: If <text> was generated with $encode(), it returns the string
 *          you originally encoded.  If it wasnt, you will probably get
 *	    nothing useful in particular.
 * Example: $decode(GGGFCAGGGJCAGGGPCAGGHFGN) returns "fe fi fo fum"
 *
 * Note: $encode($decode(text)) returns "text"
 *       $decode($encode(text)) returns "text" too.
 *
 * Note: Yes.  $decode(plain-text) can compress the data in half, but it is
 *	 lossy compression (more than one plain-text can yeild identical
 *	 output), so most input is irreversably mangled.  Dont do it.
 */
BUILT_IN_FUNCTION(function_decode)
{
	char	*result;
	int	i = 0;

	result = (char *)new_malloc(strlen(input) / 2 + 1);

	while (input[0] && input[1])
	{
		/* oops, this isnt quite right. */
		result[i] = ((input[0] - 0x41) << 4) | (input[1] - 0x41);
		input += 2;
		i++;
	}
	result[i] = '\0';

	return result;		/* DONT USE RETURN_STR HERE! */
}

/* 
 * Usage: $strip(characters text)
 * Returns: <text> with all instances of any characters in the <characters>
 *	    argument removed.
 * Example: $strip(f free fine frogs) returns "ree ine rogs"
 *
 * Note: it can be difficult (actually, not possible) to remove spaces from
 *       a string using this function.  To remove spaces, simply use this:
 *		$tr(/ //$text)
 *
 *	 Actually, i recommend not using $strip() at all and just using
 *		$tr(/characters//$text)
 *	 (but then again, im biased. >;-)
 */
BUILT_IN_FUNCTION(function_strip)
{
	char	*result;
	char	*chars;
	char	*cp, *dp;

	GET_STR_ARG(chars, input);
	RETURN_IF_EMPTY(input);

	result = (char *)new_malloc(strlen(input) + 1);
	for (cp = input, dp = result; *cp; cp++)
	{
		/* This is expensive -- gotta be a better way */
		if (!strchr(chars, *cp))
			*dp++ = *cp;
	}
	*dp = '\0';

	return result;		/* DONT USE RETURN_STR HERE! */
}

BUILT_IN_FUNCTION(function_translate)
{
	char *	oldc, 
	     *	newc, 
	     *	text,
	     *	ptr,
		delim;
	int 	size_old, 
		size_new,
		x;

	RETURN_IF_EMPTY(input);

	oldc = input;
	/* First character can be a slash.  If it is, we just skip over it */
	delim = *oldc++;
	newc = strchr(oldc, delim);

	if (!newc)
		RETURN_EMPTY;	/* no text in, no text out */

	text = strchr(++newc, delim);

	if (newc == oldc)
		RETURN_EMPTY;

	if (!text)
		RETURN_EMPTY;
	*text++ = '\0';

	if (newc == text)
	{
		*newc = '\0';
		newc = empty_string;
	}
	else
		newc[-1] = 0;

	/* this is cheating, but oh well, >;-) */
	text = m_strdup(text);

	size_new = strlen(newc);
	size_old = strlen(oldc);

	for (ptr = text; ptr && *ptr; ptr++)
	{
		for (x = 0; x < size_old; x++)
		{
			if (*ptr == oldc[x])
			{
				/* 
				 * Check to make sure we arent
				 * just eliminating the character.
				 * If we arent, put in the new char,
				 * otherwise strcpy it away
				 */
				if (size_new)
					*ptr = newc[(x<size_new)?x:size_new-1];
				else
				{
					ov_strcpy(ptr, ptr+1);
					ptr--;
				}
				break;
			}
		}
	}
	return text;
}

BUILT_IN_FUNCTION(function_strftime)
{
	char		result[128];
	time_t		ltime;
	struct tm	*tm;

	if (isdigit(*input))
		ltime = strtoul(input, &input, 0);
	else
		ltime = time(NULL);

	while (*input && my_isspace(*input))
		++input; 

	if (!*input)
		return m_strdup(empty_string);


	tm = localtime(&ltime);

	if (!strftime(result, 128, input, tm))
		return m_strdup(empty_string);

	return m_strdup(result);
}

BUILT_IN_FUNCTION(function_word)
{
	int	cvalue;
	char	*w_word;

	GET_INT_ARG(cvalue, input);
	if (cvalue < 0)
		RETURN_EMPTY;

	while (cvalue-- > 0)
		GET_STR_ARG(w_word, input);

	GET_STR_ARG(w_word, input);
	RETURN_STR(w_word);
}

BUILT_IN_FUNCTION(function_open)
{
	char *filename;
	char *mode;
	int bin_mode = O_TEXT;
	short port = -1;
		
	GET_STR_ARG(filename, input);
	GET_STR_ARG(mode, input);
	if (input && *input)
		GET_INT_ARG(port, input);
	if (tolower(*mode) == 'i')
		RETURN_INT(open_internet_socket(filename, port, SERVICE_CLIENT));
	else if (tolower(*mode) == 'r')
	{
		if (tolower(*(mode+1)) == 'b')
			bin_mode = O_BINARY;
		RETURN_INT(open_file_for_read(filename, bin_mode));
	}
	else if (tolower(*mode) == 'w')
	{
		if (tolower(*(mode+1)) == 'b')
			bin_mode = O_BINARY;
		RETURN_INT(open_file_for_write(filename, bin_mode));
	}
	RETURN_EMPTY;	
}

BUILT_IN_FUNCTION(function_close)
{
	int snum;
	GET_INT_ARG(snum, input);
	RETURN_INT(file_close(snum));
}

BUILT_IN_FUNCTION(function_write)
{
	int fdc;
	GET_INT_ARG(fdc, input);
	RETURN_INT(file_write(fdc, input, strlen(input)));
}

BUILT_IN_FUNCTION(function_read)
{
	int fdc;
	GET_INT_ARG(fdc, input);
	return file_read (fdc);
}

BUILT_IN_FUNCTION(function_eof)
{
	RETURN_IF_EMPTY(input);
	RETURN_INT(file_eof(atoi(new_next_arg(input, &input))));
}

BUILT_IN_FUNCTION(function_unlink)
{
	char *	expanded;
	int 	failure = 0;

	while (input && *input)
	{
		if ((expanded = expand_twiddle(new_next_arg(input, &input))))
		{
			failure -= unlink(expanded);	
			new_free(&expanded);
		}
	}
	RETURN_INT(failure);
}

BUILT_IN_FUNCTION(function_toupper)
{
	return (upper(m_strdup(input)));
}

BUILT_IN_FUNCTION(function_tolower)
{
	return (lower(m_strdup(input)));
}


BUILT_IN_FUNCTION(function_revw)
{
	char	*booya = NULL;

	while (input && *input)
		m_s3cat(&booya, space, last_arg(&input));
	if (!booya)
		RETURN_EMPTY;

	return booya;
}

BUILT_IN_FUNCTION(function_reverse)
{
	int     length = strlen(input);
	char    *booya = NULL;
	int     x = 0;

	booya = (char *)new_malloc(length+1);
	for (length--; length >= 0; length--,x++)
		booya[x] = input[length];
	booya[x] = '\0';
	return (booya);
}

/* $pattern(pattern string of words)
 * given a pattern and a string of words, returns all words that
 * are matched by the pattern
 * EX: $pattern(f* one two three four five) returns "four five"
 */
BUILT_IN_FUNCTION(function_pattern)
{
	char    *blah;
	char    *booya = NULL;
	char    *pattern;

	GET_STR_ARG(pattern, input)
	while (((blah = new_next_arg(input, &input)) != NULL))
	{
		if (wild_match(pattern, blah))
			m_s3cat(&booya, space, blah);
	}
	RETURN_MSTR(booya);
}

/* $filter(pattern string of inputs)
 * given a pattern and a string of inputs, returns all words that are 
 * NOT matched by the pattern
 * $filter(f* one two three four five) returns "one two three"
 */
BUILT_IN_FUNCTION(function_filter)
{
	char    *blah;
	char    *booya = NULL;
	char    *pattern;

	GET_STR_ARG(pattern, input)
	while ((blah = new_next_arg(input, &input)) != NULL)
	{
		if (!wild_match(pattern, blah))
			m_s3cat(&booya, space, blah);
	}
	RETURN_MSTR(booya);
}

/* $rpattern(word list of patterns)
 * Given a word and a list of patterns, return all patterns that
 * match the word.
 * EX: $rpattern(jnelson@iastate.edu *@* jnelson@* f*@*.edu)
 * returns "*@* jnelson@*"
 */
BUILT_IN_FUNCTION(function_rpattern)
{
	char    *blah;
	char    *booya = NULL;
	char    *pattern;

	GET_STR_ARG(blah, input)

	while ((pattern = new_next_arg(input, &input)) != NULL)
	{
		if (wild_match(pattern, blah))
			m_s3cat(&booya, space, pattern);
	}
	RETURN_MSTR(booya);
}

/* $rfilter(word list of patterns)
 * given a word and a list of patterns, return all patterns that
 * do NOT match the word
 * EX: $rfilter(jnelson@iastate.edu *@* jnelson@* f*@*.edu)
 * returns "f*@*.edu"
 */
BUILT_IN_FUNCTION(function_rfilter)
{
	char    *blah;
	char    *booya = NULL;
	char    *pattern;

	GET_STR_ARG(blah, input)
	while ((pattern = new_next_arg(input, &input)) != NULL)
	{
		if (!wild_match(pattern, blah))
			m_s3cat(&booya, space, pattern);
	}
	RETURN_MSTR(booya);
}

/*
 * Usage: $winnum()
 * Returns: the index number for the current window
 * 
 * Note: returns -1 if there are no windows open (ie, in dumb mode)
 */
BUILT_IN_FUNCTION(function_winnum)
{
	Window *win = NULL;
	if (!(win = *input ? get_window_by_desc(input) : current_window))
		RETURN_INT(-1);
	RETURN_INT(win->refnum);
}

BUILT_IN_FUNCTION(function_winsize)
{
	int refnum;
	Window *win;

	if (input && *input)
	{
		GET_INT_ARG(refnum, input);
		win = get_window_by_refnum(refnum);
	}
	else
		win = current_window;

	if (!win)
		RETURN_EMPTY;

	RETURN_INT(win->display_size);
}

/*
 * Usage: $rand(max)
 * Usage: $rand(min max)
 * Returns: A random number from zero to max-1.
 * Example: $rand(10) might return any number from 0 to 9.
 */
BUILT_IN_FUNCTION(function_rand)
{
	long	tempin, tempin2 = 0;

	GET_INT_ARG(tempin, input);
	if (input && *input)
		GET_INT_ARG(tempin2, input);
	if (tempin == 0)
		tempin = (unsigned long) -1;	/* This is cheating. :P */
	if (tempin2 != 0)
	{
		long ret;
		while ((ret = random_number(0) % tempin2) < tempin)
			;
		RETURN_INT(ret);
	}
	RETURN_INT(random_number(0) % tempin);
}

/*
 * Usage: $srand(seed)
 * Returns: Nothing.
 * Side effect: seeds the random number generater.
 * Note: the argument is ignored.
 */
BUILT_IN_FUNCTION(function_srand)
{
	random_number(time(NULL));
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_strcmp)
{
char *first, *second;
	first = new_next_arg(input, &input);
	if (!first || !*first)
		RETURN_INT(-1);
	second = new_next_arg(input, &input);
	if (!second || !*second)
		RETURN_INT(1);
	RETURN_INT(strcmp(first, second));
}

BUILT_IN_FUNCTION(function_channels)
{
Window *tmp = NULL;
char *ret = NULL;
	ret = m_strdup(empty_string);
	while (traverse_all_windows(&tmp))
	{
		ChannelStruct *ch;
		if (tmp->server != from_server)
			continue;
		ch = tmp->nchannels;
		while(ch)
		{
			if (*ret)
				malloc_strcat(&ret, " " );
			malloc_strcat(&ret, ch->channel);
			ch = ch->next;
		}
	}
	return ret;	
}

/* 
 * Next two contributed by Scott H Kilau (sheik), who for some reason doesnt 
 * want to take credit for them. *shrug* >;-)
 *
 * Deciding not to be controversial, im keeping the original (contributed)
 * semantics of these two functions, which is to return 1 on success and
 * -1 on error.  If you dont like it, then tough. =)  I didnt write it, and
 * im not going to second guess any useful contributions.
 */
BUILT_IN_FUNCTION(function_fexist)
{
        char	FileBuf[BIG_BUFFER_SIZE+1];
	char	*filename, *fullname;

	if (!(filename = new_next_arg(input, &input)))
		RETURN_INT(-1);

	if (*filename == '/')
		strlcpy(FileBuf, filename, BIG_BUFFER_SIZE);
	else if (*filename == '~') 
	{
		if (!(fullname = expand_twiddle(filename)))
			RETURN_INT(-1);

		strmcpy(FileBuf, fullname, BIG_BUFFER_SIZE);
		new_free(&fullname);
	}
	else 
	{
		getcwd(FileBuf, BIG_BUFFER_SIZE);
		strmcat(FileBuf, "/", BIG_BUFFER_SIZE);
		strmcat(FileBuf, filename, BIG_BUFFER_SIZE);
	}

	if (access(FileBuf, R_OK) == -1)
		RETURN_INT(-1);

	RETURN_INT(1);
}

BUILT_IN_FUNCTION(function_fsize)
{
        char	FileBuf[BIG_BUFFER_SIZE+1];
	char	*filename, *fullname;
        struct  stat    stat_buf;

	if (!(filename = new_next_arg(input, &input)))
		RETURN_INT(-1);

	if (*filename == '/')
		strlcpy(FileBuf, filename, BIG_BUFFER_SIZE);
	else if (*filename == '~') 
	{
		if (!(fullname = expand_twiddle(filename)))
			RETURN_INT(-1);

		strmcpy(FileBuf, fullname, BIG_BUFFER_SIZE);
		new_free(&fullname);
	}
	else 
	{
		getcwd(FileBuf, sizeof(FileBuf));
		strmcat(FileBuf, "/", BIG_BUFFER_SIZE);
		strmcat(FileBuf, filename, BIG_BUFFER_SIZE);
	}

	if (stat(FileBuf, &stat_buf) == -1)
		RETURN_INT(-1);

	RETURN_INT((int)stat_buf.st_size);	/* Might not be an int */
}


BUILT_IN_FUNCTION(function_onchannel)
{
	char		*channel;
	ChannelStruct	*chan = NULL; /* XXX */
	NickStruct	*tmp = NULL;
	char		*nicks = NULL;
		
	/* DO NOT use new_next_arg() in here. NULL is a legit value to pass CDE*/
	if (!(channel = next_arg(input, &input)) && current_window)
		channel = current_window->current_channel;
	if (input && *input)
	{
		char *n, *s;
		if ((chan = lookup_channel(channel, from_server)))
		{
			while ((n = next_arg(input, &input)))
			{
				while ((s = next_in_comma_list(n, &n)))
				{
					if (!s || !*s)
						break;
					if (find_in_list((List **)&chan->nicks, s, 0))
						m_s3cat(&nicks, space, "1");
					else
						m_s3cat(&nicks, space, "0");
				}
			}
			return nicks ? nicks : m_strdup(empty_string);
		}
		RETURN_EMPTY;
	}	
	if ((chan = lookup_channel(channel, from_server)))
	{
		for (tmp = chan->nicks; tmp; tmp = tmp->next)
		m_s3cat(&nicks, space, tmp->nick);
		RETURN_MSTR(nicks);
	}
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_numwords)
{
	RETURN_INT(word_count(input));
}

BUILT_IN_FUNCTION(function_strlen)
{
	if (input && *input)
		RETURN_INT(strlen(input));
	RETURN_INT(0);
}


/*
 * $uname()
 * Returns system information.  Expandoes %a, %s, %r, %v, %m, and %n
 * correspond to uname(1)'s -asrvmn switches.  %% expands to a literal
 * '%'.  If no arguments are given, the function returns %s and %r (the
 * same information given in the client's ctcp version reply).
 *
 * Contributed by Matt Carothers (CrackBaby) on Dec 20, 1997
 */
BUILT_IN_FUNCTION(function_uname)
{
#ifndef HAVE_UNAME
	RETURN_STR("unknown");
#else
	struct utsname un;
	char 	tmp[BIG_BUFFER_SIZE+1];
	char	*ptr = tmp;
	size_t	size = BIG_BUFFER_SIZE;
	int 	i;
	int	len;

	if (uname(&un) == -1)
		RETURN_STR("unknown");

	if (!*input)
		input = "%s %r";

	*tmp = 0;
	for (i = 0, len = strlen(input); i < len; i++)
	{
		if (ptr - tmp >= size)
			break;

		if (input[i] == '%') 
		{
		    switch (input[++i]) 
		    {
			case 'm':	strlcpy(ptr, un.machine, size);
					break;
			case 'n':	strlcpy(ptr, un.nodename, size);
					break;
			case 'r':	strlcpy(ptr, un.release, size);
					break;
			case 's':	strlcpy(ptr, un.sysname, size);
					break;
			case 'v':	strlcpy(ptr, un.version, size);
					break;
			case 'a':	
					snprintf(ptr, size, "%s %s %s %s %s",
						un.sysname, un.nodename,
						un.release, un.version,
						un.machine);
					break;
			case '%':	strlcpy(ptr, "%", size);
		    }
		    ptr += strlen(ptr);
		}
		else
		    *ptr++ = input[i];
	}
	*ptr = 0;

	RETURN_STR(tmp);
#endif
}

BUILT_IN_FUNCTION(function_shift)
{
	char    *value = NULL;
	char    *var    = NULL;
	char	*booya 	= NULL;
	char    *placeholder;

	GET_STR_ARG(var, input);

	if (input && *input)
		RETURN_STR(var);

	value = get_variable(var);

	if (!value && !*value)
		RETURN_EMPTY;

	placeholder = value;
	booya = m_strdup(new_next_arg(value, &value));
	if (var)
		add_var_alias(var, value, 0);
	new_free(&placeholder);
	if (!booya)
		RETURN_EMPTY;
	return booya;
}

BUILT_IN_FUNCTION(function_unshift)
{
	char    *value = NULL;
	char    *var    = NULL;
	char	*booya  = NULL;

	GET_STR_ARG(var, input);
	value = get_variable(var);
	if (!input || !*input)
		return value;

	booya = m_strdup(input);
	m_s3cat_s(&booya, space, value);

	add_var_alias(var, booya, 0);
	new_free(&value);
	return booya;
}

BUILT_IN_FUNCTION(function_push)
{
	char    *value = NULL;
	char    *var    = NULL;

	GET_STR_ARG(var, input);
	upper(var);
	value = get_variable(var);
	m_s3cat(&value, space, input);
	add_var_alias(var, value, 0);
	return value;
}

BUILT_IN_FUNCTION(function_pop)
{
	char *value	= NULL;
	char *var	= NULL;
	char *pointer	= NULL;
	char *blech     = NULL;

	GET_STR_ARG(var, input);

	if (input && *input)
	{
		pointer = strrchr(input, ' ');
		RETURN_STR(pointer ? pointer : input);
	}

	upper(var);
	value = get_variable(var);
	if (!value || !*value)
	{
		new_free(&value);
		RETURN_EMPTY;
	}

	if (!(pointer = strrchr(value, ' ')))
	{
		add_var_alias(var, empty_string, 0); /* dont forget this! */
		return value;	/* one input -- return it */
	}

	*pointer++ = '\0';
	add_var_alias(var, value, 0);

	/* 
	 * because pointer points to value, we *must* make a copy of it
	 * *before* we free value! (And we cant forget to free value, either)
	 */
	blech = m_strdup(pointer);
	new_free(&value);
	return blech;
}

/*
 * To save time, try to precalc the size of the output buffer.
 * jfn 03/18/98
 */
BUILT_IN_FUNCTION(function_jot)
{
	int     start 	= 0;
	int     stop 	= 0;
	int     interval = 1;
	int     counter;
	char	*booya 	= NULL;
	int	range;
	size_t	size;

        GET_INT_ARG(start,input)
        GET_INT_ARG(stop, input)
        if (input && *input)
                GET_INT_ARG(interval, input)
        else
                interval = 1;

	if (interval == 0)
		RETURN_EMPTY;
        if (interval < 0) 
                interval = -interval;

	range = abs(stop - start) + 1;
	size = range * 10;
	booya = new_malloc(size);	/* Blah. This BETTER be enough */

        if (start < stop)
	{
		strlcpy(booya, ltoa(start), size);
		for (counter = start + interval; 
		     counter <= stop; 
		     counter += interval)
		{
			strlcat(booya, space, size);
			strlcat(booya, ltoa(counter), size);
		}
	}
        else
	{
		strlcpy(booya, ltoa(start), size);
		for (counter = start - interval; 
		     counter >= stop; 
		     counter -= interval)
		{
			strlcat(booya, space, size);
			strlcat(booya, ltoa(counter), size);
		}
	}

	return booya;
}

BUILT_IN_FUNCTION(function_isalpha)
{
	if (isalpha(*input))
		RETURN_INT(1);
	else
		RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_isdigit)
{
	if (isdigit(*input))
		RETURN_INT(1);
	else
		RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_isalnum)
{
	if (isalpha(*input) || isdigit(*input))
		RETURN_INT(1);
	else
		RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_isspace)
{
	if (isspace(*input))
		RETURN_INT(1);
	else
		RETURN_INT(0);
}

BUILT_IN_FUNCTION(function_isxdigit)
{
	#define ishex(x) \
		((x >= 'A' && x <= 'F') || (x >= 'a' && x <= 'f'))

	if (isdigit(*input) || ishex(*input))
		RETURN_INT(1);
	else
		RETURN_INT(0);	
}

BUILT_IN_FUNCTION(function_findw)
{
	char	*word, *this_word;
	int	word_cnt = 0;

	GET_STR_ARG(word, input);

	while (input && *input)
	{
		GET_STR_ARG(this_word, input);
		if (!my_stricmp(this_word, word))
			RETURN_INT(word_cnt);

		word_cnt++;
	}

	RETURN_INT(-1);
}

/*
 * $rest(index string)
 * Returns 'string' starting at the 'index'th character
 * Just like $restw() does for words.
 */
BUILT_IN_FUNCTION(function_rest)
{
	int	start = 1;

	if (my_atol(input))
		GET_INT_ARG(start, input);

	if (start <= 0)
		RETURN_STR(input);

	if (start >= strlen(input))
		RETURN_EMPTY;

	RETURN_STR(input + start);
}

/* $restw(num string of text)
 * returns "string of text" that occurs starting with and including
 * word number "num"
 * NOTE: the first word is numbered 0.
 * EX: $restw(3 now is the time for) returns "time for"
 */
BUILT_IN_FUNCTION(function_restw)
{
	int     where;
	
	GET_INT_ARG(where, input);
	if (where < 0)
		RETURN_EMPTY;
	return extract(input, where, EOS);
}

/* $remw(word string of text)
 * returns "string of text" with the word "word" removed
 * EX: $remw(the now is the time for) returns "now is time for"
 */
BUILT_IN_FUNCTION(function_remw)
{
	char 	*word_to_remove;
	int	len;
	char	*str;

	GET_STR_ARG(word_to_remove, input);
	len = strlen(word_to_remove);

	str = stristr(input, word_to_remove);
	for (; str && *str; str = stristr(str + 1, word_to_remove))
	{
		if (str == input || isspace(str[-1]))
		{
			if (!str[len] || isspace(str[len]))
			{
				if (!str[len])
				{
					if (str != input)
						str--;
					*str = 0;
				}
				else if (str > input)
				{
					char *safe = (char *)alloca(strlen(str));
					strcpy(safe, str + len);
					strcpy(str - 1, safe);
				}
				else 
				{
					char *safe = (char *)alloca(strlen(str));
					strcpy(safe, str + len + 1);
					strcpy(str, safe);
				}
				break;
			}
		}
	}

	RETURN_STR(input);
}

/* $insertw(num word string of text)
 * returns "string of text" such that "word" is the "num"th word
 * in the string.
 * NOTE: the first word is numbered 0.
 * EX: $insertw(3 foo now is the time for) returns "now is the foo time for"
 */
BUILT_IN_FUNCTION(function_insertw)
{
	int     where;
	char    *what;
	char    *booya= NULL;
	char 	*str1, *str2;

	GET_INT_ARG(where, input);
	
	/* If the word goes at the front of the string, then it
	   already is: return it. ;-) */
	if (where < 1)
		booya = m_strdup(input);
	else
	{
		GET_STR_ARG(what, input);
		str1 = extract(input, 0, (where - 1));
		str2 = extract(input, where, EOS);
		booya = m_strdup(str1);
		if (str1 && *str1)
			booya = m_2dup(str1, space);
		malloc_strcat(&booya, what);
		m_s3cat_s(&booya, space, str2);
		new_free(&str1);
		new_free(&str2);
	}

	return booya;				/* DONT USE RETURN_STR HERE! */
}

/* $chngw(num word string of text)
 * returns "string of text" such that the "num"th word is removed 
 * and replaced by "word"
 * NOTE: the first word is numbered 0
 * EX: $chngw(3 foo now is the time for) returns "now is the foo for"
 */
BUILT_IN_FUNCTION(function_chngw)
{
	int     which;
	char    *what;
	char    *booya= NULL;
	char	*str1, *str2;
	
	GET_INT_ARG(which, input);
	GET_STR_ARG(what, input);

	if (which < 0)
		RETURN_STR(input);

	/* hmmm. if which is 0, extract does the wrong thing. */
	str1 = extract(input, 0, which - 1);
	str2 = extract(input, which + 1, EOS);
	booya = m_strdup(str1);
	if (str1 && *str1)
		booya = m_2dup(str1, space);
	malloc_strcat(&booya, what);
	m_s3cat_s(&booya, space, str2);
	new_free(&str1);
	new_free(&str2);

	return (booya);
}


/* $common (string of text / string of text)
 * Given two sets of words seperated by a forward-slash '/', returns
 * all words that are found in both sets.
 * EX: $common(one two three / buckle my two shoe one) returns "one two"
 * NOTE: returned in order found in first string.
 */
BUILT_IN_FUNCTION(function_common)
{
	char    *left = NULL, **leftw = NULL,
		*right = NULL, **rightw = NULL,	*booya = NULL;
	int	leftc, lefti;
	int	rightc, righti;
	
	left = input;
	if ((right = strchr(input,'/')) == NULL)
		RETURN_EMPTY;

 	*right++ = 0;
	leftc = splitw(left, &leftw);
	rightc = splitw(right, &rightw);

	for (lefti = 0; lefti < leftc; lefti++)
	{
		for (righti = 0; righti < rightc; righti++)
		{
			if (rightw[righti] && !my_stricmp(leftw[lefti], rightw[righti]))
			{
				m_s3cat(&booya, space, leftw[lefti]);
				rightw[righti] = NULL;
			}
		}
	}
	new_free((char **)&leftw);
	new_free((char **)&rightw);
	if (!booya)
		RETURN_EMPTY;

	return (booya);				/* DONT USE RETURN_STR HERE! */
}

/* $diff(string of text / string of text)
 * given two sets of words, seperated by a forward-slash '/', returns
 * all words that are not found in both sets
 * EX: $diff(one two three / buckle my two shoe)
 * returns "one two three buckle my shoe"
 */
BUILT_IN_FUNCTION(function_diff)
{
	char 	*left = NULL, **leftw = NULL,
	     	*right = NULL, **rightw = NULL, *booya = NULL;
	int 	lefti, leftc,
	    	righti, rightc;
	int 	found;

	left = input;
	if ((right = strchr(input,'/')) == NULL)
		RETURN_EMPTY;

	*right++ = 0;
	leftc = splitw(left, &leftw);
	rightc = splitw(right, &rightw);

	for (lefti = 0; lefti < leftc; lefti++)
	{
		found = 0;
		for (righti = 0; righti < rightc; righti++)
		{
			if (rightw[righti] && !my_stricmp(leftw[lefti], rightw[righti]))
			{
				found = 1;
				rightw[righti] = NULL;
			}
		}
		if (!found)
			m_s3cat(&booya, space, leftw[lefti]);
	}

	for (righti = 0; righti < rightc; righti++)
	{
		if (rightw[righti])
			m_s3cat(&booya, space, rightw[righti]);
	}

	new_free((char **)&leftw);
	new_free((char **)&rightw);

	if (!booya)
		RETURN_EMPTY;

	return (booya);
}

/*
 * $remws(word word word / word word word)
 * Returns the right hand side unchanged, except that any word on the right
 * hand side that is also found on the left hand side will be removed.
 * Space is *not* retained.  So there.
 */
BUILT_IN_FUNCTION(function_remws)
{
	char    *left = NULL,
		**lhs = NULL,
		*right = NULL,
		**rhs = NULL, 
		*booya = NULL;
	int	leftc, 	
		lefti,
		rightc,
		righti;
	int	found = 0;

	left = input;
	if (!(right = strchr(input,'/')))
	RETURN_EMPTY;

	*right++ = 0;
	leftc = splitw(left, &lhs);
	rightc = splitw(right, &rhs);

	for (righti = 0; righti < rightc; righti++)
	{
		found = 0;
		for (lefti = 0; lefti < leftc; lefti++)
		{
			if (rhs[righti] && lhs[lefti]  &&
			    !my_stricmp(lhs[lefti], rhs[righti]))
			{
				found = 1;
				break;
			}
		}
		if (!found)
			m_s3cat(&booya, space, rhs[righti]);
		rhs[righti] = NULL;
	}

	new_free((char **)&lhs);
	new_free((char **)&rhs);

	if (!booya)
		RETURN_EMPTY;

	return (booya);				/* DONT USE RETURN_STR HERE! */
}


BUILT_IN_FUNCTION(function_status)
{
	int window_refnum;
	int status_line;

	GET_INT_ARG(window_refnum, input);
	GET_INT_ARG(status_line, input);
	RETURN_STR(get_status_by_refnum(window_refnum, status_line));
}

/*
 * $crypt(password seed)
 * What it does: Returns a 13-char encrypted string when given a seed and
 *    password. Returns zero (0) if one or both args missing. Additional
 *    args ignored.
 * Caveats: Password truncated to 8 chars. Spaces allowed, but password
 *    must be inside "" quotes.
 * Credits: Thanks to Strongbow for showing me how crypt() works.
 * This cheap hack by: CrowMan
 */
BUILT_IN_FUNCTION(function_crypt)
{
#if defined(WINNT)
	RETURN_STR(empty_string);
#else
        char pass[9] = "\0";
        char seed[3] = "\0";
        char *blah, *bleh, *crypt (const char *, const char *);

	GET_STR_ARG(blah, input)
	GET_STR_ARG(bleh, input)
	strmcpy(pass, blah, 8);
	strmcpy(seed, bleh, 2);

	RETURN_STR(crypt(pass, seed));
#endif
}

BUILT_IN_FUNCTION(alias_idle)
{
	return m_sprintf("%ld", now - idle_time);
}

BUILT_IN_FUNCTION(alias_online)
{
	return m_sprintf("%ld", start_time);
}

BUILT_IN_FUNCTION(alias_sent_nick)
{
	return m_strdup(sent_nick ? sent_nick : empty_string);
}

BUILT_IN_FUNCTION(alias_recv_nick)
{
	return m_strdup(recv_nick ? recv_nick : empty_string);
}

BUILT_IN_FUNCTION(function_servernick)
{
	int serv = from_server;
	if (input && *input)
		GET_INT_ARG(serv, input);
	RETURN_MSTR(get_server_nickname(serv));
}

BUILT_IN_FUNCTION(function_chmod)
{
#ifndef __EMX__
	char 	*filearg, 
		*after;
	int 	fd = -1;
	char 	*perm_s;
	mode_t 	perm;
	char 	*expanded;
	int	retval = -1;

	GET_STR_ARG(filearg, input);
	fd = (int) strtoul(filearg, &after, 0);

	GET_STR_ARG(perm_s, input);
	perm = (mode_t) strtol(perm_s, &perm_s, 8);

	if (after != input && *after == 0)
	{
		if (file_valid(fd))
			RETURN_INT(fchmod(fd, perm));
		else
			RETURN_EMPTY;
	}
	else
	{
		if ((expanded = expand_twiddle(filearg)))
		{
			retval = chmod(expanded, perm);
			new_free(&expanded);
		}
		RETURN_INT(retval);
	}
#else
	RETURN_INT(0);
#endif
}

BUILT_IN_FUNCTION(function_isirc)
{
	int serv = from_server;
	if (input && *input)
		GET_INT_ARG(serv, input);
	RETURN_INT(get_server_ircmode(serv));
}

BUILT_IN_FUNCTION(function_wordtoindex)
{
	int		wordnum;
	const char *	ptr;

	GET_INT_ARG(wordnum, input);
	move_to_abs_word(input, &ptr, wordnum);
	RETURN_INT((int)(ptr - input));
}

BUILT_IN_FUNCTION(function_iptolong)
{
	char *	dotted_quad;
	struct in_addr	addr;

	GET_STR_ARG(dotted_quad, input);
	if (inet_aton(dotted_quad, &addr))
		return m_sprintf("%lu", (unsigned long)ntohl(addr.s_addr));
	RETURN_EMPTY;
}

BUILT_IN_FUNCTION(function_longtoip)
{
	char *	ip32;
	struct in_addr	addr;

	GET_STR_ARG(ip32, input);
	addr.s_addr = (unsigned long)ntohl(strtoul(ip32, NULL, 10));
	RETURN_STR(inet_ntoa(addr));
}

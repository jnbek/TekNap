/* $Id: mp3db.c,v 1.1.1.1 2000/10/16 01:09:49 edwards Exp $ */
 
#include "teknap.h"
#include "struct.h"
#include "ircaux.h"
#include "output.h"
#include "mp3db.h"

typedef struct _mp3db_search_ {
	struct _mp3db_search_ *next;
	char *pattern;
	int bitrate;
	int type;
	FILE *fp;
} Mp3Search;

static char	FORMAT_MP3[512] = " %N %g4.1L %b %t %f";
static char	FORMAT_DIR[512] = " %f - %N";
#define OTHER_FORMAT_MP3 " %S %B %T %L %F"

Mp3Search *search_param = NULL;

#define SEARCH_FILENAME	0x0001
#define SEARCH_BITRATE	0x0002
#define SEARCH_LABEL	0x0004

#define PRINT_LABEL	0x0001
#define PRINT_ONE	0x0002
#define PRINT_ALL	0x0004

FILE *open_database(char *filename)
{
FILE *fp = NULL;
char *p;
	if ((p = expand_twiddle(filename)))
		fp = uzfopen(&p, ".", 0, NULL);
	new_free(&p);
	return fp;
}

int mp3printf(char *, int, char *, FileStruct *);

void search_database(Mp3Search *find)
{
FILE *fp;
int print_it;
FileStruct fs = { 0 };
Tmp3_Entry tmp;
char buffer[BIG_BUFFER_SIZE+1];
int count = 0;
int number = 0;
Mp3Search *new, *last;

	fp = open_database("/etc/mp3.db");
	if (!fp)
	{
		say("No Such file as /etc/mp3.db");
		return;
	}
	for (new = find; new; new = last)
	{
		last = new->next;
		fread(&tmp, sizeof(tmp), 1, fp);
		while(!feof(fp))
		{
			count++;
			memset(&fs, 0, sizeof(FileStruct));
			fs.filename = tmp.pathname;
			fs.nick = tmp.label;
			fs.bitrate = tmp.bitrate;
			fs.seconds = tmp.time;
			fs.stereo = 1;
			fs.filesize = tmp.size;
			print_it = 0;
			switch(new->type)
			{
				case SEARCH_FILENAME:
					if (wild_match(new->pattern, fs.filename))
						if (new->bitrate == -1 || new->bitrate == fs.bitrate)
							print_it++;
					break;
				case SEARCH_BITRATE:
					if (new->bitrate == fs.bitrate)
						print_it++;
					break;
				case SEARCH_LABEL:
					if (wild_match(new->pattern, tmp.label))
						print_it++;
					break;
				default:
					break;
			}
			if (print_it)
			{
				if (tmp.mode == M_MP3)
					mp3printf(buffer, BIG_BUFFER_SIZE, FORMAT_MP3, &fs); 
				else
					mp3printf(buffer, BIG_BUFFER_SIZE, FORMAT_DIR, &fs);
				if (new->fp)
					fprintf(new->fp, "%s\n", buffer);
				else
					put_it(buffer);
				number++;
			}
			fread(&tmp, sizeof(tmp), 1, fp);
		}
		say("Searched for %s/%d and returned %d entries", new->pattern, count, number);
		count = 0; number = 0;
		new_free(&new->pattern);
		if (new->fp)
		{
			if (!last || (last && last->fp != new->fp))
				fclose(new->fp);
		}
		new_free(&new);
	}
	fclose(fp);
}

#if 0
void search_by_filename(char *pattern, int type)
{
	search_database(pattern, SEARCH_FILENAME, type);
}

void search_by_label(char *pattern, int type)
{
	search_database(pattern, SEARCH_LABEL, type);
}

void search_by_bitrate(char *pattern, int type)
{
	if (my_atol(pattern))
		search_database(pattern, SEARCH_BITRATE, type);
}
#endif

BUILT_IN_COMMAND(dbsearch)
{
char *arg;
Mp3Search *new = NULL;
int bitrate = -1;

	while ((arg = new_next_arg(args, &args)))
	{
		if (!arg || !*arg)
			break;
		new = (Mp3Search *)new_malloc(sizeof(Mp3Search));
		new->type = SEARCH_FILENAME;
		new->bitrate = bitrate;

		if (*arg == '-')
			arg++;
		else
		{
			new->pattern = m_sprintf("*%s*", arg);
			continue;
		}
		switch(*arg)
		{
			case 'F':
				if ((arg = next_arg(args, &args)))
					if (search_param)
						search_param->fp = fopen(arg, "w");
				new_free(&new);
				break;
			case 'S':
				new->type = SEARCH_FILENAME;
				new->pattern = m_sprintf("*%s*", new_next_arg(args, &args));
				break;
			case 'B':
				bitrate = my_atol(next_arg(args, &args));
				new_free(&new);
				break;
			case 'l':
				new->type = SEARCH_LABEL;
				new->pattern = m_sprintf("*%s*", new_next_arg(args, &args));
				break;
			case 'b':
				new->type = SEARCH_BITRATE;
				new->bitrate = my_atol(next_arg(args, &args));
				break;
			default:
				say("Unknown option. -S, -l, -b");
				new_free(&new);
				break;
		}
		if (!new)
			continue;
		new->next = search_param;
		search_param = new;
	}
	if (search_param)
		search_database(search_param);
	search_param = NULL;
}

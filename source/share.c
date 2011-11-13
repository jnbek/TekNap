/* $Id: share.c,v 1.1.1.1 2001/02/05 00:22:16 edwards Exp $ */

#include "teknap.h"
#include "struct.h"
#include "input.h"
#include "list.h"
#include "output.h"
#include "md5.h"
#include "server.h"
#include "status.h"
#include "vars.h"
#include "window.h"
#include "napster.h"
#include "bsdglob.h"


#include <sys/time.h>
#include <sys/stat.h>
#ifdef HAVE_MMAP
#include <sys/mman.h>
#endif

#ifndef MAP_FAILED
#define MAP_FAILED (void *) -1
#endif

#undef WANT_MD5

#define DEFAULT_FILEMASK "*.mp3"

#ifdef WINNT
#define WANT_THREAD
#endif

#if defined(WANT_THREAD) && !defined(WINNT)

static pthread_t fserv_thread = {0};

static pthread_mutex_t quit_mutex = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t fserv_struct_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t shared_count_mutex = PTHREAD_MUTEX_INITIALIZER;
static void share_thread_signal_setup(void);


#endif

unsigned long do_share(char *);

extern int scan_done;

#define DEFAULT_MD5_SIZE 292 * 1024
#define DET_BUFFER_SIZE 1024

char *mime_string[] = { "mp3", "audio", "image", "video", "application", "text", "" }; 
char *audio[] = {".wav", ".aiff", ".mid", ".mod", ""};
char *image[] = {".jpg", ".gif", ""};
char *video[] = {".mpg", ".dat", ""};
char *application[] = {".tar.gz", ".tar.Z", ".Z", ".gz", ".arc", ".bz2", ".zip", ""};

char	shared_fname[] = "shared.dat";

extern	char shared_file_dir[];

typedef struct _share_struct_ {
	struct _share_struct_ *next;
	int recurse;
	int reload;
	int update;
	int share_search_type;
	int share;
	int count;
	int total;
	char *path;
} Share_Struct;

static Share_Struct *share_files = NULL;
static Share_Struct *load_share_files = NULL;

static void *do_update(void *);

unsigned long shared_count = 0;

static int in_sharing = 0;

int in_load = 0;

static time_t fserv_start = 0;

/* Xing header flags */
#define VBR_FRAMES_FLAG     0x0001
#define VBR_BYTES_FLAG      0x0002
#define VBR_TOC_FLAG        0x0004
#define VBR_SCALE_FLAG  0x0008

typedef struct
{
	unsigned long h_id;	/* from MPEG header, 0=MPEG2, 1=MPEG1 */
	unsigned long samprate;	/* determined from MPEG header */
	unsigned long flags;	/* from Xing header data */
	unsigned long frames;	/* total bit stream frames from Xing header data */
	unsigned long bytes;	/* total bit stream bytes from Xing header data */
	long	      vbr_scale;/* encoded vbr scale from Xing header data */
	unsigned char *toc;	/* pointer to unsigned char toc_buffer[100] */
	/* may be NULL if toc not desired */
}
XHEADDATA;



char *find_mime_type(char *fn)
{
static char mime_str[100];
	if (fn)
	{
		int i;
		if (!end_strcmp(fn, ".mp3", 4))
		{
			sprintf(mime_str, "%s", mime_string[0]);
			return mime_str;
		}
		if (!end_strcmp(fn, ".exe", 4))
		{
			sprintf(mime_str, "%s", mime_string[4]);
			return mime_str;
		}
		for (i = 0; *audio[i]; i++)
		{
			if (!end_strcmp(fn, audio[i], strlen(audio[i])))
			{
				sprintf(mime_str, "%s", mime_string[1]);
				return mime_str;
			}
		}
		for (i = 0; *image[i]; i++)
		{
			if (!end_strcmp(fn, image[i], strlen(image[i])))
			{
				sprintf(mime_str, "%s", mime_string[2]);
				return mime_str;
			}
		}
		for (i = 0; *video[i]; i++)
		{
			if (!end_strcmp(fn, video[i], strlen(video[i])))
			{
				sprintf(mime_str, "%s", mime_string[3]);
				return mime_str;
			}
		}
		for (i = 0; *application[i]; i++)
		{
			if (!end_strcmp(fn, application[i], strlen(application[i]))) 
			{
				sprintf(mime_str, "%s", mime_string[4]);
				return mime_str;
			}
		}
		sprintf(mime_str, "%s", mime_string[5]);
		return mime_str;
	}
	return NULL;	
}

void clear_files(FileStruct **f)
{
FileStruct *last, *f1 = *f;
	while (f1)
	{
		last = f1->next;
		new_free(&f1->filename);
		new_free(&f1->checksum);
		new_free(&f1);
		f1 = last;
	}
	*f = NULL;
}

void remove_file(FileStruct **f)
{
	new_free(&(*f)->filename);
	new_free(&(*f)->checksum);
	new_free(f);
}

char *convertnap_dos(char *str)
{
register char *p;
	for (p = str; *p; p++)
		if (*p == '/')
			*p = '\\';
	return str;
}

char *convertnap_unix(char *arg)
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


char *mode_str(int mode)
{
	switch(mode)
	{	
		case 0:
			return "St";
		case 1:
			return "JS";
		case 2:
			return "DC";
		case 3:
			return "M";
	}
	return empty_string;
}

char *print_time(time_t input)
{
	static char	buff[40];
	time_t		seconds,
			minutes;
	seconds = input;
	minutes = seconds / 60;
	seconds = seconds % 60;
	sprintf(buff, "%02u:%02u", (unsigned int)minutes, (unsigned int)seconds);
	return buff;
}                                        

int mp3printf(char *, int, char *, FileStruct *);

char *make_mp3_string(FILE *fp, const FileStruct *f, char *dir, char *fs, char *dirbuff)
{
	static char	buffer[2*BIG_BUFFER_SIZE+1];
	char		*loc,
			*p,
			*fn;

	if (!fs || !*fs)
		return empty_string;
	memset(buffer, 0, sizeof(buffer));

	loc = LOCAL_COPY(f->filename);
	if ((fn = strrchr(loc, '/')) || (fn = strrchr(loc, '\\')))
		*fn++ = 0;
	if ((p = strrchr(loc, '/')) || (p = strrchr(loc, '\\')))
	{
		*p++ = 0;
		if (strlen(p) < 4)
		{
			char *q;
			q = --p;
			if ((p = strrchr(loc, '/')) || (p = strrchr(loc, '\\')))
			{
				*q = *p;
				*p++ = 0;
				
			}
		}
	}
	/* fn should point to the filename and p to the dir */
	/* 
	 * init the dir keeper 
	 * or cmp the old dir with the new
	 */
	if (dirbuff && p && (!*dirbuff || strcmp(dirbuff, p)))
	{
		strcpy(dirbuff, p);
		if (fp)
			fprintf(fp, "\nDirectory [ %s ]\n", p);
		else
			return NULL;
	}
	mp3printf(buffer, sizeof(buffer)-1, fs, (FileStruct *)f);
	if (fp && *buffer)
		fprintf(fp, "%s\n", buffer);
	return buffer;
}

static char return_dir[BIG_BUFFER_SIZE+1] = "\0";
char *return_current_share_dir()
{
	return return_dir;
}


int read_glob_dir(char *path, int globflags, glob_t *globpat, int recurse)
{
	char	buffer[BIG_BUFFER_SIZE+1];
	
	sprintf(buffer, "%s/*", path);
	bsd_glob(buffer, globflags, NULL, globpat);
	if (recurse)
	{
		int i = 0;
		int old_glpathc = globpat->gl_pathc;
		for (i = 0; i < old_glpathc; i++)
		{
			char *fn;
			fn = globpat->gl_pathv[i];
			if (fn[strlen(fn)-1] != '/')
				continue;
			sprintf(buffer, "%s*", fn);
			bsd_glob(buffer, globflags|GLOB_APPEND, NULL, globpat);
		}
		while (i < globpat->gl_pathc)
		{
			for (i = old_glpathc, old_glpathc = globpat->gl_pathc; i < old_glpathc; i++)
			{
				char *fn;
				fn = globpat->gl_pathv[i];
				if (fn[strlen(fn)-1] != '/')
					continue;
				sprintf(buffer, "%s*", fn);
				bsd_glob(buffer, globflags|GLOB_APPEND, NULL, globpat);
			}
		}
	}
	return 0;
}

unsigned int print_mp3(char *pattern, char *dir, char *format, int freq, int number, int bitrate, int md5, FILE * fp)
{
unsigned int count = 0;
FileStruct *new;
char dir1[BIG_BUFFER_SIZE];
char *fs = NULL;
	*dir1 = 0;
	for (new = fserv_files; new; new = new->next)
	{
		if (!pattern || (pattern && wild_match(pattern, new->filename)))
		{
			char *p;
			p = base_name(new->filename);
			if ((bitrate != -1) && (new->bitrate != bitrate))
				continue;
			if ((freq != -1) && (new->freq != freq))
				continue;
			if (!format || !*format)
			{
				if (md5)
				{
					if (new->type == AUDIO)
					{
						if (fp)
	 						fprintf(fp, "\"%s\" %s %dk [%s]\n", p, new->checksum, new->bitrate, print_time(new->seconds));
						else
							put_it("\"%s\" %s %dk [%s]", p, new->checksum, new->bitrate, print_time(new->seconds));
					}
					else
					{
						if (fp)
							fprintf(fp, "\"%s\" %s %1.2f%s %s\n", p, new->checksum, _GMKv(new->filesize), _GMKs(new->filesize), find_mime_type(new->filename));
						else
							put_it("\"%s\" %s %1.2f%s %s", p, new->checksum, _GMKv(new->filesize), _GMKs(new->filesize), find_mime_type(new->filename));
					}
				}
				else
				{
					if (new->type == AUDIO)
					{
						if (fp)
							fprintf(fp, "\"%s\" %s %dk [%s]\n", p, mode_str(new->stereo), new->bitrate, print_time(new->seconds));
						else
							put_it("\"%s\" %s %dk [%s]", p, mode_str(new->stereo), new->bitrate, print_time(new->seconds));
					}
					else
					{
						if (fp)
							fprintf(fp, "\"%s\" %1.2f%s %s\n", p, _GMKv(new->filesize), _GMKs(new->filesize), find_mime_type(new->filename));
						else
							put_it("\"%s\" %1.2f%s %s", p, _GMKv(new->filesize), _GMKs(new->filesize), find_mime_type(new->filename));
					}
				}
			}
			else
			{
				if (!(fs = make_mp3_string(fp, new, dir, format, dir1)))
				{
					if (!fp)
						put_it("Directory [ %s ]", dir1);
					fs = make_mp3_string(fp, new, dir, format, dir1);
				}
				if (!fp)
					put_it("%s", fs);
			}
			count++;
			if ((number > 0) && (count == number))
				return count;
		}
	}
	return count;
}

BUILT_IN_COMMAND(print_napster)
{
	int	count = 0, 
		i, 
		dir = 0,
		bitrate = -1,
		number = -1,
		freq = -1,
		md5 = 0;
	char 	*fs_output = NULL,
		*dir_output = NULL,
		*tmp_pat = NULL,
		*filename = NULL,
		**path = NULL;
	FILE	*fp = NULL;
		
	if ((get_string_var(FORMAT_FILENAME_VAR)))
		fs_output = m_strdup(get_string_var(FORMAT_FILENAME_VAR));
	if ((get_string_var(FORMAT_DIRECTORY_VAR)))
		dir_output = m_strdup(get_string_var(FORMAT_DIRECTORY_VAR));
	if (args && *args)
	{
		char *tmp;
		while ((tmp = next_arg(args, &args)) && *tmp)
		{
			int len;
			len = strlen(tmp);
			if (!my_strnicmp(tmp, "-BITRATE", len))
			{
				if ((tmp = next_arg(args, &args)))
					bitrate = (unsigned int) my_atol(tmp);
			}
			else if (!my_strnicmp(tmp, "-COUNT", len))
			{
				if ((tmp = next_arg(args, &args)))
					number = (unsigned int) my_atol(tmp);
			} 
			else if (!my_strnicmp(tmp, "-FREQ", 3))
			{
				if ((tmp = next_arg(args, &args)))
					freq = (unsigned int)my_atol(tmp);
			} 
			else if (!my_strnicmp(tmp, "-MD5", 3))
				md5 = 1;
			else if (!my_strnicmp(tmp, "-FORMAT", 3))
			{
				if ((tmp = new_next_arg(args, &args)))
					malloc_strcpy(&fs_output, tmp);
			} 
			else if (!my_strnicmp(tmp, "-FILE", 3))
			{
				if ((tmp = next_arg(args, &args)))
				{
					char *p;
					filename = tmp;
					if ((p = expand_twiddle(tmp)))
					{
						if (!(fp = fopen(p, "w")))
						{
							new_free(&p);
							say("Unable to open %s", filename);
							return;
						}
					}
					new_free(&p);
				}
				tmp = NULL;
			}
			else
			{
				dir++;
				path = RESIZE(path, sizeof(char *), dir);
				path[dir-1] = m_strdup(tmp);
			}
		}
	}
	if (dir)
	{
		for (i = 0; i < dir; i++)
		{
			char patbuf[BIG_BUFFER_SIZE];
			sprintf(patbuf, "*%s*", path[i]);
			count += print_mp3(patbuf, dir_output, fs_output, freq, number, bitrate, md5, fp);
			m_s3cat(&tmp_pat, " ", path[i]);
			new_free(&path[i]);
		}
		new_free(&path);
	}
	else
		count += print_mp3(NULL, dir_output, fs_output, freq, number, bitrate, md5, fp);
	
	if (filename)
		say("Found %d files matching \"%s\" saved in %s", count, tmp_pat ? tmp_pat : "*", filename);
	else
		say("Found %d files matching \"%s\"", count, tmp_pat ? tmp_pat : "*");
	new_free(&tmp_pat);
	new_free(&fs_output);
	new_free(&dir_output);
	if (fp)
		fclose(fp);
}

/* 
 * Header parsing code borrowed from mpg123 and modifed for my use.
 */

static unsigned long convert_to_header(unsigned char * buf)
{
	return (buf[0] << 24) + (buf[1] << 16) + (buf[2] << 8) + buf[3];
}


static int mpg123_head_check(unsigned long head)
{
	if ((head & 0xffe00000) != 0xffe00000)
		return 0;
	if (!((head >> 17) & 3))
		return 0;
	if (((head >> 12) & 0xf) == 0xf)
		return 0;
	if (!((head >> 12) & 0xf))
		return 0;
	if (((head >> 10) & 0x3) == 0x3)
		return 0;
	if (((head >> 19) & 1) == 1 && ((head >> 17) & 3) == 3 && ((head >> 16) & 1) == 1)
		return 0;
	return 1;
}

int tabsel_123[2][3][16] =
{
	{
    {0, 32, 64, 96, 128, 160, 192, 224, 256, 288, 320, 352, 384, 416, 448,},
       {0, 32, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 384,},
       {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320,}},

	{
       {0, 32, 48, 56, 64, 80, 96, 112, 128, 144, 160, 176, 192, 224, 256,},
	    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160,},
	    {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160,}}
};

long mpg123_freqs[9] =
{44100, 48000, 32000, 22050, 24000, 16000, 11025, 12000, 8000};

int parse_header(AUDIO_HEADER *fr, unsigned long newhead)
{
	double bpf;
	
	/* 00 2.5
	   01 reserved 
	   10 version2
	   11 version1
	*/
	if (newhead & (1 << 20))
	{
		fr->ID = (newhead & (1 << 19)) ? 0x0 : 0x1;
		fr->mpeg25 = 0;
	}
	else
	{
		fr->ID = 1;
		fr->mpeg25 = 1;
	}
	fr->layer = ((newhead >> 17) & 3);
	if (fr->mpeg25)
		fr->sampling_frequency = 6 + ((newhead >> 10) & 0x3);
	else
		fr->sampling_frequency = ((newhead >> 10) & 0x3) + (fr->ID * 3);

	fr->error_protection = ((newhead >> 16) & 0x1) ^ 0x1;

	if (fr->mpeg25)		/* allow Bitrate change for 2.5 ... */
		fr->bitrate_index = ((newhead >> 12) & 0xf);

	fr->bitrate_index = ((newhead >> 12) & 0xf);
	fr->padding = ((newhead >> 9) & 0x1);
	fr->extension = ((newhead >> 8) & 0x1);
	fr->mode = ((newhead >> 6) & 0x3);
	fr->mode_ext = ((newhead >> 4) & 0x3);
	fr->copyright = ((newhead >> 3) & 0x1);
	fr->original = ((newhead >> 2) & 0x1);
	fr->emphasis = newhead & 0x3;

	fr->stereo = (fr->mode == 3) ? 1 : 2;
	fr->true_layer = 4 - fr->layer;

	if (!fr->bitrate_index)
		return 0;

	switch (fr->true_layer)
	{
		case 1:
			fr->bitrate = tabsel_123[fr->ID][0][fr->bitrate_index];
			fr->framesize = (long) tabsel_123[fr->ID][0][fr->bitrate_index] * 12000;
			fr->framesize /= mpg123_freqs[fr->sampling_frequency];
			fr->framesize = ((fr->framesize + fr->padding) << 2) - 4;
			fr->freq = mpg123_freqs[fr->sampling_frequency];
			break;
		case 2:
			fr->framesize = (long) tabsel_123[fr->ID][1][fr->bitrate_index] * 144000;
			fr->framesize /= mpg123_freqs[fr->sampling_frequency];
			fr->framesize += fr->padding - 4;
			fr->freq = mpg123_freqs[fr->sampling_frequency];
			fr->bitrate = tabsel_123[fr->ID][1][fr->bitrate_index];
			break;
		case 3:
		{
			fr->bitrate = tabsel_123[fr->ID][2][fr->bitrate_index];
			fr->framesize = (long) tabsel_123[fr->ID][2][fr->bitrate_index] * 144000;
			fr->framesize /= mpg123_freqs[fr->sampling_frequency] << (fr->ID);
			fr->framesize = fr->framesize + fr->padding - 4;
			fr->freq = mpg123_freqs[fr->sampling_frequency];
			break;
		}
		default:
			return 0;
	}
	if (fr->framesize > 1792) /* supposedly max framesize */
		return 0;
	switch (fr->true_layer)
	{
		case 1:
			bpf =  tabsel_123[fr->ID][0][fr->bitrate_index];
			bpf *= 12000.0 * 4.0;
			bpf /= mpg123_freqs[fr->sampling_frequency] << (fr->ID);
			break;
		case 2:
		case 3:
			bpf = tabsel_123[fr->ID][fr->true_layer - 1][fr->bitrate_index];
			bpf *= 144000;
			bpf /= mpg123_freqs[fr->sampling_frequency] << (fr->ID);
			break;
		default:
			bpf = 1.0;
	}
	fr->totalframes = fr->filesize / bpf;
	return 1;
}

double compute_tpf(AUDIO_HEADER *fr)
{
	static int bs[4] = {0, 384, 1152, 1152};
	double tpf;

	tpf = (double) bs[fr->true_layer];
	tpf /= mpg123_freqs[fr->sampling_frequency] << (fr->ID);
	return tpf;
}

int check_vbrheader(AUDIO_HEADER *fr, unsigned char *buf, XHEADDATA *xing)
{
	int ssize;

	if (fr->true_layer != 3)
		return 0;

	if (fr->ID)
		ssize = (fr->stereo == 1) ? 9 : 17;
	else
		ssize = (fr->stereo == 1) ? 17 : 32;

	xing->samprate = fr->freq;
	xing->h_id = fr->ID;

	buf += ssize;
	if (( buf[0] != 'X' ) || ( buf[1] != 'i' ) ||
	    ( buf[2] != 'n' ) || ( buf[3] != 'g' ) ) 
		return 0;

	buf += 4;
	xing->flags = convert_to_header(buf);

#if 1
	if (xing->h_id == 0)
		xing->samprate >>= 1;
#endif
	
	if (xing->flags & VBR_FRAMES_FLAG)
	{
		xing->frames = convert_to_header(buf);
		buf += 4;
	}

	if (xing->flags & VBR_BYTES_FLAG) 
	{
		xing->bytes  = convert_to_header(buf); 
		buf += 4;
	}

	if (xing->flags & VBR_TOC_FLAG) 
	{
		xing->toc = new_malloc(101);
		memcpy(xing->toc,buf,100);
		buf += 100;
	}
	xing->vbr_scale = -1;
	if (xing->flags & VBR_SCALE_FLAG) 
	{
		xing->vbr_scale = convert_to_header(buf);
		buf += 4;
	}
	return 1;
}

long get_bitrate(int fdes, time_t *mp3_time, unsigned int *freq_rate, unsigned long *filesize, int *stereo, long *id3, int *mime_type)
{


	AUDIO_HEADER header = {0}, hr1 = {0};
	XHEADDATA xing_header = {0};
	unsigned long btr = 0;
	struct stat	st;
	unsigned long	head;
	double tpf;
	unsigned long avg_bitrate = 0;

unsigned char	buf[DET_BUFFER_SIZE+1];
unsigned char	tmp[5];
			
	if (freq_rate)
		*freq_rate = 0;

	fstat(fdes, &st);

	if (!(*filesize = st.st_size))
		return 0;
	memset(tmp, 0, sizeof(tmp));
	read(fdes, tmp, 4);

	if (!strcmp(tmp, "PK\003\004")) /* zip magic */
		return 0;
	if (!strcmp(tmp, "PE") || !strcmp(tmp, "MZ")) /* windows Exe magic */
		return 0;
	if (!strcmp(tmp, "\037\235")) /* gzip/compress */
		return 0;
	if (!strcmp(tmp, "\037\213") || !strcmp(tmp, "\037\036") || !strcmp(tmp, "BZh")) /* gzip/compress/bzip2 */
		return 0;
	if (!strcmp(tmp, "\177ELF")) /* elf binary */
		return 0;
		
	head = convert_to_header(tmp);

	if ((head == 0x000001ba) || (head == 0x000001b3)) /* got a video mpeg header */
		return 0;
	if ((head == 0xffd8ffe0) || (head == 0x47494638)) /* jpeg image  gif image */
		return 0;
	if ((head == 0xea60)) /* ARJ magic */
		return 0;

	while (!mpg123_head_check(head))
	{
		int in_buf;
		int i;
		/*
		 * The mpeg-stream can start anywhere in the file,
		 * so we check the entire file
		 */
		/* Optimize this */
		if ((in_buf = read(fdes, buf, DET_BUFFER_SIZE) ) < 1)
			return 0;
		for (i = 0; i < in_buf; i++)
		{
			head <<= 8;
			head |= buf[i];
			if(mpg123_head_check(head))
			{
				lseek(fdes, i+1-in_buf, SEEK_CUR);
				break;
			}
		}
	}
	header.filesize = st.st_size;
	if (!(parse_header(&header, head)))
		return 0;
	read(fdes, buf, header.framesize);
	if (!check_vbrheader(&header, buf, &xing_header))
	{	
		btr = header.bitrate;
		*mp3_time = (time_t) (header.totalframes * compute_tpf(&header));
	}
	else if (xing_header.toc)
	{
		int i = 0, count = 0;

		tpf = compute_tpf(&header);
		*mp3_time = (time_t) (tpf * xing_header.frames * 1000);

#if 1
		btr = header.bitrate;
		avg_bitrate = header.bitrate;
		lseek(fdes, 0, SEEK_SET);
		parse_header(&hr1, head);
		while (1)
		{
			i++;
			count += hr1.framesize;
			if (count >= header.filesize)
				break;
			count += hr1.error_protection ? 6 : 4;
			lseek(fdes, count, SEEK_SET);
			if ((read(fdes, tmp, 4)) < 4)
				break;
			head = convert_to_header(tmp);
			if (!mpg123_head_check(head))
				continue;
			memset(&hr1, 0, sizeof(XHEADDATA));
			parse_header(&hr1, head);
			avg_bitrate += hr1.bitrate;
		}
		if (i)
			btr = header.bitrate = avg_bitrate / i;
#else
		header.bitrate = (xing_header.bytes * 8) / (tpf * xing_header.frames * 1000);
#endif
		header.totalframes = xing_header.frames;
		new_free(&xing_header.toc);
	}
	*freq_rate = header.freq;
	if (id3)
	{
		char buff[130];
		int rc;
		lseek(fdes, 0, SEEK_SET);
		*id3 = 0;
		rc = read(fdes, buff, 128);
		if (!strncmp(buff, "ID3", 3))
		{
			struct id3v2 {
				char tag[3];
				unsigned char ver[2];
				unsigned char flag;
				unsigned char size[4];
			} id3v2;
			unsigned char bytes[4];
			/* this is id3v2 */
			memcpy(&id3v2, buff, sizeof(id3v2));
			bytes[3] = id3v2.size[3] | ((id3v2.size[2] & 1 ) << 7);
			bytes[2] = ((id3v2.size[2] >> 1) & 63) | ((id3v2.size[1] & 3) << 6);
			bytes[1] = ((id3v2.size[1] >> 2) & 31) | ((id3v2.size[0] & 7) << 5);
			bytes[0] = ((id3v2.size[0] >> 3) & 15);

			*id3 = (bytes[3] | (bytes[2] << 8) | ( bytes[1] << 16) | ( bytes[0] << 24 )) + sizeof(struct id3v2);
		}
		lseek(fdes, st.st_size-128, SEEK_SET);
		rc = read(fdes, buff, 128);
		if ((rc == 128) && !strncmp(buff, "TAG", 3))
			*id3 = *id3 ? (*id3 * -1) : 1;
	}
	*stereo = header.mode;
	return btr;
}


char *calc_md5(int r, unsigned long mapsize)
{
#ifdef WANT_MD5
unsigned char digest[16];			
md5_state_t state;
char buffer[BIG_BUFFER_SIZE+1];
struct stat st;
unsigned long size = DEFAULT_MD5_SIZE;
int di = 0;
int rc;
#ifdef HAVE_MMAP
	char *m;
#endif
#define MD5_BUFFER_SIZE 4 * BIG_BUFFER_SIZE

	*buffer = 0;
	md5_init(&state);
	if ((fstat(r, &st)) == -1)
		return m_strdup("");
		
	if (!mapsize)
	{
		if (st.st_size < size)
			size = st.st_size;
	}
	else if (st.st_size < mapsize)
		size = st.st_size;
	else
		size = mapsize;

#ifndef HAVE_MMAP
	while (size)
	{
		unsigned char md5_buff[MD5_BUFFER_SIZE + 1];
		rc = (size >= (MD5_BUFFER_SIZE)) ?  MD5_BUFFER_SIZE : size;
		if ((rc = read(r, md5_buff, rc)) <= 0)
			break;
		md5_append(&state, (unsigned char *)md5_buff, rc);
		if (size >= MD5_BUFFER_SIZE)
			size -= rc;
		else
			size = 0;
	}						
	md5_finish(digest, &state);
	{
#else
	if ((m = mmap((void *)0, size, PROT_READ, MAP_PRIVATE, r, 0)) != MAP_FAILED)
	{
		md5_append(&state, (unsigned char *)m, size);
		md5_finish(digest, &state);
		munmap(m, size);
#endif
		memset(buffer, 0, 200);
		for (di = 0, rc = 0; di < 16; ++di, rc += 2)
			snprintf(&buffer[rc], BIG_BUFFER_SIZE, "%02x", digest[di]);
		strcat(buffer, "-");
		strcat(buffer, ltoa(st.st_size));
	}
	return m_strdup(buffer);
#else
	return m_strdup("NOMD5COMPUTED");
#endif
}

unsigned int file_type(char *fn)
{
	if (wild_match(DEFAULT_FILEMASK, fn))
		return AUDIO;
	if (wild_match("*.mpg", fn) || wild_match("*.dat", fn))
		return VIDEO;
	if (wild_match("*.jpg", fn) || wild_match("*.gif", fn))
		return IMAGE;
	return ANY_FILE;
}

int add_list_dir(List *a, List *b)
{
char *cp, c;
int depth, deptha, depthb;
	for (cp = a->name, depth = 0;;) {
		c = *cp++;
		if (c == '\0')
			break;
		if ((c == '/') || (c == '\\')) {
			depth++;
		}
	}
	deptha = depth;

	for (cp = b->name, depth = 0;;) {
		c = *cp++;
		if (c == '\0')
			break;
		if ((c == '/') || (c == '\\')) {
			depth++;
		}
	}
	depthb = depth;

	if (deptha < depthb)
		return (-1);
	else if (deptha > depthb)
		return (1);


	return strcmp(a->name, b->name);
}

#ifdef WINNT
DWORD scan_mp3_dir(LPVOID arg)
#else
static void *scan_mp3_dir(void *arg)
#endif
{
	glob_t	globpat;
	int	globflag;
	
	int	i = 0;
	FileStruct	*new;
	int	r;
	int	type;
	Share_Struct *sf, *last;
	int count = 0;
	in_load++;

#ifdef WINNT
	globflag = GLOB_MARK|GLOB_NOSORT|GLOB_INSENSITIVE;
#else
	globflag = GLOB_MARK|GLOB_NOSORT;
#endif		
	if (get_int_var(SHARE_LINKS_VAR))
		globflag |= GLOB_LINK;
#if defined(WANT_THREAD) && !defined(WINNT)
	/* Lock the queue */
	share_thread_signal_setup();
	pthread_mutex_lock(&fserv_struct_mutex);
#endif
	fserv_start = now;
	
	for (sf = share_files; sf; sf = last)
	{
		last = sf->next;
		count = 0;
		if (sf->update)
		{
			do_update(NULL);
			new_free(&sf->path);
			new_free(&sf);
			share_files = last;
			continue;
		}	
		memset(&globpat, 0, sizeof(glob_t));
		read_glob_dir(sf->path, globflag, &globpat, sf->recurse);
		for (i = 0; i < globpat.gl_pathc; i++)
		{
			char	*fn;
			long	id3 = 0;
			fn = globpat.gl_pathv[i];
#if defined(WANT_THREAD) && !defined(WINNT)
			if (pthread_mutex_trylock(&quit_mutex) == 0) 
			{
				pthread_mutex_unlock(&quit_mutex);
				break;
			}
#endif
			if (fn[strlen(fn)-1] == '/')
			{
#if defined(WANT_THREAD) && defined(WINNT)
				strncpy(return_dir, fn, BIG_BUFFER_SIZE);
				build_status(current_window, NULL, 0);
#endif
				continue;
			}
			type = file_type(fn);
			if (sf->reload)
			{
		 		if ((new = (FileStruct *)find_in_list((List **)&fserv_files, globpat.gl_pathv[i], 0)))
				{
					FileStruct *r;
					struct stat st;
					
					if (stat(new->filename, &st))
					{
						if ((r = (FileStruct *)remove_from_list((List **)&fserv_files, new->filename)))
						{
							if (r->shared)
							{
								send_ncommand(CMDS_REMOVEFILE, "%s", convertnap_dos(r->filename));
								shared_stats.shared_files--;
								shared_stats.shared_filesize -= r->filesize;
							}
							shared_stats.total_files--;
							shared_stats.total_filesize -= r->filesize;
							remove_file(&r);
						}
					}
					continue;
				}
			} else if (find_in_list((List **)&fserv_files, globpat.gl_pathv[i], 0))
				continue;
			if ((sf->share_search_type != ANY_FILE) && !(sf->share_search_type & type))
				continue;
			if ((r = open(fn, O_RDONLY)) == -1)
				continue;
			new = (FileStruct *) new_malloc(sizeof(FileStruct));
			new->filename = m_strdup(fn);
			new->bitrate = get_bitrate(r, &new->seconds, &new->freq, &new->filesize, &new->stereo, &id3, &new->type);
			new->type = type;
			if (new->filesize && new->bitrate)
			{
				unsigned long size = DEFAULT_MD5_SIZE;
				switch(id3)
				{
					case 1:
					{
						if (new->filesize < size)
							size = new->filesize - 128;
					}
					case 0:
						lseek(r, 0, SEEK_SET);
						break;
					default:
					{
						lseek(r, (id3 < 0) ? -id3 : id3, SEEK_SET);
						if (id3 > 0)
						{
							if ((new->filesize - id3) < size)
								size = new->filesize - id3;
						}
						else
						{
							/*blah. got both tags. */
							if ((new->filesize + id3 - 128) < size)
								size = new->filesize + id3 - 128;
						}
						break;
					}
				}
				new->checksum = calc_md5(r, size);
				close(r);
				r = -1;
				add_to_list_ext((List **)&fserv_files, (List *)new, 0, add_list_dir);
				shared_stats.total_files++;
				shared_stats.total_filesize += new->filesize;
				count++;
#if defined(WANT_THREAD) && !defined(WINNT)
				pthread_mutex_lock(&shared_count_mutex);
				shared_count++;
				pthread_mutex_unlock(&shared_count_mutex);
#else
				shared_count++;
#endif
#if !defined(WANT_THREAD) && !defined(WINNT)
				if (!(count % 25))
				{
					io("scan_mp3_dir");
					build_status(current_window, NULL, 0);
				}
#endif
			}
			else if (sf->share_search_type != MP3_ONLY)
			{
				unsigned long size = DEFAULT_MD5_SIZE;
				if (new->filesize < size)
					size = new->filesize;
				new->checksum = calc_md5(r, size);
				close(r);
				r = -1;
				add_to_list_ext((List **)&fserv_files, (List *)new, 0, add_list_dir);
				shared_stats.total_files++;
				shared_stats.total_filesize += new->filesize;
				count++;
#if defined(WANT_THREAD) && !defined(WINNT)
				pthread_mutex_lock(&shared_count_mutex);
				shared_count++;
				pthread_mutex_unlock(&shared_count_mutex);
#else
				shared_count++;
#endif
			}
			else
			{
				new_free(&new->filename);
				new_free(&new);
			}
			if (r !=  -1)
				close(r);
			if ((sf->count != -1) && (sf->count == count))
				break;
		}
		
		bsd_globfree(&globpat);
#if !defined(WINNT)
		if (!count)
			say("Could not find any files in [%s]", sf->path);
		else 
		{
			if (last)
				say("Found %d Files in [%s]", count, sf->path);
			else
				say("Found %d Files in [%s]%s", count, sf->path, sf->share ? "" : ". To share these type /share -share");
		}
#endif
		if ((sf->total != -1) && (sf->total == shared_count))
		{
			new_free(&sf->path);
			new_free(&sf);
			share_files = last;
			break;
		}
		new_free(&sf->path);
		new_free(&sf);
		share_files = last;
	}
	if (shared_count > 0)
		scan_done = shared_count;
	else
		scan_done = -1;

#if !defined(WINNT)
	if (shared_count)
		say("Found %d files in %s", shared_count, print_time(time(NULL) - fserv_start));
	else
		say("No files found");
#endif
#if defined(WANT_THREAD) && !defined(WINNT)
	pthread_mutex_lock(&shared_count_mutex);
	shared_count = 0;
	pthread_mutex_unlock(&shared_count_mutex);
	pthread_mutex_unlock(&fserv_struct_mutex);
#else
	shared_count = 0;
#endif
	*return_dir = 0;
	in_load--;
#if defined(WANT_THREAD)
#if defined(WINNT)
	return 0;
#endif
#endif
}

void clear_shared(void)
{
FileStruct *new;
	for (new = fserv_files; new; new = new->next)
		new->shared = 0;
}

unsigned long load_shared(char *fname)
{
char buffer[BIG_BUFFER_SIZE+1];
char *expand = NULL;
FILE *fp;
unsigned long count = 0;
FileStruct *new;

	if (!strchr(fname, '/'))
#ifdef WINNT
		sprintf(buffer, "~/TekNap/%s", fname);
#else
		sprintf(buffer, "~/.TekNap/%s", fname);
#endif
	else
		sprintf(buffer, "%s", fname);
	expand = expand_twiddle(buffer);
	if ((fp = fopen(expand, "r")))
	{
		char *fn, *md5, *fs, *br, *fr, *t, *args;
		while (!feof(fp))
		{
#if defined(WANT_THREAD) && !defined(WINNT)
			if (pthread_mutex_trylock(&quit_mutex) == 0) 
			{
				pthread_mutex_unlock(&quit_mutex);
				break;
			}
#endif
			if (!fgets(buffer, BIG_BUFFER_SIZE, fp))
				break;
			args = &buffer[0];
				
			chop(args, 1);
			fn = new_next_arg(args, &args);
			if (!fn || !*fn)
				continue;
			if (find_in_list((List **)&fserv_files, fn, 0))
				continue;
			if (!(md5 = next_arg(args, &args)) || 
				!(fs = next_arg(args, &args)) ||
				!(br = next_arg(args, &args)) ||
				!(fr = next_arg(args, &args)) ||
				!(t = next_arg(args, &args)))
				continue;
			new = (FileStruct *) new_malloc(sizeof(FileStruct));
			new->filename = m_strdup(fn);
			new->checksum = m_strdup(md5);
			new->seconds = my_atol(t);
			new->bitrate = my_atol(br);
			new->freq = my_atol(fr);
			new->filesize = my_atol(fs);
			new->stereo = 1;
			new->shared = 0;
			add_to_list_ext((List **)&fserv_files, (List *)new, 0, add_list_dir);
			count++;
			shared_stats.total_files++;
			shared_stats.total_filesize += new->filesize;
		}
		fclose(fp);
	} 
#if !defined(WINNT)
	else
		say("Error loading %s/%s %s", buffer, expand ? expand : empty_string, strerror(errno));
	if (count)
		say("Finished loading ~/.TekNap/%s. %lu files", fname, count);
#endif
	new_free(&expand);
	return count;
}

unsigned long save_shared(char *fname)
{
char buffer[BIG_BUFFER_SIZE+1];
char *expand = NULL;
FILE *fp;
int count = 0;
FileStruct *new;
	if (!fname || !*fname)
		return 0;
	if (!strchr(fname, '/'))
#ifdef WINNT
		sprintf(buffer, "~/TekNap/%s", fname);
#else
		sprintf(buffer, "~/.TekNap/%s", fname);
#endif
	else
		sprintf(buffer, "%s", fname);
	expand = expand_twiddle(buffer);
	if ((fp = fopen(expand, "w")))
	{
		for (new = fserv_files; new; new = new->next)
		{
			fprintf(fp, "\"%s\" %s %lu %u %u %lu\n",
				new->filename, new->checksum, new->filesize, 
				new->bitrate, new->freq, new->seconds);
			count++;
		}
		fclose(fp);
		say("Finished saving %s [%d]", buffer, count);
	} else
		say("Error saving %s / %s %s", buffer, expand ? expand : empty_string, strerror(errno));
	new_free(&expand);
	return count;
}


unsigned long unshare_nap(void)
{
#if 0
FileStruct *new;
char *filename;
#endif
unsigned long count = 0;
	if (shared_stats.shared_files)
	{
		send_ncommand(CMDS_REMOVEALLFILES, NULL);
		count = shared_stats.shared_files;
#if 0
		for (new = fserv_files; new; new = new->next)
		{
			if (new->shared)
			{
				filename = LOCAL_COPY(new->filename);
				send_ncommand(CMDS_REMOVEFILE, "%s", convertnap_dos(filename));
				count++;
			}
		}
#endif
		clear_shared();
	}
	shared_stats.shared_files = 0;
	shared_stats.shared_filesize = 0;
	return count;
}

unsigned long clear_nap(void)
{
unsigned long count = 0;
	unshare_nap();
	count = shared_stats.total_files;
	shared_stats.total_files = 0;
	shared_stats.total_filesize = 0;
	clear_files(&fserv_files);
	return count;
}

#ifdef WINNT
DWORD load_file(LPVOID args)
#else
void *load_file(void *args)
#endif
{
Share_Struct *new, *last;
#if defined(WANT_THREAD) && !defined(WINNT)
	/* Lock the queue */
	share_thread_signal_setup();
	pthread_mutex_lock(&fserv_struct_mutex);
#endif
	in_load++;
	for (new = (Share_Struct *)load_share_files; new; new = last)
	{
		last = new->next;

		load_shared(new->path);

		new_free(&new->path);
		new_free(&new);
		load_share_files = last;
	}
	in_load--;
#if defined(WANT_THREAD) && !defined(WINNT)
	/* Lock the queue */
	pthread_mutex_unlock(&fserv_struct_mutex);
#endif
#ifdef WINNT
	return 0;
#else
#if defined(WANT_THREAD) && !defined(WINNT)
	pthread_exit(NULL);
#endif
#endif
}

unsigned long save_file(char *args)
{
char *fn;
	fn = next_arg(args, &args);
	return save_shared((fn && *fn) ? fn : shared_fname);
}

void *do_update(void *arg)
{
	FileStruct *tmp, *last, *r;
	unsigned long count = 0;
	struct stat st;
	in_load++;
#if defined(WANT_THREAD) && !defined(WINNT)
	pthread_mutex_lock(&fserv_struct_mutex);
#endif
	for (tmp = fserv_files; tmp; tmp = last)
	{
#if defined(WANT_THREAD) && !defined(WINNT)
		if (pthread_mutex_trylock(&quit_mutex) == 0) 
		{
			pthread_mutex_unlock(&quit_mutex);
			break;
		}
#endif
		last = tmp->next;
		if (stat(tmp->filename, &st))
		{
			if ((r = (FileStruct *)remove_from_list((List **)&fserv_files, tmp->filename)))
			{
				if (r->shared)
				{
					send_ncommand(CMDS_REMOVEFILE, "%s", convertnap_dos(r->filename));
					shared_stats.shared_files--;
					shared_stats.shared_filesize -= r->filesize;
				}
				shared_stats.total_files--;
				shared_stats.total_filesize -= r->filesize;
				remove_file(&r);
			}
			continue;
		}
		count++;
	}
	in_load--;
#if defined(WANT_THREAD) && !defined(WINNT)
	pthread_mutex_unlock(&fserv_struct_mutex);
	pthread_exit(NULL);
#endif
}

unsigned long remove_nap(char *pattern)
{
FileStruct *tmp;
int count = 0;
	while ((tmp = (FileStruct *) removewild_from_list((List **)&fserv_files, pattern)))
	{
		if (tmp->shared)
		{
			send_ncommand(CMDS_REMOVEFILE, "%s", convertnap_dos(tmp->filename));
			shared_stats.shared_files--;
			shared_stats.shared_filesize -= tmp->filesize;
		}
		shared_stats.total_files--;
		shared_stats.total_filesize -= tmp->filesize;
		remove_file(&tmp);
		count++;
	}
	return count;
}

/* stolen blatantly from Sheiks cdns non-blocking DNS routines */
void init_share_mutexes(void)
{
#ifdef WANT_THREAD
#ifndef WINNT
	pthread_mutex_init(&fserv_struct_mutex, NULL);
	pthread_mutex_init(&shared_count_mutex, NULL);
	pthread_mutex_init(&quit_mutex, NULL);
	pthread_mutex_init(&send_ncommand_mutex, NULL);

	pthread_mutex_unlock(&fserv_struct_mutex);
	pthread_mutex_unlock(&shared_count_mutex);
	pthread_mutex_unlock(&send_ncommand_mutex);
	pthread_mutex_unlock(&quit_mutex);

	pthread_mutex_lock(&quit_mutex);
#endif
#endif
}

BUILT_IN_COMMAND(share_napster)
{
char *path;
int share_recurse = 1;
int share_update = 0;
int share_reload = 0;
int share_search_type = MP3_ONLY;
int share_count = -1;
int share_total = -1;

	if (in_sharing || in_load)
	{
		say("Please wait while we finish %s files", in_sharing ? "sharing" : "loading");
		return;
	}
	if (!args || !*args)
	{
		char *pch, *p;
		path = get_string_var(DOWNLOAD_DIRECTORY_VAR);

		if (!path || !*path)
		{
			say("No path. /set DOWNLOAD_DIRECTORY first.");
			return;
		}
		path = LOCAL_COPY(path);
		
		while ((pch = new_next_arg(path, &path)))
		{
			if (!pch || !*pch)
				break;
			p = alloca(strlen(pch)+4);
			sprintf(p, "%s/", pch);
			p = expand_twiddle(p);
			if (p)
			{
				Share_Struct *new;
				new = (Share_Struct *)new_malloc(sizeof(Share_Struct));
				new->recurse = share_recurse;
				new->reload = share_reload;
				new->update = share_update;
				new->path = p;
				new->next = share_files;
				new->count = -1;
				new->total = -1;
				new->next = share_files;
				share_files = new;
			}
		}
		goto do_sharing;
	}
	share_search_type = MP3_ONLY;
	while ((path = new_next_arg(args, &args)))
	{
		if (!path || !*path)
			break;
		share_count = -1;
		if (!my_stricmp(path, "-share"))
		{
			if (share_files)
				return;
			say("Sharing %lu files", do_share(NULL));
		}
		else if (!my_stricmp(path, "-load"))
		{
			Share_Struct *new;
			path = next_arg(args, &args);
			new = (Share_Struct *)new_malloc(sizeof(Share_Struct));
			if (path && *path)
				new->path = m_strdup(path);
			else
				new->path = m_strdup(shared_fname);
			if (share_files)
				new->next = load_share_files;
			load_share_files = new;
#if !defined(WANT_THREAD)
#if !defined(WINNT)
			load_file(load_share_files);
			goto done_share;
#endif
#endif
		}
		else if (!my_stricmp(path, "-save"))
		{
			path = next_arg(args, &args);
			save_file(path);
		}
		else if (!my_stricmp(path, "-clear"))
			say("Cleared %d files", clear_nap());
		else if (!my_stricmp(path, "-unshare"))
			say("Unshared %d files", unshare_nap());
		else if (!my_stricmp(path, "-remove"))
		{
			while ((path = next_arg(args, &args)))
				say("Removed %d files from shared files matching %s", remove_nap(path), path);
		}
		else if (!my_stricmp(path, "-count"))
			share_count = my_atol(next_arg(args, &args));
		else if (!my_stricmp(path, "-total"))
			share_total = my_atol(next_arg(args, &args));
		else if (!my_stricmp(path, "-recurse"))
			share_recurse ^= 1;
		else if (!my_stricmp(path, "-update"))
			share_update ^= 1;
		else if (!my_stricmp(path, "-reload"))
			share_reload ^= 1;
		else if (!my_stricmp(path, "-type"))
		{
			if (!(path = next_arg(args, &args)))
			{
				say("Need to specify one of video, image, audio, any");
				goto done_share;
			}
			if (!my_stricmp(path, "video"))
				share_search_type = VIDEO_ONLY;
			else if (!my_stricmp(path, "image"))
				share_search_type = IMAGE_ONLY;
			else if (!my_stricmp(path, "any"))
				share_search_type = ANY_FILE;
			else if (!my_stricmp(path, "audio"))
				share_search_type = MP3_ONLY;
		}	
		else if (*path == '-')
			say("Unknown command %s", path+1);
		else
		{
			Share_Struct *new;
			char *p;
			p = expand_twiddle(path);
			if (!p)
				continue;
			new = (Share_Struct *)new_malloc(sizeof(Share_Struct));
			new->recurse = share_recurse;
			new->reload = share_reload;
			new->update = share_update;
			new->path = p;
			new->next = share_files;
			new->count = share_count;
			new->total = share_total;
			new->share_search_type = share_search_type;
			share_files = new;
		}
	}
do_sharing:
#ifdef WANT_THREAD
	if (load_share_files)
	{
#ifndef WINNT
		pthread_create(&fserv_thread, NULL, load_file, load_share_files);
#else
		DWORD tid;
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)load_file, (LPVOID)load_share_files, 0, &tid);
#endif
	}
#endif
	if (share_files)
	{
#if !defined(WANT_THREAD)
		scan_mp3_dir(share_files);
#elif !defined(WINNT)
		pthread_create(&fserv_thread, NULL, scan_mp3_dir, share_files);
#else
		DWORD tid;
		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)scan_mp3_dir, (LPVOID) share_files, 0, &tid);
#endif
	}
done_share:
	build_status(current_window, NULL, 0);
}

#ifdef WANT_THREAD
#ifndef WINNT
static void share_thread_signal_setup(void)
{
	sigset_t set;

	/* Create SIGQUIT signal handler for this thread */

	/* Fill set with every signal */
	sigfillset(&set);
	/* Remove the few signals that POSIX says we should */
	sigdelset(&set, SIGFPE);
	sigdelset(&set, SIGILL);
	sigdelset(&set, SIGSEGV);
	/* Apply the mask on this thread */
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}	

void clean_share_mutexes(void)
{
	pthread_mutex_unlock(&quit_mutex);
	pthread_mutex_unlock(&send_ncommand_mutex);
	pthread_mutex_destroy(&shared_count_mutex);
	pthread_mutex_destroy(&fserv_struct_mutex);
	pthread_mutex_destroy(&quit_mutex);
	pthread_mutex_destroy(&send_ncommand_mutex);
}

void scan_is_done()
{
	void *ptr;
	pthread_join(fserv_thread, &ptr);
	scan_done = 0;
}
#else

void scan_is_done()
{
	if (scan_done > 0)
		say("Found %d files in %s. Use /share -share in order to share these files", scan_done, print_time(time(NULL) - fserv_start));
	else
		say("No files found");
	build_status(current_window, NULL, 0);
	scan_done = 0;
}
#endif

#endif


unsigned long do_share(char *args)
{
FileStruct *new;
unsigned long count = 0;
int len_dir = 0;
char dirbuffer[BIG_BUFFER_SIZE+1], dir1buffer[BIG_BUFFER_SIZE+1], buffer[BIG_BUFFER_SIZE+1], tmpbuf[BIG_BUFFER_SIZE+1]; 
extern int timeout_select;
int ots = timeout_select;

	if (!(new = fserv_files))
		return 0;
	in_sharing++;
	timeout_select = 5;
	while (new)
	{
		int rc = 0;
		char *name;
		if (new->shared || (args && *args && !matchnumber(args, count)))
		{
			new = new->next;
			continue;
		}

		*buffer = 0;
		count++;
		name = LOCAL_COPY(new->filename);
		strcpy(dirbuffer, name);

		len_dir = base_name(name) - name;
		convertnap_dos(dirbuffer);
		dirbuffer[len_dir] = 0;

		name = base_name(name);
				
		snprintf(buffer, 1500, "\"%s\" \"%s\" %s %lu %u %u %lu", dirbuffer, name, new->checksum, new->filesize, new->bitrate, new->freq, new->seconds);
		new->shared = 1; /* maybe? */
		do {
			shared_stats.shared_files++;
			shared_stats.shared_filesize += new->filesize;
			if (args && *args && matchnumber(args, count))
			{
				new = new->next;
				continue;
			}
			if (!new->next) 
			{
				if (!new->shared)
				{
					sprintf(tmpbuf, "\"%s\" %s %lu %u %u %lu", base_name(name), new->checksum, new->filesize, new->bitrate, new->freq, new->seconds);
					strmopencat(buffer, 1500, space, tmpbuf, NULL); 
					new->shared = 1;
				}
				count++;
				new = new->next;
				break;			
			} 
			else
				new = new->next;

			name = LOCAL_COPY(new->filename);
			strcpy(dir1buffer, name);

			len_dir = base_name(name) - name;
			convertnap_dos(dir1buffer);
			dir1buffer[len_dir] = 0;
			convertnap_dos(name);
			
			if (!strncmp(dir1buffer, dirbuffer, len_dir))
			{
				sprintf(tmpbuf, "\"%s\" %s %lu %u %u %lu", base_name(name), new->checksum, new->filesize, new->bitrate, new->freq, new->seconds);
				if (strlen(buffer) + strlen(tmpbuf)+1 > 1450)
					break;
				strmopencat(buffer, 1500, space, tmpbuf, NULL); 
				count++;
			}
			else
				break;
			new->shared = 1;
		} while (new);
		if (*buffer)
		{
			rc = send_ncommand(CMDS_SHAREPATH, "%s", buffer);
/*			put_it("buffer = %s", buffer);*/
		}
		if (rc == -1)
		{
			in_sharing = 0;
			clear_shared();
			timeout_select = ots;
			return 0;
		}
		if (!(count % 20))
		{
			io("share napster");
			build_status(current_window, NULL, 0);
		}
		if (!new || !new->next)
			break;
	}
	in_sharing--;
	timeout_select = ots;
	return count - 1;
}


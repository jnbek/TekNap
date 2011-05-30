/*
 * MP3 player control for mpg123 versions with generic mp3 jukebox control.
 * Based heavily on cdns.c from Scott H Kilau.
 * We setup a thread which we then fork a mp3 player in giving us control of
 * stdin/stdout/stderr from the player. We can then cause the player to "LOAD"
 * a mp3 and play it, as well as controlling it's actions with several other
 * commands.
 * Copyright Colten D Edwards July 4th 2000
 * $Id: mp3control.c,v 1.1.1.1 2001/01/22 15:27:30 edwards Exp $
 */

#include <stdio.h>
#include "teknap.h"
#include "struct.h"
#include "exec.h"
#include "hook.h"
#include "ircaux.h"
#include "napster.h"
#include "newio.h"
#include "output.h"

typedef struct mp3_stats {
	char	*title;
	char	*artist;
	char	*album;
	char	*comment;
	char	*genre;
	int	year;

	char	*type;
	int	layer;
	int	freq;
	char	*mode;
	int	mode_ext;
	int	framesize;
	int	ster;
	int	copyright;
	int	crc;
	int	emp;
	int	bitrate;
	int	ext;
} Mp3Stats;

typedef struct _mp3_queue_ {
	struct _mp3_queue_ *next;
	char *filename;
	Mp3Stats stats;
} MP3_QUEUE;


#if defined(WANT_THREAD) && defined(WANT_MP3PLAYER)

/* Our static functions */
static void init_mp3_mutexes(void);
static void *start_mp3_thread(void *);
static void cleanup_mp3(void);
static void destroy_mp3_queue(MP3_QUEUE **, MP3_QUEUE **);
static void free_mp3_entry(MP3_QUEUE *);
static MP3_QUEUE *mp3_dequeue(MP3_QUEUE **,  MP3_QUEUE **);
static MP3_QUEUE *mp3_enqueue(MP3_QUEUE **, MP3_QUEUE **, MP3_QUEUE *);
#if 0
static MP3_QUEUE *mp3_enqueue_urgent(MP3_QUEUE **, MP3_QUEUE **, MP3_QUEUE *);
#endif

/* Our static globals */
static pthread_t mp3_thread;

static pthread_mutex_t pending_queue_mutex;
static pthread_mutex_t mp3_queue_mutex;
static pthread_mutex_t quit_mutex;

static pthread_cond_t pending_queue_cond;


/* holds our pending mp3s */
static MP3_QUEUE *PendingQueueHead, *PendingQueueTail;
/* holds the currently playing mp3 */
static MP3_QUEUE *Mp3QueueHead;


static int mp3_pid = -1;
static int mp3_stdin = -1;
static int mp3_stdout = -1;
static int mp3_stderr = -1;

#ifndef MP3_PLAYER
#define MP3_PLAYER "/usr/bin/mpg123-m"
#endif

#define Q_NEXT(tmp) ((tmp)->next)
#define Q_OPEN(headp, tailp) (*(headp) = *(tailp) = NULL)

int send_mp3_command(char *fmt, ...)
{
char buffer[2*BIG_BUFFER_SIZE+1];
va_list ap;
	if (mp3_stdin == -1)
		return -1;
	if (fmt)
	{
		va_start(ap, fmt);
		vsnprintf(buffer, 2*BIG_BUFFER_SIZE, fmt, ap);
		va_end(ap);
		return write(mp3_stdin, buffer, strlen(buffer));
	}
	return 0;
}


/*
 * start_mp3 : This should be called by the main app thread,
 * whenever it wants to start up the mp3 stuff
 */
void start_mp3(void)
{
struct stat st;

	/* Init our queues. */
	Q_OPEN(&PendingQueueHead, &PendingQueueTail);
	Mp3QueueHead = NULL;
	/* Create our mutexes */
	if (stat(MP3_PLAYER, &st))
	{
		say("%s was not detected on your system. Please install", MP3_PLAYER);
		return;
	}
	init_mp3_mutexes();
	/* init our pending queue condition, and set to default attributes (NULL) */
	pthread_cond_init(&pending_queue_cond, NULL);
	/* lock up the quit mutex */
	pthread_mutex_lock(&quit_mutex);
	/* create our mp3 thread */
	pthread_create(&mp3_thread, NULL, start_mp3_thread, NULL);
}

/* 
 * stop_mp3 : This should be called by the main app thread,
 * whenever it wants to stop the mp3 stuff.
 */
void stop_mp3(void)
{
	void *ptr = NULL;

	if (!mp3_thread)
		return;
	/* Unlock the quit mutex */
	pthread_mutex_unlock(&quit_mutex);
	/* lock up pending queue mutex */
	pthread_mutex_lock(&mp3_queue_mutex);
	/* signal thread to wake up */
	pthread_cond_signal(&pending_queue_cond);
	/* Give lock back, so mp3 thread can react to signal */
	pthread_mutex_unlock(&mp3_queue_mutex);
	/* Wait until the thread kills itself. */
	pthread_join(mp3_thread, &ptr);
	cleanup_mp3();
}

/* 
 * kill_mp3 : This should be called by the main app thread,
 * when it wants the mp3 thread to go away forever. This function
 * kills the thread uncleanly, and probably should only be used
 * when the main app is about to exit()
 */
void kill_mp3(void)
{
	sigset_t set, oldset;

	/* Fill set with all known signals */
	sigfillset(&set);
	/* Remove the few signals that POSIX says we should */
	sigdelset(&set, SIGFPE);
	sigdelset(&set, SIGILL);
	sigdelset(&set, SIGSEGV);
	/* Tell our thread (main) to block against the above signals */
	sigprocmask(SIG_BLOCK, &set, &oldset);
	/* lock up pending queue mutex */
	pthread_mutex_unlock(&quit_mutex);
	pthread_mutex_lock(&mp3_queue_mutex);
	/* signal thread to wake up */
	pthread_cond_signal(&pending_queue_cond);
	/* Kill the mp3 thread, using the SIGQUIT signal */
	pthread_kill(mp3_thread, SIGQUIT);
	/* Put back the previous blocking signals */
	sigprocmask(SIG_BLOCK, &oldset, &set);
	/* cleanup anything dealing with the mp3 stuff */
	cleanup_mp3();
}


static void kill_mp3_thread2(int notused)
{
	int ecode = 0;
	pthread_exit(&ecode);
}


void init_mp3_play(void)
{
	int	p0[2], p1[2], p2[2],
		pid, cnt;
	char	buffer[90];
	char	*name = &buffer[0];

	sprintf(buffer, "%s -R", MP3_PLAYER);
	p0[0] = p1[0] = p2[0] = -1;
	p0[1] = p1[1] = p2[1] = -1;

	/*
	 * Open up the communication pipes
	 */
	if (pipe(p0) || pipe(p1) || pipe(p2))
	{
		new_close(p0[0]);
		new_close(p0[1]);
		new_close(p1[0]);
		new_close(p1[1]);
		new_close(p2[0]);
		new_close(p2[1]);
		return;
	}

	switch ((pid = fork()))
	{
		case -1:
			say("Couldn't start new process!");
			break;
		/*
		 * CHILD: set up and exec the process
		 */
		case 0:
		{
			int i;

			/*
			 * Fire up a new job control session,
			 */
			setsid();
			setuid(getuid());
			setgid(getgid());
			my_signal(SIGINT, SIG_IGN, 0);
			my_signal(SIGQUIT, SIG_DFL, 0);
			my_signal(SIGSEGV, SIG_DFL, 0);
			my_signal(SIGBUS, SIG_DFL, 0);
			my_signal(SIGCHLD, SIG_DFL, 0);
			dup2(p0[0], 0);
			dup2(p1[1], 1);
			dup2(p2[1], 2);
			close(p0[1]);
			close(p1[0]);
			close(p2[0]);
			p0[1] = p1[0] = p2[0] = -1;
			for (i = 3; i < 256; i++)
				close(i);

			{
				char	**args, *arg1;
				int	max;

				cnt = 0;
				max = 5;
				args = new_malloc(sizeof(char *) * max);
				while ((arg1 = new_next_arg(name, &name)))
				{
					if (!arg1 || !*arg1)
						break;
					if (cnt == max)
					{
						max += 5;
						RESIZE(args, char *, max);
					}

					args[cnt++] = arg1;
				}
				args[cnt] = NULL;
				execvp(args[0], args);
				stop_mp3();
				return;
			}
		}
		default:
		{
			if(pid == -1)
			{
				say("Couldn't start new process!");
				return;
			}
			else
			{
				new_close(p0[0]);
				new_close(p1[1]);
				new_close(p2[1]);
			}
			mp3_pid = pid;
			mp3_stdin = p0[1];
			mp3_stdout = p1[0];
			mp3_stderr = p2[0];
			new_open(mp3_stdout);
			new_open(mp3_stderr);
		}
	}
}

void *start_mp3_thread(void *arg)
{
	MP3_QUEUE *mp3 = NULL;
	init_mp3_play();
	/* loop */

	while(1) 
	{
		/* Try the quit mutex, if we get it, that means
		 * main() wants us to die.
		 */
		if (pthread_mutex_trylock(&quit_mutex) == 0) 
			kill_mp3_thread2(0);

		pthread_mutex_lock(&mp3_queue_mutex);
		if (!mp3) {
			/*
			 * Give back the cpu, and go to sleep until cond gets signalled.
			 * Note: the mutex MUST be locked, before going into the wait!
			 */
			pthread_cond_wait(&pending_queue_cond, &mp3_queue_mutex);
			/* We have been woken up. Thus, 2 conditions are present.
			 * 1) We have been signalled that data is waiting.
			 * 2) The mutex is locked.
			 */
			pthread_mutex_unlock(&mp3_queue_mutex);
			/* Lock the queue */
			pthread_mutex_lock(&pending_queue_mutex);
			mp3 = mp3_dequeue(&PendingQueueHead, &PendingQueueTail);
			pthread_mutex_unlock(&pending_queue_mutex);
		}
		else 
		{

			pthread_mutex_unlock(&mp3_queue_mutex);
			free_mp3_entry(Mp3QueueHead);
			Mp3QueueHead = mp3;
			/* send a command to our mp3 player */
			send_mp3_command("LOAD %s\n", mp3->filename);
			mp3 = NULL;
		}
	}
}

/* init our mp3 mutexes */
static void init_mp3_mutexes(void)
{
	pthread_mutex_init(&pending_queue_mutex, NULL);
	pthread_mutex_init(&quit_mutex, NULL);
}

/* cleanup_mp3 : cleanup anything regarding the mp3 thread */
static void cleanup_mp3(void)
{
	send_mp3_command("QUIT\n");

	close(mp3_stdin);
	mp3_stdin = -1; 
	mp3_stdout = new_close(mp3_stdout);
	mp3_stderr = new_close(mp3_stderr);

	pthread_mutex_destroy(&pending_queue_mutex);
	pthread_mutex_destroy(&quit_mutex);
	pthread_mutex_destroy(&mp3_queue_mutex);
	pthread_cond_destroy(&pending_queue_cond);
	destroy_mp3_queue(&PendingQueueHead, &PendingQueueTail);
	free_mp3_entry(Mp3QueueHead);
	Mp3QueueHead = NULL;
	if (mp3_pid != -1)
	{
		mp3_pid = get_child_exit(mp3_pid);
		mp3_pid = -1;
	}
}

/* Give back memory that we allocated for this entry */
static void free_mp3_entry(MP3_QUEUE *tmp)
{
	if (!tmp)
		return;
	new_free(&tmp->filename);
	new_free(&tmp->stats.type);
	new_free(&tmp->stats.title);
	new_free(&tmp->stats.album);
	new_free(&tmp->stats.artist);
	new_free(&tmp->stats.comment);
	new_free(&tmp->stats.mode);
	new_free(&tmp);
}

MP3_QUEUE *mp3_dequeue(MP3_QUEUE **headp, MP3_QUEUE **tailp)
{
	MP3_QUEUE *tmp = NULL;

	if (*headp == NULL)
		return NULL;
	tmp = *headp;
	*headp = Q_NEXT(tmp);
	if (*headp == NULL)
		*tailp = NULL;
	Q_NEXT(tmp) = NULL;
	return tmp;
}

/* enqueue a request onto the passed in queue */
MP3_QUEUE *mp3_enqueue(MP3_QUEUE **headp, MP3_QUEUE **tailp, MP3_QUEUE *tmp)
{
	Q_NEXT(tmp) = NULL;
	if (*headp == NULL)
		*headp = *tailp = tmp;
	else 
	{
		Q_NEXT(*tailp) = tmp;
		*tailp = tmp;
	}
	return NULL;
}

#if 0
/* enqueue a request onto the passed in queue, putting it at the front
 * of the queue. This means it will be the next requested dequeue.
 */
MP3_QUEUE *mp3_enqueue_urgent(MP3_QUEUE **headp, MP3_QUEUE **tailp, MP3_QUEUE *tmp)
{
	Q_NEXT(tmp) = *headp;
	*headp = tmp;
	if (*tailp == NULL)
		*tailp = tmp;
	return NULL;
}
#endif

/* destroy_mp3_queue : Walks the queue, blowing away each node */
static void destroy_mp3_queue(MP3_QUEUE **QueueHead, MP3_QUEUE **QueueTail)
{
	MP3_QUEUE *mp3;
	
	while((mp3 = mp3_dequeue(QueueHead, QueueTail)) != NULL)
		free_mp3_entry(mp3);
}

void add_to_mp3_queue(char *filename)
{
	MP3_QUEUE *tmp;

	if (mp3_pid == -1)
		start_mp3();
	if (!mp3_thread)
		return;
	tmp = (MP3_QUEUE *)new_malloc(sizeof(MP3_QUEUE));
	malloc_strcpy(&tmp->filename, filename);
	
	/* Wait until we can get mutex lock for queue */
	pthread_mutex_lock(&pending_queue_mutex);

	mp3_enqueue(&PendingQueueHead, &PendingQueueTail, tmp);
	/* signal mp3 thread, its got work to do */

	if (pthread_mutex_trylock(&mp3_queue_mutex) == 0) 
	{
		if (!Mp3QueueHead)
			pthread_cond_signal(&pending_queue_cond);
		pthread_mutex_unlock(&mp3_queue_mutex);
	}
	/* Give lock back, so mp3 thread can react to signal */
	pthread_mutex_unlock(&pending_queue_mutex);
}

void load_mp3s(FILE *f)
{
char *f1, *p;
	while (!feof(f))
	{
		char buf[BIG_BUFFER_SIZE+1];
		memset(buf, 0, sizeof(buf));
		if (!fgets(buf,BIG_BUFFER_SIZE,f)) 
			break;
		chomp(buf);
		p = buf;
		if (!*p)
			continue;
		f1 = new_next_arg(p, &p);
		if (!f1 || !*f1)
			continue;
		add_to_mp3_queue(f1);
	}
}

void do_mp3_list(void)
{
int count = 1;
MP3_QUEUE *mp3;
	for (mp3 = PendingQueueHead; mp3; mp3 = mp3->next, count++)
		if (do_hook(MP3_LIST, "@Q %d %s", count, mp3->filename))
			put_it("%3d - %s", count, base_name(mp3->filename));
}

BUILT_IN_COMMAND(mp3play)
{
char *n;
int done_one = 0;

	while ((n = new_next_arg(args, &args)))
	{
		if (!n || !*n)
			break;
		if (!my_stricmp(n, "LOAD"))
		{
			char *fname;
			FILE *f;
			if (!(fname = new_next_arg(args, &args))) 
				break;
			if (!(f = fopen(fname, "r"))) 
				break;
			load_mp3s(f);
			fclose(f);
		} else if (!my_stricmp(n, "STOP"))
			send_mp3_command("STOP\n");
		else if (!my_stricmp(n, "CURRENT"))
		{
			pthread_mutex_lock(&mp3_queue_mutex);
			if (Mp3QueueHead)
			{
				if (do_hook(MP3_LIST, "@I ID3: \"%s\" \"%s\" \"%s\" %d \"%s\" \"%s\" %s %d %d %s %d %d %d %d %d %d %d %d %d",
					Mp3QueueHead->stats.title, Mp3QueueHead->stats.artist,
					Mp3QueueHead->stats.album, Mp3QueueHead->stats.year,
					Mp3QueueHead->stats.comment, Mp3QueueHead->stats.genre,

					Mp3QueueHead->stats.type, Mp3QueueHead->stats.layer,
					Mp3QueueHead->stats.freq, Mp3QueueHead->stats.mode,
					Mp3QueueHead->stats.mode_ext, Mp3QueueHead->stats.framesize,
					Mp3QueueHead->stats.ster, Mp3QueueHead->stats.copyright,
					Mp3QueueHead->stats.copyright, Mp3QueueHead->stats.crc,
					Mp3QueueHead->stats.emp, Mp3QueueHead->stats.bitrate,
					Mp3QueueHead->stats.ext))

				{
				put_it("Title  : %-30s  Artist  : %-30s", Mp3QueueHead->stats.title, Mp3QueueHead->stats.artist);
				put_it("Album  : %-30s  Year    : %d", Mp3QueueHead->stats.album, Mp3QueueHead->stats.year);
				put_it("Comment: %-30s  Genre   : %s", Mp3QueueHead->stats.comment, Mp3QueueHead->stats.genre);
				}
			}
			pthread_mutex_unlock(&mp3_queue_mutex);
		}
		else if (!my_stricmp(n, "START"))
			start_mp3();
		else if (!my_stricmp(n, "RESTART"))
		{
			stop_mp3();
			start_mp3();
		}
		else if (!my_stricmp(n, "QUIT"))
			stop_mp3();
		else if (!my_stricmp(n, "PAUSE"))
			send_mp3_command("PAUSE\n"); 
		else if (!my_stricmp(n, "LIST"))
		{
			do_mp3_list();
			return;
		}
		else if (!my_stricmp(n, "JUMP"))
		{
			char *pos;
			pos = next_arg(args, &args);
			send_mp3_command("JUMP %s\n", pos);
		}	
		else if (!my_stricmp(n, "EQUALIZE"))
		{
			send_mp3_command("EQUALIZE %s\n", args);
			return;
		}
		else
			add_to_mp3_queue(n);
		done_one++;
	}
	if (!done_one)
		do_mp3_list();
}

void set_mp3_output_fd(fd_set *rd)
{
	if (mp3_stdout >= 0)
		FD_SET(mp3_stdout, rd);
	if (mp3_stderr >= 0)
		FD_SET(mp3_stderr, rd);
}

void setup_id3(char *blah)
{
	char *tmp;
	struct mp3id3_ {
		char title[35];
		char artist[35];
		char album[35];
		char year[5];
		char comment[35];
		char genre[50];
	} mp3id3;

	memset(&mp3id3, 0, sizeof(struct mp3id3_));

	tmp = LOCAL_COPY(blah);
	chomp(tmp);
	strncpy(mp3id3.title, "Unknown", 30);
	strncpy(mp3id3.artist, "Unknown", 30);
	strncpy(mp3id3.album, "Unknown", 30);
	strncpy(mp3id3.year, "Unknown", 30);
	strncpy(mp3id3.comment, "Unknown", 30);
	strncpy(mp3id3.genre, "Unknown", 30);
	if (!strncmp(tmp, "@I ID3:", 6))
	{
		tmp+=7;
		if (*tmp)
		{
			strncpy(mp3id3.title, tmp, 30);
			remove_trailing_spaces(mp3id3.title);
			strncpy(mp3id3.artist, tmp+30, 30);
			remove_trailing_spaces(mp3id3.artist);
			strncpy(mp3id3.album, tmp+30+30, 30);
			remove_trailing_spaces(mp3id3.album);
			strncpy(mp3id3.year, tmp+30+30+30, 4);		
			strncpy(mp3id3.comment, tmp+30+30+30+4, 30);
			remove_trailing_spaces(mp3id3.comment);
			strncpy(mp3id3.genre, tmp+30+30+30+4+30, 50);
		}
	}
	else
	{
		char *p, *q;
		if ((p = strchr(tmp, '-')))
		{
			tmp += 3;
			*p++ = 0;
			strncpy(mp3id3.artist, tmp, 30);
			remove_trailing_spaces(mp3id3.artist);
			if ((q = strchr(p, '.')))
				*q = 0;
			while (*p == ' ')
				p++;
			strncpy(mp3id3.title, p, 30);

		}
	}
	pthread_mutex_lock(&mp3_queue_mutex);

	Mp3QueueHead->stats.title = m_strdup(mp3id3.title);
	Mp3QueueHead->stats.artist = m_strdup(mp3id3.artist);
	Mp3QueueHead->stats.album = m_strdup(mp3id3.album);
	Mp3QueueHead->stats.comment = m_strdup(mp3id3.comment);
	Mp3QueueHead->stats.genre = m_strdup(mp3id3.genre);
	Mp3QueueHead->stats.year = atol(mp3id3.year);
	pthread_mutex_unlock(&mp3_queue_mutex);
}

void setup_stat(char *blah)
{
char *tmp, *p;
	tmp = LOCAL_COPY(blah);
	chomp(tmp);
	p = next_arg(tmp, &tmp);
	p = next_arg(tmp, &tmp);
	/*@S 1.0 3 44100 Joint-Stereo 0 626 2 0 0 0 192 0*/
	pthread_mutex_lock(&mp3_queue_mutex);
	Mp3QueueHead->stats.type = m_strdup(p);
	p = next_arg(tmp, &tmp);
	Mp3QueueHead->stats.layer = my_atol(p);
	p = next_arg(tmp, &tmp);
	Mp3QueueHead->stats.freq = my_atol(p);
	p = next_arg(tmp, &tmp);
	Mp3QueueHead->stats.mode = m_strdup(p);
	p = next_arg(tmp, &tmp);
	Mp3QueueHead->stats.mode_ext = my_atol(p);
	p = next_arg(tmp, &tmp);
	Mp3QueueHead->stats.framesize = my_atol(p);
	p = next_arg(tmp, &tmp);
	Mp3QueueHead->stats.ster = my_atol(p);
	p = next_arg(tmp, &tmp);
	Mp3QueueHead->stats.copyright = my_atol(p);
	p = next_arg(tmp, &tmp);
	Mp3QueueHead->stats.crc = my_atol(p);
	p = next_arg(tmp, &tmp);
	Mp3QueueHead->stats.emp = my_atol(p);
	p = next_arg(tmp, &tmp);
	Mp3QueueHead->stats.bitrate = my_atol(p);
	p = next_arg(tmp, &tmp);
	Mp3QueueHead->stats.ext = my_atol(p);
	do_hook(MP3_LIST, "@I ID3: \"%s\" \"%s\" \"%s\" %d \"%s\" \"%s\" %s %d %d %s %d %d %d %d %d %d %d %d %d",
		Mp3QueueHead->stats.title, Mp3QueueHead->stats.artist,
		Mp3QueueHead->stats.album, Mp3QueueHead->stats.year,
		Mp3QueueHead->stats.comment, Mp3QueueHead->stats.genre,

		Mp3QueueHead->stats.type, Mp3QueueHead->stats.layer,
		Mp3QueueHead->stats.freq, Mp3QueueHead->stats.mode,
		Mp3QueueHead->stats.mode_ext, Mp3QueueHead->stats.framesize,
		Mp3QueueHead->stats.ster, Mp3QueueHead->stats.copyright,
		Mp3QueueHead->stats.copyright, Mp3QueueHead->stats.crc,
		Mp3QueueHead->stats.emp, Mp3QueueHead->stats.bitrate,
		Mp3QueueHead->stats.ext);

	pthread_mutex_unlock(&mp3_queue_mutex);
	
}

void mp3_check(fd_set *rd)
{
	char blah[BIG_BUFFER_SIZE+1];
	char *control;
	if (mp3_stdout >= 0 && FD_ISSET(mp3_stdout, rd)) 
	{
		switch (new_dgets(blah, mp3_stdout, 1, BIG_BUFFER_SIZE))
		{
			case -1:
				stop_mp3();
				return;
			case 0:
				break;
			default:
				control = &blah[0];
				control++;
				switch(*control)
				{
					case 'P':
					{
						char *tmp;
						tmp = LOCAL_COPY(control);
						next_arg(tmp, &tmp);
						if (tmp && *tmp == '0')
						{
							pthread_mutex_lock(&mp3_queue_mutex);
							pthread_cond_signal(&pending_queue_cond);
							free_mp3_entry(Mp3QueueHead);
							Mp3QueueHead = NULL;
							pthread_mutex_unlock(&mp3_queue_mutex);
						}
						break;
					}
					case 'S':
						setup_stat(blah);
						break;
					case 'E':
						do_hook(MP3_LIST, "%s", blah);
						break;
					case 'I':
						setup_id3(blah);
						break;
					case 'R':
						if (do_hook(MP3_LIST, "%s", blah))
						{
							next_arg(control, &control);
							next_arg(control, &control);
							say("Mp3 Player started %s", control);
						}
					default:
						break;			
				}
				break;
		}
	}
	if (mp3_stderr >= 0 && FD_ISSET(mp3_stderr, rd))
		read(mp3_stderr, blah, BIG_BUFFER_SIZE);
}

#elif defined(WINNT)

#include "winamp.h"

HWND hwnd_winamp = NULL;
unsigned int winamp_version = 0;

int init_winamp(void)
{
	if ((hwnd_winamp = FindWindow("Winamp v1.x",NULL)))
	{
		winamp_version = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETVERSION);
		return 0;
	}
	else
		hwnd_winamp = NULL;
	return -1; 
}

int is_playing(void)
{
	/*
	 * 1 if playing
	 * 3 if paused
	 * 0 if not playing
	 */
	if (!init_winamp())
		return(SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_ISPLAYING));
	return -1;
}

int set_volume(char *args)
{
int volume = 0;
	volume = my_atol(args);
	if (!init_winamp() && winamp_version > 0x2000)
	{
		if (volume > 255)
			volume = 255;
		SendMessage(hwnd_winamp,WM_WA_IPC,volume,IPC_SETVOLUME);
	}
	return 0;
}

char *current_fileplaying(void)
{
static char this_title[2048],*p;
	this_title[0] = 0;
	if (!init_winamp())
	{
		GetWindowText(hwnd_winamp,this_title,sizeof(this_title));
		p = this_title+strlen(this_title) - 8;
		while (p >= this_title)
		{
			if (!strnicmp(p, "- Winamp", 8)) break;
				p--;
		}
		if (p >= this_title) 
			p--;
		while (p >= this_title && *p == ' ') 
			p--;
		*++p=0;
	}
	return this_title;
}

int get_playlistlength(void)
{
int length = -1;
	if (!init_winamp() && winamp_version > 0x2000)
		length = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETLISTLENGTH);
	return length;
}

int get_playlistpos(void)
{
int pos = -1;
	if (!init_winamp() && winamp_version > 0x2000)
		pos = SendMessage(hwnd_winamp,WM_WA_IPC,0,IPC_GETLISTPOS);
	return pos;
}

int set_playlistpos(char *args)
{
int position = 0;
	position = my_atol(args);
	if (!init_winamp() && winamp_version > 0x2000)
	{
		int length;
		length = get_playlistlength();
		if (position > length || !position)
			position = 1;
		SendMessage(hwnd_winamp,WM_WA_IPC,position,IPC_SETPLAYLISTPOS);
	}
	return 0;
}

int next_in_playlist(void)
{
int pos = -1;
	if (!init_winamp())
	{
		int length;
		length = get_playlistlength();
		pos = get_playlistpos();
		if (pos == length)
			pos = 1;
		else
			pos++;
		SendMessage(hwnd_winamp, WM_WA_IPC, pos, IPC_SETPLAYLISTPOS);
/*		SendMessage(hwnd_winamp,WM_WA_IPC,(WPARAM)pos,IPC_CHANGECURRENTFILE);*/
	}
	return pos;
}

int prev_in_playlist(void)
{
int pos = -1;
	if (!init_winamp())
	{
		int length;
		length = get_playlistlength();
		pos = get_playlistpos() - 1;
		if (!pos)
			pos = length;
		SendMessage(hwnd_winamp, WM_WA_IPC, pos, IPC_SETPLAYLISTPOS);
	}
	return pos;
}

int playfile(char *args)
{
COPYDATASTRUCT cds;
	cds.dwData = IPC_PLAYFILE;
	cds.lpData = (void *) args;
	cds.cbData = strlen((char *) cds.lpData)+1; /* include space for null char */
	if (!init_winamp())
		SendMessage(hwnd_winamp,WM_COPYDATA,(WPARAM)NULL,(LPARAM)&cds);
	return 0;
}

int play_winamp(void)
{
	if (is_playing() == 1)
	{
		say("Playlist is already playing");
		return 0;
	}
	if (!init_winamp())
	{
		SendMessage(hwnd_winamp, WM_WA_IPC, 0, IPC_STARTPLAY);
		return 0;
	}
	else
		say("Winamp is not running or an error occured");
	return -1;
}

int send_winamp_command(unsigned int command)
{
	if (!init_winamp())
		SendMessage(hwnd_winamp, WM_COMMAND, 0, command);
	return 0;
}

BUILT_IN_COMMAND(playwinamp)
{
char *arg;
	while (args && *args)
	{
		arg = new_next_arg(args, &args);
		if (arg && *arg)
		{
			if (arg && *arg == '-')
			{
				arg++;
				if (!my_stricmp(arg, "NEXT"))
					next_in_playlist();
				else if (!my_stricmp(arg, "PREV"))
					prev_in_playlist();
				else if (!my_stricmp(arg, "VOLUME"))
				{
					char *vol;
					vol = next_arg(args, &args);
					set_volume(vol);
				}
				else if (!my_stricmp(arg, "POSITION"))
				{
					char *pos;
					pos = next_arg(args, &args);
					set_playlistpos(pos);
				}
				else if (!my_stricmp(arg, "PAUSE"))
					send_winamp_command(WINAMP_PAUSE_BUTTON);
				else if (!my_stricmp(arg, "STOP"))
					send_winamp_command(WINAMP_STOP_BUTTON);
				else if (!my_stricmp(arg, "FADEOUT"))
					send_winamp_command(WINAMP_FADE_STOP_BUTTON);
				else if (!my_stricmp(arg, "PLAY"))
				{
					char *file;
					file = new_next_arg(args, &args);
					playfile(file);
				}
			} 
			else
				playfile(arg);
		}
	}
	play_winamp();
}

#endif

/*
 * TekNap: a new napster client.
 * Copyright Colten Edwards Feb 14/2000
 *
 * $Id: main.c,v 1.1.1.1 2001/02/25 05:24:51 edwards Exp $
 */

#include "teknap.h"
#include "struct.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <assert.h>
#ifdef USING_CURSES
#include <curses.h>
#endif
#include <stdarg.h>

#include "alias.h"
#include "commands.h"
#include "exec.h"
#include "hook.h"
#include "server.h"
#include "status.h"
#include "vars.h"
#include "input.h"
#include "output.h"
#include "ircterm.h"
#include "screen.h"
#include "keys.h"
#include "ircaux.h"
#include "window.h"
#include "history.h"
#include "newio.h"
#include "timer.h"
#include "napster.h"
#include "cdns.h"
#include "ptest.h"
#include "tgtk.h"
#ifdef WANT_MP3PLAYER
#include "mp3control.h"
#endif

#include <pwd.h>

#ifdef __EMX__
#include <signame.h>
#endif

#ifndef VERSION
	const char nap_version[] = "TekNap-1.3g";
#else
	const char nap_version[] = VERSION;
#endif

static char main_version[] = "$Id: main.c,v 1.1.1.1 2001/02/25 05:24:51 edwards Exp $";

/*
 * major version is 75
 * minor version is 14
 * 000 is non-alpha/non-patch
 * 001 is alpha 
 * 002 is patch
 */

/*
 * INTERNAL_VERSION is the number that the special alias $V returns.
 * Make sure you are prepared for floods, pestilence, hordes of locusts( 
 * and all sorts of HELL to break loose if you change this number.
 * Its format is actually YYYYMMDD, for the _release_ date of the
 * client..
 */
const char internal_version[] = "20000909";

int	nap_port = 8875,			/* port of ircd */
	use_input = 1,				/* if 0, stdin is never
						 * checked */
	dead	   = 0, 
	inhibit_logging = 0,
	auto_connect = 1,			/* auto-connect to first server*/
	from_server = -1,
	foreground = 1,
	starting_server = -1,			/* what server to start at */
	term_initialized = 0;
	
unsigned long x_debug;

char	zero[]	= "0",
	one[]	= "1",
	space[]	= " ",
	space_plus[] = " +",
	space_minus[] = " -",
	dot[]   = ".",
	star[]	= "*",
	comma[] = ",",
	empty_string[] = "",
	on[]	= "ON",
	off[]	= "OFF";
		 
	struct	in_addr	MyHostAddr;		/* The local machine address */
	struct	sockaddr_in LocalHostAddr;

char	*LocalHostName = NULL;

extern	Server *server_list;
char	
	*my_path = NULL,		/* path to users home dir */
	*irc_path = NULL,		/* paths used by /load */
	*irc_lib = NULL,		/* path to the ircII library */
	*bircrc_file = NULL,
	*ircservers_file = NULL,	/* name  of server file */
	nickname[NICKNAME_LEN + 1],	/* users nickname */
	password[NICKNAME_LEN + 1],	/* users nickname */
	shared_file_dir[2048+1],
	*args_str = NULL,		/* list of command line args */
	*cut_buffer = NULL;		/* global cut_buffer */

	int cpu_saver = 0;
	int timeout_select = 60;
	int scan_done = 0;		/* is our scan done */

int	logfile_line_mangler = 0;
int	do_get_napigator = 0;

time_t	idle_time = 0,
	now = 0,
	start_time;


GetFile *transfer_struct = NULL;
GetFile *finished_struct = NULL;
FileStruct * fserv_files = NULL;


fd_set	readables, writables;

static	void	quit_response (char *, char *);
static	char	*parse_args (char **, int, char **);

static	int create = 0;
	
static	volatile int	cntl_c_hit = 0;
int	set_capability	= 0;

	char	version[] = _VERSION_;
	
static		char	switch_help[] =
"Usage: TekNap [switches] [nickname] [server list] \n\
  The [nickname] can be at most 15 characters long\n\
  The [server list] is a whitespace separate list of server name\n\
  The [switches] may be any or all of the following\n\
   -C\t\tcreate the account\n\
   -N\t\tdo not auto-connect to the first server\n\
   -A\t\tconnect to napigator.com and download server list\n\
   -S #\t\tstarting server to use\n\
   -H ip\tspecify an ip to use for the connection\n\
   -n nickname\tnickname to use\n\
   -p\t\trequest password to use\n\
   -a\t\tadds default servers and command line servers to server list\n\
   -r filename\tuse filename for the serverlist\n\
   -c\t\tset napster beta8 capability\n\
   -v\t\ttells you about the client's version\n";

#if defined(WINNT) || defined(EMX)
static	char	switch_help_w[] = 
"   -s\t\tsetup TekNap for use, by prompting\n";
#endif

char	*time_format = NULL;	/* XXX Bogus XXX */
char	*strftime_24hour = "%R";
char	*strftime_12hour = "%I:%M%p";
char	time_str[61];

volatile int update_refresh = 0;


Stats	shared_stats = { 0 };

void check_serverlag(int server)
{
	int i;
	struct timeval tv;
	int ofs = from_server;
	get_time(&tv);
	if (server == -1)
	{
		for (i = 0; i < server_list_size(); i++)
		{
			if (is_connected(i) && !get_server_sping(i))
			{
				from_server = i;
				if (!get_server_ircmode(i) && (get_server_admin(i) > USER_USER))
				{
					send_ncommand(CMDS_SPING, "%s LAG %lu %lu", get_server_itsname(i) ? get_server_itsname(i):get_server_name(i), tv.tv_sec, tv.tv_usec);
					set_server_sping(i, 1);
				}
			}
		}
		from_server = ofs;
		return;
	}
	if (is_connected(server) && !get_server_sping(server) && (get_server_admin(server) > USER_USER))
	{
		if (!get_server_ircmode(server))
		{
			send_ncommand(CMDS_SPING, "%s LAG %lu %lu", get_server_itsname(server) ? get_server_itsname(server) : get_server_name(server), tv.tv_sec, tv.tv_usec);
			set_server_sping(server, 1);
		}
	}
}

/* update_clock: figUres out the current time and returns it in a nice format */
char	*update_clock(int flag)
{
	static	int		min = -1,
				hour = -1;
	static	time_t		last_minute = -1;
	time_t			idlet;
	static struct tm	time_val;
		time_t		hideous;
		int		new_minute = 0;
		int		new_hour = 0;

	hideous = now;

#if !defined(NO_CHEATING)
	if (hideous / 60 > last_minute)
	{
		last_minute = hideous / 60;
		time_val = *localtime(&hideous);
	}
#else
	time_val = *localtime(&hideous);
#endif


	if (flag == RESET_TIME || time_val.tm_min != min || time_val.tm_hour != hour)
	{
		if (time_format)	/* XXXX Bogus XXXX */
			strftime(time_str, 60, time_format, &time_val);
		else if (get_int_var(CLOCK_24HOUR_VAR))
			strftime(time_str, 60, strftime_24hour, &time_val);
		else
			strftime(time_str, 60, strftime_12hour, &time_val);

		lower(time_str);
		if ((time_val.tm_min != min) || (time_val.tm_hour != hour))
		{
			if (time_val.tm_hour != hour)
				new_hour = 1;
			new_minute = 1;
			hour = time_val.tm_hour;
			min = time_val.tm_min;
			do_hook(TIMER_LIST, "%02d:%02d", hour, min);
			if (min == 0 || new_hour)
				do_hook(TIMER_HOUR_LIST, "%02d:%02d", hour, min);
			idlet = (hideous - idle_time) / 60L;
			do_hook(IDLE_LIST, "%lu", (unsigned long)idlet);
		}
		if (!((hideous - start_time) % 20))
			check_serverlag(-1);
		if (flag != RESET_TIME || new_minute)
			return time_str;
		else
			return NULL;
	}
	if (flag == GET_TIME)
		return time_str;
	else
		return NULL;
}

void reset_clock(Window *win, char *unused, int unused1)
{
	update_clock(RESET_TIME);
	update_all_status(win, NULL, 0);
}



/* sig_refresh_screen: the signal-callable version of refresh_screen */
SIGNAL_HANDLER(sig_refresh_screen)
{
	/* We used to call refresh_screen() here directly, but we have found
	 * that some libc implementations cannot deal with us refreshing
	 * the window while in a signal handler, while using threads.
	 * So now we toggle update_refresh variable, to tell our main io
	 * loop to do the refresh. This resolves the SIGWINCH - thread
	 * problem. -SHK
	 */
	update_refresh = 1;
}

SIGNAL_HANDLER(irc_exit_old)
{
	nap_exit(1,"Signal request exit", NULL);
}

volatile int dead_children_processes;

/* This is needed so that the fork()s we do to read compressed files dont
 * sit out there as zombies and chew up our fd's while we read more.
 */
SIGNAL_HANDLER(child_reap)
{
	dead_children_processes++;
}

SIGNAL_HANDLER(nothing)
{
      /* nothing to do! */
}
                           
SIGNAL_HANDLER(sigpipe)
{
static int sigpipe_hit = 0;
	sigpipe_hit++;
	
}

SIGNAL_HANDLER(sigusr1)
{
	do_hook(SIGUSR1_LIST, "%s", "Sig user1 signal");
}

/* Call this, when you are about to quit the client. It will kill all known threads */
static void kill_all_threads(void)
{
#ifdef WANT_THREAD
#ifdef WANT_NSLOOKUP
	kill_dns();
#endif
#ifdef WANT_PTEST
	kill_ptest();
#endif
#ifdef GTK
	kill_tgtk();
#endif
#ifdef WANT_MP3PLAYER
	kill_mp3();
#endif
#if !defined(WIN32_GUI)
	clean_share_mutexes();
#endif
#endif
}

          
/* nap_exit: cleans up and leaves */
void nap_exit (int really_quit, char *reason, char *format, ...)
{
	if (dead == 1) {
		kill_all_threads();
		exit(1);
	}
	else if (dead == 2) {
		kill_all_threads();
		_exit(1);
	}
	else if (dead == 3) {
		kill_all_threads();
		kill(getpid(), SIGKILL);
	}
	dead++;

	set_lastlog_size(NULL, NULL, 0);
	set_history_size(NULL, NULL, 0);

	if (really_quit)
	{
		kill_all_threads();
		say("Signon time  :    %s", my_ctime(start_time));
		say("Signoff time :    %s", my_ctime(now));
		say("Total uptime :    %s", convert_time(now - start_time));
	}
	do_hook(EXIT_LIST, "%s", reason ? reason : empty_string);
	if (reason)
		say("%s", reason);

	close_all_servers();
	close_all_sockets();
	if (term_initialized)
	{
		cursor_to_input();
		term_cr();
		term_clear_to_eol();
		term_reset();
	}

	remove_bindings();
	clear_variables();
	delete_all_windows();
	destroy_call_stack();		
	write_unfinished_list();
	
	debug_cleanup();
	fprintf(stdout, "\r");
	fflush(stdout);
	if (really_quit)
		exit(0);

	kill_all_threads();
	my_signal(SIGABRT, SIG_DFL, 0);
	kill(getpid(), SIGABRT);
	kill(getpid(), SIGQUIT);
	exit(1);
}

/*
 * quit_response: Used by irc_io when called from nap_quit to see if we got
 * the right response to our question.  If the response was affirmative, the
 * user gets booted from irc.  Otherwise, life goes on. 
 */
static	void quit_response(char *dummy, char *ptr)
{
	int	len;

	if ((len = strlen(ptr)) != 0)
		if (!my_strnicmp(ptr, "yes", len))
			nap_exit(1, "TekNap Rest in peace", NULL);
}

/* nap_quit: prompts the user if they wish to exit, then does the right thing */
void nap_quit(char key, char * ptr)
{
	static	int in_it = 0;

	if (in_it)
		return;
	in_it = 1;
	add_wait_prompt("Do you really want to quit? ", quit_response, empty_string, WAIT_PROMPT_LINE, 1);
	in_it = 0;
}

/*
 * cntl_c: emergency exit.... if somehow everything else freezes up, hitting
 * ^C five times should kill the program. 
 */
SIGNAL_HANDLER(cntl_c)
{

	if (cntl_c_hit++ >= 4)
		nap_exit(1, "User abort with 5 Ctrl-C's", NULL);
	else if (cntl_c_hit > 1)
		kill(getpid(), SIGALRM);
}


/*
 * parse_args: parse command line arguments for irc, and sets all initial
 * flags, etc. 
 *
 * major rewrite 12/22/94 -jfn
 *
 *
 * Im going to break backwards compatability here:  I think that im 
 * safer in doing this becuase there are a lot less shell script with
 * the command line flags then there are ircII scripts with old commands/
 * syntax that would be a nasty thing to break..
 *
 * Sanity check:
 *   Supported flags: -b, -l, -v, -c, -p, -f, -F, -L, -a, -S, -z
 *   New (changed) flags: -s, -I, -i, -n
 *
 * Rules:
 *   Each flag must be included by a hyphen:  -lb <filename> is not the
 * 		same as -l <filename> -b  any more...
 *   Each flag may or may not have a space between the flag and the argument.
 *   		-lfoo  is the same as -l foo
 *   Anything surrounded by quotation marks is honored as one word.
 *   The -c, -p, -L, -l, -s, -z flags all take arguments.  If no argumenTs
 *		are given between the flag and the next flag, an error
 * 		message is printed and the program is halted.
 *		Exception: the -s flag will be accepted without a argument.
 *		(ick: backwards compatability sucks. ;-)
 *   Arguments occuring after a flag that does not take an argument
 * 		will be parsed in the following way: the first instance
 *		will be an assumed nickname, and the second instance will
 *		will be an assumed server. (some semblance of back compat.)
 *   The -bl sequence will emit a depreciated feature warning.
 *   The -n flag means "nickname"
 *
 * Bugs:
 *   The -s flag is hard to use without an argument unless youre careful.
 */

static	char	*parse_args (char *argv[], int argc, char **envp)
{
	int ac;
	int add_servers = 0;
	
	char *channel = NULL;
	char *ptr;
	
	*nickname = 0;
	*password = 0;

	if ((ptr = getenv("NAPNICK")))
		strmcpy(nickname, ptr, NICKNAME_LEN);
	if ((ptr = getenv("NAPPASS")))
		strmcpy(password, ptr, NICKNAME_LEN);
	if ((ptr = getenv("NAP_HOST")) || (ptr = getenv("NAPHOST")))
		malloc_strcpy(&LocalHostName, ptr);	

	for ( ac = 1; ac < argc; ac++ )
	{
		if (argv[ac][0] == '-')
		{
		    switch (argv[ac][1]) {

			case 'r': /* Load list of servers from this file */
			{
				char *what = empty_string;

				if (argv[ac][2])
					what = &argv[ac][2];
				else if (argv[ac+1] && argv[ac+1][0] != '-')
				{
					what = argv[ac+1];
					ac++;
				}
				else
					fprintf(stderr, "Missing argumenT to -r\n");

				if (*what)
				{
					add_servers = 1;
					malloc_strcpy(&ircservers_file, what);
				}
				break;
			}
			case 'a': /* add server, not replace */
			{
				add_servers = 1;
				if (argv[ac][2])
					fprintf(stderr, "Ignoring junk after -a\n");
				break;
			}
			case 'H':
			{
				char *what = empty_string;
				
				if (argv[ac][2])
					what = &(argv[ac][2]);
				else if (argv[ac+1] && argv[ac+1][0] != '-')
				{
					what = argv[ac+1];
					ac++;
				}
				else
				{
					fprintf(stderr, "Specify a hostname\n");
					exit(1);
				}
				malloc_strcpy(&LocalHostName, what);
				break;
			}
			case 'S': 
			{
				if (argv[ac][2])
				{
					char *what;
					what = &argv[ac][2];
					starting_server = my_atol(what);
				}
				else
				{
					ac++;
					starting_server = my_atol(argv[ac]);
				}
				break;
			}
			case 'n':
			{
				char *what = empty_string;

				if (argv[ac][2])
					what = &(argv[ac][2]);
				else if (argv[ac+1] && argv[ac+1][0] != '-')
				{
					what = argv[ac+1];
					ac++;
				}
				else
				{
					fprintf(stderr,"Missing argument for -n\n");
					exit(1);
				}
				strmcpy(nickname, what, NICKNAME_LEN);
				break;
			}
			case 'p':
			{
				char *pass = NULL;
				if ((pass = getpass("Enter Password :")))
					strmcpy(password, pass, NICKNAME_LEN);
				break;
			}
			case 'N':
			{
				auto_connect = 0;
				break;
			}
			case 'A':
			{
				auto_connect = 0;
				do_get_napigator = 1;
				break;
			}
			case 'C':
				create = 1;
				break;
			case 'c':
				set_capability = 1;
				break;
			case '\0': 
				break;	/* ignore - alone */

			case 'v':
				fprintf(stderr, "%s %s\n", nap_version, internal_version); 
				exit(1);
#if defined(WINNT) || defined(EMX)
			case 's':
				setup_autoexec();
				exit(1);
#endif
			default:
				fprintf(stderr, "Unknown flag: %s\n",argv[ac]);
			case 'h':
				fprintf(stderr, "%s", switch_help);
#if defined(WINNT) || defined(EMX)
				fprintf(stderr, "%s", switch_help_w);
#endif
				exit(1);
		   } /* End of switch */
		}
		else
		{
			if (!strchr(argv[ac], '.'))
				strmcpy(nickname, argv[ac], NICKNAME_LEN);
			else
				build_server_list(argv[ac]);
		}
	}

	if ((ptr = getenv("NAPLIB")))
		irc_lib = m_opendup("/", ptr, "/", NULL);
	else
	{
		char *s;
		if ((s = expand_twiddle(NAPLIB)))
			irc_lib = s;
		else
			malloc_strcpy(&irc_lib, NAPLIB);
	}

	if ((ptr = getenv("NAPPATH")))
		malloc_strcpy(&irc_path, ptr);
	else
	{
#ifdef NAPPATH
		malloc_strcpy(&irc_path, NAPPATH);
#else
#ifdef WINNT
		malloc_strcpy(&irc_path, ".:~/TekNap:");
#else
		malloc_strcpy(&irc_path, ".:~/.TekNap:");
#endif
		irc_path = m_opendup(irc_lib, "/", "script", NULL);
#endif
	}

	if (LocalHostName)
	{
		struct hostent *hp;
		printf("Your hostname appears to be [%s]\n", LocalHostName);
		memset((void *)&LocalHostAddr, 0, sizeof(LocalHostAddr));
		if ((hp = gethostbyname(LocalHostName)))
			memcpy((void *)&LocalHostAddr.sin_addr, hp->h_addr, sizeof(struct in_addr));
 	} 
	
	if (!check_nickname(nickname))
	{
		fprintf(stderr, "\n Invalid Nickname\n");
		exit(1);
	}

	set_string_var(LOAD_PATH_VAR, irc_path);
	new_free(&irc_path);
	
	if ((ptr = getenv("HOME")))
		malloc_strcpy(&my_path, ptr);
	if (!my_path || !*my_path)
#ifdef WINNT
	{
		malloc_strcpy(&my_path, "//c/TekNap/");
		bsd_setenv("HOME", "//c/TekNap", 1);
	}
	if (access("//c/TekNap", F_OK) != 0)
	{
		fprintf(stderr, "Directory doesn't exist, creating //c/TekNap\n");
		mkdir("//c/TekNap", S_IWUSR|S_IRUSR|S_IXUSR);
	}		
#else
		malloc_strcpy(&my_path, "/");
#endif

#if defined(WINNT) || defined(__EMX__)
	convert_unix(my_path);
#endif
	if (!bircrc_file)
		malloc_sprintf(&bircrc_file, "%s%s", my_path, IRCRC_NAME);

	if ((ptr = getenv("NAPPORT")))
		nap_port = my_atol(ptr);


	if ((ptr = getenv("NAPSERVER")))
		build_server_list(ptr);
#ifdef DEFAULT_SERVER
	{
		if (!read_server_list())
		{
			ptr = LOCAL_COPY(DEFAULT_SERVER);
			build_server_list(ptr);
		}
	}	
#endif
	return (channel);
}



/* 
 * io() is a ONE TIME THROUGH loop!  It simply does ONE check on the
 * file descriptors, and if there is nothing waiting, it will time
 * out and drop out.  It does everything as far as checking for exec,
 * dcc, ttys, notify, the whole ball o wax, but it does NOT iterate!
 * 
 * You should usually NOT call io() unless you are specifically waiting
 * for something from a file descriptor.  It doesnt look like bad things
 * will happen if you call this elsewhere, but its long time behavior has
 * not been observed.  It *does* however, appear to be much more reliable
 * then the old irc_io, and i even know how this works. >;-)
 */
extern void set_screens (fd_set *, fd_set *);
extern int global_max_fd;

void io (const char *what)
{
	static	int	first_time = 1;
		long	clock_timeout = 0, 
			timer_timeout = 0,
			real_timeout = 0; 
static	struct	timeval my_now,
			my_timer,
			*time_ptr = &my_timer;

	int		hold_over,
			rc;
	fd_set		rd, 
			wd;

	get_time(&my_now);
	now = my_now.tv_sec;
	
	/* CHECK FOR CPU SAVER MODE */
	if (!cpu_saver && get_int_var(CPU_SAVER_AFTER_VAR))
		if (now - idle_time > get_int_var(CPU_SAVER_AFTER_VAR) * 60)
			cpu_saver_on(0, NULL);

	rd = readables;
	wd = writables;
	FD_ZERO(&wd);
	FD_ZERO(&rd);

	set_screens(&rd, &wd);
	set_server_bits(&rd, &wd);
	set_process_bits(&rd);
	set_socket_read(&rd, &wd);
	icmp_sockets(&rd, &wd);
	
#if defined(WANT_THREAD)
#ifdef WANT_NSLOOKUP
	set_dns_output_fd(&rd);
#endif
#ifdef WANT_PTEST
	set_ptest_output_fd(&rd);
#endif
#ifdef WANT_MP3PLAYER
	set_mp3_output_fd(&rd);
#endif
	
# if defined(GTK)
	if (tgtk_okay())
		tgtk_set_output_fd(&rd);
# endif
#endif
	clock_timeout = (timeout_select - (my_now.tv_sec % timeout_select)) * 1000;
	if (cpu_saver && get_int_var(CPU_SAVER_EVERY_VAR))
		clock_timeout += (get_int_var(CPU_SAVER_EVERY_VAR) - 1) * 60000;

	timer_timeout = TimerTimeout();

	if ((hold_over = unhold_windows()))
		real_timeout = 0;
	else if (timer_timeout <= clock_timeout)
		real_timeout = timer_timeout;
	else
		real_timeout = clock_timeout;

	if (real_timeout == -1)
		time_ptr = NULL;
	else
	{
		time_ptr->tv_sec = real_timeout / 1000;
		time_ptr->tv_usec = ((real_timeout % 1000) * 1000);
	}
	
	/* GO AHEAD AND WAIT FOR SOME DATA TO COME IN */
	switch ((rc = new_select(&rd, &wd, time_ptr)))
	{
		case 0:
			break;
		case -1:
		{
			/* if we just got a sigint */
			first_time = 0;
			if (cntl_c_hit)
				edit_char('\003');
			else if (errno && errno != EINTR)
			{
				int ii = 0;
				fd_set rd1, wd1;
				char ii_buff_r[500];
				char ii_buff_w[500];
				int ii_r[FD_SETSIZE];
				int ii_w[FD_SETSIZE];
				yell("Select failed with [%d:%s]", errno, strerror(errno));
				/* Reseed fd_sets so we can dig further */
				yell("Packing fd_sets... Dump of fd's set in fd_set");
				ii_buff_r[0] = '\0';
				ii_buff_w[0] = '\0';
				for (ii = 0; ii < FD_SETSIZE; ii++) {
					ii_r[ii] = 0;
					ii_w[ii] = 0;
				}
				FD_ZERO(&wd1);
				FD_ZERO(&rd1);
				set_screens(&rd1, &wd1);
				set_server_bits(&rd1, &wd1);
				set_process_bits(&rd1);
				set_socket_read(&rd1, &wd1);
				icmp_sockets(&rd1, &wd1);
#if defined(WANT_THREAD)
#ifdef WANT_NSLOOKUP
				set_dns_output_fd(&rd1);
#endif
#ifdef WANT_PTEST
				set_ptest_output_fd(&rd1);
#endif
#ifdef WANT_MP3PLAYER
				set_mp3_output_fd(&rd1);
#endif
	
# if defined(GTK)
	if (tgtk_okay())
				tgtk_set_output_fd(&rd1);
# endif
#endif
				for (ii = 0; ii <= global_max_fd; ii++) {
					fd_set rblah, wblah;
					memcpy(&rblah, &rd1, sizeof(fd_set));
					FD_SET(ii, &rblah);
					if (memcmp(&rblah, &rd1, sizeof(fd_set)) == 0) {
						char blahblah[20];
						sprintf(blahblah, "%d ", ii);
						strcat(ii_buff_r, blahblah);
						ii_r[ii] = 1;
					}
					memcpy(&wblah, &wd1, sizeof(fd_set));
					FD_SET(ii, &wblah);
					if (memcmp(&wblah, &wd1, sizeof(fd_set)) == 0) {
						char blahblah[20];
						yell("blah");
						sprintf(blahblah, "%d ", ii);
						strcat(ii_buff_w, blahblah);
						ii_w[ii] = 1;
					}
				}
				yell("Read fd's in set: %s", (ii_buff_r[0] == '\0') ? "<NONE>" : ii_buff_r);
				for (ii = 0; ii <= global_max_fd; ii++) {
					if (ii_r[ii] == 1) {
						struct stat st;
						if (fstat(ii, &st) == -1) {
							yell("READ FD %d is causing the select failure!", ii);
						}
						else {
							if (S_ISSOCK(st.st_mode))
								yell("READ FD %d is a socket!", ii);
							else if (S_ISREG(st.st_mode))
								yell("READ FD %d is a regular file!", ii);
							else if (S_ISFIFO(st.st_mode))
								yell("READ FD %d is a FIFO!", ii);
							else ;
						}
					}
				}
				yell("Write fd's in set: %s", (ii_buff_w[0] == '\0') ? "<NONE>" : ii_buff_w);
				for (ii = 0; ii <= global_max_fd; ii++) {
					if (ii_w[ii] == 1) {
						struct stat st;
						if (fstat(ii, &st) == -1) {
							yell("WRITE FD %d is causing the select failure!", ii);
						}
						else {
							if (S_ISSOCK(st.st_mode))
								yell("WRITE FD %d is a socket!", ii);
							else if (S_ISREG(st.st_mode))
								yell("WRITE FD %d is a regular file!", ii);
							else if (S_ISFIFO(st.st_mode))
								yell("WRITE FD %d is a FIFO!", ii);
							else ;
						}
					}
				}
				sleep(10);
			}
			else 
			{
#if 0
				yell("errno 0 rc = -1, maybe it'll go away");
				sleep(10);
#endif
			}
			break;

		}

		/* we got something on one of the descriptors */
		default:
		{
			cntl_c_hit = 0;
			now = time(NULL);
			make_window_current(NULL);
			do_screens(&rd);
			check_icmpresult(&rd, &wd);
#if defined(WANT_THREAD)
#ifdef WANT_NSLOOKUP
			dns_check(&rd);
#endif
#ifdef WANT_PTEST
			ptest_check(&rd);
#endif
#ifdef WANT_MP3PLAYER
			mp3_check(&rd);
#endif
# if defined(GTK)
			if (tgtk_okay())
				tgtk_check(&rd);
# endif
#endif
			do_server(&rd, &wd);
			do_processes(&rd);
			scan_sockets(&rd, &wd);
			clean_sockets();
			break;
		} 
	}

	now = time(NULL);
	ExecuteTimers();

        get_child_exit(-1);
	if (update_refresh)
	{
		update_refresh = 0;
		refresh_screen(0, NULL);
	}
	if (!hold_over)
		cursor_to_input();


	if (update_clock(RESET_TIME))
	{
		if (get_int_var(CLOCK_VAR))
		{
			update_all_status(current_window, NULL, 0);
			cursor_to_input();
		}
		clean_queue(get_int_var(TRANSFER_TIMEOUT_VAR)); /* timeout if send time is greater than 5 minutes */
	}

	/* (set in term.c) -- we should redraw the screen here */
	if (need_redraw)
		refresh_screen(0, NULL);
#ifdef WANT_THREAD
	if (scan_done)
		scan_is_done();	
#endif
	alloca(0);
	return;
}

int main(int argc, char *argv[], char *envp[])
{
	char	*channel;

	debug_init();
	srand((unsigned)time(NULL));
	time(&start_time);
	time(&idle_time);
	time(&now);
#ifdef WINNT
	fprintf(stdout, "%s %s\r\n", nap_version, internal_version);
	fprintf(stdout, "%s\r\n", main_version);
#else
	fprintf(stdout, "%s %s\n", nap_version, internal_version);
	fprintf(stdout, "%s\n",  main_version);
#endif
	channel = parse_args(argv, argc, envp);

	FD_ZERO(&readables);
	FD_ZERO(&writables);
	
	if (term_init(NULL))
		_exit(1);

/* 	my_signal(SIGQUIT, SIG_IGN, 0);*/
	my_signal(SIGHUP, irc_exit_old, 0);
	my_signal(SIGTERM, irc_exit_old, 0);
	my_signal(SIGPIPE, SIG_IGN, 0);
	my_signal(SIGINT, cntl_c, 0);
	my_signal(SIGALRM, nothing, 0);
        my_signal(SIGCHLD, child_reap, 0);
	my_signal(SIGCONT, term_cont, 0);
	my_signal(SIGWINCH, sig_refresh_screen, 0);
	my_signal(SIGUSR1, sigusr1, 0);
	        
	if (!init_screen()) 
	{
		create_new_screen();
		new_window(main_screen);
	}

	init_keys();
	init_keys2();
	init_variables();
#ifdef WANT_THREAD
	init_share_mutexes();
#ifdef WANT_NSLOOKUP
	start_dns();
#endif
#ifdef WANT_MP3PLAYER
	start_mp3();
#endif

#ifdef GTK
	start_tgtk();
#endif
#endif
	build_status(current_window, NULL, 0);
	update_input(UPDATE_ALL);

#ifndef WINNT
	charset_ibmpc();
#endif
	load_scripts();
	read_unfinished_list();
	if (auto_connect)
	{
		int serv = 0;
		reload_save(NULL, NULL, NULL, NULL, 0);
		if (starting_server != -1)
			serv = starting_server;
		connect_to_server_by_refnum(serv, -1, create);
		current_window->server = serv;
		xterm_settitle();
	} 
	else
	{
		if (do_get_napigator)
			get_napigator();
		display_server_list();
	}
			
	set_input_prompt(current_window, get_string_var(INPUT_PROMPT_VAR), 0);
	for (;;)
		io("main");
#ifdef GUI1
	gui_exit();
#else
	nappanic("get_line() returned");
#endif
	return (-((int)0xdead));
}

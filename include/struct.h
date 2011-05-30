 /* $Id: struct.h,v 1.1.1.1 2001/01/19 15:53:14 edwards Exp $ */
 
/*
 * struct.h: header file for structures needed for prototypes
 *
 * Written by Scott Reynolds, based on code by Michael Sandrof
 * Heavily modified by Colten Edwards for BitchX
 *
 * Copyright(c) 1997
 *
 */

#ifndef __struct_h_
#define	__struct_h_

#ifdef WINNT
#include <windows.h>
#endif

#include "alist.h"

#define MAX_FUNCTIONS 40

#define ALIAS_MAXARGS 32

struct WindowStru;
	
typedef struct  status_line {
        char *		raw;
        char *		format;
        const char *	(*func[MAX_FUNCTIONS]) (struct WindowStru *, int, int);
	int		map[MAX_FUNCTIONS];
	int		key[MAX_FUNCTIONS];
        int   		count;
        char *		result;
} Status_line;

typedef struct  status_stuff {          
        Status_line     line[3];
        int             double_status;
	int		status_split;
	int		status_lines;
        char            *special;
} Status;


struct ArgListT {
	char *	vars[ALIAS_MAXARGS];
	char *	defaults[ALIAS_MAXARGS];
	int	void_flag;
	int	dot_flag;
};

typedef struct ArgListT ArgList;

typedef struct  AliasItemStru
{
	char    *name;                  /* name of alias */
	u_32int_t hash;
	char    *stuff;                 /* what the alias is */
	char    *stub;                  /* the file its stubbed to */
	int     global;                 /* set if loaded from global' */
	int	cache_revoked;		/* Cache revocation index. */
	int	debug;			/* debug invoke? */
	ArgList *arglist;
}	Alias;


struct _CtcpEntry;

typedef char *((*CTCP_Handler) (struct _CtcpEntry *, char *, char *, char *));

typedef	struct _CtcpEntry
{
	char		*name;  /* name of ctcp datatag */
	int		id;	/* index of this ctcp command */
	int		flag;	/* Action modifiers */
	char		*desc;  /* description returned by ctcp clientinfo */
	CTCP_Handler 	func;	/* function that does the dirty deed */
	CTCP_Handler 	repl;	/* Function that is called for reply */
}	CtcpEntry;

typedef struct WhoEntryT
{
        struct WhoEntryT *next;
	char *who_target;
	char *who_mask;
	char *who_result;
	char *who_end;
	unsigned int flags;
	void (*end) (struct WhoEntryT *, char *, char *);
} WhoEntry;


typedef struct 
{
	int is_read;
	int is_write;
	int port;
	char *server;
	unsigned long flags;
	time_t time;
	void (*func_read) (int);
	void (*func_write) (int);
	void (*cleanup) (int);
	void *info;
} SocketList;

typedef char *(bf) (char *, char *);
typedef struct 
{
	char    *name;
	bf      *func;
}       BuiltInFunctions;

#ifdef UNKNOWN
#undef UNKNOWN
#endif
typedef enum NoiseEnum {
	UNKNOWN = 0,
	SILENT,
	QUIET,
	NORMAL,
	NOISY
} Noise;
                                
/* Hook: The structure of the entries of the hook functions lists */
typedef struct	hook_stru
{
struct	hook_stru *next;

	char	*nick;			/* /on type NICK stuff */
	char	*stuff;			/* /on type nick STUFF */

	int	not;			/* /on type ^nick stuff */
	Noise	noisy;			/* /on [^-+]type nick stuff */

	int	sernum;			/* /on #type NUM nick stuff */
					/* Default sernum is 0. */

	int	global;			/* set if loaded from `global' */
	int	flexible;		/* on type 'NICK' stuff */
	int	debug;			/* turn debug on/off */
	char	*filename;		/* Where it was loaded */
	int	(*hook_func) (char *, char *, char **);
	int	(*num_func) (int, char *, char **);
}	Hook;

/* HookFunc: A little structure to keep track of the various hook functions */
typedef struct
{
	char	*name;			/* name of the function */
	Hook	*list;			/* pointer to head of the list for this
					 * function */
	int	params;			/* number of parameters expected */
	int	mark;
	unsigned flags;
}	HookFunc;

typedef struct _NumericFunction
{
	struct _NumericFunction *next;
	char	*name;
	char	*module;
	int	number;
	Hook	*list;
}       NumericFunction;
                
/* IrcCommand: structure for each command in the command table */
typedef	struct
{
	char	*name;					/* what the user types */
	char	*server_func;				/* what gets sent to the server
							 * (if anything) */
	void	(*func) (char *, char *, char *, char *, unsigned int);	
				/* function that is the command */
	char	*help;
	unsigned int flag;
	unsigned int numeric;
}	IrcCommand;


/* Hold: your general doubly-linked list type structure */

typedef struct HoldStru
{
	char	*str;
	struct	HoldStru	*next;
	struct	HoldStru	*prev;
	int	logged;
}	Hold;

typedef struct	lastlog_stru
{
	int	level;
	char	*msg;
	time_t	time;
	struct	lastlog_stru	*next;
	struct	lastlog_stru	*prev;
}	Lastlog;

struct	ScreenStru;	/* ooh! */


struct _getfile_;

typedef struct _file_struct {
	struct _file_struct *next;
	char *filename;
	struct _file_struct *prev;
	struct _getfile_ *getfile;
	char *checksum;
	unsigned long filesize;
	unsigned int bitrate;
	unsigned int freq;
	int stereo;
	time_t seconds;
	char *nick;
	unsigned long ip;
	int port;
	unsigned short speed;	
	int type;
	int shared;
	int flags;
	struct timeval result;
	struct timeval start;
	int icmp;
} FileStruct;

typedef struct _getfile_ {
	struct _getfile_ *next;	
	char	*nick;
	struct	_getfile_ *prev;
	char	*ip;
	char	*checksum;
	char	*filename;
	char	*realfile;

	int	socket;
	int	port;
	int	write;
	int	count;
	unsigned long filesize;
	unsigned long received;
	unsigned long resume;
	time_t	starttime;
	time_t	addtime;
	int	speed;
	int	flags;
	int	last_perc;
	int	deleted;
	char	*passwd;
	struct  _file_struct *filestruct; 
} GetFile;

typedef struct _resume_file_ {
	struct _resume_file_ *next;
	char *checksum;
	unsigned long filesize;
	char *filename;
	char *nick;
	FileStruct *results;
} ResumeFile;




typedef	struct	DisplayStru
{
	char	*line;
	void	*data;
	int	linetype;
	struct	DisplayStru	*next;
	struct	DisplayStru	*prev;
}	Display;

typedef struct {
	int libraries;
	int gigs;
	int songs;
	int total_unreach_messages;
	int total_unreach_real;
	int total_unreach_bogus;
	int total_unreach_switch_to_zero;
	int total_unreach_duplicates;
	int total_unreach_pending;
} N_STATS;

typedef struct _nick_struct {
	struct _nick_struct *next;
	char	*nick;
	struct	_nick_struct *prev;
  	int	speed;

	unsigned long shared;
	char	*class;
	char	*version;
	time_t	online;
	char	*status;
	char	*channels;
	unsigned long	download;
	unsigned long	upload;
	
	int	limit;			/* users limit of files */
	FileStruct *file_browse;	/* file browse on a nick */
	GetFile	*Queued;		/* queue requests for this server */
	int	flag;			/* browser flags */
	FileStruct *true_bottom;        /* bottom of browse list */
	FileStruct *top;                /* current top for our browser */
	FileStruct *bottom;             /* current bottom for our browser */
	int block_mode;                 /* Are we currently in block mode? */
} NickStruct;

typedef struct _channel_struct {
	struct _channel_struct *next;
	char *channel;
	char *topic;
	int injoin;
	int server;
	int bancount;
	unsigned int mode;
	NickStruct *nicks;
} ChannelStruct;


typedef	struct	WindowStru
{
	char			*name;
	unsigned int	refnum;		/* the unique reference number,
					 * assigned by IRCII */
	int	server;			/* server index */
	int	last_server;		/* last server we were connected to */
	int	top;			/* The top line of the window, screen
					 * coordinates */
	int	bottom;			/* The botton line of the window, screen
					 * coordinates */
	int	cursor;			/* The cursor position in the window, window
					 * relative coordinates */
	int	line_cnt;		/* counter of number of lines displayed in
					 * window */
	int	absolute_size;
	int	noscroll;		/* true, window scrolls... false window wraps */
	int	scratch_line;		/* True if a scratch window */
	int	old_size;		/* if new_size != display_size, resize_display */
	int	visible;		/* true, window ise, window is drawn... false window is hidden */
	int	update;			/* window needs updating flag */
	int	repaint_start;
	int	repaint_end;
	unsigned miscflags;		/* Miscellaneous flags. */
	int	beep_always;		/* should this window beep when hidden */
	unsigned long notify_level;
	unsigned long window_level;		/* The LEVEL of the window, determines what
						 * messages go to it */
	int	skip;
	int	columns;	
	char	*prompt;		/* A prompt string, usually set by EXEC'd process */
#if 0
	int	double_status;		/* number of status lines */
	int	status_split;		/* split status to top and bottom */
	int	status_lines;		/* replacement for menu struct */
#endif
	
	Status	status;
#if 0
	char    *(*status_func[4][MAX_FUNCTIONS]) (struct WindowStru *);
	char	*status_format[4];
	char	*status_line[4];
	int	func_cnt[4];
#endif			

	Display *top_of_scrollback,	/* Start of the scrollback buffer */
		*top_of_display,	/* Where the viewport starts */
		*ceiling_of_display,	/* the furthest top of display */
		*display_ip,		/* Where next line goes in rite() */
		*scrollback_point,
		*screen_hold;	/* Where t_o_d was at start of sb */
	int	display_buffer_size;	/* How big the scrollback buffer is */
	int	display_buffer_max;	/* How big its supposed to be */
	int	display_size;		/* How big the window is - status */

	int	lines_scrolled_back;	/* Where window is relatively */

	int	hold_mode;		/* True if we want to hold stuff */
	int	holding_something;	/* True if we ARE holding something */
	int	hold_interval;
	int	held_displayed;		/* lines displayed since last hold */
	int	lines_displayed;	/* Lines held since last unhold */
	int	lines_held;		/* Lines currently being held */
	int	last_lines_held;	/* Last time we updated "lines held" */
	int	distance_from_display;

	char	*current_channel;	/* Window's current channel */
	char	*bind_channel;
	
	char	*query_nick;		/* User being QUERY'ied in this window */
		
	NickStruct *nicks;		/* List of nicks that will go to window */

	ChannelStruct	*nchannels;
	ChannelStruct	*oldchannels;
	ChannelStruct	*waiting_channels;
	
			
	/* lastlog stuff */
	Lastlog	*lastlog_head;		/* pointer to top of lastlog list */
	Lastlog	*lastlog_tail;		/* pointer to bottom of lastlog list */
	unsigned long lastlog_level;	/* The LASTLOG_LEVEL, determines what
					 * messages go to lastlog */
	int	lastlog_size;		/* number of messages in lastlog */
	int	lastlog_max;		/* Max number of msgs in lastlog */
	
	char	*logfile;		/* window's logfile name */
	/* window log stuff */
	int	log;			/* true, file logging for window is on */
	FILE	*log_fp;		/* file pointer for the log file */

	int	window_display;		/* should we display to this window */

	void	(*output_func)	(struct WindowStru *, const unsigned char *);
	void	(*status_output_func)	(struct WindowStru *);
	
	struct	ScreenStru	*screen;
	struct	WindowStru	*next;	/* pointer to next entry in window list (null
					 * is end) */
	struct	WindowStru	*prev;	/* pointer to previous entry in window list
					 * (null is end) */
	int	deceased;		/* set when window is killed */
	int	in_more;
	int	save_hold_mode;
	int	mangler;
	int	ircmode;		/* window in irc mode */
}	Window;

/*
 * WindowStack: The structure for the POP, PUSH, and STACK functions. A
 * simple linked list with window refnums as the data 
 */
typedef	struct	window_stack_stru
{
	unsigned int	refnum;
	struct	window_stack_stru	*next;
}	WindowStack;

typedef	struct
{
	int	top;
	int	bottom;
	int	position;
}	ShrinkInfo;

typedef struct PromptStru
{
	char	*prompt;
	char	*data;
	int	type;
	int	echo;
	void	(*func) (char *, char *);
	struct	PromptStru	*next;
}	WaitPrompt;


typedef	struct	ScreenStru
{
	int	screennum;
	Window	*current_window;
	unsigned int	last_window_refnum;	/* reference number of the
						 * window that was last
						 * the current_window */
	Window	*window_list;			/* List of all visible
						 * windows */
	Window	*window_list_end;		/* end of visible window
						 * list */
	Window	*cursor_window;			/* Last window to have
						 * something written to it */
	int	visible_windows;		/* total number of windows */
	WindowStack	*window_stack;		/* the windows here */

	struct	ScreenStru *prev;		/* These are the Screen list */
	struct	ScreenStru *next;		/* pointers */


	FILE	*fpin;				/* These are the file pointers */
	int	fdin;				/* and descriptions for the */
	FILE	*fpout;				/* screen's input/output */
	int	fdout;

	char	input_buffer[INPUT_BUFFER_SIZE+2];	/* the input buffer */
	int	buffer_pos;			/* and the positions for the */
	int	buffer_min_pos;			/* screen */

	int	input_cursor;
	char	*input_prompt;

	int     input_visible;
	int     input_zone_len;
	int     input_start_zone;
	int     input_end_zone;
	int     input_prompt_len;
	int     input_prompt_malloc;
	int     input_line;
	Lastlog *lastlog_hold;
		
	char	saved_input_buffer[INPUT_BUFFER_SIZE+2];
	int	saved_buffer_pos;
	int	saved_min_buffer_pos;

	WaitPrompt	*promptlist;



	int	meta_hit;
	int	quote_hit;			/* true if a key bound to
						 * QUOTE_CHARACTER has been
						 * hit. */
	int	digraph_hit;			/* A digraph key has been hit */
	unsigned char	digraph_first;


	char	*redirect_name;
	char	*redirect_token;
	int	redirect_server;

	char	*tty_name;
	int	co;
	int	li;
	int	old_co;
	int	old_li;
#ifdef WINNT
	HANDLE  hStdin;
	HANDLE  hStdout;
#endif
#ifdef __EMXPM__
	HVPS	hvps;
	HWND	hwndFrame,
		hwndClient,
		hwndMenu;
	int	VIO_font_width,
		VIO_font_height;
	char	aviokbdbuffer[256];
#elif defined(GTK)
	GtkWidget	*window,
			*viewport,
			*menubar,
			*scroller,
			*box;
        GdkFont		*font;
        char            *fontname;
	GtkAdjustment	*adjust;
	gint		gtkio;
	int 		pipe[2];
	int 		maxfontwidth,
			maxfontheight;
#endif

	char	*menu;		/* windows menu struct */

	int	alive;
}	Screen;

typedef	struct	list_stru
{
	struct	list_stru	*next;
	char	*name;
	struct	list_stru	*prev;
}	List;

/* a structure for the timer list */
typedef struct	timerlist_stru
{
	struct	timerlist_stru *next;
	char	ref[REFNUM_MAX + 1];
	unsigned long refno;
struct	timeval	time;
	int	(*callback) (void *, char *);
	char	*command;
	char	*subargs;
	int	events;
	double	interval;
	int	server;
	int	window;
	char	*whom;
	int	delete;
}	TimerList;

extern TimerList *PendingTimers;

/* IrcVariable: structure for each variable in the variable table */
typedef struct
{
	char	*name;			/* what the user types */
	u_32int_t hash;
	int	type;			/* variable types, see below */
	int	integer;		/* int value of variable */
	char	*string;		/* string value of variable */
	void	(*func)(Window *, char *, int);		/* function to do every time variable is set */
	char	int_flags;		/* internal flags to the variable */
	unsigned short	flags;		/* flags for this variable */
}	IrcVariable;

typedef Window *(*window_func) (Window *, char **args, char *usage);

typedef struct window_ops_T {
	char		*command;
	window_func	func;
	char		*usage;
} window_ops;

typedef struct _ignore_nick_struct {
	struct _ignore_nick_struct *next;
	char *nick;
	unsigned long start;
	unsigned long end;
} IgnoreStruct;
                        
typedef struct _Stats {
	int libraries;
	int gigs;
	int songs;
	unsigned long total_files;
	double total_filesize;
	unsigned long files_served;
	double filesize_served;
	unsigned long files_received;
	double filesize_received;
	double max_downloadspeed;
	char *max_downloadspeed_nick;
	double max_uploadspeed;
	char *max_uploadspeed_nick;
	time_t starttime;
	unsigned long shared_files;
	double shared_filesize;
} Stats;


typedef struct _Msgs_ {
	struct _Msgs_ *next;
	char	*nick;
	char	*msg;
	time_t	time;
} Msgs;

typedef struct _Ping {
	struct _Ping *next;
	char *nick;
	struct timeval start;
} PingStruct;


typedef struct {
	char	*name;			/* name of this server */
	int	meta;			/* use a meta server */
	int	read;			/* read socket */
	int	write;			/* write socket */
	int	port;			/* port to connect to */
	int	data;			/* data socket */
	int	dataport;		/* dataport */
	int	flags;			/* flags for this server */
	char	*password;		/* password */
	char	*d_nickname;		/* desired nick */
	char	*itsname;		/* proper name for this server */
	unsigned long channel_count;	/* number of channels */
	unsigned long nick_count;	/* number of nicks on those channels */
	unsigned long resume_results;	/* number of resume results */
	int	motd;			/* display motd */
	int	version;		/* server version */
	N_STATS statistics;
	Msgs	*msgs;			/* recieved msgs on this server */
	NickStruct *users;		/* browsed users and queues */

	NickStruct *true_bottom;
	NickStruct *top;
	NickStruct *bottom;

	FileStruct *search_results;
	WhoEntry   *who_queue;
	int	sent;
	int	waiting_out;
	int	waiting_in;
	int	cloak;
struct	timeval	lagtime;
	int	sping;
	int	level;
	int	user_count;
	char	*showuser;
	char	*showusercmd;
	char	*listmode;
	int	icmp_sock;
	int	irc_mode;
} Server;

typedef struct {
	unsigned short len;
	unsigned short command;
} N_DATA;

/* UrlList: structure for the urls in your Url List */
typedef struct  urllist_stru
{
	struct  urllist_stru *next;     /* pointer to next url entry */
	char    *name;                  /* name */
} UrlList;



#endif /* __struct_h_ */

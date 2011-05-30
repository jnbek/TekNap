
 /* $Id: napster.h,v 1.2 2001/07/13 20:45:28 edwards Exp $ */
 
#ifndef _napster_h
#define _napster_h

#define MAX_SPEED		10

#define NAP_DOWNLOAD		0x0001
#define NAP_UPLOAD		0x0010
#define NAP_CHAT		0x0020
#define NAP_CHAT_CONNECTED	0x0040
#define NAP_DCC_COMMANDS	0x0080
#define NAP_QUEUED		0x0100
#define NAP_RESUME		0x1000


#define MP3_ONLY		0x0001
#define VIDEO_ONLY		0x0010
#define IMAGE_ONLY		0x0100
#define ANY_FILE		0x1000

#define AUDIO			0x0001
#define VIDEO			0x0010
#define IMAGE			0x0100

#define MODE_STEREO		0
#define MODE_JOINT_STEREO	1
#define MODE_DUAL_CHANNEL	2
#define MODE_MONO		3


#define END_BROWSE		0x0001
#define BROWSE_IN_PROGRESS	0x0010


#define NORMAL_SORT 0
#define SONG_SORT 1
#define SPEED_SORT 2

typedef unsigned char _N_CMD;
                
typedef struct _AUDIO_HEADER {
	unsigned long filesize;
	int mpeg25;
	int ID;
	int layer;
	int error_protection;
	int bitrate_index;
	int sampling_frequency;
	int padding;
	int extension;
	int mode; /* 0 = STEREO 1 = Joint 2 = DUAL 3 = Mono */
	int mode_ext;
	int copyright;
	int original;
	int emphasis;
	int stereo;
	int jsbound;
	int sblimit;
	int true_layer;
	int framesize;
	int freq;
	unsigned long totalframes;
	unsigned long bitrate;
} AUDIO_HEADER;

enum nap_Commands {
	CMDR_ERROR		= 0,
	CMDS_UNKNOWN		= 1,
	CMDS_LOGIN		= 2, /* user pass dataport "version" speed */
	CMDR_EMAILADDR		= 3, /* email address */
	CMDR_BASTARD		= 4, /* unknown */
	CMDS_REGISTERINFO	= 6, /* userinfo */
	CMDS_CREATEUSER		= 7, /* create user account */
	CMDR_CREATED		= 8, /* account created */
	CMDR_CREATEERROR	= 9, /* username taken */
	CMDR_ILLEGALNICK	= 10, /* illegal nickname specified */
	
	CMDR_LOGINERROR		= 13, 

	CMDS_OPTIONS		= 14, /* NAME:%s ADDRESS:%s CITY:%s STATE:%s PHONE:%s AGE:%s INCOME:%s EDUCATION:%s   *login options */

	CMDR_MSTAT		= 15, 
	CMDR_REQUESTUSERSPEED	= 89,
	CMDR_SENDFILE		= 95,
	CMDS_ADDFILE		= 100,

	CMDS_REMOVEFILE		= 102, /* "\path\to\filename\" for removal */

	CMDR_GETQUEUE		= 108,

	CMDR_MOTD		= 109,
	CMDS_REMOVEALLFILES	= 110, /* remove all shared files */
	CMDR_ANOTHERUSER	= 148,
	CMDS_SEARCH		= 200,
	CMDR_SEARCHRESULTS	= 201,
	CMDR_SEARCHRESULTSEND	= 202,

	/* if dataport is 0 we use 500 to request a transfer. 0 is a firewalled host */
	CMDS_REQUESTFILE	= 203,
	CMDR_FILEREADY		= 204,

	CMDS_SENDMSG		= 205,
	CMDR_GETERROR		= 206,

	CMDS_ADDHOTLIST		= 207,

	CMDS_ADDHOTLISTSEQ	= 208,
	CMDR_HOTLISTONLINE	= 209,
	CMDR_USEROFFLINE	= 210, 

	CMDS_BROWSE		= 211,
	CMDR_BROWSERESULT	= 212,
	CMDR_BROWSEENDRESULT	= 213,
	CMDR_STATS		= 214,

	CMDS_REQUESTRESUME	= 215, /* checksum filesize */
	CMDR_RESUMESUCCESS	= 216, /* nick ip port filename checksum size connection */
	CMDR_RESUMEEND		= 217, /* end resume for checksum filesize */

	CMDS_UPDATE_GET1	= 218, /* add 1 to download */
	CMDS_UPDATE_GET		= 219, /* sub 1 from download */
	CMDS_UPDATE_SEND1	= 220, /* add 1 for send */
    	CMDS_UPDATE_SEND	= 221, /* sub 1 from send */

	CMDS_TESTPORT		= 300, /* portnum */

	CMDR_HOTLISTSUCCESS	= 301,
	CMDR_HOTLISTERROR	= 302, /* not on hotlist */
	CMDS_HOTLISTREMOVE	= 303, /* nick */
	

	CMDS_BLOCKLIST		= 330, /* none */
	CMDS_BLOCK		= 332, /* ip [reason] */
	CMDS_UNBLOCK		= 333, /* ip */	


	CMDS_JOIN		= 400,
	CMDS_PART		= 401,
	CMDS_SEND		= 402,
	CMDR_PUBLIC		= 403,
	CMDR_ERRORMSG		= 404,
	CMDR_JOIN		= 405,
	CMDR_JOINNEW		= 406,
	CMDR_PARTED		= 407,
	CMDR_NAMES		= 408,
	CMDR_ENDNAMES		= 409,
	CMDS_TOPIC		= 410, /* got/change topic */

	CMDS_CBANLIST		= 420, /* <channel> */
	CMDR_CBANLIST		= 421, /* <nick> <who> "<reason>" <time> */
	CMDS_CBAN		= 422, /* <channel> <nick> [reason] */
	CMDS_CUNBAN		= 423, /* <channel> <nick> [reason] */
	CMDS_CBANCLEAR		= 424, /* <channel> */
		
	CMDS_REQUESTFILEFIRE	= 500,
	CMDR_FILEINFOFIRE	= 501, /* if firewalled then expect a 501 request send */

	CMDS_REQUESTLINESPEED	= 600,
	CMDR_LINESPEED		= 601,

	CMDS_REQUESTSIZE	= 602,
	CMDS_WHOIS		= 603,
	CMDR_WHOIS		= 604,
	CMDR_WHOWAS		= 605,
	CMDS_SETUSERLEVEL	= 606, /* moderators/administrators/elite */
	CMDR_FILEREQUEST	= 607, /* nick \"filename\" */
	CMDS_FILEINFO		= 608, /* nick \"filename\" */
	CMDR_ACCEPTERROR	= 609, /* accept failed on request */

	CMDS_KILLUSER		= 610, /* return 404 permission denied */
	CMDS_NUKEUSER		= 611, /* return 404 */
	CMDS_BANUSER		= 612,
	CMDR_SETDATAPORT	= 613, 
	CMDS_UNBANUSER		= 614,
	CMDS_BANLIST		= 615,
	CMDR_BANLIST_IP		= 616,
	CMDS_LISTCHANNELS	= 617,
	CMDR_LISTCHANNELS	= 618,

	CMDS_SENDLIMIT		= 619, /* nick "filename" queuelimit */
	CMDR_SENDLIMIT		= 620, /* nick "filename" filesize queuelimit */

	CMDR_MOTDS		= 621, 
	CMDS_MUZZLE		= 622,
	CMDS_UNMUZZLE		= 623,
	CMDS_UNNUKEUSER		= 624, /* return 404 */
	CMDS_SETLINESPEED	= 625,
	CMDR_DATAPORTERROR	= 626,
	CMDS_OPSAY		= 627,
	CMDR_ANNOUNCE		= 628,
	CMDR_BANLIST_NICK	= 629,

	CMDS_BROWSE_DIRECT_REQ	= 640, /* new browse request */
	CMDR_BROWSE_DIRECT	= 641, /* returns, nick ip port */
	CMDR_BROWSE_DIRECT_ERROR= 642, /* nick is not online or just nick */

	CMDS_CLOAK		= 652, /* toggle moderator cloaking */
		
	CMDS_CHANGESPEED	= 700,
	CMDS_CHANGEPASS		= 701,
	CMDS_CHANGEEMAIL	= 702,
	CMDS_CHANGEDATA		= 703,

	CMDS_SPING		= 750, /* guessing at this one. returns 750*/
	CMDS_PING		= 751, /* user */
	CMDS_PONG		= 752, /* <user> recieved from a ping*/
			       /* <user> can also be used to send a pong */
	CMDS_SETPASSWORD	= 753, /* <user> <password> [reason] */
		
	CMDS_RELOADCONFIG	= 800, /* <config variable> */
	CMDS_SERVERVERSION	= 801, /* none */
	/* 805 missing */
	CMDS_SETCONFIG		= 810, /* <config string */
	/* 811 */
	CMDS_CLEARCHANNEL	= 820,  /* channelname */
	/* 821 822 */
	CMDR_CLIENTREDIR	= 821, /* client redirect <nick> <host> <port>*/
	CMDR_CYCLE		= 822, /* client cycle <nick> <host> */
	CMDS_SETCHANLEVEL	= 823, /* set channel level <channel> <level> */
	CMDS_SENDME		= 824, /* text */
	CMDR_NICK		= 825,
	CMDS_SETCHANNELLIMIT	= 826, /* <channel> <limit> default 200 */
	CMDS_SHOWALLCHANNELS	= 827, /* shows all channels */
			       /* <channel> <users> <?> <?> <limit> "Topic" */
	CMDR_ALLCHANNELS	= 828, /* <channelname> end of list is empty 827 */

	CMDS_KICK		= 829, /* channel nick [reason] */
	CMDS_NAME		= 830, /* <channel> returns 825 with nick info, 830 is recieved on end of list */
	CMDS_SHOWUSERS		= 831, /* end global list of users */
	CMDS_SHOWUSERSLIST	= 832, /* global list of users */
	CMDS_SHAREPATH		= 870, /* share path of files */

	CMDS_SET_CAPABILITY	= 920, /* set client capabilities. beta8 is 1 */

	/* the following are open-nap specific */
	CMDS_SERVERLINK		= 10100, /* link server  <server> <port> [<remote server>] */
	CMDS_SERVERUNLINK	= 10101, /* unlink server <server> <reason> */
	CMDS_SERVERKILL		= 10110, /* kill server <server> <reason> */
	CMDS_SERVERREMOVE	= 10111, /* remove it <server> <reason> */
	CMDS_SERVERLINKS	= 10112, /* links command */ 
	CMDS_SERVERUSAGE	= 10115, /* usage of server */
	CMDS_SERVERPING		= 10116, /* <server> [args] */
	CMDS_SERVERREHASH	= 10117, /* rehash all variables/motd */

	CMDS_CLIENT_VERSION	= 10118, /* show all client versions */	

	CMDS_SERVERMPING	= 10120, /* server mass ping */
	CMDS_WHOWAS		= 10121, /* WHOWAS nick */
	CMDS_MKILL		= 10122, /* ip ["reason"] */

	CMDS_HISTOGRAM		= 10123, /* show list of numerics with counts */
	CMDR_HISTOGRAM		= 10124, /* end of histogram list */

	CMDS_ADMINREGISTER	= 10200, /* admin register nick <nick> <pass> <email> [<level>] */
	CMDS_KICKUSER		= 10202, /* <channel> <nick> <reason> */
	CMDS_CREATEOP		= 10204, /* <channel> <user> [user ...] */
	CMDR_USERMODE		= 10203, /* usermode */
	CMDS_DELETEOP		= 10205, /* <channel> <user> [user ...] */
	CMDS_LISTOPS		= 10206, /* <channel> */
	CMDS_DROPCHANNEL	= 10207, /* <channel> ["reason"] */
	CMDS_OPWALL		= 10208, /* <channel> <text> */
	CMDS_CHANNELMODE	= 10209, /* <channel> [<+MODERATED> <+PRIVATE>] */
	CMDS_CHANNELINVITE	= 10210, /* <channel> <nick> */

	CMDS_CHANNELVOICE	= 10211, /* <channel> <nick> ... */
	CMDS_CHANNELUNVOICE	= 10212, /* <channel> <nick> ... */
	CMDS_CHANNELMUZZLE	= 10213, /* <channel> <nick> ... */
	CMDS_CHANNELUNMUZZLE	= 10214, /* <channel> <nick> ... */
	CMDR_CLASSADD		= 10250,
	CMDR_CLASSDELETE	= 10251,
	CMDR_CLASSLIST		= 10252, /* <ip> <limit> */
	CMDR_DLINEADD		= 10253,
	CMDR_DLINEDEL		= 10254,
	CMDR_DLINELIST		= 10255, /* <ip> */
	CMDR_ILINEADD		= 10256,
	CMDR_ILINEDEL		= 10257,
	CMDR_ILINELIST		= 10258, /* <ip> */
	CMDR_ELINEADD		= 10259,
	CMDR_ELINEDEL		= 10260,
	CMDR_ELINELIST		= 10261, /* <ip> */
	CMDS_ADDMIMEFILE	= 10300, /* add a mime file type */
	CMDS_NEW_OPENNAPBROWSE	= 10301, /* */
	CMDR_NEW_OPENNAPBROWSE	= 10302  /* */
};


void	parse_server		(N_DATA *, char *);

void	name_print		(const char *, const NickStruct *, int, int, char *);
void	clear_nicks		(int);
void	clear_nchannels		(int);
void	clear_filelist		(FileStruct **);
void	send_hotlist		(void);
int	make_listen		(int, int *);
int	widest_filename		(const FileStruct *);
void	print_file		(const FileStruct *, int, int);
void	free_nicks		(ChannelStruct *);
void	free_nickstruct		(NickStruct *);
void	switch_channels		(char, char *);
char	*base_name		(char *);
int 	files_in_progress	(char *, int);
char	*mp3_time		(time_t);
char	*calc_md5		(int, unsigned long);
char	*make_mp3_string	(FILE *, const FileStruct *, char *, char *, char *);
char	*convertnap_dos		(char *str);
char	*convertnap_unix	(char *str);
GetFile	*find_in_getfile	(int, char *, char *, char *, unsigned long, int);
int	nap_finished_file	(int, int);
void	set_napster_socket	(int);
void	clean_queue		(int);
void	send_from_queue		(int);
int	display_list		(FileStruct *);
void	create_and_do_get	(FileStruct *, int, int);
GetFile *create_send		(char *nick, FileStruct *);
char	*numeric_banner		(int);
char	*n_speed		(int);


extern	NickStruct		*nap_hotlist;
extern	PingStruct		*ping_time;

extern	GetFile			*transfer_struct;
extern	GetFile			*finished_struct;
extern	ResumeFile		*resume_struct;

extern	FileStruct		*fserv_files;

extern	FileStruct		*file_search;

typedef struct {
	int	cmd;
	int	(*func)(int, char *);
} NAP_COMMANDS;

#define NAP_COMM(name) \
	int name (int cmd, char *args)

#define NORMAL_FINISH 		0x0000
#define PREMATURE_FINISH 	0x0001

int	remove_from_resume(char *);
int	write_unfinished_list(void);
int	read_unfinished_list(void);
char	*calc_eta(GetFile *);
char	*print_time(time_t);
char	*find_mime_type(char *);
char	*mode_str(int);
int	check_dcc_msg(char *, char *);
void	clean_sockets(void);
char	*convert_time(time_t);
void	save_hotlist(FILE *);
void	add_to_transfer_list(GetFile *);
void	scan_is_done(void);
void	remove_search_from_glist(FileStruct **);
void	add_files_to_whois(void);
void	dcc_chat_transmit (char *, const char *, const char *, int);
int	add_to_browse_list(NickStruct *, char *, char *);
void	send_direct_browse(int);
int	is_valid_dcc(char *);

extern unsigned long shared_count;
#ifdef THREAD
extern pthread_mutex_t fserv_struct_mutex;
extern pthread_mutex_t shared_count_mutex;
extern pthread_mutex_t send_ncommand_mutex;
void	init_share_mutexes(void);
void	clean_share_mutexes(void);
#endif

#define NAPSTER_SERVER 0
#define OPENNAP_SERVER 1

#define USER_LEECH 0
#define USER_USER 1
#define USER_MODERATOR 2
#define USER_ADMIN 3
#define USER_ELITE 4

extern char *last_invited;

NAP_COMM(cmd_stats);
NAP_COMM(cmd_motd);
NAP_COMM(cmd_email);
NAP_COMM(cmd_bastard);
NAP_COMM(cmd_login);
NAP_COMM(cmd_unknown);
NAP_COMM(cmd_joined);
NAP_COMM(cmd_parted);
NAP_COMM(cmd_names);
NAP_COMM(cmd_endnames);
NAP_COMM(cmd_topic);
NAP_COMM(cmd_public);
NAP_COMM(cmd_msg);
NAP_COMM(cmd_search);
NAP_COMM(cmd_browse);
NAP_COMM(cmd_endsearch);
NAP_COMM(cmd_endbrowse);
NAP_COMM(cmd_whois);
NAP_COMM(cmd_whowas);
NAP_COMM(cmd_channellist);
NAP_COMM(cmd_endchannel);
NAP_COMM(cmd_newchannellist);
NAP_COMM(cmd_newendchannel);
NAP_COMM(cmd_hotlist);
NAP_COMM(cmd_hotlistsuccess);
NAP_COMM(cmd_hotlisterror);
NAP_COMM(cmd_offline);
NAP_COMM(cmd_alreadyregistered);
NAP_COMM(cmd_dataport);
NAP_COMM(cmd_registerinfo);
NAP_COMM(cmd_banlist);
NAP_COMM(cmd_ping);
NAP_COMM(cmd_sping);
NAP_COMM(cmd_pingresponse);
NAP_COMM(cmd_recname);
NAP_COMM(cmd_endname);
NAP_COMM(cmd_getfileinfo);
NAP_COMM(cmd_getfile);
NAP_COMM(cmd_fatalerror);
NAP_COMM(cmd_me);
NAP_COMM(cmd_banlistend);
NAP_COMM(cmd_dataport);
NAP_COMM(cmd_filerequest);
NAP_COMM(cmd_accepterror);
NAP_COMM(cmd_setdataport);
NAP_COMM(cmd_firewall_request);
NAP_COMM(cmd_toomanyfiles);
NAP_COMM(cmd_getlinespeed);
NAP_COMM(cmd_nosuchnick);
NAP_COMM(cmd_announce);
NAP_COMM(cmd_links);
NAP_COMM(cmd_usage);
NAP_COMM(cmd_servermsg);
NAP_COMM(cmd_resumerequest);
NAP_COMM(cmd_resumerequestend);
NAP_COMM(cmd_wallop);
NAP_COMM(cmd_partchannel);
NAP_COMM(cmd_showusersend);
NAP_COMM(cmd_showusers);
NAP_COMM(cmd_endcban);
NAP_COMM(cmd_cbanlist);
NAP_COMM(cmd_whowas1);

NAP_COMM(cmd_newbrowse);
NAP_COMM(cmd_newbrowse_error);
NAP_COMM(cmd_direct_browse);

#define SEARCH_FINISH	0x0000
#define SEARCH_START	0x0001
#define	SEARCH_FULLPATH 0x0002
int get_napigator(void);

#endif

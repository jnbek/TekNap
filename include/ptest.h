 /* $Id: ptest.h,v 1.1.1.1 2000/06/24 11:08:42 edwards Exp $ */
 
/*
 * cdns.h: header for cdns.c
 */

#ifndef _PTEST_H_
#define _PTEST_H_

/* We gotta know about the fd_set type, so we gonna inclue this  */
#include "teknap.h"
#if defined(TEKNAP)
#include "struct.h"
#include "newio.h"
#endif

typedef struct ptest_struct {
	/* 'host': This is the string we want to resolve. It can be
	 * either a host or ip. The string WILL be malloc'ed by
	 * the "add" function. Do NOT malloc or free this variable!
	 */
        char *host;

	char *alias;
	
	/* 'port': This is the port we want to connect to. It can be
	 * either a host or ip.
	 */
        int port;

	/* 'result': This is the "result" of the connect. "man connect"
	 * for the possible ints that could be returned.
	 */
        int result;

	/* 'r_errno': If the connect failed, This will contain
	 * the result errno that was set to when it failed.
	 */
        int r_errno;

	/* 'callback': This is our callback function. When our 'in' gets
	 * resolved, we will call this function, with this structure
	 * as our parameter. Do NOT malloc or free this variable.
	 */
	void (*callback) (int, int, void *); /* Our callback function */

	/* 'callinfo': This allows us to store information that you might
	 * need for later use when in your 'callback' function. This is
	 * just a void pointer, so you MUST malloc any of the data you want to
	 * store here. Once you are done with this variable, you must
	 * free it!!! BEWARE BEWARE, YOU MUST FREE THIS VARIABLE YOURSELF!
	 */
	void *callinfo;

	/* 'next': Internally used. our "next" pointer. */
        struct ptest_struct *next;
} PTEST_QUEUE;

void start_ptest(void);
void stop_ptest(void);
void kill_ptest(void);
void set_ptest_output_fd(fd_set *);
void ptest_check(fd_set *);
void check_ptest_queue(void);
void add_to_ptest_queue(char *, int, void (*) (int, int, void *), char *, void *, int);
void start_ptest(void);
void stop_ptest(void);
void do_ptestcall(char *, char *, char *);

#define PTEST_URGENT 1
#define PTEST_NORMAL 2

#ifndef Q_NEXT
#define Q_NEXT(tmp) ((tmp)->next)
#endif
#ifndef Q_OPEN
#define Q_OPEN(headp, tailp) (*(headp) = *(tailp) = NULL)
#endif

typedef struct {
	char *from;
	char *to;
	char *ip;
	int port;
	int server;
} PT_INFO;

typedef struct {
	/* Check for repeats */
	char *last_ip;
	int last_port;
} PT_STATS;

void free_pt_info(PT_INFO *);

#endif

/************
 *  ptest.c  *
 ************
 *
 * A threaded Port tester, to see if someones ip + port is open.
 *
 * This is really a rip of my cdns.c... It allows us to test whether
 * someones port is unreachable to US, and will return the answer
 * in a queue for later use.
 *
 *
 * Written by Scott H Kilau
 *
 * Copyright(c) 2000
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 * $Id: ptest.c,v 1.1.1.1 2000/11/23 18:11:15 edwards Exp $ *
 */

#include "ptest.h"
#include "teknap.h"	/* To pick up our next #define checks */
#include "struct.h"	/* To pick up stuff that vars.h needs */
#include "output.h"
#include "vars.h"	/* For our ptest vars */

#ifdef TEKNAP
#include "newio.h"
#include "if.h"
#include "commands.h"
#include "output.h"     /* For say() */
#endif

#if defined(WANT_THREAD) && defined(WANT_PTEST)

#define PTEST_TYPE_STOP 0
#define PTEST_TYPE_KILL 1
#define PTEST_RETURN_QUIT 0
#define PTEST_RETURN_CANCEL 1
#define PTEST_RETURN_KILL 2
#define PTEST_RETURN_FAIL 3
#define PTEST_RETURN_NOTRUNNING 4

/* Our static functions */
static void init_ptest_mutexes(void);
static void *start_ptest_thread(void *);
static void do_ptest_lookup(PTEST_QUEUE *);
static void cleanup_ptest(void);
static void destroy_ptest_queue(PTEST_QUEUE **, PTEST_QUEUE **);
static void free_ptest_entry(PTEST_QUEUE *);
static void kill_ptest_thread(int);
static int do_ptest_stopkill(int);
static PTEST_QUEUE *build_ptest_entry(char *, int, void (*) (int, int, void *), char *, void *);
static PTEST_QUEUE *ptest_dequeue(PTEST_QUEUE **,  PTEST_QUEUE **);
static PTEST_QUEUE *ptest_enqueue(PTEST_QUEUE **, PTEST_QUEUE **, PTEST_QUEUE *);
static PTEST_QUEUE *ptest_enqueue_urgent(PTEST_QUEUE **, PTEST_QUEUE **, PTEST_QUEUE *);

/* Our static globals */
static pthread_t ptest_thread = {0};
static pthread_mutex_t pending_queue_mutex = {0};
static pthread_mutex_t finished_queue_mutex = {0};
static pthread_mutex_t quit_mutex = {0};
static pthread_cond_t pending_queue_cond;
static PTEST_QUEUE *PendingQueueHead = NULL, *PendingQueueTail = NULL;
static PTEST_QUEUE *FinishedQueueHead = NULL, *FinishedQueueTail = NULL;
static int ptest_write = -1;
static int ptest_read = -1;
static volatile int ptest_thread_dead = 0;
static char *ptest_return[] =
        {       "PTEST Thread Quit Successfully!",
                "PTEST Thread Cancelled Successfully!",
                "PTEST Thread Killed Successfully!",
                "PTEST Thread Kill Failed!",
                "PTEST Thread Kill Not Running!"
        };

/*
 * start_ptest : This should be called by the main app thread,
 * whenever it wants to start up the ptest stuff
 */
void start_ptest(void)
{
	int fd_array[2];
	/* Init our queues. */
	Q_OPEN(&PendingQueueHead, &PendingQueueTail);
	Q_OPEN(&FinishedQueueHead, &FinishedQueueTail);
	/* Create our mutexes */
	init_ptest_mutexes();
	/* init our pending queue condition, and set to default attributes (NULL) */
	pthread_cond_init(&pending_queue_cond, NULL);
	/* lock up the quit mutex */
	pthread_mutex_lock(&quit_mutex);
	/* create our pipe [0] = main to read from, [1] = ptest to write to */
	pipe(fd_array);
	ptest_read = fd_array[0];
	ptest_write = fd_array[1];
	/* If using in TekNap, register the fds for its main select loop */
#ifdef TEKNAP
	new_open(ptest_read);
	new_open(ptest_write);
#endif
	/* create our ptest thread */
	pthread_create(&ptest_thread, NULL, start_ptest_thread, NULL);
}

/* 
 * stop_ptest : This should be called by the main app thread,
 * whenever it wants to stop the ptest stuff.
 */
void stop_ptest(void)
{
        int retval = do_ptest_stopkill(PTEST_TYPE_STOP);
	if (retval != PTEST_RETURN_NOTRUNNING) {
		/* If stop failed, go and ahead and kill it */
		if (retval == PTEST_RETURN_FAIL)
			retval = do_ptest_stopkill(PTEST_TYPE_KILL);
		say("%s", ptest_return[retval]);
	}
}

/* 
 * kill_ptest : This should be called by the main app thread,
 * when it wants the ptest thread to go away forever. This function
 * kills the thread uncleanly, and probably should only be used
 * when the main app is about to exit()
 */
void kill_ptest(void)
{
	int retval = do_ptest_stopkill(PTEST_TYPE_KILL);
	if (retval != PTEST_RETURN_NOTRUNNING)
		say("%s", ptest_return[retval]);
}

/*
 * add_to_ptest_queue : This should be called by the main app thread,
 * whenever it wants us to see if we can connect() to something or not.
 */
void 
add_to_ptest_queue(char *ip, int port, void (*callback)(int, int, void *), char *cmd, void *data, int urgency)
{
	PTEST_QUEUE *tmp = NULL;

	/* Build ptest entry */
	tmp = build_ptest_entry(ip, port, callback, cmd, data);
	/* Wait until we can get mutex lock for queue */
	pthread_mutex_lock(&pending_queue_mutex);
	/* Enqueue the entry, checking urgency */
	if (urgency == PTEST_URGENT)
		ptest_enqueue_urgent(&PendingQueueHead, &PendingQueueTail, tmp);
	else
		ptest_enqueue(&PendingQueueHead, &PendingQueueTail, tmp);
	/* signal ptest thread, its got work to do */
	pthread_cond_signal(&pending_queue_cond);
	/* Give lock back, so ptest thread can react to signal */
	pthread_mutex_unlock(&pending_queue_mutex);
}

/* set_ptest_output_fd : This makes check_ptest_queue much better.
 * This will add our piped fd into the main's select() loop, so
 * we will know right away when the output queue has data waiting.
 */
void set_ptest_output_fd(fd_set *rd)
{
	if (ptest_read >= 0)
		FD_SET(ptest_read, rd);
}

/* ptest_check : See above. */
void ptest_check(fd_set *rd)
{
	char blah[2];
	if (ptest_read >= 0 && FD_ISSET(ptest_read, rd)) {
		read(ptest_read, &blah, 1);
		check_ptest_queue();
	}
}                      

/*
 * check_ptest_queue : This should be called by the main app thread, at
 * periodic intervals, ie, whenever its convenient.
 */
void check_ptest_queue()
{
	PTEST_QUEUE *tmp = (PTEST_QUEUE *) 0;
	while (pthread_mutex_trylock(&finished_queue_mutex) == 0) {
		tmp = ptest_dequeue(&FinishedQueueHead, &FinishedQueueTail);
		pthread_mutex_unlock(&finished_queue_mutex);
		if (tmp) {
			if (tmp->alias)
			{
				char buffer[BIG_BUFFER_SIZE+1];
				snprintf(buffer, BIG_BUFFER_SIZE, "%s %d %d %d", tmp->host, tmp->port, tmp->result, tmp->r_errno);
				parse_line("PTEST", tmp->alias, buffer, 0, 0, 1);
			}
			else if (tmp->callback)
				tmp->callback(tmp->result, tmp->r_errno, tmp->callinfo);
			else
				fprintf(stderr, "%s:%d error: No callback!",
				   __FILE__, __LINE__);
			free_ptest_entry(tmp);
		}
		else
			return;
	}
}

static int do_ptest_stopkill(int type)
{
	sigset_t set, oldset;
	void *ptr;

	if (ptest_thread <= 0)
		return(PTEST_RETURN_NOTRUNNING);

	/* Here is the deal... There are 3 ways for us to kill this thread. In order of friendliness:
	 * 1) Unlock the quit mutex, send a signal to the thread to wakeup. It should quit itself.
	 * 2) Try to cancel the thread. This works better on some OS's than others.
	 * 3) Finally, brute force. Issue the inevitable pthread_kill(). This is ugly.
	 *
	 * So we will try it one by one, till one of them works.
	 */
	
	/* Unlock the quit mutex */
	pthread_mutex_unlock(&quit_mutex);
	/* signal thread to wake up */
	pthread_cond_signal(&pending_queue_cond);
	/* Give back the cpu just long enuf to pretty much guarentee the
	 * other thread will run and sense the quit mutex is unlocked.
	 */
#ifndef HAVE_PTHREAD_CANCEL
        sleep(1);
#else
        usleep(50000);
#endif
	if (ptest_thread_dead) {
		pthread_join(ptest_thread, &ptr);
		cleanup_ptest();
		return(PTEST_RETURN_QUIT);
	}
#ifndef HAVE_PTHREAD_CANCEL
	else {
		if (type == PTEST_TYPE_STOP) {
			/* since our OS doesn't seem to support cancel points,
			 * we will simply have to block until the thread sees that
			 * the quit mutex is unlocked.
			 */
			pthread_join(ptest_thread, &ptr);
			cleanup_ptest();
			return(PTEST_RETURN_QUIT);
		}
	}
#else
	/* The thread didn't see us unlock our quit mutex quick
	 * enough, so lets try cancelling the thread instead.
	 */
	pthread_cancel(ptest_thread);
	usleep(50000);
	if (ptest_thread_dead) {
		pthread_join(ptest_thread, &ptr);
		cleanup_ptest();
		return(PTEST_RETURN_CANCEL);
	}
	else {
		if (type == PTEST_TYPE_STOP) {
			/* since we have run out of options to kill the thread when we are
			 * "stopping" the thread, we will simply have to block until the
			 * thread sees that the quit mutex is unlocked or that we  cancelled it.
			 */
			pthread_join(ptest_thread, &ptr);
			cleanup_ptest();
			return(PTEST_RETURN_CANCEL);
		}
	}
#endif
	/* Well, thats the ballgame. None of the "nice" ways worked.
	 * Now we have to get dirty. Lets just kill the thing.
	 */

	/* Fill set with all known signals */
	sigfillset(&set);
	/* Remove the few signals that POSIX says we should */
	sigdelset(&set, SIGFPE);
	sigdelset(&set, SIGILL);
	/* Tell our thread (main) to block against the above signals */
	sigprocmask(SIG_BLOCK, &set, &oldset);
	/* Before we issue the kill, lets check to see if the thread is dead */
	if (ptest_thread_dead) {
		sigprocmask(SIG_BLOCK, &oldset, &set);
		pthread_join(ptest_thread, &ptr);
		cleanup_ptest();
		return(PTEST_RETURN_CANCEL);
	}
	/* Kill the ptest thread, using the SIGQUIT signal */
	pthread_kill(ptest_thread, SIGQUIT);
	/* Put back the previous blocking signals */
	sigprocmask(SIG_BLOCK, &oldset, &set);
	/* Wait until thread dies */
	pthread_join(ptest_thread, NULL);
	/* cleanup anything dealing with the ptest stuff */
	cleanup_ptest();
	return(PTEST_RETURN_KILL);
}

/* Give back memory that we allocated for this entry */
static void free_ptest_entry(PTEST_QUEUE *tmp)
{
	if (tmp->host)
		free(tmp->host);
	if (tmp->alias)
		free(tmp->alias);
	free(tmp);
}

/* init our ptest mutexes */
static void init_ptest_mutexes(void)
{
	pthread_mutex_init(&pending_queue_mutex, NULL);
	pthread_mutex_init(&finished_queue_mutex, NULL);
	pthread_mutex_init(&quit_mutex, NULL);
}

static void kill_ptest_thread(int notused)
{
	ptest_thread_dead = 1;
}

static void kill_ptest_thread2(int notused)
{
	int ecode = 0;
	ptest_thread_dead = 1;
	pthread_exit(&ecode);
}

static void ptest_thread_signal_setup(void)
{
	sigset_t set;

	/* Create SIGQUIT signal handler for this thread */
#if 0
	signal(SIGQUIT, kill_ptest_thread);
#endif
	/* Use ircii's portable implementation of signal() instead */
	my_signal(SIGQUIT, (sigfunc *) kill_ptest_thread, 0);
	/* Fill set with every signal */
	sigfillset(&set);
	/* Remove the SIGQUIT signal from the set */
	sigdelset(&set, SIGQUIT);
	sigdelset(&set, SIGALRM);
	/* Apply the mask on this thread */
	pthread_sigmask(SIG_BLOCK, &set, NULL);
}	

/* Our main function of the ptest thread */
static void *start_ptest_thread(void *args)
{
	PTEST_QUEUE *tmp = NULL;
	/* Set up the thread's signal handlers and mask */
	ptest_thread_signal_setup();
	/* loop */
	while(1) {
		/* Try the quit mutex, if we get it, that means
		 * main() wants us to die.
		 */
		if (pthread_mutex_trylock(&quit_mutex) == 0) {
			kill_ptest_thread2(0);
		}
		/* Lock the queue */
		pthread_mutex_lock(&pending_queue_mutex);
		tmp = ptest_dequeue(&PendingQueueHead, &PendingQueueTail);
		if (!tmp) {
			/*
			 * Give back the cpu, and go to sleep until cond gets signalled.
			 * Note: the mutex MUST be locked, before going into the wait!
			 */
			pthread_cond_wait(&pending_queue_cond, &pending_queue_mutex);
			/* We have been woken up. Thus, 2 conditions are present.
			 * 1) We have been signalled that data is waiting.
			 * 2) The mutex is locked.
			 */
			pthread_mutex_unlock(&pending_queue_mutex);
		}
		else {
			char c = ' ';
			pthread_mutex_unlock(&pending_queue_mutex);
			do_ptest_lookup(tmp);
			/* block until we get the lock */
			pthread_mutex_lock(&finished_queue_mutex);
			ptest_enqueue(&FinishedQueueHead, &FinishedQueueTail, tmp);
			pthread_mutex_unlock(&finished_queue_mutex);
			/* Write to our pipe, this will wake up mains select loop */
			write(ptest_write, &c, 1);
		}
	}
}

/* Does the actual ptest...   */
static void do_ptest_lookup(PTEST_QUEUE *tmp)
{
	int fd, ret = -1;
	struct sockaddr_in server;

	bzero(&server, sizeof(server));

	/* MUST be an IP, hostnames are bad  (too much delay resolving) */
	if ((server.sin_addr.s_addr = inet_addr(tmp->host)) == -1) {
		tmp->result = -1;
		tmp->r_errno = 0;
		return;
	}
	else
		server.sin_family = AF_INET;


	server.sin_port = (unsigned short) htons(tmp->port);

	fd = socket(AF_INET, SOCK_STREAM, 0);

	if (fd < 0) {
		tmp->result = -1;
		tmp->r_errno = 0;
		return;
	}


	/* Try and see if we get connect, or refused */
	alarm(get_int_var(PTEST_CONNECT_TIMEOUT_VAR));
	ret = connect(fd, (struct sockaddr *) &server, sizeof(server));
	alarm(0);

	if (ret == 0) {
		tmp->result = 0;
		tmp->r_errno = 0;
	}
	else {
		tmp->result = -1;
		tmp->r_errno = errno;
	}

	close(fd);
}

/* dequeue an entry from the passed in queue */
PTEST_QUEUE *
ptest_dequeue(PTEST_QUEUE **headp, PTEST_QUEUE **tailp)
{
	PTEST_QUEUE *tmp = NULL;

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
PTEST_QUEUE *
ptest_enqueue(PTEST_QUEUE **headp, PTEST_QUEUE **tailp, PTEST_QUEUE *tmp)
{
	Q_NEXT(tmp) = NULL;
	if (*headp == NULL)
		*headp = *tailp = tmp;
	else {
		Q_NEXT(*tailp) = tmp;
		*tailp = tmp;
	}
	return NULL;
}

/* enqueue a request onto the passed in queue, putting it at the front
 * of the queue. This means it will be the next requested dequeue.
 */
PTEST_QUEUE *
ptest_enqueue_urgent(PTEST_QUEUE **headp, PTEST_QUEUE **tailp, PTEST_QUEUE *tmp)
{
	Q_NEXT(tmp) = *headp;
	*headp = tmp;
	if (*tailp == NULL)
		*tailp = tmp;
	return NULL;
}

/* build a ptest entry struct */
static PTEST_QUEUE *
build_ptest_entry(char *host, int port, void (*callback) (int, int, void *), char *cmd, void *data)
{
	PTEST_QUEUE *tmp = (PTEST_QUEUE *) malloc(sizeof(PTEST_QUEUE));
	bzero(tmp, sizeof(PTEST_QUEUE));
	tmp->host = (char *) malloc(strlen(host) + 1);
	strcpy(tmp->host, host);
	tmp->port = port;
	tmp->callback = callback;
	tmp->callinfo = data;
	if (cmd)
		tmp->alias = strdup(cmd);
	return tmp;
}

/* cleanup_ptest : cleanup anything regarding the ptest thread */
static void cleanup_ptest(void)
{
	pthread_mutex_destroy(&pending_queue_mutex);
	pthread_mutex_destroy(&finished_queue_mutex);
	pthread_mutex_destroy(&quit_mutex);
	pthread_cond_destroy(&pending_queue_cond);
	destroy_ptest_queue(&PendingQueueHead, &PendingQueueTail);
	destroy_ptest_queue(&FinishedQueueHead, &FinishedQueueTail);
#ifdef TEKNAP
	new_close(ptest_read);
	new_close(ptest_write);
#else
	close(ptest_read);
	close(ptest_write);
#endif
	ptest_read = ptest_write = -1;
	ptest_thread = 0;
}

/* destroy_ptest_queue : Walks the queue, blowing away each node */
static void destroy_ptest_queue(PTEST_QUEUE **QueueHead, PTEST_QUEUE **QueueTail)
{
	PTEST_QUEUE *tmp;
	
	while((tmp = ptest_dequeue(QueueHead, QueueTail)) != NULL)
		free_ptest_entry(tmp);
}

void free_pt_info(PT_INFO *tmp)
{
	if (!tmp)
		return;
	if (tmp->from)
		new_free(&tmp->from);
	if (tmp->to)
		new_free(&tmp->to);
	if (tmp->ip)
		new_free(&tmp->ip);
	new_free(&tmp);
}

void ptest_checkport(int result, int r_errno, void *data)
{
	PT_INFO *tmp = (PT_INFO *) data;
	/* if there was no data, we got an error, it should always be there */
	if (!tmp)
		return;

	if (result == 0) {
		say("(%s) %d Our connect() worked.", tmp->ip, tmp->port);
	}
	else {
		say("(%s) %d unreachable. Our connect() -> %d (%s).",
		   tmp->ip, tmp->port, r_errno, strerror(r_errno));
	}
	free_pt_info(tmp);
}

void do_ptestcall(char *host, char *port, char *cmd)
{
PT_INFO *tmp;
	tmp = (PT_INFO *) new_malloc(sizeof(PT_INFO));
	tmp->to = tmp->from = tmp->ip = NULL;
	malloc_strcpy(&tmp->ip, host);
	tmp->port = my_atol(port);
	add_to_ptest_queue(host, tmp->port, ptest_checkport, cmd, tmp, PTEST_NORMAL);
}

#endif /* THREAD  && WANT_PTEST */

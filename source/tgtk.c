/************
 *  tgtk.c  *
 ************
 *
 * A threaded GTK message pump, hopefully...
 *
 * This file will create a thread, set gtk up,
 * create a message pump for our other threads to send
 * messages to, and finally enter into the gtk_main loop.
 *
 * Written by Scott H Kilau
 *
 * Copyright(c) 2000
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 * $Id: tgtk.c,v 1.1.1.1 2000/09/09 06:21:58 edwards Exp $ 
 */

#include "teknap.h"
#include "tgtk.h"
#include "struct.h"
#include "scott2.h"	/* For scott2_start() */
#include "newio.h"	/* for new_open() */

#if defined(WANT_THREAD) && defined(GTK)

Server *server_list;
extern int from_server;

/* Our static globals */
static pthread_t tgtk_thread;
static int ecode = 0;
static int _tgtk_okay = -1;
static int _tgtk_failed = -1;
static int _someone_listening = 0;
static int _sent_gtk_okay_message = 0;
static int tgtk_pipe[2] = { -1, -1 };
static int tgtk_pipe2[2] = { -1, -1 };

static void kill_tgtk_callback(void *);
static void *start_tgtk_thread(void *);
static void cleanup_tgtk(void);
static void tgtk_get_message_in_gtk_thread(gpointer, gint, GdkInputCondition);


/*
 * start_tgtk : This should be called by the main app thread,
 * whenever it wants to start up the gtk stuff.
 */
void start_tgtk(void)
{
	/* Can we start up gtk? */
	if (!gtk_init_check(0, NULL)) {
		_tgtk_okay = 0;
		_tgtk_failed = 1;
		return;
	}

	/* create our ptest thread */
	pthread_create(&tgtk_thread, NULL, start_tgtk_thread, NULL);
}

/* 
 * stop_tgtk : This should be called by the main app thread,
 * whenever it wants to stop the gtk stuff.
 */
void stop_tgtk(void)
{
	/* Stop and Kill are done the same way */
	kill_tgtk();
}

/* 
 * kill_tgtk : This should be called by the main app thread,
 * whenever it wants to stop the gtk stuff.
 */
void kill_tgtk(void)
{
	void *ptr;
        tgtk_send_message_to_gtk_thread(kill_tgtk_callback, NULL);
	pthread_join(tgtk_thread, &ptr);
}

/* make gtk go away */
static void kill_tgtk_callback(void *data)
{
	/* Cleanup, and quit */
	gtk_main_quit();
	cleanup_tgtk();
}

/* Send message to pipe, telling whoever is listening, that
 * gtk is up and running.
 */
static void send_message_that_gtk_is_up(void)
{
	tgtk_send_message_from_gtk_thread(tgtk_ready_callback, NULL);
}


/* Our main function of the ptest thread */
static void *start_tgtk_thread(void *args)
{
	/* create our pipes, if we can */
	if (pipe(tgtk_pipe)) {
		/* pipe failed, might as well kill gtk thread */
		kill_tgtk_callback(NULL);
		return NULL;
	}
	if (pipe(tgtk_pipe2)) {
		/* pipe failed, might as well kill gtk thread */
		kill_tgtk_callback(NULL);
		return NULL;
	}
	/* Register our new fds with new_open(). BTW, new_open implies
	 * that it is going to do an open(), it won't. It just adjusts
	 * our "max_fd" var, so that it will scan our fd
	 */
	new_open(tgtk_pipe[0]);
	new_open(tgtk_pipe[1]);
	new_open(tgtk_pipe2[0]);
	new_open(tgtk_pipe2[1]);

	/* create a callback in gtk for input on this pipe 1[0] */
	gdk_input_add(tgtk_pipe[0], GDK_INPUT_READ, tgtk_get_message_in_gtk_thread, NULL);
	/* Set flag telling us that gtk and our pipes are up and running */
	_tgtk_okay = 1;
	/* Send message to any listening select() loop that everything is up */
	if (_someone_listening) {
		send_message_that_gtk_is_up();
		_sent_gtk_okay_message = 1;
	}
	/* Run the gtk message pump */
	gtk_main();
	return &ecode;
}

/* cleanup_tgtk : cleanup anything regarding the gtk thread */
static void cleanup_tgtk(void)
{
	if (tgtk_pipe[0] >= 0)
		close(tgtk_pipe[0]);
	if (tgtk_pipe[1] >= 0)
		close(tgtk_pipe[1]);
	if (tgtk_pipe2[0] >= 0)
		close(tgtk_pipe2[0]);
	if (tgtk_pipe2[1] >= 0)
		close(tgtk_pipe2[1]);
	tgtk_pipe[0] = tgtk_pipe[1] = tgtk_pipe2[0] = tgtk_pipe2[1] = -1;
	/* Since we are cleaning up after kill the thread, set okay var to 0 */
	_tgtk_okay = 0;
}

/* tgtk_okay : A check to make sure that GTK is all up and running.
 * If it returns TRUE, you know that:
 * 1) The GTK thread is running.
 * 2) The setting up GTK does to make sure we can display the window
 *    (local or remote) is done. (If its remote, this setting up could
 *    take 25-30 secs or longer, thus its important to use this function
 *    before assuming that you can do anything with GTK!
 * 3) The pipes were created okay.
 * 4) The GTK thread is sleeping in its message pump.
 */
int tgtk_okay(void)
{
	if (_tgtk_okay == 1)
		return TRUE;
	return FALSE;
}

/* tgtk_failed : This will return whether our attempt to start GTK
 * failed or not.
 * Returns TRUE if its failed, FALSE if GTK is running, or hasn't been
 * attempted yet.
 */
int tgtk_failed(void)
{
	if (_tgtk_failed == 1)
		return TRUE;
	return FALSE;
}

/*
 * tgtk_send_message_to_gtk_thread : Send an arbitrary message with a callback,
 * plus a pointer to any data it might want. This could be called by ALL threads,
 * except the gtk thread!
 * Returns TRUE if message was sent, returns FALSE if it wasn't sent.
 */
int tgtk_send_message_to_gtk_thread(void (*callback)(void *), void *data)
{
	MY_GTK_MESSAGE message;

	if (!tgtk_okay())
		return FALSE;
	/* Build the send struct */
	message.callback = callback;
	message.data = data;
	/* Send message to tgtk thread thru pipe */
	write(tgtk_pipe[1], &message, sizeof(message));
	return TRUE;
}

/*
 * tgtk_get_message_in_gtk_thread : This will receive a message IN our
 * gtk thread, and run its callback function.
 * Returns TRUE if message was dispatched, or FALSE on error.
 */
static void tgtk_get_message_in_gtk_thread(gpointer data, gint source, GdkInputCondition condition)
{
	MY_GTK_MESSAGE message;

	if (!tgtk_okay())
		return;
	/* read a full message from the pipe. This should never block,
	 * as the user should be using our functions to send thru
	 * the pipe, so we can guarentee the full packet is there.
	 */
	read(source, &message, sizeof(message));
	/* Do the callback, sending it the callback data */
	message.callback(message.data);
}

/*
 * tgtk_send_message_from_gtk_thread : Send an arbitrary message from gtk thread
 * to any thread listening to our pipe, probably will be main's select() loop.
 * Input should be a callback, plus a pointer to any data it might want.
 */
void tgtk_send_message_from_gtk_thread(void (*callback)(void *), void *data)
{
	MY_GTK_MESSAGE message;

	if (!tgtk_okay())
		return;
	/* Build the send struct */
	message.callback = callback;
	message.data = data;
	/* Send message to tgtk pipe */
	write(tgtk_pipe2[1], &message, sizeof(message));
}

/* This should be called by any thread that was listening for something
 * coming in from the gtk thread's pipe. Probably main's select() loop.
 * Note: This should ONLY be called, after getting the fd triggered in
 * some select() loop. Do NOT call this randomly, as the read will block,
 * unless a message really was sent to it!
 */
void tgtk_get_message_from_gtk_thread(void)
{
	MY_GTK_MESSAGE message;

	if (!tgtk_okay())
		return;
	/* read a full message from the pipe */
	read(tgtk_pipe2[0], &message, sizeof(message));
	/* Do the callback */
	message.callback(message.data);
}


/* tgtk_set_output_fd : Use this function to add the
 * tgtk's output fd into the fd_set array. You will probably
 * use this function right before you go into your threads select() loop.
 */
void tgtk_set_output_fd(fd_set *rd)
{
	/* Check to make sure gtk is running */
	if (!tgtk_okay())
		return;

	/* Check to make sure its 0 or higher. */
        if (tgtk_pipe2[0] >= 0) {
		/* set flag so we know at least SOMEONE is trying to listen */
		_someone_listening = 1;
                FD_SET(tgtk_pipe2[0], rd);
		/* if below is true, main doesn't know that gtk is
		 * all set up, and running. Tell it now.
		 */
		if (!_sent_gtk_okay_message) {
			_sent_gtk_okay_message = 1;
			send_message_that_gtk_is_up();
		}
	}
}

/*
 * tgtk_check : Call this function to see if your select() loop
 * was triggered by the GTK thread. Used in conjunction with above.
 */
void tgtk_check(fd_set *rd)
{
        if (tgtk_pipe2[0] >= 0 && FD_ISSET(tgtk_pipe2[0], rd))
		tgtk_get_message_from_gtk_thread();
}  


/******************** REALLY KINDA EXTERNAL COMMANDS ********************/

/* tgtk_ready_callback : This is called when main has detected that
 * the gtk thread sent it a message telling it that gtk is up and running.
 * This should be used to start up any thing you want to build on top
 * of gtk, for example, scott2.c wants this, so it can build its window
 * and any special items it needs.
 */
void tgtk_ready_callback(void *data)
{
	/* Start scott2 */
	scott2_start();
	/* Here we can add more startup stuff, if we have other windows */
}


#endif /* WANT_THREAD  && GTK */

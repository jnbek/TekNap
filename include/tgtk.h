/*
 * tgtk.h
 */
 /* $Id: tgtk.h,v 1.1.1.1 2000/06/23 05:55:49 edwards Exp $ */
 
#ifndef _TGTK_H_
#define _TGTK_H_

/* We gotta know about the fd_set type, so we gotta include this  */
#include "teknap.h"
#ifdef GTK
#include <gtk/gtk.h>


/**************** FUNCTION PROTOTYPES WITH COMMENTS *******************/


/*
 * start_tgtk : This should be called by the main app thread,
 * whenever it wants to start up the gtk stuff.
 */
void start_tgtk(void);

/*
 * stop_tgtk and kill_tgtk : These should be called by the main app thread,
 * whenever it wants to stop the gtk stuff.
 */
void stop_tgtk(void);
void kill_tgtk(void);

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
int tgtk_okay(void);

/* tgtk_failed : This will return whether our attempt to start GTK
 * failed or not.
 * Returns TRUE if its failed, FALSE if GTK is running, or hasn't been
 * attempted yet.
 */
int tgtk_failed(void);

/*
 * tgtk_send_message_to_gtk_thread : Send an arbitrary message with a callback,
 * plus a pointer to any data it might want. This could be called by ALL threads,
 * except the gtk thread!
 * Returns TRUE if message was sent, returns FALSE if it wasn't sent.
 */
int tgtk_send_message_to_gtk_thread(void (*callback)(void *), void *data);

/*
 * tgtk_send_message_from_gtk_thread : Send an arbitrary message from gtk thread
 * to any thread listening to our pipe, probably will be main's select() loop.
 * Input should be a callback, plus a pointer to any data it might want.
 */
void tgtk_send_message_from_gtk_thread(void (*callback)(void *), void *data);

/* This should be called by any thread that was listening for something
 * coming in from the gtk thread's pipe. Probably main's select() loop.
 * Note: This should ONLY be called, after getting the fd triggered in
 * some select() loop. Do NOT call this randomly, as the read will block,
 * unless a message really was sent to it!
 */
void tgtk_get_message_from_gtk_thread(void);

/* tgtk_set_output_fd : Use this function to add the
 * tgtk's output fd into the fd_set array. You will probably
 * use this function right before you go into your threads select() loop.
 */
void tgtk_set_output_fd(fd_set *rd);

/*
 * tgtk_check : Call this function to see if your select() loop
 * was triggered by the GTK thread. Used in conjunction with above.
 */
void tgtk_check(fd_set *rd);

/******************** REALLY KINDA EXTERNAL COMMANDS ********************/


/* tgtk_ready_callback : This is called when main has detected that
 * the gtk thread sent it a message telling it that gtk is up and running.
 * This should be used to start up any thing you want to build on top
 * of gtk, for example, scott2.c wants this, so it can build its window
 * and any special items it needs.
 */
void tgtk_ready_callback(void *data);


/****************** STRUCTURES DEFINED *****************/


/* Use this prototype for ANY message you plan to send thru either of the
 * 2 pipes we set up to send/receive messages from/to the GTK thread!
 */
typedef struct {
	/* The callback function we should call */
	void (*callback) (void *);
	/* pointer to data we would like to send to the callback function */
	void *data;
} MY_GTK_MESSAGE;

#endif
#endif


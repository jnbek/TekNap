/**************
 *  scott2.c  *
 **************
 *
 * A GTK interface to our browse/search lists. Think of it
 * as a combination of my scott.c curses interface, with my
 * tgtk.c interface.
 *
 * Written by Scott H Kilau
 *
 * Copyright(c) 2000
 *
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT
 * $Id: scott2.c,v 1.1.1.1 2000/09/09 06:21:39 edwards Exp $
 */

#include "teknap.h"
#include "scott2.h"
#include "struct.h"
#include "tgtk.h"
#include "napster.h"	/* for base_name() */
#include "output.h"	/* For say() */
#include "server.h"

#include <string.h>

#define WANT_SCOTT2

#if defined(WANT_THREAD) && defined(WANT_SCOTT2) && defined(GTK)

#define BROWSE_TYPE 0
#define SEARCH_TYPE 1
#define MAX_TYPE 1
#define REMOVE_FROM_TREE 1

char *type_array[] = { "Browse", "Search" };

int created = 0;

GtkWidget *window1 = NULL;
GtkWidget *scrolledwindow1 = NULL;
GtkWidget *vbox1 = NULL;
GtkWidget *hbox1 = NULL;
GtkWidget *hbox2 = NULL;
GtkWidget *ctree1 = NULL;
GtkWidget *label1 = NULL;
GtkWidget *label2 = NULL;
GtkWidget *label3 = NULL;
GtkWidget *label4 = NULL;
GtkWidget *label5 = NULL;
GtkWidget *label6 = NULL;
GtkAdjustment *adj1 = NULL;
GtkWidget *frame1 = NULL;
GtkWidget *button1 = NULL;
GtkWidget *button2 = NULL;

Server *server_list;
extern int from_server;

GList *lastnode = NULL;

typedef struct {
	int server;
	int remove;
} SCOTT2_SERVER;

typedef struct {
	int server;
	int type;
	char *nick;
	int remove;
} SCOTT2_NICK;

typedef struct {
	int server;
	int type;
	char *nick;
	char *filename;
	char *bitrate;
	char *filesize;
	char *speed;
	int remove;
} SCOTT2_FILE;


/* Function prototypes */
GtkCTreeNode *get_server_branch_from_tree(int server, int remove);
GtkCTreeNode *get_type_branch_from_tree(GtkCTreeNode *parent, int type, int remove);
GtkCTreeNode *get_nick_branch_from_tree(GtkCTreeNode *parent, char *nick, int remove);
GtkCTreeNode *get_directory_branch_from_tree(GtkCTreeNode *parent, char *directory, int remove);
GtkCTreeNode *get_file_node_from_tree(GtkCTreeNode *parent, char *file, int remove);
void remove_node(GtkCTreeNode *widget);



/* delete_event : This is called when the user clicks on the button to close
 * the window.
 */
int delete_main_window(GtkWidget *widget, GdkEvent  *event, gpointer data)
{
	/* I don't really want to delete the window, lets just "hide" it */
	gtk_widget_hide(window1);
	return(TRUE);
}

/* Determines the nick in the row we are in */
char *get_current_nick(GList *list, char **buf1)
{
	GtkCTreeRow *tmp = GTK_CTREE_ROW(list);
	GList *nodes[3] = { lastnode, NULL, NULL };
	char buf2[BIG_BUFFER_SIZE] = "";
	gchar *nptr;

	while(tmp)
	{

		if(tmp->parent)
		{
			nodes[2] = nodes[1];
			nodes[1] = nodes[0];
			nodes[0] = (GList *)tmp->parent;

			gtk_ctree_get_node_info(GTK_CTREE(ctree1), (GtkCTreeNode *)tmp->parent, &nptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL);
			sprintf(buf2, "%s\\%s", nptr, *buf1);
			strcpy(*buf1, buf2);

			tmp = GTK_CTREE_ROW(tmp->parent);
		}
		else
			tmp = NULL;
	}
	if(nodes[2])
	{
		int z, count = 0;
		gtk_ctree_get_node_info(GTK_CTREE(ctree1), (GtkCTreeNode *)nodes[2], &nptr, NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		/* Ok this is messy... */
		strcpy(buf2, *buf1);
		for(z=0;z<strlen(buf2);z++)
		{
			if(buf2[z] == '\\')
			{
				count++;
				if(count == 3)
					strcpy(*buf1, &buf2[z+1]);
			}
		}
		return nptr;
	}
	return NULL;
}

/* download_selection : This is called when the user clicks on the "download" button */
int download_selection(GtkWidget *widget, GdkEvent  *event, gpointer data)
{
	if(lastnode)
	{
		GtkCTreeNode *start = GTK_CTREE_NODE(lastnode);
		gchar *nstr = NULL;
		FileStruct *file = new_malloc(sizeof(FileStruct));
		char *buf1 = new_malloc(BIG_BUFFER_SIZE);

		gtk_ctree_node_get_text(GTK_CTREE(ctree1), start, 2, &nstr);
		file->bitrate = my_atol(nstr);
		gtk_ctree_node_get_text(GTK_CTREE(ctree1), start, 3, &nstr);
		file->filesize = my_atol(nstr);
		file->checksum = m_strdup("00000");
		nstr = get_current_nick(lastnode, &buf1);
		if(nstr)
		{
			file->nick = m_strdup(nstr);
			gtk_ctree_node_get_text(GTK_CTREE(ctree1), start, 1, &nstr);
			strcat(buf1, nstr);
			file->filename = m_strdup(buf1);

			create_and_do_get(file, 0, 1);

			new_free(&file->nick);
			new_free(&file->filename);
		}

		new_free(&file->checksum);
		new_free(&file);
		new_free(&buf1);
	}

	return(TRUE);
}

/* Determines how far into the list we are */
int check_depth(GList *list)
{
	GtkCTreeRow *tmp = GTK_CTREE_ROW(list);

	int counter = 0;
	while(tmp)
	{
		counter++;
		if(tmp->parent)
			tmp = GTK_CTREE_ROW(tmp->parent);
		else
			tmp = NULL;
	}
	return counter;
}

/* remove_select : This is called when the user clicks on the "remove" button */
int remove_selection(GtkWidget *widget, GdkEvent  *event, gpointer data)
{
	if(lastnode)
	{
		/* Check to make sure we aren't removing anything important :) */
		if(check_depth(lastnode) > 2)
			remove_node((GtkCTreeNode *)lastnode);
	}
	return(TRUE);
}

/* This is called, when the window really goes away. Used for cleanup reasons */
void destroy(GtkWidget *widget, gpointer data)
{
	/* Empty, maybe eventually we could do a created = 0 */
}

/* When a row gets selected we save the list, so we can handle the next request */
void tree_select_row(GtkCTree *ctree, GList *node, gint column, gpointer data)
{
	lastnode = node;
}

void start_callback(void *data)
{
	/* If the window is already up, don't let it do it again */
	if (created == 1)
		return;

	/* Create base window, and connect some basic signals */
	window1 = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW (window1), "Sheiker Browser 0.2");
	gtk_object_set_data (GTK_OBJECT (window1), "window1", window1);
	gtk_signal_connect(GTK_OBJECT (window1), "delete_event", GTK_SIGNAL_FUNC (delete_main_window), NULL);
	gtk_signal_connect (GTK_OBJECT (window1), "destroy", GTK_SIGNAL_FUNC (destroy), NULL);

	/* create a vbox, to start packing from top -> bottom */
	vbox1 = gtk_vbox_new(FALSE, 0);
	gtk_widget_show (vbox1);
	gtk_container_add(GTK_CONTAINER (window1), vbox1);

	/* Create the scroll window, and pack it into the vbox */
	scrolledwindow1 = gtk_scrolled_window_new (NULL, NULL);
	gtk_widget_ref(scrolledwindow1);
	gtk_object_set_data_full(GTK_OBJECT (window1), "scrolledwindow1", scrolledwindow1,
	   (GtkDestroyNotify) gtk_widget_unref);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow1),
	   GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	gtk_container_set_border_width (GTK_CONTAINER (scrolledwindow1), 5);
	gtk_widget_show (scrolledwindow1);
	gtk_box_pack_start (GTK_BOX (vbox1), scrolledwindow1, TRUE, TRUE, 0);

	/* Create our ctree, and put in into the scroll window */
	ctree1 = gtk_ctree_new (6, 0);
	gtk_widget_ref (ctree1);
	gtk_object_set_data_full (GTK_OBJECT (window1), "ctree1", ctree1,
	   (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (ctree1);
	gtk_container_add (GTK_CONTAINER (scrolledwindow1), ctree1);

	/* Set some parameters to make ctree look better */
	gtk_clist_set_column_auto_resize (GTK_CLIST (ctree1), 0, TRUE);
	gtk_clist_set_column_width (GTK_CLIST (ctree1), 1, 320);
	gtk_clist_set_selection_mode (GTK_CLIST (ctree1), GTK_SELECTION_EXTENDED);
#if 0
	gtk_ctree_set_line_style (GTK_CLIST(ctree1), GTK_CTREE_LINES_DOTTED);  
#endif
	gtk_clist_set_column_width (GTK_CLIST (ctree1), 2, 40);
	gtk_clist_set_column_width (GTK_CLIST (ctree1), 3, 80);
	gtk_clist_set_column_width (GTK_CLIST (ctree1), 4, 50);
	gtk_clist_set_column_width (GTK_CLIST (ctree1), 5, 80);
	gtk_clist_column_titles_show (GTK_CLIST (ctree1));

	/* Create a bunch of labels, and put it into the ctree */
	label1 = gtk_label_new ("Tree");
	gtk_widget_ref (label1);
	gtk_object_set_data_full (GTK_OBJECT (window1), "label1", label1,
	   (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label1);
	gtk_clist_set_column_widget (GTK_CLIST (ctree1), 0, label1);

	label2 = gtk_label_new ("Filename");
	gtk_widget_ref (label2);
	gtk_object_set_data_full (GTK_OBJECT (window1), "label2", label2,
	   (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label2);
	gtk_clist_set_column_widget (GTK_CLIST (ctree1), 1, label2);

	label3 = gtk_label_new ("Bitrate");
	gtk_widget_ref (label3);
	gtk_object_set_data_full (GTK_OBJECT (window1), "label3", label3,
	   (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label3);
	gtk_clist_set_column_widget (GTK_CLIST (ctree1), 2, label3);

	label4 = gtk_label_new ("Filesize");
	gtk_widget_ref (label4);
	gtk_object_set_data_full (GTK_OBJECT (window1), "label4", label4,
	   (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label4);
	gtk_clist_set_column_widget (GTK_CLIST (ctree1), 3, label4);

	label5 = gtk_label_new ("Speed");
	gtk_widget_ref (label5);
	gtk_object_set_data_full (GTK_OBJECT (window1), "label5", label5,
	   (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label5);
	gtk_clist_set_column_widget (GTK_CLIST (ctree1), 4, label5);

	label6 = gtk_label_new ("Status");
	gtk_widget_ref (label6);
	gtk_object_set_data_full (GTK_OBJECT (window1), "label6", label6,
	   (GtkDestroyNotify) gtk_widget_unref);
	gtk_widget_show (label6);
	gtk_clist_set_column_widget (GTK_CLIST (ctree1), 5, label6);

	gtk_widget_set_usize (GTK_WIDGET (ctree1), 0, 200);

	hbox1 = gtk_hbox_new (FALSE, 5);
	gtk_container_set_border_width (GTK_CONTAINER (hbox1), 5);
	gtk_widget_show(hbox1);

	gtk_box_pack_start (GTK_BOX (vbox1), hbox1, FALSE, TRUE, 0);

	button1 = gtk_button_new_with_label ("Download Selection");
	gtk_box_pack_start (GTK_BOX (hbox1), button1, TRUE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (button1), "clicked", GTK_SIGNAL_FUNC (download_selection), ctree1); 
	gtk_widget_show(button1);

	button2 = gtk_button_new_with_label ("Remove Selection");
	gtk_box_pack_start (GTK_BOX (hbox1), button2, TRUE, TRUE, 0);
	gtk_signal_connect (GTK_OBJECT (button2), "clicked", GTK_SIGNAL_FUNC (remove_selection), ctree1); 
	gtk_widget_show(button2);

	gtk_signal_connect(GTK_OBJECT(ctree1), "tree-select-row", GTK_SIGNAL_FUNC(tree_select_row), ctree1);

	created = 1;

}

void kill_callback(void *data)
{
	/* Empty for now */
}

void scott2_start(void)
{
	tgtk_send_message_to_gtk_thread(start_callback, NULL);
}

/* returns true if our window has been built, so we could send it things to add/remove */
int scott2_okay(void)
{
	if (created)
		return TRUE;
	return FALSE;
}

static void show_on_callback(void *data)
{
	gtk_widget_set_uposition(GTK_WIDGET (window1), 20, 20);
	gtk_widget_set_usize(GTK_WIDGET (window1), 660, 300);
	gtk_widget_show (window1);
}

static void show_off_callback(void *data)
{
	gtk_widget_hide(window1);
}


void scott2_show(int show)
{
	if (tgtk_okay() && created) {
		if (show == 1)
			tgtk_send_message_to_gtk_thread(show_on_callback, NULL);
		else
			tgtk_send_message_to_gtk_thread(show_off_callback, NULL);
	}
	else
		say("GTK is not ready yet... Please wait a bit and try again");
}

void scott2_kill(void)
{
	tgtk_send_message_to_gtk_thread(kill_callback, NULL);
}



void do_add_server(int server)
{
	gchar *entry[6];
	gchar nserv[100];
	GtkCTreeNode *tnode = NULL, *tnode2 = NULL;

	entry[1] = entry[2] = entry[3] = entry[4] = entry[5] = "";
	sprintf(nserv, "%s", get_server_name(server));
	entry[0] = nserv;

	tnode = gtk_ctree_insert_node(GTK_CTREE(ctree1), NULL, NULL,
	   entry, 6, NULL, NULL, NULL, NULL, 0, 0);

	/* Now create the auto-types, "browse" and "search" */
	entry[0] = type_array[BROWSE_TYPE];
	tnode2 = gtk_ctree_insert_node(GTK_CTREE(ctree1), tnode, NULL,
	   entry, 6, NULL, NULL, NULL, NULL, 0, 0);
	entry[0] = type_array[SEARCH_TYPE];
	tnode2 = gtk_ctree_insert_node(GTK_CTREE(ctree1), tnode, NULL,
	   entry, 6, NULL, NULL, NULL, NULL, 0, 0);
}


void scott2_server_callback(void *data)
{
	SCOTT2_SERVER *tmp = (SCOTT2_SERVER *) data;
	GtkCTreeNode *tnode = get_server_branch_from_tree(tmp->server, !REMOVE_FROM_TREE);

	if (tmp->remove == REMOVE_FROM_TREE) {
		get_server_branch_from_tree(tmp->server, REMOVE_FROM_TREE);
	}
	else {
		/* Is it already added, if so, go home. */
		if (tnode)
			return;
		do_add_server(tmp->server);
	}
	/* Blow away the memory */
	free(tmp);
}

void scott2_add_server(int server)
{
	SCOTT2_SERVER *tmp = (SCOTT2_SERVER *) malloc(sizeof(SCOTT2_SERVER));
	bzero(tmp, sizeof(SCOTT2_SERVER));
	tmp->server = server;
	tmp->remove = !REMOVE_FROM_TREE;
	tgtk_send_message_to_gtk_thread(scott2_server_callback, tmp);
}

void scott2_remove_server(int server)
{
	SCOTT2_SERVER *tmp = (SCOTT2_SERVER *) malloc(sizeof(SCOTT2_SERVER));
	bzero(tmp, sizeof(SCOTT2_SERVER));
	tmp->server = server;
	tmp->remove = REMOVE_FROM_TREE;
	tgtk_send_message_to_gtk_thread(scott2_server_callback, tmp);
}


void cleanup_nick_struct(SCOTT2_NICK *tmp)
{
	if (tmp->nick)
		free(tmp->nick);
	free(tmp);
}

void do_add_nick(GtkCTreeNode *parent, char *nick)
{
	gchar *entry[6];
	GtkCTreeNode *tnode = NULL;

	entry[1] = entry[2] = entry[3] = entry[4] = entry[5] = "";
	entry[0] = nick;

	tnode = gtk_ctree_insert_node(GTK_CTREE(ctree1), parent, NULL,
	   entry, 6, NULL, NULL, NULL, NULL, 0, 0);
}


void scott2_nick_callback(void *data)
{
	GtkCTreeNode *parent = NULL, *parent2 = NULL;
	SCOTT2_NICK *tmp = (SCOTT2_NICK *) data;

	parent = get_server_branch_from_tree(tmp->server, !REMOVE_FROM_TREE);
	if (!parent) {
		if (tmp->remove == REMOVE_FROM_TREE) {
			cleanup_nick_struct(tmp);
			return;
		}
		else
			do_add_server(tmp->server);
		parent = get_server_branch_from_tree(tmp->server, !REMOVE_FROM_TREE);
	}
	/* Okay, got the server branch, now get type branch */
	parent2 = get_type_branch_from_tree(parent, tmp->type, !REMOVE_FROM_TREE);
	if (!parent2) {
		/* big time error, the type should always be there */
		g_print("%s:%d error: Parent is null!\n", __FILE__, __LINE__);
		cleanup_nick_struct(tmp);
		return;
	}
	if (tmp->remove == REMOVE_FROM_TREE) {
		parent = get_nick_branch_from_tree(parent2, tmp->nick, REMOVE_FROM_TREE);
		cleanup_nick_struct(tmp);
		return;
	}
	else
		parent = get_nick_branch_from_tree(parent2, tmp->nick, !REMOVE_FROM_TREE);
	if (parent) {
		/* No need to readd, cleanup and go home */
		cleanup_nick_struct(tmp);
		return;
	}
	do_add_nick(parent2, tmp->nick);

	/* Blow away the memory */
	cleanup_nick_struct(tmp);
}

void scott2_add_nick(int server, char *nick)
{
	SCOTT2_NICK *tmp = (SCOTT2_NICK *) malloc(sizeof(SCOTT2_NICK));
	bzero(tmp, sizeof(SCOTT2_NICK));
	tmp->nick = (char *) malloc(strlen(nick) + 1);
	strcpy(tmp->nick, nick);
	tmp->type = BROWSE_TYPE;
	tmp->server = server;
	tmp->remove = !REMOVE_FROM_TREE;
	tgtk_send_message_to_gtk_thread(scott2_nick_callback, tmp);
}

void scott2_remove_nick(int server, char *nick)
{
	SCOTT2_NICK *tmp = (SCOTT2_NICK *) malloc(sizeof(SCOTT2_NICK));
	bzero(tmp, sizeof(SCOTT2_NICK));
	tmp->nick = (char *) malloc(strlen(nick) + 1);
	strcpy(tmp->nick, nick);
	tmp->type = BROWSE_TYPE;
	tmp->server = server;
	tmp->remove = REMOVE_FROM_TREE;
	tgtk_send_message_to_gtk_thread(scott2_nick_callback, tmp);
}

void do_add_directory(GtkCTreeNode *parent, char *path)
{
	gchar *entry[6];
	GtkCTreeNode *tnode = NULL;

	entry[1] = entry[2] = entry[3] = entry[4] = entry[5] = "";
	entry[0] = path;
	tnode = gtk_ctree_insert_node(GTK_CTREE(ctree1), parent, NULL,
	   entry, 6, NULL, NULL, NULL, NULL, 0, 0);
}

void do_add_file(GtkCTreeNode *parent, SCOTT2_FILE *tmp)
{
	gchar *entry[6];
	GtkCTreeNode *tnode = NULL;

	entry[0] = "";
	entry[1] = base_name(tmp->filename);
	entry[2] = tmp->bitrate;
	entry[3] = tmp->filesize;
	entry[4] = tmp->speed;
	entry[5] = "Available";

	tnode = gtk_ctree_insert_node(GTK_CTREE(ctree1), parent, NULL,
	   entry, 6, NULL, NULL, NULL, NULL, 0, 0);
}


void cleanup_file_struct(SCOTT2_FILE *tmp)
{
	if (tmp->nick)
		free(tmp->nick);
	if (tmp->filename)
		free(tmp->filename);
	if (tmp->bitrate)
		free(tmp->bitrate);
	if (tmp->filesize)
		free(tmp->filesize);
	if (tmp->speed)
		free(tmp->speed);
	free(tmp);
}

void scott2_file_callback(void *data)
{
	GtkCTreeNode *parent = NULL, *parent2 = NULL;
	GtkCTreeNode *tmp_parent = NULL, *tmp_parent2 = NULL;
	SCOTT2_FILE *tmp = (SCOTT2_FILE *) data;
	char *ptr, *ptr2;
	char *backup = NULL, *backup2 = NULL;

	parent = get_server_branch_from_tree(tmp->server, !REMOVE_FROM_TREE);
	if (!parent) {
		if (tmp->remove == REMOVE_FROM_TREE) {
			cleanup_file_struct(tmp);
			return;
		}
		else
			do_add_server(tmp->server);
		parent = get_server_branch_from_tree(tmp->server, !REMOVE_FROM_TREE);
	}
	/* Okay, got the server branch, now get type branch */
	parent = get_type_branch_from_tree(parent, tmp->type, !REMOVE_FROM_TREE);
	if (!parent) {
		/* big time error, the type should always be there */
		g_print("%s:%d error: Parent is null!\n", __FILE__, __LINE__);
		cleanup_file_struct(tmp);
		return;
	}
	/* Okay, got the type branch, find the nick */
	parent2 = get_nick_branch_from_tree(parent, tmp->nick, !REMOVE_FROM_TREE);
	if (!parent2) {
		if (tmp->remove == REMOVE_FROM_TREE) {
			cleanup_file_struct(tmp);
			return;
		}
		else {
			/* add the nick, since its not there now */
			do_add_nick(parent, tmp->nick);
			parent2 = get_nick_branch_from_tree(parent, tmp->nick, !REMOVE_FROM_TREE);
		}
	}
	/* Okay, we got the nick, lets add file to nick */

	tmp_parent = parent2;
	malloc_strcpy(&backup, tmp->filename);
	backup2 = backup;
	ptr = backup;
	ptr2 = index(ptr, '\\');
	if (!ptr2)
		ptr2 = index(ptr, '/');

	while(1) {
		/* If !ptr2, we are at the base name now */
		if (!ptr2) {
			if (tmp->remove == REMOVE_FROM_TREE) {
				get_file_node_from_tree(tmp_parent, base_name(tmp->filename), REMOVE_FROM_TREE);
				break;
			}
			if (!get_file_node_from_tree(tmp_parent, base_name(tmp->filename), !REMOVE_FROM_TREE))
				do_add_file(tmp_parent, tmp);
			break;
		}
		else {
			/* We got a path here, dig it out */
			*ptr2 = '\0';
			ptr2++;
			tmp_parent2 = get_directory_branch_from_tree(tmp_parent, ptr, !REMOVE_FROM_TREE);
			if (!tmp_parent2) {
				do_add_directory(tmp_parent, ptr);
				tmp_parent2 = get_directory_branch_from_tree(tmp_parent, ptr, !REMOVE_FROM_TREE);
			}
			tmp_parent = tmp_parent2;
			ptr = ptr2;
			ptr2 = index(ptr, '\\');
			if (!ptr2)
				ptr2 = index(ptr, '/');
		}

	}
	cleanup_file_struct(tmp);
	free(backup2);
}


void scott2_add_search_file(int server, char *nick, char *filename, int filesize, int bitrate, char *speed)
{
	char buff[100];
	SCOTT2_FILE *tmp = (SCOTT2_FILE *) malloc(sizeof(SCOTT2_FILE));
	bzero(tmp, sizeof(SCOTT2_FILE));
	tmp->nick = (char *) malloc(strlen(nick) + 1);
	strcpy(tmp->nick, nick);
	tmp->filename = (char *) malloc(strlen(filename) + 1);
	strcpy(tmp->filename, filename);
	sprintf(buff, "%d", filesize);
	tmp->filesize = (char *) malloc(strlen(buff) + 1);
	strcpy(tmp->filesize, buff);
	sprintf(buff, "%d", bitrate);
	tmp->bitrate = (char *) malloc(strlen(buff) + 1);
	strcpy(tmp->bitrate, buff);
	tmp->speed = (char *) malloc(strlen(speed) + 1);
	strcpy(tmp->speed, speed);
	tmp->type = SEARCH_TYPE;
	tmp->server = server;
	tmp->remove = !REMOVE_FROM_TREE;
	tgtk_send_message_to_gtk_thread(scott2_file_callback, tmp);
}

void scott2_add_file(int server, char *nick, char *filename, int filesize, int bitrate, char *speed)
{
	char buff[100];
	SCOTT2_FILE *tmp = (SCOTT2_FILE *) malloc(sizeof(SCOTT2_FILE));
	bzero(tmp, sizeof(SCOTT2_FILE));
	tmp->nick = (char *) malloc(strlen(nick) + 1);
	strcpy(tmp->nick, nick);
	tmp->filename = (char *) malloc(strlen(filename) + 1);
	strcpy(tmp->filename, filename);
	sprintf(buff, "%d", filesize);
	tmp->filesize = (char *) malloc(strlen(buff) + 1);
	strcpy(tmp->filesize, buff);
	sprintf(buff, "%d", bitrate);
	tmp->bitrate = (char *) malloc(strlen(buff) + 1);
	strcpy(tmp->bitrate, buff);
	tmp->speed = (char *) malloc(strlen(speed) + 1);
	strcpy(tmp->speed, speed);
	tmp->type = BROWSE_TYPE;
	tmp->server = server;
	tmp->remove = !REMOVE_FROM_TREE;
	tgtk_send_message_to_gtk_thread(scott2_file_callback, tmp);
}


void scott2_remove_file_for_nick(int server, char *nick, char *filename, int filesize, int bitrate)
{
	char buff[100];
	SCOTT2_FILE *tmp = (SCOTT2_FILE *) malloc(sizeof(SCOTT2_FILE));
	bzero(tmp, sizeof(SCOTT2_FILE));
	tmp->nick = (char *) malloc(strlen(nick) + 1);
	strcpy(tmp->nick, nick);
	tmp->filename = (char *) malloc(strlen(filename) + 1);
	strcpy(tmp->filename, filename);
	sprintf(buff, "%d", filesize);
	tmp->filesize = (char *) malloc(strlen(buff) + 1);
	strcpy(tmp->filesize, buff);
	sprintf(buff, "%d", bitrate);
	tmp->bitrate = (char *) malloc(strlen(buff) + 1);
	strcpy(tmp->bitrate, buff);
	tmp->type = BROWSE_TYPE;
	tmp->server = server;
	tmp->remove = REMOVE_FROM_TREE;
	tgtk_send_message_to_gtk_thread(scott2_file_callback, tmp);
}

/* Shamelessly ripped from gtktest.c */
void remove_node(GtkCTreeNode *widget)
{
	GtkCList *clist;
	GtkCTreeNode *node;

	clist = GTK_CLIST (ctree1);

	if (widget) {
		GtkCTreeNode *start = NULL;
		gtk_ctree_remove_node (GTK_CTREE(ctree1), widget);
		start = gtk_ctree_node_nth(GTK_CTREE(ctree1), 0);
		if (!start) {
			return;
		}		
	}
	else {
		gtk_clist_freeze(clist);
		while (clist->selection) {
			node = clist->selection->data;
			if (GTK_CTREE_ROW (node)->is_leaf) {
				/* pages-- */
			}
			else {
				/*
				gtk_ctree_post_recursive (ctree1, node,
				   (GtkCTreeFunc) count_items, NULL);
				*/
			}
			gtk_ctree_remove_node (GTK_CTREE(ctree1), node);
			if (clist->selection_mode == GTK_SELECTION_BROWSE)
				break;
		}
	}
	if (clist->selection_mode == GTK_SELECTION_EXTENDED && !clist->selection &&
	   clist->focus_row >= 0) {
		node = gtk_ctree_node_nth (GTK_CTREE(ctree1), clist->focus_row);
		if (node)
			gtk_ctree_select (GTK_CTREE(ctree1), node);
	}
	gtk_clist_thaw (clist);
#if 0
	after_press (GTK_CTREE(ctree1), NULL);
#endif
}


/* get_server_branch_from_tree : Use this to find the specific server branch
 * that you are looking for.
 * Returns NULL if not found, or the actual Node that it starts at.
 */
GtkCTreeNode *
get_server_branch_from_tree(int server, int remove)
{
	GtkCTreeNode *start = NULL, *finish = NULL;
	char serv[100];
	int i = 0;

	sprintf(serv, "%s", get_server_name(server));

	/* Server is at the base of the tree */
	start = gtk_ctree_node_nth(GTK_CTREE(ctree1), i);
	finish = gtk_ctree_last(GTK_CTREE(ctree1), NULL);

	if (!start)
		return NULL;
		
	while(1) {
		gchar *text;

		/* For our "tree" info, we gotta do this */
		gtk_ctree_get_node_info(GTK_CTREE(ctree1), start, &text,
		   NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		if (text && !my_stricmp(text, serv)) {
			if (remove == REMOVE_FROM_TREE) {
				remove_node(start);
				return NULL;
			}
			else
				return start;
		}
		/* Did we hit the end */
		if (start == finish)
			break;
		i++;
		start = gtk_ctree_node_nth(GTK_CTREE(ctree1), i);
		if (!start)
			return NULL;
	}	
	return NULL;
}

/* get_type_branch_from_tree : Use this to find the specific browse/search branch
 * that you are looking for.
 * Returns NULL if not found, or the actual Node that it starts at.
 */
GtkCTreeNode *
get_type_branch_from_tree(GtkCTreeNode *parent, int type, int remove)
{

	GtkCTreeNode *start = NULL;
	gchar *tptr;

	/* Check to see if type is bogus */
	if (type > MAX_TYPE)
		return NULL;

	/* Define the string we are looking for */
	tptr = type_array[type];

	if(!parent)
		return NULL;

	/* Start search at parent's children */
	start = GTK_CTREE_ROW (parent)->children;

	while(1) {
		gchar *text;

		/* For our "tree" info, we gotta do this */
		gtk_ctree_get_node_info(GTK_CTREE(ctree1), start, &text,
		   NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		if (text && !my_stricmp(text, tptr)) {
			if (remove == REMOVE_FROM_TREE) {
				remove_node(start);
				return NULL;
			}
			else
				return start;
		}
		/* Move to next link */
		start = GTK_CTREE_ROW (start)->sibling;
		/* Did we run outta links? */
		if (!start)
			return NULL;
	}	
	return NULL;
}

/* get_nick_branch_from_tree : Use this to find the specific nick branch
 * that you are looking for.
 * Returns NULL if not found, or the actual Node that it starts at.
 */
GtkCTreeNode *
get_nick_branch_from_tree(GtkCTreeNode *parent, char *nick, int remove)
{

	GtkCTreeNode *start = NULL;

	/* Start search at parent's children */
	start = GTK_CTREE_ROW (parent)->children;
	/* Parent might not have any children yet! */
	if (!start)
		return NULL;
	while(1) {
		gchar *text;

		/* For our "tree" info, we gotta do this */
		gtk_ctree_get_node_info(GTK_CTREE(ctree1), start, &text,
		   NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		if (text && !my_stricmp(text, nick)) {
			if (remove == REMOVE_FROM_TREE) {
				remove_node(start);
				return NULL;
			}
			else
				return start;
		}
		/* Move to next link */
		start = GTK_CTREE_ROW (start)->sibling;
		/* Did we run outta links? */
		if (!start)
			return NULL;
	}	
	return NULL;
}

/* get_directory_branch_from_tree : Use this to find the specific directory branch
 * that you are looking for.
 * Returns NULL if not found, or the actual Node that it starts at.
 */
GtkCTreeNode *
get_directory_branch_from_tree(GtkCTreeNode *parent, char *directory, int remove)
{
	GtkCTreeNode *start = NULL;

	/* Start search at parent's children */
	start = GTK_CTREE_ROW (parent)->children;
	/* Parent might not have any children yet! */
	if (!start)
		return NULL;
	while(1) {
		gchar *text;

		/* For our "tree" info, we gotta do this */
		gtk_ctree_get_node_info(GTK_CTREE(ctree1), start, &text,
		   NULL, NULL, NULL, NULL, NULL, NULL, NULL);

		if (text && !my_stricmp(text, directory)) {
			if (remove == REMOVE_FROM_TREE) {
				remove_node(start);
				return NULL;
			}
			else
				return start;
		}
		/* Move to next link */
		start = GTK_CTREE_ROW (start)->sibling;
		/* Did we run outta links? */
		if (!start)
			return NULL;
	}	
	return NULL;
}

/* get_file_node_from_tree : Use this to find the specific file node
 * that you are looking for.
 * Returns NULL if not found, or the actual Node that it starts at.
 */
GtkCTreeNode *
get_file_node_from_tree(GtkCTreeNode *parent, char *file, int remove)
{
	GtkCTreeNode *start = NULL;

	/* Start search at parent's children */
	start = GTK_CTREE_ROW (parent)->children;
	/* Parent might not have any children yet! */
	if (!start)
		return NULL;
	while(1) {
		gchar *nstr = NULL;
		gtk_ctree_node_get_text(GTK_CTREE(ctree1), start, 1, &nstr);
		if (file && !my_stricmp(file, nstr)) {
			if (remove == REMOVE_FROM_TREE) {
				remove_node(start);
				return NULL;
			}
			else
				return start;
		}
		/* Move to next link */
		start = GTK_CTREE_ROW (start)->sibling;
		/* Did we run outta links? */
		if (!start)
			return NULL;
	}	
	return NULL;
}


#endif /* WANT_THREAD  && WANT_GTK */

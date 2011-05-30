/* $Id: scottpm.c,v 1.1.1.1 2000/09/25 05:55:57 edwards Exp $ */
  
#include <dw.h>

static int gui_started = 0;
HWND mainwindow, container;
ULONG musicicon = 0;

Server *server_list;

typedef struct _file_list {
	char *filename;
	char *nick;
	ULONG size;
	char *speed;
	ULONG bitrate;
	struct _file_list *next;
} FileList;

FileList *root = NULL;

#ifdef __EMX__
HAB hab = 0L;
HMQ hmq = 0L;

static pthread_t pm_thread;
#endif

void start_gui(void);

/* Find the index of the file that the user has chosen */
FileList *findlist(char *text)
{
	FileList *tmp;

	tmp = root;
	while(tmp)
	{
		if(strcmp(tmp->filename, text)==0)
			return tmp;
		tmp=tmp->next;
	}
	return NULL;
}

/* Configures the columns of the queue listing on the right */
void setcontainer(void)
{
	char *titles[4] = { "Nick", "Size", "Bitrate", "Speed" };
	unsigned long flags[4] = {  DW_CFA_STRING | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR,
	DW_CFA_ULONG | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR,
	DW_CFA_ULONG | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR,
	DW_CFA_STRING | DW_CFA_LEFT | DW_CFA_HORZSEPARATOR | DW_CFA_SEPARATOR };

	if(!dw_filesystem_setup(container, flags, titles, 4))
		dw_messagebox("TekNap", "Error Creating Container!");
}

/* Inserts a list of items into the queue list */
void update_browser(void)
{
	FileList *tmp;
	int count = 0, z;
	void *containerinfo;

	tmp = root;

	start_gui();

	/* Count the number of files */
	while(tmp)
	{
		count++;
		tmp=tmp->next;
	}
	tmp = root;

	dw_container_clear(container);

	containerinfo = dw_container_alloc(container, count);

	/* Insert the entries into the container */
	for(z=0;z<count;z++)
	{
		dw_filesystem_set_file(container, containerinfo, z, tmp->filename, musicicon);

		dw_filesystem_set_item(container, containerinfo, 0, z, &tmp->nick);
		dw_filesystem_set_item(container, containerinfo, 1, z, &tmp->size);
		dw_filesystem_set_item(container, containerinfo, 2, z, &tmp->bitrate);
		dw_filesystem_set_item(container, containerinfo, 3, z, &tmp->speed);

		dw_container_set_row_title(containerinfo, z, tmp->filename);
		tmp=tmp->next;
	}

	dw_container_insert(container, containerinfo, count);
}

int download_callback(HWND window, void *data)
{
	char *buffer = dw_container_query_start(container, TRUE);

	while(buffer)
	{
		FileStruct *file = new_malloc(sizeof(FileStruct));
		FileList *thislist = findlist(buffer);

		if(thislist)
		{
			file->nick = thislist->nick;
			file->filesize = thislist->size;
			file->bitrate = thislist->bitrate;
			file->checksum = m_strdup("00000");
			file->filename = thislist->filename;

			create_and_do_get(file, 0, TRUE);

			new_free(&file->checksum);
		}
		new_free(&file);

		buffer = dw_container_query_next(container, TRUE);
	}

	/* Return -1 to allow the default handlers to return. */
	return -1;
}

int remove_callback(HWND window, void *data)
{
	FileList *tmp = root;

	/* Clear the list */
	dw_container_clear(container);

	/* Free the memory of the list */
	root = NULL;

	while(tmp)
	{
		FileList *prev = tmp;
		tmp = tmp->next;

		free(prev->filename);
		free(prev->speed);
		free(prev->nick);
		free(prev);
	}

	/* Return -1 to allow the default handlers to return. */
	return -1;
}

void start_gui_thread(void *param)
{
	HWND lbbox, buttonbox, downloadbutton, removebutton;
	ULONG flStyle = DW_FCF_TITLEBAR | DW_FCF_SIZEBORDER | DW_FCF_MINMAX |
		DW_FCF_SHELLPOSITION | DW_FCF_TASKLIST | DW_FCF_DLGBORDER;

	dw_init(TRUE);

	mainwindow = dw_window_new(HWND_DESKTOP, "Sheiker Browser 0.2", flStyle);

	lbbox = dw_box_new(BOXVERT, 10);

	dw_box_pack_start(mainwindow, lbbox, 0, 0, TRUE, TRUE, 0);

	container = dw_container_new(100L);

	dw_container_set_view(container, DW_CV_DETAIL | DW_CV_MINI | DW_CA_DETAILSVIEWTITLES, 20, 20);
	dw_window_set_style(container, DW_CCS_EXTENDSEL, DW_CCS_SINGLESEL | DW_CCS_EXTENDSEL);

	setcontainer();

	dw_box_pack_start(lbbox, container, 130, 200, TRUE, TRUE, 10);

	buttonbox = dw_box_new(BOXHORZ, 0);

	dw_box_pack_start(lbbox, buttonbox, 0, 0, TRUE, TRUE, 0);

	downloadbutton = dw_button_new("Download Selection", 1001L);

	dw_box_pack_start(buttonbox, downloadbutton, 50, 30, TRUE, TRUE, 5);

	removebutton = dw_button_new("Clear List", 1002L);

	dw_box_pack_start(buttonbox, removebutton, 50, 30, TRUE, TRUE, 5);

	dw_signal_connect(downloadbutton, "clicked", DW_SIGNAL_FUNC(download_callback), (void *)mainwindow);
	dw_signal_connect(removebutton, "clicked", DW_SIGNAL_FUNC(remove_callback), (void *)mainwindow);

	dw_window_set_usize(mainwindow, 600, 350);

	musicicon = dw_icon_load(0, 100);

	gui_started = 1;

	dw_main(0, NULL);
}

void start_gui(void)
{
	if(!gui_started)
	{
#ifdef __EMX__
		PPIB pib = NULL;

		DosGetInfoBlocks(NULL,&pib);
		/*oldapptype = pib->pib_ultype;*/
		pib->pib_ultype = 3;

		hab = WinInitialize(0);
		hmq = WinCreateMsgQueue(hab, 0);

		pthread_create(&pm_thread, NULL, (void *)start_gui_thread, NULL);

		while(!gui_started)
			DosSleep(100);
#else
		DWORD tid;

		CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)start_gui_thread,
					 NULL, 0, &tid);

		while(!gui_started)
			usleep(100);

#endif

	}
}

BUILT_IN_COMMAND(scott)
{
	start_gui();

	dw_window_show(mainwindow);
}

void refresh_browser(void)
{
}

void browser_main_loop(void)
{
}

void scott2_add_file(int server, char *nick, char *filename, int filesize, int bitrate, char *speed)
{
	FileList *tmp = root, *newfile;

	if(!root)
		root = newfile = malloc(sizeof(FileList));
	else
	{
		while(tmp->next)
			tmp = tmp->next;
		tmp->next = newfile = malloc(sizeof(FileList));
	}

	newfile->next = NULL;
	newfile->nick = strdup(nick);
	newfile->size = (ULONG)filesize;
	newfile->bitrate = (ULONG)bitrate;
	newfile->speed = strdup(speed);
	newfile->filename = strdup(filename);
}

void scott2_add_search_file(int server, char *nick, char *filename, int filesize, int bitrate, char *speed)
{
	scott2_add_file(server, nick, filename, filesize, bitrate, speed);
}


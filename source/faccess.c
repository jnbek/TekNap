/*
 * files.c -- allows you to read/write files. Wow.
 *
 * (C) 1995 Jeremy Nelson (ESL)
 * Modifed by Colten Edwards 1997
 * $Id: faccess.c,v 1.1.1.1 2001/01/17 21:19:03 edwards Exp $
 */

#include "teknap.h"
#include "ircaux.h"

/* Here's the plan.
 *  You want to open a file.. you can READ it or you can WRITE it.
 *    unix files can be read/written, but its not the way you expect.
 *    so we will only alllow one or the other.  If you try to write to
 *    read only file, it punts, and if you try to read a writable file,
 *    you get a null.
 *
 * New functions: open(FILENAME <type>)
 *			<type> is 0 for read, 1 for write, 0 is default.
 *			Returns fd of opened file, -1 on error
 *		  read (fd)
 *			Returns line for given fd, as long as fd is
 *			opened via the open() call, -1 on error
 *		  write (fd text)
 *			Writes the text to the file pointed to by fd.
 *			Returns the number of bytes written, -1 on error
 *		  close (fd)
 *			closes file for given fd
 *			Returns 0 on OK, -1 on error
 *		  eof (fd)
 *			Returns 1 if fd is at EOF, 0 if not. -1 on error
 */

struct FILE___ {
	FILE *file;
	char *name;
	int mode;
	int socket;
	struct FILE___ *next;
};
extern int new_close(int);
extern int new_open(int);

typedef struct FILE___ File;

static File *FtopEntry = NULL;

static File *new_file (void)
{
	File *tmp = FtopEntry;
	File *tmpfile = (File *)new_malloc(sizeof(File));

	if (FtopEntry == NULL)
		FtopEntry = tmpfile;
	else
	{
		while (tmp->next)
			tmp = tmp->next;
		tmp->next = tmpfile;
	}
	return tmpfile;
}

static void _remove_file_ (File *file)
{
	File *tmp = FtopEntry;

	if (file == FtopEntry)
		FtopEntry = file->next;
	else
	{
		while (tmp->next && tmp->next != file)
			tmp = tmp->next;
		if (tmp->next)
			tmp->next = tmp->next->next;
	}
	if (file->socket != -1)
		new_close(file->socket);
	else
		fclose(file->file);
	new_free(&file->name);
	new_free((char **)&file);
}

int open_internet_socket(char *hostname, int port, int type)
{
	unsigned short portnum = port;
	int ret;
	ret = connect_by_number(hostname, &portnum, type, PROTOCOL_TCP, 1);
	if (ret > -1)
	{
		File *nfs = new_file();
		nfs->name = m_strdup(hostname);
		nfs->mode = portnum;
		nfs->socket = ret;
		new_open(ret);
		return ret;
	}
	return -1;
}
	
int open_file_for_read (char *filename, int mode)
{
	char *dummy_filename = NULL;
	FILE *file;

	malloc_strcpy(&dummy_filename, filename);
	file = uzfopen(&dummy_filename, ".", 0, NULL);
	new_free(&dummy_filename);
	if (file)
	{
		File *nfs = new_file();
		nfs->file = file;
		nfs->next = NULL;
		nfs->mode = mode;
		nfs->name = m_strdup(filename);
		nfs->socket = -1;
		return fileno(file);
	}
	else
		return -1;
}

int open_file_for_write (char *filename, int mode)
{
	/* patch by Scott H Kilau so expand_twiddle works */
	char *expand = NULL;
	FILE *file;

	if (!(expand = expand_twiddle(filename)))
		malloc_strcpy(&expand, filename);
	file = fopen(expand, "a");
	new_free(&expand);
	if (file)
	{
		File *nfs = new_file();
		nfs->file = file;
		nfs->next = NULL;
		nfs->mode = mode;
		nfs->name = m_strdup(filename);
		nfs->socket = -1;
		return fileno(file);
	}
	else 
		return -1;
}

static File *lookup_file (int fd)
{
	File *ptr = FtopEntry;

	while (ptr)
	{
		if (((ptr->socket != -1) && (fd == ptr->socket)) || (fileno(ptr->file) == fd))
			return ptr;
		else
			ptr = ptr -> next;
	}
	return NULL;
}

int file_write (int fd, char *stuff, int len)
{
	File *ptr = lookup_file(fd);
	int ret;
	if (!ptr)
		return -1;
	if (ptr->socket != -1)
		ret = write(ptr->socket, stuff, len);
	else if (ptr->mode == O_BINARY)
		ret = write(fileno(ptr->file), stuff, len);
	else
	{
		ret = fprintf(ptr->file, "%s\n", stuff);
		if (fflush(ptr->file) == EOF)
			return -1;
	}
	return ret;
}

char *file_read (int fd)
{
	File *ptr = lookup_file(fd);
	if (!ptr)
		return m_strdup(empty_string);
	else
	{
		char blah[10240];
		char *m = NULL;
		int rc;
		if (ptr->socket != -1)
		{
			rc = read(ptr->socket, blah, 10239);
			if (rc != -1)
			{
				m = new_malloc(rc);
				memcpy(m, blah, rc);
			}
			if (!m)
				m = m_strdup(empty_string);
			return m;
		}
		else if (ptr->mode == O_BINARY)
		{
			rc = read(fileno(ptr->file), blah, 10239);
			if (rc != -1)
			{
				m = new_malloc(rc);
				memcpy(m, blah, rc);
			}
			if (!m)
				m = m_strdup(empty_string);
			return m;
		}
		else
		{
			if ((fgets(blah, 10239, ptr->file)))
				chop(blah, 1);
			else
				blah[0] = 0;
			return m_strdup(blah);
		}
	}
	return m_strdup(empty_string);
}

int	file_eof (int fd)
{
	File *ptr = lookup_file (fd);
	if (!ptr)
		return -1;
	else if (ptr->socket == -1)
		return feof(ptr->file);
	else
		return 0;
}

int	file_close (int fd)
{
	File *ptr = lookup_file (fd);
	if (!ptr)
		return -1;
	else
		_remove_file_ (ptr);
	return 0;
}

int	file_valid (int fd)
{
	if (lookup_file(fd))
		return 1;
	return 0;
}


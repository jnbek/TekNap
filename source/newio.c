/*
 * newio.c: This is some handy stuff to deal with file descriptors in a way
 * much like stdio's FILE pointers 
 *
 * IMPORTANT NOTE:  If you use the routines here-in, you shouldn't switch to
 * using normal reads() on the descriptors cause that will cause bad things
 * to happen.  If using any of these routines, use them all 
 *
 * Copyright 1990 Michael Sandrof
 * Copyright 1995 Matthew Green
 * Copyright 1997 EPIC Software Labs
 * See the COPYRIGHT file, or do a HELP IRCII COPYRIGHT 
 * $Id: newio.c,v 1.5 2001/10/11 16:27:08 edwards Exp $
 */

#include "teknap.h"
#include "struct.h"
#include "newio.h"
#include "ircaux.h"
#include "output.h"
#include "napster.h"

#include <sys/ioctl.h>
#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#if defined(HAVE_SYSCONF) && defined(_SC_OPEN_MAX) && !defined(__EMX__)
# define IO_ARRAYLEN sysconf(_SC_OPEN_MAX)
#else
# ifdef FD_SETSIZE
#  define IO_ARRAYLEN FD_SETSIZE
# else
#  define IO_ARRAYLEN NFDBITS
# endif
#endif

#if defined(__EMX__) && defined(select)
#undef select
#endif

#define MAX_SEGMENTS 16

#define DCC_PACKETID  0xfeab		/* used to figure out endianess 
					 * as well as identify the resend
					 * packet 
					 */

typedef	struct	myio_struct
{
	char		*buffer;
	unsigned char	databuffer[4];
	int		data_read;
	size_t		buffer_size;
	unsigned 	read_pos,
			write_pos;
	int		segments;
	int		error;
	N_DATA		data;
} MyIO;

typedef	struct	newmyio_struct
{
	char		*buffer;
	int		is_read;
	int		is_write;
	size_t		buffer_size;
	int		port;
	unsigned long	flags;
	char		*server;
	void		(*func_read)(int);
	void		(*func_write)(int);
	void		(*cleanup)(int);
	time_t		timeout;
	void		*info;
	int		error;
} NewMyIO;

static	MyIO	**io_rec = NULL;
/*static	NewMyIO	**new_io_rec = NULL;*/

	int	dgets_errno = 0;

extern	int	set_capability;

/*
 * Get_pending_bytes: What do you think it does?
 */
size_t get_pending_bytes (int fd)
{
	if (fd >= 0 && io_rec[fd] && io_rec[fd]->buffer)
		return strlen(io_rec[fd]->buffer);

	return 0;
}

static	void	init_io (void)
{
	static	int	first = 1;

	if (first)
	{
		int	c, max_fd = IO_ARRAYLEN;

		io_rec = (MyIO **)new_malloc(sizeof(MyIO *) * max_fd);
		for (c = 0; c < max_fd; c++)
			io_rec[c] = NULL;
		first = 0;
	}
}

N_DATA *dget_data(int des, N_DATA *data, int len)
{
	MyIO	*ioe;
	if (des < 0)
		return NULL;
	if (!io_rec)
		init_io();

	ioe = io_rec[des];
	memcpy(data, &ioe->data, len);
	return data;
}
void dget_clear_data(int des)
{
	MyIO	*ioe;
	if (des < 0)
		return;
	if (!io_rec)
		init_io();

	if (!io_rec[des])
		return;
	ioe = io_rec[des];
	memset(&ioe->data, 0, sizeof(N_DATA));
}

/*
 * All new dgets -- no more trap doors!
 *
 * There are at least four ways to look at this function.
 * The most important variable is 'buffer', which determines if
 * we force line buffering.  If it is on, then we will sit on any
 * incomplete lines until they get a newline.  This is the default
 * behavior for server connections, because they *must* be line
 * delineated.  However, when are getting input from an untrusted
 * source (eg, dcc chat, /exec'd process), we cannot assume that every
 * line will be newline delinated.  So in those cases, 'buffer' is 0,
 * and we force a flush on whatever we can slurp, without waiting for
 * a newline.
 *
 * Return values:
 *
 *	-1 -- something really died.  Either a read error occured, the
 *	      fildesc wasnt really ready for reading, or the input buffer
 *	      for the filedesc filled up (8192 bytes)
 *	 0 -- If the data read in from the file descriptor did not form a 
 *	      complete line, then zero is always returned.  This should be
 *	      considered a stopping condition.  Do not call dgets() again
 *	      after it returns 0, because unless more data is avaiable on
 *	      the fd, it will return -1, which you would misinterpret as an
 *	      error condition.
 *	      If "buffer" is 0, then whatever we have available will be 
 *	      returned in "str".
 *	      If "buffer" is not 0, then we will retain whatever we have
 *	      available, waiting for the newline to occur perhaps next time.
 *	>0 -- If a full, newline terminated line was available, the length
 *	      of the line is returned.
 */
int new_dgets(char *str, int des, int buffer, int buffersize)
{
	int nbytes;
	int cnt = 0, c;
	MyIO	*ioe;

	if (!io_rec)
		init_io();

	ioe = io_rec[des];

	if (ioe == NULL)
	{
		ioe = io_rec[des] = (MyIO *)new_malloc(sizeof(MyIO));
		ioe->buffer_size = IO_BUFFER_SIZE;
		ioe->buffer = (char *)new_malloc(ioe->buffer_size + 2);
		ioe->read_pos = ioe->write_pos = 0;
	}
	else if (ioe->read_pos == ioe->write_pos)
	{
		ioe->read_pos = ioe->write_pos = 0;
		ioe->buffer[0] = 0;
		ioe->segments = 0;
	}
	if (!strchr(ioe->buffer + ioe->read_pos, '\n'))
	{
		if (ioe->read_pos)
		{
			ov_strcpy(ioe->buffer, ioe->buffer + ioe->read_pos);
			ioe->read_pos = 0;
			ioe->write_pos = strlen(ioe->buffer);
			ioe->segments = 1;
		}

		/*
		 * Dont try to read into a full buffer.
		 */
		if (ioe->write_pos >= ioe->buffer_size)
		{
			yell("***XXX*** Buffer for des [%d] is filled!", des);
			dgets_errno = ENOMEM; /* Cough */
			return -1;
		}
		/*
		 * Check to see if any bytes are ready.  If this fails,
		 * then its almost always due to the filedesc being 
		 * bogus.  Thats a fatal error.
		 */
		if (ioctl(des, FIONREAD, &nbytes) == -1)
		{
			*str = 0;
			dgets_errno = errno;
			return -1;
		} 

		/*
		 * Check for a quasi-EOF condition.  If we get to this
		 * point, then new_select() indicated that this fd is ready.
		 * The fd is ready if either:
		 *	1) A newline is in the buffer
		 *	2) select(2) returned ready for the fd.
		 *
		 * If 1) is true, then write_pos will not be zero.  So we can
		 * use that as a cheap way to check for #1.  If #1 is false,
		 * then #2 must have been true, and if nbytes is 0, then 
		 * that indicates an EOF condition.
		 */
		else if (!nbytes && ioe->write_pos == 0)
		{
			*str = 0;
			dgets_errno = errno;
			return -1;
		}
		else if (nbytes)
		{
			if (nbytes >= IO_BUFFER_SIZE)
				nbytes = IO_BUFFER_SIZE-1;
			c = read(des, ioe->buffer + ioe->write_pos,
				      ioe->buffer_size - ioe->write_pos - 1);
			if (c <= 0)
			{
				*str = 0;
				dgets_errno = (c == 0) ? -1 : errno;
				return -1;
			}
			ioe->buffer[ioe->write_pos + c] = 0;
			ioe->write_pos += c;
			ioe->segments++;
		}
		else
		{
			/*
			 * At this point nbytes is 0, and it doesnt
			 * appear the socket is at EOF or ready to read.
			 * Very little to do at this point but force the
			 * issue and figure out what the heck went wrong.
			 */
			struct timeval t = { 0, 0 };
			fd_set testing;

			FD_ZERO(&testing);
			FD_SET(des, &testing);
			switch (select(des + 1, &testing, NULL, NULL, &t))
			{
				case -1:
				{
					yell("Aberrant condition for des [%d], closing down the connection out of desperation.", des);
					*str = 0;
					dgets_errno = errno;
					return -1;
				}
				case 0:
				{
					yell("des [%d] passed to dgets(), but it isnt ready.", des);
					if (ioe->write_pos == 0)
					{
						yell("X*X*X*X*X*X*X*X*X ABANDON SHIP! X*X*X*X*X*X*X*X*X*X");
						nappanic("write_pos is zero when it cant be.");
					}
					else
					{
						yell("But something is buffered.  Flushing it to see if that helps.");
						ioe->buffer[ioe->write_pos++] = '\n';
						break;
					}
				}
				case 1:
				{
					errno = ECONNABORTED;
					*str = 0;
					dgets_errno = errno;
					return -1;
				}
			}
		}
	}

	dgets_errno = 0;

	/*
	 * If the caller wants us to force line buffering, and if there
	 * is no complete line, just stop right here.
	 */
	if (!strchr(ioe->buffer + ioe->read_pos, '\n'))
	{
		if (ioe->segments > MAX_SEGMENTS)
		{
			yell("*** Too many read()s on des [%d] without a newline!", des);
			*str = 0;
			dgets_errno = ECONNABORTED;
			return -1;
		}
		return 0;
	}
	/*
	 * Slurp up the data that is available into 'str'. 
	 */
	while (ioe->read_pos < ioe->write_pos)
	{
		if (((str[cnt] = ioe->buffer[ioe->read_pos++])) == '\n')
			break;
		cnt++;
		if (cnt >= buffersize-1)
			break;
	}

	/*
	 * Terminate it
	 */
	str[cnt + 1] = 0;

	/*
	 * If we end in a newline, then all is well.
	 * Otherwise, we're unbuffered, tell the caller.
	 * The caller then would need to do a strlen() to get
 	 * the amount of data.
	 */
	if (str[cnt] == '\n')
		return cnt;
	else
		return 0;
	
}

int dgets (char *str, int des, int buffer, int buffersize)
{
	int	cnt = 0, c;
	MyIO	*ioe;
	int	nbytes;

	if (!io_rec)
		init_io();

	ioe = io_rec[des];

	if (ioe == NULL)
	{
		ioe = io_rec[des] = (MyIO *)new_malloc(sizeof(MyIO));
		ioe->buffer_size = IO_BUFFER_SIZE;
		ioe->buffer = (char *)new_malloc(ioe->buffer_size + 2);
		ioe->read_pos = ioe->write_pos = 0;
	}
	else if (ioe->read_pos == ioe->write_pos)
	{
		ioe->read_pos = ioe->write_pos = 0;
		ioe->buffer[0] = 0;
		ioe->segments = 0;
	}

	if (!ioe->data.len)
	{
		nbytes = 4 - ioe->data_read;
		c = read(des, ioe->databuffer + ioe->data_read, nbytes);
		if (!c)
		{
			dgets_errno = errno;
			return -1;
		}
		if (c && c != nbytes)
		{
			if (c > -1)
				ioe->data_read = c;
			return -2;
		}
		ioe->data_read = 0;

		ioe->data.len = ioe->databuffer[0] + ((ioe->databuffer[1] << 8) & 0xff00);
		ioe->data.command = ioe->databuffer[2] + ((ioe->databuffer[3] << 8) & 0xff00);
		if (set_capability)
		{
			ioe->data.len = ntohs(ioe->data.len);
			ioe->data.command = ntohs(ioe->data.command);
		}
#if 0
		ioe->data.len = BSWAP16(ioe->data.len);
		ioe->data.command = BSWAP16(ioe->data.command);
#endif
	}
	if (ioe->data.len)
	{
		if (ioe->write_pos + ioe->data.len >= ioe->buffer_size)
		{
			yell("***XXX*** Buffer for des [%d] is filled! %s ", des, ioe->buffer);
			dgets_errno = ENOMEM; /* Cough */
			return -1;
		}
		/*
		 * Check to see if any bytes are ready.  If this fails,
		 * then its almost always due to the filedesc being 
		 * bogus.  Thats a fatal error.
		 */
		if (ioctl(des, FIONREAD, &nbytes) == -1)
		{
			*str = 0;
			dgets_errno = errno;
			return -1;
		} 

#if 0
		else if (!nbytes && ioe->write_pos == 0)
		{
			*str = 0;
			dgets_errno = errno;
			return -1;
		}
#endif
		else if (nbytes)
		{
			int len;
			len = ioe->data.len - ioe->write_pos;
			c = read(des, ioe->buffer + ioe->write_pos, len);
			if (c <= 0)
			{
				*str = 0;
				dgets_errno = (c == 0) ? -1 : errno;
				return -1;
			}
			ioe->buffer[ioe->write_pos + c] = 0;
			ioe->write_pos += c;
			ioe->segments++;
		}
		else
		{
			struct timeval t = { 0, 0 };
			fd_set testing;
			/*
			 * At this point nbytes is 0, and it doesnt
			 * appear the socket is at EOF or ready to read.
			 * Very little to do at this point but force the
			 * issue and figure out what the heck went wrong.
			 */

			FD_ZERO(&testing);
			FD_SET(des, &testing);
			switch (select(des + 1, &testing, NULL, NULL, &t))
			{
				case -1:
				{
					yell("Aberrant condition for des [%d], closing down the connection out of desperation.", des);
					*str = 0;
					dgets_errno = errno;
					return -1;
				}
				case 0:
				{
					*str = 0;
					return 0;
				}
				case 1:
				{
					int len;
					len = ioe->data.len - ioe->write_pos;
					c = read(des, ioe->buffer + ioe->write_pos, len);
#if 0
					put_it("len = %d ioe->write_pos = %d read = %d", len, ioe->write_pos, c);
#endif
					if (c <= 0)
					{
						*str = 0;
						dgets_errno = (c == 0) ? -1 : errno;
						return -1;
					}
					ioe->buffer[ioe->write_pos + c] = 0;
					ioe->write_pos += c;
					ioe->segments++;
				}
			}
		}
	}

	dgets_errno = 0;

	/*
	 * If the caller wants us to force line buffering, and if there
	 * is no complete line, just stop right here.
	 */
	if ((ioe->data.len != ioe->write_pos))
	{
		if (ioe->segments > MAX_SEGMENTS)
		{
			yell("*** Too many read()s on des [%d] without a newline!", des);
			*str = 0;
			dgets_errno = ECONNABORTED;
			return -1;
		}
		return 0;
	}
	/*
	 * Slurp up the data that is available into 'str'. 
	 */
	while (ioe->read_pos < ioe->write_pos)
	{
		if (!((str[cnt] = ioe->buffer[ioe->read_pos++])))
			break;
		cnt++;
		if (cnt >= buffersize-1)
			break;
	}

	/*
	 * Terminate it
	 */
	str[cnt] = 0;

	/*
	 * If we end in a newline, then all is well.
	 * Otherwise, we're unbuffered, tell the caller.
	 * The caller then would need to do a strlen() to get
 	 * the amount of data.
	 */
	return cnt;
}

int global_max_fd = -1;

/*
 * new_select: works just like select(), execpt I trimmed out the excess
 * parameters I didn't need.  
 */
int new_select (fd_set *rd, fd_set *wd, struct timeval *timeout)
{
		int	i,
			set = 0;
		fd_set 	new;
static	int polls = 0;

	struct timeval	thetimeout;
	struct timeval *newtimeout = &thetimeout;

	if (timeout)
	{
		thetimeout = *timeout;
		if (timeout->tv_sec == 0)
		{
			if (polls++ > 10000)
				nappanic("Stuck in a polling loop. Help!");
		}
		else
			polls = 0;
	}
	else
		newtimeout = NULL;

	if (!io_rec)
		nappanic("new select called before io_rec init");
	if (newtimeout && newtimeout->tv_usec < 0)
		nappanic("new select with < -1");
		
	FD_ZERO(&new);
	for (i = 0; i <= global_max_fd; i++)
	{
		if (io_rec[i])
		{
			if ((io_rec[i]->read_pos < io_rec[i]->write_pos) &&
				strchr(io_rec[i]->buffer + io_rec[i]->read_pos, '\n'))
			{
				FD_SET(i, &new);
				set++;
			}
		}
	}

	if (set)
	{
		*rd = new;
		return set;
	}
	return (select(global_max_fd + 1, rd, wd, NULL, newtimeout));
}

/*
 * Register a filedesc for readable events
 * Set up its input buffer
 */
int 	new_open (int des)
{
	if (des < 0)
		return des;		/* Invalid */

	if (!io_rec)
		init_io();

	if (!FD_ISSET(des, &readables))
		FD_SET(des, &readables);
	if (des > global_max_fd)
		global_max_fd = des;
		
	return des;
}

int 	new_open_write (int des)
{
#if 0
	if (des < 0)
		return des;		/* Invalid */

	if (!io_rec)
		init_io();

	if (!FD_ISSET(des, &writables))
		FD_SET(des, &writables);
#if 0
	if (des > global_max_fd)
		global_max_fd = des;
#endif		
#endif
	return des;
}


/*
 * Unregister a filedesc for readable events 
 * and close it down and free its input buffer
 */
int	new_close (int des)
{
	if (des < 0)
		return -1;

	if (FD_ISSET(des, &readables))
		FD_CLR(des, &readables);
	if (FD_ISSET(des, &writables))
		FD_CLR(des, &writables);

	if (io_rec && io_rec[des])
	{
	        new_free(&(io_rec[des]->buffer));
        	new_free((char **)&(io_rec[des]));
	}
	close(des);

	/*
	 * If we're closing the highest fd in use, then we
	 * want to adjust global_max_fd downward to the next highest fd.
	 */
	if (des == global_max_fd)
	{
		do
			des--;
		while (des >= 0 && !FD_ISSET(des, &readables));

		global_max_fd = des;
	}
	return -1;
}

int	new_close_write (int des)
{
#if 0
	if (des < 0)
		return -1;

	if (FD_ISSET(des, &writables))
		FD_CLR(des, &writables);

	if (io_rec && io_rec[des])
	{
	        new_free(&(io_rec[des]->buffer));
        	new_free((char **)&(io_rec[des]));
	}
	close(des);

#if 0
	/*
	 * If we're closing the highest fd in use, then we
	 * want to adjust global_max_fd downward to the next highest fd.
	 */
	if (des == global_max_fd)
	{
		do
			des--;
		while (!FD_ISSET(des, &writables));

		global_max_fd = des;
	}
#endif
#endif
	return -1;
}

void set_keepalive(int s)
{
int opt = 1;
	setsockopt(s, SOL_SOCKET, SO_KEEPALIVE, (char *)&opt, sizeof(opt));
}

/* set's socket options */
void set_socket_options (int s)
{
	int	opt = 1;
#ifndef NO_STRUCT_LINGER
	struct linger	lin;

	lin.l_onoff = lin.l_linger = 0;
	setsockopt(s, SOL_SOCKET, SO_LINGER, (char *)&lin, sizeof(struct linger));
#endif

	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt));
	set_keepalive(s);

#if notyet
	/* This is waiting for nonblock-aware code */
	info = fcntl(fd, F_GETFL, 0);
	info |= O_NONBLOCK;
	fcntl(fd, F_SETFL, info);
#endif
}

#if 0
static	void	init_newio (void)
{
	static	int	first = 1;

	if (first)
	{
		int	c, max_fd = IO_ARRAYLEN;

		new_io_rec = (NewMyIO **)new_malloc(sizeof(NewMyIO *) * max_fd);
		for (c = 0; c < max_fd; c++)
			new_io_rec[c] = NULL;
		first = 0;
	}
}

int add_socketread(int s, int port, unsigned long flags, char *server, void (*func_read)(int), void (*func_write)(int))
{
	if (s < 0)
		return s;
	if (s > FD_SETSIZE)
		return -1;

	if (!io_rec)
		init_newio();
	ioe = io_rec[s];

	if (ioe == NULL)
	{
		ioe = io_rec[s] = (NewMyIO *)new_malloc(sizeof(NewMyIO));
		ioe->read = s;
		ioe->port = port;
		ioe->flags = flags;
		if (server)
			ioe->server = m_strdup(server);
		ioe->func_read = func_read;
		ioe->func_write = func_read;
	}
	return s;
}

#endif

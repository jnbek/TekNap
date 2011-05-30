/*
 * xmms control program.
 * Copyright Colten Edwards 2000
 */

#include "teknap.h"
#include "struct.h"
#include "alias.h"
#include "ircaux.h"


#if !defined(WINNT) && !defined(__EMX__) && defined(WANT_XMMS)

typedef struct {
	unsigned short version;
	int length;
} ServerPktHeader;

typedef struct
{
	unsigned short version;
	unsigned short command;
	int length;
} ClientPktHeader;

enum
{
	CMD_GET_VERSION, CMD_PLAYLIST_ADD, CMD_PLAY, CMD_PAUSE, CMD_STOP,
	CMD_IS_PLAYING, CMD_IS_PAUSED, CMD_GET_PLAYLIST_POS,
	CMD_SET_PLAYLIST_POS, CMD_GET_PLAYLIST_LENGTH, CMD_PLAYLIST_CLEAR,
	CMD_GET_OUTPUT_TIME, CMD_JUMP_TO_TIME, CMD_GET_VOLUME,
	CMD_SET_VOLUME, CMD_GET_SKIN, CMD_SET_SKIN, CMD_GET_PLAYLIST_FILE,
	CMD_GET_PLAYLIST_TITLE, CMD_GET_PLAYLIST_TIME, CMD_GET_INFO,
	CMD_GET_EQ_DATA, CMD_SET_EQ_DATA, CMD_PL_WIN_TOGGLE,
	CMD_EQ_WIN_TOGGLE, CMD_SHOW_PREFS_BOX, CMD_TOGGLE_AOT,
	CMD_SHOW_ABOUT_BOX, CMD_EJECT, CMD_PLAYLIST_PREV, CMD_PLAYLIST_NEXT,
	CMD_PING, CMD_GET_BALANCE, CMD_TOGGLE_REPEAT, CMD_TOGGLE_SHUFFLE, CMD_MAIN_WIN_TOGGLE, CMD_PLAYLIST_ADD_URL_STRING,
	CMD_IS_EQ_WIN, CMD_IS_PL_WIN, CMD_IS_MAIN_WIN, CMD_PLAYLIST_DELETE,
	CMD_IS_REPEAT, CMD_IS_SHUFFLE,
	CMD_GET_EQ, CMD_GET_EQ_PREAMP, CMD_GET_EQ_BAND,
	CMD_SET_EQ, CMD_SET_EQ_PREAMP, CMD_SET_EQ_BAND,
	CMD_QUIT
};

#define XMMS_PROTOCOL_VERSION 1

static int remote_cmd(unsigned int);


int connect_to_xmms(void)
{
	DIR *dp = NULL;
	struct dirent *dir;
	char buffer[BIG_BUFFER_SIZE];                
	int ret;
	
	sprintf(buffer, "/tmp");
	if (!(dp = opendir(buffer)))
		return -1;
	while ((dir = readdir(dp)))
	{
		if (!dir->d_ino)
			continue;
		if (!strncmp(dir->d_name, "xmms", 4))
		{
			unsigned short port = 0;
			sprintf(buffer, "/tmp/%s", dir->d_name);
			ret = connect_by_number(buffer, &port, SERVICE_CLIENT, PROTOCOL_TCP, 1);
			if (ret != -1)
			{
				closedir(dp);
				return ret;
			}
		}
	}
	closedir(dp);
	return -1;
}

static void *remote_read_packet(int fd, ServerPktHeader *pkt_hdr)
{
	void *data = NULL;

	if (read(fd, pkt_hdr, sizeof (ServerPktHeader)) == sizeof (ServerPktHeader))
	{
		if (pkt_hdr->length)
		{
			data = (void *)new_malloc(pkt_hdr->length);
			read(fd, data, pkt_hdr->length);
		}
	}
	return data;
}

static void remote_read_ack(int fd)
{
	char *data;
	ServerPktHeader pkt_hdr;

	data = (char *)remote_read_packet(fd, &pkt_hdr);
	if (data != NULL && *data)
		new_free(&data);

}

static void remote_send_packet(int fd, unsigned long command, void * data, unsigned long data_length)
{
	ClientPktHeader pkt_hdr;

	pkt_hdr.version = XMMS_PROTOCOL_VERSION;
	pkt_hdr.command = command;
	pkt_hdr.length = data_length;
	write(fd, &pkt_hdr, sizeof (ClientPktHeader));
	if (data_length)
		write(fd, data, data_length);
}

static void remote_send_unsigned_long( unsigned long cmd, unsigned long val)
{
	int fd;

	if ((fd = connect_to_xmms()) == -1)
		return;
	remote_send_packet(fd, cmd, &val, sizeof (unsigned long));
	remote_read_ack(fd);
	close(fd);
}


static int remote_cmd(unsigned int cmd)
{
	int fd;

	if ((fd = connect_to_xmms()) == -1)
		return FALSE;
	remote_send_packet(fd, cmd, NULL, 0);
	remote_read_ack(fd);
	close(fd);
	return TRUE;
}

static int remote_get_int(int cmd)
{
	ServerPktHeader pkt_hdr;
	int ret = 0;
	int *data;
	int fd;

	if ((fd = connect_to_xmms()) == -1)
		return ret;
	remote_send_packet(fd, cmd, NULL, 0);
	data = (int *)remote_read_packet(fd, &pkt_hdr);
	if (data)
	{
		ret = *data;
		new_free(&data);
	}
	remote_read_ack(fd);
	close(fd);
	return ret;
}

char *remote_get_string(int cmd)
{
	ServerPktHeader pkt_hdr;
	char *data;
	int fd;

	if ((fd = connect_to_xmms()) == -1)
		return NULL;
	remote_send_packet(fd, cmd, NULL, 0);
	data = (char *)remote_read_packet(fd, &pkt_hdr);
	remote_read_ack(fd);
	close(fd);
	return data;
}

char *remote_get_string_pos(int cmd, unsigned int pos)
{
	ServerPktHeader pkt_hdr;
	char *data;
	int fd;

	if ((fd = connect_to_xmms()) == -1)
		return NULL;
	remote_send_packet(fd, cmd, &pos, sizeof (unsigned int));
	data = (char *)remote_read_packet(fd, &pkt_hdr);
	remote_read_ack(fd);
	close(fd);
	return data;
}


char *xmms_remote_get_playlist_title(int pos)
{
	return remote_get_string_pos(CMD_GET_PLAYLIST_TITLE, pos);
}


int xmms_remote_get_output_time(void)
{
	return remote_get_int(CMD_GET_OUTPUT_TIME);
}

void xmms_remote_jump_to_time(int pos)
{
	remote_send_unsigned_long(CMD_JUMP_TO_TIME, pos);
}




void xmms_remote_play(void)
{
	remote_cmd(CMD_PLAY);
}

void xmms_remote_pause(void)
{
	remote_cmd(CMD_PAUSE);
}

void xmms_remote_stop(void)
{
	remote_cmd(CMD_STOP);
}

int xmms_remote_is_running(void)
{
	return remote_cmd(CMD_PING);
}

int xmms_remote_is_paused(void)
{
	return remote_get_int(CMD_IS_PAUSED);
}

void xmms_remote_playlist_prev(void)
{
	remote_cmd(CMD_PLAYLIST_PREV);
}

void xmms_remote_playlist_next(void)
{
	remote_cmd(CMD_PLAYLIST_NEXT);
}

int xmms_remote_get_playlist_pos(void)
{
	return remote_get_int(CMD_GET_PLAYLIST_POS);
}

void xmms_remote_set_playlist_pos(int pos)
{
	remote_send_unsigned_long(CMD_SET_PLAYLIST_POS, pos);
}

int xmms_remote_get_playlist_length(void)
{
	return remote_get_int(CMD_GET_PLAYLIST_LENGTH);
}

void xmms_remote_playlist_clear(void)
{
	remote_cmd(CMD_PLAYLIST_CLEAR);
}

int xmms_remote_get_playlist_time(int pos)
{
	ServerPktHeader pkt_hdr = {0};
	unsigned int *data;
	int fd, ret = 0;
	unsigned int p = pos;

	if ((fd = connect_to_xmms()) == -1)
		return ret;
	remote_send_packet(fd, CMD_GET_PLAYLIST_TIME, &p, sizeof (unsigned int));
	data = (unsigned int *)remote_read_packet(fd, &pkt_hdr);
	if (data)
	{
		ret = *data;
		new_free(&data);
	}
	remote_read_ack(fd);
	close(fd);
	return ret;
}

void xmms_remote_get_info( int * rate, int * freq, int * nch)
{
	ServerPktHeader pkt_hdr;
	int fd;
	void *data;

	if ((fd = connect_to_xmms()) == -1)
		return;
	remote_send_packet(fd, CMD_GET_INFO, NULL, 0);
	data = (int *)remote_read_packet(fd, &pkt_hdr);
	if (data)
	{
		*rate = ((unsigned int *) data)[0];
		*freq = ((unsigned int *) data)[1];
		*nch = ((unsigned int *) data)[2];
		new_free(&data);
	}
	remote_read_ack(fd);
	close(fd);
}


char *song_information(int i)
{
char *ret, *str;

	if (i < 0)
		i = xmms_remote_get_playlist_pos();
	str = xmms_remote_get_playlist_title(i);
	ret = m_sprintf("%d %d \"%s\"", 
		i,
		xmms_remote_get_playlist_time(i) / 1000,
		xmms_remote_get_playlist_title(i));
	new_free(&str); 
	return ret;
}

void xmms_remote_get_volume( int * vl, int * vr)
{
	ServerPktHeader pkt_hdr;
	int fd;
	int *data;

	if ((fd = connect_to_xmms()) == -1)
		return;
	remote_send_packet(fd, CMD_GET_VOLUME, NULL, 0);
	data = (int *)remote_read_packet(fd, &pkt_hdr);
	if (data)
	{
		*vl = ((unsigned int *) data)[0];
		*vr = ((unsigned int *) data)[1];
		new_free(&data);
	}
	remote_read_ack(fd);
	close(fd);
}

int xmms_remote_get_main_volume(void)
{
	int vl, vr;

	xmms_remote_get_volume(&vl, &vr);

	return (vl > vr) ? vl : vr;
}

void xmms_remote_set_volume( int vl, int vr)
{
	int fd;
	unsigned int v[2];

	if (vl < 0)
		vl = 0;
	if (vl > 100)
		vl = 100;
	if (vr < 0)
		vr = 0;
	if (vr > 100)
		vr = 100;

	if ((fd = connect_to_xmms()) == -1)
		return;
	v[0] = vl;
	v[1] = vr;
	remote_send_packet(fd, CMD_SET_VOLUME, v, 2 * sizeof (unsigned int));
	remote_read_ack(fd);
	close(fd);
}

int xmms_remote_get_balance(void)
{
	return remote_get_int(CMD_GET_BALANCE);
}

void xmms_remote_set_main_volume(int v)
{
	int b, vl, vr;

	b = xmms_remote_get_balance();

	if (b < 0)
	{
		vl = v;
		vr = (v * (100 - abs(b))) / 100;
	}
	else if (b > 0)
	{
		vl = (v * (100 - b)) / 100;
		vr = v;
	}
	else
		vl = vr = v;
	xmms_remote_set_volume(vl, vr);
}


BUILT_IN_FUNCTION(function_xmms)
{
char *cmd;
	if (!xmms_remote_is_running())
		RETURN_INT(-1);
		
	{
		cmd = next_arg(input, &input);
		if (!cmd)
			RETURN_EMPTY;
		if (!my_stricmp(cmd, "PLAYLIST_ADD"))
		{
			if (input && *input)
			{
				char **list, *str, *n;
				int total = 0, len, cnt = 0, size = 10;
				list = (char **)new_malloc(size);
				while ((n = new_next_arg(input, &input)))
				{
					if (!n || !*n)
						break;
					if (cnt == size)
					{
						size+=5;
						RESIZE(list, char *, size);
					}
					list[cnt] = n;
					total += (((strlen(list[cnt++]) + 1) + 3) / 4) * 4 + 4;
				}
				if (total)
				{
					int i; char *ptr, fd;
					total += 4;
					str = (char *)new_malloc(total);
					for (i = 0, ptr = str; i < cnt; i++)
					{
						len = strlen(list[i]) + 1;
						*((unsigned long *) ptr) = len;
						ptr += 4;
						memcpy(ptr, list[i], len);
						ptr += ((len + 3) / 4) * 4;
					}
					*((unsigned long *) ptr) = 0;
					if ((fd = connect_to_xmms()) == -1)
						RETURN_INT(-1);
					remote_send_packet(fd, CMD_PLAYLIST_ADD, str, total);
					remote_read_ack(fd);
					close(fd);
					new_free(&str);
				}
				new_free(&list);
			}
		}			
		else if (!my_stricmp(cmd, "PAUSE"))
		{
			if (!xmms_remote_is_paused())
				xmms_remote_pause();
		}
		else if (!my_stricmp(cmd, "NEXT"))
			xmms_remote_playlist_next();
		else if (!my_stricmp(cmd, "PREV"))
			xmms_remote_playlist_prev();
		else if (!my_stricmp(cmd, "PLAY"))
		{
			int pos = 0;
			if (input && *input)
			{
				pos = my_atol(input);			
				xmms_remote_set_playlist_pos(pos);
			}
			xmms_remote_play();
		}
		else if (!my_stricmp(cmd, "STOP"))
			xmms_remote_stop();
		else if (!my_stricmp(cmd, "INFO"))
		{
			char *ret;
			int pos = -1;
			if (input && *input)
				pos = my_atol(input);
			ret = song_information(pos);
			RETURN_MSTR(ret);
		}
		else if (!my_stricmp(cmd, "PLAYLIST"))
		{
			char *tmp, *ret = NULL;
			int i;
			for (i = 0; i < xmms_remote_get_playlist_length(); i++)
			{
				tmp = song_information(i);
				m_s3cat(&ret, " ", tmp);
				new_free(&tmp);
			}
			RETURN_MSTR(ret);
		}
		else if (!my_stricmp(cmd, "VOLUME"))
		{
			int vol;
			if (!input || !*input)
				RETURN_INT(xmms_remote_get_main_volume());
			vol = my_atol(input);
			xmms_remote_set_main_volume(vol);			
		}
	}
	RETURN_INT(0);
}
#else
BUILT_IN_FUNCTION(function_xmms)
{
	RETURN_EMPTY;
}
#endif

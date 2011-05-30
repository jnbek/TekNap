 /* $Id: mp3db.h,v 1.1.1.1 2000/06/22 09:34:55 edwards Exp $ */
 
#ifndef _mp3db_h
#define _mp3db_h

#define M_DIR 0 // directory
#define M_MP3 1 // mp3 file
#define M_BIN 2 // binary file
#define M_SDR 4 // sub directory

#define LABEL_SIZE 30
#define PATH_SIZE 255


typedef struct {
	char	label[LABEL_SIZE+1];
	char	pathname[PATH_SIZE+1];
	unsigned int	bitrate, 
			mode, 
			size;
	time_t	time;
} Tmp3_Entry;

#endif

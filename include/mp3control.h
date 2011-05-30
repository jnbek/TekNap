/*
 * MP3 player control for mpg123 versions with generic mp3 jukebox control.
 * Based heavily on cdns.c from Scott H Kilau.
 * We setup a thread which we then fork a mp3 player in giving us control of
 * stdin/stdout/stderr from the player. We can then cause the player to "LOAD"
 * a mp3 and play it, as well as controlling it's actions with several other
 * commands.
 * Copyright Colten D Edwards July 4th 2000
 * $Id: mp3control.h,v 1.1.1.1 2000/07/09 13:08:40 edwards Exp $
 */

#ifndef _mp3control_h_
#define _mp3control_h_

void mp3_check(fd_set *);
void set_mp3_output_fd(fd_set *rd);
BUILT_IN_COMMAND(mp3play);
void start_mp3(void);
void kill_mp3(void);

#endif

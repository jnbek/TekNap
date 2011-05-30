 /* $Id: scott2.h,v 1.1.1.1 2000/06/22 09:35:00 edwards Exp $ */
 
/*
 * scott2.h
 */

#ifndef _SCOTT2_H_
#define _SCOTT2_H_

void scott2_start(void);
void scott2_kill(void);
void scott2_show(int);
int scott2_okay(void);

void scott2_add_server(int server);
void scott2_remove_server(int server);

void scott2_add_nick(int server, char *nick);
void scott2_remove_nick(int server, char *nick);

void scott2_add_file(int server, char *nick, char *filename, int filesize, int bitrate, char *speed);
void scott2_remove_file(int server, char *nick, char *filename);

void scott2_add_search_file(int server, char *nick, char *filename, int filesize, int bitrate, char *speed);

#endif

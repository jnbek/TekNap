
 /* $Id: whois.h,v 1.1.1.1 2000/11/22 22:14:54 edwards Exp $ */
 
#ifndef _WHOIS_H_
#define _WHOIS_H_

void who_result (int, char *);
void whowas_result(int, char *);
void whobase(char *args, int);
void dnsbase(char *args);
void who_remove_queue(int, char *);

#endif

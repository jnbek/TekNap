/*
 * network.c -- handles stuff dealing with connecting and name resolving
 *
 * Written by Jeremy Nelson in 1995
 * See the COPYRIGHT file or do /help ircii copyright
 * $Id: network.c,v 1.2 2001/07/08 21:33:55 edwards Exp $
 */
#define SET_SOURCE_SOCKET

#include "teknap.h"
#include "struct.h"
#include "cdns.h"
#include "ircterm.h"
#include "newio.h"
#include "ircaux.h"
#include "output.h"
#include "vars.h"

#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif
#ifdef PARANOID
/* NaiL^d0d: no hijack please, we need random bytes, in stdlib.h */
#include <stdlib.h>
#endif

#if 0
#include <netinet/ip_icmp.h>
#endif

#ifdef WINNT
#define NON_BLOCKING_CONNECTS
#define NBLOCK_SYSV
#endif

extern char *LocalHostName;
extern struct sockaddr_in LocalHostAddr;

/*
 * connect_by_number:  Wheeeee. Yet another monster function i get to fix
 * for the sake of it being inadequate for extension.
 *
 * we now take four arguments:
 *
 *	- hostname - name of the host (pathname) to connect to (if applicable)
 *	- portnum - port number to connect to or listen on (0 if you dont care)
 *	- service -	0 - set up a listening socket
 *			1 - set up a connecting socket
 *	- protocol - 	0 - use the TCP protocol
 *			1 - use the UDP protocol
 *			2 - use ICMP protocol
 *
 *
 * Returns:
 *	Non-negative number -- new file descriptor ready for use
 *	-1 -- could not open a new file descriptor or 
 *		an illegal value for the protocol was specified
 *	-2 -- call to bind() failed
 *	-3 -- call to listen() failed.
 *	-4 -- call to connect() failed
 *	-5 -- call to getsockname() failed
 *	-6 -- the name of the host could not be resolved
 *	-7 -- illegal or unsupported request
 *	-8 -- no socks access
 *
 *
 * Credit: I couldnt have put this together without the help of BSD4.4-lite
 * User SupplimenTary Document #20 (Inter-process Communications tutorial)
 */
int connect_by_number(char *hostn, unsigned short *portnum, int service, int protocol, int nonblocking)
{
	int 	fd = -1,
	 	is_unix = (hostn && *hostn == '/'),
	 	sock_type, 
		proto_type,
		sock_opts = 0;
	
	sock_type = (is_unix) ? AF_UNIX : AF_INET;
	proto_type = (protocol == PROTOCOL_TCP) ? SOCK_STREAM : SOCK_DGRAM;

	if ((fd = socket(sock_type, proto_type, sock_opts)) <= 0)
		return -1;

	set_socket_options (fd);

	/* Unix domain server */
#ifdef HAVE_SYS_UN_H
	if (is_unix)
	{
		struct sockaddr_un name;

		memset(&name, 0, sizeof(struct sockaddr_un));
		name.sun_family = AF_UNIX;
		strcpy(name.sun_path, hostn);
#ifdef HAVE_SUN_LEN
# ifdef SUN_LEN
		name.sun_len = SUN_LEN(&name);
# else
		name.sun_len = strlen(hostn) + 1;
# endif
#endif

		if (is_unix && (service == SERVICE_SERVER))
		{
			if (bind(fd, (struct sockaddr *)&name, strlen(name.sun_path) + sizeof(name.sun_family)))
				return close(fd), -2;
			if (protocol == PROTOCOL_TCP)
				if (listen(fd, 4) < 0)
					return close(fd), -3;
		}

		/* Unix domain client */
		else if (service == SERVICE_CLIENT)
		{
			alarm(get_int_var(CONNECT_TIMEOUT_VAR));
			if (connect (fd, (struct sockaddr *)&name, strlen(name.sun_path) + 2) < 0)
			{
				alarm(0);
				return close(fd), -4;
			}
			alarm(0);
		}
	}
	else
#endif

	/* Inet domain server */
	if (!is_unix && (service == SERVICE_SERVER))
	{
		int length;
		int backlog = get_int_var(SEND_LIMIT_VAR);
		struct sockaddr_in name;

		memset(&name, 0, sizeof(struct sockaddr_in));
		name.sin_family = AF_INET;
		name.sin_addr.s_addr = htonl(INADDR_ANY);
		name.sin_port = htons(*portnum);
		
		if (bind(fd, (struct sockaddr *)&name, sizeof(name)))
			return close(fd), -2;

		length = sizeof (name);
		if (getsockname(fd, (struct sockaddr *)&name, &length))
			return close(fd), -5;
		*portnum = ntohs(name.sin_port);

		if (backlog < 10)
			backlog = 10;
		else
			backlog += 5;
		if (protocol == PROTOCOL_TCP)
			if (listen(fd, backlog) < 0)
				return close(fd), -3;
#ifdef NON_BLOCKING_CONNECTS
		if (nonblocking && set_non_blocking(fd) < 0)
			return close(fd), -4;
#endif
	}

	/* Inet domain client */
	else if (!is_unix && (service == SERVICE_CLIENT))
	{
		struct sockaddr_in server;
		struct hostent *hp;

		struct sockaddr_in localaddr;
		if (LocalHostName)
		{
			memset(&localaddr, 0, sizeof(struct sockaddr_in));
			localaddr.sin_family = AF_INET;
			localaddr.sin_addr = LocalHostAddr.sin_addr;
			localaddr.sin_port = 0;
			if (bind(fd, (struct sockaddr *)&localaddr, sizeof(localaddr)))
				return close(fd), -2;
		}

		memset(&server, 0, sizeof(struct sockaddr_in));
		if (isdigit(hostn[strlen(hostn)-1]))
			inet_aton(hostn, (struct in_addr *)&server.sin_addr);
		else
		{
			if (!(hp = resolv(hostn)))
	  			return close(fd), -6;
			memcpy(&server.sin_addr, hp->h_addr, hp->h_length);
		}
		server.sin_family = AF_INET;
		server.sin_port = htons(*portnum);

#ifdef NON_BLOCKING_CONNECTS
		if (nonblocking && set_non_blocking(fd) < 0)
			return close(fd), -4;
#endif

		alarm(get_int_var(CONNECT_TIMEOUT_VAR));
		if (connect (fd, (struct sockaddr *)&server, sizeof(server)) < 0)
		{
			alarm(0);
#ifdef NON_BLOCKING_CONNECTS
			if ((nonblocking && (errno != EINPROGRESS)) || !nonblocking)
#endif
				return close(fd), -4;
		}
		alarm(0);
	}

	/* error */
	else
		return close(fd), -7;

	return fd;
}

int	lame_resolv (const char *hostname, struct in_addr *buffer)
{
	struct hostent 	*hp;

	if (!(hp = resolv(hostname)))
		return -1;

	memmove(buffer, hp->h_addr, hp->h_length);
	return 0;
}


extern struct hostent *resolv (const char *stuff)
{
	struct hostent *hep;

	if ((hep = lookup_host(stuff)) == NULL)
		hep = lookup_ip(stuff);

	return hep;
}

extern struct hostent *lookup_host (const char *host)
{
	struct hostent *hep;

	alarm(1);
	hep = gethostbyname(host);
	alarm(0);
	return hep;
}

extern char *host_to_ip (const char *host)
{
	struct hostent *hep = lookup_host(host);
	static char ip[30];

	return (hep ? sprintf(ip,"%u.%u.%u.%u",	hep->h_addr[0] & 0xff,
						hep->h_addr[1] & 0xff,
						hep->h_addr[2] & 0xff,
						hep->h_addr[3] & 0xff),
						ip : empty_string);
}

extern struct hostent *lookup_ip (const char *ip)
{
	int b1 = 0, b2 = 0, b3 = 0, b4 = 0;
	char foo[4];
	struct hostent *hep;

	sscanf(ip,"%d.%d.%d.%d", &b1, &b2, &b3, &b4);
	foo[0] = b1;
	foo[1] = b2;
	foo[2] = b3;
	foo[3] = b4;

	alarm(1);
	hep = gethostbyaddr(foo, 4, AF_INET);
	alarm(0);

	return hep;
}

extern char *ip_to_host (const char *ip)
{
	struct hostent *hep = lookup_ip(ip);
	static char host[101];

	return (hep ? strncpy(host, hep->h_name, 100): empty_string);
}

extern char *one_to_another (const char *what)
{

	if (!isdigit(what[strlen(what)-1]))
		return host_to_ip (what);
	else
		return ip_to_host (what);
}



/*
 * It is possible for a race condition to exist; such that select()
 * indicates that a listen()ing socket is able to recieve a new connection
 * and that a later accept() call will still block because the connection
 * has been closed in the interim.  This wrapper for accept() attempts to
 * defeat this by making the accept() call nonblocking.
 */
int	my_accept (int s, struct sockaddr *addr, int *addrlen)
{
	int	retval;
	set_non_blocking(s);
	retval = accept(s, addr, addrlen);
	set_blocking(s);
	return retval;
}



int set_non_blocking(int fd)
{
#ifdef NON_BLOCKING_CONNECTS
	int	res;

#if defined(NBLOCK_POSIX)
	int nonb = 0;
	nonb |= O_NONBLOCK;
#else
# if defined(NBLOCK_BSD)
	int nonb = 0;
	nonb |= O_NDELAY;
# else
#  if defined(NBLOCK_SYSV)
	res = 1;

	if (ioctl (fd, FIONBIO, &res) < 0)
		return -1;
#  else
#   error no idea how to set an fd to non-blocking 
#  endif
# endif
#endif
#if (defined(NBLOCK_POSIX) || defined(NBLOCK_BSD)) && !defined(NBLOCK_SYSV)
	if ((res = fcntl(fd, F_GETFL, 0)) == -1)
		return -1;
	else if (fcntl(fd, F_SETFL, res | nonb) == -1)
		return -1;
#endif
#endif
	return 0;
}

int set_blocking(int fd)
{
#ifdef NON_BLOCKING_CONNECTS
	int	res;

#if defined(NBLOCK_POSIX)
	int nonb = 0;
	nonb |= O_NONBLOCK;
#else
# if defined(NBLOCK_BSD)
	int nonb = 0;
	nonb |= O_NDELAY;
# else
#  if defined(NBLOCK_SYSV)
	res = 0;

	if (ioctl (fd, FIONBIO, &res) < 0)
		return -1;
#  else
#   error no idea how to return an fd blocking 
#  endif
# endif
#endif
#if (defined(NBLOCK_POSIX) || defined(NBLOCK_BSD)) && !defined(NBLOCK_SYSV)
	if ((res = fcntl(fd, F_GETFL, 0)) == -1)
		return -1;
	else if (fcntl(fd, F_SETFL, res &~ nonb) == -1)
		return -1;
#endif
#endif
	return 0;
}

#ifdef WANT_THREAD
void dns_nslookup_callback(DNS_QUEUE * dns)
{
	if (dns->callinfo)
	{
		say("DNS for %s (%s -> %s)", (char *) dns->callinfo, dns->in,
			dns->out ? dns->out : "<UNKNOWN>");
		free(dns->callinfo);
	}
	else
	{
		say("DNS for %s -> %s", dns->in, 
				dns->out ? dns->out : "<UNKNOWN>");
	}
}

/* Add this nick/host to our threaded dns queue */
void do_nslookup(char *nick, char *host, char *cmd)
{
#ifdef WANT_NSLOOKUP
        add_to_dns_queue(host, dns_nslookup_callback, cmd, NULL, DNS_URGENT);
#endif
}
#endif

#ifndef WINNT
struct icmphdr
{
	unsigned char	type,
			code;
	unsigned short	checksum;
	union
	{
		struct
		{
			unsigned short	id, 
					sequence;
		} echo;
		unsigned long gateway;
		struct
		{
			unsigned short	_unused_; 
			unsigned short	mtu;
		} frag;
	} un;
};

#define ICMP_ECHO 8

/*
 * send icmp echo packets. in_cksum() borrowed from nap client
 *
 */
static int my_in_cksum(u_short *addr, int len)
{
	register int nleft = len;
	register u_short *w = addr;
	register int sum = 0;
	u_short answer = 0;

	/*
	 * Our algorithm is simple, using a 32 bit accumulator (sum), we add
	 * sequential 16 bit words to it, and at the end, fold back all the
	 * carry bits from the top 16 bits into the lower 16 bits.
	 */
	while (nleft > 1)  {
		sum += *w++;
		nleft -= 2;
	}

	/* mop up an odd byte, if necessary */
	if (nleft == 1) {
		*(u_char *)(&answer) = *(u_char *)w ;
		sum += answer;
	}

	/* add back carry outs from top 16 bits to low 16 bits */
	sum = (sum >> 16) + (sum & 0xffff);	/* add hi 16 to low 16 */
	sum += (sum >> 16);			/* add carry */
	answer = ~sum;				/* truncate to 16 bits */
	return (answer);
}
#endif

void send_ping(int s, unsigned long ip)
{
#ifndef WINNT
struct sockaddr_in dest;
struct icmphdr *icmp;

	if (s == -1 || get_int_var(PING_VAR) == 0)
		return;
	dest.sin_addr.s_addr = /*htonl(*/ip;
	dest.sin_family = AF_INET;

	icmp = (struct icmphdr *) new_malloc(sizeof(struct icmphdr));
	icmp->type = ICMP_ECHO;
	icmp->code = 0;
	icmp->un.echo.id = (getpid()&0xffff);
	icmp->un.echo.sequence = 0;
	icmp->checksum = my_in_cksum((u_short *)icmp, sizeof(struct icmphdr));
      
	sendto(s, icmp, sizeof(struct icmphdr), 0, (struct sockaddr *)&dest, sizeof(dest));
	new_free(&icmp);
#endif
}

int open_icmp(void)
{
#ifndef WINNT
int sock;
struct sockaddr_in me;

	if (get_int_var(PING_VAR) == 0)
		return -1;
	if ((sock = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP)) == -1)
		return(-1);
  
	me.sin_addr.s_addr = INADDR_ANY;
	me.sin_family = AF_INET;
  
	if (bind(sock, (struct sockaddr *)&me, sizeof(me)) == -1)
	{
		close(sock);
		return(-1);
	}
	return sock;
#else
	return -1;
#endif
}

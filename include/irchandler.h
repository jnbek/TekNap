/*
 * irc handler for TekNap.
 * designed to handle just simple commands. Nothing fancy.
 */
 
 #ifndef IRC_HANDLER_H
 #define IRC_HANDLER_H
 void parse_irc_server(char *);
 
 typedef struct {
	const char	*command;
	void 		(*inbound_handler) (char *, char **);
	void		(*outbound_handler) (char *);
	int		flags;
	unsigned long	bytes;
	unsigned long	count;
} protocol_command;

extern 	protocol_command rfc1459[];
extern	int		 num_protocol_cmds;

#define MAXPARA		15
#define PROTO_NOQUOTE	1 << 0
#define PROTO_DEPREC	1 << 1
#define CTCP_DELIM_CHAR '\001'

#endif


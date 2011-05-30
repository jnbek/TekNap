 /* $Id: ignore.c,v 1.1.1.1 2001/01/10 12:03:12 edwards Exp $ */
 
#if 0
320	user ignore list [CLIENT, SERVER]

	client: no data
	server: <count>

	client request to display the list of ignored users.
	server returns the number of users being ignored

321	user ignore list entry [SERVER]

	<user>

	server sends each ignored nick in response to a 320 request.  the
	list is terminated by a 320 message with the number of ignored users.

322	add user to ignore list [CLIENT, SERVER]

	<user>

	server acks the request by returning the nick

323	remove user from ignore list [CLIENT]

	<user>

	server acks the request by returning the nick to be removed from
	the ignore list.

324	user is not ignored [SERVER]

	<user>

	server indicates that <user> is not currently ignored in response to
	a 323 request.

325	user is already ignored [SERVER]

	<user>

	server indicates the specified user is already on the user's ignore
	list

326	clear ignore list [CLIENT, SERVER]

	client: no data.
	server: <count>

	client requests the server clear its ignore list.  server returns the
	number of entries removed from the ignore list.
#endif


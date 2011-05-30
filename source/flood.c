/*
 * flood.c: handle channel flooding. 
 *
 * This attempts to give you some protection from flooding.  Basically, it keeps
 * track of how far apart (timewise) messages come in from different people.
 * If a single nickname sends more than 3 messages in a row in under a
 * second, this is considered flooding.  It then activates the ON FLOOD with
 * the nickname and type (appropriate for use with IGNORE). 
 *
 * Thanks to Tomi Ollila <f36664r@puukko.hut.fi> for this one. 
 * $Id: flood.c,v 1.1.1.1 2000/06/23 05:55:51 edwards Exp $
 */

#include "teknap.h"
#include "struct.h"
#include "flood.h"
#include "hook.h"
#include "ircaux.h"
#include "output.h"
#include "server.h"
#include "vars.h"

static	char	*ignore_types[NUMBER_OF_FLOODS] =
{
	"JOINS",
	"MSGS",
	"PUBLIC",
};

typedef struct flood_stru
{
	char		nick    [NICKNAME_LEN + 1];
	char		channel [NICKNAME_LEN + 1];
	int		server;

	FloodType	type;
	long		cnt;
	time_t		start;
	int		floods;
}	Flooding;

static	Flooding *flood = NULL;


/*
 * check_flooding: This checks for message flooding of the type specified for
 * the given nickname.  This is described above.  This will return 0 if no
 * flooding took place, or flooding is not being monitored from a certain
 * person.  It will return 1 if flooding is being check for someone and an ON
 * FLOOD is activated. 
 */
int check_flooding (char *nick, char *chan, char *line, FloodType type)
{
static	int	 users = 0,
		 pos = 0;
	int	 i, 
		 numusers,
		 server,
		 retval = 1;
	time_t	 right_now,
		 diff;
	Flooding *tmp;

	if (from_server == -1)
		return 0;
	/*
	 * Figure out how many people we want to track
	 */
	numusers = get_int_var(FLOOD_USERS_VAR);

	/*
	 * Following 0 people turns off flood checking entirely.
	 */
	if (numusers == 0)
	{
		if (flood)
			new_free((char **)&flood);
		return 1;
	}

	/*
	 * If the number of users has changed, then resize the info array
	 */
	if (users != numusers)
	{
		users = numusers;
		RESIZE(flood, Flooding, users);
		for (i = 0; i < users; i++)
		{
			flood[i].nick[0] = 0;
			flood[i].channel[0] = 0;
			flood[i].server = -1;
			flood[i].type = -1;
			flood[i].cnt = 0;
			flood[i].start = 0;
			flood[i].floods = 0;
		}
	}

	/*
	 * What server are we using?
	 */
	server = from_server;

	/*
	 * Look in the flooding array.
	 * Find an entry that matches us:
	 *	It must be the same flooding type and server.
	 *	It must be for the same nickname
	 *	If we're for a channel, it must also be for a channel
	 *		and it must be for our channel
	 *	else if we're not for a channel, it must also not be for
	 *		a channel.
	 */
	for (i = 0; i < users; i++)
	{
		/*
		 * Do some inexpensive tests first
		 */
		if (type != flood[i].type)
			continue;
		if (server != flood[i].server)
			continue;

		/*
		 * Must be for the person we're looking for
		 */
		if (my_stricmp(nick, flood[i].nick))
			continue;

		/*
		 * Must be for a channel if we're for a channel
		 */
		if (!!flood[i].channel[0] != !!chan)
			continue;

		/*
		 * Must be for the channel we're looking for.
		 */
		if (chan && my_stricmp(chan, flood[i].channel))
			continue;

		/*
		 * We have a winner!
		 */
		break;
	}

	time(&right_now);

	/*
	 * We didnt find anybody.
	 */
	if (i == users)
	{
		/*
		 * pos points at the next insertion point in the array.
		 */
		int old_pos = pos;
		do
			pos = (pos + 1) % users;
		while (flood[pos].floods && pos != old_pos);

		tmp = flood + pos;
		strmcpy(tmp->nick, nick, NICKNAME_LEN);
		if (chan)
			strmcpy(tmp->channel, chan, NICKNAME_LEN);
		else
			tmp->channel[0] = 0;

		tmp->server = server;
		tmp->type = type;
		tmp->cnt = 1;
		tmp->start = right_now;

		return 1;
	}
	else
		tmp = flood + i;

	/*
	 * Has the person flooded too much?
	 */
	if (++tmp->cnt >= get_int_var(FLOOD_AFTER_VAR))
	{
		diff = right_now - tmp->start;

		if (diff == 0 || tmp->cnt / diff >= get_int_var(FLOOD_RATE_VAR))
		{
			if (get_int_var(FLOOD_WARNING_VAR))
				say("%s flooding detected from %s on %s", 
						ignore_types[type], nick, 
						tmp->channel[0] ? chan : "*");
			if (tmp->channel[0])
				retval = do_hook(FLOOD_LIST, "%s %s %s %s",
					nick, ignore_types[type], chan, line);
			else
				retval = do_hook(FLOOD_LIST, "%s %s * %s",
					nick, ignore_types[type], line);

			tmp->floods++;
		}
		else
		{
			/*
			 * Not really flooding -- reset back to normal.
			 */
			tmp->floods = 0;
			tmp->cnt = 1;
			tmp->start = right_now;
		}
	}
	return retval;
}


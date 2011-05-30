/* $Id: help.h,v 1.1.1.1 2001/01/23 09:40:55 edwards Exp $ */
 
#ifndef __help_h
# define __help_h

	BUILT_IN_COMMAND(help);

/* Since some people are too lazy to do this, I will - MH */

#define HELP_ADMIN "[killserver|banuser|setdataport|setlinespeed|setuserlevel|connect|disconnect|config|unnukeuser|unbanuser|unmuzzle|removeserver|opsay|announce|version|links|reload|kill|nukeuser|banlist|muzzle|speed|password|email|dataport|clearchannel|stats|register\n- WOW!"
#define HELP_ALIAS "<command(s)>\n- Adds a new alias with <command(s)>\n- Hint: If no argument is given, all aliases will be displayed"
#define HELP_ASSIGN "\n- Used in scripting to set a variable"
#define HELP_BEEP "\n- Emits a beep on your computer"
#define HELP_BIND "<key> <command>\n- Binds <key> to <command>"
#define HELP_BROWSE "<nick>\n- Browses <nick> mp3 list"
#define HELP_CD "[directory]\n- Changes current directory to <directory>. If <directory> isnt specified, it will display the current directory"
#define HELP_CIGNORE "[<channel>|-REMOVE <channel>]\n- With no arguments it displays your current channel ignore list\n- <channel> will be added to your ignore list\n- -REMOVE <channel> - <channel> will be removed from your ignore list"
#define HELP_CLEAR "[-all|-unhold|scrollback]\n- Clears the screen\n- -all  Clears everything (Scrollback, screen, hold window)\n- -unhold Clears the hold window\n- -scrollback Clears your scrollback buffer"
#define HELP_CLOSE "[servernum]\n- Disconnects from the current server or specified server"
#define HELP_DELETE "[#|*|range]\n- # - Deletes specific file in download/upload queue\n- * - Deletes all files in download/upload queue\n- range - Deletes a range of files in the download/upload queue"
#define HELP_DF "\n- Displays current disk usage"
#define HELP_DISCONNECT "\n- Disconnects from the current server"
#define HELP_DO "\n- do { .... } while (true) "
#define HELP_DU "\n- Displays current disk freespace"
#define HELP_DUMP "[bind][key][array][variables][alias][on][local][timers][all]\n- Dump all external variables and alias"
#define HELP_ECHO "<text>\n- Echos stuff to your local screen"
#define HELP_EXEC "[-out|-name arg|-window|-msg nick|-line {commands}|-linepart {command}|-errorpart {command}|-end {command}|-close index|-start|-in text|-direct|-num %process| <command>]\n- Executes command"
#define HELP_EVAL "<refnum> <command>\n- Sends <command> to a reference numbered server"
#define HELP_GET "[-browse nick|-request] args\n- Attempts to get a file after a /search or /browse is performed.\n- Args can be a range of numbers to get"
#define HELP_GLIST "\n- Displays your current download/upload queue"
#define HELP_HISTORY "[-clear|#]\n- -clear Clears your history buffer\n- # - Shows history line #"
#define HELP_HOTLIST "[<nick>|-<nick>|-remove nick]\n- With no arguments it displays your current hotlist\n- <nick> will be added to your hotlist\n- -<nick> - <nick> will be removed from your hotlist"
#define HELP_IGNORE "[<nick>|-REMOVE <nick>]\n- With no arguments it displays your current ignore list\n- <nick> will be added to your ignore list\n- -REMOVE <nick> - <nick> will be removed from your ignore list"
#define HELP_JOIN "<channel>\n- Joins a <channel>"
#define HELP_KILL "[-ip] <nick> [reason]\n- To kill a user, use -ip for a ip kill"
#define HELP_KICK "[<channel>] <nick> [reason]\n- To kick a user from a channel"
#define HELP_L HELP_LEAVE
#define HELP_LASTLOG "[-MAX #][-LITERAL][-REVERSE][-TIME][-BEEP][-CLEAR][-APPEND][-FILE filename][-MORE][-[crap|public|msgs|wallops|userlog1|userlog2|userlog3|userlog4|userlog5|beep|send_msg|kill|parts|join|topic|notify|server]|ALL] text\n- Display the lastlog to the screen"
#define HELP_LEAVE "[channel]\n- With no arguments it leaves the current channel your in\n- If a channel is specified it leaves that channel"
#define HELP_LIST "\n- Displays a list of channels that you can join"
#define HELP_LOAD "<script>\n- Loads a script.  This is almost like an irc script"
#define HELP_LOCAL "\n- This is only useful in scripts to set a local variable for a alias"
#define HELP_LS "[options] [pattern/dir]\n- An alias to the ls command"
#define HELP_M HELP_MSG
#define HELP_ME "<text>\n- Sends an action to the channel"
#define HELP_MOTD "\n- Displays the MOTD of the server. Dependant upon the value of /set SUPPRESS_SERVER_MOTD"
#define HELP_MSG "<nick> <text>\n- Sends a <text> message to <nick>.\n- Multiple nicks can be specified by using a , to seperate them"
#define HELP_NAMES "<channel>\n- With no arguments it displays a list of nicks on the current channel\n- Requests a list of people on <channel>"
#define HELP_NSLOOKUP "[-cmd {...}] <hostname/ip>\n- Returns the IP address for <hostname> and vice-versa"
#define HELP_ON "\n- This is a scripting command from BitchX, more documentation to be written"
#define HELP_PARSEKEY "<key>\n- Used for changing key binds"
#define HELP_PART HELP_LEAVE
#define HELP_PING "<nick>\n- Attempts to ping <nick> on the server. Indication of server or user lag"
#define HELP_PRINT "[-bitrate #|-count #|-freq #|-md5|-format \"string\"|-file]\n- With no arguments, prints out a list of files currently loaded\n- -bitrate # - Prints out files with a bitrate of #\n- -count # - Prints out # number of files\n- -freq # - Prints out files with a frequency of #\n- -md5 - Prints out files with an MD5 string\n- -format \"string\" - Changes the format of the output\n- -file <filename> - Instead of printing to your screen, it writes it to <filename>"
#define HELP_PS "\n- exec the ps command to display various process"
#define HELP_PURGE "arg1 [arg2 arg3 .....]\n- Removes listed aliases"
#define HELP_QUERY "<nick>\n- If no arguments are given it ends the current conversation\n- Starts a conversation with <nick> all text typed will be sent to <nick>"
#define HELP_QUITNAP "\n- Quit from the client. If there are active file transfers, a second chance is given"
#define HELP_RAW "<number> [<args>]\n- Sends a raw server message to server <number>\n- Most useful during debugging"
#define HELP_RBIND "<key>\n- Show a key binding for <key>"
#define HELP_RELM "-\n Display last 10 recieved/sent messages"
#define HELP_RELOAD "\n- Reloads the ~/.TekNap/TekNap.sav file and all settings"
#define HELP_RESUME "\n- Shows files which can be resumed"
#define HELP_REQUEST "<nick> <exact filename/path>\n- Attempts to download a file from <nick>"
#define HELP_S HELP_SEARCH
#define HELP_SAVE "\n- Saves all your settings to ~/.TekNap/TekNap.sav"
#define HELP_SAY "<text>\n- Display text to the current channel"
#define HELP_SCAN "[channel]\n- If no arguments are given, it shows users on your current channel\n- Scans for users on <channel>"
#define HELP_SC HELP_SCAN
#define HELP_SEARCH "[-type audio/video/image|-any|-local|-end|max-results #|-bitrate #|-freq #|-line #|-duration #|-size #|-exclude word] pattern\n- -type is only supported under opennap servers\n- -maxresults # - Maximum results are 100, it IS a server                             limitation\n- -bitrate #    - Finds files with a bitrate of #\n- -freq #       - Finds files with a frequency of #\n- -local        - displays only local files(opennap only)\n- -line #       - Display users with a minimum line speed of #\n- -end         - end a search prematurely\n- -duration #   - Find songs of a certain time\n- -size #       - Find songs with a certain filesize\n- -exclude x    - exclude word from search"
#define HELP_SERVER "[-add arg] [-create] <server:port:nick:password:meta>\n- -add - Adds a server to the list without connecting to it\n- -create - Creates an account on a server"
#define HELP_SET "\n- Sets various options used in the client"
#define HELP_SHARE "[-load [filename]| -save [filename] | -share | -clear | -unshare | -type mimetype | -reload | -recurse | -remove \"pattern\"] [dir dir1]\n- This particular command is fairly complicated. In its simplest form the following 4 commands will do what is most often needed.\n\t/share /home/mp3        scans /home/mp3 recursively for mp3s\n\t/share -save            saves the shared.dat file\n\t/share -load            loads the shared.dat file\n\t/share -share           actually shares the files with the\n\t\t\t\tserver\n-type can be used to specify a particular \"mime\" type from\nthe following list. video, audio, image, any\n-reload will scan the directory, and search for any missing files\n-clear will clear the internal list of files and notify the server that we are no longer sharing\n-update will check the internal filelist against your HD files to see if they exist. If a file is found that does not exist, then it is removed and if the server knows about it the server is notified\n-recurse is a toggle to turn off/on the directory recursion.\n-remove will remove the \"pattern\" specified"
#define HELP_SIGNORE "[\"pattern(s)\"]\n- In order to ignore certain 404 messages from the server (usually moderator or higher) a server ignore was implemented. The pattern is a quoted string which you wish to ignore in the future. You can use a * to match multiple words and % to replace a single word in the string. Multiple patterns may be specified\n\t/signore \"Notification from %: % % - configured data port * unreachable\"\n\nWould ignore the following message\nNotification from q: qr1 (255.255.255.255) - configured data port 6699 unreachable"
#define HELP_SOUNDEX "\n- See search. This attempts to use a soundex for mispelled words.\n- Only supported on opennap servers"
#define HELP_STATS "\n- Displays various statistics about the server and yourself"
#define HELP_TIMER "[-del #] [-ref #] [-rep #] <delay> <command>\n\n-del # - Deletes an active timer\n-rep # - Repeats an event # times, -1 is infinite"
#define HELP_TOPIC "[channel] [text]\n- If no arguments are given, it displays the current topic\n- If arguments are given, it changes the current topic to <text>"
#define HELP_VERSION "\n- Displays the current version of your client to the channel your in"
#define HELP_W HELP_WHOIS
#define HELP_WALLOP "text\n- Sends a wallop to all moderator+"
#define HELP_WHOIS "[nick]\n- If <nick> is not specified, it will display whois information about yourself\n- If <nick> is specified, it will display whois information about <nick>"
#define HELP_WINDOW "Use /window help for help with this"
#define HELP_XECHO "\n- Used in scripting to display text to the window"
#define HELP_HELP "[section] [name]\n- Used to display help from the comprehensive help system"
#define HELP_UNMUZZLE "<nick>|<nick1,nick2,nick3,...> [reason]\n- Unmuzzle nick(s) with optional reason"
#define HELP_MUZZLE "<nick>|<nick1,nick2,nick3,....> [reason]\n- Muzzle nick with a reason"
#define HELP_PTEST "<nick> <port>\n- Test a port for activity"
#define HELP_RESET "\n- reset the terminal in case of a problem"
#define HELP_CLOAK "\n- This is a server command for moderators to hide with. its a toggle"
#define HELP_CBANLIST "<channel>\n- Display the <channel> banlist"
#define HELP_CBAN "<channel> <nick> [reason]\n- Ban nick on channel with optional reason"
#define HELP_CBANCLEAR "<channel>\n- Clear a channels banlist"
#define HELP_CUNBAN "<channel> <ban> [reason]\n- Remove a single ban from a channel" 
#define HELP_HOOK "\n- Gives the ability to trigger a \"on\""
#define HELP_GUSERS " [server|*] [-Elite][-Admin][-Moderator][-Leech][-Cloaked][-Muzzled][-Ip #][-Channel #name]\n- Shows all users with the specifed level (all by default) on the specified server or your current server"
#define HELP_CHANLEVEL "<channel> <level>\n- Set a channels required level for joining"
#define HELP_OPLIST "<channel>\n- Display the channel oplist"
#define HELP_OP "<channel> <nick>\n- Make nick a op on the channel requested"
#define HELP_DELOP "<channel> <nick>\n- Delete a nick from a channel oplist"
#define HELP_BANCMD "<nick | ip> [reason]\n- ban a nick or a ip using a reason if specified"
#define HELP_TBANCMD "<nick | ip> <time> [reason]\n- ban a nick or a ip using a reason if specified for <time> seconds or 1y1d1h1s or 1w"
#define HELP_OPSAY "<text>\n- Text is sent to all current channel ops and mod+"
#define HELP_MODE "[<channel>] [[+/-][private][moderated]]\n- Sets or unsets the private or moderated flag on a channel. Current channel is used if not specifed. With no +/- will display channel mode."
#define HELP_INVITE "[<channel>] <nick>|<nick,nick1,nick2...>\n- Invite a nick to <channel> or current channel"
#define HELP_SETCHANNELLIMIT "<nick> <limit>\n- Set a channels limit on the number of users allowed"
#define HELP_SETUSERLEVEL "<nick> <level>\n- Set a users level to <leech|user|moderator|admin|elite>"
#define HELP_SETCHANNELLEVEL "<channel> <level>\n- Set a channels required level for joining"
#define HELP_SETDATAPORT "<nick> <portnum>\n- Set nicks data port"
#define HELP_SETLINESPEED "<nick> <speed>\n- Set nicks linespeed (numeric value)"
#define HELP_SETPASSWORD "<nick> <password>\n- Set nicks password in case they forget"
#define HELP_ANNOUNCE "<text>\n- Admin+ command to make server announcements"
#define HELP_SPING "<servername|-A>\n- Send a server ping and evaluate the result. -A for a mass server ping"
#define HELP_DNS "[-cmd {...}][-flush][-list] <nick> [<nick> ...]\n- attempt to lookup <nick> ip as a hostname"

#define HELP_DBSEARCH NULL
#define HELP_FE NULL
#define HELP_FEC NULL
#define HELP_FOR NULL
#define HELP_FOREACH NULL
#define HELP_IFCMD NULL
#define HELP_QUEUE NULL
#define HELP_REPEAT NULL
#define HELP_SCOTT NULL
#define HELP_STUB NULL
#define HELP_SWITCH NULL
#define HELP_TYPE NULL
#define HELP_UNLESS NULL
#define HELP_WHILE NULL
#define HELP_CONTINUE NULL
#define HELP_BREAK NULL
#define HELP_INPUT NULL
#define HELP_INPUTCHAR NULL
#define HELP_PAUSE NULL
#define HELP_RETURN NULL
#define HELP_SEND NULL
#define HELP_SLEEP NULL
#define HELP_USLEEP NULL
#define HELP_WAIT NULL
#define HELP_BLOCK NULL
#define HELP_UNBLOCK NULL
#define HELP_BLOCKLIST NULL
#define HELP_SENDLINE NULL
#define HELP_WHICH NULL
#define HELP_POP NULL
#define HELP_PUSH NULL
#define HELP_STACKCMD NULL
#define HELP_SETENV NULL
#define HELP_XTYPE NULL
#define HELP_SHIFT NULL
#define HELP_UNSHIFT NULL
#define HELP_PRETEND "<number> [args]\n- Send a pretend server command through the client"

#define HELP_CHANMUZZLE "<channel> n1 [n2 ....]\n- Muzzle specified nicks on a channel"
#define HELP_CHANUNMUZZLE "<channel> n1 [n2 ....]\n- UnMuzzle specified nicks on a channel"
#define HELP_UNVOICE "<channel> n1 [n2 ....]\n- UnVoice specified nicks on a channel"
#define HELP_VOICE "<channel> n1 [n2 ....]\n- Voice specified nicks on a channel"
#define HELP_URL "[save][url][-[#]][list][+]\n- Depends on /set url_grab on/off.\n\t- save  saves the url list\n\t- url  toggles on/off the grabber\n\t- -[#] delete entire list or specified number\n\t- list|+  lists the captured urls"
#define HELP_DCCGET "<nick>\n- Attempt to get a file which someone has sent to you"
#define HELP_PASTE "[-win #][-topic][channel] <#>\n- Paste a line or a range of lines to the current channel or specified channel"
#define HELP_IRCCMD "[server <server name:port:nick>] [nick <nickname>] [who <target>]\n"
#define HELP_WINAMPPLAY "[-volume #] [-next] [-prev] [-position #] [-stop] [-fadeout] [-pause] [-play c:\filename] [c:\filename]\n- Winamp control "

#define HELP_HISTOGRAM "numeric histogram display"
#define HELP_CLASSLIST "<ip> <limit>\n- Adds a clone limit for a ip. 24/8 for example"
#define HELP_ADDCLASS NULL
#define HELP_DELCLASS NULL
#define HELP_ILINELIST "<ip>\n- IP is allowed on server at all times 192.68.0.1"
#define HELP_ADDILINE NULL
#define HELP_DELILINE NULL
#define HELP_DLINELIST "<ip>\n- IP is essentially banned but with no ban msg displayed."
#define HELP_ADDDLINE NULL
#define HELP_DELDLINE NULL
#define HELP_ELINELIST "<ip>\n- IP is exempted from the dline" 
#define HELP_ADDELINE NULL
#define HELP_DELELINE NULL

#define HELP_SHOWALLCHANNELS NULL
#define HELP_KILLSERVER NULL
#define HELP_BANUSER NULL
#define HELP_SCONNECT NULL
#define HELP_SDISCONNECT NULL
#define HELP_CONFIG NULL
#define HELP_UNNUKEUSER NULL
#define HELP_UNBANUSER NULL
#define HELP_REMOVESERVER NULL
#define HELP_SVERSION NULL
#define HELP_LINKS NULL
#define HELP_SRELOAD NULL
#define HELP_NUKEUSER NULL
#define HELP_BANLIST NULL
#define HELP_SPEED NULL
#define HELP_PASSWORD NULL
#define HELP_EMAIL NULL
#define HELP_DATAPORT NULL
#define HELP_CLEARCHANNEL NULL
#define HELP_SERVERSTATS NULL
#define HELP_REGISTER NULL
#define HELP_REHASH NULL
#define HELP_DROP NULL
#define HELP_CVERSION NULL


#endif /* __help_h */

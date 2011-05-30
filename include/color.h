 /* $Id: color.h,v 1.1.1.1 2001/01/17 21:18:33 edwards Exp $ */
 
#ifndef _INC_COLOR
#define _INC_COLOR

/*
 * DEFAULT_DOUBLE STATUS  can be defined as 0 or 1
 * 0 makes the status line be 1 line only. 1 makes it 2 lines
 * this will be the default for all created windows.
 * DEFAULT_STATUS_LINES can be defined as 0 or 1
 * 0 is off, and 1 makes the window split the default for ALL windows.
 * a window split is the topic bar.
 */
 
#ifdef HUMBLE


/* Color.h by Humble - Lets try the hades look =) */

#define DEFAULT_STATUS_FORMAT "[1;30;40mÚ[0;37m%T %*[1;36m%N[0;37m%# %@%C%+ %A%W%H%B%M%Q %>%D %L [1;30m]"
#define DEFAULT_STATUS_FORMAT1 "[1;30;40mÚÄ [1;37mU[0;37mser[1;30m: %*[1;36m%N[0;37m%#%A %^%H%B %>%D %S [1;30mÄ¿"
#define DEFAULT_STATUS_FORMAT2 "[1;30;40m³  [1;37mC[0;37mhannel[1;30m:[0;37m %@%C%+%W %M%Q %>%T %L [1;30mÄÙ"

#define DEFAULT_STATUS_AWAY "[1;30;40m([1;37mA[1;30m)[0;37m"
#define DEFAULT_STATUS_CHANNEL "[1;36m%C[0;37m"
#define DEFAULT_STATUS_CHANOP "[0;36m@[0;37m"
#define DEFAULT_STATUS_HALFOP "[0;36m%[0;37m"
#define DEFAULT_STATUS_CLOCK "[1;37mT[0;37mime[1;30m: [1;36m%T[0;37m"
#define DEFAULT_STATUS_HOLD " [1;30;40m--[1;37mMore[1;30m--[0;37m"
#define DEFAULT_STATUS_HOLD_LINES " [1;37mH[0;37mold[1;30m: [1;36m%B[0;37m"
#define DEFAULT_STATUS_LAG " [1;37mL[0;37mag[1;30m: [1;36m%L[0;37m"
#define DEFAULT_STATUS_MODE "[1;30;40m([1;37m%+[1;30m)[0;37m"
#define DEFAULT_STATUS_MAIL " [1;37mM[0;37mail[1;30m: [1;36m%M[0;37m"
#define DEFAULT_STATUS_NOTIFY " [1;37mN[0;37motify[1;30m: [1;36m%F[0;37m"
#define DEFAULT_STATUS_OPER "[0;36m*[0;37m"
#define DEFAULT_STATUS_VOICE "[0;36m+[0;37m"
#define DEFAULT_STATUS_QUERY " [1;37mQ[0;37muery[1;30m: [1;36m%Q[0;37m"
#define DEFAULT_STATUS_SERVER "[1;37mS[0;37merver[1;30m: [1;36m%S[0;37m"
#define DEFAULT_STATUS_UMODE "[1;30;40m([1;37m+%#[1;30m)[0;37m"
#define DEFAULT_STATUS_OPER_KILLS "[1;30;40m[[1;37mnk [1;36m%d[1;30m:[1;37mok [1;36m%d[1;30m][0;37m"
#define DEFAULT_STATUS_WINDOW "[1;30;40m[[1;37mþ[1;30m][0;37m"

#define DEFAULT_STATUS_FORMAT3 "BXnap by panasync, Hades formats by Humble"
#define DEFAULT_STATUS_INSERT ""
#define DEFAULT_STATUS_MSGCOUNT " [1;37mM[0;37msgs[1;30m: ([1;36m%^[1;30m)[0;37m"
#define DEFAULT_STATUS_NICK "%N"
#define DEFAULT_STATUS_OVERWRITE "(overtype) "
#define DEFAULT_STATUS_TOPIC "%-"
#define DEFAULT_STATUS_USER " * type /help for help "
#define DEFAULT_STATUS_DCCCOUNT  "[DCC  gets/%& sends/%&]"
#define DEFAULT_STATUS_CDCCCOUNT "[CDCC gets/%| offer/%|]"
#define DEFAULT_STATUS_USERS "[0;44m[[37mO[36m/[1;37m%! [0;44mN[36m/[1;37m%! [0;44mI[36m/[1;37m%! [0;44mV[36m/[1;37m%! [0;44mF[36m/[1;37m%![0;44m]"
#define DEFAULT_STATUS_CPU_SAVER " (%J)"

#define DEFAULT_STATUS_USER1 ""
#define DEFAULT_STATUS_USER2 ""
#define DEFAULT_STATUS_USER3 ""
#define DEFAULT_STATUS_USER4 ""
#define DEFAULT_STATUS_USER5 ""
#define DEFAULT_STATUS_USER6 ""
#define DEFAULT_STATUS_USER7 ""
#define DEFAULT_STATUS_USER8 ""
#define DEFAULT_STATUS_USER9 ""
#define DEFAULT_STATUS_USER10 ""
#define DEFAULT_STATUS_USER11 ""
#define DEFAULT_STATUS_USER12 ""
#define DEFAULT_STATUS_USER13 ""
#define DEFAULT_STATUS_USER14 ""
#define DEFAULT_STATUS_USER15 ""
#define DEFAULT_STATUS_USER16 ""
#define DEFAULT_STATUS_USER17 ""
#define DEFAULT_STATUS_USER18 ""
#define DEFAULT_STATUS_USER19 ""
#define DEFAULT_STATUS_USER20 ""
#define DEFAULT_STATUS_USER21 ""
#define DEFAULT_STATUS_USER22 ""
#define DEFAULT_STATUS_USER23 ""
#define DEFAULT_STATUS_USER24 ""
#define DEFAULT_STATUS_USER25 ""
#define DEFAULT_STATUS_USER26 ""
#define DEFAULT_STATUS_USER27 ""
#define DEFAULT_STATUS_USER28 ""
#define DEFAULT_STATUS_USER29 ""
#define DEFAULT_STATUS_USER30 ""
#define DEFAULT_STATUS_USER31 ""
#define DEFAULT_STATUS_USER32 ""
#define DEFAULT_STATUS_USER33 ""
#define DEFAULT_STATUS_USER34 ""
#define DEFAULT_STATUS_USER35 ""
#define DEFAULT_STATUS_USER36 ""
#define DEFAULT_STATUS_USER37 ""
#define DEFAULT_STATUS_USER38 ""
#define DEFAULT_STATUS_USER39 ""
#define DEFAULT_STATUS_FLAG ""
#define DEFAULT_STATUS_SCROLLBACK " (Scroll)"

#define DEFAULT_INPUT_PROMPT "[1;30;40mÀ-[[1;37mB[0;37mWap[1;30m]Ä>[0;37m "

#ifndef ONLY_STD_CHARS
#define DEFAULT_SHOW_NUMERICS_STR "[1;30mù[0m[1;36mí[1;30mù[0m"
#else
#ifndef LATIN1
#define DEFAULT_SHOW_NUMERICS_STR "***"
#else
#define DEFAULT_SHOW_NUMERICS_STR "[1;30m-[0m[1;36m:[1;30m-[0m"
#endif
#endif


#else 

/* default TekNap */


#define DEFAULT_STATUS_FORMAT1 "[0;44;36m[[1;37m%T[0;44;36m][[0;44;37m%h%N[0;44;36m]%A%D [0;44;36m[%C:%c%+%W[0;44;36m] %Q %H%B%F%G[%L]%{1}K "
#define DEFAULT_STATUS_FORMAT2 "[0;44;36m[[1;37m%s[0;44;36m]%S%>%d %J"

#define DEFAULT_STATUS_AWAY " [0;44;36m([1;32mzZzZ[1;37m %A[0;44;36m)[0;44;37m"
#define DEFAULT_STATUS_CHANNEL "[0;44;37m%C"
#define DEFAULT_STATUS_CHANOP "@"
#define DEFAULT_STATUS_HALFOP "%"
#define DEFAULT_STATUS_CLOCK "%T"
#define DEFAULT_STATUS_HOLD " -- more --"
#define DEFAULT_STATUS_HOLD_LINES " [0;44;36m([1;37m%B[0;44m)[0;44;37m"
#define DEFAULT_STATUS_INSERT ""
#define DEFAULT_STATUS_LAG "[0;44;36m[[0;44;37mLag [1;37m%L[0;44;36m]"
#define DEFAULT_STATUS_MODE "[1;37m([0;44;36m+[0;44;37m%+[1;37m)"
#define DEFAULT_STATUS_MAIL "[0;44;36m[[0;44;37mMail: [1;37m%M[0;44;36m]"
#define DEFAULT_STATUS_MSGCOUNT " Aw[0;44;36m[[1;37m%^[0;44;36m]"
#define DEFAULT_STATUS_NICK "%N"
#define DEFAULT_STATUS_NOTIFY " [0;44;36m[[37mAct: [1;37m%F[0;44;36m]"
#define DEFAULT_STATUS_OPER "[1;31m*[0;44;37m"
#define DEFAULT_STATUS_VOICE "[1;32m+[0;44;37m"
#define DEFAULT_STATUS_OVERWRITE "(overtype) "
#define DEFAULT_STATUS_QUERY " [0;44;36m[[37mQuery: [1;37m%Q[0;44;36m]"
#define DEFAULT_STATUS_SERVER " via %S"
#define DEFAULT_STATUS_STATS "%s/%s/%s"
#define DEFAULT_STATUS_TOPIC "%-"
#define DEFAULT_STATUS_UMODE "[1;37m([0;44;36m+[37m%#[1;37m)"
#define DEFAULT_STATUS_USER " * type /help for help "

#define DEFAULT_STATUS_DCCCOUNT  "[DCC  gets/%& sends/%&]"
#define DEFAULT_STATUS_CDCCCOUNT "[CDCC gets/%| offer/%|]"

#define DEFAULT_STATUS_OPER_KILLS "[0;44;36m[[37mnk[36m/[1;37m%K [0;44mok[36m/[1;37m%K[0;44;36m]"
#define DEFAULT_STATUS_USERS "[0;44;36m[[37mO[36m/[1;37m%! [0;44mN[36m/[1;37m%! [0;44mI[36m/[1;37m%! [0;44mV[36m/[1;37m%! [0;44mF[36m/[1;37m%![0;44;36m]"
#define DEFAULT_STATUS_CPU_SAVER " (%J)"

#define DEFAULT_STATUS_USER1 ""
#define DEFAULT_STATUS_USER2 ""
#define DEFAULT_STATUS_USER3 ""
#define DEFAULT_STATUS_USER4 ""
#define DEFAULT_STATUS_USER5 ""
#define DEFAULT_STATUS_USER6 ""
#define DEFAULT_STATUS_USER7 ""
#define DEFAULT_STATUS_USER8 ""
#define DEFAULT_STATUS_USER9 ""
#define DEFAULT_STATUS_USER10 ""
#define DEFAULT_STATUS_USER11 ""
#define DEFAULT_STATUS_USER12 ""
#define DEFAULT_STATUS_USER13 ""
#define DEFAULT_STATUS_USER14 ""
#define DEFAULT_STATUS_USER15 ""
#define DEFAULT_STATUS_USER16 ""
#define DEFAULT_STATUS_USER17 ""
#define DEFAULT_STATUS_USER18 ""
#define DEFAULT_STATUS_USER19 ""
#define DEFAULT_STATUS_USER20 ""
#define DEFAULT_STATUS_USER21 ""
#define DEFAULT_STATUS_USER22 ""
#define DEFAULT_STATUS_USER23 ""
#define DEFAULT_STATUS_USER24 ""
#define DEFAULT_STATUS_USER25 ""
#define DEFAULT_STATUS_USER26 ""
#define DEFAULT_STATUS_USER27 ""
#define DEFAULT_STATUS_USER28 ""
#define DEFAULT_STATUS_USER29 ""
#define DEFAULT_STATUS_USER30 ""
#define DEFAULT_STATUS_USER31 ""
#define DEFAULT_STATUS_USER32 ""
#define DEFAULT_STATUS_USER33 ""
#define DEFAULT_STATUS_USER34 ""
#define DEFAULT_STATUS_USER35 ""
#define DEFAULT_STATUS_USER36 ""
#define DEFAULT_STATUS_USER37 ""
#define DEFAULT_STATUS_USER38 ""
#define DEFAULT_STATUS_USER39 ""
#define DEFAULT_STATUS_WINDOW "[1;44;33m^^^^^^^^[0;44;37m"
#define DEFAULT_STATUS_FLAG ""
#define DEFAULT_STATUS_SCROLLBACK " (Scroll)"

#define DEFAULT_INPUT_PROMPT "[$C] "

#ifndef ONLY_STD_CHARS
#define DEFAULT_SHOW_NUMERICS_STR "[1;31mù[0m[1;37mí[1;31mù[0m"
#else
#ifndef LATIN1
#define DEFAULT_SHOW_NUMERICS_STR "***"
#else
#define DEFAULT_SHOW_NUMERICS_STR "[1;31m-[0m[1;37m:[1;31m-[0m"
#endif
#endif

#endif

#define DARK_BLACK	"[0;30m"
#define DARK_BLUE	"[0;34m"
#define DARK_GREEN	"[0;32m"
#define DARK_CYAN	"[0;36m"
#define DARK_RED	"[0;31m"
#define DARK_MAGENTA	"[0;35m"
#define DARK_YELLOW	"[0;33m"
#define DARK_WHITE	"[0m"
#define LIGHT_BLACK	"[1;30m"
#define LIGHT_BLUE	"[1;34m"
#define LIGHT_GREEN	"[1;32m"
#define LIGHT_CYAN	"[1;36m"
#define LIGHT_RED	"[1;31m"
#define LIGHT_MAGENTA	"[1;35m"
#define LIGHT_YELLOW	"[1;33m"
#define	LIGHT_WHITE	"[1;37m"

#if 0
"[0;40m", "[0;41m", "[0;42m","[0;43m", "[0;44m","[0;45m","[0;46m", "[0;47m",
"[1;40m", "[1;41m", "[1;42m","[1;43m", "[1;44m","[1;45m","[1;46m", "[1;47m",
"[7m", "[1m", "[5m", "[4m"
#endif

#define FORMAT_SENDMSG LIGHT_BLACK"["DARK_RED"msg"LIGHT_BLACK"("LIGHT_RED"%s"LIGHT_BLACK")]"DARK_WHITE" %s"
#define FORMAT_SENDPUBLIC LIGHT_MAGENTA"<"DARK_WHITE"%s"LIGHT_MAGENTA">"DARK_WHITE" %s"
#define FORMAT_PUBLIC LIGHT_BLUE"<"DARK_WHITE"%s"LIGHT_BLUE">"DARK_WHITE" %s"
#define FORMAT_PUBLIC_OTHER LIGHT_BLUE"<"DARK_WHITE"%s"LIGHT_WHITE"/"DARK_WHITE"%s"LIGHT_BLUE">"DARK_WHITE" %s"
#define FORMAT_NICKCOLOR LIGHT_BLACK"[       %16s"LIGHT_BLACK"]"DARK_WHITE
#define FORMAT_MSG  LIGHT_BLACK"["LIGHT_MAGENTA"%s"LIGHT_BLACK"("DARK_MAGENTA"%s"LIGHT_BLACK")]"DARK_WHITE" %s"
#define FORMAT_NOTICE  LIGHT_BLACK"-"LIGHT_MAGENTA"%s"LIGHT_BLACK"("DARK_MAGENTA"%s"LIGHT_BLACK")-"DARK_WHITE" %s"

#define FORMAT_GLIST1 LIGHT_BLACK"ÚÄÄÄÄÄ"DARK_GREEN"D"DARK_WHITE"ownload"LIGHT_BLACK"/"DARK_GREEN"U"DARK_WHITE"pload  Size     ETA  Speed   %%    Filename"
#define FORMAT_GLIST2 LIGHT_BLACK"ÀÄÄ"DARK_WHITE"Ä"LIGHT_WHITE"Ä"DARK_WHITE"Ä"LIGHT_BLACK"ÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄÄ"

#define FORMAT_DCC_MSG LIGHT_BLACK"["DARK_GREEN"%s/%s"LIGHT_BLACK"("LIGHT_GREEN"dcc"LIGHT_BLACK")]"DARK_WHITE" %s"
#define FORMAT_SENDDCC LIGHT_BLACK"["DARK_RED"dcc"LIGHT_BLACK"("LIGHT_RED"%s/%s"LIGHT_BLACK")]"DARK_WHITE" %s"

#endif

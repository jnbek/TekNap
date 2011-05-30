#ifndef __cdrom_h_
#define __cdrom_h_
 /* $Id: cdrom.h,v 1.1.1.1 2000/06/23 06:27:55 edwards Exp $ */
 
#ifndef __FreeBSD__
#include <mntent.h>
#else
#include <sys/param.h>
#include <sys/ucred.h>
#include <sys/mount.h>
#include <sys/file.h>
#include <sys/cdio.h>
#endif /* __FreeBSD__ */


#include <sys/ioctl.h>
#include <signal.h>
#include <fcntl.h>

#ifndef __FreeBSD__
#include <linux/cdrom.h>
#include <linux/errno.h>
#endif

#ifndef MODULE_CDROM
extern void	set_cd_device (Window *, char *, int);
BUILT_IN_COMMAND(cd_stop);
BUILT_IN_COMMAND(cd_eject);
BUILT_IN_COMMAND(cd_play);
BUILT_IN_COMMAND(cd_list);
BUILT_IN_COMMAND(cd_volume);
BUILT_IN_COMMAND(cd_pause);
BUILT_IN_COMMAND(cd_help);
#endif

#ifndef __FreeBSD__
struct cdrom_etocentry 
{
	u_char	cdte_track;
	u_char	cdte_adr	:4;
	u_char	cdte_ctrl	:4;
	u_char	cdte_format;
	union cdrom_addr cdte_addr;
	u_char	cdte_datamode;
	int avoid;
	int length;
	int m_length;
	int m_start;
};
#else

struct cdrom_etocentry
{
	u_char m_length;
	u_char m_start;
	int avoid;
};
	
#define CDROMSTOP CDIOCSTOP
#define CDROMEJECT CDIOCEJECT
#define CDROMREADTOCHDR CDIOREADTOCHEADER
#define CDROMVOLCTRL CDIOCSETVOL
#define CDROMPAUSE CDIOCPAUSE
#define CDROMRESUME CDIOCRESUME
#define CDROMVOLREAD CDIOCGETVOL
#endif /* __FreeBSD__ */


#define HELP_CDEJECT NULL
#define HELP_CDLIST NULL
#define HELP_CDPAUSE NULL
#define HELP_CDPLAY NULL
#define HELP_CDSTOP NULL
#define HELP_CDVOLUME NULL


#endif /* cdrom.h */

 /* $Id: scott.h,v 1.1.1.1 2000/06/22 09:35:00 edwards Exp $ */
 
#ifndef _scott_h
#define _scott_h

/* Our 1 global variable we export out to the world.
 * This flag is set, whenever the file browser is running.
 */
extern int in_browser;
/* And our 2 global functions we export out to the world. */
void browser_main_loop(void);
BUILT_IN_COMMAND(scott);
void refresh_browser(void);


#define CTRL_A 1
#define CTRL_B 2
#define CTRL_C -1
#define CTRL_D 4
#define CTRL_E 5
#define CTRL_F 6
#define CTRL_G 7
#define CTRL_H 8
#define CTRL_I 9
#define CTRL_J 10
#define CTRL_K 11
#define CTRL_L 12
#define CTRL_M 13
#define CTRL_N 14
#define CTRL_O 15
#define CTRL_P 16
#define CTRL_Q 17
#define CTRL_R 18
#define CTRL_S 19
#define CTRL_T 20
#define CTRL_U 21
#define CTRL_V 22
#define CTRL_W 23
#define CTRL_X 24
#define CTRL_Y 25
#define CTRL_Z 26
#define BLOCK_KEY 30
#define SPACEBAR 32

#ifndef COLOR_BLACK
#define COLOR_BLACK	0
#endif
#ifndef COLOR_RED
#define COLOR_RED	1
#endif
#ifndef COLOR_GREEN
#define COLOR_GREEN	2
#endif
#ifndef COLOR_YELLOW
#define COLOR_YELLOW	3
#endif
#ifndef COLOR_BLUE
#define MY_BLUE		4
#endif
#ifndef COLOR_MAGENTA
#define COLOR_MAGENTA	5
#endif
#ifndef COLOR_CYAN
#define COLOR_CYAN	6
#endif
#ifndef COLOR_WHITE
#define COLOR_WHITE	7
#endif

#define BROWSE_TOGGLE	0x1
#define BLOCK_TOGGLE	0x2
#define DOWNLOADED_FILE 0x4
#define DONT_FORCE 0
#define FORCE_ON 1
#define FORCE_OFF 2
#define SWITCH_HELP 1
#define MY_INDENT 2
#define MY_CENTER(MAX, X) ((MAX / 2) - (X / 2))

#endif


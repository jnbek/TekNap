/*
 * seems windows people dont get it. autoexec.bat NEEDS to be setup for 
 * certain things. Also a teknap.rc file in the HOME directory is kinda
 * required for making TekNap nicer.
 */
 
#include "teknap.h"
#include "ircaux.h"

#if defined(WINNT) || defined(EMX)

char HOME[BIG_BUFFER_SIZE];
char NAPNICK[40];
char NAPPASS[40];

char NAPSERVER[100];
char NAPSPORT[10];
char NAPPORT[10];

void setup_autoexec()
{
FILE *fp;
int meta = 0;

	memset(HOME, 0, sizeof(HOME));
	memset(NAPNICK, 0, sizeof(NAPNICK));
	memset(NAPPASS, 0, sizeof(NAPPASS));
	
	fprintf(stdout, "Enter the directory that TekNap will use for a home directory or \r\npress Enter to use c:\\TekNap\r\n");
	fgets(HOME, sizeof(HOME)-1, stdin);
	chomp(HOME);
	if (!*HOME)
		strcpy(HOME, "//C/TekNap");
	while (!*NAPNICK)
	{
		fprintf(stdout, "Enter the desired nick : ");
		fgets(NAPNICK, sizeof(NAPNICK)-1, stdin);
		chomp(NAPNICK);
	}
	while (!*NAPPASS)
	{
		fprintf(stdout, "Enter the desired password : ");
		fgets(NAPPASS, sizeof(NAPPASS)-1, stdin);
		chomp(NAPPASS);
	}
	fprintf(stdout, "Enter the desired dataport (enter for default) : ");
	fgets(NAPPORT, sizeof(NAPPORT)-1, stdin);
	chomp(NAPPORT);
	fprintf(stdout, "Enter the desired servername (enter for none): ");
	fgets(NAPSERVER, sizeof(NAPSERVER)-1, stdin);
	chomp(NAPSERVER);
	if (*NAPSERVER)
	{
		fprintf(stdout, "Enter the desired server port (enter for 8888) : ");
		fgets(NAPSPORT, sizeof(NAPSPORT)-1, stdin);
		chomp(NAPSPORT);
		if (!*NAPSPORT)
			strcpy(NAPSPORT, "8888");
		if (my_atol(NAPSPORT) == 8875)
			meta = 1;
	}

	fp = fopen("//c/autoexec.bat", "a+");
	if (!fp)
		return;

	fprintf(fp, "REM setup of various variables for TekNap\r\n");
	fprintf(fp, "SET HOME=%s\r\n", HOME);
	fprintf(fp, "SET NAPNICK=%s\r\n", NAPNICK);
	fprintf(fp, "SET NAPPASS=%s\r\n", NAPPASS);
	if (*NAPPORT)
		fprintf(fp, "SET NAPPORT=%s\r\n", NAPPORT);
	if (*NAPSERVER)
		fprintf(fp, "SET NAPSERVER=%s:%s:%s:%s:%d\r\n", NAPSERVER, NAPSPORT, NAPNICK, NAPPASS, meta);
	fclose(fp);
	fprintf(stdout, "\r\n\r\nPlease reboot the computer in order for the changes made to take effect\r\n");
	exit(1);
}
#endif

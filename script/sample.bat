@echo off
REM this is an example of what to add to the AUTOEXEC.BAT file
REM to get a better working TekNap32
REM copy cygwindevo.dll to c:\windows
REM mkdir c:\TekNap and copy TekNap32.exe to c:\TekNap


REM set the server we want to use by default. Replace nick and pass
REM with the desired nickname and password. 
REM Here's the tricky part. Currently this is setup for a direct login to
REM the server. But if you need to use the META server, you change the 8888
REM to the port number of the META server and replace the 0 with a 1.
REM How can you tell if this is needed? telnet server.name portnumber
REM if you get a "ip:port" and an immediate disconnect you need to use the
REM META server.
set NAPSERVER=bitchx.dimension6.com:8888:nick:pass:0
REM set NAPSERVER=bitchx.dimension6.com:8875:nick:pass:1


REM set the default nickname. There is also a default password.
set NAPNICK=luna
REM set NAPPASS=password


REM this is so that TekNap32 knows where to find it's teknap.rc file and where
REM to download any files to. Make sure c:\TekNap exists. Also notice that this
REM is a posix style path.
set HOME=//c/TekNap

REM if you like, instead of changing your autoexec.bat, rename this file to
REM something more appropriate and remove the REM from the following line.
REM c:\TekNap\TekNap.exe


#!/bin/sh
# This is my autogen script. I knwo there is a "better" one out there but this
# works for me.

echo Running aclocal...
aclocal
echo Running autoheader...
autoheader
echo Running autoconf...
autoconf

if test -z "$*"; then
  echo Running ./configure with no arguments...
else
  echo Running ./configure $*... 
fi
./configure $*

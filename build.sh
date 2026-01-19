#!/bin/sh

CC="clang"
DEBUG=1

COMPILERS="clang gcc cc"

SOURCES="main_sdl3.c"
OPT_FLAGS="-O0 -g"
WARN_FLAGS="-Wall -Wextra -Werror"
LINK_FLAGS="-lSDL3"
OUTPUT_NAME="blanker"

for arg in "$@"; do
    if [ "$arg" = "clang" ]; then CC="clang"; fi
    if [ "$arg" = "gcc" ]; then CC="gcc"; fi
    if [ "$arg" = "release" ]; then DEBUG=0; fi
    if [ "$arg" = "debug" ]; then DEBUG=1; fi
done

if [ ! -x $(which "$CC") ]; then
   echo "$CC not found attempting alternatives"
   found=0
   for cc in "$COMPILERS"; do
       if [ -x $(which "$cc") ]; then
           CC="$cc"
           found=1
           echo "$cc found"
           break
       fi
   done

   if [ found -eq 1 ]; then
        echo "no supported C compiler found"
        exit 1
   fi
fi

echo "Settings:"
echo "  CC=$CC"
if [ $DEBUG -eq 1 ]; then
    OPT_FLAGS="-O0 -g"
    echo "  BUILD_TYPE=debug"
else
    OPT_FLAGS="-O2"
    echo "  BUILD_TYPE=release"
fi

set -x
$CC $SOURCES $OPT_FLAGS $WARN_FLAGS $LINK_FLAGS "-DDEBUG=$DEBUG" -o "$OUTPUT_NAME"

#!/bin/bash
GREEN="\033[1;32m"
RED="\033[1;31m"
WHITE="\033[1;37m"
YELLOW="\033[1;33m"
BLUE="\033[1;34m"
DONE="\033[0m"

OK=$GREEN
FAIL=$RED
WARN=$YELLOW
TITLE=$BLUE

printf $TITLE"Compiling MapGraphics...$DONE\n"

cd MapGraphics
qmake -qt=4
make clean
make

printf $TITLE"Compiling XcMap...$DONE\n"

cd ..
qmake -qt=4
make clean
make


#!/bin/sh

if [ -z $MF_ROOT ]; then
	echo "MF_ROOT is not defined"
	exit 1
fi

if [ ! -d "$MF_ROOT/bigworld" ]; then
	echo "$MF_ROOT doesn't appear to be a valid (no bigworld subdirectory found)"
	exit 1
fi

TOOLS_ROOT=$MF_ROOT/bigworld/tools/server/misc

case $MF_CONFIG in
	Hybrid) BINDIR=$MF_ROOT/bigworld/bin/Hybrid;;
	Release) BINDIR=$MF_ROOT/bigworld/bin/Release;;
	Debug) BINDIR=$MF_ROOT/bigworld/bin/Debug;;
	*) BINDIR=$MF_ROOT/bigworld/bin/Hybrid;
		echo MF_CONFIG is not set. Defaulting to Hybrid;;
esac

if test $# -lt 1 -o "X$1" = "X--help"; then
	echo "Usage: start [hold] {world|cellapp|baseapp|cellappmgr|baseapp|dbmgr|loginapp|bots}"
	echo "        Start the given process in an xterm."
	echo "        hold: Do not close terminal when process finishes."
	exit
fi

precmd=
cmd=

# For backwards compatibility - no longer used.
if [ X$1 = Xterm ]; then
	shift
fi

if [ X$1 = Xhold ]; then
	cmd="xterm -hold -sl 1000 -geometry 100x12 -e "
	shift
else
	cmd="xterm -sl 1000 -geometry 120x9 -e "
fi


case $1 in
	world)
		precmd="export MF_CONFIG=$MF_CONFIG; "
		cmd="${cmd}$TOOLS_ROOT/world"
	;;
	cellappmgr)
		precmd="cd $BINDIR; "
		cmd="${cmd}./cellappmgr"
	;;
	cellapp)
		precmd="cd $BINDIR; "
		cmd="${cmd}./cellapp"
	;;
	baseappmgr)
		precmd="cd $BINDIR; "
		cmd="${cmd}./baseappmgr"
	;;
	baseapp)
		precmd="cd $BINDIR; "
		cmd="${cmd}./baseapp"
	;;
	loginapp)
		precmd="cd $BINDIR; "
		cmd="${cmd}./loginapp"
	;;
	dbmgr)
		precmd="cd $BINDIR; "
		cmd="${cmd}./dbmgr"
	;;
    bots)
		precmd="cd $BINDIR; "
		cmd="${cmd}./bots"
	;;
	*)
		echo "Don't know how to start" $1
		exit
	;;
esac

/bin/bash -c "$precmd $cmd" < /dev/null &

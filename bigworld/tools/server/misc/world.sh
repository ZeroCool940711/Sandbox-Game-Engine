#!/bin/sh

if [ -z $MF_ROOT ]; then
	echo "MF_ROOT is not defined"
	exit 1
fi

if [ ! -d "$MF_ROOT/bigworld" ]; then
	echo "$MF_ROOT doesn't appear to be a valid (no bigworld subdirectory found)"
	exit 1
fi

hostname > $HOME/.last_world_machine

case $MF_CONFIG in
	Hybrid) BINDIR=$MF_ROOT/bigworld/bin/Hybrid;;
	Release) BINDIR=$MF_ROOT/bigworld/bin/Release;;
	Debug) BINDIR=$MF_ROOT/bigworld/bin/Debug;;
	*) BINDIR=$MF_ROOT/bigworld/bin/Hybrid;
		echo MF_CONFIG is not set. Defaulting to Hybrid;;
esac

cd $BINDIR
./dbmgr --prefix DB: &
./cellappmgr --prefix CM: &
./baseappmgr --prefix BM: &
./loginapp --prefix LG:
echo LoginApp done. Waiting for other processes
wait

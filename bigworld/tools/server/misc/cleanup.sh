#!/bin/sh

function killifthere () {
	killist=`ps -C $2 h -o uid,pid | awk /$UID/'{print $2}' | xargs`
	if [ X"$killist" != X ]; then
		echo Killing stray $2\'s with $1:  $killist
		kill $1 $killist
		gotany=1
	fi;
}

echo Cleaning up after system shutdown...

gotany=0

killifthere -TERM cellappmgr
killifthere -TERM cellapp
killifthere -TERM baseapp
killifthere -TERM baseappmgr
killifthere -TERM loginapp
killifthere -TERM bots

sleep 2
killifthere -TERM dbmgr

if [ $gotany -eq 0 ]; then
	echo System shut down cleanly.
	exit
fi

echo System did not shut down cleanly.
sleep 5

gotany=0

killifthere -KILL cellappmgr
killifthere -KILL cellapp
killifthere -KILL baseapp
killifthere -KILL baseappmgr
killifthere -KILL loginapp
killifthere -KILL bots

sleep 2
killifthere -KILL dbmgr

if [ $gotany -eq 0 ]; then
	echo System shut down ok after SIGTERM\'s.
	exit
fi

echo "System required SIGKILL's to shut down! (this is bad)"



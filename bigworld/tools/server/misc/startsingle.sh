#!/bin/sh

# This script starts a single-machine server

# Still accept one argument for backwards compatibility
if [ $# -gt 1 ]; then
	echo "Usage: startsingle.sh"
	echo "        Starts the server locally in xterm windows."
	exit 1
fi

# Comment this if you do not want the xterms to stay up after "Stop the World"
HOLD=hold

# Set this to anything to have a single world terminal
# SINGLE_WORLD_TERMINAL = 1

if [ X$SINGLE_WORLD_TERMINAL != X ]; then
./start.sh term $HOLD world
else
./start.sh term $HOLD cellappmgr
./start.sh term $HOLD baseappmgr
./start.sh term $HOLD loginapp
./start.sh term $HOLD dbmgr
fi
sleep 1
./start.sh term $HOLD cellapp
./start.sh term $HOLD baseapp

# Set this if you want to renice all components
# RENICE_PRIORITY=19

if [ X$RENICE_PRIORITY != X ]; then
	echo renicing components to $RENICE_PRIORITY
	sudo ./renice_all.sh -$RENICE_PRIORITY cellapp
	sudo ./renice_all.sh -$RENICE_PRIORITY cellmgr
	sudo ./renice_all.sh -$RENICE_PRIORITY baseapp
	sudo ./renice_all.sh -$RENICE_PRIORITY baseappmgr
	sudo ./renice_all.sh -$RENICE_PRIORITY loginapp
	sudo ./renice_all.sh -$RENICE_PRIORITY dbmgr
fi

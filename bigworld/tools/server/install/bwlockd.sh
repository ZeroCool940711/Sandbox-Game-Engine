#!/bin/sh
#
# Redhat service management information
# description: The bwlockd daemon maintains locks for editing world chunks in WorldEditor.
# processname: bwlockd
# chkconfig: 2345 25 25

SOURCE_SCRIPT_NAME=bwlockd.sh
SCRIPT_NAME=bw_lockd
EXEC=bwlockd.py
INSTALL_DIR=/etc/init.d
LOG_OUTPUT_PATH=/var/log/bigworld/bwlockd.log
PORT=8168
BIND_IP=0.0.0.0

case "$1" in

  start)
	. $INSTALL_DIR/bw_functions
	if [[ -z $BW_TOOLS_DIR ]]; then
		echo "Could not get BW_TOOLS_DIR from bw_functions"
		exit 1
	fi
	if [[ -z $BW_TOOLS_PIDDIR ]]; then
		echo "Could not get BW_TOOLS_PIDDIR from bw_functions"
		exit 1
	fi

	BWLOCKD_HOME=$BW_TOOLS_DIR/../bwlockd
	PIDFILE="$BW_TOOLS_PIDDIR/$SCRIPT_NAME.pid"
	
	echo -n "Starting $SCRIPT_NAME: "

	cd $BWLOCKD_HOME
	if [ $? != 0 ]; then
		echo "Could not chdir into bwlockd home: $BWLOCKD_HOME"
		bw_print_failure
		exit 1
	fi
	
	# Check for an existing service running
	bw_is_running "$PIDFILE" "bwlockd.py --daemon" $BW_TOOLS_USERNAME
	if [ $? != 0 ]; then
		bw_print_failure
		exit 1
	fi

	cd $BWLOCKD_HOME

	su $BW_TOOLS_USERNAME -c "./$EXEC --daemon --pid $PIDFILE \
				-o $LOG_OUTPUT_PATH \
				--bind-ip=$BIND_IP --port=$PORT" 
	if [ $? != 0 ]; then
		bw_print_failure
	else
		bw_print_success
	fi
	;;

  stop)
	. $INSTALL_DIR/bw_functions
	BWLOCKD_HOME=$BW_TOOLS_DIR/../bwlockd
	PIDFILE="$BW_TOOLS_PIDDIR/$SCRIPT_NAME.pid"

 	echo -n "Stopping $SCRIPT_NAME: "

	cd $BWLOCKD_HOME
	if [ $? != 0 ]; then
		echo "Could not chdir into bwlockd home: $BWLOCKD_HOME"
		bw_print_failure
		exit 1
	fi

	if [[ ! -e $PIDFILE ]]; then
		bw_print_failure
		echo "$SCRIPT_NAME not running or pid file '$PIDFILE' missing"
		RETVAL=1
	else
		if kill -s INT `cat $PIDFILE` > /dev/null; then
			bw_print_success
			RETVAL=0
		else
			bw_print_failure
			echo "could not kill process `cat $PIDFILE`"
			rm $PIDFILE
			RETVAL=1
		fi
	fi
	;;

  status)
	. $INSTALL_DIR/bw_functions
	PIDFILE="$BW_TOOLS_PIDDIR/$SCRIPT_NAME.pid"

	echo -n "Status of $SCRIPT_NAME: "
	if [[ ! -e $PIDFILE ]]; then
		echo "stopped"
		RETVAL=0
	else
		# Extract the PID from the PID file 
		PID=`head -n 1 $PIDFILE 2>/dev/null`
		if [ $? != 0 ]; then 
			echo "Unable to read PID from '$PIDFILE'"
			RETVAL=1
		else

			# Check if the PID is in the process list
			ps -p $PID > /dev/null 2>&1
			if [ $? == 0 ]; then
				echo "running"
				RETVAL=0
			else
				echo "pid file exists, but no process running as $PID"
				RETVAL=1
			fi
		fi
	fi
	;;

  restart|reload)
	$0 stop
	RETVAL=$?
	$0 start
	RETVAL=$[ $RETVAL + $? ]
	;;

  install)
  	$0 remove

  	if [[ ! -f bw_functions.sh ]]; then
		echo "Could not find bw_functions.sh"
		exit 1
	fi
	
  	if [[ ! -e $INSTALL_DIR/bw_functions ]]; then
		ln -sf `pwd`/bw_functions.sh $INSTALL_DIR/bw_functions
	fi
	
  	if [[ -x $INSTALL_DIR/$SCRIPT_NAME ]]; then
		$INSTALL_DIR/$SCRIPT_NAME stop
	fi
	
	cp $SOURCE_SCRIPT_NAME $INSTALL_DIR/$SCRIPT_NAME
	
	# install into runlevel script dirs
	if [ -x /usr/sbin/update-rc.d ]; then
		/usr/sbin/update-rc.d $SCRIPT_NAME defaults 25
	elif [ -x /sbin/chkconfig ]; then
		/sbin/chkconfig --add $SCRIPT_NAME
	else
		ln -s /etc/init.d/$SCRIPT_NAME /etc/rc1.d/K25$SCRIPT_NAME
		ln -s /etc/init.d/$SCRIPT_NAME /etc/rc2.d/S25$SCRIPT_NAME
		ln -s /etc/init.d/$SCRIPT_NAME /etc/rc3.d/S25$SCRIPT_NAME
		ln -s /etc/init.d/$SCRIPT_NAME /etc/rc4.d/S25$SCRIPT_NAME
		ln -s /etc/init.d/$SCRIPT_NAME /etc/rc5.d/S25$SCRIPT_NAME
		ln -s /etc/init.d/$SCRIPT_NAME /etc/rc6.d/K25$SCRIPT_NAME
	fi
	
  	$INSTALL_DIR/$SCRIPT_NAME start
	RETVAL=$?
	;;

  remove)
  	$0 stop
	RETVAL=$?
	rm -f $INSTALL_DIR/$SCRIPT_NAME
	# remove from runlevel script dirs
	if [ -x /usr/sbin/update-rc.d ]; then
		/usr/sbin/update-rc.d -f $SCRIPT_NAME remove 
	elif [ -x /sbin/chkconfig ]; then
		/sbin/chkconfig --del $SCRIPT_NAME
	else
		rm -f /etc/rc[1-6].d/[SK]25$SCRIPT_NAME 
	fi
	
	;;
  *)
  	echo "Usage: $0 {start|stop|status|restart|reload|install|remove}"
	exit 1
	;;
esac

exit $RETVAL



#! /bin/sh

# Redhat service management information
# chkconfig: 2345 90 40
# description: Logs component messages from BigWorld services.

if [ -f "/etc/redhat-release" ]; then
        . /etc/init.d/functions
fi
. /etc/init.d/bw_functions


PROCNAME=message_logger
ML_DIR="$BW_TOOLS_DIR/message_logger"
BIN_INSTALL="$ML_DIR/$PROCNAME"
CONFFILE="$ML_DIR/message_logger.conf"
# LOGDIR is the directory that stdout / stderr should be logged to.
# this is also specified in the install script install_tools.py
# should this value ever change, update it in the install script.
LOGDIR=/var/log/bigworld

# Set this to be an email address to activate summaries of logs mailed to that
# address
#LOGROTATE_SUMMARY_MAIL_TO=programmers@example.com

# The summary mail From field.
LOGROTATE_SUMMARY_MAIL_FROM=bwtools@example.com

# The SMTP mail host to send through
LOGROTATE_SUMMARY_MAIL_HOST="mail"

# The summary mail subject prefix, defaults to [bwlog-summary]
#LOGROTATE_SUMMARY_MAIL_SUBJECT_PREFIX=

# The summary flags
LOGROTATE_SUMMARY_FLAGS="Psm"

# The summary severities
LOGROTATE_SUMMARY_SEVERITIES="wec"

# The summary period - this defaults to 1 day
#LOGROTATE_SUMMARY_DAYS=1

#Set this for specifying the summary period in hours instead of days
#LOGROTATE_SUMMARY_HOURS=

# Set this to only perform summary on specific user, otherwise all users
#LOGROTATE_SUMMARY_USER=

# Set this to set the minimum count for summaries 
#LOGROTATE_SUMMARY_MINCOUNT=1


if [ ! -d "$ML_DIR" ]; then
	echo "ERROR: '$ML_DIR' doesn't appear to exist."
	echo "       Is /etc/bigworld.conf up to date?"
	exit 1
fi

# Read the message logger configuration file and extract the
# log directory / pid file from it.
if [ ! -f "$CONFFILE" ]; then
	echo "ERROR: Unable to find configuration file '$CONFFILE'."
	exit 1
fi
ML_LOGDIR=`grep logdir $CONFFILE | sed -r -e "s/$BW_VAL_REGEX/\1/"`
if [ ! -d $ML_LOGDIR ]; then
	echo "ERROR: '$ML_LOGDIR' does not appear to be a directory."
	exit 1
fi
PIDFILE="$ML_LOGDIR/pid"

ML_DEFAULT_ARCHIVE=`grep default_archive $CONFFILE | sed -r -e "s/$BW_VAL_REGEX/\1/"`
if [ -z $ML_DEFAULT_ARCHIVE ]; then
	echo "ERROR: No 'default_archive' specified in '$CONFFILE'."
	exit 1
fi


case "$1" in

  start)
	# Check for an existing service running
	bw_is_running "$PIDFILE" "message_logger --daemon" "$BW_TOOLS_USERNAME"
	if [ $? != 0 ]; then
		if [ -f "/etc/debian-version" ]; then
			echo -n "already running, rolling logs. "
		else
			echo "already running, rolling logs. "
		fi
		read pid < $PIDFILE
		if [ $? != 0 ]; then
			if [ -f "/etc/debian-version" ]; then
				bw_print_failure
			fi
			echo "Unable to read PID from $PIDFILE"
			echo -n "A message_logger process is already running, but its pid "
			echo -n "file is missing.  Please check that the logdir "
		    echo -n "configuration option in $CONFFILE is set "
			echo "correctly."
			exit 1
		fi

		kill -HUP $pid 2> /dev/null
		if [ $? != 0 ]; then
			if [ -f "/etc/debian-version" ]; then
				bw_print_failure
			fi
			echo "Failed to send SIGHUP to process $pid"
			exit 1
		fi

		if [ -f "/etc/debian-version" ]; then
			bw_print_success
		fi
		exit 0
	fi

	LOGFILE="$LOGDIR/message_logger.log"
	if [ -f "/etc/redhat-release" ]; then
		echo -n "Starting bw_message_logger: "
		daemon su " -c \"$BIN_INSTALL --daemon \
			-o \"$LOGFILE\" \
			-e \"$LOGFILE\"\" $BW_TOOLS_USERNAME"
		echo
	else
		su -c "$BIN_INSTALL --daemon \
			-o \"$LOGFILE\" \
			-e \"$LOGFILE\"" $BW_TOOLS_USERNAME
	fi

	RETVAL=$?
	if [ $RETVAL != 0 ]; then
		if [ -f "/etc/debian-version" ]; then
			bw_print_failure
		fi
		exit 1
	else

		bw_has_started "$PIDFILE" "message_logger --daemon" "$BW_TOOLS_USERNAME"
		if [ $? == 0 ]; then
			if [ -f "/etc/debian-version" ]; then
				bw_print_failure
			fi
			echo "message_logger doesn't appear to have started."
			echo "Check '$LOGFILE' for errors"
			exit 1
		fi
	fi

	if [ -f "/etc/debian-version" ]; then
		bw_print_success
	fi
	RETVAL=0
	;;


  stop)

	bw_stop_process "$PROCNAME" \
					"$PIDFILE" \
					"message_logger --daemon" \
					"$BW_TOOLS_USERNAME"
	RETVAL=$?
	;;

  summarise)
  	RETVAL=0
	cd "$ML_DIR"
	if [[ -z $LOGROTATE_SUMMARY_MAIL_TO ]]; then
		# not configured
		RETVAL=0
		break
	else
		# do all the compulsory configured options
		FLAGS="--mail-to=\"$LOGROTATE_SUMMARY_MAIL_TO\" \
			--mail-from=\"$LOGROTATE_SUMMARY_MAIL_FROM\" \
			--mail-host=$LOGROTATE_SUMMARY_MAIL_HOST" \
			--summary-flags=$LOGROTATE_SUMMARY_FLAGS" \
			--severities=$LOGROTATE_SUMMARY_SEVERITIES"

		if [[ ! -z "$LOGROTATE_SUMMARY_MAIL_SUBJECT_PREFIX" ]]; then
			FLAGS="$FLAGS --mail-summary=\"$LOGROTATE_SUMMARY_MAIL_SUBJECT_PREFIX\""
		fi


		if [[ ! -z $LOGROTATE_SUMMARY_HOURS ]]; then
			FLAGS="$FLAGS --hours $LOGROTATE_SUMMARY_HOURS"
		else
			if [[ ! -z $LOGROTATE_SUMMARY_DAYS ]]; then
				FLAGS="$FLAGS -- days $LOGROTATE_SUMMARY_DAYS"
			fi
		fi
		
		if [[ -z $LOGROTATE_SUMMARY_USER ]]; then
			FLAGS="$FLAGS --all-users"
		else
			FLAGS="$FLAGS -u $LOGROTATE_SUMMARY_USER"
		fi

		if [[ ! -z $LOGROTATE_SUMMARY_MINCOUNT ]]; then
			FLAGS="$FLAGS --summary-min=$LOGROTATE_SUMMARY_MINCOUNT"
		fi

		echo -n "Summarizing logs: "
		if python mlsum.py $FLAGS; then
			bw_print_success
		else
			bw_print_failure
		fi
	fi
  	;;

  logrotate)
  	$0 summarise

	RETVAL=0
	echo -n "Rolling message_logger logs: "
	cd "$ML_DIR"

	# Send a SIGHUP to message_logger
	python mlroll.py "$ML_LOGDIR"
	if [ $? != 0 ]; then
		echo "Failed to mlroll '$ML_LOGDIR'."
		RETVAL=$[ $RETVAL + 1 ]
	fi

	# Archive all the old logs
	nice -n 19 python mltar.py -cvrd --all-users --hours-old 23 "$ML_LOGDIR"
	if [ $? != 0 ]; then
		echo "Failed to mltar '$ML_LOGDIR'."
		RETVAL=$[ $RETVAL + 1 ]
	fi

	# The logrotate step will most often be run as root, so chown the
	# newly created archive back to the user that owns the rest of the logs.
	if [ -f $ML_DEFAULT_ARCHIVE ]; then
		chown "$BW_TOOLS_USERNAME:$BW_TOOLS_GROUPNAME" "$ML_DEFAULT_ARCHIVE"
		if [ $? != 0 ]; then
			echo "Failed to chown '$ML_DEFAULT_ARCHIVE' to user '$BW_TOOLS_USERNAME'"
			RETVAL=$[ $RETVAL + 1 ]
		fi
	fi

	# Remove any unpacked / 'dirty' logs
	python mlrm.py --dirty
	if [ $? != 0 ]; then
		echo "Failed to remove dirty files in '$ML_LOGDIR'."
		RETVAL=$[ $RETVAL + 1 ]
	fi

	# Do a regular mlrm pass over the logdir to blow away any empty user dirs
	python mlrm.py -a --hours 24
	if [ $? != 0 ]; then
		echo "Failed to cleanup empty user dirs in '$ML_LOGDIR'."
		RETVAL=$[ $RETVAL + 1 ]
	fi

	if [ $RETVAL != 0 ]; then
		bw_print_failure
		RETVAL=1
	fi
	;;

  status)

	echo -n "Status of $PROCNAME: "
	if [[ ! -e $PIDFILE ]]; then
		echo "stopped"
		RETVAL=1
	else
		# Extract the PID from the pid file
		PID=`head -n 1 $PIDFILE 2>/dev/null`
		if [ $? != 0 ]; then
			echo "Unable to read pid from '$PIDFILE'."
			RETVAL=1
		else

			# Check if the pid is in the process list
			ps -p $PID > /dev/null 2>&1
			if [ $? == 0 ]; then
				echo "running"
				RETVAL=0
			else
				echo "pid file exists, but no process running as `head -n 1 $PIDFILE`"
				RETVAL=1
			fi
		fi
	fi
	;;


  restart)

	$0 stop
	RETVAL=$?
	$0 start
	RETVAL=$[ $RETVAL + $? ]
	;;

  *)
	echo "Usage: $PROCNAME {start|stop|status|restart}"
	exit 1
	;;
esac

exit $RETVAL

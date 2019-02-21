#!/bin/sh
#
# description: The bwmachined daemon reports machine usage statistics, \
#		including processes MicroForte is interested in.
# processname: bwmachined
# chkconfig: 2345 80 50
# description: BigWorld server - machine monitoring daemon \
#             http://www.bigworldtech.com

SBINDIR=/usr/sbin
EXE=$SBINDIR/bwmachined2
BASE=`basename $EXE`
ARGS=

# Note that any processes started by machined will also get this nice value
NICE=-10

# Check linux distribution redhat or debian
if [ -f /etc/debian_version ]; then
	PLOCK=/var/run/$BASE.pid
else
	PLOCK=/var/lock/subsys/$BASE
	. /etc/rc.d/init.d/functions
fi

do_debian_start()
{
	start-stop-daemon --start --quiet --exec /usr/bin/nice -- -n $NICE $EXE $ARGS
	RETVAL=$?
	ps aux | grep $EXE | grep -v grep | awk '{ print $2 }' > $PLOCK
	echo "."
}

do_redhat_start()
{
	daemon nice -n $NICE $EXE $ARGS
	RETVAL=$?
	echo
	[ $RETVAL -eq 0 ] && touch $PLOCK
}

do_debian_stop()
{
	start-stop-daemon --stop --pidfile $PLOCK --exec $EXE
	RETVAL=$?
	echo "."
}

do_redhat_stop()
{
	killproc $BASE
	RETVAL=$?
	echo
	[ $RETVAL -eq 0 ] && rm -f $PLOCK
}

case "$1" in
  start)
	echo -n "Starting $BASE: "
	echo 16777216 > /proc/sys/net/core/rmem_max
	echo 1048576 > /proc/sys/net/core/wmem_max
	echo 1048576 > /proc/sys/net/core/wmem_default
	echo core.%e.%h.%p > /proc/sys/kernel/core_pattern
	ulimit -c unlimited
	if [ -f /etc/debian_version ]; then
		do_debian_start
	else
		do_redhat_start
	fi
	;;
  stop)
  	echo -n "Stopping $BASE: "
	if [ -f /etc/debian_version ]; then
		do_debian_stop
	else
		do_redhat_stop
	fi
	;;
  status)
	status $BASE
	RETVAL=$?
	;;
  restart|reload)
	$0 stop
	RETVAL=$?
	$0 start
	RETVAL=$[ $RETVAL + $? ]
	;;

  install)
	$0 remove
	shift 
 	awk -v v="$*" '{print} /^ARGS=/{print $0 "\"" v "\""}' < $0 > /etc/init.d/$BASE
	chmod +x /etc/init.d/$BASE
	
	# copy bwmachined2 binary
	mkdir  -p $SBINDIR
	cp $BASE $EXE

	# install into runlevel script dirs
	if [ -x /usr/sbin/update-rc.d ]; then # Debian system
		/usr/sbin/update-rc.d $BASE defaults 21
	elif [ -x /sbin/chkconfig ]; then # RedHat system
		/sbin/chkconfig --add $BASE
	else
		ln -s /etc/init.d/$BASE /etc/rc1.d/K21$BASE
		ln -s /etc/init.d/$BASE /etc/rc2.d/S21$BASE
		ln -s /etc/init.d/$BASE /etc/rc3.d/S21$BASE
		ln -s /etc/init.d/$BASE /etc/rc4.d/S21$BASE
		ln -s /etc/init.d/$BASE /etc/rc5.d/S21$BASE
		ln -s /etc/init.d/$BASE /etc/rc6.d/K21$BASE
	fi
	/etc/init.d/$BASE start
	;;

  remove)
	if [ -f /etc/init.d/$BASE ]; then
		$0 stop
		rm -f $EXE
		if [ -x /usr/sbin/update-rc.d ]; then
			/usr/sbin/update-rc.d -f $BASE remove
		elif [ -x /sbin/chkconfig ]; then
			/sbin/chkconfig --del $BASE
		else
			rm -f /etc/rc[1-6].d/[SK][0-9][0-9]$BASE
		fi
	  	rm -f /etc/init.d/$BASE
	fi
	RETVAL=0
	;;

  *)
	echo "Usage: $BASE.sh {start|stop|status|restart|reload|install|remove}"
	exit 1
	;;
esac

exit $RETVAL

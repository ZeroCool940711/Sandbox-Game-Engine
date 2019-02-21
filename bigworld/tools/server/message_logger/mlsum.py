#!/usr/bin/env python
"""
Script to summarise logs for the last user-defined time interval.
"""

import bwsetup; bwsetup.addPath( ".." )

DATE_FORMAT="%a %d %b %Y %H:%M:%S"

import os
import datetime
import optparse
import subprocess
import smtplib
import cStringIO

import util
import mlcat

def main():
	optionParser = optparse.OptionParser( 
		usage="%prog [options] tools_dir [logdir]",
		description="Show summaries of log output of the last user-defined "\
			"time interval. " )

	optionParser.add_option( "--days", 
		dest="days",
		action="store",
		type="int",
		help="the interval, in days, (default %default)",
		default="1"
	)


	optionParser.add_option( "--hours",
		dest="hours",
		action="store",
		type="int",
		help="the interval, in hours (overrides the --days option)",
		default=None
	)
	
	optionParser.add_option( "-u", "--uid",
		dest="uid",
		action="store",
		type="string",
		help="the user to summarise for (default current user)",
		default=None
	)

	optionParser.add_option( "--all-users",
		dest="allUsers",
		action="store_true",
		default=False,
		help="run summary for all users" )

	optionParser.add_option( "--mail-to",
		dest="mailToAddrs",
		metavar="MAIL_TO_ADDRS",
		action="store",
		help="if set, mails the log to the given mail addresses",
		default=None
	)

	optionParser.add_option( "--mail-from",
		dest="mailFromAddr",
		metavar="ADDR",
		action="store",
		help="if set, then mail sent has the given from address "\
			"(default '%default')",
		default="bwlog"
	)

	optionParser.add_option( "--summary-flags",
		dest="summaryFlags",
		metavar="FLAGS",
		action="store",
		help="the summary flags, same as mlcat.py --summary "\
			"(default='%default')",
		default="Sm" 
	)
	
	optionParser.add_option( "--summary-min", 
		dest="summaryMin",
		type="int", 
		default=1,
		help="The minimum count to include in a summary" )

	optionParser.add_option( "--mail-subject-prefix",
		dest="mailSubjectPrefix",
		metavar="PREFIX",
		action="store",
		help="if set, defines the mail subject prefix "
			"(default '%default')",
		default="[bwlog-summary]"
	)

	optionParser.add_option( "--mail-host",
		dest="mailSmtpHost",
		metavar="SMTPHOST",
		action="store",
		default="localhost",
		help="the SMTP host to use when sending mail summaries "
			"(default '%default')"
	)

	optionParser.add_option( "--severities",
		dest="severities",
		metavar="SEVERITIES",
		action="store",
		help="the severities mask, same as in mlcat.py --severities",
		default=None
	)


	mlog, kwargs, options, args = util.basicInit( optionParser )
	output = cStringIO.StringIO()
	
	toTime = datetime.datetime.now()

	if not options.hours is None:
		interval = datetime.timedelta( seconds = options.hours * 60 )
	else:
		interval = datetime.timedelta( days = options.days )

	fromTime = toTime - interval
	if options.severities:
		mask = 0
		invert = False

		for c in options.severities:
			if c == '^':
				invert = True
			elif c in "012345678":
				mask += 1 << int( c )
			elif c in mlcat.SEVERITY_FLAGS:
				mask += 1 << mlcat.SEVERITY_FLAGS[ c ]
			else:
				log.error( "Unsupported severity level: %s", c )

		if invert:
			mask = ((1 << log.NUM_MESSAGE_PRIORITY) - 1) & ~mask

		kwargs[ "severities" ] = mask

	# name -> uid
	allUsers = mlog.getUsers()
	allUsersReverse = dict( (uid, name) for (name, uid) in allUsers.items() )

	users = []
	if options.allUsers:
		users = allUsers.keys()
	elif options.uid in allUsersReverse:
		users = [allUsersReverse[options.uid]]
	elif options.uid in allUsers:
		users = [options.uid]
	users.sort()

	for user in users:
		kwargs['uid'] = allUsers[user]
		print >> output, "==== Summary for user %s ====\n" % user
		mlcat.summary( mlog, options.summaryFlags, options.summaryMin,
			kwargs, False, output )
		print >> output, "\n\n"
	
	if not options.mailToAddrs is None:
		headers = dict()
		if not options.mailFromAddr is None:
			headers["From"] = options.mailFromAddr
		
		hours = interval.seconds / 60
		if hours != 1:
			hoursPlural = "s"
		if interval.days:
			daysPlural = ""
			if interval.days != 1:
				daysPlural = "s"
			hoursPlural = ""
			
			if hours:
				intervalString = "%d day%s %d hour%s" % (interval.days, 
					daysPlural, hours, hoursPlural )
			else:
				intervalString = "%d day%s" % (interval.days, daysPlural )
		else:
			intervalString = "%d hour%s" % (hours, hoursPlural)

		subject = "Summary for last %s" % (intervalString)

		headers["Subject"] = "%s %s" % (options.mailSubjectPrefix, subject )
		headers["To"] = options.mailToAddrs

		msg = "%s\r\n\r\n%s" % ("\r\n".join( 
				"%s: %s" % (key, value) 
					for key, value in headers.items()), 
			output.getvalue()) 
		smtpServer = smtplib.SMTP( options.mailSmtpHost )
		
		smtpServer.sendmail( options.mailFromAddr, options.mailToAddrs, msg )
		smtpServer.quit()
	else:
		print output.getvalue()

	

if __name__ == "__main__":
	main()

#!/usr/bin/env python

import sys
import os
import time
import re

import bwlog
import util

import bwsetup; bwsetup.addPath( ".." )
from pycommon import log

DISPLAY_FLAGS = { "d": bwlog.SHOW_DATE,
				  "t": bwlog.SHOW_TIME,
				  "h": bwlog.SHOW_HOST,
				  "u": bwlog.SHOW_USER,
				  "i": bwlog.SHOW_PID,
				  "a": bwlog.SHOW_APPID,
				  "p": bwlog.SHOW_PROCS,
				  "s": bwlog.SHOW_SEVERITY,
				  "m": bwlog.SHOW_MESSAGE }

SEVERITY_FLAGS = { "t": log.MESSAGE_PRIORITY_TRACE,
				   "d": log.MESSAGE_PRIORITY_DEBUG,
				   "i": log.MESSAGE_PRIORITY_INFO,
				   "n": log.MESSAGE_PRIORITY_NOTICE,
				   "w": log.MESSAGE_PRIORITY_WARNING,
				   "e": log.MESSAGE_PRIORITY_ERROR,
				   "c": log.MESSAGE_PRIORITY_CRITICAL,
				   "h": log.MESSAGE_PRIORITY_HACK,
				   "s": log.MESSAGE_PRIORITY_SCRIPT }

# NOTE: We use 'stringOffset' instead of 'message' when calculating summaries
# based on log message because they are just as unique and much faster for
# calculating hash values.
SUMMARY_FLAGS = { "t": "time",
				  "h": "host",
				  "u": "username",
				  "i": "pid",
				  "a": "appid",
				  "p": "component",
				  "s": "severity",
				  "m": "stringOffset" }

TIME_FMT = "%a %d %b %Y %H:%M:%S"

USAGE = """%prog [options] [logdir]

Dump log output to the console.

By default, output is dumped in a `cat`-like manner.  With the -f switch, output
can be continuously dumped in a `tail -f`-like manner.

Specifying Times
================

The times for which output is dumped can be constrained using the --from, --to,
and --around switches.  The arguments to each command can either be a literal
date in the format 'Thu 09 Nov 2006 16:09:01', or a file, whose last modified
time will be used.

To search through recent log output, the --back switch can be used, which
accepts a number of seconds as an argument, and will search forwards to the
current time.  The time can also be given with an 'm', 'h', or 'd' suffix for
minutes, hours, and days respectively.

The log output for a particular server run can also be selected using the
--last-startup and --startup switches.  As expected, --last-startup will show
output from the most recent server run.  --startup expects an integer to be
passed in identifying a particular server run.  The IDs can be dumped using the
--show-startups switch.

Output Formatting
=================

The columns shown in the output can be manually configured using the -c switch,
which accepts a string of characters, each of which turns on a single column.
Note that the ordering of the output columns is fixed, and cannot be set by
passing flags to -c in a different order to the default.

The accepted flags are:

	d: SHOW_DATE
	t: SHOW_TIME
	h: SHOW_HOST
	u: SHOW_USER
	i: SHOW_PID
	a: SHOW_APPID
	p: SHOW_PROCS
	s: SHOW_SEVERITY
	m: SHOW_MESSAGE

--narrow is an alias for '-c tpasm'.

Filters
=======

Most of the filtering capabilities of WebConsole's LogViewer page are exposed
via this commandline interface too.  Supported filters are:

--message:     Include based on match against message text
--exclude:     Exclude based on match against message text
--pid:         Filter by PID
--appid:       Filter by AppID
--context:     Control number of context lines around matches
--severities:  Filter by list of severities -

                 0 or t: TRACE
                 1 or d: DEBUG
                 2 or i: INFO
                 3 or n: NOTICE
                 4 or w: WARNING
                 5 or e: ERROR
                 6 or c: CRITICAL
                 7 or h: HACK
                 8 or s: SCRIPT

               e.g. --severities 45 (just warnings and errors)
                    --severities we (same again)
                    --severities ^td (everything but trace and debug)

--procs:       Filter by comma-separated list of processes
               e.g. --procs CellApp,BaseApp (include BaseApp & CellApp output)
               e.g. --procs ^LoginApp,DBMgr (exclude LoginApp & DBMgr output)

--host:        Filter by a single hostname

The --message|--exclude options also accept filenames which should be a list of
strings (one per line) to include or exclude from output.

Summary Output
==============

Instead of displaying actual log output, a summary of search results can be
generated using the --summary option.  The argument to this option should be a
set of flags, the same as the ones accepted by -c/--cols above.  The flags
specify which columns are used in calculating the histogram.

One of the flags may be specified in uppercase, which indicates that the results
should be sorted by this column before being sorted by count.  For example,
--summary Sm will calculate a summary based on severity and message, and
results will be sorted by severity, then count.

The --summarymin switch can be used to specify the minimum count for which results
will be displayed.

Etc
===

For debugging purposes, the log's format string database can be dumped by
passing --strings.

A list of the nub addresses for all apps since the last server startup can be
generated with --addresses.

A list of server startup times can be generated using --show-startups.  This is
useful when combined with the --startup switch for selecting output from a
particular server run.

Addresses in log output can be translated into their app names automatically
using the --translate switch.

Format string interpolation can be controlled with the --interpolate switch.  By
default, pre-interpolation is used, which means format strings have their
arguments substituted in before matching against message text is performed.
This is so the default behaviour is as similar to `cat`'s as possible.  If you
are searching through large amounts of log data and are looking for a particular
log message, you can realise big speed improvements by using post interpolation.
""".rstrip()


def main():

	opt = util.getBasicOptionParser( USAGE )

	opt.add_option( "-f", "--follow", action = "store_true",
					help = "`tail -f`-like behaviour" )
	opt.add_option( "--summary", metavar = "COLS",
					help = "Generate a summary of results" )
	opt.add_option( "--summarymin", type = "int", default = 1,
					help = "The minimum count to include in a summary" )
	opt.add_option( "--no-progress-bar", action="store_false",
					dest = "showProgress", default=True,
					help = "Don't show a progress bar while summarising "
						"(default is to show)" )
	opt.add_option( "--interval", default = 1.0,
					type = "float",
					help = "Refresh interval when in follow mode" )
	opt.add_option( "-c", "--cols",
					help = "Set columns to be shown in output" )
	opt.add_option( "-s", "--last-startup", action = "store_true",
					help = "Only show logs after last server startup" )
	opt.add_option( "-n", "--narrow", action = "store_const", dest = "cols",
					const = "tpasm",
					help = "An alias for '-c tpsm'" )
	opt.add_option( "--from", dest = "ffrom", metavar = "TIME",
					help = "Specify start time" )
	opt.add_option( "--to", metavar = "TIME",
					help = "Specify end time" )
	opt.add_option( "--around", metavar = "TIME",
					help = "Specify time of interest" )
	opt.add_option( "--back", metavar = "SECONDS",
					help = "Specify amount of history to search" )
	opt.add_option( "--startup", type = "int",
					help = "Specify server run to show output for" )
	opt.add_option( "-C", "--context", type = "int", default = -1,
					metavar = "N",
					help = "Specify number of lines of context" )
	opt.add_option( "--strings", action = "store_true",
					help = "Dump the log's format string database" )
	opt.add_option( "--addresses", action = "store_true",
					help = "Dump nub addresses" )
	opt.add_option( "--show-startups", action = "store_true",
					help = "Show server start times" )
	opt.add_option( "--translate", action = "store_true",
					help = "Translate addresses in output into app names" )

	# Filter options
	opt.add_option( "-M", "--message", metavar = "PATT",
					help = "Pattern to match against in message for inclusion" )
	opt.add_option( "-X", "--exclude", metavar = "PATT",
					help = "Pattern to match against in message for exclusion" )
	opt.add_option( "-P", "--pid", type = "int",
					help = "Filter by PID" )
	opt.add_option( "--appid", type = "int",
					help = "Filter by AppID" )
	opt.add_option( "-i", "--insensitive", action = "store_true",
					help = "Use case-insensitive pattern matching" )
	opt.add_option( "-I", "--interpolate",
					help = "Specify interpolation stage (pre|post|none)" )
	opt.add_option( "-S", "--severities", metavar = "FLAGS",
					help = "Specify severities to match" )
	opt.add_option( "--procs", metavar = "LIST",
					help = "Specify processes to match" )
	opt.add_option( "-H", "--host",
					help = "Specify hostname to match" )

	mlog, kwargs, options, args = util.basicInit( opt )

	# If we're dumping format strings, do it now then bail
	if options.strings:
		ss = mlog.getStrings()
		for s in ss:
			print s,
		log.info( "\n* %d unique strings *", len( ss ) )
		return 0

	# If we're dumping server nub addresses, do it and bail
	if options.addresses:

		results = mlog.getUserLog( options.uid ).getNubAddresses()

		if results:

			print "%-12s %6s %-20s %s" % \
				  ("App", "PID", "Address", "Type")

			for name, pid, addr, nubtype in results:
				print "%-12s %6d %-20s %s" % (name, pid, addr, nubtype)

		return 0

	# If we're dumping startup times, do it and bail
	if options.show_startups:

		results = mlog.getUserLog( options.uid ).getAllServerStartups()

		for index, (entry, addr) in enumerate( results ):

			print "#%-3d %s" % (index,
								entry.format( bwlog.SHOW_DATE |
											  bwlog.SHOW_TIME )),

		return 0


	# Macro to process files of string patterns as appropriate
	def handlePatterns( opt ):
		if os.path.isfile( opt ):
			patts = [s.strip() for s in open( opt ).readlines()]
			return "(" + "|".join( patts ) + ")"
		else:
			return opt

	# Handle filter options
	if options.message:
		kwargs[ "message" ] = handlePatterns( options.message )
	if options.exclude:
		kwargs[ "exclude" ] = handlePatterns( options.exclude )
	if options.pid:
		kwargs[ "pid" ] = options.pid
	if options.appid:
		kwargs[ "appid" ] = options.appid
	if options.insensitive:
		kwargs[ "casesens" ] = 0

	if options.severities:

		mask = 0
		invert = False

		for c in options.severities:
			if c == '^':
				invert = True
			elif c in "012345678":
				mask += 1 << int( c )
			elif c in SEVERITY_FLAGS:
				mask += 1 << SEVERITY_FLAGS[ c ]
			else:
				log.error( "Unsupported severity level: %s", c )

		if invert:
			mask = ((1 << log.NUM_MESSAGE_PRIORITY) - 1) & ~mask

		kwargs[ "severities" ] = mask

	if options.procs:

		if options.procs[0] == '^':
			names = options.procs[1:]
			flip = True
		else:
			names = options.procs
			flip = False

		mask = sum( [1 << mlog.componentID( s ) for s in names.split( "," )] )

		if not flip:
			kwargs[ "procs" ] = mask
		else:
			allmask = (1 << len( mlog.getComponentNames() )) - 1
			kwargs[ "procs" ] = allmask & ~mask

	if options.host:
		try:
			kwargs[ "host" ] = options.host
		except KeyError:
			log.error( "Host %s does not exist in this log", options.host )
			return 1

	if options.context != -1:
		backContext = options.context
	else:
		backContext = 20
		options.context = 0

	if options.interpolate == "pre":
		kwargs[ "interpolate" ] = bwlog.PRE_INTERPOLATE
	elif options.interpolate == "post":
		kwargs[ "interpolate" ] = bwlog.POST_INTERPOLATE
	elif options.interpolate == "none":
		kwargs[ "interpolate" ] = bwlog.DONT_INTERPOLATE

	if options.cols:
		flags = sum( [DISPLAY_FLAGS[ c ] for c in options.cols] )
	else:
		flags = bwlog.SHOW_ALL

	# Find last server restart if required
	if options.last_startup:
		ul = mlog.getUserLog( options.uid )
		startup = ul.getLastServerStartup()
		if startup:
			kwargs[ "startaddr" ] = startup[ "addr" ]
		else:
			log.warning( "No server startup in log, starting from beginning" )

	# If looking for a particular server run, find its endpoints
	elif options.startup:

		startups = mlog.getUserLog( options.uid ).getAllServerStartups()

		try:
			startentry, startaddr = startups[ options.startup ]
		except IndexError:
			log.error( "Server startup #%d not found", options.startup )
			return 1

		kwargs[ "startaddr" ] = startaddr

		# If this wasn't the most recent server run, set the start of the next
		# run as the end address.
		if options.startup < len( startups ) - 1:
			kwargs[ "endaddr" ] = startups[ options.startup + 1 ][1]

	# Enable address translation if required
	if options.translate:
		mlog.addAddressTranslations( options.uid )

	# For --back
	elif options.back:

		groups = re.search( "(\d+)(\w?)", options.back ).groups()

		if not groups:
			log.error( "Could not parse history spec: %s", options.back )
			return 1

		num = int( groups[0] )

		if len( groups ) == 2:
			if groups[1].lower() == "m":
				num *= 60
			elif groups[1].lower() == "h":
				num *= 60 * 60
			elif groups[1].lower() == "d":
				num *= 60 * 60 * 24

		kwargs[ "start" ] = time.time() - num

	# For --around
	if options.around:

		if not parseTime( options.around, "start", kwargs ):
			return 1

		# Need to increment to get expected behaviour
		backContext += 1

		# Display backwards context first
		results, query = mlog.fetchContext( kwargs, max = backContext )
		for result in results:
			print result.format( flags ),

		return cat( mlog, flags, kwargs, max = backContext, query = query )

	else:
		if options.ffrom and not parseTime( options.ffrom, "start", kwargs ):
			return 1

		if options.to and not parseTime( options.to, "end", kwargs ):
			return 1

	if options.follow:
		return tail( mlog, flags, options.interval, kwargs, backContext )

	elif options.summary:
		return summary( mlog, options.summary, options.summarymin, kwargs,
			options.showProgress )

	else:
		# We check the context option here instead of at the top with the rest
		# of the filter options because it is used for different things in the
		# other modes
		if options.context:
			kwargs[ "context" ] = options.context

		return cat( mlog, flags, kwargs )


def parseTime( s, name, kw ):
	"""
	Write the time represented by 's' as kw[ name ].
	"""

	if os.path.isfile( s ):
		t = os.stat( s ).st_mtime
		kw[ name ] = t
		log.info( "File timestamp is %s",
				  time.strftime( TIME_FMT, time.localtime( t ) ) )

	else:
		try:
			# If the time contains a decimal point, parse ms
			if "." in s:
				ms = int( s.split( "." )[1] )
				s = s.split( "." )[0]
			else:
				ms = 0

			kw[ name ] = time.mktime( time.strptime( s, TIME_FMT ) ) + \
						 (ms / 1000.0)

		except ValueError:
			log.error( "Time format must be like 'Thu 09 Nov 2006 16:09:01'" )
			return False

	return True


def cat( mlog, flags, kwargs, max = None, query = None ):
	"""
	Dump the contents of a user's log to stdout.  An already-started query can
	be passed in using the 'query' argument, otherwise a new one will be started
	using the kwargs.
	"""

	if not query:
		query = mlog.fetch( **kwargs )

	i = 0

	for result in query:
		if max and i >= max:
			break
		mlog.write( result, flags )
		i += 1

	return True


def tail( mlog, flags, interval, kwargs, context ):
	"""
	Display log output from a user's log in a similar fashion to `tail -f`.
	"""

	kwargs[ "start" ] = time.time()

	# Display some context
	context, query = mlog.fetchContext( kwargs, max = context )
	for entry in context:
		mlog.write( entry, flags )

	# Now start tailing
	while True:
		query.waitForResults( interval )
		mlog.dump( query, flags )

	return True


def summary( mlog, flags, minThresh, kwargs, showProgress=True,
		stream=sys.stdout ):
	"""
	Display a summary of log results.
	"""

	# If interpolation isn't specified, then disable interpolation to avoid
	# stupid results.
	if "interpolate" not in kwargs:
		kwargs[ "interpolate" ] = bwlog.DONT_INTERPOLATE

	# If one of the flags is capitalised, we group results by that before
	# sorting by count.
	match = re.search( "([A-Z])", flags )
	if match:
		group = SUMMARY_FLAGS[ match.group( 1 ).lower() ]
		flags = flags.lower()
	else:
		group = None

	# Make a list of the columns to be used in calculating the summary.
	histCols = [SUMMARY_FLAGS[ c ] for c in flags]

	# Compute the flags to be used when displaying the results
	displayFlags = sum( [DISPLAY_FLAGS[ c ] for c in flags] )

	# Compute summary
	results = mlog.histogram( histCols, showProgress=showProgress, **kwargs )

	# Work out width for count column
	if results:
		countWidth = len( str( results[0][1] ) )
		fmt = "%%%dd: %%s" % countWidth

	# Re-sort if required
	if group:

		# Special case to get criticals, error, warnings etc up the top
		inReverse = group == "severity"

		results.sort( key = lambda (result, count): getattr( result, group ),
					  reverse = inReverse )

	# Display results
	prev = None

	for result, count in results:

		if count >= minThresh:

			# If we re-sorted and we're in a new section, insert some whitespace.
			if group and prev and getattr( result, group ) != getattr( prev, group ):
				print >> stream, "-" * countWidth

			stream.write( fmt % (count, result.format( displayFlags )) )
			prev = result


if __name__ == "__main__":
	try:
		sys.exit( main() )
	except IOError, e:
		if e.errno != 32: # Broken pipe
			raise e
	except KeyboardInterrupt:
		pass

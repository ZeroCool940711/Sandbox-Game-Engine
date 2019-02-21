#!/usr/bin/env python

"""

Tool for archiving and extracting segments of a user's log.

Archiving
---------

Usage: %prog -czf <outfile> [options] [logdir]

The contents of the resulting archive are a self-contained log that can be
accessed with any of the command-line message_logger tools or the Web Console.

Archiving can be done with either gzip or bzip2 compression.  Whilst bzip2
delivers noticeably better compression ratios, it comes with disproportionately
longer processing times.  We recommend using gzip unless you have extreme disk
space constraints.

Segment selection can be done using the --all-segments, --hours-old or
--days-old options, or if none of those options are passed, an interactive menu
is displayed where the user can hand pick which segments are archived.

If --move is passed, the per-segment files (i.e. entries.*, args.*) will be
moved into the archive (analogous to `tar --remove-files`)

If --all-users is passed, archiving is done for all users.  In this mode,
interactive segment selection is not supported, so one of --all-segments,
--hours-old, and --days-old must be passed.

The --all option is a convenience option that is the same as passing 
--all-users, --all-segments and --active-segment (i.e. please archive 
everything).

Even more convenient is the --auto-roll option, which is equivalent to `-avrdzc`.

Extracting
----------

Usage: %prog -xf <archive> [logdir] [-o <outdir>]

Since the archive created by this utility is just a tar file, you're welcome to
use tar to extract it anywhere you like.  The resulting directory structure
will be a self-contained log that you can operate on with any of the provided
message_logger tools.

The extraction provided by this utility is a little different to the standard
`tar` extraction; the archive is unpacked into the log directory referenced by
message_logger.conf (unless -o <outdir> is specified) and no existing files will
be overwritten.  This means that the monolithic files already in the directory
will be preserved and only the entries.*/args.* files will be extracted.

The extraction process maintains a file called 'dirty_files' in the log's root
directory, indicating which segment files were extracted.  If this tool is used
to archive the same directory at a later date, those files will be skipped.
This behaviour is useful for cron-managed log rotation.
"""

import os
import sys
import re
import tempfile
import logging
import tarfile
import time
import signal

import message_log
import util
import mlls

import bwsetup; bwsetup.addPath( ".." )
from pycommon import log
import pycommon.util

USAGE = "%prog [options] [logdir]\n" + __doc__.rstrip()

SEG_PATT = re.compile( "/(entries|args|text)\." )
USER_PATT = re.compile( "/(entries\.|args\.|text\.|uid|components)" )

def main():

	parser = util.getBasicOptionParser( __doc__.rstrip() )
	parser.add_option( "-c", dest = "create", action = "store_true",
					   help = "Create archive" )
	parser.add_option( "-x", dest = "extract", action = "store_true",
					   help = "Extract archive" )
	parser.add_option( "-f", dest = "file",
					   help = "Specify archive name" )
	parser.add_option( "-o", dest = "outdir",
					   help = "Specify output dir for extraction" )
	parser.add_option( "-r", "--remove-files", action = "store_true",
					   help = "Remove files after adding them to the archive" )
	parser.add_option( "-z", "--gzip", dest = "compression",
					   action = "store_const", const = "gzip",
					   help = "Use gzip compression (recommended)" )
	parser.add_option( "-j", "--bzip2", dest = "compression",
					   action = "store_const", const = "bzip2",
					   help = "Use bzip2 compression (smaller but much slower)" )
	parser.add_option( "--hours-old", type = "int", metavar = "N",
					   help = "Select segments older than this for archiving" )
	parser.add_option( "--days-old", type = "int", metavar = "N",
					   help = "Select segments older than this for archiving" )
	parser.add_option( "--all-segments", dest = "hours_old",
					   action = "store_const", const = 0,
					   help = "Archive all non-active segments" )
	parser.add_option( "--all-users", action = "store_true",
					   help = "Archive for all users" )
	parser.add_option( "--active-segment", action = "store_true", 
					   help = "Include the active segment if selected.  " \
					          "This will roll the logs." )
	parser.add_option( "-a", "--all", action = "store_true",
					   help = "Same as --all-users --all-segments" )
	parser.add_option( "-d", "--default-archive", action = "store_true",
					   help = "Operate on archive specified as 'default_archive' " \
					   "in message_logger.conf" )
	parser.add_option( "--auto-roll", action = "store_true",
					   help = "Same as -avrdc" )
	parser.add_option( "--force", action = "store_true",
					   help = "Clobber the output archive if it already exists" )
	parser.add_option( "-v", "--verbose", action = "store_true",
					   help = "Verbose mode" )

	mlog, kwargs, options, args = util.basicInit( parser )

	if options.auto_roll:
		options.all = True
		options.verbose = True
		options.remove_files = True
		options.default_archive = True
		options.create = True
		options.compression = "gzip"

	if options.verbose:
		logging.getLogger().setLevel( log.VERBOSE )

	if options.all:
		options.all_users = True
		options.days_old = 0
		options.active_segment = True


	if options.default_archive:

		conf = message_log.MessageLog.getConfig()[0]
		options.file = conf.get( "message_logger", "default_archive" )

		# If it's expressed as a relative path, it's relative to the
		# directory this file lives in
		if options.file[0] != "/":
			options.file = bwsetup.appdir + "/" + options.file

		# If compression hasn't been given on the commandline, we can infer
		# it from the extension of the default filename
		if not options.compression:
			if options.file.endswith( "bz2" ):
				options.compression = "bzip2"
			elif options.file.endswith( "gz" ):
				options.compression = "gzip"

	if not options.active_segment:
		options.active_segment = False

	if options.create:

		if options.days_old is not None:
			age = options.days_old * 24 * 60 * 60
		elif options.hours_old is not None:
			age = options.hours_old * 60 * 60
		else:
			age = None

		if options.all_users:

			if options.days_old is None and options.hours_old is None:
				log.error( "You must use either --days-old or --hours-old "
						   "when --all-users is specified" )
				return 1

			options.uid = -1

		if not options.file:
			log.error( "You must specify an output file" )
			return 1

		if not options.compression:
			log.error( "You must specify either gzip (-z) or bzip2 (-j) "
					   "compression" )
			return 1

		if options.force and os.path.exists( options.file ):
			try:
				os.unlink( options.file )
			except Exception, e:
				log.error( "Couldn't clobber %s: %s", options.file, e )
				return 1

		return int( not archive( mlog, options.uid, options.file,
									  age, options.remove_files,
									  options.compression,
									  options.active_segment ) )

	elif options.extract:
		return int( not extract( mlog, options.file, options.outdir ) )

	else:
		log.error( "You must specify either -x or -c. No action taken. ")


def archive( mlog, uid, fname, age = None, move = False, 
             compression = "gzip", shouldAddActiveSegment = False ):
	"""
	Archive selected segments from a user's log into a tar archive.  If 'age' is
	passed (as a number of seconds) then all segments with a ctime later than
	that will be archive.  If not, an interactive selection dialog is presented.

	If 'uid' is passed as -1, then archiving is done for all users.  In this
	mode, 'age' must be defined (i.e. interactive selection is not possible).

	If 'move' is True, then per-segment files (i.e. entries.*, args.*, and maybe
	text.*) will be deleted after archiving.

	'compression' can be passed as either 'gzip' or 'bzip2'.
	"""

	outfile = os.path.abspath( fname )
	if os.path.exists( outfile ):
		log.error( "%s already exists", fname )
		log.error( "Aborted (use --force to clobber existing files)" )
		return False

	os.chdir( mlog.root )

	# Macro for extracting the part of a segment filename that contains the date
	timestamp = lambda fname: fname[ fname.index( "." ) + 1: ]

	dirty = mlog.readFileList( "dirty_files", missingOK = True )
	active = mlog.getActiveFiles()

	# Assemble a list of the most recent suffix for each userlog
	cutoffs = dict( [(os.path.dirname( x ), timestamp( x ))
					 for x in active if "entries" in x] )

	# The core set of files we need
	files = ["strings",
			 "hostnames",
			 "component_names",
			 "version"]

	if uid == -1:
		uids = mlog.getUsers().values()
	else:
		uids = [uid]

	shouldRollLogs = False
	for uid in uids:

		userlog = mlog.getUserLog( uid )
		username = userlog.username

		# Monolithic files from this userlog
		files.extend( ("%s/uid" % username,
					   "%s/components" % username) )

		# Time-based segment selection
		if age is not None:
			for seg in [s for s in userlog.getSegments()
						if s.start + age < time.time()]:
				files.append( "%s/entries.%s" % (userlog.username, seg.suffix) )
				files.append( "%s/args.%s" % (userlog.username, seg.suffix) )

		# Interactive segment selection
		else:
			print "Please select the segments to archive (e.g. 0,1,5-10):\n"
			segments = mlls.ls( mlog, uid, True )
			print

			while True:
				input = pycommon.util.prompt( "Enter segments to archive", "all" )

				if input == "all":
					parts = map( str, range( len( segments ) ) )
				else:
					parts = input.split( "," )

				try:
					for part in parts:
						if "-" in part:
							start, end = map( int, part.split( "-" ) )
						else:
							start = end = int( part )
						for i in xrange( start, end+1 ):
							files.append( "%s/entries.%s" %
										  (username, segments[i].suffix) )
							files.append( "%s/args.%s" %
										  (username, segments[i].suffix) )
					break

				except ValueError:
					print "Input error, please try again"

		# Checking if active segment is selected.  If selected and if the user 
		# specified to include the active segment, logs will be rolled before
		# archiving.  Otherwise, remove entries and args file that belong to 
		# the active segment.
		if len(files) == 4:
			# "files" only contains core set of files.  See initialisation of 
			# "files" variable.
			continue

		# entries and args file are added in pairs.  entries file first then
		# args file.
		try:
			entriesFile = files[-2]
			argsFile = files[-1]
			# If active_files is missing, then the if statement is not 
			# executed.	
			if (username in cutoffs) and \
		   	   (timestamp( entriesFile ) >= cutoffs[ username ]) and \
		       (timestamp( argsFile ) >= cutoffs[ username ]):
				# entries and args file belong to active segment.  Check if should
				# add them.
				if shouldAddActiveSegment:
					shouldRollLogs = True

				else:
					# Remove args file.
					log.verbose( "Skipping %s (active)", argsFile )
					files.pop()
					# Remove entries file.
					log.verbose( "Skipping %s (active)", entriesFile )
					files.pop()
		except ValueError:
			# Handles the case where user does not have a log segment to include.
			# This might happen, for example, when --hours-old flag is used.
			pass



	# Rolling logs to include the active segment.
	if shouldRollLogs:
		# No need to check if MessageLogger is running.  Would have encountered
		# problem before getting to here.

		# Send HUP to the logger process which causes logs to be rolled.
		os.kill( int( open( "%s/pid" % mlog.root ).read() ), signal.SIGHUP )

		# Wait a little bit to allow the select() loop in message_logger to cycle
		time.sleep( 2 )


	# Attempt to open the file in write mode before calling popen() to flush out
	# permissions errors
	try:
		open( outfile, "w" ).close()
	except IOError, e:
		log.error( "Couldn't open %s for writing:\n%s", outfile, e )
		return False

	if logging.getLogger().getEffectiveLevel() <= log.VERBOSE:
		verbose = "v"
	else:
		verbose = ""

	# Start tar process and feed it with filenames
	pipe = os.popen( "tar c%sf %s --%s -T-" %
					 (verbose, outfile, compression), "w" )

	trash = []

	for fname in files:

		# Ignore files that were extracted from an archive
		if fname in dirty:
			log.verbose( "Skipping %s (dirty)", fname )
			continue

		# Tell `tar` to add the file to the archive
		print >>pipe, fname

		# Clean up segment files if in move mode
		if move and SEG_PATT.search( fname ):
			trash.append( fname )

	status = pipe.close()
	if status:
		log.error( "tar subprocess had non-zero exit status: %02x", status )
		return False

	for fname in trash:
		os.unlink( fname )

	return True


def extract( mlog, archive, outdir ):
	"""
	Extract only the per-segment log data from the named archive into the logdir
	inhabited by mlog.
	"""

	tar = tarfile.open( archive, "r" )

	# If we're extracting into a new directory, we don't need to worry about all
	# the dirty file + overwriting stuff below
	if outdir:

		# Must be fresh
		if os.path.exists( outdir ):
			log.error( "Output directory already exists: %s", outdir )
			log.error( "You must specify a new directory name when not "
					   "extracting into an existing log directory" )
			return False

		if os.system( "mkdir -p %s" % outdir ):
			log.error( "Couldn't create output directory %s", outdir )
			return False

		os.chdir( outdir )
		for member in tar.getmembers():
			log.verbose( "Inflating %s", member.name )
			tar.extract( member )

		tar.close()
		return True

	# Read existing dirty database
	try:
		dirty = set( [x.rstrip() for x in
					  open( "%s/dirty_files" % mlog.root ).readlines()] )
	except IOError:
		dirty = set()

	# Change into the root directory of the log
	os.chdir( mlog.root )

	# Extract per-segment data and record dirty status
	for fname in filter( USER_PATT.search, tar.getnames() ):
		# if its a segment file, then check whether it exists before
		# clobbering
		if SEG_PATT.search( fname ):
			if os.path.exists( fname ):
				log.warning( "%s already exists, skipping", fname )
				continue
		elif os.path.exists( fname ):
			# otherwise it's a uid or a components file, uid should be
			# constant (across a network), so if neither do not exist, then
			# extract them both
			continue

		log.verbose( "Inflating %s", fname )
		tar.extract( fname )
		dir, base = os.path.split( fname )
		dirty.add( fname )

	tar.close()

	# Update dirty database
	dirtyf = open( "%s/dirty_files" % mlog.root, "w" )
	for fname in sorted( dirty ):
		dirtyf.write( "%s\n" % fname )
	dirtyf.close()

	return True


if __name__ == "__main__":
	sys.exit( main() )

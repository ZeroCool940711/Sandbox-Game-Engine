#!/usr/bin/env python

import os
import sys
import pwd
import tempfile
import getpass

import bwsetup; bwsetup.addPath( ".." )
from pycommon import util
from pycommon import log

USAGE = """
*****************
*** IMPORTANT ***
*****************

Please check the following before continuing:

1) Your sysadmin has entered you in the sudoers file on this box (with `visudo`)
2) You know where your home directory will be on this box
3) You have shared the MF directory on your windows box
4) You have allowed full control for everyone on your windows share

NOTE: If you are asked for a password, enter your *own* password, not the root
password.  This will not work unless you are in the sudoers file (see (1) above)
""".lstrip()

BASH_PROFILE_TEMPLATE = """
export PS1='[\u@\h \W]\$ '
""".lstrip()

# Alias this common function call
prompt = util.prompt

# We want INFO and DEBUG prefixes on output in this app
log.console.setFormatter( log.CleanFormatter( stripInfoPrefix = False ) )


def sudo_fwrite( text, fname ):
	fd, tmpname = tempfile.mkstemp()
	done = os.write( fd, text )
	if done != len( text ):
		log.error( "Only %d/%d bytes written to %s" % (done, len( text ), tmpname) )
		return False
	if os.system( "sudo cp %s %s" % (tmpname, fname) ):
		log.error( "Couldn't `sudo cp` %s -> %s" % (tmpname, fname) )
		return False
	return True


def main():

	print USAGE

	# Verify we are in the sudoers file
	if os.system( "sudo ls > /dev/null" ):
		log.critical( "You must be in the sudoers list for this machine" )

	# Who are we configuring
	ent = pwd.getpwuid( os.getuid() )
	user = ent.pw_name

	# Make home dir if necessary
	if not os.path.exists( ent.pw_dir ):
		log.info( "Your home directory doesn't exist, will create one now ..." )
		home = prompt( "Enter path for home directory", ent.pw_dir )

		if not util.softMkDir( home ):
			log.critical( "Couldn't make home directory %s" % home )
		else:
			log.info( "Made %s" % home )

		if home != ent.pw_dir:
			if not util.softSymlink( ent.pw_dir, home ):
				log.critical( "Couldn't symlink %s -> %s" % (default, home) )
			else:
				log.info( "Symlinked %s -> %s" % (default, home) )
	else:
		home = ent.pw_dir

	# Set up MF directory
	mfroot = None
	while not mfroot or not util.softMkDir( mfroot ):
		mfroot = prompt( "Enter path to root development directory",
						 "%s/mf" % home )

	# Find out where the windows MF directory is shared
	winmfroot = prompt( "Enter Samba share name for your windows MF directory",
						"//%s/mf" % user )
	mounted = False
	while True:
		winmfmount = prompt( "Enter local mount point for windows MF directory",
							 "/mnt/mf-%s" % user )

		# Check if it is already correctly mounted
		grepout = os.popen( "mount | grep '%s on %s'" %
							(winmfroot, winmfmount) ).read().strip()
		if grepout:
			log.info( "Seems to be mounted already: %s" % grepout )
			if prompt( "Is this correct?", "y" ) == "y":
				mounted = True
				break

		# Check if something else is mounted at the mount point
		grepout = os.popen( "mount | grep 'on %s'" % winmfmount ).read().strip()
		if grepout:
			log.error( "Another filesystem already mounted at %s" % winmfmount )
			log.info( grepout )
			continue

		if os.path.isdir( winmfmount ) and os.listdir( winmfmount ):
			log.error( "%s already exists and is not empty" % winmfmount )
			continue
		if os.path.exists( winmfmount ) and not os.path.isdir( winmfmount ):
			log.error( "%s already exists and is not a directory" % winmfmount )
			continue
		break

	# Make the mount point if necessary
	if not os.path.isdir( winmfmount ):
		if util.softMkDir( winmfmount, sudo = True ):
			log.info( "Made mount point: %s" % winmfmount )
		else:
			log.critical( "Couldn't make mount point for windows share: %s",
						  winmfmount )

	# Ask the user if he wants to automount on every boot
	if util.promptYesNo(
		"Do you want to automount your windows share each time this linux box "
		"boots?\nThis will leave your password in /etc/fstab in the CLEAR:" ):

		password = password2 = None

		while not password or password != password2:
			password = getpass.getpass( "Password: " )
			password2 = getpass.getpass( "Confirm password: " )

	else:
		password = None

	# Patch /etc/fstab if necessary
	orig_fstab = open( "/etc/fstab" ).read().rstrip()
	fstablines = orig_fstab.split( "\n" )
	for line in fstablines[:]:
		if winmfmount in line or winmfroot in line:
			fstablines.remove( line )

	if password:
		credentials = "user=%s,password=%s" % (user, password)
	else:
		credentials = "user=%s,noauto" % user

	fstablines.append( "%s\t%s\tcifs\tuid=%d,gid=%d,"
					   "file_mode=0777,dir_mode=0777,%s\t0 0\n" % \
					   (winmfroot, winmfmount, ent.pw_uid, ent.pw_gid,
						credentials) )

	# Try to write patched table back to /etc/fstab
	if sudo_fwrite( "\n".join( fstablines ), "/etc/fstab" ):
		log.info( "Patched /etc/fstab successfully" )
	else:
		log.critical( "Couldn't patch /etc/fstab with your MF mountpoint" )

	# Attempt to mount the windows MF dir
	if mounted or os.system( "sudo mount %s" % winmfmount ) == 0:
		log.info( "%s is mounted at %s" % (winmfroot, winmfmount) )
	else:
		log.error( "Couldn't mount windows MF directory, restoring original fstab" )
		sudo_fwrite( orig_fstab, "/etc/fstab" )
		log.critical( "Aborting due to previous errors" )

	# Detect which subdirs in $MF_ROOT have res trees
	resdirs = []
	for dirname in [os.path.join( winmfmount, f ) for f in
					os.listdir( winmfmount ) if
					os.path.isdir( os.path.join( winmfmount, f ) )]:
		if os.path.isdir( os.path.join( dirname, "res" ) ):
			resdirs.append( os.path.join( dirname, "res" ) )
	log.info( "The following res trees were detected on your windows share: " )
	for i in xrange( len( resdirs ) ):
		print "[%d] %s" % (i, resdirs[i])
	while True:
		choice = prompt( "Enter a comma-sep list of the dirs to link in", "all" )
		if choice == "all":
			selection = range( len( resdirs ) )
		else:
			try:
				selection = map( int, choice.split( "," ) )
			except:
				log.error( "Invalid input" )
				continue
		break

	# Link in the res trees
	projects = []
	for i in selection:
		resdir = resdirs[i]
		project = os.path.basename( os.path.dirname( resdir ) )
		projects.append( project )
		projdir = os.path.join( mfroot, project )
		reslink = os.path.join( projdir, "res" )

		if not os.path.isdir( projdir ) and \
			   os.system( "mkdir -p %s" % projdir ):
			log.critical( "Couldn't create %s" % projdir )

		# If res tree is already a link to the required location, skip
		if os.path.islink( reslink ) and os.readlink( reslink ) == resdir:
			log.info( "%s is already a link to %s" % (reslink, resdir) )
			continue

		if os.path.exists( reslink ):
			log.critical( "%s already exists and is not a link to %s" %
				   (reslink, resdir) )

		os.chdir( projdir )
		os.symlink( resdir, "res" )
		log.info( "Symlinked %s -> %s" % (reslink, resdir) )

	# Set up BW_RES_PATH
	while True:
		resorder = prompt( "Enter project names in order for your BW_RES_PATH",
						   ",".join( projects ) ).split( "," )
		bwrespath = ":".join( ["%s/%s/res" % (mfroot, proj)
							   for proj in resorder] )
		log.info( "BW_RES_PATH will be %s" % bwrespath )
		if prompt( "Hit [ENTER] to accept" ) == "":
			break

	# Set up src links if necessary
	if util.promptYesNo( "Do you want to symlink source dirs so you can compile "
						 "the server from your windows source tree?" ):

		for path in ("src", "bigworld/src"):
			realpath = os.path.join( winmfmount, path )
			linkpath = os.path.join( mfroot, path )
			if os.path.isdir( realpath ):
				util.softSymlink( linkpath, realpath )

	# Write .bash_profile
	if util.promptYesNo( "Do you want to generate a basic ~/.bash_profile?" ):
		profile = "%s/.bash_profile" % home
		f = open( profile, "a" )
		f.write( BASH_PROFILE_TEMPLATE )
		f.close()
		log.info( "Wrote %s" % profile )

		# Make sure .bashrc sources .bash_profile
		rc = "%s/.bashrc" % home
		if not os.path.isfile( rc ) or \
			   ".bash_profile" not in open( rc ).read():
			open( rc, "a" ).write( ". ~/.bash_profile\n" )
			log.info( "~/.bashrc now sources ~/.bash_profile" )

	conffile = "%s/.bwmachined.conf" % home
	newconf = "%s;%s\n" % (mfroot, bwrespath)

	# Write ~/.bwmachined.conf if necessary
	if not os.path.isfile( conffile ) or newconf not in open( conffile ).read():
		open( conffile, "a" ).write( newconf )
		log.info( "Wrote %s/.bwmachined.conf" % home )

	# Ask user if he wants to do a checkout.
	if util.promptYesNo( "Fetch the server binaries from SVN?" ):

		os.chdir( mfroot )
		cocmd = prompt(
			"Enter checkout command",
			"svn co http://svn/svn/mf/bigworld/trunk/bigworld/bin/Hybrid "
			"bigworld/bin/Hybrid" )

		if os.system( cocmd ):
			log.critical( "Couldn't check out server binaries" )

	log.info( "*** All done! ***" )

if __name__ == "__main__":
	try:
		sys.exit( main() )
	except KeyboardInterrupt:
		print "\n[terminated]"
	except RuntimeError, e:
		print e

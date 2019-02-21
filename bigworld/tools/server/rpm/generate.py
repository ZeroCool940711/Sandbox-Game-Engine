#!/usr/bin/env python


import os
import re
import shutil
import sys
import tempfile

CLEANUP_TMP_FILE = "cleanup.tmp"
BINARY_RPM_TMP_FILE = "binary_rpm.tmp"


class RPMFile( object ):
	"""
	Represents a file that needs to be included in the binary RPM.
	Information about a particular file is in the <package>.lst.

	"""

	def __init__( self, detailsList, destFileUnexpanded ):
		object.__init__( self )

		self.sourceFile = detailsList[0].strip()
		self.destFile = detailsList[1].strip()
		self.permissionMode = detailsList[2].strip()

		if len(detailsList) >= 5:
			self.owner = detailsList[3].strip()
			self.group = detailsList[4].strip()
		else:
			self.owner = "root"
			self.group = "root"

		self.destMacroFile = destFileUnexpanded

		return


class RPMHelper( object ):
	"""
	A class that contains methods to prepare files for the RPM system 
	which will create binary RPMs.
	"""

	def __init__( self, package ):
		object.__init__( self )

		# Path to BigWorld root.
		self.BW_ROOT = os.path.abspath( "../../../../" )

		# Required files.
		self.package = package
		self.listFile = "bwmachined/%s.lst" % self.package
		self.specFile = "bwmachined/%s.spec" % self.package
		self.versionFile = self.BW_ROOT + "/bigworld/res/version.xml"
		self.templateSpecFile = "bwmachined/%s_template.spec" % self.package

		# Get BigWorld version number.
		self.__parseVersionNumber()
		
		# Get macros defined in package.spec file.
		self.macros = {}
		self.__parseMacros( package )

		# Define useful constants.
		# Create BuildRoot directory (top level only) for RPM system.
		self.buildRoot = self.__createBuildRootDir()

		self.files = []
		self.furthestDestDirs = []

		# This boolean is used to throw an error at the end of the
		# constructor to allow as many errors to be detected as possible
		hasError = False

		# Load the file list in
		f = open( self.listFile )
		lineNum = 0
		errorMsgs = []
		for line in f:

			lineNum += 1

			# Don't bother with any empty lines
			trimLine = line.strip()
			if len( trimLine ) == 0:
				continue
			elif trimLine[ 0 ] == "#":
				continue

			# Assumes no leading or training whitespaces.
			components = trimLine.split( "," )
	
			# Verify each entry has the required three components.
			if len(components) < 3:
				print "ERROR: Syntax Error: Line %i: " \
					"Invalid Entry: Expected format is <src_dir>," \
					"<dest_dir>,<permission>"
				hasError = True
				continue;

			# Verify the source file exists.
			srcFile = "%s/%s" % (self.BW_ROOT, components[0])
			if not os.path.exists( srcFile ):
				print "ERROR: Invalid entry: Line %i: Source file does not " \
					"exist." % lineNum

			# Replace macros in destination path.
			destDir = components[1]
			destDirUnexpanded = components[1]
			while len(destDir) > 0:
				m = re.search( "%{_[a-z]+}", destDir )
				if m != None:
					macroMatch = m.group( 0 )
					result = self.__evaluateMacro( macroMatch )
				
					if result == None:
						print "ERROR: Unable to evaluate macro '%s'" \
							% macroMatch
						hasError = True
			
					else:
						# Replace the macro within the current string
						replacement = \
							re.sub( macroMatch, result, components[1] )
						components[1] = replacement

				else:
					# No more macros left.
					break

				destDir = destDir[len(macroMatch):]

			
			# Destination path should start with "/".
			if not components[1].startswith( "/" ):
				print "ERROR: Invalid Entry: Line %i: Destination path " \
					"must start with '/'" % lineNum
				hasError = True

			self.files += [RPMFile( components, destDirUnexpanded )]

			self.__addToFurthestDestDirs( os.path.dirname( components[1] ) )

		f.close()


		if hasError:
			sys.exit()


		# Save the directories that needs to be deleted as part of the 
		# cleanup process.
		cleanupFile = open( CLEANUP_TMP_FILE, "a")
		cleanupFile.write( self.buildRoot + "\n" )
		cleanupFile.close()

		# Save the name of the binary RPM that needs to be copied from 
		# RPM directory to binary_rpms directory.
		binaryRPMFile = open( BINARY_RPM_TMP_FILE, "a" )
		line = self.macros["name"] + "-" + self.version + "." + \
			self.patch + "-" + self.patch + ".i386.rpm" + \
			"\n"
		binaryRPMFile.write ( line )
		binaryRPMFile.close()

		return


	def __createBuildRootDir( self ):
		"""
		Creates top level BuildRoot directory.
		"""
		
		path = tempfile.mkdtemp( "", self.package + "_", "/tmp" )
		
		return path 
	

	def __parseMacros( self, package ):
		"""
		Parse macros defined in package_template.spec file.
		Assumptions:
			- Macros defined in package_template.spec file are in a single 
			  block.  First %define marks the start of the block, and first 
			  non-%define found since start of the block marks the end 
			  of the block.
		"""
	
		lineNum = 1
		file = open( self.templateSpecFile )
		hasBlockStarted = False
		for line in file:
			tokens = line.split()

			if (len(tokens) == 0):
				# Empty line.
				continue;

			elif (not(tokens[0] == "%define")):
				# Non-macro line.
				if hasBlockStarted:
					break

				continue

			elif len(tokens) != 3:
				# A macro definition should have three parts:
				# %define <macro_name> <macro_value>
				print "ERROR: Syntax Error: Line %i: Invalid macro " \
					"definition." % (lineNum) 
				sys.exit()
				
			
			self.macros[tokens[1]] = tokens[2]
			hasBlockStarted = True
			
			lineNum += 1

		return


	def __parseVersionNumber( self ):
		"""
		Parse version number from bigworld/res/version.xml.
		"""

		file = open( self.versionFile )
		tags =  [("<major>", "</major>", "major"), \
			 	 ("<minor>", "</minor>", "minor"), \
			 	 ("<reserved>", "</reserved>", "reserved"), \
			 	 ("<patch>", "</patch>", "patch")]
		versionInfo = []		

		for line in file:
			trimLine = line.strip()
			openTag, closeTag, key = tags[0]
			startPos = trimLine.find(openTag)
			if startPos != -1:
				endPos = trimLine.find( closeTag, (startPos + len(openTag)) )
				if endPos != -1:
					data = trimLine[startPos+len(openTag):endPos].strip()
					versionInfo += [data]

				else:
					print "ERROR: Did not find '%s' tag in '%s'." \
						% (openTag, versionFilename)
					sys.exit()
					return 	

				tags = tags[1:]
				if len(tags) == 0:
					break

		self.version = ".".join( versionInfo[0:len(versionInfo)-1] )
		self.patch = versionInfo[-1]

		file.close()
		return


	def __evaluateMacro( self, macro ):
		"""
		Evaluate the input macro.  Macros defined in package_template.spec 
		file take precedence over evaluation by the RPM system.
		

		Returns None if evaluation failed.

		@param macro: Macro to evaluate.
		"""

		macro = macro.strip()
		
		# Check if macro is defined in package_template.spec file.
		if self.macros.has_key( macro ):
			return self.macros[macro]

		# Ask RPM system to evaluate the macro.
		fd = os.popen( "rpm --eval '%s'" % macro )
		line = fd.readline()
		fd.close()

		trimLine = line.strip()
		if trimLine != macro:
			return trimLine
		else:
			return None


	def __addToFurthestDestDirs( self, dir ):
		"""Check if the provided directory is the deepest directory seen
			down the file-system path heirachy so far and add to the list,
			replacing any shallower directories in the process. The list of
			deepest directories is then used for creating the BuildRoot
			directory structure that the source package will populate into.
		"""

		# Adding a trailing path marker here to assist in isolating 
		# directories when we perform 'startswith'.
		dir = dir + "/"

		shouldAdd = True
		for furthestDir in self.furthestDestDirs:
			if dir.startswith( furthestDir ):
				self.furthestDestDirs.remove( furthestDir )
				break

			if furthestDir.startswith( dir ):
				shouldAdd = False
				break

		if shouldAdd == True:
			self.furthestDestDirs.append( dir )

		return


	def generateBuildRoot( self ):
		"""
		Generates the BuildRoot directory for the RPM system.
		"""

		# Create all the subdirectories in the Buildroot directory
		for dir in self.furthestDestDirs:
			buildSubDir = "%s%s" % (self.buildRoot, dir)
			print buildSubDir
			os.makedirs( buildSubDir )

		return


	def populateBuildRoot( self ):
		"""
		Populate the BuildRoot directory with files that will be included
		in the binary RPM.
		"""

		numFilesProcessed = 0
		for rpmFile in self.files: 

			srcFilePath = "%s/%s" % (self.BW_ROOT, rpmFile.sourceFile)
			dstFilePath = "%s%s" % (self.buildRoot, rpmFile.destFile)

			# Copy the file now
			shutil.copy( srcFilePath, dstFilePath )


			# Patch this file if required.
			self.patchFile( dstFilePath, rpmFile )

			# Set the security context if required
			# TODO	

			# Print processing status.
			numFilesProcessed += 1
			# Do not remove trailing comma.
			print "\r",
			status = "%i" % ((numFilesProcessed/len( self.files )) * 100)
			# Do not remove trailing comma.
			print status + "% completed ...",
		
		print ""

		return

	
	def patchFile( self, file, rpmFile):
		"""
		Make changes to the file if required.
	
		@param file: Name of the file to patch.
		@param rpmFile: RPMFile representing the file to patch.	
		"""

		if self.package == "bwmachined":
			self.patchBWMachined( file, rpmFile )

		return

	
	def patchBWMachined( self, file, rpmFile ):
		"""
		Patch files in the BWMachined package.
	
		@param file: File to patch.
		@param rpmFile: RPMFile representing the file to patch.
		"""

		# Update the SBINDIR variable in %{_initrddir}/bwmachined2.
		# %{_initrddir}/bwmachined is a copy of 
		# bigworld/tools/server/install/bwmachined2.sh.
		if rpmFile.destMacroFile == "%{_initrddir}/bwmachined2":
			initrddir = self.__evaluateMacro( "%{_initrddir}" )
			sbindir = self.__evaluateMacro( "%{_sbindir}" )
			buildroot_initrddir = self.buildRoot + initrddir
			
			try:
				# Rename old file.
				os.rename( \
					buildroot_initrddir + "/bwmachined2",  \
					buildroot_initrddir + "/bwmachined2.tmp" )

				# Patch the file and write to actual bwmachined2 file.
				file = open( buildroot_initrddir + "/bwmachined2", "w" )
				oldFile = open( buildroot_initrddir + "/bwmachined2.tmp" )

				for line in oldFile:
					if line.startswith( "SBINDIR=" ):
						file.write( "SBINDIR=" + sbindir + "\n" )
					else:
						file.write( line )

				file.close()
				oldFile.close()

				# Delete the temporary file.
				os.remove( buildroot_initrddir + "/bwmachined2.tmp" )

			except OSError:
				print "ERROR: Failed to patch", buildroot_initdir + \
					"/bwmachined2."

		return


	def updateSpec( self ):
		"""
		Update the RPM spec file with the list of files to be included
		in the binary RPM.
		"""

		# Read in the template spec file and generate the actual spec file 
		# used to generate the binary RPM.
		spec = open( self.specFile, "w" )
		specTemplate = open( self.templateSpecFile )

		for line in specTemplate.xreadlines():

			if line.startswith( "## PLACEHOLDER: FILES FOR RPM" ):
				# Write the original line, and start skipping any existing
				# lines until the end of the replacement block.

				# Add files to be included in the binary RPM to the 
				# %files section of the spec file.
				for rpmFile in self.files:

					entry = \
						"%attr(" + rpmFile.permissionMode + ", " + \
						rpmFile.owner + ", " + rpmFile.group + ") " + \
						rpmFile.destMacroFile + "\n"
	
					# Mark the file as configuration file if it is one.
					if rpmFile.sourceFile.endswith(".conf"):
						entry = "%config " + entry

					spec.write( entry )

			elif line.startswith( "## PLACEHOLDER: PACKAGE SPECIFIC MACROS" ):
				# Adding version information macros.
				spec.write( "%define version\t" + self.version + "\n" )
				spec.write( "%define patch\t" + self.patch + "\n" )

			elif line.startswith( "## PLACEHOLDER: BUILDROOT" ):
				# Add BuildRoot tag.
				spec.write( "BuildRoot: " + self.buildRoot + "\n" )

			else:
				# As long as we aren't avoiding lines, write the output back
				if not line.startswith( "## " ):
					# If a comment starts with two hashes and space, 
					# do not add it to actual spec file.
					spec.write( line )

		spec.close()
		specTemplate.close()
		return


def cleanUp( ):
	"""
	Performs cleanup:
	- Copy binary RPMs to binary_rpms directory.
	- Delete directories used to build binary RPMs.
	"""

	
	# Copy binary RPMs to binary_rpms directory.
	print "Moving binary RPM to binary_rpms directory ..."
	try:
		file = open( BINARY_RPM_TMP_FILE )
		for line in file:
			line = line.strip()
			srcFilePath = "/usr/src/redhat/RPMS/i386/" + line
			destFilePath = "binary_rpms"
			shutil.move( srcFilePath, destFilePath )
		file.close()

	except IOError:
		print "No binary RPMs to move."


	# Remove directories used to build binary RPMs.
	print "Deleting directories used to build binary RPM ..."
	try:
		file = open( CLEANUP_TMP_FILE )
		for line in file:
			line = line.strip()
			print "Deleting", line
			shutil.rmtree( line, True )
		file.close()
	
	except IOError:
		print "No directories to delete."


	# Delete the temporary files.
	try:
		os.remove( BINARY_RPM_TMP_FILE ) 
	except OSError:
		# binary rpm temp file already deleted.
		pass

	try:
		os.remove( CLEANUP_TMP_FILE )
	except OSError:
		# cleanup temp file already deleted.
		pass


	print "Cleanup completed ..."

	return


if __name__ == "__main__":

	try:

		# Make sure this script is not run by the root user.
		# Root user always has uid 0.
		if os.getuid() == 0:
			print "ERROR: Executing generate.py script using root user: " \
				"This script must not be executed by the root user."
			sys.exit()


		# Get the first argument, which is either name of the component
		# that binary RPM is to be build for or "cleanup", which means to 
		# cleanup any directory used in RPM creation process.
		if len(sys.argv) < 2:
			print "ERROR: One argument expected:" 
			print "Usage:" 
			print "\t./generate.py <component_name>"
			print "\t./generate.py cleanup"
			sys.exit()

		arg = (sys.argv[1]).strip()
	
		if arg == "cleanup":
			# Perform cleanup.
			cleanUp()

		else:
			# Build RPM.
			rpm = RPMHelper( arg ) 
			print "Generating Buildroot directory for RPM system ..."
			rpm.generateBuildRoot()
			print "Populating Buildroot directory for RPM system ..."
			rpm.populateBuildRoot()
			print "Creating", arg, "spec file for RPM system ..."
			rpm.updateSpec()
			print "Preparation completed ..."
			

	except SystemExit:
		print "\n"
		print "Failed: script exited with error."

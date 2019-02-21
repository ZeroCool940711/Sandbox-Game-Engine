import Copy
import Verify
import sys
import os
import Manifest

class ResourcePackager:
	def __init__( self ):
		self.doCopy = 1
		self.doVerify = 1
		self.showErrors = 1
		self.copy = None
		self.verify = None
		
	def printUsage( self ):
		print "Usage : python resourcePackager.py destFolder [-noCopy] [-noVerify] [-noErrors]"
		print "  where destFolder is the path where the games' resources should go."
		print ""
		print "The ResourcePackager is part of the BigWorld toolkit"
		
		
	def printReport( self, errorList ):
		print ""			
		print "ResourcePackager has finished processing the manifest"
		print "-----------------------------------------------------"
					
		if (self.showErrors) and len(errorList) > 0:
			print ""
			print "There were errors:"
			print "------------------"
			for i in errorList:
				print i
				
		print ""
		
		if self.copy:
			self.copy.printReport()
			
		if self.verify:
			self.verify.printReport()
				
				
	def parseCmdLineArg( self, arg ):
		if arg == "-noCopy":
			self.doCopy = 0
			return 1
			
		if arg == "-noVerify":
			self.doVerify = 0
			return 1
			
		if arg == "-noErrors":
			self.showErrors = 0
			return 1
			
		return 0
		
	def go( self ):
		if len(sys.argv) <= 1:
			self.printUsage()
			return
			
		if self.parseCmdLineArg(sys.argv[1]):
			self.printUsage()
			return
			
		destFolder = sys.argv[1]
			
		if len(sys.argv) > 2:
			for i in sys.argv[2:]:
				if not self.parseCmdLineArg(i):
					self.printUsage()
					return
		
		errorList = []
		
		#Compile the manifest
		manifest = Manifest.Manifest( errorList )
		
		if len(manifest.folders) == 0:
			errorList.append( "Manifest file contained no files." )
			self.printReport( errorList )
			return
		
		if self.doCopy:
			#Create the destination folder, and check that it was not
			#already there.  We don't want any trouble with copying over
			#existing builds, or unclean folders.  We insist on creating
			#our own!
			try:	
				os.mkdir( destFolder )
			except OSError, e:
				if e.errno == 17:
					errorList.append( "Destination folder already exists." )
					errorList.append( "Please remove the destination folder and try again." )
				else:
					errorList.append( e.strerror + ":" + e.filename )
				self.printReport( errorList )
				return

			#Looks good so far.  Try copying all the appropriate files.
			self.copy = Copy.Copy( errorList, manifest, destFolder )
		
		#Right!  Now lets verify those files
		if self.doVerify:
			if (not self.doCopy) or (self.copy.nFiles > 0):
				self.verify = Verify.Verify( errorList, manifest, destFolder )
								
		#Ok, finished.  Print out any errors encountered.
		self.printReport( errorList )
		
rp = ResourcePackager()
rp.go()
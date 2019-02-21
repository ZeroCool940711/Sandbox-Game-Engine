import sys
import os
import shutil
import Progress


class Copy:
	def __init__( self, errorList, manifest, destFolder ):
		self.sourceFolders = os.environ["BW_RES_PATH"].split(";")
		self.nFolders = 0
		self.nFiles = 0
		self.nFilesInManifest = 0
		
		if len(self.sourceFolders)<1:
			errorList.append( "Copy : No paths were found in BW_RES_PATH.  Source of files is unknown!" )
		else:
			self.copyFromManifest( errorList, manifest, destFolder )
			
	def printReport( self ):
		print ""
		print "Copy Status:"
		print "Created %d folders" % (self.nFolders,)
		print "Copied %d/%d files" % (self.nFiles,self.nFilesInManifest)
		
		
	def createFolder( self, folderName, errorList ):
		try:
			os.mkdir( folderName )
			self.nFolders += 1
		except OSError, e:
			if e.errno == 17:
				#directory already exists.  ignore since
				#we will get this due to how the manifest
				#is stored.
				pass
			else:
				errorList.append( e.strerror + ":" + e.filename )
		
		
	def copyFile( self, srcFolder, srcFilename, destFolder, destFilename, errorList ):		
		try:
			shutil.copy( srcFolder+"/"+srcFilename, destFolder+"/"+destFilename )
			self.nFiles += 1
			return 1
		except IOError, e:
			if e.errno == 2:
				#file not found. ignore for now
				pass
			else:
				errorList.append( e.strerror + ":" + e.filename )		
		return 0
		
		
	def copyFromManifest( self, errorList, manifest, destFolder ):
		progress = Progress.Progress( "Copying Files", manifest.fileCount )
		self.nFilesInManifest = manifest.fileCount
		
		for z in manifest.folders:			
			folderName = destFolder + "/" + z[0]

			self.createFolder( folderName, errorList )
		
			for (filename,digest) in z[1]:
				progress.increment()
				success = 0
				
				for j in self.sourceFolders:
					#If there is a .client version of the required file, copy that
					success = self.copyFile( j, filename + ".client", destFolder, filename, errorList )
					if not success:
						success = self.copyFile( j, filename, destFolder, filename, errorList )
					if success:
						break
						
				if not success:
					errorList.append( "File not found: " + filename )
					
		print ""
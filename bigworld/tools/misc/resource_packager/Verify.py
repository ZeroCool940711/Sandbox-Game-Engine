import md5
import Progress

class Verify:
	def __init__( self, errorList, manifest, destFolder ):
		self.okFiles = 0
		self.nFiles = 0
		self.verifyManifest( errorList, manifest, destFolder )
		
	def printReport( self ):
		print ""
		print "Verify Status:"
		print "%d/%d files verified correctly" % (self.okFiles,self.nFiles)
		
	def verifyManifest( self, errorList, manifest, destFolder ):
		progress = Progress.Progress( "Verifying Files", manifest.fileCount )
		
		for (folder,fileList) in manifest.folders:			
			folderName = destFolder + "/" + folder
			
			for (fileName,digest) in fileList:
				progress.increment()
				try:
					f = open(destFolder+"/"+fileName,'rb')
					m = md5.new()
					m.update(f.read())
					hexDigest = reduce( lambda x, y: x+y, map( lambda x: "%02X" % ord(x), m.digest() ))
					f.close()
					if hexDigest != digest:
						errorList.append( "MD5 check failed: " + fileName )
					else:
						self.okFiles += 1
					self.nFiles += 1
				except IOError, e:
					if e.errno == 2:
						#error already listed in the Copy module. ignore
						pass
					else:
						errorList.append( e.strerror + ":" + e.fileName )
						
		print ""
import os
import resmgrdll
import ResMgr

#Manifest is a class that turns manifest.xml into
#a list of (filename, md5)

class Manifest:
	def __init__( self, errorList):
		self.manifest = self.findManifestFile()
		self.folders = []
		self.fileCount = 0
		if self.manifest:
			self.findAllFiles()
		else:
			errorList.append( "Manifest.xml not found" )
		
	def findManifestFile( self ):
		ds = ResMgr.openSection("manifest.xml")
		return ds
		
	def addFiles( self, ds, path = "" ):
		if len(ds.keys()) == 0:
			self.folders[-1][1].append( (path+ds.name,ds.asString) )
			self.fileCount += 1
		else:
			folderName = path+ds.name+"/"
			self.folders.append( (folderName,[]) )
			for i in ds.items():
				self.addFiles(i[1],folderName)
		
	def findAllFiles( self ):
		self.fileCount
		for i in self.manifest.values():
			self.folders.append(("",[]))
			self.addFiles(i,"")
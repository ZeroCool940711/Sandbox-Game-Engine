# this is a _really_ dodgy options class to store user preferences
# in a sudo xml format, one level deep
#
# TODO: there has got to be a lib somewhere that does nice xml

import os

class Options:
	def __init__( self, fileName ):
		# open the file and read in the options
		self.optFileName = fileName
		self.oList = []

		if not os.path.exists( self.optFileName ):
			return

		optFile = file( self.optFileName, 'rU' )
		baseRead = False
		for line in optFile:
			text = line.strip()
			if text[0] != "<":
				continue

			if not baseRead:
				baseRead = True
				continue

			if text[1] == "/":
				break

			text = text.replace( "</", "<" )
			text = text.replace( ">", "<" )
			text = text.strip( "<" )
			text = text.strip();
			iList = text.split( "<" )

			numberItems = len( iList )
			if numberItems != 3:
				print "Error: cannot parse options file, line:\n"
				print " " + text
				continue

			# insert into dict
			tag = iList[0]
			tag = tag.strip()
			value = iList[1]
			value = value.strip()
			self.oList.append( [ tag, value ] )

		optFile.close()

	#------------------------------

	def writeAndClose( self ):
		if len( self.oList ) == 0:
			return

		# close the file and save the options
		optFile = file( self.optFileName, 'w' )
		optFile.write( "<" + self.optFileName + ">\n" )
		if self.oList[0] != "":
			for item in self.oList:
				tag = item[0]
				value = item[1]
				optFile.write( "\t<" + tag + ">\t" + value + "\t</" + tag + ">\n" )
			optFile.write( "</" + self.optFileName + ">\n" )
		optFile.close()

	#------------------------------

	def getTags( self ):
		tags = []
		for item in self.oList:
			tags.append( item[0] )
		return tags

	#------------------------------

	def read( self, tag ):
		for item in self.oList:
			if item[0] == tag:
				return item[1]
		return ""

	#------------------------------

	def write( self, tag, value ):
		for item in self.oList:
			if item[0] == tag:
				item[1] = value
				return
		# insert at the end
		self.oList.append( [tag, value] )

	#------------------------------

	def erase( self, tag ):
		itemToRemove = []
		for item in self.oList:
			if item[0] == tag:
				itemToRemove = item
				break
		self.oList.remove( itemToRemove )

	#------------------------------

	def readTags( self, tag ):
		values = []
		for item in self.oList:
			if item[0] == tag:
				values.append( item[1] )
		return values

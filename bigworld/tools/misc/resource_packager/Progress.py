import sys

class Progress:
	def __init__( self, title, totalItems ):
		self.progress = 1.0
		self.threshold = totalItems / 50.0
		self.incrAmount = 1.0
		
		print ""
		print title + " Please wait..."
		print "[--------------------------------------------------]"
		sys.stdout.write(" ")
		
	def increment( self ):
		self.progress += self.incrAmount
		while self.progress > self.threshold:
			sys.stdout.write(".")
			self.progress -= self.threshold
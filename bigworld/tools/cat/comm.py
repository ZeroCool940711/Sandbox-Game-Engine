# provide a wrapper for basic telnet functionality

import telnetlib
import wx

class CommTelnet:
	def __init__( self ):
		self.telnet = None
		self.ip = ""
		self.port = ""
		self.reconnected = True

	#------------------------------

	def connect( self, ip, port ):
		# let the user know this may take a while
		wait = wx.BusyCursor()
		print "Connecting to %s:%d ..." % (ip, port)
		try:
			self.telnet = telnetlib.Telnet( ip, port )
			print "Connection established."

			# import the libs we need
			self.post( "import cPickle" )

			# store where connected to in case need to reconnect
			self.ip = ip
			self.port = port

			self.reconnected = True
			return True
		except Exception, e:
			self.telnet = None
			print "Connection failed."
			return False

	#------------------------------

	def disconnect( self ):
		wait = wx.BusyCursor()
		if self.telnet != None:
			self.telnet.close()
			self.telnet = None
			print "Connection closed."

	#------------------------------

	def connected( self ):
		return self.telnet != None

	#------------------------------

	def checkConnection( self ):
		if not self.connected():
			return False

		ok = True
		try:
			# test the connection is alive by clearing buffer (need to do this anyhow)
			self.telnet.read_very_eager()
			# ask again, using read_eager as this may flush out the buffers.
			self.telnet.read_eager()
		except EOFError:
			print "CommTelnet: Lost connection. Trying to reconnect"
			ok = self.connect( self.ip, self.port )
			if ok:
				# clear read buffer
				self.telnet.read_very_eager()
		return ok

	#------------------------------

	def hasReconnected( self ):
		if self.reconnected:
			self.reconnected = False
			return True
		return False

	#------------------------------

	def post( self, command ):
		# if weren't in a connection, then don't try to reconnect
		if not self.connected():
			print "CommTelnet: No connection."
			return False

		# print "post:", command
		ok = True
		try:
			# test the connection is alive by clearing buffer (need to do this anyhow)
			self.telnet.read_very_eager()
			# ask again, to make sure (may have only looked locally before)
			self.telnet.read_very_eager()
		except EOFError:
			print "CommTelnet: Lost connection. Trying to reconnect"
			ok = self.connect( self.ip, self.port )


		if ok:
			# clear read buffer
			self.telnet.read_very_eager()
			command = command.strip()
			self.telnet.write( command + "\r" )
			# get the echoed command string
			self.telnet.read_until("\r")
			return True
		else:
			return False

	#------------------------------

	def ask( self, command ):
		if not self.post( command ):
			return ""

		# wait for a line of response
		response = self.telnet.read_until("\r")
		response = response.strip()
		
		# We sometimes get different responses when an error occurs
		# handle the common ones
		if response.startswith("Traceback") or \
			response.startswith("TypeError") or \
			response.startswith(">>>") or \
			response.startswith("File"):
			return ""
		else:
			return response.strip()


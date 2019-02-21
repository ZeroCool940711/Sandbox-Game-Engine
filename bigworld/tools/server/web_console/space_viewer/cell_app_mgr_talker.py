import socket
import struct
import select
import copy
import StringIO

import bwsetup; bwsetup.addPath( "../.." )

from pycommon.util import MFRectangle
from pycommon.watcher_data_type import BW_UNPACK3 as BW_UNPACK3

#if this variable is true modifications done by spaceviewer flex are included
FLEX_SPACEVIEWER=True

class BranchNode:
	def __init__( self, position, load, isHorizontal ):
		self.position = position
		self.load = load
		self.left = None
		self.right = None
		self.isHorizontal = isHorizontal

	def visit( self, rect, visitor ):
		leftRect = copy.deepcopy( rect )
		rightRect = copy.deepcopy( rect )

		if self.isHorizontal:
			leftRect.maxY = self.position
			rightRect.minY = self.position
			pt1 = (rect.minX, self.position)
			pt2 = (rect.maxX, self.position)
		else:
			leftRect.maxX = self.position
			rightRect.minX = self.position
			pt1 = (self.position, rect.minY)
			pt2 = (self.position, rect.maxY)

		if hasattr( visitor, "visitInterval" ):
			visitor.visitInterval( self, pt1, pt2 )

		self.left.visit( leftRect, visitor )
		self.right.visit( rightRect, visitor )

	def printTree( self, depth = 0 ):
		print "  " * depth, self.isHorizontal, self.position
		self.left.printTree( depth + 1 )
		self.right.printTree( depth + 1 )

	def cellAt( self, x, y ):
		if (not self.isHorizontal and (x < self.position)) or \
				(self.isHorizontal and (y < self.position)):
			return self.left.cellAt( x, y )
		else:
			return self.right.cellAt( x, y )

class LeafNode:
	def __init__( self, addr, load, id, viewerPort, entityBounds,
			chunkBounds, isRetiring, isOverloaded ):
		self.addr = addr
		self.load = load
		self.viewerPort = viewerPort
		self.entityBounds = entityBounds
		self.chunkBounds = chunkBounds
		self.appID = id
		self.isRetiring = isRetiring
		self.isOverloaded = isOverloaded

	def __str__( self ):
		s = "%d\n" % self.appID
		s += "%s:%d @ %.2f\n" % \
			 (socket.inet_ntoa( struct.pack( "I", self.addr[0] ) ),
			  self.addr[1], self.load)
		s += "chunk bounds: %s\n" % self.chunkBounds
		s += "entity bounds: %s\n" % self.entityBounds
		return s

	def visit( self, rect, visitor ):
		if hasattr( visitor, "visitCell" ):
			visitor.visitCell( self, rect )

	def printTree( self, depth = 0 ):
		print "  " * depth, hex( self.addr[0] ), hex( self.addr[1] )

	def cellAt( self, x, y ):
		return self

# This class wraps a socket and makes it look a little like a file object.
# Well... only implements the read method for now.
class SocketFileWrapper:

	def __init__( self, socket ):
		self.socket = socket

	def read( self, numToRead ):
		numOutstanding = numToRead
		result = ''

		while numOutstanding > 0:
			incoming = self.socket.recv( numOutstanding );
			if len( incoming ) == 0:
				raise socket.error, "socket connection broken"

			result += incoming;
			numOutstanding -= len( incoming )

		return result

class CellAppMgrTalker:
	"""
	Handles TCP connection to the Cell App Manager.
	"""

	# These must match
	GET_CELLS = 'b'
	REMOVE_CELL = 'c'
	STOP_CELL_APP = 'd'
	GET_VERSION = 'e'
	GET_SPACE_GEOMETRY_MAPPINGS = 'f'

	def __init__( self, space, addr = None ):
		"""
		Create a talker for the given space, and optionally connect it to the
		cellappmgr at the given address.  Loggers will be connected, viewers
		will not.  This of course means that if a viewer calls any method that
		uses the socket, an exception will be thrown - naughty!
		"""
		if FLEX_SPACEVIEWER:
			self.cellList=[]
		self.appsTotal = 0
		self.space = space
		self.addr = addr

		# The following fields are only set for talker objects which are
		# actually connected to the cellappmgr (i.e. those running on loggers)
		if self.addr:

			# A buffer we use to record the network stream each tick
			self.streamBuf = StringIO.StringIO()

			self.s = socket.socket( socket.AF_INET, socket.SOCK_STREAM )
			self.s.connect( self.addr )

			self.getCells()

	def visit( self, rect, visitor ):
		if self.root:
			self.root.visit( rect, visitor )

	def deleteCell( self, cell, sock = None ):

		# IP, port, salt, spaceID
		toSend = struct.pack( "<cIHHI", self.REMOVE_CELL,
				cell.addr[0], cell.addr[1], 0, cell.spaceId )

		# This is a bit wierd, but to save implementing this send() in two
		# places I've allowed specification of the socket to send the message on
		# so that a replayer can send this message to a logger, as well as the
		# normal case of the logger sending it to the actual cellappmgr.
		if sock == None:
			sock = self.s
		sock.send( toSend )

	def deleteCellApp( self, cell, sock = None ):
		# IP, port, salt
		toSend = struct.pack( "<cIHH", self.STOP_CELL_APP,
				cell.addr[0], cell.addr[1], 0 )

		# Same optional socket hackery as above
		if sock == None:
			sock = self.s
		sock.send( toSend )

	def cellAt( self, x, y ):
		return self.root.cellAt( x, y )

	def getCellAppID( self, addr ):
		if self.cells.has_key( addr ):
			return self.cells[ addr ].appID

	def getTitle( self ):
		return "Space %d. Cell Apps %d of %d" % \
			(self.space, len(self.cells), self.appsTotal)

	def getCells( self, streamBuf = None ):
		"""
		If streamBuf is None, this is a logger process fetching the BSP from a
		cellappmgr.  The stream read from the cellappmgr will be returned.

		If streamBuf is not None, this is a replayer process reconstructing the
		BSP from a stream sent by the logger.  Nothing is returned.
		"""

		if not streamBuf:
			fetch = lambda n: self._getNBytes( n )

			# Clear old stream buffer
			self.streamBuf.seek( 0 ); self.streamBuf.truncate()

			# Request the cell info
			if not self.s.send( struct.pack( "=cI", self.GET_CELLS,
											 self.space ) ):
				raise socket.error, "socket connection broken"

		else:
			fetch = lambda n: streamBuf.read( n )


		# Read reply header
		dataLength, spaceID, self.appsTotal, self.numEntities = \
							struct.unpack( "iiii", fetch( 16 ) )

		if FLEX_SPACEVIEWER:
			self.cellList=[]
		stack = []
		self.root = None
		self.cells = {}

		while stack or not self.root:

			nodeType, = struct.unpack( 'b', fetch( 1 ) )
			shouldAppend = False

			# Leaf nodes (i.e. cellapps)
			if nodeType == 2:

				# Header
				ip, port, salt, load, appID, viewerPort = \
						struct.unpack( "IHHfIH", fetch( 18 ) )

				# Entity and chunk bounds
				b = struct.unpack( "ffff", fetch( 16 ) )
				entityBounds = MFRectangle( b[0], b[2], b[1], b[3] )
				b = struct.unpack( "ffff", fetch( 16 ) )
				chunkBounds = MFRectangle( b[0], b[2], b[1], b[3] )

				isRetiring = fetch( 1 ) != '\0'
				isOverloaded = fetch( 1 ) != '\0'
				addr = (ip, port)

				# Create object for cell and map it in
				newNode = LeafNode( addr, load, appID,
									viewerPort, entityBounds, chunkBounds,
									isRetiring, isOverloaded )
				self.cells[ addr ] = newNode
				if FLEX_SPACEVIEWER:
					temp={'nodeType':2,'appID':appID}
					self.cellList.append(temp)

			# Branch nodes
			elif nodeType in (0, 1):
				position, load = struct.unpack( '<ff', fetch( 8 ) )
				newNode = BranchNode( position, load, nodeType == 0 )
				shouldAppend = True
				if FLEX_SPACEVIEWER:
					temp={'nodeType':nodeType,'position':position}
					self.cellList.append(temp)

			else:
				newNode = None

			if stack:
				if stack[-1].left == None:
					stack[-1].left = newNode
				else:
					stack[-1].right = newNode
					stack.pop()
			else:
				assert( not self.root )
				self.root = newNode

			if shouldAppend:
				stack.append( newNode )

		# If the post BSP dataLength is not 0, then we didn't get the full tree
		# somehow (very bad!)
		dataLength, = struct.unpack( "i", fetch( 4 ) )
		if dataLength != 0:
			raise RuntimeError, \
				  "Incomplete BSP read from socket (%d byte overflow)" % \
				  dataLength

		if not streamBuf:
			return self.streamBuf.getvalue()

	def getVersion( self ):
		msg = struct.pack( "=c", self.GET_VERSION )
		if not self.s.send( msg ):
			raise socket.error, "socket connection broken"

		stream = SocketFileWrapper( self.s )
		replyLen, = struct.unpack( "<I", stream.read( 4 ) )
		if (replyLen > 0):
			return struct.unpack( "<H", stream.read( 2 ) )
		else:
			return 0;

	def getSpaceGeometryMappings( self ):
		# Send request
		msg = struct.pack( "=cI", self.GET_SPACE_GEOMETRY_MAPPINGS, self.space )
		if not self.s.send( msg ):
			raise socket.error, "socket connection broken"

		# Process response
		stream = SocketFileWrapper( self.s )
		return self.unpackSpaceGeometryMappingsMsg( stream )

	def unpackSpaceGeometryMappingsMsg( self, stream ):
		mappings = []

		msglen, = struct.unpack( "<I", stream.read( 4 ) )
		while (msglen > 0):
			key, = struct.unpack( "<H", stream.read( 2 ) )

			matrix = []
			matrix.append( struct.unpack( "<ffff", stream.read( 16 ) ) )
			matrix.append( struct.unpack( "<ffff", stream.read( 16 ) ) )
			matrix.append( struct.unpack( "<ffff", stream.read( 16 ) ) )
			matrix.append( struct.unpack( "<ffff", stream.read( 16 ) ) )

			strlen = BW_UNPACK3( stream )
			geometryPath = stream.read( strlen )

			mappings.append( (key, matrix, geometryPath) )

			msglen -= 2 + 16 + 16 + 16 + 16 + strlen
			if (strlen < 255):
				msglen -= 1
			else:
				msglen -= 4

		return mappings


	# --------------------------------------------------------------------------
	# Section: Private Methods
	# --------------------------------------------------------------------------

	def _getNBytes( self, n ):
		"""
		Read n bytes from the socket, and record everything that is read into
		self.streamBuf.
		"""

		toGet = n
		toRet = ''

		while toGet > 0:
			incoming = self.s.recv( toGet );
			if len( incoming ) == 0:
				raise socket.error, "socket connection broken"
				# return toRet
			toRet += incoming;
			toGet -= len( incoming )
			self.streamBuf.write( incoming )
		return toRet

# cell_app_mgr_talker.py

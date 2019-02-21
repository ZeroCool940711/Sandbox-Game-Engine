import wx

class RubberBander:

	""" 
	Manages mouse events / rubberbanding 
	"""

	def __init__( self ):
		self.m_stpoint = wx.Point( 0,0 )
		self.m_endpoint = wx.Point( 0, 0 )
		self.m_savepoint = wx.Point( 0, 0 )
		self.w = 0
		self.h = 0
		
		self.middleClicked = 0
		self.selected = 0
		
	def onMouseEvent( self, event ):
		if event:
			if event.MiddleDown():
				self.m_stpoint = event.GetPosition()
				self.m_savepoint = self.m_stpoint
				self.m_endpoint = self.m_stpoint
				self.w = 0
				self.h = 0
				self.selected = 0
				self.middleClicked = 1
		
			elif event.Dragging():	
				if self.middleClicked:
					self.m_endpoint =  event.GetPosition()
					self.w = (self.m_endpoint.x - self.m_stpoint.x)
					self.h = (self.m_endpoint.y - self.m_stpoint.y)
					self.m_savepoint = self.m_endpoint 
						
			elif event.MiddleUp():	
				self.selected = 1  #selection is done
				self.middleClicked = 0 # end of clicking  
				
	def GetCurrentSelection(self):
		""" Return the current selected rectangle """
		
		left = wx.Point( 0, 0 )
		right = wx.Point( 0, 0 )

		if self.m_endpoint.y > self.m_stpoint.y:
			right = self.m_endpoint
			left = self.m_stpoint
			
		elif self.m_endpoint.y < self.m_stpoint.y:
			right = self.m_stpoint
			left = self.m_endpoint
		else:
			right = self.m_stpoint
			left = self.m_stpoint
			
		return (left.x, left.y, right.x, right.y)

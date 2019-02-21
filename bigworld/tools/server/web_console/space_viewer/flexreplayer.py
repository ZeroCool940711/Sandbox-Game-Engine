from space_viewer import replay
class flexreplayer:
	def __init__(self, replayer,UID,spaceId,loginUser):
		self.replayer=replayer
		self.UID=UID
		self.spaceId=spaceId
		self.prevX=-500
		self.prevY=500
		self.loginUser=loginUser
		

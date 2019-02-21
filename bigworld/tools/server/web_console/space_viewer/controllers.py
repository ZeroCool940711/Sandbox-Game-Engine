import cherrypy
import turbogears
import tempfile
import os
import time
import socket
import struct
import thread

from turbogears import controllers, expose, redirect
from turbogears import identity
import svlogger
import flexreplayer
import replay

#import xmlprefs
from xml.dom import minidom
from xml.dom.minidom import parse, parseString


import model

# BigWorld modules
import bwsetup;
bwsetup.addPath( "../.." )
from pycommon import xmlprefs
from pycommon import cluster
from pycommon import uid as uidmodule
from pycommon import util as pyUtil
from web_console.common import model, module, util
from pycommon.util import MFRectangle

# Allows disabling of SpaceViewer for Austin
ENABLE_SPACE_VIEWER = True

UPLOAD_DIR = os.path.join(os.getcwd(),"space_viewer/static/assets")
if not os.path.exists(UPLOAD_DIR):
    os.makedirs(UPLOAD_DIR)

class SpaceViewer( module.Module ):
	def __init__( self, *args, **kw ):
		module.Module.__init__( self, *args, **kw )
		if  ENABLE_SPACE_VIEWER:
			self.addPage( "Space Viewer", "sv_main" )
		self.addPage( "All Users", "allusers")
		self.addPage( "Global Settings","globalPrefSetting")
		self.addPage( "Upload Files","uploadKid")
		self.addPage( "Delete Images","deleteKid")
		self.addPage( "Help", "help", popup=True )
		#Replayers declaration
		self.lock=thread.allocate_lock()
		self.replayers=[]

	@identity.require( identity.not_anonymous() )
	@expose( template="common.templates.help_panes" )
	def help( self ):
		return dict( PAGE_TITLE="BigWorld WebConsole: SpaceViewer Help")

	@identity.require( identity.not_anonymous() )
	@expose( template="common.templates.help_left_pane" )
	def helpLeftPane( self ):
		return dict( section="space_viewer" )

	@identity.require( identity.not_anonymous() )
	@expose( template="space_viewer.templates.sv_main" )
	def sv_main( self, user=None ):
		self.mycluster = cluster.Cluster()
		# Set up a mapping from uid -> Process for each cellappmgr
		self.cellappmgrs = {}
		#getProcs("cellappmgr") return proc objects of type cellappmgr
		for p in self.mycluster.getProcs( "cellappmgr" ):
			self.cellappmgrs[ p.uid ] = p

		if not user:
			user = self.getCurrentUser()
		return dict(user=user,serviceURL=cherrypy.request.base)

	@identity.require( identity.not_anonymous() )
	@expose( template="space_viewer.templates.warning" )
	def warning( self ):
		return dict()

	@identity.require(identity.not_anonymous())
	@expose()
	def index(self):
		if ENABLE_SPACE_VIEWER:
			raise redirect( turbogears.url( "sv_main" ) )
		else:
			raise redirect( turbogears.url( "warning" ) )

	@identity.require(identity.not_anonymous())
	@expose( template="space_viewer.templates.allusers" )
	def allusers( self ):
		c = cluster.Cluster()
		activeUsers = sorted ( c.getUsers() )
		return dict(activeUsers = activeUsers)

	@identity.require( identity.not_anonymous() )
	@expose()
	def usersFlush( self ):
		c = cluster.Cluster()
		ms = c.getMachines()
		for m in ms:
			m.flushMappings()

		raise redirect( "allusers" )

	def getCurrentUser(self):
		user = None
		user = util.getUser(self.mycluster, user)
		if user:
			return user.name
		else:
			return None

	@identity.require(identity.not_anonymous())
	@expose()
	def getSpaces(self,UserIn=None):
		spaceList=[]
		if not UserIn:
			UserIn = self.getCurrentUser()
		for (uid, proc) in self.cellappmgrs.items():
			user = self.mycluster.getUser( uid )
			if str(user.name)==UserIn:
				spaces = proc.getWatcherData( "spaces" )
				for wd in spaces.getChildren():
					spaceList.append(str(wd.name))
		return dict(spaces=spaceList)

	@expose()
	@expose("json")
	def getData(self,UID,space,x1=0,y1=0,x2=0,y2=0,**keywords):
		"""
		This function returns all the data required to draw space		
		"""
		# This is flag used in checking for available replayer object
		avail=False
		self.defaultUser=UID
		gridResolution=None
		entityData=None
		ghostEntityData=None
		cellBoundary=None
		cellRect=None
		cellRects=None
		cellDataIds=None
		cellDataIps=None
		chunkBounds1=None
		chunkBounds=None
		typeNames=None
		entityBoundsList=None
		BranchLoads=None
		spaceGoemetry=None
		cellList=None
		if not cherrypy.request.identity.user:
			return dict()
		loginUser=cherrypy.request.identity.user.user_name
		#getting previous replayer object which is already created
		i=0
		while i < len(self.replayers):                   
			if self.replayers[i].UID==UID and self.replayers[i].spaceId==space and self.replayers[i].loginUser==loginUser:
				try:
					#self.lock.acquire()
					self.replayers[i].replayer.update()
					#self.lock.release()
				except:
					print "error while updating"
					self.replayers[i].replayer.stopLogger()
					del self.replayers[i]
					#self.lock.release()
					continue

				if not self.replayers[i].replayer.ct:
					print "ct is None"
					self.selectCell(UID,space,self.replayers[i].prevX,self.replayers[i].prevY)
					return dict()
				avail=True
				break
			i=i+1
		if avail:
			replayer=self.replayers[i].replayer
			#replayer.update()
		else:
			replayer=self.createReplayer(UID,space,loginUser)
		spaceBounds=replayer.spaceBounds
		#wclip is a rectangle in original application it was dc

		wclip=MFRectangle(float(x1),float(y1),float(x2),float(y2))
		#This class is to get cell boundary point visitInterval executes in cellappmgr
		class EdgeDrawer:
			def __init__( self):
				self.listOfPoints=[]
				self.BranchLoads=[]
			def visitInterval( self, branch, pt1, pt2 ):
				self.listOfPoints.append(pt1)
				self.listOfPoints.append(pt2)
				self.BranchLoads.append(branch.load)

		edgeobj=EdgeDrawer()
		replayer.cmt.visit( wclip,edgeobj)
		cellList=replayer.cmt.cellList
		cellBoundary=edgeobj.listOfPoints
		BranchLoads=edgeobj.BranchLoads
		spaceGoemetry=[]
		for types,matrix,path in replayer.spaceGeometryMappings:
			spaceGoemetry.append({'types':types , 'matrix':matrix ,'path' :path})

		#this class is to get cell related data and selected cell
		class CentreDrawer:
			def __init__(self,replayer_temp):
				self.cellRect=[]
				self.replayer=replayer_temp
				self.cellRects=[]
				self.cellDataIds=[]
				self.cellDataIps=[]
				self.cellLoads=[]
				self.entityBoundsList=[]
			def visitCell(self,cell,rect):
				ct=self.replayer.ct
				templist=[]
				templist.append(rect.minX)
				templist.append(rect.minY)
				templist.append(rect.maxX)
				templist.append(rect.maxY)
				#here we converting the MFRectangle object into list
				self.cellRects.append(templist)
				self.cellDataIds.append(cell.appID)
				self.cellDataIps.append(str(socket.inet_ntoa( struct.pack( "<I", cell.addr[0] ))))
				self.cellLoads.append(cell.load)
				templist1=[]
				templist1.append(cell.entityBounds.minX)
				templist1.append(cell.entityBounds.minY)
				templist1.append(cell.entityBounds.maxX)
				templist1.append(cell.entityBounds.maxY)
				self.entityBoundsList.append(templist1)
				if ct and cell.addr ==ct.addr:
					self.cellRect.append(rect.minX)
					self.cellRect.append(rect.minY)
					self.cellRect.append(rect.maxX)
					self.cellRect.append(rect.maxY)
					self.cellRect.append(cell.appID)
					self.cellRect.append(str(socket.inet_ntoa( struct.pack( "<I", cell.addr[0] ))))
					self.cellRect.append(cell.load)
		centreobj=CentreDrawer(replayer)
		replayer.cmt.visit(wclip,centreobj)
		cellRect=centreobj.cellRect
		cellRects=centreobj.cellRects
		cellDataIds=centreobj.cellDataIds
		cellDataIps=centreobj.cellDataIps
		cellLoads=centreobj.cellLoads
		entityBoundsList=centreobj.entityBoundsList
		if replayer.ct:
			ghostEntityData=replayer.ct.ghostEntityData
			entityData=replayer.ct.entityData
			gridResolution=replayer.ct.gridResolution
			cell = replayer.cmt.cells[ replayer.ct.addr ]
			chunkBounds1=cell.chunkBounds
			chunkBounds=[]
			chunkBounds.append(chunkBounds1.minX)
			chunkBounds.append(chunkBounds1.minY)
			chunkBounds.append(chunkBounds1.maxX)
			chunkBounds.append(chunkBounds1.maxY)
			typeNames=replayer.ct.typeName
		else:
			#if replayer doesn't have any selected cell (this happens because of delay of socket response )
			#select default cell or previously selected cell
			i=0
			while i < len(self.replayers):
				if self.replayers[i].UID==UID and self.replayers[i].spaceId==space and self.replayers[i].loginUser==loginUser:
					self.selectCell(UID,space,self.replayers[i].prevX,self.replayers[i].prevY)
					break
				i=i+1


		return dict(gridResolution=gridResolution,spaceBounds=spaceBounds,
			    ghostEntityData=ghostEntityData,entityData=entityData,
			    cellBoundary=cellBoundary,cellRect=cellRect,cellRects=cellRects,
			    cellDataIds=cellDataIds,cellDataIps=cellDataIps,chunkBounds=chunkBounds,
			    cellLoads=cellLoads,typeNames=typeNames,entityBoundsList=entityBoundsList,
			    branchLoads=BranchLoads,spaceGoemetry=spaceGoemetry,cellList=cellList)

	@expose()
	@expose("json")
	def getEntityNames(self,**keywords):
		"""
		This function returns all the data required to draw space
		it is flag used in checking available replayer object
		"""
		avail=False
		typeNames=None
		typeList=None
		if not cherrypy.request.identity.user:
			return dict()
		loginUser=cherrypy.request.identity.user.user_name
		#getting previous replayer object which is already created
		i=0
		while i < len(self.replayers):
			if self.replayers[i].loginUser==loginUser:
				try:
					self.replayers[i].replayer.update()
				except:
					print "error while updating"
					break
				avail=True
				break
			i=i+1
		if avail:
			replayer=self.replayers[i].replayer
			#replayer.update()
		else:
			return dict(typeNames=typeNames)
		if replayer.ct:
			typeNames=replayer.ct.typeName
			typeList=[]
			for k,v in typeNames.iteritems():
				typeList.append(v)
		return dict(typeNames=typeList)

	@identity.require(identity.not_anonymous())
	def createReplayer(self,UID,space,loginUser):
		# Get temp filename to write listener address to
		fd, tmp = tempfile.mkstemp()
		os.close( fd )
		os.unlink( tmp )
		os.spawnl( os.P_NOWAIT, pyUtil.interpreter(), pyUtil.interpreter(),
			   svlogger.FULLPATH,"-s", str( space ),"-u", str( UID ),"-w", tmp,"-k" )
		# Read the tempfile until we get an address out of it (have to wait for
		# logger to start up and write it
		while True :
			try:
				ip, port = open( tmp ).read().split( ":" )
				port = int( port )
				os.unlink( tmp )
				break
			except:
				time.sleep( 0.2 )
		# If the address is invalid, then the logger hasn't started up
		if ip == "none":
			print "invalid ip so terminating  returning none"
			return None
		# Make replayer
		replayer = replay.Replayer( (ip, port) )
		flexreplay=flexreplayer.flexreplayer(replayer,UID,space,loginUser)
		self.replayers.append(flexreplay)
		cell = replayer.cmt.cellAt(-500,500)
		replayer.selectCell(cell.addr)
		count=0
		# Wait untill Cell is selected
		while not replayer.ct:
			time.sleep(1)
			count=count+1
			if count>15:
				break
			try:
				replayer.update()
			except:
				print " error while updating replayer "

		return replayer

	@expose("json")
	@expose()
	@identity.require(identity.not_anonymous())
	def selectCell(self,UID,space,x,y,**keywords):
		avail=False
		#self.lock.acquire()
		loginUser=cherrypy.request.identity.user.user_name
		i=0
		while i < len(self.replayers):
			if self.replayers[i].UID==UID and self.replayers[i].spaceId==space and self.replayers[i].loginUser==loginUser:
				avail=True
				break
			i=i+1
		if avail:
			replayer=self.replayers[i].replayer
		else:
			#self.lock.release()
			return dict()
		x1=self.replayers[i].prevX
		y1=self.replayers[i].prevY
		cellprev=None
		#currently no cell is selected no need of checking
		if not replayer.ct:
			x=int(float(x))
			y=int(float(y))
			cell = replayer.cmt.cellAt(x,y)
			replayer.selectCell(cell.addr)
			self.replayers[i].prevX=x
			self.replayers[i].prevY=y
			time.sleep(1)
			replayer.update()
			if not replayer.ct:
				time.sleep(1)
			#self.lock.release()
			return dict()
		#currently cell is selected. If the request is for selecting same cell
		#don't do any thing
		if replayer.ct:
			#previously selected cell
			cellprev=replayer.ct.addr
			#requested cell
			cellreq=replayer.cmt.cellAt(int(float(x)),int(float(y))).addr
			if cellprev!=cellreq:
				x=int(float(x))
				y=int(float(y))
				cell = replayer.cmt.cellAt(x,y)
				replayer.selectCell(cell.addr)
				self.replayers[i].prevX=x
				self.replayers[i].prevY=y
				time.sleep(1)
				replayer.update()
				if not replayer.ct:
					time.sleep(1)
			#self.lock.release()
			return dict()

	def getDefaultUser(self):
		return self.defaultUser

	@expose("json")
	@expose()
	@identity.require(identity.not_anonymous())
	def getPref(self):
		"""
		This funtion returns the user level preferences
		"""
		spacelist=[]
		imagelist=[]
		imageoverlay=None
		imagepath=None
		#xmlprefs file is modified version of the exixting xmlpref of pycommon
		str1=os.path.dirname( __file__ )+"/preferences/pref.xml"
		myxml=xmlprefs.Prefs(str1,"pref")
		user=str(cherrypy.request.identity.user.user_name)
		#Getting the node for the requested user 
		userNode=myxml.getNode(user,True)
		for child in userNode.childNodes[:]:
			if str(child.nodeName)=="imageOverlay":
				attr=child.attributes
				spacelist.append(attr["name"].value)
				imagelist.append(str(child.childNodes[0].nodeValue))
		return dict(space_list=spacelist,image_paths=imagelist)

	@expose("json")
	@expose()
	@identity.require(identity.not_anonymous())
	def setPref(self,xmlStr=None,**keys):
		"""
		This function is save user level preferences
		"""
		str1=os.path.dirname( __file__ )+"/preferences/pref.xml"
		myxml=xmlprefs.Prefs(str1,"pref")
		xmlfromclient=parseString(xmlStr)
		mynode=xmlfromclient.firstChild
		mynodes=mynode.childNodes
		print len(mynodes)
		user=str(cherrypy.request.identity.user.user_name)
		#Getting the node for requested user
		userNode=myxml.getNode(user,True)
		#delete all the existing child nodes of the requested user node i.e saved preferences
		for itemNode in userNode.childNodes[:]:
			userNode.removeChild(itemNode)
			itemNode.unlink()
		#userNode.appendChild(mynode)
		for node1 in mynodes[:]:
			print node1
			userNode.appendChild(node1)
		myxml.save()
		return dict()

	@expose( template="space_viewer.templates.globalsettings" )
	@identity.require(identity.not_anonymous())
	def globalPrefSetting(self):
		user = self.getCurrentUser()
		serviceURL=cherrypy.request.base
		return dict(user=user,serviceURL=serviceURL)

	
	@expose( template="space_viewer.templates.upload" )
	@identity.require(identity.not_anonymous())
	def uploadKid(self):
		user = self.getCurrentUser()
		serviceURL=cherrypy.request.base
		return dict(user=user,serviceURL=serviceURL)

	@expose( template="space_viewer.templates.delete" )
	@identity.require(identity.not_anonymous())
	def deleteKid(self):
		user = self.getCurrentUser()
		serviceURL=cherrypy.request.base
		return dict(user=user,serviceURL=serviceURL)
	    
	    
	@expose("json")
	@expose()
	@identity.require(identity.not_anonymous())
	def setGlobalPref(self,entitysize=None,iconentity=None,iconpath=None):
		"""
		This funtion saves the global preferences which are related to entities
		"""
		str1=os.path.dirname( __file__ )+"/preferences/globalpref.xml"
		myxml=xmlprefs.Prefs(str1,"pref")
		user="global"
		userNode=myxml.getNode(user,True)
		#delete all the existing child nodes i.e previously saved
		for itemNode in userNode.childNodes[:]:
			userNode.removeChild(itemNode)
			itemNode.unlink()
		if entitysize:
			entityNode=myxml.doc.createElement( "entitysize" )
			text = myxml.doc.createTextNode( str( entitysize ) )
			entityNode.appendChild(text)
			userNode.appendChild(entityNode)
		if iconentity:
			iconNode=myxml.doc.createElement( "iconentity" )
			text1 = myxml.doc.createTextNode( str( iconentity ) )
			iconNode.appendChild(text1)
			userNode.appendChild(iconNode)
		if iconpath:
			iconPathNode=myxml.doc.createElement( "iconpath" )
			text2 = myxml.doc.createTextNode( str( iconpath ) )
			iconPathNode.appendChild(text2)
			userNode.appendChild(iconPathNode)
		myxml.save()
		return dict()

	@expose()
	@identity.require(identity.not_anonymous())
	def uploadFile(self, upload_file,IsImage,  **keywords):
		"""
		This function is used to upload files it reads data from request
		and creates file at server
		"""
		#Note: this function doesn't work for uploading .bmp files
		data = upload_file.file.read()
		#check whether file being uploaded is for image or icon
		if IsImage=="image":
			target_file_name =os.path.join(UPLOAD_DIR,"images",upload_file.filename.rsplit("\\")[len(upload_file.filename.rsplit("\\"))-1])
		else:
			target_file_name =os.path.join(UPLOAD_DIR,"icons",upload_file.filename.rsplit("\\")[len(upload_file.filename.rsplit("\\"))-1])
		try:
			#if file already exsists don't overwrite
			f1=open(target_file_name)
			f1.close()
			raise cherrypy.HTTPRedirect(turbogears.url("/sv/uploadError"))
		except IOError:
			#file doesn't exsists we can create new file
			f = open(target_file_name, 'w')
			f.write(data)
			f.close()
			#file is uploaded
			# A new node for image or icon should be added in there catalogue xml file.
			# Xml file is used in flex application for getting present images at server
			if IsImage=="image":
				str1=os.path.dirname( __file__ )+"/static/assets/catalogue.xml"
				myxml=xmlprefs.Prefs(str1,"catalogue")
				root=myxml.root
				productNode=myxml.doc.createElement( "image" )
				nameNode=myxml.doc.createElement( "name" )
				textName=myxml.doc.createTextNode( str( upload_file.filename.rsplit("\\")[len(upload_file.filename.rsplit("\\"))-1] ) )
				nameNode.appendChild(textName)
				imageNode=myxml.doc.createElement( "path" )
				textImage=myxml.doc.createTextNode( str( "../assets/images/"+str(upload_file.filename.rsplit("\\")[len(upload_file.filename.rsplit("\\"))-1] )) )
				imageNode.appendChild(textImage)
				productNode.appendChild(nameNode)
				productNode.appendChild(imageNode)
				root.appendChild(productNode)
				myxml.save()
			if IsImage=="icon":
				str1=os.path.dirname( __file__ )+"/static/assets/catalogueforicons.xml"
				myxml=xmlprefs.Prefs(str1,"catalogue")
				root=myxml.root
				productNode=myxml.doc.createElement( "image" )
				nameNode=myxml.doc.createElement( "name" )
				textName=myxml.doc.createTextNode( str( upload_file.filename.rsplit("\\")[len(upload_file.filename.rsplit("\\"))-1] ) )
				nameNode.appendChild(textName)
				imageNode=myxml.doc.createElement( "path" )
				textImage=myxml.doc.createTextNode( str( "../assets/icons/"+str(upload_file.filename.rsplit("\\")[len(upload_file.filename.rsplit("\\"))-1] )) )
				imageNode.appendChild(textImage)
				productNode.appendChild(nameNode)
				productNode.appendChild(imageNode)
				root.appendChild(productNode)
				myxml.save()
			del myxml
			raise cherrypy.HTTPRedirect(turbogears.url("/sv/successUpload"))

	@expose()
	@identity.require(identity.not_anonymous())
	def deleteFile(self, deleteFileName,IsImage,  **keywords):
		"""
		This function is used to deletes files it reads data from request
		and creates file at server
		"""
		if IsImage=="image":
			str1=os.path.dirname( __file__ )+"/static/assets/catalogue.xml"
			myxml=xmlprefs.Prefs(str1,"catalogue")
			root=myxml.root
			imageschilds=root.childNodes
			for images in imageschilds:
				imageNode=images
				nameNode=imageNode.firstChild
				textName=nameNode.childNodes[0].nodeValue
				if textName == deleteFileName:
					print "image found"
					print os.path.dirname( __file__ )+"/static/assets/images/"+deleteFileName
					if os.path.exists(os.path.dirname( __file__ )+"/static/assets/images/"+deleteFileName):
						os.unlink(os.path.dirname( __file__ )+"/static/assets/images/"+deleteFileName)  
					root.removeChild(imageNode)
					break
			myxml.save()
			del myxml
		if IsImage=="icon":
			str1=os.path.dirname( __file__ )+"/static/assets/catalogueforicons.xml"
			myxml=xmlprefs.Prefs(str1,"catalogue")
			root=myxml.root
			imageschilds=root.childNodes
			for images in imageschilds:
				imageNode=images
				nameNode=imageNode.firstChild
				pathNode=nameNode.nextSibling
				textName=nameNode.childNodes[0].nodeValue
				if textName == deleteFileName:
					print "image found"
					#print os.path.dirname( __file__ )+"/static/assets/icons/"+deleteFileName
					if os.path.exists(os.path.dirname( __file__ )+"/static/assets/icons/"+deleteFileName):
						os.unlink(os.path.dirname( __file__ )+"/static/assets/icons/"+deleteFileName)                                           
					root.removeChild(imageNode)
					break
				
			myxml.save()
			del myxml
			#raise cherrypy.HTTPRedirect(turbogears.url("/sv/successUpload"))
		return dict()

		    
	@expose( template="space_viewer.templates.uploaderror" )
	@identity.require(identity.not_anonymous())
	def uploadError(self):
		user = self.getCurrentUser()
		return dict()

	@expose( template="space_viewer.templates.uploadsuccess" )
	@identity.require(identity.not_anonymous())
	def successUpload(self):
		user = self.getCurrentUser()
		return dict()

	@identity.require(identity.not_anonymous())
	def onLogOut(self):
		loginUser=cherrypy.request.identity.user.user_name
		i=0
		#delete all the replayer objects created for that user
		while i < len(self.replayers):
			if self.replayers[i].loginUser==loginUser:
				self.replayers[i].replayer.stopLogger()
				del self.replayers[i]
				i=i-1
			i=i+1
		return dict()

	@expose("json")
	@expose()
	@identity.require(identity.not_anonymous())
	def saveGlobalPref(self,xmlstr,**key):
		"""
		This funtion saves the global preferences
		which are related to entities
		"""
		str1=os.path.dirname( __file__ )+"/preferences/gpref.xml"
		fileptr=open(str1,"w").write(xmlstr)
		return dict()

	@expose("json")
	@expose()
	@identity.require(identity.not_anonymous())
	def getGlobalPref(self,**key):
		"""
		This funtion returns the global preferences which are related to entities
		"""
		entitysize=None
		iconentity=None
		iconpath=None
		str1=os.path.dirname( __file__ )+"/preferences/gpref.xml"
		myxml=xmlprefs.Prefs(str1,"pref")
		user=myxml.root
		userNode=myxml.root
		entity_type=[]
		entity_path=[]
		for child in userNode.childNodes[:]:
			if str(child.nodeName)=="entitysize":
				entitysize=str(child.childNodes[0].nodeValue)
			elif str(child.nodeName)=="iconentity":
				attr=child.attributes
				entity_type.append(attr["type"].value)
				entity_path.append(str(child.childNodes[0].nodeValue))
				str(child.childNodes[0].nodeValue)
		return dict(entitysize=entitysize,entity_type=entity_type,entity_path=entity_path)
	
	
	@expose("json")
	@expose()
	@identity.require(identity.not_anonymous())
	def currentImages(self,**key):
		iconsPath=os.path.dirname( __file__ )+"/static/assets/icons"
		imagesPath=os.path.dirname( __file__ )+"/static/assets/images"
		iconList=os.listdir(iconsPath)
		imageList=os.listdir(imagesPath)
		return dict(currentIcons=iconList,currentImages=imageList)

	@expose("json")
	@expose()
	@identity.require(identity.not_anonymous())
	def currentImages1(self,**key):
		str1=os.path.dirname( __file__ )+"/static/assets/images"
		imageList=os.listdir(str1)
		return dict(currentImages=imageList)
		
	    



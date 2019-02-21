import cherrypy
import turbogears

class Module( turbogears.controllers.Controller ):

	instances = []

	def __init__( self, parent, name, path, icon, auth = (lambda: True) ):

		self.parent = parent
		self.name = name
		self.path = path
		self.icon = icon
		self.pages = []
		self.auth = auth

		Module.instances.append( self )

	def isCurrent( self ):
		return cherrypy.request.path.startswith( "/" + self.path )

	def attrs( self ):
		if self.isCurrent():
			return {"class": "top-level current"}
		else:
			return {"class": "top-level"}

	def header( self ):
		for page in self.pages:
			if page.isCurrent():
				return "%s :: %s" % (self.name, page.name)
		return self.name

	def addPage( self, name, path, popup = False, **kw ):
		self.pages.append( Page( self, name, path, popup, kw ) )

	@classmethod
	def all( self ):
		return self.instances

	@classmethod
	def current( self ):
		for module in self.instances:
			if module.isCurrent():
				return module
		raise RuntimeError, "No module is current.  Wahh!"

class Page( object ):

	def __init__( self, module, name, path, popup, params ):
		self.module = module
		self.name = name
		self.path = path
		self.params = params
		self.popup = popup

	def isCurrent( self ):
		return cherrypy.request.path.startswith(
			"/%s/%s" % (self.module.path, self.path) )

	def attrs( self ):
		if self.isCurrent():
			return {"class": "current"}
		else:
			return {"class": "not-current"}

	def url( self ):
		return turbogears.url( "/%s/%s" % (self.module.path, self.path),
							   **self.params )

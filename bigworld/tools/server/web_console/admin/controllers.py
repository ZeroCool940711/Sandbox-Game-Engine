from turbogears import expose, identity, redirect, validate, validators
from turbogears.identity import soprovider

import bwsetup; bwsetup.addPath( "../.." )
from pycommon import cluster
from web_console.common import module, model, util

class Admin( module.Module ):

	def __init__( self, *args, **kw ):
		module.Module.__init__( self, *args, **kw )
		self.addPage( "User Listing", "list" )
		self.addPage( "Add New User", "edit?action=add" )
		self.addPage( "Flush User Mappings", "flush" )


	@identity.require( identity.in_group( "admin" ) )
	@expose( template = "admin.templates.list" )
	def list( self ):

		users = sorted( list( model.User.select() ),
						key = lambda u: u.user_name )

		# Generate actions for each user
		options = {}
		for user in users:
			opt = util.ActionMenuOptions()

			opt.addRedirect( "Edit", "edit",
							 params = dict( username = user.user_name,
											action = "edit" ) )

			if str( user.user_name ) != "admin":
				opt.addRedirect( "Delete", "delete",
							 params = dict( username = user.user_name ) )

			options[ user ] = opt

		return dict( users = users, options = options )


	@identity.require( identity.in_group( "admin" ) )
	@validate( validators = dict( id = validators.Int() ) )
	@expose( template = "admin.templates.edit" )
	@util.unicodeToStr
	def edit( self, action, username = None, pass1 = None, pass2 = None,
			  serveruser = None, id = None ):

		if action == "add" and username is None:
			return dict( user = None )

		if action == "edit" and serveruser is None:
			user = list( model.User.select(
				model.User.q.user_name == username ) )[0]
			return dict( user = user )

		# Verify
		if pass1 != pass2:
			return self.error( "Passwords did not match" )

		elif not username:
			return self.error( "'username' must be defined" )

		elif not serveruser:
			return self.error( "'serveruser' must be defined" )

		c = cluster.Cluster()
		serverusers = c.getAllUsers()

		if serveruser not in [user.name for user in serverusers]:
			errStr = """Server user '%s' does not exist or does not 
			 			have a valid ~/.bwmachined.conf.  Click 'Flush User 
			            Mappings' to refresh user mappings for all 
						bwmachined processes, and try again."""
			return self.error( errStr % serveruser )

		if action == "add":

			priors = list( model.User.select(
				model.User.q.user_name == username ) )

			if priors:
				return self.error( "User '%s' already exists" % username )
			else:
				model.User( user_name = username,
							password = pass1,
							serveruser = serveruser )

		elif action == "edit":
			prior = model.User.get( id )
			if not prior:
				return self.error( "User ID '%d' doesn't exist" % id )
			else:
				prior.user_name = username
				prior.serveruser = serveruser
				if pass1:
					prior.password = pass1

		raise redirect( "list" )


	@identity.require( identity.in_group( "admin" ) )
	@expose( template = "admin.templates.flush" )
	def flush( self, confirmed = False ):
		
		# Flush user cache of all bwmachined in the network so that new
		# users in the network will be recognised.
		if not confirmed:
			return dict( )
		else:
			c = cluster.Cluster.get()
			ms = c.getMachines()
			for m in ms:
				m.flushMappings()

			raise redirect( "list" )


	@identity.require( identity.in_group( "admin" ) )
	@validate( validators = dict( confirmed = validators.StringBool() ) )
	@expose( template = "admin.templates.delete" )
	@util.unicodeToStr
	def delete( self, username = None, confirmed = False ):

		if not confirmed:
			return dict( username = username )
		else:
			rec = list( model.User.select(
				model.User.q.user_name == username ) )[0]
			rec.destroySelf()
			raise redirect( "list" )


	@identity.require( identity.in_group( "admin" ) )
	@expose()
	def index( self, **kw ):
		raise redirect( "list" )


	@identity.require( identity.not_anonymous() )
	@expose( template="web_console.common.templates.error" )
	def error( self, msg ):
		debugmsgs = util.getDebugErrors()
		return dict( msg = msg, debug = debugmsgs )

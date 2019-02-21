"""
Trivial subclassing of Catwalk to enforce identity control.
"""

import turbogears
from turbogears import identity
from turbogears.toolbox import catwalk

import bwsetup; bwsetup.addPath( "../.." )
from web_console.common import module

class SafeCatWalk( catwalk.CatWalk, module.Module ):
	def __init__( self, model, *args, **kw ):
		catwalk.CatWalk.__init__( self, model )
		module.Module.__init__( self, *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def error( self, *args, **kw ):
		return super( SafeCatWalk, self ).error( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def noacces( self, *args, **kw ):
		return super( SafeCatWalk, self ).noacces( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def add( self, *args, **kw ):
		return super( SafeCatWalk, self ).add( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def update( self, *args, **kw ):
		return super( SafeCatWalk, self ).update( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def remove( self, *args, **kw ):
		return super( SafeCatWalk, self ).remove( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def remove_single_join( self, *args, **kw ):
		return super( SafeCatWalk, self ).remove_single_join( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def saveModelOrder( self, *args, **kw ):
		return super( SafeCatWalk, self ).saveModelOrder( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def columnOrder( self, *args, **kw ):
		return super( SafeCatWalk, self ).columnOrder( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def instances( self, *args, **kw ):
		return super( SafeCatWalk, self ).instances( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def manageRelatedJoins( self, *args, **kw ):
		return super( SafeCatWalk, self ).manageRelatedJoins( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def updateJoins( self, *args, **kw ):
		return super( SafeCatWalk, self ).updateJoins( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def updateColumns( self, *args, **kw ):
		return super( SafeCatWalk, self ).updateColumns( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def updateColumnsJoinView( self, *args, **kw ):
		return super( SafeCatWalk, self ).updateColumnsJoinView( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def joins( self, *args, **kw ):
		return super( SafeCatWalk, self ).joins( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def instance( self, *args, **kw ):
		return super( SafeCatWalk, self ).instance( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def columnsForLabel( self, *args, **kw ):
		return super( SafeCatWalk, self ).columnsForLabel( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def setColumnForLabel( self, *args, **kw ):
		return super( SafeCatWalk, self ).setColumnForLabel( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def columns( self, *args, **kw ):
		return super( SafeCatWalk, self ).columns( *args, **kw )

	@turbogears.expose( format="json" )
	@identity.require( identity.in_group( "admin" ) )
	def list( self, *args, **kw ):
		return super( SafeCatWalk, self ).list( *args, **kw )

	@turbogears.expose( template="turbogears.toolbox.catwalk.catwalk" )
	@identity.require( identity.in_group( "admin" ) )
	def index( self, *args, **kw ):
		return super( SafeCatWalk, self ).index( *args, **kw )

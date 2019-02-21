/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#include "pch.hpp"

#include "py_resource_refs.hpp"
#include "resource_ref.hpp"
#include "cstdmf/bgtask_manager.hpp"

DECLARE_DEBUG_COMPONENT2( "Script", 0 )

// ----------------------------------------------------------------------------
// Section: BGResourceRefsLoader
// ----------------------------------------------------------------------------
/**
 *	Helper class to load resource references in the background.  It loads the
 *	list of resources specified by resourceIDs and stores the resulting refs
 *	in the passed in resourceRefs.  When finished it calls the callback func.
 */
class BGResourceRefsLoader : public BackgroundTask
{
public:
	BGResourceRefsLoader(
		const std::vector< std::string > & resourceIDs,
		PyObject * pCallback,
		int priority,
		PyResourceRefs* pResourceRefs  );

	virtual void doBackgroundTask( BgTaskManager & mgr );
	virtual void doMainThreadTask( BgTaskManager & mgr );

private:
	PyObjectPtr pCallback_;	
	std::vector< std::string >	resourceIDs_;
	PyObjectPtr pResourceRefs_;
};


/**
 *	Constructor.
 */
BGResourceRefsLoader::BGResourceRefsLoader(		
		const std::vector< std::string > & resourceIDs,
		PyObject * pCallback,
		int priority,
		PyResourceRefs* pResourceRefs ):
	resourceIDs_( resourceIDs ),
	pCallback_( pCallback ),
	pResourceRefs_( pResourceRefs  )
{	
	BgTaskManager::instance().addBackgroundTask( this, priority );
}


/**
 *	This method is called by the Background Task Manager from the background
 *	thread.  Here we can load items and do blocking i/o without interrupting
 *	the rendering thread.
 *
 *	@param	mgr	The background task manager.
 */
void BGResourceRefsLoader::doBackgroundTask( BgTaskManager & mgr )
{	
	PyResourceRefs* pRr = static_cast<PyResourceRefs*>(
		pResourceRefs_.getObject() );

	for (uint i = 0; i < resourceIDs_.size(); i++)
	{
		ResourceRef rr = ResourceRef::getOrLoad( resourceIDs_[i] );
		if (rr)
			pRr->addLoadedResourceRef(rr);
		else
			pRr->addFailedResourceID( resourceIDs_[i] );
	}

	BgTaskManager::instance().addMainThreadTask( this );	
}


/**
 *	This method is called by the Background Task Manager from the main
 *	thread.  Here we can finish up our loading operation and call back
 *	to the rendering thread.  Here we call our python callback function,
 *	as python should only be used in the rendering thread.
 *
 *	@param	mgr	The background task manager.
 */
void BGResourceRefsLoader::doMainThreadTask( BgTaskManager & mgr )
{
	Py_INCREF( pCallback_.getObject() );
	Script::call( &*(pCallback_),
		Py_BuildValue("(O)", (PyObject*)pResourceRefs_.getObject()),
		"BGResourceRefsLoader callback: " );
	Py_DECREF( pResourceRefs_.getObject() );
}



// ----------------------------------------------------------------------------
// Section: PyResourceRefs
// ----------------------------------------------------------------------------
/**
 *	This structure contains the function pointers necessary to provide a Python
 *	Mapping interface.
 */
static PyMappingMethods s_pyResourceRefMapping =
{
	PyResourceRefs::s_length,		// mp_length
	PyResourceRefs::s_subscript,	// mp_subscript
	NULL							// mp_ass_subscript
};

PY_TYPEOBJECT_WITH_MAPPING( PyResourceRefs, &s_pyResourceRefMapping )

PY_BEGIN_METHODS( PyResourceRefs )
	/*~ function PyResourceRefs.pop
	 *	@components{ all }
	 *
	 *	This function returns the resource associated with a particular resource
	 *	reference and erases it from PyResourceRefs ownership.  If there is no
	 *	python representation, or if the resource failed to load, None will
	 *	be returned.  In either case the PyResourceRefs object will no longer
	 *	keep a reference to the resource and this is now up to the caller to
	 *	manage.
	 *
	 *	@param	resourceID		the name of the required resource.	 
	 *
	 *	@return					python resource, or None.
	 */
	PY_METHOD( pop )
	/*~ function PyResourceRefs.has_key
	 *	@components{ all }
	 *
	 *	This function returns whether the PyResourceRefs has a reference
	 *	to the resource given by the key.	 	 
	 *
	 *	@param	resourceID		the name of the resource to check.
	 *
	 *	@return					True or False.
	 */
	PY_METHOD( has_key )
	/*~ function PyResourceRefs.keys
	 *	@components{ all }
	 *
	 *	This function returns the list of resourceIDs.	 
	 *
	 *	@return					list of strings.
	 */
	PY_METHOD( keys )
	/*~ function PyResourceRefs.items
	 *	@components{ all }
	 *
	 *	This function returns a list of tuples of (resourceID, object)
	 *	representing this objects resources by name and their python equivalent
	 *
	 *	@return					list of tuples of (resourceID, object).
	 */
	PY_METHOD( items )
	/*~ function PyResourceRefs.values
	 *	@components{ all }
	 *
	 *	This function returns a list of the python equivalents for the
	 *	resources held onto by this object.
	 *
	 *	@return					list of resource objects.
	 */
	PY_METHOD( values )
PY_END_METHODS()
PY_BEGIN_ATTRIBUTES( PyResourceRefs )	
	/*~ attribute ResourceRefs.failedIDs
	 *
	 *	This attribute contains a list of resources that failed to load.  It
	 *	is a subset of the resourceIDs list.	 
	 *
	 *	@type	Read-Only Object
	 */
	s_attributes_.di_.addMember( "failedIDs" );
	//Note above attribute is implemented in pyGetAttribute, we add it
	//manually here so it turns up as an available attribute using dir()
PY_END_ATTRIBUTES()

PY_SCRIPT_CONVERTERS( PyResourceRefs )

PY_FACTORY_NAMED( PyResourceRefs, "ResourceRefs", BigWorld )


/**
 *	Constructor.
 */
PyResourceRefs::PyResourceRefs(
	PyObject * pResourceIDs,
	PyObject * pCallbackFn,
	int priority,
	PyTypePlus * pType ) : 
PyObjectPlus( pType )
{	
	std::vector< std::string > resourceIDs;
	int sz = PyTuple_Size( pResourceIDs );
	for (int i = 0; i < sz; i++)
	{
		PyObject * pItem = PyTuple_GetItem( pResourceIDs, i );
		if (!PyString_Check( pItem )) continue;
		resourceIDs.push_back( PyString_AsString( pItem ) );
	}

	//Schedule resources for loading.  This object adds itself to the
	//Background task list and deletes itself when finished.
	new BGResourceRefsLoader( resourceIDs, pCallbackFn, priority, this );
}


/**
 *	Constructor.  This constructor wraps already loaded resource
 *	references (used by entity prerequisites)
 *
 *	@param	rr		ResourceRefs to wrap in a python object.
 *	@param	pType	Python type to initialse the PyResourceRefs as.
 */
PyResourceRefs::PyResourceRefs( const ResourceRefs& rr, PyTypePlus * pType ):
	PyObjectPlus( pType ),
	resourceRefs_( rr )
{
	for (size_t i=0; i<rr.size(); i++)
	{
		if (!rr[i])
		{
			pFailedIDs_.push_back(rr[i].id());
		}
	}
}


/**
 *	Get an attribute for python
 */
PyObject * PyResourceRefs::pyGetAttribute( const char * attr )
{	
	if (!_stricmp(attr, "failedIDs"))
	{
		PyObject* pList = PyList_New( pFailedIDs_.size() );

		for (size_t i=0; i<pFailedIDs_.size(); i++)
		{
			PyList_SET_ITEM(
				pList, i, PyString_FromString(pFailedIDs_[i].c_str()) );
		}

		return pList;
	}

	PY_GETATTR_STD();
	return PyObjectPlus::pyGetAttribute( attr );
}


/**
 *	This method gets the python wrapper for a particular instance,
 *	and removes the resource from our lists.  It is then up to the
 *	caller of PyResourceRefs.pop to manage the lifetime of the resource.
 *
 *	@param	args			Python string object containing the name of the resource.
 *
 *	@return	Python representation of the resource.
 */
PyObject * PyResourceRefs::py_pop( PyObject * args )
{
	char* resourceID;

	if (!PyArg_ParseTuple( args, "s", &resourceID ))
	{
		PyErr_SetString( PyExc_TypeError, "py_resource: Argument parsing error. "
			"Expected a string" );
		return NULL;
	}

	PyObject * ret = this->pyResource(resourceID);
	this->erase(resourceID);
	return ret;
}


/**
 *	This private method erases the resource and means we no longer
 *	manage it.
 *
 *	@param	resourceID		Name of the resource to erase. 
 */
void PyResourceRefs::erase( const std::string& name )
{
	ResourceRefs::iterator it = resourceRefs_.begin();
	ResourceRefs::iterator end = resourceRefs_.end();
	while (it != end)
	{
		ResourceRef& rr = *it;
		if (rr.id() == name)
		{
			resourceRefs_.erase(it);
			return;
		}
		++it;
	}
}


/**
 *	This private method gets the python wrapper for a particular instance,
 *	given a std::string as input.
 *
 *	@param	resourceID		Name of the resource
 *	@return	PyObject		Python representation of the resource.
 */
PyObject * PyResourceRefs::pyResource( const std::string& resourceID )
{
	std::vector<std::string>::iterator it =
		std::find( pFailedIDs_.begin(), pFailedIDs_.end(), resourceID );

	if (it != pFailedIDs_.end())
	{
		PyErr_SetString( PyExc_ValueError, "py_resource: Requested resource "
			"had previously failed to load." );
		return NULL;
	}

	ResourceRef * rr = this->refByName( resourceID );
	if (rr)
	{
		return rr->pyInstance();
	}
	else
	{
		PyErr_SetString( PyExc_KeyError, "py_resource: Requested resource "
			"not found in resources list." );
		return NULL;
	}
}


/**
 *	Find loaded resource ref by name.
 */
ResourceRef* PyResourceRefs::refByName( const std::string& name )
{
	for (size_t i=0; i<resourceRefs_.size(); i++)
	{
		if (resourceRefs_[i].id() == name)
			return &resourceRefs_[i];
	}

	return NULL;
}


/**
 *	Set an attribute for python
 */
int PyResourceRefs::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return PyObjectPlus::pySetAttribute( attr, value );
}


/**
 *	This method is called by the loading thread, and adds a loaded
 *	resource reference for us to keep a hold of.
 */
void PyResourceRefs::addLoadedResourceRef( ResourceRef& r )
{
	resourceRefs_.push_back(r);
}


/**
 *	This method is called by the loading thread, and indicates that a
 *	resource that was requested failed to load.
 */
void PyResourceRefs::addFailedResourceID( const std::string& resourceID )
{
	pFailedIDs_.push_back(resourceID);
}


/**
 *	Factory method
 */
PyObject * PyResourceRefs::pyNew( PyObject * args )
{
	PyErr_SetString( PyExc_SyntaxError,
		"BigWorld.ResourceRefs() cannot be constructed explicitly, it is "
		"an asynchronous operation.  Please use BigWorld.loadResourceListBG"
		" instead.");
	return NULL;
}


// -----------------------------------------------------------------------------
// Section: Mapping methods
// -----------------------------------------------------------------------------

/**
 *	This method finds the resource by the given resourceID, and is used
 *	to implement the mapping operator.
 *
 *	@param	resourceID	The resourceID.  Must be a string.
 *
 *	@return	The python object associated with input resourceID.
 */
PyObject * PyResourceRefs::subscript( PyObject* resourceID )
{
	char * res = PyString_AsString( resourceID );

	if(PyErr_Occurred())
	{
		return NULL;
	}

	return this->pyResource( res );
}


/**
 *	This method implements the python [] operator, and finds the resource by
 *	the given resourceID.
 *
 *	@param	self		The ResourceRefs object being subscripted.
 *	@param	resourceID	The resourceID.
 *
 *	@return PyObject	The python object at specified resourceID subscript.
/*static*/ PyObject* PyResourceRefs::s_subscript(
	PyObject* self, PyObject* resourceID )
{
	return ((PyResourceRefs*)(self))->subscript( resourceID );
}


/**
 *	This method returns the number of resourceIDs.
 *
 *	@return	The number of resourceIDs originally asked for.
 */
int PyResourceRefs::length()
{
	return resourceRefs_.size();
}


/**
 *	This method returns the number of resourceIDs, and
 *	implement the python len() function.
 *
 *	@param	self	The PyResourceRefs object.
 *	@return	int			The number of resourceIDs originally asked for.
 */
/*static*/ int PyResourceRefs::s_length( PyObject * self )
{
	return ((PyResourceRefs*)(self))->length();
}


/*~ function PyResourceRefs.has_key
 *	@components{ all }
 *
 *	This method checks if we have a resource by the given resourceID.
 *
 *	@param	key		A string containing the name of the resourceID to check for.
 *
 *	@return	Bool 	True if the resource exists, False otherwise.
 */
/**
 *	This method returns True if the given resource exists, or False if it doesn't.
 *
 *	@param args		A python tuple containing the arguments.
 */
PyObject* PyResourceRefs::py_has_key( PyObject* args )
{
	char * resourceID;

	if (!PyArg_ParseTuple( args, "s", &resourceID ))
	{
		PyErr_SetString( PyExc_TypeError, "Expected a string argument." );
		return NULL;
	}

	if (this->refByName(resourceID))
	{
		Py_RETURN_TRUE;
	}
	else
	{
		Py_RETURN_FALSE;
	}
}


/*~ function PyResourceRefs.keys
 *	@components{ all }
 *
 *	This method returns a list of all the resourceIDs, in the order in
 *  which they appear.
 *
 *	@return		The list of the resources (strings)
 */
/**
 *	This method returns a list of all the resources.
 */
PyObject* PyResourceRefs::py_keys( PyObject* /*args*/ )
{
	size_t size = resourceRefs_.size();

	PyObject * pList = PyList_New( size );

	for (size_t i = 0; i < size; i++)
	{
		PyList_SetItem( pList, i,
			PyString_FromString( resourceRefs_[i].id().c_str() ) );
	}

	return pList;
}


/*~ function PyResourceRefs.values
 *	@components{ all }
 *
 *	This method returns a list of all resources, in the order in which
 *  they appear in the XML.
 *
 *	@return		a list of Python object
 */
/**
 *	This method returns a list of all python resources.
 */
PyObject* PyResourceRefs::py_values(PyObject* /*args*/)
{
	size_t size = resourceRefs_.size();
	PyObject* pList = PyList_New( size );

	for (size_t i = 0; i < size; i++)
	{
		PyList_SetItem(pList, i,
			this->pyResource(resourceRefs_[i].id()));
	}

	return pList;
}


/*~ function PyResourceRefs.items
 *	@components{ all }
 *	This method returns a list of name and resource object tuples.
 *
 *	@return		The list of (name, PythonObject) tuples of our resources 
 */
/**
 *	This method returns a list of name and resource object tuples.
 */
PyObject* PyResourceRefs::py_items( PyObject* /*args*/ )
{
	size_t size = resourceRefs_.size();
	PyObject* pList = PyList_New( size );

	for (size_t i = 0; i < size; i++)
	{
		PyObject * pTuple = PyTuple_New( 2 );

		PyTuple_SetItem( pTuple, 0,
			PyString_FromString( resourceRefs_[i].id().c_str() ) );
		PyTuple_SetItem( pTuple, 1, this->pyResource(resourceRefs_[i].id()) );

		PyList_SetItem( pList, i, pTuple );
	}

	return pList;
}


/*~ function BigWorld.loadResourceListBG
 *
 *	This function loads a list of resources in the background thread and
 *	is designed to be use as an extension to the Entity prerequisites
 *	mechanism, for times when you don't know ahead if time what resources
 *	an entity requires, or if the entity changes in such a way that requires
 *	new resources to load.
 *
 *	The function takes a tuple of resource IDs, a callback function and an optional
 *	priority number as arguments.  The resource list is loaded in the background thread
 *	and then the callback function is called with a new ResourceRefs instance
 *	as the argument.  You can use the resources immediately, or you can hold
 *	onto the ResourceRefs object; the loaded objects are tied to its lifetime. 
 *	Nothing is returned from this function call immediately. The priority number defines 
 *  which resources are need to be loaded before others where a higher number indicates a
 *  higher priority.
 *
 *
 *	For example:
 *	@{
 *
 *	# In the example, a new item is obtained; the item is one of many available
 *	# and thus there is no way to know before-hand what resources will be
 *	# required.  When the item is obtained, a ResourceRefs object is
 *	# constructed; it loads the resources in the background, and calls back the
 *	# function when done.  While the resourceRefs object is in existence, it
 *	# will hold onto the resources making sure they aren't unloaded.  In this
 *	# case a model is immediately constructed and a gui texture is immediately
 *	# set - thus it is unnecessary to hold onto the extra resource refs at all.
 *	# There is an alternative example not being called, onLoad2, which does not
 *	# use the resources immediately, it uses the ResourceRefs object to hold
 *	# onto them until needed later.
 *
 *	self.obtainItem( "maps/items/icon_item4.tga","models/items/item4.model" )
 *
 *	def obtainItem( self, iconName, modelName ):
 *		resources = (iconName, modelName)
 *		BigWorld.loadResourceListBG(resources, self.onLoad)
 *
 *	# In this case we will construct the objects we needed, so we don't
 *	# have to keep a hold of the resource refs object.  Note we check
 *	# isDestroyed, as in this example self is an entity, and the entity
 *	# may have left the world during our resource loading.
 *	def onLoad( self, resourceRefs ):
 *		assert isinstance( self, BigWorld.Entity )
 *		if self.inWorld:
 *			failed = resourceRefs.failedIDs
 *			guiName = resourceRefs.resourceIDs[0]
 *			modelName = resourceRefs.resourceIDs[1]
 *
 *			#now you can construct models or set gui textures or whatever
 *			#without causing resource loads in the main thread.			 
 *
 *			if guiName not in failed:
 *				self.itemGUI.textureName = guiName
 *			else:
 *				ERROR_MSG( "Could not load gui texture %s" % (guiName,) )
 *
 *			if modelName not in failed:
 *				self.newItemModel = resourceRefs[modelName]
 *			else:
 *				ERROR_MSG( "Could not load model %s" % (modelName,) )
 *
 *	# In this case we won't use the resources straight away, so
 *	# we keep a hold of the resource refs until we need them.
 *	def onLoad2( self, resourceRefs ):
 *		if not self.inWorld:
 *			self.resourcesHolder = resourceRefs			
 *
 *	def discardItem( self ):
 *		#release the ResourceRefs object, freeing the references
 *		#to all associated resources.  If nobody else is using these
 *		#resources they will be freed.
 *		del self.resourcesHolder
 *	@}
 *
 *	@param	resourceList	a list of resources to load.  This is similar to
 *	the list returned by entity prerequisites.
 *
 *	@param	callbackFn		a function to be called back when the load of all
 *	resources is complete.  The callback function takes no arguments.
 *
 *  @param	priority		[optional] integer to indicate the priority number (defaults to 64).
 *
 *	@return					None
 */
PyObject * py_loadResourceListBG( PyObject * args )
{
	PyObject* resourceList;
	PyObject* callbackFn;
	int priority = BgTaskManager::DEFAULT;

	if (!PyArg_ParseTuple( args, "OO|i", &resourceList, &callbackFn, &priority ))
	{
		PyErr_SetString( PyExc_TypeError,
			"BigWorld.PyResourceRefs() expects "
			"a list of resource references, callback function and optional priority number." );
		return NULL;
	}

	if (!PyTuple_Check(resourceList))
	{
		PyErr_SetString( PyExc_TypeError,
			"BigWorld.PyResourceRefs() expects "
			"a tuple of resource IDs passed in as the first argument." );
		return NULL;
	}		

	if (!PyCallable_Check(callbackFn))
	{
		PyErr_SetString( PyExc_TypeError,
			"BigWorld.PyResourceRefs() expects "
			"a callback function passed in as the second argument." );
		return NULL;
	}

	//The new PyResourceRefs object is held by the background loader and
	//passed into the callback function.  The receiver can then either
	//store the PyResourceRefs object or use it immediately and let it
	//automatically delete itself when it goes out of scope.
	new PyResourceRefs( resourceList, callbackFn, priority );

	Py_Return;
}



PY_MODULE_FUNCTION( loadResourceListBG, BigWorld )

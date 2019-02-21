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

// This file contains code that uses both the pyscript and network libraries
// and is common to many processes. This is not big enough to be its own
// library...yet.

// BW Tech Headers
#include "pyscript/script.hpp"
#include "network/nub.hpp"
#include "pyscript/pyobject_plus.hpp"

namespace PyNetwork
{

// -----------------------------------------------------------------------------
// Section: File descriptor registration
// -----------------------------------------------------------------------------

// Maps from the file descriptor to a callback
typedef std::map< int, class PyFileDescriptorHandler * > FileDescriptorMap;

// Maps from the file descriptor to the callback associated with reading
FileDescriptorMap g_fdReadHandlers;

// Maps from the file descriptor to the callback associated with writing
FileDescriptorMap g_fdWriteHandlers;

// Stores the Nub used to register file descriptors.
Mercury::Nub * g_pNub = NULL;

/**
 *	This function initialises the script interface to the network library.
 *
 *	@param nub The nub that will be used to register file descriptors. It should
 *		be the internal nub if the component has two.
 */
bool init( Mercury::Nub & nub )
{
	if (g_pNub != NULL)
	{
		MF_ASSERT_DEV( !"PyNetwork already initialised" );
		return false;
	}

	g_pNub = &nub;

	return true;
}


/**
 *	This class is used to handle the cleanup tasks.
 */
class PyNetworkFiniTimeJob : public Script::FiniTimeJob
{
protected:
	virtual void fini()
	{
		FileDescriptorMap::iterator iter = g_fdReadHandlers.begin();
		while (iter != g_fdReadHandlers.end())
		{
			g_pNub->deregisterFileDescriptor( iter->first );
			++iter;
		}

		iter = g_fdWriteHandlers.begin();
		while (iter != g_fdWriteHandlers.end())
		{
			g_pNub->deregisterWriteFileDescriptor( iter->first );
			++iter;
		}
	}
};
PyNetworkFiniTimeJob g_finiTimeJob;


/**
 *	This class is used to store the callbacks that have been registered with
 *	file descriptors.
 */
class PyFileDescriptorHandler : public Mercury::InputNotificationHandler
{
public:
	explicit PyFileDescriptorHandler( PyObject * pCallback,
			PyObject * pFileOrSocket ) :
		pCallback_( pCallback ),
		pFileOrSocket_( pFileOrSocket ) {}

	int handleInputNotification( int fd )
	{
		Py_INCREF( pCallback_.get() );
		Script::call( pCallback_.get(),
				PyTuple_Pack( 1, pFileOrSocket_.get() ) );

		return 0;
	}

private:
	PyObjectPtr pCallback_;
	PyObjectPtr pFileOrSocket_;
};


// -----------------------------------------------------------------------------
// Section: Common file descriptor methods
// -----------------------------------------------------------------------------

/**
 *	This helper method extracts the file descriptor from the input object.
 */
bool extractFileDescriptor( PyObject * pFileOrSocket, int & fd )
{
	if (g_pNub == NULL)
	{
		PyErr_SetString( PyExc_SystemError, "PyNetwork not yet initialised" );
		return false;
	}

	PyObject * pFileDescriptor =
		PyObject_CallMethod( pFileOrSocket, "fileno", "" );

	// If the object does not have a fileno method, it could be an integer.
	if (pFileDescriptor == NULL)
	{
		MF_ASSERT( PyErr_Occurred() );

		PyErr_Clear();
		pFileDescriptor = pFileOrSocket;
	}

	fd = int( PyInt_AsLong( pFileDescriptor ) );

	if ((fd == -1) && PyErr_Occurred())
	{
		PyErr_SetString( PyExc_TypeError,
				"First argument is not a file descriptor" );
		return false;
	}

	return true;
}


/**
 *	This method registers a callback that will be called associated with
 *	activity on the input file descriptor.
 */
bool registerFileDescriptorCommon(
		PyObject * pFileOrSocket, PyObject * pCallback, bool isRead )
{
	int fd = -1;

	if (!extractFileDescriptor( pFileOrSocket, fd ))
	{
		return false;
	}

	if (!PyCallable_Check( pCallback ))
	{
		PyErr_SetString( PyExc_TypeError,
				"Second argument is not callable. "
					"Expected a file descriptor and a callback" );
		return false;
	}

	FileDescriptorMap & map = isRead ? g_fdReadHandlers : g_fdWriteHandlers;

	if (map.find( fd ) != map.end())
	{
		PyErr_Format( PyExc_ValueError, "Handler already registered for %d",
				fd );
		return false;
	}

	PyFileDescriptorHandler * pHandler =
		new PyFileDescriptorHandler( pCallback, pFileOrSocket );

	bool result = isRead ?
		g_pNub->registerFileDescriptor( fd, pHandler ) :
		g_pNub->registerWriteFileDescriptor( fd, pHandler );

	if (!result)
	{
		delete pHandler;
		PyErr_SetString( PyExc_TypeError, "Invalid file descriptor" );
		return false;
	}

	map[ fd ] = pHandler;
	return true;
}


/**
 *	This method deregisters the read callback associated with the input file
 *	descriptor.
 */
bool deregisterFileDescriptorCommon( PyObject * pFileOrSocket, bool isRead )
{
	int fd = -1;

	if (!extractFileDescriptor( pFileOrSocket, fd ))
	{
		return false;
	}

	FileDescriptorMap & map = isRead ? g_fdReadHandlers : g_fdWriteHandlers;

	FileDescriptorMap::iterator iter = map.find( fd );

	if (iter == map.end())
	{
		PyErr_Format( PyExc_ValueError, "No handler registered for %d", fd );
		return false;
	}

	delete iter->second;
	map.erase( iter );

	bool result = isRead ?
		g_pNub->deregisterFileDescriptor( fd ) :
		g_pNub->deregisterWriteFileDescriptor( fd );

	if (!result)
	{
		WARNING_MSG( "PyNetwork::deregisterFileDescriptorCommon: "
				"Failed to deregister file descriptor. isRead = %d\n", isRead );
	}

	return true;
}


// -----------------------------------------------------------------------------
// Section: Read file descriptor methods
// -----------------------------------------------------------------------------

/*~ function BigWorld.registerFileDescriptor
 *	@components{ base, cell }
 *
 *	This function registers a callback that will be called whenever there is
 *	data for reading on the input file descriptor.
 *
 *	@param fileDescriptor An object that has a fileno method such as a socket
 *	or file, or an integer representing a file descriptor.
 *	@param callback A callable object that expects the fileDescriptor object as
 *		its only argument.
 */
/**
 *	This method registers a callback that will be called whenever there is data
 *	for reading on the input file descriptor.
 *
 *	@param pFileOrSocket The file descriptor.
 *	@param pCallback	The callback that will be called when there is data to be
 *		read from the file descriptor. The callback will be called with the
 *		file descriptor object passed in.
 */
bool registerFileDescriptor( PyObject * pFileOrSocket, PyObject * pCallback )
{
	return registerFileDescriptorCommon( pFileOrSocket, pCallback, true );
}
PY_AUTO_MODULE_FUNCTION( RETOK, registerFileDescriptor,
		ARG( PyObject *, ARG( PyObject *, END ) ), BigWorld )


/*~ function BigWorld.deregisterFileDescriptor
 *	@components{ base, cell }
 *
 *	This function deregisters a callback that has been registered with
 *	BigWorld.registerFileDescriptor.
 *
 *	@param fileDescriptor An object that has a fileno method such as a socket
 *	or file, or an integer representing a file descriptor.
 */
/**
 *	This method deregisters the read callback associated with the input file
 *	descriptor.
 *
 *	@param pFileOrSocket The file descriptor to deregister.
 */
bool deregisterFileDescriptor( PyObject * pFileOrSocket )
{
	return deregisterFileDescriptorCommon( pFileOrSocket, true );
}
PY_AUTO_MODULE_FUNCTION( RETOK,
	deregisterFileDescriptor, ARG( PyObject *, END ), BigWorld )


// -----------------------------------------------------------------------------
// Section: Write file descriptor methods
// -----------------------------------------------------------------------------

/*~ function BigWorld.registerWriteFileDescriptor
 *	@components{ base, cell }
 *
 *	This function registers a callback that will be called when the input file
 *	descriptor is available for writing.
 *
 *	@param fileDescriptor An object that has a fileno method such as a socket
 *	or file, or an integer representing a file descriptor.
 *	@param callback A callable object that expects the fileDescriptor object as
 *		its only argument.
 */
/**
 *	This method registers a callback that will be called when the input file
 *	descriptor becomes available for writing.
 *
 *	@param pFileOrSocket	The file descriptor
 *	@param pCallback	The callback that will be called when the file descriptor is
 *		available for writing. The callback should take no arguments.
 */
bool registerWriteFileDescriptor(
		PyObject * pFileOrSocket, PyObject * pCallback )
{
	return registerFileDescriptorCommon( pFileOrSocket, pCallback, false );
}
PY_AUTO_MODULE_FUNCTION( RETOK, registerWriteFileDescriptor,
		ARG( PyObject *, ARG( PyObject *, END ) ), BigWorld )


/*~ function BigWorld.deregisterWriteFileDescriptor
 *	@components{ base, cell }
 *
 *	This function deregisters a callback that has been registered with
 *	BigWorld.registerWriteFileDescriptor.
 *
 *	@param fileDescriptor An object that has a fileno method such as a socket
 *	or file, or an integer representing a file descriptor.
 */
/**
 *	This method deregisters the read callback associated with the input file
 *	descriptor.
 *
 *	@param pFileOrSocket The file descriptor to deregister.
 */
bool deregisterWriteFileDescriptor( PyObject * pFileOrSocket )
{
	return deregisterFileDescriptorCommon( pFileOrSocket, false );
}
PY_AUTO_MODULE_FUNCTION( RETOK,
	deregisterWriteFileDescriptor, ARG( PyObject *, END ), BigWorld )


// -----------------------------------------------------------------------------
// Section: Misc
// -----------------------------------------------------------------------------

/*~ function BigWorld.address
 *	@components{ base, cell }
 *
 *	This method returns the address of the internal nub.
 */
/**
 *	This function returns a tuple containing the address of the nub.
 */
PyObject * address()
{
	if (g_pNub == NULL)
	{
		PyErr_SetString( PyExc_SystemError, "PyNetwork not yet initialised" );
		return NULL;
	}

	const Mercury::Address & addr = g_pNub->address();

	return Py_BuildValue( "si", addr.ipAsString(), ntohs( addr.port ) );
}
PY_AUTO_MODULE_FUNCTION( RETOWN, address, END, BigWorld )

} // namespace PyNetwork


// py_network.cpp

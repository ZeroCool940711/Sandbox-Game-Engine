/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef BWVOIP_HPP
#define BWVOIP_HPP

#include "pyscript/pyobject_plus.hpp"

#include "voip.hpp"

class PyVOIPResponse;


/**
 *	This class forms a python wrapper to the Voice over IP (VoIP) interface.
 */
class PyVOIP : public PyObjectPlus
{
	Py_Header( PyVOIP, PyObjectPlus )

public:
	PyVOIP( PyTypePlus* pType = &PyVOIP::s_type_ );
	~PyVOIP();

	PyObject * pyGetAttribute( const char * attr );

	PY_METHOD_DECLARE( py_initialise );
	PY_METHOD_DECLARE( py_finalise );

	PY_METHOD_DECLARE( py_command );
	PY_METHOD_DECLARE( py_getStatistics );

	PY_METHOD_DECLARE( py_setHandler );

	PY_METHOD_DECLARE( py_login );
	PY_METHOD_DECLARE( py_logout );

	PY_METHOD_DECLARE( py_createChannel );
	PY_METHOD_DECLARE( py_joinChannel );
	PY_METHOD_DECLARE( py_leaveChannel );
	PY_METHOD_DECLARE( py_queryChannels );

	PY_METHOD_DECLARE( py_inviteUser );
	PY_METHOD_DECLARE( py_kickUser );

	PY_METHOD_DECLARE( py_enableActiveChannel );
	PY_METHOD_DECLARE( py_disableActiveChannel );

	PY_METHOD_DECLARE( py_enablePositional );
	PY_METHOD_DECLARE( py_disablePositional );

	PY_METHOD_DECLARE( py_enableMicrophone );
	PY_METHOD_DECLARE( py_disableMicrophone );

	PY_METHOD_DECLARE( py_setMasterVolume );
	PY_METHOD_DECLARE( py_setChannelVolume );
	PY_METHOD_DECLARE( py_setMicrophoneVolume );

	void tick( float x, float y, float z, float yaw, float pitch, float roll, int spaceID );

private:
	PyVOIPResponse * response_;
};


/**
 *	This class is an implementation of the of the VOIPResponse 
 *	that passes the callback into python.
 */
class PyVOIPResponse : public VOIPResponse
{
public:
	PyVOIPResponse( VOIPClient * voip, PyObject * callback );
	virtual ~PyVOIPResponse();

	virtual void response( int message, const VOIPDataDict * data );

private:
	PyObject * callback_;
};


#endif // BWVOIP_HPP

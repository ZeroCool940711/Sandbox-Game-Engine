/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef PY_SOUND_PARAMETER_HPP
#define PY_SOUND_PARAMETER_HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pysound.hpp"

#if FMOD_SUPPORT
# include <fmod_event.hpp>
#endif

/**
 *  A PySoundParameter is a wrapper for an FMOD::EventParameter, and provides an
 *  interface to alter sound parameters that have been defined for a particular
 *  sound event using the FMOD designer tool.
 */
class PySoundParameter : public PyObjectPlus
{
	Py_Header( PySoundParameter, PyObjectPlus )

public:
	PySoundParameter( SoundManager::EventParameter * pParam, PySound * pSound,
		PyTypePlus * pType = &PySoundParameter::s_type_ );

	// PyObjectPlus overrides
	PyObject*			pyGetAttribute( const char * attr );
	int					pySetAttribute( const char * attr, PyObject * value );

	//  Attributes
	PY_RO_ATTRIBUTE_DECLARE( minimum_, min );
	PY_RO_ATTRIBUTE_DECLARE( maximum_, max );

	float value();
	void value( float newValue );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, value, value );

	float velocity();
	void velocity( float newVelocity );
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, velocity, velocity );

	const char *name() const;
	PY_RO_ATTRIBUTE_DECLARE( name(), name );

	/// The acceptable states for an event when calling refresh() on it from a
	/// PySoundParameter operation.
	static const SoundManager::EventState REFRESH_MASK =
		FMOD_EVENT_STATE_READY |
		FMOD_EVENT_STATE_PLAYING;

protected:
	bool refresh();

	float minimum_;
	float maximum_;

	SoundManager::EventParameter * pParam_;

	// The PySound this PySoundParameter is associated with
	PySound * pSound_;

	// The Event* that this PySoundParameter is associated with
	SoundManager::Event * pEvent_;

	// The index of this parameter in it's enclosing FMOD::Event*
	int index_;

private:
	~PySoundParameter();

	PySoundParameter(const PySoundParameter&);
	PySoundParameter& operator=(const PySoundParameter&);
};

typedef SmartPointer< PySoundParameter > PySoundParameterPtr;

#endif // PY_SOUND_PARAMETER_HPP

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

#include "options.hpp"

#include "cstdmf/debug.hpp"

#ifndef CODE_INLINE
#include "options.ipp"
#endif

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script_math.hpp"


DECLARE_DEBUG_COMPONENT2( "App", 0 );

// -----------------------------------------------------------------------------
// Section: Statics and globals
// -----------------------------------------------------------------------------

Options Options::s_instance_;

// -----------------------------------------------------------------------------
// Section: Options
// -----------------------------------------------------------------------------

/**
 *	Constructor.
 */
Options::Options():
	rootDirty_(false),
	optionsExisted_(true)
{
}


/**
 *	This method initialises this object from command line options.
 */
bool Options::init( int argc, char * argv[], bool useUserTag /* = true*/ )
{
	// Include the current directory in the default path, incase the current
	// dir changes before save is called; we want to still save to the same
	// file we loaded from
	char buffer[1024];
	::GetCurrentDirectory( sizeof( buffer ), buffer );
	strcat( buffer, "\\options.xml" );

	char * optionsFilename = buffer;

	for (int i = 0; i < argc - 1; i++)
	{
		if (strcmp( "--options", argv[i] ) == 0)
		{
			optionsFilename = argv[ i+1 ];
		}
	}

	return init( optionsFilename, useUserTag );
}


bool Options::init( const std::string& optionsFilenameString, bool useUserTag /* = true*/ )
{
	const char* optionsFilename = optionsFilenameString.c_str();
	INFO_MSG( "Options file is %s\n", optionsFilename );

	if (!s_instance_.options_)
		s_instance_.options_ = new DataResource();
	if (s_instance_.options_->load( optionsFilename ) != DataHandle::DHE_NoError)
	{
		CRITICAL_MSG( "Failed to load \"%s\". Check paths.xml\n",
			optionsFilename );

		return false;
	}

	s_instance_.pRootSection_ = s_instance_.options_->getRootSection();
	MF_ASSERT( s_instance_.pRootSection_ );

	if (s_instance_.pRootSection_ == static_cast<DataSection *>( NULL ) ||
		s_instance_.pRootSection_->countChildren() == 0)
	{
		WARNING_MSG( "Options::init: "
			"Options file is empty or nonexistent.\n" );
		s_instance_.optionsExisted_ = false;
	}
	else if (useUserTag)
	{
		//create the user name
		char buf[256];
		int num;
		num = ::GetEnvironmentVariable( "USERNAME", buf, 256 );
		if ( num >= 2 )
		{
			char userTag[3];
			userTag[0] = buf[0];
			userTag[1] = buf[num-1];
			userTag[2] = 0;
			strlwr( userTag );
			s_instance_.pRootSection_->writeString( "userTag", userTag );
		}
	}

	return true;
}


/**
 *	This static method saves the current options.
 */
bool Options::save( const char * path )
{
	if (!s_instance_.options_)
		s_instance_.options_ = new DataResource();
	s_instance_.syncRoot();
	return s_instance_.options_->save( path ? path : "" ) == DataHandle::DHE_NoError;
}


/**
 *	This static method returns whether the options file existed.
 */
bool Options::optionsFileExisted()
{
	return s_instance_.optionsExisted_;
}


/**
 *	This static method cleans up the options.
 */
void Options::fini()
{
	s_instance_.cache_.clear();
	s_instance_.options_ = NULL;
	s_instance_.pRootSection_ = NULL;
}


void Options::syncRoot()
{
	if (rootDirty_)
	{
		for 
		(
			OptionsCache::iterator it = cache_.begin();
			it != cache_.end();
			++it
		)
		{
			pRootSection_->writeString(it->first, it->second);
		}
		rootDirty_ = false;
	}
}

bool Options::optionExists( const std::string& name )
{
	if (s_instance_.cache_.find(name) != s_instance_.cache_.end())
		return true;
	else
		return s_instance_.pRootSection_->openSection( name );
}

void Options::setOptionString( const std::string& name, const std::string& value )
{
	s_instance_.rootDirty_ = true;
	s_instance_.cache_[ name ] = value;
}

PY_MODULE_STATIC_METHOD( Options, setOptionString, WorldEditor )

/*~ function WorldEditor.setOptionString
 *	@components{ tools }
 *
 *	This function sets the string option in the options.xml file.
 *
 *	@param name The name of the option to set.
 *	@param value The value to set in the options.xml file.
 */
PyObject * Options::py_setOptionString( PyObject * args )
{
	char * name = NULL;
	char * value = NULL;
	if (!PyArg_ParseTuple( args, "ss:Options.setOptionString", &name, &value ))
	{
		return NULL;
	}

	setOptionString( name, value );
	Py_Return;
}

std::string Options::getOptionString( const std::string& name )
{
	OptionsCache::iterator it = s_instance_.cache_.find( name );
	if( it == s_instance_.cache_.end() )
	{
		it = s_instance_.cache_.insert(
			OptionsCache::value_type( name, s_instance_.pRootSection_->readString( name ) ) ).first;
	}

	return (*it).second;
}

std::string Options::getOptionString( const std::string& name, const std::string& defaultVal )
{
	OptionsCache::iterator it = s_instance_.cache_.find( name );
	if( it == s_instance_.cache_.end() )
	{
		it = s_instance_.cache_.insert(
			OptionsCache::value_type( name, s_instance_.pRootSection_->readString( name, defaultVal ) ) ).first;
	}

	return (*it).second;
}

PY_MODULE_STATIC_METHOD( Options, getOptionString, WorldEditor )

/*~ function WorldEditor.getOptionString
 *	@components{ tools }
 *
 *	This function gets the string option in the options.xml file.
 *	If none can be found then the defaultVal is returned.
 *
 *	@param name			The name of the option to get.
 *	@param defaultVal	The defaultVal to return in case no value was found
 *						in the options.xml file.
 *
 *	@return Returns the value from the options.xml that corresponds to the option name.
 */
PyObject * Options::py_getOptionString( PyObject * args )
{
	char * name = NULL;
	char * defaultVal = NULL;

	if( PyArg_ParseTuple( args, "ss", &name, &defaultVal ) )
	{
		return Py_BuildValue( "s", getOptionString( name, defaultVal ).c_str() );
	}
	else if( PyArg_ParseTuple( args, "s", &name ) )
	{
		PyErr_Clear();
		return Py_BuildValue( "s", getOptionString( name ).c_str() );
	}

	PyErr_Clear();
	return Py_BuildValue( "s", "" );
}

void Options::setOptionInt( const std::string& name, int value )
{
	char buffer[64];
	bw_snprintf(buffer, sizeof(buffer), "%d", value);
	s_instance_.cache_[ name ] = buffer;
	s_instance_.rootDirty_ = true;
}

PY_MODULE_STATIC_METHOD( Options, setOptionInt, WorldEditor )

/*~ function WorldEditor.setOptionInt
 *	@components{ tools }
 *
 *	This function sets the int option in the options.xml file.
 *
 *	@param name The name of the option to set.
 *	@param value The value to set in the options.xml file.
 */
PyObject * Options::py_setOptionInt( PyObject * args )
{
	char * name = NULL;
	int value;
	if (!PyArg_ParseTuple( args, "si:Options.setOptionInt", &name, &value ))
	{
		return NULL;
	}

	setOptionInt( name, value );
	Py_Return;
}

int Options::getOptionInt( const std::string& name )
{
	OptionsCache::iterator it = s_instance_.cache_.find( name );
	if( it == s_instance_.cache_.end() )
	{
		it = s_instance_.cache_.insert(
			OptionsCache::value_type( name, s_instance_.pRootSection_->readString( name ) ) ).first;
	}

	return (*it).second.empty() ? 0 : atoi( (*it).second.c_str() );
}

int Options::getOptionInt( const std::string& name, int defaultVal )
{
	OptionsCache::iterator it = s_instance_.cache_.find( name );
	if( it == s_instance_.cache_.end() )
	{
		char formattedString[512];
		bw_snprintf( formattedString, sizeof(formattedString), "%d", defaultVal );
		it = s_instance_.cache_.insert(
			OptionsCache::value_type( name, s_instance_.pRootSection_->readString( name, formattedString ) ) ).first;
	}

	return (*it).second.empty() ? defaultVal : atoi( (*it).second.c_str() );
}

PY_MODULE_STATIC_METHOD( Options, getOptionInt, WorldEditor )

/*~ function WorldEditor.getOptionInt
 *	@components{ tools }
 *
 *	This function gets the int option in the options.xml file.
 *	If none can be found then the defaultVal is returned.
 *
 *	@param name			The name of the option to get.
 *	@param defaultVal	The defaultVal to return in case no value was found
 *						in the options.xml file.
 *
 *	@return Returns the value from the options.xml that corresponds to the option name.
 */
PyObject * Options::py_getOptionInt( PyObject * args )
{
	char * name = NULL;
	int defaultVal = 0;

	if( PyArg_ParseTuple( args, "si", &name, &defaultVal ) )
	{
		return Py_BuildValue( "i", getOptionInt( name, defaultVal ) );
	}
	else if( PyArg_ParseTuple( args, "s", &name ) )
	{
		PyErr_Clear();
		return Py_BuildValue( "i", getOptionInt( name ) );
	}

	PyErr_Clear();
	return Py_BuildValue( "i", 0 );
}

void Options::setOptionBool( const std::string& name, bool value )
{
	s_instance_.cache_[ name ] = value ? "true" : "false";
	s_instance_.rootDirty_ = true;
}

bool Options::getOptionBool( const std::string& name )
{
	OptionsCache::iterator it = s_instance_.cache_.find( name );
	if( it == s_instance_.cache_.end() )
	{
		it = s_instance_.cache_.insert(
			OptionsCache::value_type( name, s_instance_.pRootSection_->readString( name ) ) ).first;
	}

	return stricmp( (*it).second.c_str(), "true" ) == 0 ? true : false;
}

bool Options::getOptionBool( const std::string& name, bool defaultVal )
{
	OptionsCache::iterator it = s_instance_.cache_.find( name );
	if( it == s_instance_.cache_.end() )
	{
		char formattedString[512];
		bw_snprintf( formattedString, sizeof(formattedString), "%s", defaultVal ? "true" : "false" );
		it = s_instance_.cache_.insert(
			OptionsCache::value_type( name, s_instance_.pRootSection_->readString( name, formattedString ) ) ).first;
	}

	if( stricmp( (*it).second.c_str(), "true" ) == 0 )
		return true;
	if( stricmp( (*it).second.c_str(), "false" ) == 0 )
		return false;
	return defaultVal;
}

void Options::setOptionFloat( const std::string& name, float value )
{
	char buffer[64];
	bw_snprintf(buffer, sizeof(buffer), "%f", value);
	s_instance_.cache_[ name ] = buffer;
	s_instance_.rootDirty_ = true;
}

PY_MODULE_STATIC_METHOD( Options, setOptionFloat, WorldEditor )

/*~ function WorldEditor.setOptionFloat
 *	@components{ tools }
 *
 *	This function sets the float option in the options.xml file.
 *
 *	@param name The name of the option to set.
 *	@param value The value to set in the options.xml file.
 */
PyObject * Options::py_setOptionFloat( PyObject * args )
{
	char * name = NULL;
	float value;
	if (!PyArg_ParseTuple( args, "sf:Options.setOptionFloat", &name, &value ))
	{
		return NULL;
	}

	setOptionFloat( name, value );
	Py_Return;
}

float Options::getOptionFloat( const std::string& name )
{
	OptionsCache::iterator it = s_instance_.cache_.find( name );
	if( it == s_instance_.cache_.end() )
	{
		it = s_instance_.cache_.insert(
			OptionsCache::value_type( name, s_instance_.pRootSection_->readString( name ) ) ).first;
	}

	return (float)atof( (*it).second.c_str() );
}

float Options::getOptionFloat( const std::string& name, float defaultVal )
{
	OptionsCache::iterator it = s_instance_.cache_.find( name );
	if( it == s_instance_.cache_.end() )
	{
		char formattedString[512];
		bw_snprintf( formattedString, sizeof(formattedString), "%f", defaultVal );
		it = s_instance_.cache_.insert(
			OptionsCache::value_type( name, s_instance_.pRootSection_->readString( name, formattedString ) ) ).first;
	}

	return (*it).second.empty() ? defaultVal : (float)atof( (*it).second.c_str() );
}

PY_MODULE_STATIC_METHOD( Options, getOptionFloat, WorldEditor )

/*~ function WorldEditor.getOptionFloat
 *	@components{ tools }
 *
 *	This function gets the float option in the options.xml file.
 *	If none can be found then the defaultVal is returned.
 *
 *	@param name			The name of the option to get.
 *	@param defaultVal	The defaultVal to return in case no value was found
 *						in the options.xml file.
 *
 *	@return Returns the value from the options.xml that corresponds to the option name.
 */
PyObject * Options::py_getOptionFloat( PyObject * args )
{
	char * name = NULL;
	float defaultVal;

	if( PyArg_ParseTuple( args, "sf", &name, &defaultVal ) )
	{
		return Py_BuildValue( "f", getOptionFloat( name, defaultVal ) );
	}
	else if( PyArg_ParseTuple( args, "s", &name ) )
	{
		PyErr_Clear();
		return Py_BuildValue( "f", getOptionFloat( name ) );
	}

	PyErr_Clear();
	return Py_BuildValue( "f", 0 );
}

void Options::setOptionVector2( const std::string& name, const Vector2& value )
{
	char buffer[128];
	bw_snprintf(buffer, sizeof(buffer), "%f %f", value.x, value.y);
	s_instance_.cache_[ name ] = buffer;
	s_instance_.rootDirty_ = true;
}

PY_MODULE_STATIC_METHOD( Options, setOptionVector2, WorldEditor )

/*~ function WorldEditor.setOptionVector2
 *	@components{ tools }
 *
 *	This function sets the Vector2 option in the options.xml file.
 *
 *	@param name The name of the option to set.
 *	@param value The value to set in the options.xml file.
 */
PyObject * Options::py_setOptionVector2( PyObject * args )
{
	char * name = NULL;
	PyObject * pValueObject;
	Vector2 value;
	if (!PyArg_ParseTuple( args, "sO:Options.setOptionVector2",
				&name, &pValueObject ) || 
			Script::setData( pValueObject, value ) != 0)
	{
		return NULL;
	}

	setOptionVector2( name, value );
	Py_Return;
}

Vector2 Options::getOptionVector2( const std::string& name )
{
	OptionsCache::iterator it = s_instance_.cache_.find( name );
	if( it == s_instance_.cache_.end() )
	{
		it = s_instance_.cache_.insert(
			OptionsCache::value_type( name, s_instance_.pRootSection_->readString( name ) ) ).first;
	}

	Vector2 v;
	if( sscanf( (*it).second.c_str(), "%f%f", &v.x, &v.y ) == 2 )
		return v;
	return Vector2( 0.f, 0.f );
}

Vector2 Options::getOptionVector2( const std::string& name, const Vector2& defaultVal )
{
	OptionsCache::iterator it = s_instance_.cache_.find( name );
	if( it == s_instance_.cache_.end() )
	{
		char formattedString[512];
		bw_snprintf( formattedString, sizeof(formattedString), "%f %f", defaultVal[ 0 ], defaultVal[ 1 ] );
		it = s_instance_.cache_.insert(
			OptionsCache::value_type( name, s_instance_.pRootSection_->readString( name, formattedString ) ) ).first;
	}

	Vector2 v;
	if( sscanf( (*it).second.c_str(), "%f%f", &v.x, &v.y ) == 2 )
		return v;
	return defaultVal;
}

PY_MODULE_STATIC_METHOD( Options, getOptionVector2, WorldEditor )

/*~ function WorldEditor.getOptionVector2
 *	@components{ tools }
 *
 *	This function gets the Vector2 option in the options.xml file.
 *	If none can be found then the defaultVal is returned.
 *
 *	@param name			The name of the option to get.
 *
 *	@return Returns the value from the options.xml that corresponds to the option name, or (0,0) otherwise.
 */
PyObject * Options::py_getOptionVector2( PyObject * args )
{
	const char * name = NULL;
	Vector2 value( 0.f, 0.f );
	if (!PyArg_ParseTuple( args, "s:Options.getOptionVector2", &name ))
	{
		return NULL;
	}

	value = getOptionVector2( name );
	PyObject * result = PyTuple_New( 2 );
	PyTuple_SetItem( result, 0, Script::getData( value[0] ) );
	PyTuple_SetItem( result, 1, Script::getData( value[1] ) );
	return result;
}

void Options::setOptionVector3( const std::string& name, const Vector3& value )
{
	char buffer[128];
	bw_snprintf(buffer, sizeof(buffer), "%f %f %f", value.x, value.y, value.z);
	s_instance_.cache_[ name ] = buffer;
	s_instance_.rootDirty_ = true;
}

PY_MODULE_STATIC_METHOD( Options, setOptionVector3, WorldEditor )

/*~ function WorldEditor.setOptionVector3
 *	@components{ tools }
 *
 *	This function sets the Vector3 option in the options.xml file.
 *
 *	@param name The name of the option to set.
 *	@param value The value to set in the options.xml file.
 */
PyObject * Options::py_setOptionVector3( PyObject * args )
{
	char * name = NULL;
	PyObject * pValueObject;
	Vector3 value;
	if (!PyArg_ParseTuple( args, "sO:Options.setOptionVector3",
				&name, &pValueObject ) ||
			Script::setData( pValueObject, value ) != 0)
	{
		return NULL;
	}

	setOptionVector3( name, value );
	Py_Return;
}

Vector3 Options::getOptionVector3( const std::string& name )
{
	OptionsCache::iterator it = s_instance_.cache_.find( name );
	if( it == s_instance_.cache_.end() )
	{
		it = s_instance_.cache_.insert(
			OptionsCache::value_type( name, s_instance_.pRootSection_->readString( name ) ) ).first;
	}

	Vector3 v;
	if( sscanf( (*it).second.c_str(), "%f%f%f", &v.x, &v.y, &v.z ) == 3 )
		return v;
	return Vector3( 0.f, 0.f, 0.f );
}

Vector3 Options::getOptionVector3( const std::string& name, const Vector3& defaultVal )
{
	OptionsCache::iterator it = s_instance_.cache_.find( name );
	if( it == s_instance_.cache_.end() )
	{
		char formattedString[512];
		bw_snprintf( formattedString, sizeof(formattedString), "%f %f %f", defaultVal[ 0 ], defaultVal[ 1 ], defaultVal[ 2 ] );
		it = s_instance_.cache_.insert(
			OptionsCache::value_type( name, s_instance_.pRootSection_->readString( name, formattedString ) ) ).first;
	}

	Vector3 v;
	if( sscanf( (*it).second.c_str(), "%f%f%f", &v.x, &v.y, &v.z ) == 3 )
		return v;
	return defaultVal;
}

PY_MODULE_STATIC_METHOD( Options, getOptionVector3, WorldEditor )

/*~ function WorldEditor.getOptionVector3
 *	@components{ tools }
 *
 *	This function gets the Vector3 option in the options.xml file.
 *	If none can be found then the defaultVal is returned.
 *
 *	@param name			The name of the option to get.
 *
 *	@return Returns the value from the options.xml that corresponds to the option name, or a zero Vector3.
 */
PyObject * Options::py_getOptionVector3( PyObject * args )
{
	char * name = NULL;
	Vector3 value( 0.f, 0.f, 0.f );
	if (!PyArg_ParseTuple( args, "s:Options.getOptionVector3", &name ) )
	{
		return NULL;
	}

	value = getOptionVector3( name );
	PyObject * result = PyTuple_New( 3 );
	PyTuple_SetItem( result, 0, Script::getData( value[0] ) );
	PyTuple_SetItem( result, 1, Script::getData( value[1] ) );
	PyTuple_SetItem( result, 2, Script::getData( value[2] ) );
	return result;
}

void Options::setOptionVector4( const std::string& name, const Vector4& value )
{
	char buffer[256];
	bw_snprintf(buffer, sizeof(buffer), "%f %f %f %f", value.x, value.y, value.z, value.w);
	s_instance_.cache_[ name ] = buffer;
	s_instance_.rootDirty_ = true;
}

PY_MODULE_STATIC_METHOD( Options, setOptionVector4, WorldEditor )

/*~ function WorldEditor.setOptionVector4
 *	@components{ tools }
 *
 *	This function sets the Vector4 option in the options.xml file.
 *
 *	@param name The name of the option to set.
 *	@param value The value to set in the options.xml file.
 */
PyObject * Options::py_setOptionVector4( PyObject * args )
{
	char * name = NULL;
	PyObject * pValueObject;
	Vector4 value;
	if (!PyArg_ParseTuple( args, "sO:Options.setOptionVector4",
				&name, &pValueObject ) ||
			Script::setData( pValueObject, value ) != 0 )
	{
		return NULL;
	}

	setOptionVector4( name, value );
	Py_Return;
}

Vector4 Options::getOptionVector4( const std::string& name )
{
	OptionsCache::iterator it = s_instance_.cache_.find( name );
	if( it == s_instance_.cache_.end() )
	{
		it = s_instance_.cache_.insert(
			OptionsCache::value_type( name, s_instance_.pRootSection_->readString( name ) ) ).first;
	}

	Vector4 v;
	if( sscanf( (*it).second.c_str(), "%f%f%f%f", &v.x, &v.y, &v.z, &v.w ) == 4 )
		return v;
	return Vector4( 0.f, 0.f, 0.f, 0.f );
}

Vector4 Options::getOptionVector4( const std::string& name, const Vector4& defaultVal )
{
	OptionsCache::iterator it = s_instance_.cache_.find( name );
	if( it == s_instance_.cache_.end() )
	{
		char formattedString[512];
		bw_snprintf( formattedString, sizeof(formattedString), "%f %f %f %f", defaultVal[ 0 ], defaultVal[ 1 ], defaultVal[ 2 ],
			defaultVal[ 3 ] );
		it = s_instance_.cache_.insert(
			OptionsCache::value_type( name, s_instance_.pRootSection_->readString( name, formattedString ) ) ).first;
	}

	Vector4 v;
	if( sscanf( (*it).second.c_str(), "%f%f%f%f", &v.x, &v.y, &v.z, &v.w ) == 4 )
		return v;
	return defaultVal;
}

PY_MODULE_STATIC_METHOD( Options, getOptionVector4, WorldEditor )

/*~ function WorldEditor.getOptionVector4
 *	@components{ tools }
 *
 *	This function gets the Vector4 option in the options.xml file.
 *	If none can be found then the defaultVal is returned.
 *
 *	@param name			The name of the option to get.
 *	@param defaultVal	The defaultVal to return in case no value was found
 *						in the options.xml file.
 *
 *	@return Returns the value from the options.xml that corresponds to the option name.
 */
PyObject * Options::py_getOptionVector4( PyObject * args )
{
	char * name = NULL;
	Vector4 value( 0.f, 0.f, 0.f, 0.f );
	if (!PyArg_ParseTuple( args, "s:Options.getOptionVector4", &name ))
	{
		return NULL;
	}

	value = getOptionVector4( name );
	PyObject * result = PyTuple_New( 4 );
	PyTuple_SetItem( result, 0, Script::getData( value[0] ) );
	PyTuple_SetItem( result, 1, Script::getData( value[1] ) );
	PyTuple_SetItem( result, 2, Script::getData( value[2] ) );
	PyTuple_SetItem( result, 3, Script::getData( value[3] ) );
	return result;
}

void Options::setOptionMatrix34( const std::string& name, const Matrix& value )
{
	char buffer[128];
	char rowName[1024];
	for ( int i = 0; i < 4; i++ )
	{
		bw_snprintf(buffer, sizeof(buffer), "%f %f %f", value[i][0], value[i][1], value[i][2] );
		bw_snprintf(rowName, sizeof(rowName), "%s/row%d", name.c_str(), i );
		s_instance_.cache_[ rowName ] = buffer;
	}
	
	s_instance_.rootDirty_ = true;
}

PY_MODULE_STATIC_METHOD( Options, setOptionMatrix34, WorldEditor )

/*~ function WorldEditor.setOptionMatrix34
 *	@components{ tools }
 *
 *	This function sets the 3x4 Matrix option in the options.xml file.
 *
 *	@param name The name of the option to set.
 *	@param value The value to set in the options.xml file.
 */
PyObject * Options::py_setOptionMatrix34( PyObject * args )
{
	char * name = NULL;
	PyObject * pValueObject;
	Matrix value;
	if (!PyArg_ParseTuple( args, "sO:Options.setOptionMatrix34", 
				&name, &pValueObject ) ||
			Script::setData( pValueObject, value ) != 0)
	{
		return NULL;
	}

	setOptionMatrix34( name, value );
	Py_Return;
}

Matrix Options::getOptionMatrix34( const std::string& name )
{
	if( s_instance_.cache_.find( name+"/row0" ) == s_instance_.cache_.end() )
	{
		s_instance_.cache_[ name+"/row0" ] = s_instance_.pRootSection_->readString( name+"/row0" );
		s_instance_.cache_[ name+"/row1" ] = s_instance_.pRootSection_->readString( name+"/row1" );
		s_instance_.cache_[ name+"/row2" ] = s_instance_.pRootSection_->readString( name+"/row2" );
		s_instance_.cache_[ name+"/row3" ] = s_instance_.pRootSection_->readString( name+"/row3" );
	}

	Matrix v;
	if( ( sscanf( s_instance_.cache_[ name+"/row0" ].c_str(), "%f%f%f", &v[0][0], &v[0][1], &v[0][2] ) == 3 ) &&
		( sscanf( s_instance_.cache_[ name+"/row1" ].c_str(), "%f%f%f", &v[1][0], &v[1][1], &v[1][2] ) == 3 ) &&
		( sscanf( s_instance_.cache_[ name+"/row2" ].c_str(), "%f%f%f", &v[2][0], &v[2][1], &v[2][2] ) == 3 ) &&
		( sscanf( s_instance_.cache_[ name+"/row3" ].c_str(), "%f%f%f", &v[3][0], &v[3][1], &v[3][2] ) == 3 ) )
		return v;
	return Matrix::identity;
}

Matrix Options::getOptionMatrix34( const std::string& name, const Matrix& defaultVal )
{
	if( s_instance_.cache_.find( name+"/row0" ) == s_instance_.cache_.end() )
	{
		char formattedString[512];
		char rowName[1024];
		for ( int i = 0; i < 4; i++ )
		{
			bw_snprintf(formattedString, sizeof(formattedString), "%f %f %f", defaultVal[i][0], defaultVal[i][1], defaultVal[i][2] );
			bw_snprintf(rowName, sizeof(rowName), "%s/row%d", name.c_str(), i );
			s_instance_.cache_[ rowName ] = s_instance_.pRootSection_->readString( rowName, formattedString );
		}
	}

	Matrix v;
	if( ( sscanf( s_instance_.cache_[ name+"/row0" ].c_str(), "%f%f%f", &v[0][0], &v[0][1], &v[0][2] ) == 3 ) &&
		( sscanf( s_instance_.cache_[ name+"/row1" ].c_str(), "%f%f%f", &v[1][0], &v[1][1], &v[1][2] ) == 3 ) &&
		( sscanf( s_instance_.cache_[ name+"/row2" ].c_str(), "%f%f%f", &v[2][0], &v[2][1], &v[2][2] ) == 3 ) &&
		( sscanf( s_instance_.cache_[ name+"/row3" ].c_str(), "%f%f%f", &v[3][0], &v[3][1], &v[3][2] ) == 3 ) )
		return v;
	return defaultVal;
}

PY_MODULE_STATIC_METHOD( Options, getOptionMatrix34, WorldEditor )

/*~ function WorldEditor.getOptionMatrix34
 *	@components{ tools }
 *
 *	This function gets the 3x4 Matrix option in the options.xml file.
 *	If none can be found then the identity matrix is returned.
 *
 *	@param 	name	The name of the option to get.
 *
 *	@return 		Returns the value from the options.xml that corresponds to
 *					the option name, or the identity matrix if no corresponding
 *					option name exists.
 */
PyObject * Options::py_getOptionMatrix34( PyObject * args )
{
	char * name = NULL;
	Matrix value = Matrix::identity;
	if (!PyArg_ParseTuple( args, "s:Options.getOptionMatrix34", &name ))
	{
		return NULL;
	}
	value = getOptionMatrix34( name );
	return Script::getData( value );
}

PY_MODULE_STATIC_METHOD( Options, setOption, WorldEditor )

/*~ function WorldEditor.setOption
 *	@components{ tools }
 *
 *	This function sets the option in the options.xml file.
 *
 *	@param name The name of the option to set.
 *	@param value The value to set in the options.xml file.
 */
PyObject * Options::py_setOption( PyObject * args )
{
	char * path;
	PyObject * pValueObject;

	if (!PyArg_ParseTuple( args, "sO:Options.setOption",
			&path, &pValueObject ))
	{
		return NULL;
	}

	if (PyInt_Check( pValueObject ))
	{
		return py_setOptionInt( args );
	}
	else if (PyFloat_Check( pValueObject ))
	{
		return py_setOptionFloat( args );
	}
	else if (PyString_Check( pValueObject ))
	{
		return py_setOptionString( args );
	}
	else if (PyTuple_Check( pValueObject ))
	{
		const int size = PyTuple_Size( pValueObject );

		if (size == 3)
		{
			return py_setOptionVector3( args );
		}
		else if (size == 4)
		{
			return py_setOptionVector4( args );
		}
	}
	else if (PyVector<Vector3>::Check( pValueObject ))
	{
		return py_setOptionVector3( args );
	}
	else if (PyVector<Vector4>::Check( pValueObject ))
	{
		return py_setOptionVector4( args );
	}

	PyErr_Format( PyExc_TypeError, "WorldEditor.setOption: "
		"Unrecognised object type %s", pValueObject->ob_type->tp_name );

	return NULL;
}

// options.cpp

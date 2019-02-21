/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

/*
 * A class that encapsulates the document into data sections. The class allows
 * manipulation and access to elements by hierarchical data sections.
 */

#include "pch.hpp"
#include "sanitise_helper.hpp"
#include "cstdmf/base64.h"
#include "cstdmf/concurrency.hpp"
#include "cstdmf/memory_counter.hpp"
#include <algorithm>

// Support for XML escape sequences, such as &amp; and &#65, has been added.
// If for some reason, you need to disable this support and fall back to the
// old parsing, just un comment the line below:
//#define NO_XML_ESCAPE_SEQUENCE 1


// Standard Library Headers.
#include <fstream>
#include <vector>
#include <algorithm>
#include <stack>
#include <sstream>

using namespace std;

// Application Specific Headers.
#include "xml_section.hpp"
#include "xml_special_chars.hpp"
#include "file_system.hpp"
#include "cstdmf/debug.hpp"

DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )

// Dodgy thing to get server compiling.
#ifndef INLINE
#define INLINE
#endif


memoryCounterDefine( xml, Entity );


template < class T >
class ScopedPointerArray : public std::vector< T* >
{
public:
	ScopedPointerArray() { }

	virtual ~ScopedPointerArray()
	{
		for ( uint i = 0; i < this->size(); i++ )
			delete this->operator[](i);
	}
};

// -----------------------------------------------------------------------------
//	Constructor(s) for XMLSection.
// -----------------------------------------------------------------------------


/**
 *	This does a quick check to see whether a tag is valid XML.  It is not
 *	exhaustive.  Rather we check to commonly allowed errors in tag names,
 *	and in particular tags beginning with numbers, tags containing spaces
 *	or slashes.
 *
 *	@param str					The tag to check.
 *	@returns					False if the tag begins with a number or
 *								contains spaces, true otherwise.
 */
bool isValidXMLTag(const char *str)
{
	// Handle degenerate cases:
	if (str == NULL || str[0]=='\0')
		return false;

	// The tag cannot begin with a number:
	// (Note that str can accidentally be binary data.  If it is then we need
	// to cast str[0] to unsigned otherwise an error in the runtime can occur).
	if (str[0]<0 || ::isdigit(str[0]) != 0)
		return false;

	// The tag cannot have spaces or slashes:
	for (size_t i = 0; str[i] != '\0'; ++i)
	{
		if (str[i] == ' ' || str[i] == '\\' || str[i] == '/')
			return false;
	}

	return true; // probably ok
}


/**
 *	Ordinary constructor for XMLSection.
 *
 *	@param tag						The tag name for this section
 *
 */
XMLSection::XMLSection( const std::string & tag ) :
	ctag_( NULL ),
	cval_( NULL ),
	tag_( tag )
{
	memoryCounterAdd( xml );
	memoryClaim( tag_ );
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
		tag_.capacity() )
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
		sizeof(*this) )
#endif
	if (!isValidXMLTag(tag_.c_str()))
		WARNING_MSG("%s is not a valid XML tag\n", tag_.c_str());
}

/**
 *	Constructor for XMLSection when we don't want to copy the tag string
 */
XMLSection::XMLSection( const char * tag, bool /*cstrToken*/ ) :
	ctag_( tag ),
	cval_( NULL )
{
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
		sizeof(*this) )
#endif
	if (ctag_ != NULL && !isValidXMLTag(ctag_))
		WARNING_MSG("%s is not a valid XML tag\n", ctag_);
}


// -----------------------------------------------------------------------------
// Methods of accessing with XML elements and sections for XMLSection.
// -----------------------------------------------------------------------------

/**	This method returns the number of sections under this  one.
 *
 *	@return Number of sections.
 */
int XMLSection::countChildren()
{
	return children_.size();
}

/**	This method opens the section with the given index.
 *
 * 	@param index			Index of the section to open
 *
 *	@return A pointer to the section.
 */
DataSectionPtr XMLSection::openChild( int index )
{
	// MF_ASSERT( (uint)index < children_.size() );
	if ((uint)index < children_.size())
	{
		return children_[index].getObject();
	}
	else
	{
		ERROR_MSG( "XMLSection::openChild: Invalid index %d. Children = %zu\n",
				index, children_.size() );

		return NULL;
	}
}

/**
 * This method returns the immediate child of this section that is associated
 * with the input tag.
 */
DataSectionPtr XMLSection::findChild( const std::string & tag,
										DataSectionCreator* creator )
{
	Children::const_iterator iter = children_.begin();

	while (iter != children_.end())
	{
		if ((*iter)->sectionName() == tag)
		{
			return (*iter).getObject();
		}

		iter++;
	}

	return (DataSection *)NULL;
}

class XMLSectionCreator : public DataSectionCreator
{
public:
	virtual DataSectionPtr create( DataSectionPtr pSection,
									const std::string& tag )
	{
		return new XMLSection( tag );
	}

	virtual DataSectionPtr load(DataSectionPtr pSection,
									const std::string& tag,
									BinaryPtr pBinary = NULL)
	{
		XMLSectionPtr xmlSection = XMLSection::createFromBinary(tag, pBinary);

		return xmlSection.getObject();
	}
};

DataSectionCreator* XMLSection::creator()
{
	static XMLSectionCreator s_creator;
	return &s_creator;
}


/**
 *	This method creates a new section under the current section.
 *	It will create a new section regardless of whether a section under the
 *	same tag exists at the current section.
 *
 *	@param tag		The name/path of the section to be added.
 *  @param creator  The DataSectionCreator to use to create a new section with.
 *
 *	@return	A pointer to the DataSection added if successful. NULL otherwise.
 */
DataSectionPtr XMLSection::newSection( const std::string &tag,
										DataSectionCreator* creator )
{
	XMLSection * pNewSect = this->addChild( NULL );
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
		pNewSect->tag_.capacity() )
#endif
	pNewSect->tag_ = tag;
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
		pNewSect->tag_.capacity() )
#endif

	memoryCounterAdd( xml );
	memoryClaim( pNewSect->tag_ );

	return pNewSect;
}

/**
 *	This method creates a new section at the index specified.
 *	It will create a new section regardless of whether a section under the
 *	same tag exists at the current section.
 *
 *	@param tag		The name/path of the section to be added.
 *  @param index	The index at which to insert the new DataSection
 *
 *	@return	A pointer to the DataSection added if successful. NULL otherwise.
 */
DataSectionPtr XMLSection::insertSection( const std::string &tag, int index /* = -1 */ )
{
	XMLSection * pNewSect = this->addChild( NULL, index );
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
		pNewSect->tag_.capacity() )
#endif
	pNewSect->tag_ = tag;
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
		pNewSect->tag_.capacity() )
#endif

	memoryCounterAdd( xml );
	memoryClaim( pNewSect->tag_ );

	return pNewSect;
}


/**
 * This method deletes the immediate child of this section that is associated
 * with the input tag.
 */
void XMLSection::delChild( const std::string & tag )
{
	Children::iterator iter = children_.begin();

	while (iter != children_.end())
	{
		if ((*iter)->sectionName() == tag)
		{
			{
				memoryCounterSub( xml );
				memoryClaim( children_ );
			}
#if ENABLE_RESOURCE_COUNTERS
			RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
				children_.capacity() * sizeof(children_[0]) )
#endif
			children_.erase(iter);
#if ENABLE_RESOURCE_COUNTERS
			RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
				children_.capacity() * sizeof(children_[0]) )
#endif
			{
				memoryCounterAdd( xml );
				memoryClaim( children_ );
			}
			return;
		}

		iter++;
	}
}

/**
 * This method deletes the immediate child of this section that is associated
 * with the input pointer.
 */
void XMLSection::delChild( DataSectionPtr pSection )
{
	Children::iterator iter = children_.begin();

	while (iter != children_.end())
	{
		if (iter->getObject() == pSection.getObject())
		{
			{
				memoryCounterSub( xml );
				memoryClaim( children_ );
			}
#if ENABLE_RESOURCE_COUNTERS
			RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
				children_.capacity() * sizeof(children_[0]) )
#endif
			children_.erase(iter);
#if ENABLE_RESOURCE_COUNTERS
			RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
				children_.capacity() * sizeof(children_[0]) )
#endif
			{
				memoryCounterAdd( xml );
				memoryClaim( children_ );
			}
			return;
		}

		iter++;
	}
}

/**
 *	This method deletes all children of this section.
 */
void XMLSection::delChildren()
{
	memoryCounterSub( xml );
	memoryClaim( children_ );
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
		children_.capacity() * sizeof(children_[0]) )
#endif
	children_.clear();
#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
		children_.capacity() * sizeof(children_[0]) )
#endif
}


/**
 * This method returns the name of this section.
 *
 * @return	The name of this section.
 */
std::string XMLSection::sectionName() const
{
	if (ctag_) return ctag_;
	return tag_;
}



/**
 *	This method sets the parent of this section.
 *
 *	Take care that when this method is used, the parent does NOT keep a
 *	reference to to its children, or else a circular reference will result.
 *
 *	@param pParent	The new parent of the section.
 */
void XMLSection::setParent( DataSectionPtr pParent )
{
	parent_ = pParent;
}

/**
 *	This method gets the size of this section's overhead,
 *	including that of all its children.
 */
uint32 XMLSection::sizeInBytes() const
{
	uint32 sz = 0;
	sz += tag_.length() + value_.length();
	sz += children_.capacity() * sizeof(children_[0]);
	for (Children::const_iterator it = children_.begin();
		it != children_.end();
		it++)
	{
		sz += (*it)->sizeInBytes();
	}
	return sz;
}

/**
 * This method returns the approximate number of bytes allocated by this
 * section. Since the XMLSection owns its children, this should be recursive.
 *
 * @return	Number of bytes allocated
 */

int XMLSection::bytes() const
{
	return std::max( 1024, block_ ? block_->len() : 0 );
}

// -----------------------------------------------------------------------------
// Methods of reading in values of XML elements for XMLSection.
// -----------------------------------------------------------------------------

/**
 *	This method reads in the value of the specified tag as a boolean value.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The boolean value read in or the default value if the read failed.
 */
bool XMLSection::asBool( bool defaultVal )
{
	// Read in the string ignoring leading and trailing whitespaces.
	std::string readValue = this->asString( "", DS_TrimWhitespace );

	if (!bw_stricmp( readValue.c_str(), "true" ))
	{
		return true;
	}
	if (!bw_stricmp( readValue.c_str(), "false" ))
	{
		return false;
	}

	WARNING_MSG( "XMLSection::asBool: Invalid bool '%s' for %s\n",
			readValue.c_str(), this->sectionName().c_str() );

	return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as an integer value.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The int value read in or the default value if the read failed.
 */
int XMLSection::asInt( int defaultVal )
{
	std::string readValue = this->asString( );

	if ( readValue != "" )
	{
		return atoi( readValue.c_str() );
	}
	else
	{
		return defaultVal;
	}
}


/**
 *	This method reads in the value of the specified tag as a long integer
 *	value. With all of the read methods, a default value can be specified if
 *	the operation fails. The operation can fail if the tag given does not
 *	match any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The long value read in or the default value if the read failed.
 */
long XMLSection::asLong( long defaultVal )
{
	std::string readValue = this->asString( );

	if ( readValue != "" )
	{
		return atol( readValue.c_str() );
	}
	else
	{
		return defaultVal;
	}
}


/**
 *	This method reads in the value of the specified tag as a floating-point
 *	value. With all of the read methods, a default value can be specified if
 *	the operation fails. The operation can fail if the tag given does not
 *	match any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The float value read in or the default value if the read failed.
 */
inline float XMLSection::asFloat( float defaultVal )
{
	return (float) asDouble( defaultVal );
}


/**
 *	This method returns the value of this section as a double.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The double value read in or the default value if the read failed.
 */
double XMLSection::asDouble( double defaultVal )
{
	std::string readValue = this->asString( );

	if ( readValue != "" )
	{
		return atof( readValue.c_str() );
	}
	else
	{
		return defaultVal;
	}
}


/**
 *	This method reads in the value of the specified tag as a string. With all
 *	of the read methods, a default value can be specified if the operation
 *	fails. The operation can fail if the tag given does not match any tags in
 *	the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *	@param flags		Any operational flags. These are bitwise ORed by the
 *						calling code. Flags accepted are:
 *						DS_TrimWhitespace and
 *						DS_IncludeWhitespace.
 *
 *	@return The string read in or the default value if the read failed.
 */
std::string XMLSection::asString( const std::string &defaultVal,
		int /*flags*/ )
{
	// #### Need to consider stripping the white space.
	if (cval_ && strlen(cval_) != 0)
		return cval_;
	else if (!value_.empty())
		return value_;
	else
		return defaultVal;
}


/**
 *	This method reads in the value of the specified tag as a wide string.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *	@param flags		Any operational flags. These are bitwise ORed by the
 *						calling code. Flags accepted are:
 *						DS_TrimWhitespace and
 *						DS_IncludeWhitespace.
 *
 *	@return The string read in or the default value if the read failed.
 */
std::wstring XMLSection::asWideString( const std::wstring & /*defaultVal*/,
		int /*flags*/ )
{
	return decodeWideString( this->asString() );
}


/**
 *	This method reads in the value of the specified tag as a 2D vector.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The Vector2 read in or the default value if the read failed.
 */
Vector2 XMLSection::asVector2( const Vector2 &defaultVal )
{
	std::string readValue = this->asString( );

	Vector2 v;

	if (sscanf( readValue.c_str(), "%f%f", &v.x, &v.y ) != 2)
	{
		ERROR_MSG( "XMLSection::asVector2: Bad format '%s' in section %s\n",
			readValue.c_str(),
			this->sectionName().c_str() );
		return defaultVal;
	}

	return v;
}


/**
 *	This method reads in the value of the specified tag as a 3D vector.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The Vector3 read in or the default value if the read failed.
 */
Vector3 XMLSection::asVector3( const Vector3 &defaultVal )
{
	std::string readValue = this->asString( );

	Vector3 v;

	if (sscanf( readValue.c_str(), "%f%f%f", &v.x, &v.y, &v.z ) != 3)
	{
		ERROR_MSG( "XMLSection::asVector3: Bad format '%s' in section %s\n",
			readValue.c_str(),
			this->sectionName().c_str() );
		return defaultVal;
	}

	return v;
}


/**
 *	This method reads in the value of the specified tag as a 4D vector.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The Vector4 read in or the default value if the read failed.
 */
Vector4 XMLSection::asVector4( const Vector4 &defaultVal )
{
	std::string readValue = this->asString( );

	Vector4 v;
	if ( sscanf( readValue.c_str(), "%f%f%f%f", &v.x, &v.y, &v.z, &v.w ) != 4 )
	{
		ERROR_MSG( "XMLSection::asVector4: Bad format '%s' in section %s\n",
			readValue.c_str(),
			this->sectionName().c_str() );
		return defaultVal;
	}
	else
	{
		return v;
	}
}


/**
 *	This method reads in the value of the specified tag as a 3x4 matrix.
 *	With all of the read methods, a default value can be specified if the
 *	operation fails. The operation can fail if the tag given does not match
 *	any tags in the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The Matrix read in or the default value if the read failed.
 */
Matrix XMLSection::asMatrix34( const Matrix &defaultVal )
{
	Matrix	ret;
	ret( 3, 3 ) = 1.f;

	ret[0] = this->readVector3( "row0", defaultVal[0] );
	ret[1] = this->readVector3( "row1", defaultVal[1] );
	ret[2] = this->readVector3( "row2", defaultVal[2] );
	ret[3] = this->readVector3( "row3", defaultVal[3] );

	return ret;
}


/**
 *	This method converts the whole XML section to a binary pointer.
 *	This is called in order to save the data in this section.
 */
BinaryPtr XMLSection::asBinary()
{
	std::stringstream stream;

	if (!this->writeToStream(stream, 0)) return NULL;

	BinaryPtr pBinary = new BinaryBlock(stream, stream.tellp(), "BinaryBlock/XMLSection");

	return pBinary;
}

/**
 *	This method reads in the value of the specified tag as a BLOB. With all
 *	of the read methods, a default value can be specified if the operation
 *	fails. The operation can fail if the tag given does not match any tags in
 *	the section.
 *
 *	@param defaultVal	The default value should an error occur.
 *
 *	@return The string read in or the default value if the read failed.
 */
std::string XMLSection::asBlob( const std::string & /*defaultVal*/ )
{
	std::string encoded = this->asString();
	// asssume the input string is in Base64 encoding.
	int encLen = encoded.length();
	char * decoded = new char[ encLen ];
	int decLen = Base64::decode( encoded, decoded, encLen );
	std::string retString( decoded, decLen );
	delete [] decoded;
	return retString;
}

// -----------------------------------------------------------------------------
// Methods of writing out values of document elements for XMLSection.
// -----------------------------------------------------------------------------

/**
 *	This method writes a boolean value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *	The format string should not be longer than 512 bytes including the null
 *	terminator and contents.
 *
 *	@param value		The boolean value that will be associated with that
 *						identifier.
 *	@param format		A format string similar to that used by printf() to
 *						help format the output.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setBool( bool value, const char *format )
{
	char formattedString[512];

	// TODO:PM Does it make sense for these to have a format string?
	bw_snprintf( formattedString, sizeof(formattedString), format, value ? "true" : "false" );
	return this->setString( formattedString );
}


/**
 *	This method writes an integer value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *	The format string should not be longer than 512 bytes including the null
 *	terminator and contents.
 *
 *	@param value		The integer value that will be associated with that
 *						identifier.
 *	@param format		A format string similar to that used by printf() to
 *						help format the output.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setInt( int value, const char *format )
{
	char formattedString[512];

	bw_snprintf( formattedString, sizeof(formattedString), format, value );
	return this->setString( formattedString );
}


/**
 *	This method writes a long value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *	The format string should not be longer than 512 bytes including the null
 *	terminator and contents.
 *
 *	@param value		The long value that will be associated with that
 *						identifier.
 *	@param format		A format string similar to that used by printf() to
 *						help format the output.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setLong( long value, const char *format )
{
	char formattedString[512];

	bw_snprintf( formattedString, sizeof(formattedString), format, value );
	return this->setString( formattedString );
}


/**
 *	This method writes a floating-point value to the tag specified. It uses
 *	the write method for the string to avoid code duplication.
 *	The format string should not be longer than 512 bytes including the null
 *	terminator and contents.
 *
 *	@param value		The floating-point value that will be associated
 *						with that identifier.
 *	@param format		A format string similar to that used by printf() to
 *						help format the output.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setFloat( float value, const char *format )
{
	char formattedString[512];

	bw_snprintf( formattedString, sizeof(formattedString), format, value );
	return this->setString( formattedString );
}


/**
 *	This method writes a double floating-point value to the tag specified.
 *	It uses the write method for the string to avoid code duplication.
 *	The format string should not be longer than 512 bytes including the null
 *	terminator and contents.
 *
 *	@param value		The double floating-point value that will be
 *						associated with that identifier.
 *	@param format		A format string similar to that used by printf() to
 *						help format the output.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setDouble( double value, const char *format )
{
	char formattedString[512];

	bw_snprintf( formattedString, sizeof(formattedString), format, value );
	return this->setString( formattedString );
}


/**
 *	This method writes a string value to the tag specified. The
 *	write(Wide)String methods are the only methods with no format string in
 *	the parameter list.
 *	This means the that string will have to be formatted before calling this
 *	method.
 *
 *	@param value		The string value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setString( const std::string &value )
{
	cval_ = NULL;
	{
		memoryCounterSub( xml );
		memoryClaim( value_ );

#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
			value_.capacity() )
#endif
	}
	value_ = value;
	{
#if ENABLE_RESOURCE_COUNTERS
		// Track memory usage
		RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
			value_.capacity() )
#endif
		memoryCounterAdd( xml );
		memoryClaim( value_ );
	}
	return true;
}


/**
 *	This method writes a wide string value to the tag specified. The
 *	write(Wide)String methods are the only methods with no format string in the
 *	parameter list.
 *	This means the that string will have to be formatted before calling this
 *	method.
 *
 *	@param value		The string value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setWideString( const std::wstring &value )
{
	this->setString( encodeWideString(value) );
	return true;
}


/**
 *	This method writes a Vector2 value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *	The format string should not be longer than 512 bytes including the null
 *	terminator and contents.
 *
 *	@param value		The Vector2 value that will be associated with that
 *						identifier.
 *	@param format		A format string similar to that used by printf() to
 *						help format the output.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setVector2( const Vector2 &value, const char *format )
{
	char formattedString[512];

	bw_snprintf( formattedString, sizeof(formattedString), format, value.x, value.y );
	return this->setString( formattedString );
}


/**
 *	This method writes a Vector3 value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *	The format string should not be longer than 512 bytes including the null
 *	terminator and contents.
 *
 *	@param value		The Vector3 value that will be associated with that
 *						identifier.
 *	@param format		A format string similar to that used by printf() to
 *						help format the output.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setVector3( const Vector3 &value, const char *format )
{
	char formattedString[512];

	bw_snprintf( formattedString, sizeof(formattedString), format, value.x, value.y, value.z );
	return this->setString( formattedString );
}


/**
 *	This method writes a Vector4 value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *	The format string should not be longer than 512 bytes including the null
 *	terminator and contents.
 *
 *	@param value		The Vector4 value that will be associated with that
 *						identifier.
 *	@param format		A format string similar to that used by printf() to
 *						help format the output.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setVector4( const Vector4 &value, const char *format )
{
	char formattedString[512];

	bw_snprintf( formattedString, sizeof(formattedString), format, value[0], value[1], value[2], value[3] );
	return this->setString( formattedString );
}


/**
 *	This method writes a Matrix34 value to the tag specified. It uses the
 *	write method for the string to avoid code duplication.
 *	The format string should not be longer than 2048 bytes including the null
 *	terminator and contents.
 *
 *	@param value		The Vector3 value that will be associated with that
 *						identifier.
 *	@param vectorFormat	A format string similar to that used by printf() to
 *						help format the output.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setMatrix34( const Matrix &value, const char *vectorFormat )
{
	bool good = true;

	good &= this->writeVector3( "row0", value[0], vectorFormat );
	good &= this->writeVector3( "row1", value[1], vectorFormat );
	good &= this->writeVector3( "row2", value[2], vectorFormat );
	good &= this->writeVector3( "row3", value[3], vectorFormat );

	return good;
}


/**
 *	This method writes a BLOB value to the tag specified. The
 *	This means the that string will have to be formatted before calling this
 *	method.
 *
 *	@param value		The string value that will be associated with that
 *						identifier.
 *
 *	@return	True if it was successful, False otherwise.
 */
bool XMLSection::setBlob( const std::string &value )
{
	std::string encoded = Base64::encode( value.data(), value.length() );
	return this->setString( encoded );
}

/**
 * The destructor does not do much as it cannot delete any of the pointers in
 * the data. All other objects have their own destructors.
 */
XMLSection::~XMLSection()
{
	memoryCounterSub( xml );

	memoryClaim( children_ );

	memoryClaim( tag_ );
	memoryClaim( value_ );

#if ENABLE_RESOURCE_COUNTERS
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
		tag_.capacity() )
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
		value_.capacity() )
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
		sizeof(*this) )
	RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
		children_.capacity() * sizeof(children_[0]) )
#endif
}


// -----------------------------------------------------------------------------
// Private Helper Methods.
// -----------------------------------------------------------------------------

/**
 *	This method strips the whitespace from a string. It works by taking the
 *	input string (null-terminated) and marking off the null-terminator before
 *	any trailing whitespace characters. It then returns a pointer to the first
 *	non-whitespace character in the string.
 *
 *	@param inputString A pointer to a null-terminated ASCII string.
 *
 *	@return The string with trailing and leading whitespace removed.
 */
char *XMLSection::stripWhitespace( char *inputString )
{
	// Find the end of the string.
	char *stringPos = inputString;
	while ( *stringPos != '\0' )
	{
		stringPos++;
	}

	// Find and mark the start of trailing whitespace.
	while ( isspace(*( stringPos - 1 ) ) &&
		( stringPos != inputString ) )
	{
		stringPos--;
	}
	*stringPos = '\0';

	// Find end of leading whitespace and return that as the new start of
	// string.
	stringPos = inputString;
	while ( isspace( *stringPos ) && ( *stringPos != '\0' ) )
	{
		stringPos++;
	}
	return stringPos;
}



/**
 * This method is used to add a child to this section.
 */
XMLSection * XMLSection::addChild( const char * tag, int index /* = -1 */ )
{
	XMLSection * pNewSection = new XMLSection( tag, true );

	{
		memoryCounterSub( xml );
		memoryClaim( children_ );
	}

	if (index == -1)
	{
#if ENABLE_RESOURCE_COUNTERS
		RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
			children_.capacity() * sizeof(children_[0]) )
#endif
		children_.push_back( pNewSection );
#if ENABLE_RESOURCE_COUNTERS
		RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
			children_.capacity() * sizeof(children_[0]) )
#endif
	}
	else
	{
		if (index > (int)children_.size()) index = children_.size(); // Just in case
#if ENABLE_RESOURCE_COUNTERS
		RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
			children_.capacity() * sizeof(children_[0]) )
#endif
		children_.insert( children_.begin() + index, pNewSection );
#if ENABLE_RESOURCE_COUNTERS
		RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
			children_.capacity() * sizeof(children_[0]) )
#endif
	}

	{
		memoryCounterAdd( xml );
		memoryClaim( children_ );
	}

	return pNewSection;
}


inline bool isWhiteSpace( char c )
{
	return (c == ' ' || c == '\t' || c == '\r' || c == '\n');
}

template <class C>
inline void skipWhiteSpace( C & stream )
{
    while (stream && isWhiteSpace( stream.peek() ))
    {
        stream.get();
    }
}

// TODO:PM Should get rid of these constants.
const int MAX_TAG_LENGTH = 1024;
const int MAX_VALUE_LENGTH = 1024;


/**
 * This static method creates a tree of XMLSections from the input file.
 *
 * @param filename	The full path to the file to load.
 * @param returnEmptySection 	True if an empty section should be returned
 *								on failure.
 *
 * @return			The root of the tree on success, NULL otherwise.
 */
XMLSectionPtr XMLSection::createFromFile( const char * filename,
		bool returnEmptySection)
{
	ifstream stream( filename );

	if (!stream.good())
	{
		if(returnEmptySection)
		{
			return new XMLSection("root");
		}
		else
		{
			ERROR_MSG("XMLSection: Could not open %s.\n", filename);
			return (XMLSection *)NULL;
		}
    }

	XMLSectionPtr ret = XMLSection::createFromStream( "root", stream );
	return ret;
}


/**
 * This static method creates a tree of XMLSections from the input stream.
 *
 * @param tag		The name of the root section.
 * @param astream	Stream containing the file to load.
 *
 * @return			The root of the tree on success, NULL otherwise.
 */
XMLSectionPtr XMLSection::createFromStream(
	const std::string & tag, std::istream& astream )
{
	std::streampos bufnow = astream.tellg();
	astream.seekg( 0, ios_base::end );
	std::streampos buflen = astream.tellg() - bufnow;
	astream.seekg( bufnow, ios_base::beg );

	BinaryPtr pBlock = new BinaryBlock( astream, buflen, "BinaryBlock/XMLSection" );

	return XMLSection::createFromBinary( tag, pBlock );
	//istrstream stream((char *)pBinary->data(), pBinary->len());
	//return XMLSection::createFromStream(stream);
}





//#include "cstdmf/profile.hpp"

/**
 *	A local class based on VectorNoDestructor for our special
 *	parse stack requirements.
 */
template<class _Ty, class _A = std::allocator<_Ty> >
class MyVNoDest : public std::vector< _Ty, _A >
{
	typedef std::vector< _Ty, _A > BASE_TYPE;
public:

	MyVNoDest() :
		nElements_( 0 )
	{

	}

	INLINE void push_back( const _Ty & element )
	{
		MF_ASSERT_DEBUG( nElements_ >= 0 );
		if( BASE_TYPE::size() <= (typename BASE_TYPE::size_type)nElements_ )
		{
			BASE_TYPE::push_back( element );
			nElements_ = BASE_TYPE::size();
		}
		else
		{
			*( this->begin() + nElements_ ) = element;
			nElements_++;
		}
	}

	INLINE typename BASE_TYPE::iterator end()
	{
		return this->begin() + nElements_;
	}

	INLINE typename BASE_TYPE::const_iterator end() const
	{
		return this->begin() + nElements_;
	}

	INLINE typename BASE_TYPE::reverse_iterator rbegin()
	{
		return BASE_TYPE::reverse_iterator(this->begin() + nElements_);
	}

	INLINE typename BASE_TYPE::const_reverse_iterator rbegin() const
	{
		return BASE_TYPE::const_reverse_iterator(this->begin() + nElements_);
	}

	INLINE typename BASE_TYPE::reverse_iterator rend()
	{
		return BASE_TYPE::reverse_iterator(this->begin());
	}

	INLINE typename BASE_TYPE::const_reverse_iterator rend() const
	{
		return BASE_TYPE::const_reverse_iterator(this->begin());
	}


	INLINE typename BASE_TYPE::size_type size()
	{
		return nElements_;
	}

	INLINE typename BASE_TYPE::size_type size() const
	{
		return nElements_;
	}

	INLINE void reduceTo( int n )
	{
		nElements_ = n;
	}

	INLINE bool empty() const
	{
		return nElements_ == 0;
	}

	INLINE void clear()
	{
		nElements_ = 0;
	}

	INLINE _Ty &back()
	{
		MF_ASSERT_DEBUG( nElements_ > 0 );
		return *(this->begin() + nElements_ - 1 );
	}

	INLINE const _Ty &back() const
	{
		MF_ASSERT_DEBUG( nElements_ > 0 );
		return *(this->begin() + nElements_ - 1 );
	}

	INLINE void pop_back()
	{
		MF_ASSERT_DEBUG( nElements_ > 0 );
		nElements_ --;
	}

	INLINE void erase( typename BASE_TYPE::iterator _P )
	{
		std::copy(_P + 1, end(), _P);
		nElements_ --;
	}

private:
	int nElements_;
};

typedef MyVNoDest<XMLSection*> XMLParseStack;

static THREADLOCAL(XMLParseStack *) s_parseStack;
static THREADLOCAL(XMLParseStack *) s_parseKids;
//MF_THREAD_STORAGE char g_longTime[512];

// We store a vector a XMLParseStacks that can only be accessed by one thread at at time
static SimpleMutex s_parseStackMutex;
ScopedPointerArray< XMLParseStack > s_ParseStacks;

/**
 *	Helper class to dress a BinaryBlock up as a stream
 */
class WrapperStream
{
public:
	WrapperStream( BinaryPtr pBlock ) :
		pBlock_( pBlock ),
		cursor_( 0 )
	{ }
	~WrapperStream()
	{ }

	operator bool() const
	{
		return cursor_ < uint32(pBlock_->len());
	}

	uint32 tellg() const
	{
		return cursor_;
	}

	char * pCurr()
	{
		return &((char*)pBlock_->data())[cursor_];
	}

	char peek() const
	{
		if (cursor_ >= uint32(pBlock_->len())) return 0;
		return ((char*)(pBlock_->data()))[cursor_];
	}

	char get()
	{
		if (cursor_ >= uint32(pBlock_->len())) return 0;
		return ((char*)(pBlock_->data()))[cursor_++];
	}

	char * seek( char c )
	{
		char * data = (char*)pBlock_->data();
		while (*this)
		{
			if (data[ cursor_++ ] == c)
			{
				return &data[ cursor_ - 1 ];
			}
		}

		return NULL;
	}

private:
	BinaryPtr	pBlock_;
	uint32		cursor_;
};


/**
 *	This static method processes a "<!" section. This may be a comment or a
 *	CDATA section.
 */
bool XMLSection::processBang( WrapperStream & stream, XMLSection * pCurrNode )
{
	MF_VERIFY( stream.get() == '!' );

	char typeChar = stream.get();

	if (typeChar == '-')
	{
		if (stream.get() != '-')
		{
			ERROR_MSG( "XMLSection::createFromStream: "
				"Bad Comment: Expecting \"--\" following \"<!\"\n" );
			return false;
		}

		// Consume the comment.
		int commentCount = 1;

		char prev3 = '\0';
		char prev2 = '\0';
		char prev1 = stream.get();
		char currChar = stream.get();

		bool printedNestedError = false;

		while (stream && commentCount != 0)
		{
			prev3 = prev2;
			prev2 = prev1;
			prev1 = currChar;
			currChar = stream.get();

			if (prev2 == '-' && prev1 == '-' && currChar == '>')
			{
				commentCount--;
			}
			else if (prev3 == '<' && prev2 == '!' && prev1 == '-' &&
					currChar == '-')
			{
				commentCount++;
				if (!printedNestedError && commentCount > 1)
				{
					WARNING_MSG( "XMLSection %s contains nested comments\n",
						pCurrNode->sectionName().c_str() );
					printedNestedError = true;
				}
			}
		}

		if (!stream)
		{
			ERROR_MSG( "XMLSection::processBang: Comment not closed.\n" );
		}
	}
	else if (typeChar == '[')
	{
		if (!((stream.get() == 'C') &&
			(stream.get() == 'D') &&
			(stream.get() == 'A') &&
			(stream.get() == 'T') &&
			(stream.get() == 'A') &&
			(stream.get() == '[')))
		{
			ERROR_MSG( "XMLSection::createFromStream: "
				"Bad CDATA: Expecting \"CDATA[ following \"<[\"\n" );
			return false;
		}

		char * pTagStart = stream.pCurr();

		while (stream)
		{
			if (stream.get() == ']')
			{
				char * pTagEnd = stream.pCurr() - 1;
				if (stream.get() == ']' && stream.get() == '>')
				{
					*pTagEnd = '\0';
					if (pCurrNode == NULL)
					{
						ERROR_MSG( "XMLSection::processBang: "
								"CDATA outside section.\n" );
						return false;
					}
					else if (pCurrNode->cval_ == NULL)
					{
						pCurrNode->cval_ = pTagStart;
						return true;
					}
					else
					{
						ERROR_MSG( "XMLSection::processBang: "
								"Trying to set the value twice\n"
								"Tag = \"%s\"\n"
								"Old value = \"%s\"\n"
								"New value = \"%s\"\n",
								pCurrNode->sectionName().c_str(),
								pCurrNode->asString().c_str(),
								pTagStart );

						return false;
					}
				}
			}
		}

		ERROR_MSG( "XMLSection::processBang: CDATA section not closed.\n" );
	}

	return bool( stream );
}


/**
 *	This static method processes a "<?" section. If present, it is usually the
 *	XML description at the top of the file.
 */
bool XMLSection::processQuestionMark( WrapperStream & stream )
{
	MF_VERIFY( stream.get() == '?' );

	char prev1 = stream.get();
	char currChar = stream.get();

	while (stream && !(prev1 == '?' && currChar == '>'))
	{
		prev1 = currChar;
		currChar = stream.get();
	}

	if (!stream)
	{
		ERROR_MSG( "XMLSection::processQuestionMark: "
				"\"<?\" section not closed.\n" );
	}

	return bool( stream );
}

/**
 *	This static method creates a tree of XMLSections from the input buffer.
 *  NOTE: the data in pBlock will get modified. If you need to keep pBlock
 *  unmodified, you must pass in a deep copy of it instead.
 *
 *	@param rootTag	The tag of the root section.
 * 	@param pBlock	Binary block containing XML
 *
 * 	@return			The root of the tree on success, NULL otherwise.
 */
XMLSectionPtr XMLSection::createFromBinary(
	const std::string & rootTag, BinaryPtr pBlock )
{
	bool isInError = false;
	if (s_parseStack == NULL)
	{
		s_parseStack = new XMLParseStack();
		SimpleMutexHolder parseStackMutexHolder( s_parseStackMutex );
		s_ParseStacks.push_back( s_parseStack );
	}
	s_parseStack->clear();

	if (s_parseKids == NULL)
	{
		s_parseKids = new XMLParseStack();
		SimpleMutexHolder parseStackMutexHolder( s_parseStackMutex );
		s_ParseStacks.push_back( s_parseKids );
	}
	s_parseKids->clear();

	memoryCounterAdd( xml );

	char currChar = '\0';
	XMLSection * pCurrNode = NULL;
	XMLSectionPtr pRootNode = NULL;

	pRootNode = 0;

	// make a new stream
	//istrstream stream( bigbuf, buflen );
	WrapperStream stream( pBlock );
	char * bigbuf = (char*)pBlock->data();

	//uint64 addnodeTime = 0;
	//uint64 vsetTime = 0;
	//int nnodes = 0;
	//uint64 fullTime = timestamp();

	char lastPeek = stream.peek();
	do
	{
		if (isWhiteSpace( lastPeek ))
		{
			stream.get();
			skipWhiteSpace( stream );
			lastPeek = stream.peek();
		}

		if (lastPeek == '<')
		{
			// Consume '<'
			stream.get();

			const char peek = stream.peek();

			if (peek == '!')
			{
				isInError = !XMLSection::processBang( stream, pCurrNode );
			}
			else if (peek == '?')
			{
				isInError = !XMLSection::processQuestionMark( stream );
			}
			else
			{
				// Process the tag
				bool isEndTag = (stream.peek() == '/');

				if (isEndTag)
				{
					stream.get();
				}

				// Get the tag's string.
				int tagBeg = stream.tellg();
				char * tag = bigbuf + tagBeg;
				char * pTagEnd = stream.seek( '>' );

				if (pTagEnd)
				{
					*pTagEnd = '\0';

					while ((pTagEnd > tag) && isWhiteSpace( *(pTagEnd-1) ))
					{
						--pTagEnd;
					}

					*pTagEnd = '\0';
				}

				// Is it the root node?

				if (pTagEnd == NULL)
				{
					ERROR_MSG( "XMLSection::createFromStream: "
							"Unclosed tag\n" );
					isInError = true;
				}
				else if (pCurrNode == NULL)
				{
					if (isEndTag)
					{
						ERROR_MSG( "XMLSection::createFromStream: "
								"Cannot have an end tag at start of file\n" );
						isInError = true;
					}
					else
					{
						pRootNode = pCurrNode =
							new XMLSection( bigbuf+tagBeg, true );
					}
				}
				else
				{
					if (isEndTag)
					{
						bool matched = true;

						// Check whether the start and end tags match. If they
						// do not, we still check whether the start tag has
						// attributes and strip them if it'll help.

						if (pCurrNode->sectionName() != tag)
						{
							matched = false;
							char * startTag = (char *)pCurrNode->ctag_;
							int endLength = strlen( tag );

							if (strncmp( startTag, tag, endLength ) == 0)
							{
								if (isWhiteSpace( startTag[ endLength ] ))
								{
									startTag[ endLength ] = '\0';
									matched = true;
								}
							}
						}

						if (matched)
						{
							// We have finished with this tag and we are back in
							// the scope of the parent node.

#if ENABLE_RESOURCE_COUNTERS
							RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
								pCurrNode->children_.capacity() * sizeof(pCurrNode->children_[0]) )
#endif
							pCurrNode->children_.assign(
								s_parseKids->end() - *(int *)(&(pCurrNode->block_)),
								s_parseKids->end() );
#if ENABLE_RESOURCE_COUNTERS
							RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
								pCurrNode->children_.capacity() * sizeof(pCurrNode->children_[0]) )
#endif
							s_parseKids->reduceTo(
								s_parseKids->size() - *(int *)(&(pCurrNode->block_)) );

							// Clean up dodgy use of block_ for the child count.
							int & childCount = *(int *)(&(pCurrNode->block_));
							childCount = 0;

							pCurrNode->block_ = pBlock;

							memoryClaim( pCurrNode->children_ );

							if(!s_parseStack->empty())
							{
								pCurrNode = s_parseStack->back();
								s_parseStack->pop_back();
							}
							else
							{
								pCurrNode = NULL;
							}
						}
						else
						{
							ERROR_MSG( "XMLSection::createFromStream: "
										"Tags do not match\n"
										"\tStart tag = \"%s\"\n"
										"\tEnd tag   = \"%s\"\n",
									pCurrNode->sectionName().c_str(),
									tag );

							// Needed or bad things start happening
							pRootNode->incRef();

							// return NULL on failure as mentioned in the comments
							return NULL;
						}
					}
					else
					{
						//uint64 st = timestamp();

						// Strip off any '/' at the end then move it back on
						// after creating the XMLSection:
						bool removedTrailingSlash = false;
						size_t tagSz = strlen(tag);
						if (tag[tagSz - 1] == '/')
						{
							tag[tagSz - 1] = '\0';
							removedTrailingSlash = true;
						}

						XMLSection * pNewNode = new XMLSection( tag, true );

						if (removedTrailingSlash)
							tag[tagSz - 1] = '/';

						//addnodeTime += timestamp() - st;
						//nnodes++;

						// add this to its list of children ... later
						s_parseKids->push_back( pNewNode );

						// Dodgy use of block_ for the child count.
						int & childCount = *(int *)(&(pCurrNode->block_));
						childCount++;

						// Check for empty tag shorthand.
						if (*(pTagEnd-1) == '/')
						{
							// If it is an empty tag, we do not set it as the
							// current node since it is closed straight away.
							--pTagEnd;

							while ((pTagEnd > tag) &&
									isWhiteSpace( *(pTagEnd-1) ))
							{
								--pTagEnd;
							}
							*pTagEnd = '\0';

							// If the tag has white space, check whether it
							// looks like attributes and strip them.
							char * pCurr = tag;
							while (*pCurr && !isWhiteSpace( *pCurr ))
								++pCurr;
							if (*pCurr)
							{
								if (strchr( pCurr, '=' ))
								{
									*pCurr = '\0';
								}
								else
								{
									WARNING_MSG( "XMLSection:createFromStream: "
											"Tag '%s' contains white space.\n",
										tag );
								}
							}

							pNewNode->block_ = pBlock;
						}
						else
						{
							// Remember the old node.
							s_parseStack->push_back( pCurrNode );
							pCurrNode = pNewNode;
						}
					}
				}
			}

			lastPeek = stream.peek();
		}
		else if (!stream)
		{
			ERROR_MSG(  "XMLSection::createFromStream: Unterminated tag %s\n",
				(pCurrNode != NULL) ? pCurrNode->sectionName().c_str() : "NULL" );
			isInError = true;
		}
		else if (pCurrNode == NULL)
		{
			ERROR_MSG( "XMLSection::createFromStream: "
					"File must start with a tag\n" );
			isInError = true;
		}
		else
		{
			// Read in the value for the current section
			int index = 0;
			//char * pStringEnd = value;
			int valueBeg = stream.tellg();
			int valueEnd = valueBeg;

			while (stream && stream.peek() != '<')
			{
				currChar = stream.get();
				//value[ index++ ] = currChar;
				index++;

				// This is used to trim trailing white space.
				if (!isWhiteSpace( currChar ))
				//	pStringEnd = value + index;	// index already incremented
					valueEnd = valueBeg + index;
			}

			// Need to set here because we're about to set it to '\0'.
			lastPeek = stream.peek();

			//*pStringEnd = '\0';
			bigbuf[ valueEnd ] = 0;

			if (stream)
			{
				if (pCurrNode->cval_ != NULL)
				{
					ERROR_MSG( "XMLSection::createFromStream: "
							"Trying to set the value twice\n"
							"Tag = \"%s\"\n"
							"Old value = \"%s\"\n"
							"New value = \"%s\"\n",
							pCurrNode->sectionName().c_str(),
							pCurrNode->asString().c_str(),
							bigbuf+valueBeg );

					isInError = true;
				}
				else
				{
					//uint64 st = timestamp();
					//pCurrNode->value( value );

#ifndef NO_XML_ESCAPE_SEQUENCE
					XmlSpecialChars::reduce( bigbuf + valueBeg );
#endif // NO_XML_ESCAPE_SEQUENCE
					pCurrNode->cval_ = bigbuf + valueBeg;

					//vsetTime += timestamp() - st;
				}
			}
			else
			{
				ERROR_MSG( "XMLSection::createFromStream: "
						"Unterminated tag %s\n",
						pCurrNode ? pCurrNode->sectionName().c_str() : "NULL" );
				isInError = true;
			}
		}
	}
	while (((pCurrNode != NULL) ||(pRootNode == NULL)) && !isInError);

	if (isInError)
	{
		// Clean up dodgy use of block_ for the child count.
		if (pCurrNode)
		{
			int & childCount = *(int *)(&(pCurrNode->block_));
			childCount = 0;
		}

		while (!s_parseStack->empty())
		{
			int & childCount = *(int *)(&(s_parseStack->back()->block_));
			childCount = 0;
			s_parseStack->pop_back();
		}

		pRootNode = (XMLSection*)NULL;
	}

	if (pRootNode)
	{
		if (!rootTag.empty())
		{
			pRootNode->ctag_ = NULL;
#if ENABLE_RESOURCE_COUNTERS
			RESOURCE_COUNTER_SUB(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
				pRootNode->tag_.capacity() )
#endif
			pRootNode->tag_ = rootTag;
#if ENABLE_RESOURCE_COUNTERS
			RESOURCE_COUNTER_ADD(ResourceCounters::DescriptionPool("XMLSection", (uint)ResourceCounters::SYSTEM),
				pRootNode->tag_.capacity() )
#endif
			memoryClaim( pRootNode->tag_ );
		}
	}

	/*
	fullTime = timestamp() - fullTime;
	if (fullTime > stampsPerSecond() / 20)
	{
		std::strstream ss( g_longTime, sizeof(g_longTime) );
		ss << "full " << NiceTime( fullTime );
		ss << " add node " << NiceTime( addnodeTime );
		ss << " set value " << NiceTime( vsetTime );
		ss << " nnodes " << nnodes;
		ss.str();
	}
	*/

	return pRootNode;
}

/**
 * This method saves the tree rooted at this node to the input ostream.
 *
 * @param stream		The output stream to write to.
 * @param indentation	The indentation level to use.
 *
 * @return		True if successful, otherwise false.
 */
bool XMLSection::writeToStream( std::ostream & stream, int indentation ) const
{
	// Print the opening tag.
	for (int i = 0; i < indentation; i++)
		stream << '\t';

	std::string sectionTag;
	if (indentation == 0 && !this->isValidSectionName())
	{
		sectionTag = "root";
	}
	else
	{
		sectionTag = this->sectionName();
	}

	stream << '<' << sectionTag << '>';

	// Print the value of this section.
	//
	std::string value = cval_ ? std::string(cval_) : value_;
	if (value != "")
	{
#ifndef NO_XML_ESCAPE_SEQUENCE
		value = XmlSpecialChars::expand( value.c_str() );
#endif // NO_XML_ESCAPE_SEQUENCE
		stream << '\t' << value.c_str();
	}

	// Print the children.

	if (!children_.empty())
	{
		stream << endl;

		Children::const_iterator iter = children_.begin();

		while (iter != children_.end())
		{
			(*iter)->writeToStream( stream, indentation + 1 );

			iter++;
		}

		for (int i = 0; i < indentation; i++)
			stream << '\t';
	}
	else
	{
		stream << '\t';
	}

	// Print the closing tag.
	stream << "</" << sectionTag << '>' << endl;

	return true;
}

/**
 * 	This method saves the given section to a file.
 */
bool XMLSection::save( const std::string & saveAsFileName )
{
	// change our allegiance if we're doing a save as
	if (!saveAsFileName.empty())
	{
		if (!DataSection::splitSaveAsFileName( saveAsFileName, parent_, tag_ ))
			return false;
	}

	// and now save
	if (!parent_)
	{
		ERROR_MSG( "XMLSection: Can't save a section without a parent.\n" );
		return false;
	}

	return parent_->saveChild( this, false );
}


/**
 *	This static method replaces space characters with periods.
 *
 *	@param val				The string to sanitise.
 *	@return					The sanitised string.
 */
std::string XMLSection::sanitise( const std::string & val ) const
{
	return SanitiseHelper::substringReplace(val);
}


/**
 *	This static method replaces period characters with spaces.
 *
 *	@param val				The string to sanitise.
 *	@return					The sanitised string.
 */
std::string XMLSection::unsanitise( const std::string & val ) const
{
	return SanitiseHelper::substringReplace(
		val, SanitiseHelper::SANITISING_TOKEN, SanitiseHelper::SPACE_TOKEN);
}



/**
 *	This function sanitizes the sectionName.
 */
bool XMLSection::santiseSectionName()
{
	// Switch to using a std::string if necessary
	if (ctag_ != NULL)
	{
		tag_ = ctag_;
		ctag_ = NULL;
	}
	std::string newName = sanitise(tag_);
	if (newName[0] != '\0' && newName[0]>=0 && ::isdigit(newName[0]))
		newName = "id." + newName;
	if (newName != tag_)
	{
		tag_ = newName;
		return true;
	}
	else
	{
		return false;
	}
}


/**
 *	This function returns whether the section name is valid.
 */
bool XMLSection::isValidSectionName() const
{
	if (ctag_ != NULL)
		return isValidXMLTag(ctag_);
	else
		return isValidXMLTag(tag_.c_str());
}


/**
 *	This static method decodes a wide string that was encoded using
 *	XMLSection::encodeWideString()
 *
 *	@see XMLSection::encodeWideString
 *	@param	val				The encoded representation of the wide string.
 *	@return	std::wstring	The decoded wide string.
 */
std::wstring XMLSection::decodeWideString( const std::string& val )
{
	if ( !val.size() )
		return L"";

	const int BUF_LEN = 256;
	wchar_t buf[BUF_LEN];

	//if the first character is a bang, then do a straight ansi->unicode
	//this means that during development, we can easily hand-edit files
	//that require wide-string descriptions.
	if ( val[0] == '!' )
	{
		// Note: Strings longer than 256 characters are truncated!!
#ifdef _WIN32
		bw_snwprintf( buf, sizeof(buf)/sizeof(wchar_t), L"%.256S", val.c_str() );
#else
		swprintf( buf, BUF_LEN, L"%s", val.c_str() );
#endif
		return (buf+1);	//strip off the bang
	}

	int len;
	if ( (len = Base64::decode( val, (char*)buf, sizeof(wchar_t)*BUF_LEN)) >= 0 )
		return std::wstring(buf, len/sizeof(wchar_t));
	return L"error";
}


/**
 *	This static method encodes a wide string into a string that is xml-friendly.
 *	This string can be decoded using XMLSection::decodeWideString
 *
 *	@see XMLSection::decodeWideString
 *	@param	val				The encoded representation of the wide string.
 *	@return	std::wstring	The decoded wide string.
 */
std::string XMLSection::encodeWideString( const std::wstring& val )
{
	// no need to encode the null terminator, encoding only 'val.size()' chars
	return Base64::encode( (char*)&val[0], val.size() * sizeof(wchar_t));
}


// xml_section.cpp

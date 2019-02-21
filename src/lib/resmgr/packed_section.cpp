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

#include "packed_section.hpp"
#include "bin_section.hpp"

#include "xml_section.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/memory_counter.hpp"
#include "cstdmf/base64.h"
#include "cstdmf/concurrency.hpp"

#include <stdio.h>

DECLARE_DEBUG_COMPONENT2( "ResMgr", 0 )

using namespace PackedSectionData;

namespace
{
const uint32 PACKED_SECTION_MAGIC = 0x62a14e45;
const VersionType PACKED_SECTION_VERSION = 0;
}

/*
 *	Description of file format:
 *	Contains, in order:
 *	Magic number
 *	Version number
 *	String table
 *		Sequence of NULL-terminated strings terminated by an empty string.
 *		These strings are referred to with the array index of the string in this
 *		list. E.g. The first string is 0, the second 1, ...
 *	A packed data section with children block
 *
 *	Format of a packed data section with children block:
 *	Number of children
 *	Sequence of ChildRecords (DataPos, KeyPos pairs) (one for each child).
 *	A final DataPos
 *	A binary data block of the data for this section and then the data for each
 *	child section concatenated together.
 *
 *	The size of the data associated with this section is indicated by the first
 *	DataPos. This is located at the start of the binary data block.
 *
 *	For each child, the data starts at the DataPos of its record and ends at the
 *	start position of the next record. (This is offset from the start of the
 *	binary data block. The KeyPos is an index into the String table and is the
 *	name associated with that section.
 *
 *	The type of this section's data is indicated in the high bits of the first
 *	DataPos field. The type of the child data sections is in the high bits of
 *	its DataPos end field.
 *
 *	Format of data sections that have no children:
 *	Raw binary data for that type. For example, a float is four bytes of the
 *	float, a Vector3 is 3 consecutive floats, a Matrix 12 floats.
 *
 *	An integer is a little optimised. If 0, no data is stored, if the value fits
 *	in an int8, 1 byte is stored etc.
 */

namespace {

class OutputWriter
{
private:
	typedef std::list< BinaryPtr > List;

public:
	OutputWriter() : size_( 0 ) {}

	void write( const void * ptr, size_t size )
	{
		list_.push_back( new BinaryBlock( ptr, size, "BinaryBlock/OutputWriter" ) );
		size_ += size;
	}

	void write( BinaryPtr pBinary )
	{
		list_.push_back( pBinary );
		size_ += pBinary->len();
	}

	BinaryPtr getBinary()
	{
		BinaryPtr pBinary = new BinaryBlock( NULL, size_, "BinaryBlock/OutputWriter" );
		char * pDst = pBinary->cdata();

		List::const_iterator iter = list_.begin();
		while (iter != list_.end())
		{
			memcpy( pDst, (*iter)->data(), (*iter)->len() );
			pDst += (*iter)->len();
			++iter;
		}

		return pBinary;
	}

	int size() const	{ return size_; }

private:
	std::list< BinaryPtr > list_;
	int size_;
};


bool isVector3( DataSectionPtr pDS )
{
	if (!pDS)
		return false;

	float x, y, z;

	std::stringstream stream;
	stream << pDS->asString();
	stream >> x >> y >> z;

	return !stream.fail() && stream.eof();
}

/*
 *	This function returns whether or not the input section corresponds to a
 *	matrix.
 */
bool isMatrix( DataSectionPtr pDS )
{
	return (pDS->countChildren() == 4) &&
		isVector3( pDS->findChild( "row0" ) ) &&
		isVector3( pDS->findChild( "row1" ) ) &&
		isVector3( pDS->findChild( "row2" ) ) &&
		isVector3( pDS->findChild( "row3" ) );
}


/*
 *	This helper method return an XMLSection that is helpful for PackedSection
 *	conversions.
 */
DataSectionPtr getXMLSection()
{
	static DataSectionPtr pXMLSection = new XMLSection( "scratch" );

	return pXMLSection;
}


} // anon namespace

// -----------------------------------------------------------------------------
// Section: OutputStringTable
// -----------------------------------------------------------------------------

/**
 *	This class is used to collect the tags that will be written as the string
 *	table.
 */
class OutputStringTable
{
public:
	bool save( DataSectionPtr pDS, OutputWriter & output );
	int indexOf( const std::string & name ) const;
	int size() const	{ return map_.size(); }

private:
	void populate( DataSectionPtr pDS );
	bool save( char * pOutput, int len ) const;
	int calculateSaveSizeAndUpdate();

	typedef std::map< std::string, int > Map;
	Map map_;
};


/**
 *	This method saves the tags of the input data section to the output writer.
 *	It also initialises the table for later use.
 */
bool OutputStringTable::save( DataSectionPtr pDS, OutputWriter & output )
{
	this->populate( pDS );
	int size = this->calculateSaveSizeAndUpdate();
	BinaryPtr pBinary = new BinaryBlock( NULL, size, "BinaryBlock/OutputStringTable" );
	this->save( pBinary->cdata(), pBinary->len() );
	output.write( pBinary );
	return true;
}

/**
 *	This method populates this table with the tags of the nodes in the tree
 *	rooted at the input node. Note that the root nodes tag is not added.
 */
void OutputStringTable::populate( DataSectionPtr pDS )
{
	if (!isMatrix( pDS ))
	{
		DataSection::iterator iter = pDS->begin();

		while (iter != pDS->end())
		{
			// Note: the root section's name is not added.
			map_[ (*iter)->sectionName() ] = -1;
			this->populate( *iter );

			++iter;
		}
	}
}


/**
 *	This method calculates what the size of the string table will be on disk. It
 *	also updates the indices of the strings that will be used during saving.
 *
 *	@return The size of the string table on disk.
 */
int OutputStringTable::calculateSaveSizeAndUpdate()
{
	int size = 0;
	int count = 0;
	Map::iterator iter = map_.begin();

	while (iter != map_.end())
	{
		// The index associated with each string is just its index in the keys
		// list.
		iter->second = count;
		size += iter->first.size() + 1;

		++count;
		++iter;
	}

	return size + 1; // For extra trailing '\0'
}


/**
 *	This method saves the string table to the input string.
 */
bool OutputStringTable::save( char * pOutput, int len ) const
{
	// All of the strings are just concatenated together. The table is ended
	// with an empty string.
	int pos = 0;
	Map::const_iterator iter = map_.begin();

	while (iter != map_.end())
	{
		const std::string & str = iter->first;
		int endPos = pos + str.size() + 1;

		if (endPos >= len)
		{
			ERROR_MSG( "OutputStringTable::save: "
					"Data is too small to save table\n" );
			return false;
		}

		memcpy( pOutput + pos, str.c_str(), str.size() + 1 );
		pos = endPos;

		++iter;
	}

	// Null terminated. Empty string signifies the end of the string table
	pOutput[ pos++ ] = '\0';

	MF_ASSERT( pos == len );

	return pos == len;
}


/**
 *	This method returns the index associated with the input string.
 */
int OutputStringTable::indexOf( const std::string & name ) const
{
	Map::const_iterator iter = map_.find( name );

	if (iter != map_.end())
	{
		return iter->second;
	}

	CRITICAL_MSG( "OutputStringTable::indexOf: Did not find '%s'\n",
			name.c_str() );
	return 0;
}


// -----------------------------------------------------------------------------
// Section: Memory exchange
// -----------------------------------------------------------------------------

namespace
{

const char BW_DECRYPTED_BLOCK = 0;
const char BW_ENCRYPTED_BLOCK = 1;
const char BW_ENCRYPTION_KEY = char(0x9c);

/*
 *	This function returns an encrypted version of the input data.
 */
BinaryPtr encryptBinaryBlock( BinaryPtr pData )
{
	BinaryPtr pEncrypted = 
		new BinaryBlock( NULL, pData->len() + 1, "BinaryBlock/Encrypted" );
	((char*)pEncrypted->data())[0] = BW_ENCRYPTED_BLOCK;
	for (int i = 0; i < pData->len(); ++i)
	{
		((char*)pEncrypted->data())[i+1] = ((char *)pData->data())[i] ^ BW_ENCRYPTION_KEY;
	}

	return pEncrypted;
}


/*
 *	This function returns a decrypted version of the input data. The len
 *	parameter is modified to indicate the length of the decrypted data.
 *	If an error occurs, NULL is returned.
 */
char * decryptBinaryBlock( char * pData, int & len )
{
	if (len <= 0)
	{
		ERROR_MSG( "decryptBinaryBlock: Invalid length %d\n", len );

		return NULL;
	}

	if (pData[0] == BW_ENCRYPTED_BLOCK)
	{
		pData[0] = BW_DECRYPTED_BLOCK;
		for (int i = 1; i < len; ++i)
		{
			pData[i] ^= BW_ENCRYPTION_KEY;
		}
	}
	else if (pData[0] == BW_DECRYPTED_BLOCK)
	{
		// Already decrypted
	}
	else
	{
		ERROR_MSG( "decryptBinaryBlock: Invalid first character 0x%2x\n",
				pData[0] );
		return NULL;
	}

	len -= 1;
	return pData + 1;
}

}


// -----------------------------------------------------------------------------
// Section: PackedSection
// -----------------------------------------------------------------------------

/**
 *	Constructor for PackedSection.
 *
 *	@param name			The tag name of this section
 *	@param pData		The data associated with the PackedSection.
 *	@param dataLen		The length of the data.
 *	@param type			Indicates the type of packed section.
 *	@param pFile		A smart pointer to the parent file.
 */
PackedSection::PackedSection( const char * name, const char * pData,
		int dataLen, SectionType type, PackedSectionFilePtr pFile ) :
	name_( name ),
	ownDataLen_( dataLen ),
	pOwnData_( pData ),
	ownDataType_( type ),
	totalDataLen_( 0 ),
	pTotalData_( NULL ),
	pFile_( pFile )
{
	if (ownDataType_ == TYPE_DATA_SECTION)
	{
		pTotalData_ = pOwnData_;
		totalDataLen_ = ownDataLen_;

		if (totalDataLen_ < int(sizeof( NumChildrenType )))
		{
			ERROR_MSG( "PackedSection::PackedSection: "
					"%d is not enough data for %s\n", totalDataLen_, name_ );
			goto error;
		}

		// Find the start of the data block
		int numChildren = this->countChildren();
		int offset = sizeof( NumChildrenType ) +
			numChildren * sizeof( ChildRecord ) + sizeof( DataPosType );

		ownDataLen_ = this->pRecords()[-1].endPos();
		ownDataType_ = this->pRecords()[-1].type();

		if (offset < 0 || totalDataLen_ + 1 < offset + ownDataLen_)
		{
			ERROR_MSG( "PackedSection::PackedSection: "
					"Invalid offset %d for %s (Total = %d. Own = %d)\n",
				offset, name_, dataLen, ownDataLen_ );
			goto error;
		}
		else
		{
			pOwnData_ = pTotalData_ + offset;
		}
	}

	if (ownDataType_ == TYPE_ENCRYPTED_BLOB)
	{
		// Note: This may also change ownDataLen_.
		pOwnData_ =
			decryptBinaryBlock( const_cast<char *>( pOwnData_ ), ownDataLen_ );
		ownDataType_ = TYPE_BLOB;

		if (pOwnData_ == NULL)
		{
			goto error;
		}
	}

	return;
error:
	pTotalData_ = NULL;
	totalDataLen_ = 0;
	pOwnData_ = NULL;
	ownDataLen_ = 0;
	ownDataType_ = TYPE_STRING;
}


/**
 *	Destructor
 */
PackedSection::~PackedSection()
{
}


/**
 *	This method returns the number of children of this section-
 *
 *	@return 	Number of children.
 */
int PackedSection::countChildren()
{
	if (pTotalData_)
	{
		return *(NumChildrenType *)pTotalData_;
	}
	else if (this->isMatrix())
	{
		// Special case for Matrix34.
		return 4;
	}

	return 0;
}


/**
 *	Open the child with the given index
 *
 *	@param index	Index of the child to return.
 *
 *	@return 		Pointer to the specified child.
 */
DataSectionPtr PackedSection::openChild( int index )
{
	// TODO: Could do some range checking
	// return pTotalData_ ? this->pRecords()[ index ].createSection( this ) : NULL;
    if( pTotalData_ )
        return this->pRecords()[ index ].createSection( this );

	if (this->isMatrix() &&
		0 <= index && index < 4)
	{
		static const char * pRow[] = { "row0", "row1", "row2", "row3" };

		return new PackedSection( pRow[ index ],
			pOwnData_ + index * sizeof( Vector3 ),
			sizeof( Vector3 ), TYPE_FLOAT,
			this->pFile() );
	}

    return NULL;
}


/**
 *	This method returns a pointer to the array of child records.
 */
const PackedSection::ChildRecord * PackedSection::pRecords() const
{
	if (pTotalData_)
		return (ChildRecord *)(pTotalData_ + sizeof( NumChildrenType ));
	else
		return NULL;
}


/*
 *	Override from DataSection.
 */
DataSectionPtr PackedSection::newSection( const std::string & tag,
										 DataSectionCreator* creator )
{
	MF_ASSERT( !"PackedSection is currently read-only" );

	return NULL;
}

class PackedSectionCreator : public DataSectionCreator
{
public:
	virtual DataSectionPtr create( DataSectionPtr pSection,
									const std::string& tag )
	{
		MF_ASSERT( !"PackedSection is currently read-only" );

		return NULL;
	}

	virtual DataSectionPtr load(DataSectionPtr pSection,
									const std::string& tag,
									BinaryPtr pBinary = NULL)
	{
		DataSectionPtr pDS = PackedSection::create( tag, pBinary );
        // BCB6 doesn't like the following line
        // return pDS ? pDS : new BinSection( tag, pData );
        if( pDS )
            return pDS;
		return new BinSection( tag, pBinary );
	}
};

DataSectionCreator* PackedSection::creator()
{
	static PackedSectionCreator s_creator;
	return &s_creator;
}

/**
 *	Find the child with the given tag name
 *
 *	@param tag		Name of child to find
 *  @param creator  This parameter is ignored for PackedSections.
 *
 *	@return		A PackedSection matching tag on success, NULL on error.
 */
DataSectionPtr PackedSection::findChild( const std::string & tag,
										 DataSectionCreator* creator )
{
	int numChildren = this->countChildren();
	const ChildRecord * pCurr = this->pRecords();
	if(!pCurr)
		return NULL;
	const ChildRecord * pEnd = pCurr + numChildren;

	while (pCurr != pEnd)
	{
		if (pCurr->nameMatches( *pFile_, tag ))
			return pCurr->createSection( this );
		++pCurr;
	}

	if (this->isMatrix())
	{
		static const char * pRow[] = { "row0", "row1", "row2", "row3" };

		if ((tag.size() == 4) &&
			(tag[0] == 'r') &&
			(tag[1] == 'o') &&
			(tag[2] == 'w'))
		{
			int index = tag[3] - '0';
			if (0 <= index && index < 4)
			{
				return new PackedSection( pRow[ index ],
					pOwnData_ + index * sizeof( Vector3 ),
					sizeof( Vector3 ), TYPE_FLOAT,
					this->pFile() );
			}
		}
	}

	return NULL;
}


/*
 *	Override from DataSection.
 */
void PackedSection::delChild( const std::string & tag )
{
	MF_ASSERT( !"PackedSection::delChild: Not yet implemented" );
}


/*
 *	Override from DataSection.
 */
void PackedSection::delChild( DataSectionPtr pSection )
{
	MF_ASSERT( !"PackedSection::delChild: Not yet implemented" );
}

/*
 *	Override from DataSection.
 */
void PackedSection::delChildren()
{
	MF_ASSERT( !"PackedSection::delChild: Not yet implemented" );
}


/**
 * 	This method returns the name of the section
 *
 *	@return 		The section name
 */
std::string PackedSection::sectionName() const
{
	return name_;
}


/**
 * 	This method returns the number of bytes used by this section.
 *
 *	@return 		Number of bytes used by this DirSection.
 */
int PackedSection::bytes() const
{
	return std::max( ownDataLen_, totalDataLen_ );
}


/*
 *	Override from DataSection.
 */
bool PackedSection::save( const std::string& saveAsFileName )
{
	// MF_ASSERT( !"Not implemented" );
	ERROR_MSG( "PackedSection::save: Not implemented. Trying to save %s\n",
		saveAsFileName.c_str() );
	return false;
}


// -----------------------------------------------------------------------------
// Section: Value accessors
// -----------------------------------------------------------------------------

/*
 *	Override from DataSection.
 */
bool PackedSection::asBool( bool defaultVal )
{
	if (ownDataType_ == TYPE_BOOL)
	{
		return (ownDataLen_ > 0);
	}

	DataSectionPtr pDS = getXMLSection();
	std::string asString = static_cast< DataSection * >( this )->asString();
	pDS->setString( asString );
	WARNING_MSG( "PackedSection::asBool: "
				"Accessing %s as bool when type is %x '%s'\n",
			name_, ownDataType_, asString.c_str() );
	return pDS->asBool( defaultVal );
}


/*
 *	Override from DataSection.
 */
int PackedSection::asInt( int defaultVal )
{
	if (ownDataType_ == TYPE_INT)
	{
		switch (ownDataLen_)
		{
			case 0: return 0;
			case 1: return *(int8 *)pOwnData_;
			case 2: return *(int16 *)pOwnData_;
			case 4: return *(int32 *)pOwnData_;

			default:
				ERROR_MSG( "PackedSection::asInt: Unhandled length %d\n",
						ownDataLen_ );
			break;
		}
	}

	WARNING_MSG( "PackedSection::asInt: "
			"Accessing %s as int when type is %x\n", name_, ownDataType_ );

	DataSectionPtr pDS = getXMLSection();
	pDS->setString( static_cast< DataSection * >( this )->asString() );
	return pDS->asInt( defaultVal );
}


/*
 *	Override from DataSection.
 */
long PackedSection::asLong( long defaultVal )
{
	return this->asInt( defaultVal );
}


/*
 *	Override from DataSection.
 */
float PackedSection::asFloat( float defaultVal )
{
	if (ownDataType_ == TYPE_FLOAT)
	{
		switch (ownDataLen_)
		{
			case 0: return 0.f;
			case 4: return *(float *)pOwnData_;

			default:
				ERROR_MSG( "PackedSection::asFloat: Incorrect length %d\n",
						ownDataLen_ );
			break;
		}
	}
	else if (ownDataType_ == TYPE_INT)
	{
		return float( this->asInt( 0 ) );
	}
	else if (ownDataType_ == TYPE_STRING)
	{
		if (ownDataLen_ > 0)
		{
			std::string tmp( pOwnData_, ownDataLen_ );
			return float( atof( tmp.c_str() ) );
		}
		else
		{
			return defaultVal;
		}
	}

	WARNING_MSG( "PackedSection::asFloat: "
			"Accessing %s as float when type is %x\n", name_, ownDataType_ );

	DataSectionPtr pDS = getXMLSection();
	pDS->setString( static_cast< DataSection * >( this )->asString() );
	return pDS->asFloat( defaultVal );
}


/*
 *	Override from DataSection.
 */
double PackedSection::asDouble( double defaultVal )
{
	// TODO: Should we handle doubles?
	return this->asFloat( float( defaultVal ) );
}


/*
 *	Override from DataSection.
 */
std::string PackedSection::asString( const std::string &defaultVal,
		int /*flags*/ )
{
	if (ownDataType_ == TYPE_STRING)
		return std::string( pOwnData_, ownDataLen_ );

	if (ownDataType_ == TYPE_BLOB)
	{
		return Base64::encode( pOwnData_, ownDataLen_ );
	}

	std::stringstream stream;

	switch (ownDataType_)
	{
		case TYPE_INT:
			stream << static_cast< DataSection *>( this )->asInt();
			break;

		case TYPE_FLOAT:
			if (this->isMatrix())
			{
				// Matrix is an empty string. It has children
			}
			else if (ownDataLen_ % sizeof( float ) == 0)
			{
				/*
				for (unsigned int i = 0; i < ownDataLen_/sizeof( float ); ++i)
				{
					if (i != 0)
						stream << " ";
					stream.setf( stream.showpoint );
					stream << ((float *)pOwnData_)[i];
				}
				*/
				DataSectionPtr pDS = getXMLSection();
				DataSection * pThis = this;

				switch( ownDataLen_/sizeof( float ) )
				{
					case 1:
						pDS->setFloat( pThis->asFloat() );
						break;

					case 2:
						pDS->setVector2( pThis->asVector2() );
						break;

					case 3:
						pDS->setVector3( pThis->asVector3() );
						break;

					case 4:
						pDS->setVector4( pThis->asVector4() );
						break;

					default:
						ERROR_MSG( "PackedSection::asString: "
								"Invalid float data.\n" );
						pDS->setString( "" );
						break;

				}

				return pDS->asString();
			}
			else
			{
				ERROR_MSG( "PackedSection::asString: Invalid float length %d\n",
						ownDataLen_ );
			}
			break;

		case TYPE_BOOL:
			if (static_cast< DataSection * >( this )->asBool())
				stream << "true";
			else
				stream << "false";
			break;

		default:
			ERROR_MSG( "PackedSection::asString: Invalid type %x for %s\n",
					ownDataType_, name_ );
			break;
	}

	return stream.str();
}


/*
 *	Override from DataSection.
 */
std::wstring PackedSection::asWideString(
		const std::wstring &defaultVal, int flags )
{
	// NOTE: PackedSection::asString() currently doesn't use the defaultVal
	return XMLSection::decodeWideString( this->asString( std::string(), flags ) );

	// This code is commented out because it doesn't match how res_packer 
	// works.
	/*	
	if (ownDataType_ == TYPE_STRING)
	{
		if (ownDataLen_ % sizeof( wchar_t ) == 0)
		{
			return std::wstring( (wchar_t *)pOwnData_,
					ownDataLen_ / sizeof( wchar_t ) );
		}
		else
		{
			ERROR_MSG( "PackedSection::asWideString: Invalid size %d for %s\n",
					ownDataLen_, name_ );
		}
	}

	std::wstringstream wstream;
	wstream << static_cast< DataSection * >( this )->asString().c_str();

	return wstream.str();
	*/
}


/*
 *	Override from DataSection.
 */
Vector2 PackedSection::asVector2( const Vector2 &defaultVal )
{
	if ((ownDataType_ == TYPE_FLOAT) && (ownDataLen_ == sizeof( Vector2 )))
	{
		return *(Vector2 *)pOwnData_;
	}

	DataSectionPtr pDS = getXMLSection();
	pDS->setString( static_cast< DataSection * >( this )->asString() );
	return pDS->asVector2( defaultVal );
}


/*
 *	Override from DataSection.
 */
Vector3 PackedSection::asVector3( const Vector3 &defaultVal )
{
	if ((ownDataType_ == TYPE_FLOAT) && (ownDataLen_ == sizeof( Vector3 )))
	{
		return *(Vector3 *)pOwnData_;
	}

	DataSectionPtr pDS = getXMLSection();
	pDS->setString( static_cast< DataSection * >( this )->asString() );
	return pDS->asVector3( defaultVal );
}


/*
 *	Override from DataSection.
 */
Vector4 PackedSection::asVector4( const Vector4 &defaultVal )
{
	if ((ownDataType_ == TYPE_FLOAT) && (ownDataLen_ == sizeof( Vector4 )))
	{
		return *(Vector4 *)pOwnData_;
	}

	DataSectionPtr pDS = getXMLSection();
	pDS->setString( static_cast< DataSection * >( this )->asString() );
	return pDS->asVector4( defaultVal );
}


/*
 *	Override from DataSection.
 */
Matrix PackedSection::asMatrix34( const Matrix &defaultVal )
{
	Matrix m; // Defaults to the zero matrix.
	m( 3, 3 ) = 1.f;

	if ((ownDataType_ == TYPE_FLOAT) &&
			(ownDataLen_ == 4 * sizeof( Vector3 )))
	{		
		const float * pData = (float*)pOwnData_;
		
		for ( uint32 i = 0; i < 4; ++i)
		{
			for ( uint32 j = 0; j < 3; ++j )
			{
				m( i, j ) = *pData++;
			}
		}
	}
	else
	{
		m[0] = this->readVector3( "row0", defaultVal[0] );
		m[1] = this->readVector3( "row1", defaultVal[1] );
		m[2] = this->readVector3( "row2", defaultVal[2] );
		m[3] = this->readVector3( "row3", defaultVal[3] );
	}

	return m;
}


/*
 *	Override from DataSection.
 */
std::string PackedSection::asBlob( const std::string &defaultVal )
{
	if (ownDataType_ == TYPE_BLOB)
		return std::string( pOwnData_, ownDataLen_ );

	if ((ownDataType_ == TYPE_STRING) && (ownDataLen_ == 0))
		return "";

	if ((ownDataType_ == TYPE_INT) || (ownDataType_ == TYPE_BOOL))
	{
		std::string decoded;
		if (Base64::decode(
					static_cast< DataSection * >(this)->asString(), decoded ))
			return decoded;
	}

	ERROR_MSG( "PackedSection::asBlob: Invalid type %x for %s\n",
					ownDataType_, name_ );

	DataSectionPtr pDS = getXMLSection();
	pDS->setString( static_cast< DataSection * >( this )->asString() );
	return pDS->asBlob( defaultVal );
}


/*
 *	Override from DataSection.
 */
BinaryPtr PackedSection::asBinary()
{
	if (ownDataType_ == TYPE_BLOB)
		return 
			new BinaryBlock
			( 
				pOwnData_, 
				ownDataLen_, 
				"BinaryBlock/PackedSection",
				pFile_->pFileData()				
			);

	std::string asBlob = this->asBlob( "" );
	return new BinaryBlock( asBlob.data(), asBlob.length(), "BinaryBlock/PackedSection" );
}


// -----------------------------------------------------------------------------
// Section: Static methods
// -----------------------------------------------------------------------------

/**
 *	This static method creates a data section from the input data if it is of
 *	the correct type.
 *
 *	@return A new data section, if one was created, otherwise NULL.
 */
DataSectionPtr PackedSection::create( const std::string & tag, BinaryPtr pData )
{
	int headerSize = sizeof( PACKED_SECTION_MAGIC ) + sizeof( VersionType );
	if (pData->len() >= headerSize)
	{
		if (*(uint32 *)pData->data() == PACKED_SECTION_MAGIC)
		{
			// The data sections keep a reference to their file.
			PackedSectionFilePtr pFile = new PackedSectionFile( tag, pData );

			return pFile->createRoot();
		}
	}

	return NULL;
}


// -----------------------------------------------------------------------------
// Section: Conversion
// -----------------------------------------------------------------------------

#include "bwresource.hpp"

#include <limits>

namespace
{

template <class TYPE>
bool isInRange( int value )
{
	return (std::numeric_limits<TYPE>::min() <= value) &&
		(value <= std::numeric_limits<TYPE>::max());
}


/*
 *	This function makes the best guess on how to convert a section's value.
 */
SectionType addBestGuess( DataSectionPtr pDS, OutputWriter & output,
		std::list< std::string > & path )
{
	if (!pDS->canPack())
	{
		// Currently assuming all non-packable data section want to be
		// encrypted. We may want to be more selective. We should only get
		// here is shouldEncrypt is true in the call to convert.
		BinaryPtr pEncrypted = ::encryptBinaryBlock( pDS->asBinary() );
		output.write( pEncrypted->data(), pEncrypted->len() );
		return TYPE_ENCRYPTED_BLOB;
	}

	const std::string & str = pDS->asString();

	if (str.empty())
	{
		return TYPE_STRING;
	}

	if (str == "true")
	{
		output.write( "\1", 1 );
		return TYPE_BOOL;
	}

	if (str == "false")
	{
		return TYPE_BOOL;
	}

	{
		std::stringstream stream;
		stream << str;
		int value = 1234;
		stream >> value;

		if (!stream.fail() && stream.eof())
		{
			if (value == 0)
			{
				// Do nothing
			}
			else if (isInRange<int8>( value ))
			{
				int8 newVal = value;
				output.write( &newVal, sizeof( newVal ) );
			}
			else if (isInRange<int16>( value ))
			{
				int16 newVal = value;
				output.write( &newVal, sizeof( newVal ) );
			}
			else
			{
				output.write( &value, sizeof( value ) );
			}

			return TYPE_INT;
		}
	}

	{
		std::stringstream stream;
		stream << str;

		float vals[4];
		int i = 0;

		do
		{
			stream >> vals[i];
			++i;
		}
		while ((i < 4) && !stream.fail() && !stream.eof());

		if (!stream.fail() && stream.eof())
		{
			bool isOkay = false;
			DataSectionPtr pDS = getXMLSection();

			switch (i)
			{
			case 1:
				pDS->setFloat( vals[0] );
				isOkay = (str == pDS->asString());
				break;

			case 2:
				pDS->setVector2( *(Vector2*)vals );
				isOkay = (str == pDS->asString());
				break;

			case 3:
				pDS->setVector3( *(Vector3*)vals );
				isOkay = (str == pDS->asString());
				break;

			case 4:
				pDS->setVector4( *(Vector4*)vals );
				isOkay = (str == pDS->asString());
				break;

			}

			if (isOkay)
			{
				output.write( &vals, i * sizeof( float ) );
				return TYPE_FLOAT;
			}
			else
			{
				std::stringstream stream;
				std::list< std::string >::iterator iter = path.begin();
				while (iter != path.end())
				{
					stream << '/' << *iter;
					++iter;
				}

				WARNING_MSG( "addBestGuess: Float: %s '%s' != '%s'\n",
						stream.str().c_str(),
						pDS->asString().c_str(),
						str.c_str() );
			}
		}
	}

	std::string decoded;
	if (Base64::decode( str, decoded ) &&
			(Base64::encode( decoded ) == str))
	{
		output.write( decoded.data(), decoded.size() );
		return TYPE_BLOB;
	}

	output.write( str.data(), str.size() );
	return TYPE_STRING;
}


SectionType writeAsPackedSection( const OutputStringTable & stringTable,
		DataSectionPtr pDS, OutputWriter & output,
		std::list< std::string > & path )
{
	int numChildren = pDS->countChildren();

	// Also checking here that this is not the root node (path.empty). This is
	// needed because the root node is assumed to be TYPE_DATA_SECTION.
	if (numChildren == 0 && !path.empty())
	{
		return addBestGuess( pDS, output, path );
	}
	else if (isMatrix( pDS ) && !path.empty())
	{
		Vector3 v[4];
		for (int i = 0; i < 4; ++i)
		{
			v[i] = pDS->openChild( i )->asVector3();
		}
		output.write( v, sizeof( v ) );

		return TYPE_FLOAT;
	}

	{
		int headerSize = sizeof( NumChildrenType ) +
			numChildren * sizeof( PackedSection::ChildRecord ) +
			sizeof( DataPosType );
		BinaryPtr pBlock = new BinaryBlock( NULL, headerSize, "BinaryBlock/PackedSection" );
		output.write( pBlock );
		*(NumChildrenType *)pBlock->cdata() = numChildren;
		PackedSection::ChildRecord * pRecord =
			(PackedSection::ChildRecord *)(pBlock->cdata() + sizeof( NumChildrenType ));

		int startPos = output.size();

		SectionType ownType = addBestGuess( pDS, output, path );
		pRecord[ -1 ].setEndPosAndType( output.size() - startPos, ownType );

		DataSection::iterator iter = pDS->begin();
		while (iter != pDS->end())
		{
			path.push_back( (*iter)->sectionName() );
			SectionType childType =
				writeAsPackedSection( stringTable, *iter, output, path );
			path.pop_back();
			pRecord->setEndPosAndType(
					output.size() - startPos, childType );
			pRecord->setKeyPos( stringTable.indexOf( (*iter)->sectionName() ) );

			++pRecord;
			++iter;
		}

		return TYPE_DATA_SECTION;
	}
}


bool convertToPackedFile( DataSectionPtr pDS, OutputWriter & output )
{
	output.write( &PACKED_SECTION_MAGIC, sizeof( PACKED_SECTION_MAGIC ) );
	output.write( &PACKED_SECTION_VERSION, sizeof( PACKED_SECTION_VERSION ) );

	OutputStringTable stringTable;
	stringTable.save( pDS, output );

	std::list< std::string > path;
	return writeAsPackedSection( stringTable, pDS, output, path ) ==
			TYPE_DATA_SECTION;
}

}


/**
 *	This static helper method opens a data section from an path that is _not_
 *	relative to BW_RES_PATH.
 */
DataSectionPtr PackedSection::openDataSection( const std::string & path )
{
	// Note: This isn't really anything to do with PackedSection and could be in
	// DataSection. It is here because it is used by res_packer.

	FILE * pInFile = fopen( path.c_str(), "rb" );
	if (!pInFile)
	{
		ERROR_MSG( "PackedSection::openDataSection: "
				"Failed to open input file %s\n", path.c_str() );
		return NULL;
	}

	fseek( pInFile, 0, SEEK_END );
	int len = ftell( pInFile );
	fseek( pInFile, 0, SEEK_SET );

	BinaryPtr pData = new BinaryBlock( NULL, len, "BinaryBlock/PackedSection" );
	if (!fread( pData->cdata(), len, 1, pInFile ))
	{
		ERROR_MSG( "PackedSection::openDataSection: "
				"Failed to read from %s\n", path.c_str() );
		fclose( pInFile );
		return NULL;
	}
	fclose( pInFile );

	DataSectionPtr pDS = DataSection::createAppropriateSection( "root", pData );

	return pDS;
}


/**
 *	This static method converts the file at inPath and writes it to a file at
 *	outPath.
 */
bool PackedSection::convert( const std::string & inPath,
		const std::string & outPath,
		std::vector< std::string > * pStripStrings,
		bool shouldEncrypt )
{
	DataSectionPtr pDS = PackedSection::openDataSection( inPath );

	if (!pDS)
	{
		ERROR_MSG( "PackedSection::convert: "
				"Failed to open data section %s\n", inPath.c_str() );
		return false;
	}

	if (pStripStrings)
	{
		std::vector< std::string >::iterator iter = pStripStrings->begin();
		while (iter != pStripStrings->end())
		{
			pDS->delChild( *iter );
			++iter;
		}
	}

	BinaryPtr pOutData;

	if (pDS->canPack() || shouldEncrypt)
	{
		OutputWriter output;
		convertToPackedFile( pDS, output );

		pOutData = output.getBinary();
	}
	else
	{
		pOutData = pDS->asBinary();
	}

	FILE * pOutFile = fopen( outPath.c_str(), "wb" );

	if (!pOutFile)
	{
		ERROR_MSG( "PackedSection::convert: "
				"Failed to open output file %s\n", outPath.c_str() );
		return false;
	}

	if (fwrite( pOutData->data(), pOutData->len(), 1, pOutFile ) != 1)
	{
		ERROR_MSG( "PackedSection::convert: Failed to write to %s\n",
				outPath.c_str() );
		fclose( pOutFile );
		return false;
	}

	fclose( pOutFile );

	return true;
}


/**
 *	This static method converts the input DataSection tree as a PackedSection
 *	at the input path relative to BW_RES_PATH.
 */
bool PackedSection::convert( DataSectionPtr pDS, const std::string & path )
{
	bool			rv = false;
	OutputWriter	output;

	if ( convertToPackedFile( pDS, output ) )
	{
		BinaryPtr pBinary = output.getBinary();
		DataSectionPtr pParent;
		std::string childTag;
		DataSection::splitSaveAsFileName( path, pParent, childTag );
		DataSectionPtr pTemp = new BinSection( childTag, pBinary );

		rv  = pParent->saveChild( pTemp, true );
	}

	return rv;
}


// -----------------------------------------------------------------------------
// Section: ChildRecord
// -----------------------------------------------------------------------------

/**
 *	This method creates a data section corresponding to this record.
 */
DataSectionPtr PackedSection::ChildRecord::createSection(
						const PackedSection * pSection ) const
{
	SectionType type = this->type();

	int sPos = this->startPos();
	int ePos = this->endPos();

	const char * pNewData = pSection->getDataBlock() + sPos;
	int newDataLen = ePos - sPos;
	const char * name = this->getName( *pSection->pFile() );

	return new PackedSection( name,
				pNewData, newDataLen, type, pSection->pFile() );
}


/**
 *	This method return whether the input string matches this record.
 */
bool PackedSection::ChildRecord::nameMatches(
		const PackedSectionFile & file, const std::string & tag ) const
{
	return tag == this->getName( file );
}


/**
 *	 This method returns the name associated with this child section record.
 */
const char * PackedSection::ChildRecord::getName(
				const PackedSectionFile & parentFile ) const
{
	return parentFile.getIndexedString( keyPos_ );
}


// -----------------------------------------------------------------------------
// Section: PackedSectionFile
// -----------------------------------------------------------------------------

/**
 *	This method creates the root data section associated with this file.
 */
DataSectionPtr PackedSectionFile::createRoot()
{
	int usedSize = sizeof( PACKED_SECTION_MAGIC ) + sizeof( VersionType );
	int stringTableSize = stringTable_.init( pFileData_->cdata() + usedSize,
			pFileData_->len() - usedSize );

	usedSize += stringTableSize;

	DataSectionPtr pDS =
		new PackedSection( storedName_.c_str(),
				pFileData_->cdata() + usedSize, pFileData_->len() - usedSize,
				TYPE_DATA_SECTION, this );

	return pDS;
}


// -----------------------------------------------------------------------------
// Section: StringTable
// -----------------------------------------------------------------------------

/**
 *	This method initialises this StringTable using the data passed in. The data
 *	that is left over is returned.
 */
int PackedSectionFile::StringTable::init( const char * pData, int dataLen )
{
	// It should not have been initialised yet.
	MF_ASSERT( table_.empty() );

	const char * pCurr = pData;
	const char * pEnd = pData + dataLen;

	while ((pCurr != pEnd) && (*pCurr != '\0'))
	{
		table_.push_back( pCurr );

		// Find the next string.
		while (*(pCurr++) && (pCurr != pEnd))
			/* do nothing */;
	}

	if (pCurr == pEnd)
	{
		ERROR_MSG( "PackedSection::StringTable::init: Not enough data.\n" );
		table_.clear();
		// Consume all the data so that PackedSection with handle the error.
		return dataLen;
	}

	++pCurr;

	return pCurr - pData;
}


/**
 *	This method returns the string corresponding to the input key.
 */
const char * PackedSectionFile::StringTable::getString( KeyPosType key ) const
{
	if ((0 <= key) && (key < KeyPosType( table_.size() )))
	{
		return table_[ key ];
	}

	return NULL;
}

// packed_section.cpp

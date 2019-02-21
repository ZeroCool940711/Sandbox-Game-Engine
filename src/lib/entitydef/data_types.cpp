/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

/**
 * 	@file data_types.cpp
 *
 *	Implementations for concrete DataTypes
 *
 *	@ingroup entity
 */

#include "pch.hpp"

#include "data_types.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/md5.hpp"
#include "cstdmf/memory_stream.hpp"

#include "resmgr/bwresource.hpp"

#include "pyscript/pickler.hpp"
#include "pyscript/script_math.hpp"

#include "mailbox_base.hpp"

#include <limits>

DECLARE_DEBUG_COMPONENT2( "DataTypes", 0 )


// token to force linking this file in
int DATA_TYPES_TOKEN = 0;


// force tokens of other data types in as well
extern int UserDataObjectLinkDataType_token;
static int s_tokenSet = UserDataObjectLinkDataType_token;


// -----------------------------------------------------------------------------
// Section: IntegerDataType
// -----------------------------------------------------------------------------

/**
 *	This template class is used to represent the different types of integer data
 *	type.
 *
 *	@ingroup entity
 */
template <class INT_TYPE>
class IntegerDataType : public DataType
{
	public:
		IntegerDataType( MetaDataType * pMeta ) : DataType( pMeta ) { }

	protected:
		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::isSameType
		 */
		virtual bool isSameType( PyObject * pValue )
		{
			int intValue;
			if (Script::setData( pValue, intValue ) != 0)
			{
				PyErr_PrintEx(0);
				return false;
			}

			INT_TYPE value = (INT_TYPE) intValue;
			if (intValue != int(value))
			{
				ERROR_MSG( "IntegerDataType::isSameType: "
						"%d is out of range (truncated = %d).\n",
					intValue, int(value) );

				return false;
			}

			return true;
		}

		/**
		 *	This method sets the default value for this type.
		 *
		 *	@see DataType::setDefaultValue
		 */
		virtual void setDefaultValue( DataSectionPtr pSection )
		{
			pDefaultValue_ = (pSection) ? this->createFromSection( pSection ) :
								PyObjectPtr( PyInt_FromLong( 0 ),
										PyObjectPtr::STEAL_REFERENCE );
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::pDefaultValue
		 */
		virtual PyObjectPtr pDefaultValue() const
		{
			return pDefaultValue_;
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::addToStream
		 */
		virtual void addToStream( PyObject * pValue,
				BinaryOStream & stream, bool /*isPersistentOnly*/ ) const
		{
			int intValue = 0;
			int err = Script::setData( pValue, intValue,
				"IntegerDataType.addToStream" );
			if (err != 0)
			{
				ERROR_MSG( "IntegerDataType::addToStream: "
					"setData failed\n" );
				PyErr_PrintEx(0);
				MF_EXIT( "IntegerDataType::addToStream: "
					"setData failed\n" );
			}

			INT_TYPE value = (INT_TYPE) intValue;
			MF_ASSERT_DEV( intValue == int(value) );

			stream << value;
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::createFromStream
		 */
		virtual PyObjectPtr createFromStream( BinaryIStream & stream,
				bool /*isPersistentOnly*/ ) const
		{
			INT_TYPE value = 0;
			stream >> value;

			if (stream.error())
			{
				ERROR_MSG( "IntegerDataType::createFromStream: "
						   "Not enough data on stream to read value\n" );
				return NULL;
			}

			return PyObjectPtr( PyInt_FromLong( (long)value ),
				PyObjectPtr::STEAL_REFERENCE );
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::addToSection
		 */
		virtual void addToSection( PyObject * pValue, DataSectionPtr pSection )
			const
		{
			int intValue = PyInt_AsLong( pValue );
			INT_TYPE value = (INT_TYPE) intValue;
			MF_ASSERT_DEV( intValue == int(value) );

			if (PyErr_Occurred())
			{
				ERROR_MSG( "IntegerDataType::addToStream: "
						"PyInt_AsLong failed\n" );
				PyErr_PrintEx(0);
				MF_EXIT( "IntegerDataType::addToStream: "
						"PyInt_AsLong failed\n" );
			}

			pSection->setInt( value );
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::createFromSection
		 */
		virtual PyObjectPtr createFromSection( DataSectionPtr pSection ) const
		{
			int intValue = pSection->asInt();
			INT_TYPE value = (INT_TYPE) intValue;
			MF_ASSERT_DEV( int(value) == intValue );

			return PyObjectPtr( PyInt_FromLong( (long)value ),
				PyObjectPtr::STEAL_REFERENCE );
		}

		virtual bool fromStreamToSection( BinaryIStream & stream,
				DataSectionPtr pSection, bool isPersistentOnly ) const
		{
			INT_TYPE value = 0;
			stream >> value;
			if (stream.error()) return false;

			pSection->setInt( value );
			return true;
		}

		virtual void addToMD5( MD5 & md5 ) const
		{
			if (std::numeric_limits<INT_TYPE>::is_signed)
				md5.append( "Int", sizeof( "Int" ) );
			else
				md5.append( "Uint", sizeof( "Uint" ) );
			int size = sizeof(INT_TYPE);
			md5.append( &size, sizeof(int) );
		}

		virtual bool operator<( const DataType & other ) const
		{
			if (this->DataType::operator<( other )) return true;
			if (other.DataType::operator<( *this )) return false;

			const IntegerDataType& otherInt =
				static_cast< const IntegerDataType& >( other );
			return (Script::compare( pDefaultValue_.getObject(),
				otherInt.pDefaultValue_.getObject() ) < 0);
		}

	private:
		PyObjectPtr pDefaultValue_;
};


SIMPLE_DATA_TYPE( IntegerDataType< uint8 >,  UINT8 )
SIMPLE_DATA_TYPE( IntegerDataType< uint16 >, UINT16 )
// SIMPLE_DATA_TYPE( IntegerDataType< uint32 >, UINT32 )

SIMPLE_DATA_TYPE( IntegerDataType< int8 >,  INT8 )
SIMPLE_DATA_TYPE( IntegerDataType< int16 >, INT16 )
SIMPLE_DATA_TYPE( IntegerDataType< int32 >, INT32 )

// -----------------------------------------------------------------------------
// Section: LongIntegerDataType
// -----------------------------------------------------------------------------

// The following is to avoid a silly compile error with gcc3.2.
inline int64 DataSection_asInt64( DataSectionPtr pDS )
{
	return pDS->as< int64 >();
}

/**
 *	This template class is used to represent the different types of integer data
 *	type.
 *
 *	@ingroup entity
 */
template <class INT_TYPE>
class LongIntegerDataType : public DataType
{
	public:
		LongIntegerDataType( MetaDataType * pMeta ) : DataType( pMeta ) { }

	protected:
		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::isSameType
		 */
		virtual bool isSameType( PyObject * pValue )
		{
			INT_TYPE value;
			if (Script::setData( pValue, value ) == -1)
			{
				PyObject * pAsStr = PyObject_Str( pValue );
				if (pAsStr)
				{
					PyErr_Format( PyExc_TypeError, "%s is not a valid %s value",
						  PyString_AsString(pAsStr), this->typeName().c_str() );
					Py_DECREF( pAsStr );
				}
				PyErr_PrintEx(0);
				return false;
			}

			return true;
		}

		/**
		 *	This method sets the default value for this type.
		 *
		 *	@see DataType::setDefaultValue
		 */
		virtual void setDefaultValue( DataSectionPtr pSection )
		{
			pDefaultValue_ = (pSection) ? this->createFromSection( pSection ) :
								PyObjectPtr( PyLong_FromLong( 0 ),
										PyObjectPtr::STEAL_REFERENCE );
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::pDefaultValue
		 */
		virtual PyObjectPtr pDefaultValue() const
		{
			return pDefaultValue_;
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::addToStream
		 */
		virtual void addToStream( PyObject * pNewValue,
				BinaryOStream & stream, bool /*isPersistentOnly*/ ) const
		{
			// int64 value = PyLong_AsLongLong( pNewValue );
			INT_TYPE value = 0;
			int err = Script::setData( pNewValue, value );
			MF_ASSERT_DEV( err == 0 );
			stream << value;
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::createFromStream
		 */
		virtual PyObjectPtr createFromStream( BinaryIStream & stream,
			bool /*isPersistentOnly*/ ) const
		{
			INT_TYPE value;
			stream >> value;

			if (stream.error())
			{
				ERROR_MSG( "LongIntegerDataType::createFromStream: "
						   "Not enough data on stream to read value\n" );
				return NULL;
			}

			return PyObjectPtr( Script::getData( value ),
				PyObjectPtr::STEAL_REFERENCE );
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::addToSection
		 */
		virtual void addToSection( PyObject * pNewValue,
				DataSectionPtr pSection ) const
		{
			INT_TYPE value;
			Script::setData( pNewValue, value );
			pSection->be( int64(value) );
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::createFromSection
		 */
		virtual PyObjectPtr createFromSection( DataSectionPtr pSection ) const
		{
			INT_TYPE value = pSection->as<INT_TYPE>();
			return PyObjectPtr( Script::getData( INT_TYPE(value) ),
				PyObjectPtr::STEAL_REFERENCE );
		}

		virtual bool fromStreamToSection( BinaryIStream & stream,
				DataSectionPtr pSection, bool isPersistentOnly ) const
		{
			INT_TYPE value = 0;
			stream >> value;
			if (stream.error()) return false;

			// TODO: Should probably support uint32 version of be.
			pSection->be( int64(value) );
			return true;
		}

		virtual void addToMD5( MD5 & md5 ) const
		{
			if (std::numeric_limits<INT_TYPE>::is_signed)
				md5.append( "Int", sizeof( "Int" ) );
			else
				md5.append( "Uint", sizeof( "Uint" ) );
			int size = sizeof(INT_TYPE);
			md5.append( &size, sizeof(int) );
		}

		virtual bool operator<( const DataType & other ) const
		{
			if (this->DataType::operator<( other )) return true;
			if (other.DataType::operator<( *this )) return false;

			const LongIntegerDataType& otherInt =
				static_cast< const LongIntegerDataType& >( other );
			return (Script::compare( pDefaultValue_.getObject(),
				otherInt.pDefaultValue_.getObject() ) < 0);
		}

	private:
		PyObjectPtr pDefaultValue_;
};

SIMPLE_DATA_TYPE( LongIntegerDataType<uint32>, UINT32 )
SIMPLE_DATA_TYPE( LongIntegerDataType<int64>, INT64 )
SIMPLE_DATA_TYPE( LongIntegerDataType<uint64>, UINT64 )

// -----------------------------------------------------------------------------
// Section: FloatDataType
// -----------------------------------------------------------------------------

// TODO: Need to think about how to do float types properly. There is probably
// a few parameter to these types. E.g. How many bits are used, are they fixed
// point floats, and what are the range of values.

/**
 *	This class is used to represent a float data type.
 *
 *	@ingroup entity
 */
template <class FLOATTYPE>
class FloatDataType : public DataType
{
	public:
		FloatDataType( MetaDataType * pMeta ) : DataType( pMeta ) { }

	protected:
		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::isSameType
		 */
		virtual bool isSameType( PyObject * pValue )
		{
			return PyFloat_Check( pValue );
		}

		/**
		 *	This method sets the default value for this type.
		 *
		 *	@see DataType::setDefaultValue
		 */
		virtual void setDefaultValue( DataSectionPtr pSection )
		{
			pDefaultValue_ = (pSection) ? this->createFromSection( pSection ) :
								PyObjectPtr( PyFloat_FromDouble( 0.0 ),
										PyObjectPtr::STEAL_REFERENCE );
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::pDefaultValue
		 */
		virtual PyObjectPtr pDefaultValue() const
		{
			return pDefaultValue_;
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::addToStream
		 */
		virtual void addToStream( PyObject * pNewValue,
				BinaryOStream & stream, bool /*isPersistentOnly*/ ) const
		{
			FLOATTYPE value = (FLOATTYPE) PyFloat_AsDouble( pNewValue );

			stream << value;
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::createFromStream
		 */
		virtual PyObjectPtr createFromStream( BinaryIStream & stream,
			bool /*isPersistentOnly*/ ) const
		{
			FLOATTYPE value = 0.f;
			stream >> value;

			if (stream.error())
			{
				ERROR_MSG( "FloatDataType::createFromStream: "
						   "Not enough data on stream to read value\n" );
				return NULL;
			}

			return PyObjectPtr( PyFloat_FromDouble( (double)value ),
				PyObjectPtr::STEAL_REFERENCE );
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::addToSection
		 */
		virtual void addToSection( PyObject * pNewValue,
				DataSectionPtr pSection ) const
		{
			FLOATTYPE value = (FLOATTYPE) PyFloat_AsDouble( pNewValue );
			pSection->be< FLOATTYPE >( value );
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::createFromSection
		 */
		virtual PyObjectPtr createFromSection( DataSectionPtr pSection ) const
		{
			FLOATTYPE value = pSection->as< FLOATTYPE >();

			return PyObjectPtr( PyFloat_FromDouble( (double)value ),
				PyObjectPtr::STEAL_REFERENCE );
		}

		virtual bool fromStreamToSection( BinaryIStream & stream,
				DataSectionPtr pSection, bool isPersistentOnly ) const
		{
			IF_NOT_MF_ASSERT_DEV( pSection )
			{
				return false;
			}

			FLOATTYPE value = 0;
			stream >> value;
			if (stream.error()) return false;

			pSection->be< FLOATTYPE >( value );
			return true;
		}

		virtual bool fromSectionToStream( DataSectionPtr pSection,
					BinaryOStream & stream, bool /*isPersistentOnly*/ ) const
		{
			FLOATTYPE value = pSection->as< FLOATTYPE >();
			stream << value;
			return true;
		}

		virtual void addToMD5( MD5 & md5 ) const
		{
			// TODO: Make this more generic!
			if (sizeof(FLOATTYPE) == sizeof(float))
				md5.append( "Float", sizeof( "Float" ) );
			else
				md5.append( "Float64", sizeof( "Float64" ) );
		}

		virtual bool operator<( const DataType & other ) const
		{
			if (this->DataType::operator<( other )) return true;
			if (other.DataType::operator<( *this )) return false;

			const FloatDataType& otherFloat =
				static_cast< const FloatDataType& >( other );
			return (Script::compare( pDefaultValue_.getObject(),
				otherFloat.pDefaultValue_.getObject() ) < 0);
		}

	private:
		PyObjectPtr pDefaultValue_;
};

/// Datatype for floats.
SIMPLE_DATA_TYPE( FloatDataType< float >, FLOAT32 )
SIMPLE_DATA_TYPE( FloatDataType< double >, FLOAT64 )


// -----------------------------------------------------------------------------
// Section: StringDataType
// -----------------------------------------------------------------------------

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
bool StringDataType::isSameType( PyObject * pValue )
{
	return PyString_Check( pValue );
}

/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
void StringDataType::setDefaultValue( DataSectionPtr pSection )
{
	pDefaultValue_ = (pSection) ? this->createFromSection( pSection ) :
						PyObjectPtr( PyString_FromString( "" ),
								PyObjectPtr::STEAL_REFERENCE );
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::pDefaultValue
 */
PyObjectPtr StringDataType::pDefaultValue() const
{
	return pDefaultValue_;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToStream
 */
void StringDataType::addToStream( PyObject * pNewValue,
		BinaryOStream & stream, bool /*isPersistentOnly*/ ) const
{
	// This handles strings that are not null terminated and may contain
	// null characters.
	int size = PyString_Size( pNewValue );
	std::string newString( PyString_AsString( pNewValue ), size );
	stream << newString;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromStream
 */
PyObjectPtr StringDataType::createFromStream( BinaryIStream & stream,
		bool /*isPersistentOnly*/ ) const
{
	// This handles strings that are not null terminated and may contain
	// null characters.
	std::string value;
	stream >> value;

	return PyObjectPtr(
		PyString_FromStringAndSize( value.c_str(), value.length() ),
		PyObjectPtr::STEAL_REFERENCE );
}


/*
 *	Overrides the DataType method.
 */
void StringDataType::addToSection( PyObject * pNewValue,
		DataSectionPtr pSection ) const
{
	int size = PyString_Size( pNewValue );
	std::string newString( PyString_AsString( pNewValue ), size );
	pSection->setString( newString );
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
PyObjectPtr StringDataType::createFromSection( DataSectionPtr pSection) const
{
	std::string value;
	value = pSection->asString();
	return PyObjectPtr(
		PyString_FromStringAndSize( value.c_str(), value.length() ),
		PyObjectPtr::STEAL_REFERENCE );
}

bool StringDataType::fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool /*isPersistentOnly*/ ) const
{
	std::string value;
	stream >> value;
	if (stream.error()) return false;

	pSection->setString( value );
	return true;
}

bool StringDataType::fromSectionToStream( DataSectionPtr pSection,
			BinaryOStream & stream, bool /*isPersistentOnly*/ ) const
{
	stream << pSection->asString();
	return true;
}

void StringDataType::addToMD5( MD5 & md5 ) const
{
	md5.append( "String", sizeof( "String" ) );
}

bool StringDataType::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	const StringDataType& otherStr =
		static_cast< const StringDataType& >( other );
	return (Script::compare( pDefaultValue_.getObject(),
		otherStr.pDefaultValue_.getObject() ) < 0);
}

/// Datatype for strings.
SIMPLE_DATA_TYPE( StringDataType, STRING )



// -----------------------------------------------------------------------------
// Section: PythonDataType
// -----------------------------------------------------------------------------

/**
 *	This template class is used to represent the data type of any Python object.
 *	It is only meant to be temporary so that we can support any generic Python
 *	object/type.
 *
 *	@ingroup entity
 *	@todo Check whether or not we should have this data type.
 */
class PythonDataType : public DataType
{
	public:
		PythonDataType( MetaDataType * pMeta ) :
			DataType( pMeta, /*isConst:*/false )
		{}

	protected:
		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::isSameType
		 */
		virtual bool isSameType( PyObject * pValue )
		{
			// TODO: We shouldn't have to pickle the object twice.
			return !pickler().pickle( pValue ).empty();

			// This supports any Python object that can be pickled.
//			return true;
		}

		/**
		 *	This method sets the default value for this type.
		 *
		 *	@see DataType::setDefaultValue
		 */
		virtual void setDefaultValue( DataSectionPtr pSection )
		{
			pDefaultSection_ = pSection;
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::pDefaultValue
		 */
		virtual PyObjectPtr pDefaultValue() const
		{
			return (pDefaultSection_) ?
				this->createFromSection( pDefaultSection_ ) : Py_None;
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::pDefaultSection
		 */
		virtual DataSectionPtr pDefaultSection() const
		{
			return pDefaultSection_;
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::addToStream
		 */
		virtual void addToStream( PyObject * pNewValue,
				BinaryOStream & stream, bool /*isPersistentOnly*/ ) const
		{
			stream << pickler().pickle( pNewValue );
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::createFromStream
		 */
		virtual PyObjectPtr createFromStream( BinaryIStream & stream,
				bool /*isPersistentOnly*/ ) const
		{
			std::string value;
			stream >> value;

			// Check for zero length as unpickle doesn't work correctly and it will cause
			// a crash.
			if (stream.error() || (value.length() <= 0) )
			{
				ERROR_MSG( "PythonDataType::createFromStream: "
						   "Not enough data on stream to read value\n" );
				return NULL;
			}

			return PyObjectPtr( pickler().unpickle( value ),
				PyObjectPtr::STEAL_REFERENCE );
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::addToStream
		 */
		virtual void addToSection( PyObject * pNewValue,
				DataSectionPtr pSection ) const
		{
			pSection->setBlob( pickler().pickle( pNewValue ) );
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::createFromSection
		 */
		virtual PyObjectPtr createFromSection( DataSectionPtr pSection ) const
		{
			if (!pSection)
			{
				ERROR_MSG( "PythonDataType::createFromSection: "
					"pSection = NULL\n" );
				return NULL;
			}

			std::string value = pSection->asString();

			if (PythonDataType::isExpression( value ))
			{
				PyObject * pResult = Script::runString( value.c_str(), false );

				if (!pResult)
				{
					ERROR_MSG( "PythonDataType::createFromSection: "
						"Failed to evaluate '%s'\n", value.c_str() );
					PyErr_PrintEx(0);
				}

				return PyObjectPtr( pResult, PyObjectPtr::STEAL_REFERENCE );
			}
			else
			{
				return PyObjectPtr( pickler().unpickle( pSection->asBlob() ),
					PyObjectPtr::STEAL_REFERENCE );
			}
		}

		virtual bool fromStreamToSection( BinaryIStream & stream,
				DataSectionPtr pSection, bool isPersistentOnly ) const
		{
			std::string value;
			stream >> value;
			if (stream.error()) return false;

			pSection->setBlob( value );
			return true;
		}

		virtual bool fromSectionToStream( DataSectionPtr pSection,
							BinaryOStream & stream, bool isPersistentOnly ) const
		{
			if (!pSection)
			{
				return false;
			}

			std::string value = pSection->asBlob();

			stream.appendString( value.data(), value.length() );

			return true;
		}

		virtual void addToMD5( MD5 & md5 ) const
		{
			md5.append( "Python", sizeof( "Python" ) );
		}

		virtual bool operator<( const DataType & other ) const
		{
			if (this->DataType::operator<( other )) return true;
			if (other.DataType::operator<( *this )) return false;

			const PythonDataType& otherPy =
				static_cast< const PythonDataType& >( other );
			if (otherPy.pDefaultSection_)
				return otherPy.pDefaultSection_->compare( pDefaultSection_ ) > 0;
			// else we are equal or greater than other.
			return false;
		}

		static Pickler & pickler()
		{
			static Pickler pickler;

			return pickler;
		}

	public:
		/**
		 *	This function has a good guess as to whether the given value is
		 * 	a Python expression.
		 */
		static bool isExpression( const std::string& value )
		{
			// Since PYTHON values in DataSections can either be a Python
			// expression or a Base64 encoded pickled object, we return
			// true if it doesn't look like a Base64 encoded blob.
			int inputLen = value.length();
			return ((inputLen > 0) && (value[ inputLen - 1 ] != '='));
		}

	private:
		DataSectionPtr pDefaultSection_;
};

/// Datatype for strings.
SIMPLE_DATA_TYPE( PythonDataType, PYTHON )

bool PythonDataType_IsExpression( const std::string& value )
{
	return PythonDataType::isExpression( value );
}


// -----------------------------------------------------------------------------
// Section: VectorDataType
// -----------------------------------------------------------------------------

// This is used to implement some helper functions used to implement the
// VectorDataType template.

#define VECTOR_DATA_TYPE_HELPERS( VECTOR )									\
void fromSectionToVector( DataSectionPtr pSection, VECTOR & v )				\
{ v = pSection->as##VECTOR(); }												\
																			\
void fromVectorToSection( const VECTOR & v, DataSectionPtr pSection )		\
{ pSection->set##VECTOR( v ); }												\

VECTOR_DATA_TYPE_HELPERS( Vector2 )
VECTOR_DATA_TYPE_HELPERS( Vector3 )
VECTOR_DATA_TYPE_HELPERS( Vector4 )


/**
 *	This template class is used to represent the different types of vector data
 *	type.
 *
 *	@ingroup entity
 */
template <class VECTOR>
class VectorDataType : public DataType
{
	public:
		VectorDataType( MetaDataType * pMeta ) :
			DataType( pMeta, /*isConst:*/false )
		{}

	protected:
		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::isSameType
		 */
		virtual bool isSameType( PyObject * pValue )
		{
			if (PyVector< VECTOR >::Check( pValue ))
			{
				return true;
			}
			else if (PyTuple_Check( pValue ) &&
					PyTuple_Size( pValue ) == NUM_ELEMENTS)
			{
				float f;
				for (int i = 0; i < NUM_ELEMENTS; i++)
				{
					if (Script::setData( PyTuple_GetItem( pValue, i ), f ) != 0)
					{
						PyErr_Clear();
						return false;
					}
				}

				return true;
			}

			return false;
		}

		/**
		 *	This method sets the default value for this type.
		 *
		 *	@see DataType::setDefaultValue
		 */
		virtual void setDefaultValue( DataSectionPtr pSection )
		{
			if (pSection)
				::fromSectionToVector( pSection, defaultValue_ );
			else
				defaultValue_.setZero();
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::pDefaultValue
		 */
		virtual PyObjectPtr pDefaultValue() const
		{
			return PyObjectPtr( Script::getData( defaultValue_ ),
					PyObjectPtr::STEAL_REFERENCE );
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::addToStream
		 */
		virtual void addToStream( PyObject * pValue,
				BinaryOStream & stream, bool /*isPersistentOnly*/ ) const
		{
			if (PyVector< VECTOR >::Check( pValue ))
			{
				stream <<
					static_cast< PyVector< VECTOR > * >( pValue )->getVector();
			}
			else if (PyTuple_Check( pValue ) &&
					PyTuple_Size( pValue ) == NUM_ELEMENTS)
			{
				float f;
				for (int i = 0; i < NUM_ELEMENTS; i++)
				{
					if (Script::setData( PyTuple_GetItem( pValue, i ), f ) == 0)
					{
						stream << f;
					}
					else
					{
						CRITICAL_MSG( "Vector element was not a float "
							"after passing isSameType\n" );
					}
				}
			}
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::createFromStream
		 */
		virtual PyObjectPtr createFromStream( BinaryIStream & stream,
				bool /*isPersistentOnly*/ ) const
		{
			VECTOR value;
			stream >> value;

			if (stream.error())
			{
				ERROR_MSG( "VectorDataType::createFromStream: "
						   "Not enough data on stream to read value\n" );
				return NULL;
			}

			return PyObjectPtr( Script::getData( value ),
				PyObjectPtr::STEAL_REFERENCE );
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::addToSection
		 */
		virtual void addToSection( PyObject * pValue,
				DataSectionPtr pSection ) const
		{
			if (PyVector< VECTOR >::Check( pValue ))
			{
				::fromVectorToSection(
					static_cast< PyVector< VECTOR > * >( pValue )->getVector(),
					pSection );
			}
			else if (PyTuple_Check( pValue ) &&
					PyTuple_Size( pValue ) == NUM_ELEMENTS)
			{
				VECTOR v;
				for (int i = 0; i < NUM_ELEMENTS; i++)
				{
					if (Script::setData(
						PyTuple_GetItem( pValue, i ), v[i] ) != 0)
					{
						CRITICAL_MSG( "Vector element was not a float "
							"after passing isSameType\n" );
					}
				}
				::fromVectorToSection( v, pSection );
			}
		}

		/**
		 *	Overrides the DataType method.
		 *
		 *	@see DataType::createFromSection
		 */
		virtual PyObjectPtr createFromSection( DataSectionPtr pSection ) const
		{
			VECTOR value;
			::fromSectionToVector( pSection, value );

			return PyObjectPtr( Script::getData( value ),
				PyObjectPtr::STEAL_REFERENCE );
		}

		virtual bool fromStreamToSection( BinaryIStream & stream,
				DataSectionPtr pSection, bool isPersistentOnly ) const
		{
			VECTOR value;
			stream >> value;
			if (stream.error()) return false;

			::fromVectorToSection( value, pSection );
			return true;
		}

		virtual void addToMD5( MD5 & md5 ) const
		{
			md5.append( "Vector", sizeof( "Vector" ) );
			md5.append( &NUM_ELEMENTS, sizeof(int) );
		}

		virtual bool operator<( const DataType & other ) const
		{
			if (this->DataType::operator<( other )) return true;
			if (other.DataType::operator<( *this )) return false;

			const VectorDataType& otherVec =
				static_cast< const VectorDataType& >( other );
			return (defaultValue_ < otherVec.defaultValue_);
		}

	private:
		static const int NUM_ELEMENTS;

		VECTOR defaultValue_;
};

template <class VECTOR> const int VectorDataType< VECTOR >::NUM_ELEMENTS =
	sizeof( VECTOR )/sizeof( float );

SIMPLE_DATA_TYPE( VectorDataType< Vector2 >, VECTOR2 )
SIMPLE_DATA_TYPE( VectorDataType< Vector3 >, VECTOR3 )
SIMPLE_DATA_TYPE( VectorDataType< Vector4 >, VECTOR4 )


// -----------------------------------------------------------------------------
// Section: SequenceDataType
// -----------------------------------------------------------------------------

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
bool SequenceDataType::isSameType( PyObject * pValue )
{
	if (!PySequence_Check( pValue ))
	{
		ERROR_MSG( "SequenceDataType::isSameType: Not a sequence.\n" );
		if (pValue != NULL)
		{
			PyObject * pAsStr = PyObject_Str( pValue );

			if (pAsStr)
			{
				ERROR_MSG( "\tpValue = %s\n",
						PyString_AsString( pAsStr ) );
				Py_DECREF( pAsStr );
			}
		}

		return false;
	}

	int size = PySequence_Size( pValue );

	if ((size_ != 0) && (size != size_))
	{
		ERROR_MSG( "SequenceDataType::isSameType: "
				"Wrong size %d instead of %d.\n",
			size, size_ );
		return false;
	}

	for (int i = 0; i < size; i++)
	{
		PyObject * pElement = PySequence_GetItem( pValue, i );
		bool isOkay = elementType_.isSameType( pElement );

		Py_XDECREF( pElement );

		if (!isOkay)
		{
			ERROR_MSG( "SequenceDataType::isSameType: "
				"Bad element %d.\n", i );
			return false;
		}
	}

	return true;
}

/**
 *	Creates a default sequence i.e. empty sequence or sequence with correct
 * 	number of default elements if &lt;size&gt; is specified.
 */
PyObjectPtr SequenceDataType::createDefaultValue() const
{
	PyObjectPtr pSeq = this->newSequence( size_ );

	for (int i = 0; i < size_; ++i)
	{
		this->setItem( &*pSeq, i, elementType_.pDefaultValue() );
	}

	return pSeq;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToStream
 */
void SequenceDataType::addToStream( PyObject * pNewValue,
	BinaryOStream & stream, bool isPersistentOnly ) const
{
	int size = PySequence_Size( pNewValue );
	if (size_ == 0)
	{
		stream << size;
	}

	for (int i = 0; i < size; i++)
	{
		PyObject * pElement = PySequence_GetItem( pNewValue, i );

		elementType_.addToStream( pElement, stream, isPersistentOnly );

		Py_XDECREF( pElement );
	}
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromStream
 */
PyObjectPtr SequenceDataType::createFromStream( BinaryIStream & stream,
	bool isPersistentOnly ) const
{
	int size = size_;

	if (size == 0)
		stream >> size;

	// If they didn't even put a size on there, abort now
	if (stream.error())
	{
		ERROR_MSG( "SequenceDataType::createFromStream: "
				   "Missing size parameter on stream\n" );
		return NULL;
	}

	// Work out whether there's possibly enough data on the stream to
	// create a sequence of this size
	if (stream.remainingLength() < size || size < 0)
	{
		ERROR_MSG( "SequenceDataType::createFromStream: "
				   "Invalid size on stream: %d "
				   "(%d bytes remaining)\n",
				   size, stream.remainingLength() );
		stream.error( true );
		return NULL;
	}

	PyObjectPtr pList = this->newSequence( size );

	// If someone's asked for sequence that's too big, abort now
	IF_NOT_MF_ASSERT_DEV( pList )
	{
		stream.error( true );
		return NULL;
	}

	for (int i = 0; i < size; i++)
	{
		PyObjectPtr pElement =
				elementType_.createFromStream( stream, isPersistentOnly );
		if (pElement)
		{
			this->setItem( &*pList, i, pElement );
		}
		else
		{
			stream.error( true );
			break;
		}
	}

	// If at any point during that loop we ran out of data, we should
	// abort
	if (stream.error())
	{
		ERROR_MSG( "SequenceDataType::createFromStream: "
				   "Insufficient data on stream to create %d "
				   "elements\n", size );
		return NULL;
	}

	return pList;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
void SequenceDataType::addToSection( PyObject * pNewValue,
	DataSectionPtr pSection ) const
{
	int size = PySequence_Size( pNewValue );

	for (int i = 0; i < size; i++)
	{
		PyObject * pElement = PySequence_GetItem( pNewValue, i );

		DataSectionPtr itemSection = pSection->newSection( "item" );
		elementType_.addToSection( pElement, itemSection );

		Py_XDECREF( pElement );
	}
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
PyObjectPtr SequenceDataType::createFromSection( DataSectionPtr pSection ) const
{
	if (!pSection)
	{
		ERROR_MSG( "ArrayDataType::createFromSection: "
				"Section is NULL.\n" );
		return NULL;
	}

	int size = pSection->countChildren();
	int children = size;

	MF_ASSERT_DEV( (size_ == 0) || (size == size_) );
	if ((size_ != 0) && (size != size_))
	{
		size = size_;
	}

	PyObjectPtr pList = this->newSequence( size );

	for (int i = 0; i < size; i++)
	{
		PyObjectPtr pElement;

		if (i < children)
		{
			pElement = elementType_.createFromSection(
						pSection->openChild( i ) );
		}
		else
		{
			pElement = elementType_.pDefaultValue();
		}

		this->setItem( &*pList, i, pElement );
	}

	return pList;
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::fromStreamToSection
 */
bool SequenceDataType::fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool isPersistentOnly ) const
{
	int size = size_;

	if (size == 0)
	{
		stream >> size;
		if (stream.error()) return false;
	}

	std::vector< std::string > dummy;
	dummy.resize( size );
	pSection->writeStrings( "item", dummy );

	std::vector< DataSectionPtr > sections;
	pSection->openSections( "item", sections );

	bool ok = true;
	for (int i = 0; i < size && ok; i++)
	{
		ok &= elementType_.fromStreamToSection( stream, sections[ i ],
												isPersistentOnly );
	}
	return ok;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::fromSectionToStream
 */
bool SequenceDataType::fromSectionToStream( DataSectionPtr pSection,
		BinaryOStream & stream, bool isPersistentOnly ) const
{
	if (!pSection)
	{
		ERROR_MSG( "ArrayDataType::fromSectionToStream: "
				"Section is NULL.\n" );
		return false;
	}

	int size = pSection->countChildren();
	int children = size;

	MF_ASSERT_DEV( (size_ == 0) || (size == size_) );
	if (size_ == 0)
	{
		stream << size;
	}
	else if (size != size_)
	{
		size = size_;
	}

	for (int i = 0; i < size; i++)
	{
		if (i < children)
		{
			if ( !elementType_.fromSectionToStream(
							pSection->openChild( i ), stream, isPersistentOnly ) )
				return false;
		}
		else
		{
			elementType_.addToStream( &*elementType_.pDefaultValue(),
									stream, isPersistentOnly );
		}
	}

	return true;
}

void SequenceDataType::addToMD5( MD5 & md5 ) const
{
//			md5.append( "Array", sizeof( "Array" ) );
	md5.append( &size_, sizeof( size_ ) );
	elementType_.addToMD5( md5 );
}

bool SequenceDataType::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	// ok, equal metas, so downcast other and compare with us
	SequenceDataType & otherSeq = (SequenceDataType&)other;
	if (size_ < otherSeq.size_) return true;
	if (otherSeq.size_ < size_) return false;

	// equal lengths so now let our element types fight it out
	if (elementType_ < otherSeq.elementType_) return true;
	if (otherSeq.elementType_ < elementType_) return false;

	return this->compareDefaultValue( other ) < 0;
}

std::string SequenceDataType::typeName() const
{
	return this->DataType::typeName() +" of "+ elementType_.typeName();
}


// -----------------------------------------------------------------------------
// Section: ArrayDataType
// -----------------------------------------------------------------------------

class ArrayDataType;

/**
 *	This class is the Python object for instances of a mutable array.
 */
class PyArrayDataInstance : public PropertyOwner
{
	Py_Header( PyArrayDataInstance, PropertyOwner );

public:
	PyArrayDataInstance( const ArrayDataType * pDataType,
		int initialSize, PyTypePlus * pPyType = &s_type_ );
	~PyArrayDataInstance();

	void owned( PropertyOwnerBase * pNewOwner, int ownerRef )
		{ pOwner_ = pNewOwner; ownerRef_ = ownerRef; }
	void disowned()		{ pOwner_ = NULL; }
	bool hasOwner()		{ return pOwner_ != NULL; }

	void setInitialItem( int index, PyObjectPtr pItem );
	void setInitialSequence( PyObject * pSeq );

	const ArrayDataType * dataType() const			{ return &*pDataType_; }
	void setDataType( ArrayDataType * pDataType )	{ pDataType_ = pDataType; }

	virtual void propertyChanged( PyObjectPtr val, const DataType & type,
		ChangePath path );
	virtual int propertyDivisions();
	virtual PropertyOwnerBase * propertyVassal( int ref );
	virtual PyObjectPtr propertyRenovate( int ref, BinaryIStream & data,
		PyObjectPtr & pValue, DataType *& pType );


	PyObject * pyGetAttribute( const char * attr );
	int pySetAttribute( const char * attr, PyObject * value );

	PyObject * pyRepr();

	int strictSize() const;
	PY_RO_ATTRIBUTE_DECLARE( strictSize(), strictSize );


	PY_SIZE_INQUIRY_METHOD(			pySeq_length )			// len(x)
	PY_BINARY_FUNC_METHOD(			pySeq_concat )			// x + y
	PY_INTARG_FUNC_METHOD(			pySeq_repeat )			// x * n
	PY_INTARG_FUNC_METHOD(			pySeq_item )			// x[i]
	PY_INTINTARG_FUNC_METHOD(		pySeq_slice )			// x[i:j]
	PY_INTOBJARG_PROC_METHOD(		pySeq_ass_item )		// x[i] = v
	PY_INTINTOBJARG_PROC_METHOD(	pySeq_ass_slice )		// x[i:j] = v
	PY_OBJOBJ_PROC_METHOD(			pySeq_contains )		// v in x
	PY_BINARY_FUNC_METHOD(			pySeq_inplace_concat )	// x += y
	PY_INTARG_FUNC_METHOD(			pySeq_inplace_repeat )	// x *= n

	bool append( PyObjectPtr pObject );
	PY_AUTO_METHOD_DECLARE( RETOK, append, ARG( PyObjectPtr, END ) )

	int count( PyObjectPtr pObject );
	PY_AUTO_METHOD_DECLARE( RETDATA, count, ARG( PyObjectPtr, END ) )

	bool extend( PyObjectPtr pObject );
	PY_AUTO_METHOD_DECLARE( RETOK, extend, ARG( PyObjectPtr, END ) )

	PY_METHOD_DECLARE( py_index );

	bool insert( int before, PyObjectPtr pObject );
	PY_AUTO_METHOD_DECLARE( RETOK, insert, ARG( int, ARG( PyObjectPtr, END ) ) )

	PyObject * pop( int index );
	PY_AUTO_METHOD_DECLARE( RETOWN, pop, OPTARG( int, -1, END ) )

	bool remove( PyObjectPtr pObject );
	PY_AUTO_METHOD_DECLARE( RETOK, remove, ARG( PyObjectPtr, END ) )


	PY_PICKLING_METHOD_DECLARE( Array )

	static PyObject * PickleResolve( PyObject * list );
	PY_AUTO_UNPICKLING_FACTORY_DECLARE( ARG( PyObject *, END ), Array )

private:
	int findFrom( uint beg, PyObject * needle );


	ConstSmartPointer<ArrayDataType>	pDataType_;
	PropertyOwnerBase					* pOwner_;
	int									ownerRef_;

	std::vector<PyObjectPtr>			values_;

	static PySequenceMethods s_seq_methods;
};

typedef SmartPointer<PyArrayDataInstance> PyArrayDataInstancePtr;


/**
 *	This template class is used to represent the data type of an array object.
 *	There are only certain types of arrays that are currently supported.
 *
 *	@ingroup entity
 */
class ArrayDataType : public SequenceDataType
{
public:
	/**
	 *	Constructor.
	 *
	 *	@param pMeta	The MetaDataType associated with this DataType.
	 *	@param elementType	The data type of the elements of this array type.
	 *	@param size	The size of the array. If size is 0, the array is of
	 *				variable size.
	 */
	ArrayDataType( MetaDataType * pMeta, DataTypePtr elementType,
			int size = 0 ) :
		SequenceDataType( pMeta, elementType, size, /*isConst:*/false )
	{
	}

	static DataType * construct( MetaDataType * pMeta,
		DataTypePtr elementType, int size )
	{
		return new ArrayDataType( pMeta, elementType, size );
	}

protected:
	virtual PyObjectPtr newSequence( int size ) const
	{
		//return PyObjectPtr( PyList_New( size ), STEAL_REFERENCE );
		return PyObjectPtr( new PyArrayDataInstance( this, size ),
			PyObjectPtr::STEAL_REFERENCE );
	};

	virtual void setItem( PyObject * pSeq,
					int i, PyObjectPtr pElement ) const
	{
		//Py_INCREF( &*pElement );
		//PyList_SetItem( pSeq, i, pElement );	// steals a reference
		((PyArrayDataInstance*)pSeq)->setInitialItem( i, pElement );
	}

	virtual int compareDefaultValue( const DataType & other ) const
	{
		const ArrayDataType& otherArray =
			static_cast< const ArrayDataType& >( other );
		if (pDefaultSection_)
			return pDefaultSection_->compare( otherArray.pDefaultSection_ );
		return (otherArray.pDefaultSection_) ? -1 : 0;
	}

	virtual void setDefaultValue( DataSectionPtr pSection )
	{
		pDefaultSection_ = pSection;
	}

	virtual PyObjectPtr pDefaultValue() const
	{
		return (pDefaultSection_) ? this->createFromSection( pDefaultSection_ )
					: this->createDefaultValue();
	}

	virtual DataSectionPtr pDefaultSection() const
	{
		return pDefaultSection_;
	}

	/**
	 *	Attach to the given owner; or copy the object if we already have one
	 *	(or it is foreign and should be copied anyway).
	 */
	virtual PyObjectPtr attach( PyObject * pObject,
		PropertyOwnerBase * pOwner, int ownerRef )
	{
		if (!this->DataType::attach( pObject, pOwner, ownerRef )) return NULL;

		PyArrayDataInstancePtr pInst;

		// it's easy if it's the right python + entitydef type
		if (PyArrayDataInstance::Check( pObject ) &&
			((PyArrayDataInstance*)pObject)->dataType() == this)
		{
			pInst = (PyArrayDataInstance*)pObject;
			if (pInst->hasOwner())
			{	// note: up to caller to check that prop isn't being set back
				PyArrayDataInstance * pOldInst = &*pInst;	// into itself
				pInst = PyArrayDataInstancePtr( new PyArrayDataInstance( this,
					pOldInst->pySeq_length() ), pInst.STEAL_REFERENCE );
				pInst->setInitialSequence( pOldInst );
			}
		}
		// otherwise it must be a sequence with the correct types
		else	// (since base class method calls isSameType)
		{
			pInst = PyArrayDataInstancePtr( new PyArrayDataInstance( this,
				PySequence_Size( pObject ) ), pInst.STEAL_REFERENCE );
			pInst->setInitialSequence( pObject );
		}

		pInst->owned( pOwner, ownerRef );
		return pInst;
	}

	virtual void detach( PyObject * pObject )
	{
		IF_NOT_MF_ASSERT_DEV( PyArrayDataInstance::Check( pObject ) )
		{
			return;
		}
		((PyArrayDataInstance*)pObject)->disowned();
	}

	virtual PropertyOwnerBase * asOwner( PyObject * pObject )
	{
		IF_NOT_MF_ASSERT_DEV( PyArrayDataInstance::Check( pObject ) )
		{
			return NULL;
		}
		// __kyl__ (17/2/2006) The following assert is actually not true after
		// reloadScript. May be can restore it after updater is fully
		// operational.
		// MF_ASSERT( ((PyArrayDataInstance*)pObject)->dataType() == this );
		return (PyArrayDataInstance*)pObject;
	}

	virtual void addToMD5( MD5 & md5 ) const
	{
		md5.append( "Array", sizeof( "Array" ) );
		this->SequenceDataType::addToMD5( md5 );
	}

private:
	DataSectionPtr pDefaultSection_;
};





PY_TYPEOBJECT_WITH_SEQUENCE( PyArrayDataInstance,
	&PyArrayDataInstance::s_seq_methods )

PY_BEGIN_METHODS( PyArrayDataInstance )
	PY_METHOD( append );
	PY_METHOD( count );
	PY_METHOD( extend );
	PY_METHOD( index );
	PY_METHOD( insert );
	PY_METHOD( pop );
	PY_METHOD( remove );
	PY_PICKLING_METHOD()
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyArrayDataInstance )
	PY_ATTRIBUTE( strictSize );
PY_END_ATTRIBUTES()

PySequenceMethods PyArrayDataInstance::s_seq_methods =
{
	_pySeq_length,			// inquiry sq_length;				len(x)
	_pySeq_concat,			// binaryfunc sq_concat;			x + y
	_pySeq_repeat,			// intargfunc sq_repeat;			x * n
	_pySeq_item,			// intargfunc sq_item;				x[i]
	_pySeq_slice,			// intintargfunc sq_slice;			x[i:j]
	_pySeq_ass_item,		// intobjargproc sq_ass_item;		x[i] = v
	_pySeq_ass_slice,		// intintobjargproc sq_ass_slice;	x[i:j] = v
	_pySeq_contains,		// objobjproc sq_contains;			v in x
	_pySeq_inplace_concat,	// binaryfunc sq_inplace_concat;	x += y
	_pySeq_inplace_repeat	// intargfunc sq_inplace_repeat;	x *= n
};

PY_UNPICKLING_FACTORY( PyArrayDataInstance, Array )


/**
 *	Constructor.
 */
PyArrayDataInstance::PyArrayDataInstance( const ArrayDataType * pDataType,
		int initialSize, PyTypePlus * pPyType ) :
	PropertyOwner( pPyType ),
	pDataType_( pDataType ),
	pOwner_( NULL ),
	values_( initialSize )
{
}

/**
 *	Destructor
 */
PyArrayDataInstance::~PyArrayDataInstance()
{
	DataType & elemType = pDataType_->getElemType();
	for (uint i = 0; i < values_.size(); ++i)
		if (values_[i])
			elemType.detach( &*values_[i] );
}

/**
 *	Set one of the initial items that initialSize allocated space for.
 */
void PyArrayDataInstance::setInitialItem( int index, PyObjectPtr pItem )
{
	values_[index] = pDataType_->getElemType().attach( &*pItem, this, index );
}

/**
 *	Set all of the initial items to the given sequence which is guaranteed
 *	to be the correct size with the correct type elements.
 */
void PyArrayDataInstance::setInitialSequence( PyObject * pSeq )
{
	int sz = PySequence_Size( pSeq );
	IF_NOT_MF_ASSERT_DEV( pDataType_->getSize() == 0 || pDataType_->getSize() == sz )
	{
		return;
	}
	IF_NOT_MF_ASSERT_DEV( uint(sz) == values_.size() )
	{
		return;
	}

	DataType & elemType = pDataType_->getElemType();
	for (int i = 0; i < sz; ++i)
	{
		PyObject * pItem = PySequence_GetItem( pSeq, i );
		values_[i] = elemType.attach( pItem, this, i );
		Py_DECREF( pItem );
	}
}


/**
 *	One of our properties is telling us it's been changed internally.
 */
void PyArrayDataInstance::propertyChanged( PyObjectPtr val,
	const DataType & type, ChangePath path )
{
	if (pOwner_ != NULL)
	{
		path.push_back( ownerRef_ );
		pOwner_->propertyChanged( val, type, path );
	}
}

/**
 *	Someone wants to know how we have divided our property
 */
int PyArrayDataInstance::propertyDivisions()
{
	int size = pDataType_->getSize();
	return (size > 0) ? size : -1;
}

/**
 *	Someone wants to know if this element is an owner in its own right.
 */
PropertyOwnerBase * PyArrayDataInstance::propertyVassal( int ref )
{
	if (uint(ref) >= values_.size()) return NULL;
	return pDataType_->getElemType().asOwner( &*values_[ref] );
}

/**
 *	Someone wants us to change the value of this element.
 */
PyObjectPtr PyArrayDataInstance::propertyRenovate( int ref,
	BinaryIStream & data, PyObjectPtr & pRetValue, DataType *& pRetType )
{
	DataType & dataType = pDataType_->getElemType();
	PyObjectPtr pNewValue = dataType.createFromStream( data, false );
	if (!pNewValue)
		return NULL;

	PyObjectPtr & valRef = values_[ref];
	PyObjectPtr pOldValue = valRef;
	if (valRef != pNewValue)
	{
		// detach old value and attach new one
		dataType.detach( &*valRef );
		valRef = dataType.attach( &*pNewValue, this, ref );
	}

	pRetValue = valRef;
	pRetType = &dataType;
	return pOldValue;
}


/**
 *	Python get attribute method
 */
PyObject * PyArrayDataInstance::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();

	return PropertyOwner::pyGetAttribute( attr );
}


/**
 *	Python set attribute method
 */
int PyArrayDataInstance::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();

	return PropertyOwner::pySetAttribute( attr, value );
}


/**
 *	Get the representation of this object
 */
PyObject * PyArrayDataInstance::pyRepr()
{
	if (values_.empty()) return Script::getData( "[]" );

	std::string compo = "[";
	for (uint i = 0; i < values_.size(); ++i)
	{
		PyObject * pSubRepr = PyObject_Repr( &*values_[i] );
		if (pSubRepr == NULL) return NULL;

		if (i != 0) compo += ", ";
		compo += PyString_AsString( pSubRepr );
		Py_DECREF( pSubRepr );
	}
	return Script::getData( compo + "]" );
}

/**
 *	Helper accessor to return the strict size of this array, if one applies.
 */
int PyArrayDataInstance::strictSize() const
{
	return pDataType_->getSize();
}


/**
 *	Get length
 */
int PyArrayDataInstance::pySeq_length()
{
	return values_.size();
}

/**
 *	Concatenate ourselves with another sequence, returning a new sequence
 */
PyObject * PyArrayDataInstance::pySeq_concat( PyObject * pOther )
{
	if (!PySequence_Check( pOther ))
	{
		PyErr_SetString( PyExc_TypeError,
			"Array argument to + must be a sequence" );
		return NULL;
	}

	int szA = values_.size();
	int szB = PySequence_Size( pOther );
	PyObject * pList = PyList_New( szA + szB );

	for (int i = 0; i < szA; i++)
		PyList_SET_ITEM( pList, i, Script::getData( values_[i] ) );
	for (int i = 0; i < szB; i++)
		PyList_SET_ITEM( pList, szA + i, PySequence_GetItem( pOther, i ) );

	return pList;
}


/**
 *	Repeat ourselves a number of times, returning a new sequence
 */
PyObject * PyArrayDataInstance::pySeq_repeat( int n )
{
	if (n <= 0) return PyList_New( 0 );

	int sz = values_.size();

	PyObject * pList = PyList_New( sz * n );
	if (pList == NULL) return NULL;	// e.g. out of memory!

	// add the first repetition
	for (int j = 0; j < sz; j++)
	{
		PyList_SET_ITEM( pList, j, Script::getData( values_[j] ) );
	}

	// add the others (from the first lot)
	for (int i = 1; i < n; i++)
	{
		for (int j = 0; j < sz; j++)
		{
			PyObject * pTemp = PyList_GET_ITEM( pList, j );
			PyList_SET_ITEM( pList, i * sz + j, pTemp );
			Py_INCREF( pTemp );
		}
	}

	return pList;
}


/**
 *	Get the given item index
 */
PyObject * PyArrayDataInstance::pySeq_item( int index )
{
	if (uint(index) < values_.size())
		return Script::getData( values_[ index ] );

	PyErr_SetString( PyExc_IndexError, "Array index out of range" );
	return NULL;
}


/**
 *	Get the given slice
 */
PyObject * PyArrayDataInstance::pySeq_slice( int startIndex, int endIndex )
{
	if (startIndex < 0)
		startIndex = 0;

	if (endIndex > int(values_.size()))
		endIndex = values_.size();

	if (endIndex < startIndex)
		endIndex = startIndex;

	int length = endIndex - startIndex;

	if (length == int(values_.size())) return Script::getData( this );

	PyObject * pResult = PyList_New( length );
	for (int i = startIndex; i < endIndex; ++i)
		PyList_SET_ITEM( pResult, i-startIndex, Script::getData( values_[i] ) );
	// always make a detached list for partial copies...
	// alternative would be to make a new type that just points to us

	return pResult;
}


/**
 *	Swap the item currently at the given index with the given one.
 */
int PyArrayDataInstance::pySeq_ass_item( int index, PyObject * value )
{
	if (uint(index) >= values_.size())
	{
		PyErr_SetString( PyExc_IndexError,
				"Array assignment index out of range" );
		return -1;
	}

	DataType & dataType = pDataType_->getElemType();

	if (value)
	{
		if (!dataType.isSameType( value ))
		{
			PyErr_Format( PyExc_TypeError,
				"Array elements must be set to type %s (setting index %d)",
				dataType.typeName().c_str(), index );
			return -1;
		}

		PyObjectPtr & valRef = values_[index];
		// Allow propagation of non-const elements to that user can force
		// progation by assigning element back to itself.
		// __kyl__ (31/3/2006) Theoretically, we should exclude non-const
		// elements that already propagates changes to itself e.g. ARRAY,
		// CLASS, FIXED_DICT, but there is no convenient test for that.
		if ( (valRef != value) || !dataType.isConst() )
		{
			bool changed = !valRef || !dataType.isConst() ||
				PyObject_Compare( valRef.getObject(), value );
			// detach old value and attach new one
			dataType.detach( &*valRef );
			valRef = dataType.attach( value, this, index );
			// easy! now let our owner in on it
			if (pOwner_ != NULL && changed)
			{
				int pathHead[2] = { index, ownerRef_ };
				pOwner_->propertyChanged( valRef, dataType,
					ChangePath( pathHead, 2 ) );
			}
		}
	}
	else
	{
		if (pDataType_->getSize() != 0)
		{
			PyErr_Format( PyExc_TypeError, "Cannot delete elements from "
				"fixed sized PyArrayDataInstance" );
			return -1;
		}

		// Deleting this element
		PyObjectPtr & valRef = values_[index];
		dataType.detach( &*valRef );
		values_.erase( values_.begin() + index );
		// Notify owner of this change.
		if (pOwner_ != NULL)
		{
			int pathHead[1] = { ownerRef_ };	// say it all changed
			pOwner_->propertyChanged( this, *pDataType_, ChangePath( pathHead, 1 ));
		}
	}

	return 0;
}


/**
 *	Swap the slice defined by the given range with the given one.
 */
int PyArrayDataInstance::pySeq_ass_slice( int indexA, int indexB,
	PyObject * pOther )
{
	struct LocalHelperFunctions
	{
		void deleteElements( std::vector<PyObjectPtr>& values,
				int indexA, int indexB, DataType& elementType )
		{
			// only erase if there's something to erase
			if (indexA < indexB)	// this behaviour is the same as PyList's
			{
				for (int i = indexA; i < indexB; ++i)
					elementType.detach( &*values[i] );
				values.erase( values.begin()+indexA, values.begin()+indexB );
			}
		}
		void notifyOwnerOfChange( PropertyOwnerBase* pOwner,
				int ownerRef, PyObjectPtr val, const DataType& dataType )
		{
			if (pOwner != NULL)
			{
				int pathHead[1] = { ownerRef };	// say it all changed
				pOwner->propertyChanged( val, dataType,
						PropertyOwnerBase::ChangePath( pathHead, 1 ));
			}
		}
	} helper;

	// See if the slice is being removed
	if (!pOther)
	{
		if (pDataType_->getSize() != 0)
		{
			PyErr_Format( PyExc_TypeError, "Cannot delete elements from "
				"fixed sized PyArrayDataInstance" );
			return -1;
		}

		helper.deleteElements( values_, indexA, indexB,
				pDataType_->getElemType() );

		helper.notifyOwnerOfChange( pOwner_, ownerRef_, this, *pDataType_ );

		return 0;
	}

	// make sure we're setting it to a sequence
	if (!PySequence_Check( pOther ))
	{
		PyErr_Format( PyExc_TypeError,
			"PyArrayDataInstance slices can only be assigned to a sequence" );
		return -1;
	}

	if (pOther == this)
	{
		PyErr_Format( PyExc_TypeError, "PyArrayDataInstance does not support "
			"assignment of itself to a slice of itself" );
		return -1;
	}

	int sz = values_.size();
	int osz = PySequence_Size( pOther );

	// put indices in range (slices don't generate index errors)
	if (indexA > sz ) indexA = sz;
	if (indexA < 0) indexA = 0;
	if (indexB > sz ) indexB = sz;
	if (indexB < 0) indexB = 0;

	// make sure the sequnce will still be the right size
	if (pDataType_->getSize() != 0 && std::max(indexB-indexA,0) != osz)
	{
		PyErr_Format( PyExc_TypeError, "PyArrayDataInstance slice assignment "
			"would create array of wrong size" );
		return -1;
	}

	// make sure they are all the correct type
	DataType & dataType = pDataType_->getElemType();
	for (int i = 0; i < osz; ++i)
	{
		PyObject * pVal = PySequence_GetItem( pOther, i );
		bool ok = dataType.isSameType( pVal );
		Py_DECREF( pVal );
		if (!ok)
		{
			PyErr_Format( PyExc_TypeError,
				"Array elements must be set to type %s (setting slice %d-%d)",
				dataType.typeName().c_str(), indexA, indexB );
			return -1;
		}
	}

	helper.deleteElements( values_, indexA, indexB, dataType );

	// make room for the new elements
	values_.insert( values_.begin() + indexA, osz, PyObjectPtr() );

	// and set them in there
	for (int i = 0; i < osz; ++i)
	{
		PyObject * pTemp = PySequence_GetItem( pOther, i );
		MF_ASSERT_DEV( pTemp != NULL );
		values_[indexA+i] = dataType.attach( pTemp, this, indexA+i );
		Py_XDECREF( pTemp );
	}

	helper.notifyOwnerOfChange( pOwner_, ownerRef_, this, *pDataType_ );

	return 0;
}


/**
 *	See if the given object is in the sequence
 */
int PyArrayDataInstance::pySeq_contains( PyObject * pObj )
{
	return this->findFrom( 0, pObj ) >= 0;
}


/**
 *	Concatenate the given sequence to ourselves
 */
PyObject * PyArrayDataInstance::pySeq_inplace_concat( PyObject * pOther )
{
	// TODO: should probably just call
	//	pySeq_ass_slice( values_.size(), values_.size(). pOther );

	// check that we're dealing with a sequence
	if (!PySequence_Check( pOther ))
	{
		PyErr_SetString( PyExc_TypeError, "PyArrayDataInstance: "
			"Argument to += must be a sequence" );
		return NULL;
	}

	// get the sizes
	int szA = values_.size();
	int szB = PySequence_Size( pOther );

	if (szB == 0) return Script::getData( this );

	if (pDataType_->getSize() != 0)
	{
		PyErr_SetString( PyExc_TypeError, "PyArrayDataInstance "
			"concatenation with += would yield wrong size array" );
		return NULL;
	}

	// make sure they are all the correct type
	DataType & dataType = pDataType_->getElemType();
	for (int i = 0; i < szB; ++i)
	{
		PyObject * pVal = PySequence_GetItem( pOther, i );
		bool ok = dataType.isSameType( pVal );
		Py_DECREF( pVal );
		if (!ok)
		{
			PyErr_Format( PyExc_TypeError,
				"Array elements must be set to type %s (appending with +=)",
				dataType.typeName().c_str() );
			return NULL;
		}
	}

	// make room for the new values
	values_.insert( values_.end(), szB, PyObjectPtr() );

	// and concatenate away (it's fine if pOther == this here)
	for (int i = 0; i < szB; ++i)
	{
		PyObject * pTemp = PySequence_GetItem( pOther, i );
		MF_ASSERT_DEV( pTemp != NULL );
		values_[szA+i] = dataType.attach( pTemp, this, szA+i );
		Py_XDECREF( pTemp );
	}

	// tell our owner about this change
	if (pOwner_ != NULL)
	{
		int pathHead[1] = { ownerRef_ };	// say it all changed
		pOwner_->propertyChanged( this, *pDataType_, ChangePath( pathHead, 1 ));
	}

	return Script::getData( this );
}



/**
 *	Repeat ourselves a number of times
 */
PyObject * PyArrayDataInstance::pySeq_inplace_repeat( int n )
{
	if (n == 1) return Script::getData( this );

	if (pDataType_->getSize() != 0)
	{
		PyErr_SetString( PyExc_TypeError, "PyArrayDataInstance "
			"repetition with *= would yield wrong size array" );
		return NULL;
	}

	int sz = values_.size();

	DataType & dataType = pDataType_->getElemType();
	if (n <= 0)
	{
		for (uint i = 0; i < values_.size(); ++i)
			dataType.detach( &*values_[i] );
		values_.clear();
	}
	else
	{
		values_.insert( values_.end(), (n-1)*sz, PyObjectPtr() );

		for (int i = 1; i < n; ++i)
		{
			for (int j = 0; j < sz; ++j)
			{
				values_[i*sz+j] = dataType.attach( &*values_[j], this, i*sz+j );
			}	// that copy is coming into its own here!
		}
	}

	// tell our owner about this change
	if (pOwner_ != NULL)
	{
		int pathHead[1] = { ownerRef_ };	// say it all changed
		pOwner_->propertyChanged( this, *pDataType_, ChangePath( pathHead, 1 ));
	}

	return Script::getData( this );
}


/**
 *	Python method: append the given object
 */
bool PyArrayDataInstance::append( PyObjectPtr pObject )
{
	PyObjectPtr pTuple( PyTuple_New( 1 ), PyObjectPtr::STEAL_REFERENCE );
	PyTuple_SET_ITEM( &*pTuple, 0, Script::getData( pObject ) );

	return this->pySeq_ass_slice(
		values_.size(), values_.size(), &*pTuple ) == 0;
}

/**
 *	Python method: count the occurrences of the given object
 */
int PyArrayDataInstance::count( PyObjectPtr pObject )
{
	int count = 0, cur;
	for (uint i = 0; (cur = this->findFrom( i, &*pObject )) >= 0; i = cur+1)
		++count;
	return count;
}

/**
 *	Python method: append the given iterable
 */
bool PyArrayDataInstance::extend( PyObjectPtr pObject )
{
	return this->pySeq_ass_slice(
		values_.size(), values_.size(), &*pObject ) == 0;
}

/**
 *	Python method: find index of given value
 */
PyObject * PyArrayDataInstance::py_index( PyObject * args )
{
	PyObject * pObject = NULL;
	if (!PyArg_ParseTuple( args, "O", &pObject ))
		return NULL;
	int index = this->findFrom( 0, pObject );
	if (index == -1)
	{
		PyErr_SetString( PyExc_ValueError,
			"PyArrayDataInstance.index: value not found" );
		return NULL;
	}
	return Script::getData( index );
}

/**
 *	Python method: insert value at given location
 */
bool PyArrayDataInstance::insert( int before, PyObjectPtr pObject )
{
	PyObjectPtr pTuple( PyTuple_New( 1 ), PyObjectPtr::STEAL_REFERENCE );
	PyTuple_SET_ITEM( &*pTuple, 0, Script::getData( pObject ) );

	// insert shouldn't generate index errors, so ass_slice is perfect
	return this->pySeq_ass_slice( before, before, &*pTuple ) == 0;
}

/**
 *	Python method: pop the last element
 */
PyObject * PyArrayDataInstance::pop( int index )
{
	if (values_.empty())
	{
		PyErr_SetString( PyExc_IndexError,
			"PyArrayDataInstance.pop: empty array" );
		return NULL;
	}

	if (index < 0) index += values_.size();
	if (uint(index) >= values_.size())
	{
		PyErr_SetString( PyExc_IndexError,
			"PyArrayDataInstance.pop: index out of range" );
		return NULL;
	}

	PyObjectPtr pItem = values_[index];

	PyObjectPtr pTuple( PyTuple_New( 0 ), PyObjectPtr::STEAL_REFERENCE );
	if (this->pySeq_ass_slice( index, index+1, &*pTuple ) != 0)
		return NULL;

	Py_INCREF( &*pItem );
	return &*pItem;
}

/**
 *	Python method: remove first occurrence of given value
 */
bool PyArrayDataInstance::remove( PyObjectPtr pObject )
{
	int index = this->findFrom( 0, &*pObject );
	if (index == -1)
	{
		PyErr_SetString( PyExc_ValueError,
			"PyArrayDataInstance.remove: value not found" );
		return false;
	}

	PyObjectPtr pTuple( PyTuple_New( 0 ), PyObjectPtr::STEAL_REFERENCE );
	return this->pySeq_ass_slice( index, index+1, &*pTuple ) == 0;
}


/**
 *	Helper method to find an object at or after a given index
 */
int PyArrayDataInstance::findFrom( uint beg, PyObject * needle )
{
	for (uint i = beg; i < values_.size(); ++i)
		if (needle == &*values_[i]) return i;

	for (uint i = beg; i < values_.size(); ++i)
		if (PyObject_Compare( needle, &*values_[i] ) == 0) return i;

	return -1;
}

/**
 *	Reduce this array into something that can be serialised
 */
PyObject * PyArrayDataInstance::pyPickleReduce()
{
	PyObject * pList = PyList_New( values_.size() );
	for (uint i = 0; i < values_.size(); ++i)
		PyList_SET_ITEM( pList, i, Script::getData( values_[i] ) );

	PyObject * pArgs = PyTuple_New( 1 );
	PyTuple_SET_ITEM( pArgs, 0, pList );
	return pArgs;
}

/**
 *	Resolve this deserialised object into an array
 */
PyObject * PyArrayDataInstance::PickleResolve( PyObject * list )
{
	// since there's no good way to get back to the ArrayDataType,
	// we just return the list that the reduce method produced
	return list; // already incref'd by setData
}


// -----------------------------------------------------------------------------
// Section: TupleDataType
// -----------------------------------------------------------------------------

/**
 *	This template class is used to represent the data type of a tuple object.
 *	There are only certain types of tuples that are currently supported. Each
 *	element must be of the same type.
 *
 *	@ingroup entity
 */
class TupleDataType : public SequenceDataType
{
	public:
		/**
		 *	Constructor.
		 *
		 *	@param pMeta The meta type.
		 *	@param elementType The type of the tuple's elements.
		 *	@param size	The size of the tuple. If size is 0, the tuple is of
		 *				variable size.
		 */
		TupleDataType( MetaDataType * pMeta, DataTypePtr elementType,
				int size = 0 ) :
			SequenceDataType( pMeta, elementType, size, /*isConst:*/true )
		{
		}

		static DataType * construct( MetaDataType * pMeta,
			DataTypePtr elementType, int size )
		{
			return new TupleDataType( pMeta, elementType, size );
		}

	protected:
		virtual PyObjectPtr newSequence( int size ) const
		{
			return PyObjectPtr( PyTuple_New( size ),
				PyObjectPtr::STEAL_REFERENCE );
		};

		virtual void setItem( PyObject * pSeq,
						int i, PyObjectPtr pElement ) const
		{
			Py_INCREF( &*pElement );
			PyTuple_SetItem( pSeq, i, &*pElement );	// steals a reference
		}

		virtual int compareDefaultValue( const DataType & other ) const
		{
			const TupleDataType& otherTuple =
					static_cast< const TupleDataType& >( other );
			return Script::compare( pDefaultValue_.getObject(),
						otherTuple.pDefaultValue_.getObject() );
		}

		virtual void addToMD5( MD5 & md5 ) const
		{
			md5.append( "Tuple", sizeof( "Tuple" ) );
			this->SequenceDataType::addToMD5( md5 );
		}

		virtual void setDefaultValue( DataSectionPtr pSection )
		{
			pDefaultValue_ = (pSection) ? this->createFromSection( pSection ) :
									this->createDefaultValue();
		}

		virtual PyObjectPtr pDefaultValue() const
		{
			return pDefaultValue_;
		}

	private:
		PyObjectPtr pDefaultValue_;
};


// -----------------------------------------------------------------------------
// Section: SequenceMetaDataType
// -----------------------------------------------------------------------------

/**
 *	This class is used for objects that create sequence types.
 */
class SequenceMetaDataType : public MetaDataType
{
public:
	typedef DataType * (*SequenceTypeFactory)(
		MetaDataType * pMeta, DataTypePtr elementPtr , int size );

	SequenceMetaDataType( const char * name, SequenceTypeFactory factory ) :
		name_( name ),
		factory_( factory )
	{
		MetaDataType::addMetaType( this );
	}

	virtual ~SequenceMetaDataType()
	{
		MetaDataType::delMetaType( this );
	}

	virtual const char * name() const { return name_; }

	virtual DataTypePtr getType( DataSectionPtr pSection )
	{
		int size = pSection->readInt( "size", 0 );
		DataTypePtr pOfType =
			DataType::buildDataType( pSection->openSection( "of" ) );

		if (pOfType)
		{
			return (*factory_)( this, pOfType, size );
		}

		ERROR_MSG( "SequenceMetaDataType::getType: "
			"Unable to create sequence of '%s'\n",
				pSection->readString( "of" ).c_str() );

		return NULL;
	}

protected:
	const char * name_;
	SequenceTypeFactory factory_;
};


static SequenceMetaDataType s_ARRAY_metaDataType(
	"ARRAY", &ArrayDataType::construct );

static SequenceMetaDataType s_TUPLE_metaDataType(
	"TUPLE", &TupleDataType::construct );


// -----------------------------------------------------------------------------
// Section: ClassDataType and friends
// -----------------------------------------------------------------------------


class ClassDataType;

namespace
{
	const char * EMPTY_CLASS_NAME = "PickleableClass";	// was "EmptyClass"

	// This class handles the registration on deregistration of an empty
	// class object that we use for pickling/unpickling of PyClassDataInstance.
	class PyEmptyClassObject : public Script::InitTimeJob,
								public Script::FiniTimeJob
	{
		PyObjectPtr pEmptyClassObject_;

	public:
		PyEmptyClassObject() :
			Script::InitTimeJob( 1 ), Script::FiniTimeJob( 1 )
		{}

		virtual void init()
		{
			PyObjectPtr pDict( PyDict_New(), PyObjectPtr::STEAL_REFERENCE );
			MF_ASSERT_DEV( pDict );
			PyObjectPtr pModuleName( PyString_FromString( "_BWp" ),
									 PyObjectPtr::STEAL_REFERENCE );
			MF_VERIFY( PyDict_SetItemString( pDict.getObject(), "__module__",
											pModuleName.getObject() ) != -1 );
			PyObjectPtr pClassName( PyString_FromString( EMPTY_CLASS_NAME ),
									PyObjectPtr::STEAL_REFERENCE );
			MF_ASSERT_DEV( pClassName );
			pEmptyClassObject_ = PyObjectPtr(
				PyClass_New( NULL, pDict.getObject(), pClassName.getObject() ),
				PyObjectPtr::STEAL_REFERENCE );
			MF_VERIFY( PyObject_SetAttrString( PyImport_AddModule( "_BWp" ),
							const_cast<char*>(EMPTY_CLASS_NAME),
							pEmptyClassObject_.getObject() ) != -1 );
		}

		virtual void fini()
		{
			pEmptyClassObject_ = NULL;
		}

		PyObject* getObject()
		{
			MF_ASSERT_DEV( pEmptyClassObject_ );
			return pEmptyClassObject_.getObject();
		}
	};
}

/**
 *	This class is the Python object for instances of a CLASS.
 *	It monitors changes to its properties, and tells its owner when they
 *	change so that its owner can propagate the changes around.
 */
class PyClassDataInstance : public PropertyOwner
{
	Py_Header( PyClassDataInstance, PropertyOwner );

public:
	PyClassDataInstance( const ClassDataType * pDataType,
		PyTypePlus * pPyType = &s_type_ );
	~PyClassDataInstance();

	void owned( PropertyOwnerBase * pNewOwner, int ownerRef )
		{ pOwner_ = pNewOwner; ownerRef_ = ownerRef; }
	void disowned()		{ pOwner_ = NULL; }
	bool hasOwner()		{ return pOwner_ != NULL; }

	void setToDefault();
	void setToCopy( PyClassDataInstance & other );

	typedef PyObject * (*Interrogator)( PyObject * pObj, const char * prop );
	bool setToForeign( PyObject * pForeign, Interrogator interrogator );

#ifndef _MSC_VER
	#define FN(X) X
#else
#define FN(X) X##_FN
	static Interrogator FOREIGN_MAPPING, FOREIGN_ATTRS;
#endif
	static PyObject * FN(FOREIGN_MAPPING)( PyObject * pObj, const char * prop )
		{ return PyMapping_GetItemString( pObj, const_cast<char*>(prop) ); }
	static PyObject * FN(FOREIGN_ATTRS)( PyObject * pObj, const char * prop )
		{ return PyObject_GetAttrString( pObj, const_cast<char*>(prop) ); }


	PY_PICKLING_METHOD_DECLARE( Class )
	static PyObject * PickleResolve( PyObject * list );
	PY_AUTO_UNPICKLING_FACTORY_DECLARE( ARG( PyObject *, END ), Class )


	void setFieldValue( int index, PyObjectPtr val );
	PyObjectPtr getFieldValue( int index );


	const ClassDataType * dataType() const			{ return &*pDataType_; }
	void setDataType( ClassDataType * pDataType )	{ pDataType_ = pDataType; }


	virtual void propertyChanged( PyObjectPtr val, const DataType & dataType,
		ChangePath path );
	virtual int propertyDivisions();
	virtual PropertyOwnerBase * propertyVassal( int ref );
	virtual PyObjectPtr propertyRenovate( int ref, BinaryIStream & data,
		PyObjectPtr & pValue, DataType *& pType );


	PyObject * pyGetAttribute( const char * attr );
	int pySetAttribute( const char * attr, PyObject * value );

	PyObject * pyAdditionalMembers( PyObject * pBaseSeq );

private:
	ConstSmartPointer<ClassDataType>	pDataType_;
	PropertyOwnerBase					* pOwner_;
	int									ownerRef_;

	std::vector<PyObjectPtr>			fieldValues_;

	static PyEmptyClassObject			s_emptyClassObject_;
};

typedef SmartPointer<PyClassDataInstance> PyClassDataInstancePtr;

#ifdef _MSC_VER
#undef FN
PyClassDataInstance::Interrogator PyClassDataInstance::FOREIGN_MAPPING =
		PyClassDataInstance::FOREIGN_MAPPING_FN;
PyClassDataInstance::Interrogator PyClassDataInstance::FOREIGN_ATTRS =
		PyClassDataInstance::FOREIGN_ATTRS_FN;
#endif

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
bool ClassDataType::isSameType( PyObject * pValue )
{
	if (pValue == Py_None && allowNone_) return true;

	PyClassDataInstancePtr pInst;

	// check if an instance of our special Python class
	if (PyClassDataInstance::Check( pValue ) &&
		((PyClassDataInstance*)pValue)->dataType() == this)
	{
		return true;
	}
	else
	{
		// ok try to import it then
		pInst = PyClassDataInstancePtr( new PyClassDataInstance( this ),
			pInst.STEAL_REFERENCE );
		bool ok = pInst->setToForeign( pValue, PyMapping_Check( pValue ) ?
			pInst->FOREIGN_MAPPING : pInst->FOREIGN_ATTRS );
		if (!ok) PyErr_PrintEx(0);
		// throw it away and return (what a waste! maybe add a cache?...)
		return ok;
	}
}

/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
void ClassDataType::setDefaultValue( DataSectionPtr pSection )
{
	pDefaultSection_ = pSection;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::pDefaultValue
 */
PyObjectPtr ClassDataType::pDefaultValue() const
{
	if (pDefaultSection_)
		return this->createFromSection( pDefaultSection_ );

	if (allowNone_) return Py_None;

	PyClassDataInstance * pNewInst = new PyClassDataInstance( this );
	pNewInst->setToDefault();
	return PyObjectPtr( pNewInst, PyObjectPtr::STEAL_REFERENCE );
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToStream
 */
void ClassDataType::addToStream( PyObject * pValue,
	BinaryOStream & stream, bool isPersistentOnly ) const
{
	if (allowNone_)
	{
		if (pValue == Py_None)
		{
			stream << uint8(0);
			return;
		}
		stream << uint8(1);
	}

	PyClassDataInstancePtr pInst;
	if (PyClassDataInstance::Check( pValue ) &&
		((PyClassDataInstance*)pValue)->dataType() == this)
	{
		pInst = (PyClassDataInstance*)pValue;
	}
	else
	{
		pInst = PyClassDataInstancePtr( new PyClassDataInstance( this ),
			pInst.STEAL_REFERENCE );
		pInst->setToForeign( pValue, PyMapping_Check( pValue ) ?
			pInst->FOREIGN_MAPPING : pInst->FOREIGN_ATTRS );
	}

	for (Fields_iterator it = fields_.begin(); it != fields_.end(); ++it)
		it->type_->addToStream(	&*pInst->getFieldValue( it - fields_.begin() ),
								stream, isPersistentOnly );
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromStream
 */
PyObjectPtr ClassDataType::createFromStream( BinaryIStream & stream,
	bool isPersistentOnly ) const
{
	if (allowNone_)
	{
		uint8 hasValues;
		stream >> hasValues;

		if (stream.error())
		{
			ERROR_MSG( "ClassDataType::createFromStream: "
			   "Missing None/values indicator on stream\n" );
			return NULL;
		}

		if (!hasValues) return Py_None;
	}

	PyClassDataInstance * pInst = new PyClassDataInstance( this );

	for (Fields_iterator it = fields_.begin(); it != fields_.end(); ++it)
	{
		PyObjectPtr pValue = it->type_->createFromStream( stream,
															isPersistentOnly );
		if (!pValue) return NULL;	// error already printed

		pInst->setFieldValue( it - fields_.begin(), pValue );
	}

	return PyObjectPtr( pInst, PyObjectPtr::STEAL_REFERENCE );
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
void ClassDataType::addToSection( PyObject * pValue, DataSectionPtr pSection )
	const
{
	if (pValue == Py_None) return;	// leave as empty section

	PyClassDataInstancePtr pInst;
	if (PyClassDataInstance::Check( pValue ) &&
		((PyClassDataInstance*)pValue)->dataType() == this)
	{
		pInst = (PyClassDataInstance*)pValue;
	}
	else
	{
		pInst = PyClassDataInstancePtr( new PyClassDataInstance( this ),
			pInst.STEAL_REFERENCE );
		pInst->setToForeign( pValue, PyMapping_Check( pValue ) ?
			pInst->FOREIGN_MAPPING : pInst->FOREIGN_ATTRS );
	}

	for (Fields_iterator it = fields_.begin(); it != fields_.end(); ++it)
	{
		it->type_->addToSection(
			&*pInst->getFieldValue( it - fields_.begin() ),
			pSection->newSection( it->name_ ) );
	}
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
PyObjectPtr ClassDataType::createFromSection( DataSectionPtr pSection ) const
{
	if (!pSection)
	{
		ERROR_MSG( "ClassDataType::createFromSection: Section is NULL.\n" );
		return NULL;
	}

	if (allowNone_ && pSection->countChildren() == 0)
		return Py_None;


	PyClassDataInstance * pInst = new PyClassDataInstance( this );

	for (Fields_iterator it = fields_.begin(); it != fields_.end(); ++it)
	{
		DataSectionPtr pChildSection = pSection->openSection( it->name_ );
		if (pChildSection)
		{
			PyObjectPtr pValue = it->type_->createFromSection( pChildSection );
			if (!pValue) return NULL;	// error already printed

			pInst->setFieldValue( it - fields_.begin(), pValue );
		}
		else
		{
			pInst->setFieldValue( it - fields_.begin(),
								it->type_->pDefaultValue() );
		}
	}

	return PyObjectPtr( pInst, PyObjectPtr::STEAL_REFERENCE );
}


/**
 *	Attach to the given owner; or copy the object if we already have one
 *	(or it is foreign and should be copied anyway).
 */
PyObjectPtr ClassDataType::attach( PyObject * pObject,
	PropertyOwnerBase * pOwner, int ownerRef )
{
	//if (!this->DataType::attach( pObject, pOwner, ownerRef )) return NULL;
	// don't call the base class method as it's inefficient for foreign
	// types - it would convert them all twice!

	// if it's None and that's ok, just return that
	if (pObject == Py_None && allowNone_) return Py_None;


	PyClassDataInstancePtr pInst;

	// it's easy if it's the right python + entitydef type
	if (PyClassDataInstance::Check( pObject ) &&
		((PyClassDataInstance*)pObject)->dataType() == this)
	{
		pInst = (PyClassDataInstance*)pObject;
		if (pInst->hasOwner())
		{	// note: up to caller to check that prop isn't being set back
			PyClassDataInstance * pOldInst = &*pInst;	// into itself
			pInst = PyClassDataInstancePtr( new PyClassDataInstance( this ),
				pInst.STEAL_REFERENCE );
			pInst->setToCopy( *pOldInst );
		}
	}
	// otherwise just see if it has the same keys/attrs
	else
	{
		pInst = PyClassDataInstancePtr( new PyClassDataInstance( this ),
			pInst.STEAL_REFERENCE );
		if (!pInst->setToForeign( pObject, PyMapping_Check( pObject ) ?
			pInst->FOREIGN_MAPPING : pInst->FOREIGN_ATTRS ))
		{
			PyErr_PrintEx(0);
			return NULL;
		}
	}

	pInst->owned( pOwner, ownerRef );
	return pInst;
}

void ClassDataType::detach( PyObject * pObject )
{
	if (pObject == Py_None) return;

	IF_NOT_MF_ASSERT_DEV( PyClassDataInstance::Check( pObject ) )
	{
		return;
	}

	((PyClassDataInstance*)pObject)->disowned();
}

PropertyOwnerBase * ClassDataType::asOwner( PyObject * pObject )
{
	if (!PyClassDataInstance::Check( pObject )) return NULL; // for None

	// __kyl__ (17/2/2006) The following assert is actually not true after
	// reloadScript. May be can restore it after updater is fully operational.
	// MF_ASSERT( ((PyClassDataInstance*)pObject)->dataType() == this );
	return (PyClassDataInstance*)pObject;
}


void ClassDataType::addToMD5( MD5 & md5 ) const
{
	md5.append( "Class", sizeof( "Class" ) );
	md5.append( &allowNone_, sizeof(allowNone_) );
	for (Fields_iterator it = fields_.begin(); it != fields_.end(); ++it)
	{
		md5.append( it->name_.data(), it->name_.size() );
		it->type_->addToMD5( md5 );
	}
}

bool ClassDataType::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	// ok, equal metas, so downcast other and compare with us
	ClassDataType & otherCl= (ClassDataType&)other;
	if (allowNone_ != otherCl.allowNone_)
		return !allowNone_;	// we are less if our allowNone is false

	if (fields_.size() < otherCl.fields_.size()) return true;
	if (otherCl.fields_.size() < fields_.size()) return false;

	for (uint i = 0; i < fields_.size(); ++i)
	{
		int res = fields_[i].name_.compare( otherCl.fields_[i].name_ );
		if (res < 0) return true;
		if (res > 0) return false;

		if ((*fields_[i].type_) < (*otherCl.fields_[i].type_)) return true;
		if ((*otherCl.fields_[i].type_) < (*fields_[i].type_)) return false;
	}

	if (otherCl.pDefaultSection_)
		return otherCl.pDefaultSection_->compare( pDefaultSection_ ) > 0;
	// else we are equal or greater than other.
	return false;
}

std::string ClassDataType::typeName() const
{
	std::string acc = this->DataType::typeName();
	if (allowNone_) acc += " [None Allowed]";
	for (Fields_iterator it = fields_.begin(); it != fields_.end(); ++it)
	{
		acc += (it == fields_.begin()) ? " props " : ", ";
		acc += it->name_ + ":(" + it->type_->typeName() + ")";
	}
	return acc;
}

/**
 *	This class implements an object that creates class types with properties
 *	like an entity.
 */
class ClassMetaDataType : public MetaDataType
{
public:
	typedef ClassDataType DataType;

	ClassMetaDataType()
	{
		MetaDataType::addMetaType( this );
   	}
	virtual ~ClassMetaDataType()
	{
		MetaDataType::delMetaType( this );
	}

protected:

	virtual const char * name()	const { return "CLASS"; }

	virtual DataTypePtr getType( DataSectionPtr pSection )
	{
		return ClassMetaDataType::buildType( pSection, *this );
	}

public:
	// This method is used to build both ClassDataType and FixedDictDataType
	template <class METADATATYPE>
	static typename METADATATYPE::DataType* buildType( DataSectionPtr pSection,
		METADATATYPE& metaDataType )
	{
		typename METADATATYPE::DataType::Fields fields;

		DataSectionPtr pParentSect = pSection->openSection( "Parent" );
		if (pParentSect)
		{
			DataTypePtr pParent = DataType::buildDataType( pParentSect );
			if (!pParent || pParent->pMetaDataType() != &metaDataType)
			{
				ERROR_MSG( "<Parent> of %s must also be a %s\n",
							metaDataType.name(), metaDataType.name() );
				return NULL;
			}

			typename METADATATYPE::DataType * pParentClass =
				static_cast<typename METADATATYPE::DataType*>(&*pParent);

			// just rip its fields out for now
			fields = pParentClass->getFields();
		}

		DataSectionPtr pProps = pSection->openSection( "Properties" );
		if ((!pProps || pProps->countChildren() == 0) && fields.empty())
		{
			ERROR_MSG( "<Properties> section missing or empty in %s type spec\n",
						metaDataType.name() );
			return NULL;
		}

		for (DataSection::iterator it = pProps->begin();
			it != pProps->end();
			++it)
		{
			typename METADATATYPE::DataType::Field f;
			f.name_ = (*it)->sectionName();

			f.type_ = DataType::buildDataType( (*it)->openSection( "Type" ) );
			if (!f.type_)
			{
				ERROR_MSG( "Could not build type for property '%s' in %s type\n",
					f.name_.c_str(), metaDataType.name() );
				return NULL;
			}

			f.dbLen_ =
					(*it)->readInt( "DatabaseLength", DEFAULT_DATABASE_LENGTH );
			f.isPersistent_ = (*it)->readBool( "Persistent", true );

			fields.push_back( f );
		}

		// should have got an error if fields are empty before here
		MF_ASSERT_DEV( !fields.empty() );

		bool allowNone = pSection->readBool( "AllowNone", false );

		return new typename METADATATYPE::DataType( &metaDataType, fields,
														allowNone );
	}
};

static ClassMetaDataType s_CLASS_metaDataType;

// -----------------------------------------------------------------------------
// Section: PyClassDataInstance implementation
// -----------------------------------------------------------------------------

// These are out-of-line because we need to use ClassDataType which is still
// undeclared during our declaration.

PyEmptyClassObject PyClassDataInstance::s_emptyClassObject_;

PY_TYPEOBJECT( PyClassDataInstance )

PY_BEGIN_METHODS( PyClassDataInstance )
	PY_PICKLING_METHOD()
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyClassDataInstance )
PY_END_ATTRIBUTES()

PY_UNPICKLING_FACTORY( PyClassDataInstance, Class )


/**
 *	Constructor for PyClassDataInstance.
 */
PyClassDataInstance::PyClassDataInstance( const ClassDataType * pDataType,
		PyTypePlus * pPyType ) :
	PropertyOwner( pPyType ),
	pDataType_( pDataType ),
	pOwner_( NULL ),
	fieldValues_( pDataType->fields().size() )
{
}

/**
 *	Destructor for PyClassDataInstance
 */
PyClassDataInstance::~PyClassDataInstance()
{
	for (uint i = 0; i < fieldValues_.size(); ++i)
		if (fieldValues_[i])
			pDataType_->fields()[i].type_->detach( &*fieldValues_[i] );
}

/**
 *	Set to the default values for our data types (from blank)
 *	(Note: no defaults in our type specs... that could be good too)
 */
void PyClassDataInstance::setToDefault()
{
	uint fieldCount = fieldValues_.size();
	for (uint i = 0; i < fieldCount; ++i)
	{
		DataType & dt = *pDataType_->fields()[i].type_;
		fieldValues_[i] = dt.attach( &*dt.pDefaultValue(), this, i );
		MF_ASSERT_DEV( fieldValues_[i] );
	}
}

/**
 *	Make a copy of the given object (from blank).
 */
void PyClassDataInstance::setToCopy( PyClassDataInstance & other )
{
	IF_NOT_MF_ASSERT_DEV( pDataType_ == other.pDataType_ )
	{
		return;
	}

	uint fieldCount = fieldValues_.size();
	for (uint i = 0; i < fieldCount; ++i)
	{
		fieldValues_[i] = pDataType_->fields()[i].type_->attach(
			&*other.fieldValues_[i], this, i );
		MF_ASSERT_DEV( fieldValues_[i] );
	}
}

/**
 *	Attempt to set to the given foreign object (from blank).
 *	Returns false and sets a Python error on failure.
 */
bool PyClassDataInstance::setToForeign( PyObject * pForeign,
	Interrogator interrogator )
{
	uint fieldCount = fieldValues_.size();
	for (uint i = 0; i < fieldCount; ++i)
	{
		PyObject * gotcha = interrogator( pForeign,
			pDataType_->fields()[i].name_.c_str() );
		if (!gotcha) return false;			// leave python error there

		fieldValues_[i] = pDataType_->fields()[i].type_->attach(
			gotcha, this, i );
		Py_DECREF( gotcha );

		if (!fieldValues_[i])
		{
			PyErr_Format( PyExc_TypeError, "Class::setToForeign: "
				"Foreign object elt has wrong data type for prop '%s'",
				pDataType_->fields()[i].name_.c_str() );
			return false;
		}
	}
	return true;
}

/**
 *	Set the value of an uninitialised field.
 */
void PyClassDataInstance::setFieldValue( int index, PyObjectPtr val )
{
	fieldValues_[index] =
		pDataType_->fields()[index].type_->attach( &*val, this, index );
}

/**
 *	Get the value of a field.
 */
PyObjectPtr PyClassDataInstance::getFieldValue( int index )
{
	return fieldValues_[index];
}


/**
 *	One of our properties is telling us it's been changed internally.
 */
void PyClassDataInstance::propertyChanged( PyObjectPtr val,
	const DataType & type, ChangePath path )
{
	if (pOwner_ != NULL)
	{
		path.push_back( ownerRef_ );
		pOwner_->propertyChanged( val, type, path );
	}
}

/**
 *	Someone wants to know how we have divided our property
 */
int PyClassDataInstance::propertyDivisions()
{
	return fieldValues_.size();
}

/**
 *	Someone wants to know if this property is an owner in its own right.
 */
PropertyOwnerBase * PyClassDataInstance::propertyVassal( int ref )
{
	if (uint(ref) >= fieldValues_.size()) return NULL;
	return pDataType_->fields()[ref].type_->asOwner( &*fieldValues_[ref] );
}

/**
 *	Someone wants us to change the value of this property.
 */
PyObjectPtr PyClassDataInstance::propertyRenovate( int ref,
	BinaryIStream & data, PyObjectPtr & pRetValue, DataType *& pRetType )
{
	DataType & dataType = *pDataType_->fields()[ref].type_;
	PyObjectPtr pNewValue = dataType.createFromStream( data, false );
	if (!pNewValue)
		return NULL;

	PyObjectPtr & valRef = fieldValues_[ref];
	PyObjectPtr pOldValue = valRef;
	if (valRef != pNewValue)
	{
		// detach old value and attach new one
		dataType.detach( &*valRef );
		valRef = dataType.attach( &*pNewValue, this, ref );
	}

	pRetValue = valRef;
	pRetType = &dataType;
	return pOldValue;
}


/**
 *	Python get attribute method
 */
PyObject * PyClassDataInstance::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();

	int index = pDataType_->fieldIndexFromName( attr );
	if (index >= 0)
	{
		return Script::getData( fieldValues_[index] );
	}

	return PropertyOwner::pyGetAttribute( attr );
}


/**
 *	Python set attribute method
 */
int PyClassDataInstance::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();

	int index = pDataType_->fieldIndexFromName( attr );
	if (index >= 0)
	{
		DataType & dataType = *pDataType_->fields()[index].type_;
		if (!dataType.isSameType( value ))
		{
			PyErr_Format( PyExc_TypeError,
				"Class property %s must be set to type %s",
				attr, dataType.typeName().c_str() );
			return -1;
		}

		PyObjectPtr & valRef = fieldValues_[index];
		if (valRef != value)
		{
			bool changed = !valRef || !dataType.isConst() || 
				PyObject_Compare( valRef.getObject(), value );
			// detach old value and attach new one
			dataType.detach( &*valRef );
			valRef = dataType.attach( value, this, index );
			// easy! now let our owner in on it
			if (pOwner_ != NULL && changed)
			{
				int pathHead[2] = { index, ownerRef_ };
				pOwner_->propertyChanged( valRef, dataType,
					ChangePath( pathHead, 2 ) );
			}
		}
		return 0;
	}

	return PropertyOwner::pySetAttribute( attr, value );
}

/**
 *	Return additional members for dir.
 */
PyObject * PyClassDataInstance::pyAdditionalMembers( PyObject * pBaseSeq )
{
	PyObject * pTuple = PyTuple_New( PySequence_Size( pBaseSeq ) +
			pDataType_->fields().size() );
	uint i;
	for (i = 0; i < (uint)PySequence_Size( pBaseSeq ); ++i)
	{
		PyTuple_SET_ITEM( pTuple, i, PySequence_GetItem( pBaseSeq, i ) );
	}
	for (ClassDataType::Fields_iterator it = pDataType_->fields().begin();
		it != pDataType_->fields().end();
		++it, ++i)
	{
		PyTuple_SET_ITEM( pTuple, i, Script::getData( it->name_ ) );
	}

	Py_DECREF( pBaseSeq );
	return pTuple;
}

/**
 *	Python pickling helper. Returns the second element of the tuple expected of
 * 	Python's __reduce__ method. Declared in PY_PICKLING_METHOD_DECLARE.
 */
PyObject * PyClassDataInstance::pyPickleReduce()
{
	// Make an equivalent pickle-able Python class instance.
	const ClassDataType::Fields& fields = pDataType_->getFields();
	PyObject* pClassInstance =
		PyInstance_New( s_emptyClassObject_.getObject(), NULL, NULL );
	for ( ClassDataType::Fields::size_type i = 0; i < fields.size(); ++i )
	{
		PyObject_SetAttrString( pClassInstance,
								const_cast<char*>(fields[i].name_.c_str()),
								fieldValues_[i].getObject() );
	}

	// Make a tuple (method arguments for PickleResolve())
	PyObject * pArgs = PyTuple_New( 1 );
	PyTuple_SET_ITEM( pArgs, 0, pClassInstance );

	return pArgs;
}

/**
 *	Python unpickling helper. Creates a PyClassDataInstance from the
 *	information returned by PyClassDataInstance::pyPickleReduce().
 * 	Required by PY_AUTO_UNPICKLING_FACTORY_DECLARE.
 */
PyObject * PyClassDataInstance::PickleResolve( PyObject * pClassInstance )
{
	// __kyl__ (1/3/2006) Currently doesn't return a PyClassDataInstance
	// since it's too hard to find the right ClassDataType for pDataType_.
	// We'd also need pOwner_ and ownerRef_ and re-attach
	// PyClassDataInstance to the right entity here.

	// Simply return the equivalent class instance for now. pClassInstance
	// already increfed by PY_AUTO_UNPICKLING_FACTORY_DECLARE
	return pClassInstance;
}


// -----------------------------------------------------------------------------
// Section: FixedDictDataType and friends
// -----------------------------------------------------------------------------

/**
 *	This class is the Python object for instances of a FIXED_DICT.
 *	It monitors changes to its properties, and tells its owner when they
 *	change so that its owner can propagate the changes around.
 */
class PyFixedDictDataInstance : public PropertyOwner
{
	Py_Header( PyFixedDictDataInstance, PropertyOwner );

public:
	PyFixedDictDataInstance( FixedDictDataType* pDataType );
	PyFixedDictDataInstance( FixedDictDataType* pDataType,
		const PyFixedDictDataInstance& other );
	~PyFixedDictDataInstance();

	void setOwner( PropertyOwnerBase * pNewOwner, int ownerRef )
	{ pOwner_ = pNewOwner; ownerRef_ = ownerRef; }
	void disowned()		{ pOwner_ = NULL; }
	bool hasOwner()		{ return pOwner_ != NULL; }

	void initFieldValue( int index, PyObjectPtr val )
	{
		fieldValues_[index] =
			pDataType_->getFieldDataType(index).attach( val.get(), this, index );
	}

	PyObjectPtr getFieldValue( int index )	{ return fieldValues_[index]; }

	const FixedDictDataType& dataType() const		{ return *pDataType_; }
	void setDataType( FixedDictDataType* pDataType ){ pDataType_ = pDataType; }

	static bool isSameType( PyObject * pValue,
			const FixedDictDataType& dataType )
	{
		return PyFixedDictDataInstance::Check( pValue ) &&
			(&static_cast<PyFixedDictDataInstance*>(pValue)->dataType() == &dataType);
	}

	// PropertyOwner overrides
	virtual void propertyChanged( PyObjectPtr val, const DataType & dataType,
		ChangePath path );
	virtual int propertyDivisions();
	virtual PropertyOwnerBase * propertyVassal( int ref );
	virtual PyObjectPtr propertyRenovate( int ref, BinaryIStream & data,
		PyObjectPtr & pValue, DataType *& pType );

	// PyObjectPlus framework
	PyObject * pyGetAttribute( const char * attr );
	int pySetAttribute( const char * attr, PyObject * value );

	PY_PICKLING_METHOD_DECLARE( FixedDict )
	static PyObject * PickleResolve( PyObject * list );
	PY_AUTO_UNPICKLING_FACTORY_DECLARE( ARG( PyObject *, END ), FixedDict )

	// Python Mapping interface functions
	PY_METHOD_DECLARE( py_has_key )
	PY_METHOD_DECLARE( py_keys )
	PY_METHOD_DECLARE( py_values )
	PY_METHOD_DECLARE( py_items )

	static Py_ssize_t pyGetLength( PyObject* self );
	static PyObject* pyGetFieldByKey( PyObject* self, PyObject* key );
	static int pySetFieldByKey( PyObject* self, PyObject* key, PyObject* value );

	int getNumFields() const	{ return int(fieldValues_.size()); }

	PyObject* getFieldByKey( PyObject * key );
	PyObject* getFieldByKey( const char * keyString );

	int setFieldByKey( PyObject * key, PyObject * value );
	int setFieldByKey( const char * key, PyObject * value );

private:
	SmartPointer<FixedDictDataType>	pDataType_;

	PropertyOwnerBase			* pOwner_;
	int							ownerRef_;

	typedef	std::vector<PyObjectPtr> FieldValues;
	FieldValues					fieldValues_;
};

typedef SmartPointer<PyFixedDictDataInstance> PyFixedDictDataInstancePtr;

static PyMappingMethods g_fixedDictMappingMethods =
{
	PyFixedDictDataInstance::pyGetLength,		// mp_length
	PyFixedDictDataInstance::pyGetFieldByKey,	// mp_subscript
	PyFixedDictDataInstance::pySetFieldByKey	// mp_ass_subscript
};

PY_TYPEOBJECT_WITH_MAPPING( PyFixedDictDataInstance, &g_fixedDictMappingMethods )

PY_BEGIN_METHODS( PyFixedDictDataInstance )
	PY_METHOD( has_key )
	PY_METHOD( keys )
	PY_METHOD( items )
	PY_METHOD( values )
	PY_PICKLING_METHOD()
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PyFixedDictDataInstance )
PY_END_ATTRIBUTES()

PY_UNPICKLING_FACTORY( PyFixedDictDataInstance, FixedDict )

/**
 *	Default constructor. Field values are NULL.
 */
PyFixedDictDataInstance::PyFixedDictDataInstance(
		FixedDictDataType* pDataType ) :
	PropertyOwner( &s_type_ ),
	pDataType_( pDataType ),
	pOwner_( NULL ),
	fieldValues_( pDataType->getNumFields() )
{}

/**
 *	Copy constructor...sort of.
 */
PyFixedDictDataInstance::PyFixedDictDataInstance(
		FixedDictDataType* pDataType,
		const PyFixedDictDataInstance& other ) :
	PropertyOwner( &s_type_ ),
	pDataType_( pDataType ),
	pOwner_( NULL ),
	fieldValues_( pDataType->getNumFields() )
{
	uint fieldCount = fieldValues_.size();
	for (uint i = 0; i < fieldCount; ++i)
	{
		this->initFieldValue( i, other.fieldValues_[i] );
		MF_ASSERT_DEV( fieldValues_[i] );
	}
}

/**
 *	Destructor
 */
PyFixedDictDataInstance::~PyFixedDictDataInstance()
{
	for (uint i = 0; i < fieldValues_.size(); ++i)
		if (fieldValues_[i])
			pDataType_->getFieldDataType(i).detach( &*fieldValues_[i] );
}

/**
 *	One of our properties is telling us it's been changed internally.
 */
void PyFixedDictDataInstance::propertyChanged( PyObjectPtr val,
	const DataType & type, ChangePath path )
{
	// NOTE: After finalisation we may not be detached properly from our owner
	// (see FixedDictDataType::detach()) if we are wrapped by a custom type that
	// keeps a reference to us. That's why we should avoid propagating a
	// change to our owner which may already be destroyed.
	if ((pOwner_ != NULL) && Py_IsInitialized())
	{
		path.push_back( ownerRef_ );
		pOwner_->propertyChanged( val, type, path );
	}
}

/**
 *	Someone wants to know how we have divided our property
 */
int PyFixedDictDataInstance::propertyDivisions()
{
	return fieldValues_.size();
}

/**
 *	Someone wants to know if this property is an owner in its own right.
 */
PropertyOwnerBase * PyFixedDictDataInstance::propertyVassal( int ref )
{
	if (uint(ref) >= fieldValues_.size()) return NULL;
	return pDataType_->getFieldDataType( ref ).asOwner( &*fieldValues_[ref] );
}

/**
 *	Someone wants us to change the value of this property.
 */
PyObjectPtr PyFixedDictDataInstance::propertyRenovate( int ref,
	BinaryIStream & data, PyObjectPtr & pRetValue, DataType *& pRetType )
{
	DataType& fieldType = pDataType_->getFieldDataType( ref );
	PyObjectPtr pNewValue = fieldType.createFromStream( data, false );
	if (!pNewValue)
		return NULL;

	PyObjectPtr & valRef = fieldValues_[ref];
	PyObjectPtr pOldValue = valRef;
	if (valRef != pNewValue)
	{
		// detach old value and attach new one
		fieldType.detach( &*valRef );
		valRef = fieldType.attach( &*pNewValue, this, ref );
	}

	pRetValue = valRef;
	pRetType = &fieldType;
	return pOldValue;
}

/**
 *	Python get attribute method
 */
PyObject * PyFixedDictDataInstance::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();

	PyObject * pResult = PropertyOwner::pyGetAttribute( attr );

	if (pResult == NULL)
	{
		PyErr_Clear();
		pResult = this->getFieldByKey( attr );
	}

	return pResult;
}


/**
 *	Python set attribute method
 */
int PyFixedDictDataInstance::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();

	int result = PropertyOwner::pySetAttribute( attr, value );

	if (result != 0)
	{
		PyErr_Clear();
		result = this->setFieldByKey( attr, value );
	}

	return result;
}

/**
 *	Python mapping interface. Returns the number of items in the map.
 */
Py_ssize_t PyFixedDictDataInstance::pyGetLength( PyObject* self )
{
	return static_cast<PyFixedDictDataInstance*>(self)->getNumFields();
}

/**
 *	Python mapping interface. Returns the value keyed by given key.
 */
PyObject* PyFixedDictDataInstance::pyGetFieldByKey( PyObject* self,
		PyObject* key )
{
	return static_cast<PyFixedDictDataInstance*>(self)->getFieldByKey( key );
}

/**
 *	Python mapping interface. Sets the value keyed by given key.
 */
int PyFixedDictDataInstance::pySetFieldByKey( PyObject* self, PyObject* key,
		PyObject* value )
{
	return static_cast<PyFixedDictDataInstance*>(self)->
					setFieldByKey( key, value );
}

/**
 *	This method gets the value keyed by the input key.
 */
PyObject* PyFixedDictDataInstance::getFieldByKey( PyObject* key )
{
	if (!PyString_Check( key ))
	{
		PyErr_Format( PyExc_KeyError,
				"Key type must be a string, %s given",
				key->ob_type->tp_name );
		return NULL;
	}

	return this->getFieldByKey( PyString_AsString( key ) );
}


/**
 *	This method gets the value keyed by the input key.
 */
PyObject* PyFixedDictDataInstance::getFieldByKey( const char * keyString )
{
	PyObject * pValue = NULL;

	int idx = pDataType_->getFieldIndex( keyString );
	if (idx >= 0)
	{
		pValue = fieldValues_[idx].getObject();
		Py_INCREF( pValue );
	}
	else
	{
		PyObject * pKey = PyString_FromString( keyString );
		PyErr_SetObject( PyExc_KeyError, pKey );
		Py_DECREF( pKey );
		pValue = NULL;
	}

	return pValue;
}


/**
 *	Sets the value keyed by given key.
 */
int PyFixedDictDataInstance::setFieldByKey( PyObject* key, PyObject* value )
{
	if (!PyString_Check( key ))
	{
		PyErr_Format( PyExc_KeyError,
				"Key type must be a string, %s given",
				key->ob_type->tp_name );
		return -1;
	}

	return this->setFieldByKey( PyString_AsString( key ), value );
}

/**
 *	Sets the value keyed by given key.
 */
int PyFixedDictDataInstance::setFieldByKey( const char * keyString,
		PyObject* value )
{
	if (value == NULL)
	{
		PyErr_Format( PyExc_TypeError,
				"Cannot delete entry %s from FIXED_DICT",
				keyString );
		return -1;
	}

	int idx = pDataType_->getFieldIndex( keyString );
	if (idx >= 0)
	{
		DataType& fieldDataType = pDataType_->getFieldDataType( idx );
		if (fieldDataType.isSameType( value ))
		{
			PyObjectPtr& pCurValRef = fieldValues_[idx];
			if (pCurValRef != value)
			{

				bool changed = !pCurValRef ||
					!fieldDataType.isConst() ||
					PyObject_Compare( pCurValRef.getObject(), value );
				// detach old value and attach new one
				fieldDataType.detach( pCurValRef.getObject() );
				pCurValRef = fieldDataType.attach( value, this, idx );
				// Tell our owner about this change.
				// NOTE: After finalisation we may not be detached properly
				// from our owner (see FixedDictDataType::detach()) so
				// our owner may already be destroyed.
				if ((pOwner_ != NULL) && Py_IsInitialized() && changed)
				{
					int pathHead[2] = { idx, ownerRef_ };
					pOwner_->propertyChanged( pCurValRef, fieldDataType,
						ChangePath( pathHead, 2 ) );
				}
			}
		}
		else
		{
			PyErr_Format( PyExc_TypeError,
				"Value keyed by '%s' must be set to type '%s'",
				keyString, fieldDataType.typeName().c_str() );
			return -1;
		}
	}
	else
	{
		PyObject * pKey = PyString_FromString( keyString );
		PyErr_SetObject( PyExc_KeyError, pKey );
		Py_DECREF( pKey );
		return -1;
	}

	return 0;
}


/**
 * 	This method returns true if the given entry exists.
 */
PyObject * PyFixedDictDataInstance::py_has_key( PyObject* args )
{
	char * key;

	if (!PyArg_ParseTuple( args, "s", &key ))
	{
		PyErr_SetString( PyExc_TypeError, "Expected a string argument." );
		return NULL;
	}

	if (pDataType_->getFieldIndex( key ) >= 0)
		Py_RETURN_TRUE;
	else
		Py_RETURN_FALSE;
}


/**
 * 	This method returns a list of all the keys of this object.
 */
PyObject* PyFixedDictDataInstance::py_keys(PyObject* /*args*/)
{
	const FixedDictDataType::Fields& fields = pDataType_->getFields();

	PyObject* pyKeyList = PyList_New( fields.size() );
	if (pyKeyList)
	{
		for ( FixedDictDataType::Fields::const_iterator i = fields.begin();
				i < fields.end(); ++i )
		{
			PyList_SET_ITEM( pyKeyList, i - fields.begin(),
							PyString_FromString( i->name_.c_str() ) );
		}
	}

	return pyKeyList;
}


/**
 * 	This method returns a list of all the values of this object.
 */
PyObject* PyFixedDictDataInstance::py_values( PyObject* /*args*/ )
{
	PyObject* pyValueList = PyList_New( fieldValues_.size() );
	if (pyValueList)
	{
		for ( FieldValues::iterator i = fieldValues_.begin();
				i < fieldValues_.end(); ++i )
		{
			PyObject* pyFieldValue = i->getObject();
			Py_INCREF( pyFieldValue );
			PyList_SET_ITEM( pyValueList, i - fieldValues_.begin(),
								pyFieldValue );
		}
	}
	return pyValueList;
}


/**
 * 	This method returns a list of tuples of all the keys and values of this
 *	object.
 */
PyObject* PyFixedDictDataInstance::py_items( PyObject* /*args*/ )
{
	const FixedDictDataType::Fields& fields = pDataType_->getFields();

	PyObject* pyItemList = PyList_New( fields.size() );
	if (pyItemList)
	{
		for ( FixedDictDataType::Fields::size_type i = 0; i < fields.size();
				++i )
		{
			PyObject* pyTuple = Py_BuildValue( "(sO)", fields[i].name_.c_str(),
										fieldValues_[i].getObject() );
			PyList_SET_ITEM( pyItemList, i, pyTuple );
		}
	}

	return pyItemList;
}

/**
 *	Python pickling helper. Returns the second element of the tuple expected of
 * 	Python's __reduce__ method. Declared in PY_PICKLING_METHOD_DECLARE.
 */
PyObject * PyFixedDictDataInstance::pyPickleReduce()
{
	// Make an equivalent dictionary.
	const FixedDictDataType::Fields& fields = pDataType_->getFields();
	PyObject* pDict = PyDict_New();
	for ( FixedDictDataType::Fields::size_type i = 0; i < fields.size();
			++i )
	{
		PyDict_SetItemString( pDict, fields[i].name_.c_str(),
							fieldValues_[i].getObject() );
	}

	// Make a tuple (method arguments for PickleResolve())
	PyObject * pArgs = PyTuple_New( 1 );
	PyTuple_SET_ITEM( pArgs, 0, pDict );

	return pArgs;
}

/**
 *	Python unpickling helper. Creates a PyFixedDictDataInstance from the
 *	information returned by PyFixedDictDataInstance::pyPickleReduce().
 * 	Required by PY_AUTO_UNPICKLING_FACTORY_DECLARE.
 */
PyObject * PyFixedDictDataInstance::PickleResolve( PyObject * pDict )
{
	// __kyl__ (1/3/2006) Currently doesn't return a PyFixedDictDataInstance
	// since it's too hard to find the right FixedDictDataType for pDataType_.
	// We'd also need pOwner_ and ownerRef_ and re-attach
	// PyFixedDictDataInstance to the right entity here.

	// Simply return the equivalent dictionary for now. pDict already increfed
	// by PY_AUTO_UNPICKLING_FACTORY_DECLARE
	return pDict;
}



/**
 *	Virtual destructor
 */
FixedDictDataType::~FixedDictDataType()
{
	// If this assertion is hit, it means that this data type is being
	// destructed after the call to Script::fini. Make sure that the
	// EntityDescription is destructed or EntityDescription::clear called on it
	// before Script::fini.
	MF_ASSERT_DEV( !pImplementor_ || !Script::isFinalised() );
}


/**
 *	This method sets the user-defined instance that will be responsible for
 * 	converting our dictionary to/from their custom Python class.
 */
void FixedDictDataType::setCustomClassImplementor(
		const std::string& moduleName, const std::string& instanceName )
{
	moduleName_ = moduleName;
	instanceName_ = instanceName;

	isCustomClassImplInited_ = false;
}

/**
 *	This method sets our internal function pointers to the user-defined instance
 */
void FixedDictDataType::setCustomClassFunctions()
{
	INFO_MSG( "FixedDictDataType::setCustomClassFunctions: %s.%s\n",
				moduleName_.c_str(), instanceName_.c_str() );

	PyObjectPtr pModule(
			PyImport_ImportModule( const_cast<char *>( moduleName_.c_str() ) ),
			PyObjectPtr::STEAL_REFERENCE );

	if (pModule)
	{
		pImplementor_ = PyObjectPtr(
							PyObject_GetAttrString( pModule.getObject(),
								const_cast<char *>( instanceName_.c_str() ) ),
							PyObjectPtr::STEAL_REFERENCE );

		if (pImplementor_)
		{
			pGetDictFromObjFn_ =
				PyObjectPtr( PyObject_GetAttrString( pImplementor_.getObject(),
													"getDictFromObj" ),
							PyObjectPtr::STEAL_REFERENCE );
			if (!pGetDictFromObjFn_)
			{
				PyErr_Clear();
				ERROR_MSG( "FixedDictDataType::setCustomClassImplementor: "
							"%s.%s is missing method getDictFromObj\n",
							moduleName_.c_str(), instanceName_.c_str() );
			}

			pCreateObjFromDictFn_ =
				PyObjectPtr( PyObject_GetAttrString( pImplementor_.getObject(),
													"createObjFromDict" ),
							PyObjectPtr::STEAL_REFERENCE );
			if (!pCreateObjFromDictFn_)
			{
				PyErr_Clear();
				ERROR_MSG( "FixedDictDataType::setCustomClassImplementor: "
							"%s.%s is missing method createObjFromDict\n",
							moduleName_.c_str(), instanceName_.c_str() );
			}

			pIsSameTypeFn_ =
				PyObjectPtr( PyObject_GetAttrString( pImplementor_.getObject(),
													"isSameType" ),
							PyObjectPtr::STEAL_REFERENCE );
			if (!pIsSameTypeFn_)
			{
				PyErr_Clear();
				WARNING_MSG( "FixedDictDataType::setCustomClassImplementor: "
							"%s.%s is missing method isSameType\n",
							moduleName_.c_str(), instanceName_.c_str() );
			}

			pAddToStreamFn_ =
				PyObjectPtr( PyObject_GetAttrString( pImplementor_.getObject(),
													"addToStream" ),
							PyObjectPtr::STEAL_REFERENCE );
			if (!pAddToStreamFn_) PyErr_Clear();

			pCreateFromStreamFn_ =
				PyObjectPtr( PyObject_GetAttrString( pImplementor_.getObject(),
													"createFromStream" ),
							PyObjectPtr::STEAL_REFERENCE );
			if (!pCreateFromStreamFn_) PyErr_Clear();

			if ((pAddToStreamFn_ && !pCreateFromStreamFn_) ||
				(!pAddToStreamFn_ && pCreateFromStreamFn_))
			{
				ERROR_MSG( "FixedDictDataType::setCustomClassImplementor: "
							"%s.%s must implement both addToStream and "
							"createFromStream, or implement neither\n",
							moduleName_.c_str(), instanceName_.c_str() );
				pAddToStreamFn_ = NULL;
				pCreateFromStreamFn_ = NULL;
			}
		}
		else
		{
			PyErr_PrintEx(0);
			ERROR_MSG( "FixedDictDataType::setCustomClassImplementor: "
						"Unable to import %s from %s\n",
						instanceName_.c_str(), moduleName_.c_str() );
		}
	}
	else
	{
		PyErr_PrintEx(0);
		ERROR_MSG( "FixedDictDataType::setCustomClassImplementor: Unable to "
					"import %s\n", moduleName_.c_str() );
	}
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
bool FixedDictDataType::isSameType( PyObject* pValue )
{
	initCustomClassImplOnDemand();

	bool isSame = true;

	if (allowNone_ && pValue == Py_None )
	{
		// isSame = true;
	}
	else if (this->hasCustomClass())
	{
		if (this->hasCustomIsSameType())
		{
			isSame = this->isSameTypeCustom( pValue );
		}
		else
		{
			// Create a PyFixedDictDataInstance using the user provided
			// getDictFromObj() function. If it succeeds, we'll assume that it is
			// an acceptable type.
			PyObjectPtr pObj = this->createInstanceFromCustomClass( pValue );
			isSame = pObj;
		}
	}
	// check if an instance of our special Python class
	else if (PyFixedDictDataInstance::isSameType( pValue, *this ))
	{
		// isSame = true;
	}
	else if (PyMapping_Check( pValue ))
	{
		// Check that it has the keys that we need.
		// __kyl__ (10/1/2006) TODO: Should we allow other keys to be present?
		for (Fields::const_iterator i = fields_.begin();
				(i != fields_.end()) && isSame; ++i)
		{
			PyObjectPtr pKeyedValue(
				PyMapping_GetItemString( pValue,
										const_cast<char*>(i->name_.c_str()) ),
				PyObjectPtr::STEAL_REFERENCE );

			if (pKeyedValue)
			{
			  	isSame = i->type_->isSameType( pKeyedValue.getObject() );
			}
			else
			{
				PyErr_Clear();
				isSame = false;
			}
		}
	}
	else
	{
		isSame = false;
	}

	return isSame;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::reloadScript
 */
void FixedDictDataType::reloadScript()
{
	// __kyl__ (12/1/2007) If we haven't inited our custom class functions,
	// then we don't need to reload. this->hasCustomClass() is conveniently
	// false if we either haven't inited or if we don't have a custom class.
	// initCustomClassImplOnDemand();
	// DEBUG_MSG( "FixedDictDataType::reloadScript: this=%p\n", this );

	if (this->hasCustomClass())
	{
		pImplementor_ = NULL;
		pGetDictFromObjFn_ = NULL;
		pCreateObjFromDictFn_ = NULL;
		pIsSameTypeFn_ = NULL;
		pAddToStreamFn_ = NULL;
		pCreateFromStreamFn_ = NULL;

		this->setCustomClassFunctions();
	}
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::clearScript
 */
void FixedDictDataType::clearScript()
{
	pImplementor_ = NULL;
	pGetDictFromObjFn_ = NULL;
	pCreateObjFromDictFn_ = NULL;
	pIsSameTypeFn_ = NULL;
	pAddToStreamFn_ = NULL;
	pCreateFromStreamFn_ = NULL;
}

/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
void FixedDictDataType::setDefaultValue( DataSectionPtr pSection )
{
	pDefaultSection_ = pSection;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::pDefaultValue
 */
PyObjectPtr FixedDictDataType::pDefaultValue() const
{
	const_cast<FixedDictDataType*>(this)->initCustomClassImplOnDemand();

	PyObjectPtr pDefault;
	if (pDefaultSection_)
	{
		return this->createFromSection( pDefaultSection_ );
	}
	else if (allowNone_)
	{
		return Py_None;
	}
	else
	{
		pDefault = this->createDefaultInstance();
		if (this->hasCustomClass())
			pDefault = this->createCustomClassFromInstance(
				static_cast<PyFixedDictDataInstance*>(pDefault.getObject()) );

		return pDefault;
	}
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToStream
 */
void FixedDictDataType::addToStream( PyObject * pValue,
		BinaryOStream & stream, bool isPersistentOnly ) const
{
	if (allowNone_)
	{
		if (pValue == Py_None)
		{
			stream << uint8(0);
			return;
		}
		stream << uint8(1);
	}

	const_cast<FixedDictDataType*>(this)->initCustomClassImplOnDemand();

	if (this->hasCustomClass())
	{
		if (!isPersistentOnly && this->hasCustomAddToStream())
		{
			this->addToStreamCustom(pValue, stream, isPersistentOnly);
		}
		else
		{
			PyObjectPtr pObj = this->createInstanceFromCustomClass( pValue );
			if (pObj)
			{
				this->addInstanceToStream(
					static_cast<PyFixedDictDataInstance*>( pObj.getObject() ),
					stream, isPersistentOnly );
			}
			else
			{
				ERROR_MSG( "FixedDictDataType::addToStream: Failed for %s\n",
							this->typeName().c_str() );
				// Can't return failure from here, so must add something to the
				// stream to prevent the de-streaming from going crazy.
				this->addInstanceToStream(
					static_cast<PyFixedDictDataInstance*>(
							this->createDefaultInstance().getObject()),
					stream, isPersistentOnly );
			}
		}
	}
	else if (PyFixedDictDataInstance::isSameType( pValue, *this ))
	{
		this->addInstanceToStream( static_cast<PyFixedDictDataInstance*>(pValue),
							stream, isPersistentOnly );
	}
	else
	{
		PyObjectPtr pObj = this->createInstanceFromMappingObj( pValue );
		if (pObj)
		{
			this->addInstanceToStream(
					static_cast<PyFixedDictDataInstance*>( pObj.getObject() ),
					stream, isPersistentOnly );
		}
		else
		{
			ERROR_MSG( "FixedDictDataType::addToStream: Failed for %s\n",
						this->typeName().c_str() );
			// Can't return failure from here, so must add something to the
			// stream to prevent the de-streaming from going crazy.
			this->addInstanceToStream(
				static_cast<PyFixedDictDataInstance*>(
						this->createDefaultInstance().getObject()),
				stream, isPersistentOnly );
		}
	}
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromStream
 */
PyObjectPtr FixedDictDataType::createFromStream( BinaryIStream & stream,
		bool isPersistentOnly ) const
{
	if (allowNone_)
	{
		uint8 hasValues;
		stream >> hasValues;

		if (!hasValues)	return Py_None;
	}

	const_cast<FixedDictDataType*>(this)->initCustomClassImplOnDemand();

	PyObjectPtr pObj;
	if (this->hasCustomClass())
	{
		if (!isPersistentOnly && this->hasCustomCreateFromStream())
		{
			pObj = this->createFromStreamCustom( stream, isPersistentOnly );
		}
		else
		{
			PyObjectPtr pInst =
				this->createInstanceFromStream( stream, isPersistentOnly );
			if (pInst)
			{
				pObj = this->createCustomClassFromInstance(
					static_cast<PyFixedDictDataInstance*>(pInst.getObject()) );
			}
		}
	}
	else
	{
		pObj = this->createInstanceFromStream( stream, isPersistentOnly );
	}

	return pObj;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
void FixedDictDataType::addToSection( PyObject * pValue,
		DataSectionPtr pSection ) const
{
	if (pValue == Py_None)
		return;	// leave as empty section

	const_cast<FixedDictDataType*>(this)->initCustomClassImplOnDemand();

	PyObjectPtr pObj;

	if (this->hasCustomClass())
	{
		pObj = this->createInstanceFromCustomClass( pValue );
	}
	else if (PyFixedDictDataInstance::isSameType( pValue, *this ))
	{
		pObj = pValue;
	}
	else
	{
		pObj = this->createInstanceFromMappingObj( pValue );
	}

	if (pObj)
		this->addInstanceToSection(
				static_cast<PyFixedDictDataInstance*>( pObj.getObject() ),
				pSection );
	else
		ERROR_MSG( "FixedDictDataType::addToSection: Failed for %s\n",
					this->typeName().c_str() );
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
PyObjectPtr FixedDictDataType::createFromSection( DataSectionPtr pSection )
		const
{
	if (!pSection)
	{
		ERROR_MSG( "FixedDictDataType::createFromSection: Section is NULL.\n" );
		return NULL;
	}

	if (allowNone_ && pSection->countChildren() == 0)
		return Py_None;

	const_cast<FixedDictDataType*>(this)->initCustomClassImplOnDemand();

	PyObjectPtr pObj = this->createInstanceFromSection( pSection );
	if (this->hasCustomClass())
	{
		pObj = this->createCustomClassFromInstance(
				static_cast< PyFixedDictDataInstance* >( pObj.getObject() ) );
	}

	return pObj;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToMD5
 */
void FixedDictDataType::addToMD5( MD5 & md5 ) const
{
	const_cast<FixedDictDataType*>(this)->initCustomClassImplOnDemand();

	md5.append( "FixedDict", sizeof( "FixedDict" ) );
	if (this->hasCustomClass())
	{
		md5.append( moduleName_.c_str(), moduleName_.size() );
		md5.append( instanceName_.c_str(), instanceName_.size() );
	}
	md5.append( &allowNone_, sizeof(allowNone_) );
	for (Fields::const_iterator it = fields_.begin(); it != fields_.end(); ++it)
	{
		md5.append( it->name_.data(), it->name_.size() );
		it->type_->addToMD5( md5 );
	}
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::operator<
 */
bool FixedDictDataType::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	// ok, equal metas, so downcast other and compare with us
	FixedDictDataType& otherCl= (FixedDictDataType&)other;

	if (moduleName_ < otherCl.moduleName_) return true;
	if (otherCl.moduleName_ < moduleName_) return false;

	if (instanceName_ < otherCl.instanceName_) return true;
	if (otherCl.instanceName_ < instanceName_) return false;

	if (allowNone_ != otherCl.allowNone_)
		return !allowNone_;	// we are less if our allowNone is false

	if (fields_.size() < otherCl.fields_.size()) return true;
	if (otherCl.fields_.size() < fields_.size()) return false;

	for (uint i = 0; i < fields_.size(); ++i)
	{
		int res = fields_[i].name_.compare( otherCl.fields_[i].name_ );
		if (res < 0) return true;
		if (res > 0) return false;

		if ((*fields_[i].type_) < (*otherCl.fields_[i].type_)) return true;
		if ((*otherCl.fields_[i].type_) < (*fields_[i].type_)) return false;
	}

	if (otherCl.pDefaultSection_)
		return otherCl.pDefaultSection_->compare( pDefaultSection_ ) > 0;
	// else we are equal or greater than other.
	return false;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::typeName
 */
std::string FixedDictDataType::typeName() const
{
	std::string acc = this->DataType::typeName();
	if (!isCustomClassImplInited_ || this->hasCustomClass())
		acc += " [Implemented by " + moduleName_ + "." + instanceName_ + "]";
	if (allowNone_) acc += " [None Allowed]";
	for (Fields::const_iterator it = fields_.begin(); it != fields_.end(); ++it)
	{
		acc += (it == fields_.begin()) ? " props " : ", ";
		acc += it->name_ + ":(" + it->type_->typeName() + ")";
	}
	return acc;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::attach
 */
PyObjectPtr FixedDictDataType::attach( PyObject * pObject,
	PropertyOwnerBase * pOwner, int ownerRef )
{
	// if it's None and that's ok, just return that
	if (pObject == Py_None && allowNone_)
		return Py_None;

	initCustomClassImplOnDemand();

	if (this->hasCustomClass())
	{
		// First part of isSameType() check
		if (this->hasCustomIsSameType() && !this->isSameTypeCustom( pObject ))
			return NULL;

		// See if they are referencing our PyFixedDictDataInstance
		PyObjectPtr pDict( this->getDictFromCustomClass( pObject ),
						PyObjectPtr::STEAL_REFERENCE );
		// Second part of isSameType() check
		if (PyFixedDictDataInstance::isSameType( pDict.getObject(), *this ))
		{
			// Yay! isSameType() == true
			PyFixedDictDataInstancePtr pInst(
				static_cast<PyFixedDictDataInstance*>( pDict.getObject() ) );
			if (pInst->hasOwner())
			{
				// Create copy
				pInst = PyFixedDictDataInstancePtr(
							new PyFixedDictDataInstance( this, *pInst ),
							pInst.STEAL_REFERENCE );
				pInst->setOwner( pOwner, ownerRef );
				return this->createCustomClassFromInstance( pInst.getObject() );
			}
			else
			{
				pInst->setOwner( pOwner, ownerRef );
				return pObject;
			}
		}
		else	// not referencing PyFixedDictDataInstance
		{
			// Third part of isSameType() check, for custom class without
			// isSameType() method and doesn't reference PyFixedDictDataInstance
			if (this->hasCustomIsSameType() ||
				this->createInstanceFromMappingObj( pDict.getObject() ))
				return pObject;
			else
				return NULL;
		}
	}

	PyFixedDictDataInstancePtr pInst;

	// it's easy if it's the right python + entitydef type
	if (PyFixedDictDataInstance::isSameType( pObject, *this ))
	{
		pInst = static_cast<PyFixedDictDataInstance*>( pObject );
		if (pInst->hasOwner())
		{
			// Create copy
			pInst = PyFixedDictDataInstancePtr(
						new PyFixedDictDataInstance( this, *pInst ),
						pInst.STEAL_REFERENCE );
			// note: up to caller to check that prop isn't being set back
		}
	}
	else
	{
		PyObjectPtr pObj = this->createInstanceFromMappingObj( pObject );
		pInst = PyFixedDictDataInstancePtr(
					static_cast<PyFixedDictDataInstance*>( pObj.getObject()) );
	}

	if (pInst)
		pInst->setOwner( pOwner, ownerRef );

	return pInst;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::detach
 */
void FixedDictDataType::detach( PyObject * pObject )
{
	initCustomClassImplOnDemand();

	if (pObject == Py_None)
	{
		// do nothing
	}
	else if (this->hasCustomClass())
	{
		// TODO: Call isSameType()? A bit nasty to expect user code to handle
		// unexpected types but do we want to incur overhead?
		// See if they are referencing our PyFixedDictDataInstance
		PyObjectPtr pDict( this->getDictFromCustomClass( pObject ),
						PyObjectPtr::STEAL_REFERENCE );
		if (PyFixedDictDataInstance::Check( pDict.getObject() ))
		{
			static_cast<PyFixedDictDataInstance*>( pDict.getObject() )->
					disowned();
		}
	}
	else
	{
		// NOTE: After finalisation hasCustomClass() will always return
		// false so custom objects will end up here. We can't call
		// getDictFromCustomClass() so if the custom object is referencing our
		// PyFixedDictDataInstance its pOwner_ won't be set to NULL. This is
		// why PyFixedDictDataInstance always checks that we have not been
		// finalised before using its pOwner_.
		// NOTE: Can't use Script::isFinalised() since it is set too late. What
		// we really need is flag indicating is whether clearScript() has
		// already been called.
		MF_ASSERT_DEV( PyFixedDictDataInstance::Check( pObject ) ||
					!Py_IsInitialized() );
		if ( PyFixedDictDataInstance::Check( pObject ) )
			static_cast<PyFixedDictDataInstance*>( pObject )->disowned();
	}
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::asOwner
 */
PropertyOwnerBase* FixedDictDataType::asOwner( PyObject * pObject )
{
	initCustomClassImplOnDemand();

	PyFixedDictDataInstance* pOwner;

	if (this->hasCustomClass() && (pObject != Py_None))
	{
		// TODO: Call isSameType()? A bit nasty to expect user code to handle
		// unexpected types but do we want to incur overhead?
		// See if they are referencing our PyFixedDictDataInstance
		PyObjectPtr pDict( this->getDictFromCustomClass( pObject ),
						PyObjectPtr::STEAL_REFERENCE );
		if (PyFixedDictDataInstance::Check( pDict.getObject() ))
			pOwner = static_cast<PyFixedDictDataInstance*>( pDict.getObject() );
		else
			pOwner = NULL;
	}
	else
	{
		if (PyFixedDictDataInstance::Check( pObject ))
			pOwner = static_cast<PyFixedDictDataInstance*>( pObject );
		else
			pOwner = NULL;
	}

	return pOwner;
}

/**
 *	Creates a PyFixedDictDataInstance with default values for each item.
 */
PyObjectPtr FixedDictDataType::createDefaultInstance() const
{
	PyFixedDictDataInstancePtr pInst(
		new PyFixedDictDataInstance( const_cast<FixedDictDataType*>(this) ),
		PyFixedDictDataInstancePtr::STEAL_REFERENCE );
	for (Fields::const_iterator i = fields_.begin(); i != fields_.end(); ++i)
	{
		PyObjectPtr pValue = i->type_->pDefaultValue();
		MF_ASSERT_DEV( pValue );

		pInst->initFieldValue( i - fields_.begin(), pValue );
	}

	return PyObjectPtr( pInst );
}

/**
 *	This method adds a PyFixedDictDataInstance into the stream.
 */
void FixedDictDataType::addInstanceToStream( PyFixedDictDataInstance* pInst,
		BinaryOStream& stream, bool isPersistentOnly ) const
{
	for (Fields::const_iterator i = fields_.begin(); i != fields_.end(); ++i)
	{
		if ((!isPersistentOnly) || (i->isPersistent_))
			i->type_->addToStream(
				pInst->getFieldValue( i - fields_.begin() ).getObject(),
				stream, isPersistentOnly );
	}
}

/**
 *	This method creates a PyFixedDictDataInstance from the stream.
 */
PyObjectPtr FixedDictDataType::createInstanceFromStream(
		BinaryIStream& stream, bool isPersistentOnly ) const
{
	PyFixedDictDataInstancePtr pInst(
		new PyFixedDictDataInstance( const_cast<FixedDictDataType*>(this) ),
		PyObjectPtr::STEAL_REFERENCE );

	for (Fields::const_iterator i = fields_.begin(); i != fields_.end(); ++i)
	{
		if ((!isPersistentOnly) || (i->isPersistent_))
		{
			PyObjectPtr pValue = i->type_->createFromStream( stream,
										isPersistentOnly );
			if (!pValue) return NULL;	// error already printed

			pInst->initFieldValue( i - fields_.begin(), pValue );
		}
		else
		{
			pInst->initFieldValue( i - fields_.begin(),
									i->type_->pDefaultValue() );
		}
	}

	return PyObjectPtr( pInst.getObject() );
}

/**
 *	This method creates a PyFixedDictDataInstance from a data section.
 */
PyObjectPtr FixedDictDataType::createInstanceFromSection(
		DataSectionPtr pSection ) const
{
	PyFixedDictDataInstancePtr pInst(
			new PyFixedDictDataInstance( const_cast<FixedDictDataType*>(this) ),
			PyFixedDictDataInstancePtr::STEAL_REFERENCE );
	for (Fields::const_iterator i = fields_.begin(); i != fields_.end(); ++i)
	{
		DataSectionPtr pChildSection = pSection->openSection( i->name_ );
		if (pChildSection)
		{
			PyObjectPtr pValue = i->type_->createFromSection( pChildSection );
			if (!pValue) return NULL;	// error already printed

			pInst->initFieldValue( i - fields_.begin(), pValue );
		}
		else
		{
			pInst->initFieldValue( i - fields_.begin(),
									i->type_->pDefaultValue() );
		}
	}

	return pInst;
}

/**
 *	This method adds data from PyFixedDictDataInstance into the section.
 */
void FixedDictDataType::addInstanceToSection( PyFixedDictDataInstance* pInst,
		DataSectionPtr pSection ) const
{
	for (Fields::const_iterator i = fields_.begin(); i != fields_.end(); ++i)
	{
		i->type_->addToSection(
			pInst->getFieldValue( i - fields_.begin() ).getObject(),
			pSection->newSection( i->name_ ) );
	}
}


/**
 *	This method creates a PyFixedDictDataInstance from a PyObject that supports
 * 	the mapping protocol.
 */
PyObjectPtr FixedDictDataType::createInstanceFromMappingObj(
		PyObject* pyMappingObj ) const
{
	PyFixedDictDataInstancePtr pInst;

	if (PyMapping_Check( pyMappingObj ))
	{
		pInst = PyFixedDictDataInstancePtr(
			new PyFixedDictDataInstance( const_cast<FixedDictDataType*>(this) ),
			PyObjectPtr::STEAL_REFERENCE );

		for (Fields::const_iterator i = fields_.begin(); i != fields_.end(); ++i)
		{
			PyObjectPtr pValue(
				PyMapping_GetItemString( pyMappingObj,
							const_cast<char*>(i->name_.c_str()) ),
				PyObjectPtr::STEAL_REFERENCE );

			if (pValue)
			{
				if (i->type_->isSameType( pValue.getObject() ))
				{
					pInst->initFieldValue( i - fields_.begin(), pValue );
				}
				else
				{
					ERROR_MSG( "FixedDictDataType::createInstanceFromMappingObj: "
							"Value keyed by '%s' should be of type %s\n",
							i->name_.c_str(), i->type_->typeName().c_str() );
					return NULL;
				}
			}
			else
			{
				PyErr_PrintEx(0);
				ERROR_MSG( "FixedDictDataType::createInstanceFromMappingObj: "
							"Missing key '%s'\n", i->name_.c_str() );
				return NULL;
			}
		}
	}
	else
	{
		ERROR_MSG( "FixedDictDataType::createInstanceFromMappingObj: "
					"Cannot create from non-dictionary-like object\n" );
	}

	return pInst;
}

/**
 *	This method checks whether the given Python object is the same type as
 * 	the user-defined type by calling the isSameType() function provided by
 * 	the user.
 */
bool FixedDictDataType::isSameTypeCustom( PyObject* pValue ) const
{
	PyObjectPtr pResult(
		PyObject_CallFunctionObjArgs( pIsSameTypeFn_.getObject(), pValue,
									NULL ),
						PyObjectPtr::STEAL_REFERENCE );
	if (pResult)
	{
		if (PyBool_Check( pResult.getObject() ))
		{
			return pResult.getObject() == Py_True;
		}
		else
		{
			ERROR_MSG( "FixedDictDataType::isSameTypeCustom: "
						"%s.%s.isSameType method returned non-bool object\n",
						moduleName_.c_str(), instanceName_.c_str() );
		}
	}
	else
	{
		PyErr_PrintEx(0);
		ERROR_MSG( "FixedDictDataType::isSameTypeCustom: %s.%s.isSameType "
					"call failed\n", moduleName_.c_str(),
					instanceName_.c_str() );
	}

	return false;
}

/**
 *	This method calls the getDictFromObj() script method and returns the result.
 */
PyObject* FixedDictDataType::getDictFromCustomClass( PyObject* pCustomClass )
		const
{
	if (pGetDictFromObjFn_)
	{
		PyObjectPtr pResult(
			PyObject_CallFunctionObjArgs( pGetDictFromObjFn_.getObject(),
										pCustomClass, NULL ),
			PyObjectPtr::STEAL_REFERENCE );
		if (pResult)
		{
			if (PyMapping_Check( pResult.getObject() ))
			{
				Py_INCREF( pResult.getObject() );	// Prevent auto-destruction
				return pResult.getObject();
			}
			else
			{
				ERROR_MSG( "FixedDictDataType::getDictFromCustomClass: "
							"%s.%s.getDictFromObj returned non-dictionary "
							"object\n",
							moduleName_.c_str(), instanceName_.c_str() );
			}
		}
		else
		{
			PyErr_PrintEx(0);
			ERROR_MSG( "FixedDictDataType::getDictFromCustomClass: "
						"%s.%s.getDictFromObj call failed\n",
						moduleName_.c_str(), instanceName_.c_str() );
		}
	}

	return NULL;
}

/**
 *	This method calls the getDictFromObj() script method and creates a
 * 	PyFixedDictDataInstance from the result.
 */
PyObjectPtr FixedDictDataType::createInstanceFromCustomClass(
		PyObject* pValue ) const
{
	PyObjectPtr pDict( this->getDictFromCustomClass( pValue ),
						PyObjectPtr::STEAL_REFERENCE );
	if (pDict)
		return this->createInstanceFromMappingObj( pDict.getObject() );

	return NULL;
}

/**
 *	This method calls the createObjFromDict() script method returns the
 * 	result of that call.
 */
PyObjectPtr FixedDictDataType::createCustomClassFromInstance(
		PyFixedDictDataInstance* pInst ) const
{
	PyObjectPtr pResult(
		PyObject_CallFunctionObjArgs( pCreateObjFromDictFn_.getObject(),
					static_cast<PyObject*>(pInst), NULL ),
		PyObjectPtr::STEAL_REFERENCE );
	if (!pResult)
	{
		PyErr_PrintEx(0);
		ERROR_MSG( "FixedDictDataType::createCustomClassFromInstance: "
					"%s.%s.createObjFromDict call failed\n",
					moduleName_.c_str(), instanceName_.c_str() );
	}

	return pResult;
}

/**
 *	This method calls the addToStream script method.
 */
void FixedDictDataType::addToStreamCustom( PyObject* pValue,
	BinaryOStream& stream, bool isPersistentOnly) const
{
	PyObjectPtr pResult(
		PyObject_CallFunctionObjArgs( pAddToStreamFn_.getObject(), pValue,
									NULL ),
		PyObjectPtr::STEAL_REFERENCE );
	if (pResult)
	{
		if (PyString_Check( pResult.getObject() ))
		{
			std::string objStream( PyString_AsString( pResult.getObject() ),
									PyString_Size( pResult.getObject() ) );
			stream << objStream;
		}
		else
		{
			ERROR_MSG( "FixedDictDataType::addToStreamCustom: "
						"%s.%s.addToStream method returned non-string "
						"object\n",	moduleName_.c_str(),
						instanceName_.c_str() );
			stream << "";
		}
	}
	else
	{
		PyErr_PrintEx(0);
		ERROR_MSG( "FixedDictDataType::addToStreamCustom: %s.%s.addToStream "
					"call failed\n", moduleName_.c_str(),
					instanceName_.c_str() );
		stream << "";
	}
}

/**
 *	This method calls the createFromStream() script method and returns the
 * 	result of that call.
 */
PyObjectPtr FixedDictDataType::createFromStreamCustom(
		BinaryIStream & stream, bool isPersistentOnly ) const
{
	std::string data;
	stream >> data;

	if (stream.error())
	{
		ERROR_MSG( "FixedDictDataType::createFromStreamCustom: "
				   "Not enough data on stream to read value\n" );
		return NULL;
	}

	PyObjectPtr pResult(
		PyObject_CallFunction( pCreateFromStreamFn_.getObject(), "s#",
								data.data(), data.length() ),
		PyObjectPtr::STEAL_REFERENCE );
	if (!pResult)
	{
		PyErr_PrintEx(0);
		ERROR_MSG( "FixedDictDataType::createFromStreamCustom: "
					"%s.%s.createFromStream call failed\n", moduleName_.c_str(),
					instanceName_.c_str() );
	}

	return pResult;
}

/**
 *	This class implements a factory for FixedDictDataType.
 */
class FixedDictMetaDataType : public MetaDataType
{
public:
	typedef FixedDictDataType DataType;

	FixedDictMetaDataType()
	{
		MetaDataType::addMetaType( this );
   	}
	virtual ~FixedDictMetaDataType()
	{
		MetaDataType::delMetaType( this );
	}

	virtual const char * name()	const { return "FIXED_DICT"; }

	virtual DataTypePtr getType( DataSectionPtr pSection )
	{
		// Process <Properties> section just like CLASS
		FixedDictDataType* pFixedDictType =
			ClassMetaDataType::buildType( pSection, *this );

		if (pFixedDictType)
		{
			std::string implementedBy = pSection->readString( "implementedBy" );

			if (!implementedBy.empty())
			{
				std::string::size_type pos = implementedBy.find_last_of( "." );

				if (pos == std::string::npos)
				{
					ERROR_MSG( "FixedDictMetaDataType::getType: "
								"<implementedBy> %s - must contain a '.'",
								implementedBy.c_str() );
					delete pFixedDictType;
					return NULL;
				}

				std::string moduleName = implementedBy.substr( 0, pos );
				std::string instanceName = implementedBy.substr( pos + 1,
						implementedBy.size() );

				pFixedDictType->setCustomClassImplementor( moduleName,
						instanceName );
			}
		}

		return pFixedDictType;
	}

};

static FixedDictMetaDataType s_FIXED_DICT_metaDataType;


// -----------------------------------------------------------------------------
// Section: UserDataType
// -----------------------------------------------------------------------------

// Should UserDataType be in this file?
#include "pyscript/py_data_section.hpp"

/**
 *	Destructor.
 */
UserDataType::~UserDataType()
{
	// If this assertion is hit, it means that this data type is being
	// destructed after the call to Script::fini. Make sure that the
	// EntityDescription is destructed or EntityDescription::clear called on it
	// before Script::fini.
	MF_ASSERT_DEV( !pObject_ || !Script::isFinalised() );
}


/**
 *	Overrides the DataType method.
 *
 *	@see DataType::isSameType
 */
bool UserDataType::isSameType( PyObject * /*pValue*/ )
{
	// TODO: We should allow script check if this type is okay.
	return this->pObject() != NULL;
}

void UserDataType::reloadScript()
{
	this->init();
}

void UserDataType::clearScript()
{
	pObject_ = NULL;
	isInited_ = false;
}

/**
 *	This method sets the default value for this type.
 *
 *	@see DataType::setDefaultValue
 */
void UserDataType::setDefaultValue( DataSectionPtr pSection )
{
	pDefaultSection_ = pSection;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::pDefaultValue
 */
PyObjectPtr UserDataType::pDefaultValue() const
{
	if (pDefaultSection_)
		return this->createFromSection( pDefaultSection_ );

	// TODO: Use this->method + script::ask
	PyObject * pObject = this->pObject();

	if (!pObject)
	{
		ERROR_MSG( "UserDataType::pDefaultValue: "
				"Do not have %s.%s\n",
			moduleName_.c_str(), instanceName_.c_str() );
		return Py_None;
	}

	PyObject * pResult =
		PyObject_CallMethod( pObject, "defaultValue", "" );

	if (!pResult)
	{
		ERROR_MSG( "UserDataType::pDefaultValue: (%s.%s)"
						"Script call failed.\n",
			moduleName_.c_str(), instanceName_.c_str() );
		PyErr_PrintEx(0);
		return Py_None;
	}

	return PyObjectPtr( pResult, PyObjectPtr::STEAL_REFERENCE );
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToStream
 */
void UserDataType::addToStream( PyObject * pNewValue,
		BinaryOStream & stream, bool /*isPersistentOnly*/ ) const
{
	// TODO: Use this->method + script::ask
	PyObject * pObject = this->pObject();

	if (!pObject)
	{
		ERROR_MSG( "UserDataType::addToStream: "
				"Do not have %s.%s\n",
			moduleName_.c_str(), instanceName_.c_str() );
		// TODO: Should be able to return an error.
		stream << "";
		return;
	}

	PyObjectPtr pResult(
		PyObject_CallMethod( pObject, "addToStream", "O", pNewValue ),
		PyObjectPtr::STEAL_REFERENCE );

	if (!pResult)
	{
		ERROR_MSG( "UserDataType::addToStream: Script call failed.\n" );
		PyErr_PrintEx(0);
		// TODO: Should be able to return an error.
		stream << "";
		return;
	}

	if (!PyString_Check( pResult.getObject() ))
	{
		ERROR_MSG( "UserDataType::addToStream: (%s.%s)"
					"String not returned.\n",
				moduleName_.c_str(), instanceName_.c_str() );
		// TODO: Should be able to return an error.
		stream << "";
		return;
	}

	int size = PyString_Size( pResult.getObject() );
	std::string newString(
		PyString_AsString( pResult.getObject() ), size );
	stream << newString;
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromStream
 */
PyObjectPtr UserDataType::createFromStream( BinaryIStream & stream,
	bool /*isPersistentOnly*/ ) const
{
	// TODO: Use this->method + script::ask
	PyObject * pObject = this->pObject();

	if (!pObject)
	{
		ERROR_MSG( "UserDataType::createFromStream: "
				"Do not have %s.%s\n",
			moduleName_.c_str(), instanceName_.c_str() );
		return NULL;
	}

	std::string data;
	stream >> data;

	if (stream.error())
	{
		ERROR_MSG( "UserDataType::createFromStream: "
				   "Not enough data on stream to read value\n" );
		return NULL;
	}

	PyObject * pResult =
		PyObject_CallMethod( pObject, "createFromStream", "s#",
					data.data(), data.length() );

	if (!pResult)
	{
		ERROR_MSG( "UserDataType::createFromStream: Script call failed.\n" );
		PyErr_PrintEx(0);
		return NULL;
	}

	return PyObjectPtr( pResult, PyObjectPtr::STEAL_REFERENCE );
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::addToSection
 */
void UserDataType::addToSection( PyObject * pNewValue,
	DataSectionPtr pSection ) const
{
	// TODO: Use this->method + script::ask
	PyObject * pObject = this->pObject();

	if (!pObject)
	{
		ERROR_MSG( "UserDataType::addToSection: "
				"Do not have %s.%s\n",
			moduleName_.c_str(), instanceName_.c_str() );
		// TODO: Should be able to return an error.
		return;
	}

	PyObject * pPySection = new PyDataSection( pSection );

	PyObjectPtr pResult(
		PyObject_CallMethod( pObject, "addToSection",
								"OO", pNewValue, pPySection ),
		PyObjectPtr::STEAL_REFERENCE );

	Py_DECREF( pPySection );

	if (!pResult)
	{
		ERROR_MSG( "UserDataType::addToSection: (%s,%s) "
					"Script call failed.\n",
				moduleName_.c_str(), instanceName_.c_str() );
		PyErr_PrintEx(0);
		// TODO: Should be able to return an error.
		return;
	}
}

/**
 *	Overrides the DataType method.
 *
 *	@see DataType::createFromSection
 */
PyObjectPtr UserDataType::createFromSection( DataSectionPtr pSection ) const
{
	// TODO: Use this->method + script::ask
	if (!pSection)
	{
		ERROR_MSG( "UserDataType::createFromSection: (%s.%s) "
					"Section is NULL.\n",
				moduleName_.c_str(), instanceName_.c_str() );
		return NULL;
	}

	PyObject * pObject = this->pObject();

	if (!pObject)
	{
		ERROR_MSG( "UserDataType::createFromSection: "
				"Do not have %s.%s\n",
			moduleName_.c_str(), instanceName_.c_str() );
		return NULL;
	}

	PyObject * pPySection = new PyDataSection( pSection );

	PyObject * pResult =
		PyObject_CallMethod( pObject, "createFromSection", "O",
						pPySection );
	Py_DECREF( pPySection );

	if (!pResult)
	{
		ERROR_MSG( "UserDataType::createFromSection: "
						"Script call failed.\n" );
		PyErr_PrintEx(0);
		return NULL;
	}

	return PyObjectPtr( pResult, PyObjectPtr::STEAL_REFERENCE );
}


bool UserDataType::fromStreamToSection( BinaryIStream & stream,
		DataSectionPtr pSection, bool isPersistentOnly ) const
{
	PyObject * pMethod = this->method( "fromStreamToSection" );
	if (pMethod == NULL)
		return this->DataType::fromStreamToSection( stream, pSection,
													isPersistentOnly );

	std::string data;
	stream >> data;
	if (stream.error())
	{
		ERROR_MSG( "UserDataType::fromStreamToSection: "
		   "Bad string on stream\n" );
		Py_DECREF( pMethod );
		return false;
	}

	PyObject * pPySection = new PyDataSection( pSection );

	PyObject * pResult = Script::ask(
		pMethod,
		Py_BuildValue( "s#O", data.data(), data.length(), pPySection ),
		"UserDataType::fromStreamToSection: " );

	Py_DECREF( pPySection );

	if (pResult == NULL) return false;

	Py_DECREF( pResult );
	return true;
}

bool UserDataType::fromSectionToStream( DataSectionPtr pSection,
		BinaryOStream & stream, bool isPersistentOnly ) const
{
	PyObject * pMethod = this->method( "fromSectionToStream" );
	if (pMethod == NULL)
		return this->DataType::fromSectionToStream( pSection, stream,
													isPersistentOnly );

	if (!pSection) return false;

	PyObject * pPySection = new PyDataSection( pSection );

	PyObject * pResult = Script::ask(
		pMethod,
		Py_BuildValue( "(O)", pPySection ),
		"UserDataType::fromSectionToStream: " );

	Py_DECREF( pPySection );

	if (!pResult)
	{
		stream << "";	// hmmm
		return false;
	}

	if (!PyString_Check( pResult ))
	{
		ERROR_MSG( "UserDataType::fromSectionToStream: (%s.%s) "
					"Method did not return a string.\n",
				moduleName_.c_str(), instanceName_.c_str() );
		Py_DECREF( pResult );
		stream << "";	// hmmm
		return false;
	}

	std::string s;
	Script::setData( pResult, s );
	Py_DECREF( pResult );

	stream << s;
	return true;
}

void UserDataType::addToMD5( MD5 & md5 ) const
{
	md5.append( "User", sizeof( "User" ) );
	md5.append( moduleName_.c_str(), moduleName_.size() );
	md5.append( instanceName_.c_str(), instanceName_.size() );
}

bool UserDataType::operator<( const DataType & other ) const
{
	if (this->DataType::operator<( other )) return true;
	if (other.DataType::operator<( *this )) return false;

	// ok, equal metas, so downcast other and compare with us
	UserDataType & otherUser = (UserDataType&)other;
	int diff = (moduleName_ + instanceName_).compare(
					otherUser.moduleName_ + otherUser.instanceName_ );
	if (diff < 0) return true;
	if (diff > 0) return false;
	// much more stable to compare strings than PyObjectPtrs

	if (otherUser.pDefaultSection_)
		return otherUser.pDefaultSection_->compare( pDefaultSection_ ) > 0;
	// else we are equal or greater than other.
	return false;
}

std::string UserDataType::typeName() const
{
	return this->DataType::typeName() + " by " + moduleName_ + "." +
		instanceName_;
}

void UserDataType::init()
{
	isInited_ = true;
	PyObjectPtr pModule(
			PyImport_ImportModule(
				const_cast< char * >( moduleName_.c_str() ) ),
			PyObjectPtr::STEAL_REFERENCE );

	if (pModule)
	{
		pObject_ = PyObject_GetAttrString( pModule.getObject(),
						const_cast< char * >( instanceName_.c_str() ) );
		if (!pObject_)
		{
			ERROR_MSG( "UserDataType::pObject: "
									"Unable to import %s from %s\n",
				instanceName_.c_str(), moduleName_.c_str() );
			PyErr_PrintEx(0);
		}
		else
		{
			Py_DECREF( pObject_.getObject() );
		}
	}
	else
	{
		ERROR_MSG( "UserDataType::pObject: Unable to import %s\n",
			moduleName_.c_str() );
		PyErr_PrintEx(0);
	}
}

inline PyObject * UserDataType::method( const char * name ) const
{
	PyObject * pObject = this->pObject();

	if (!pObject)
	{
		ERROR_MSG( "UserDataType: Do not have %s.%s\n",
			moduleName_.c_str(), instanceName_.c_str() );
		return NULL;
	}

	PyObject * pMethod = PyObject_GetAttrString(
		pObject, const_cast<char*>( name ) );
	if (pMethod == NULL) { PyErr_Clear(); }
	return pMethod;
}


// -----------------------------------------------------------------------------
// Section: UserMetaDataType
// -----------------------------------------------------------------------------

/**
 *	This class implements an object that creates user defined (in script) data
 *	types.
 *
 *	An alias should usually be defined for these types.
 *
 *	In alias.xml, you could have the following:
 *	@code
 *	<ITEM>
 *		USER_TYPE <implementedBy> ItemDataType.instance </implementedBy>
 *	</ITEM>
 *	@endcode
 *
 *	The object ItemDataType.instance needs to support a set of data type
 *	methods.
 *
 *	@see UserDataType
 */
class UserMetaDataType : public MetaDataType
{
public:
	UserMetaDataType()
	{
		MetaDataType::addMetaType( this );
	}
	virtual ~UserMetaDataType()
	{
		MetaDataType::delMetaType( this );
	}

	virtual const char * name() const
	{
		return "USER_TYPE";
	}

	virtual DataTypePtr getType( DataSectionPtr pSection )
	{
		std::string implementedBy = pSection->readString( "implementedBy" );

		if (implementedBy.empty())
		{
			ERROR_MSG( "UserMetaDataType::getType: "
										"implementedBy is not specified.\n" );
			return NULL;
		}

		std::string::size_type pos = implementedBy.find_last_of( "." );

		if (pos == std::string::npos)
		{
			ERROR_MSG( "UserMetaDataType::getType: "
								"Invalid implementedBy %s - must contain a '.'",
							implementedBy.c_str() );
			return NULL;
		}


		std::string moduleName = implementedBy.substr( 0, pos );
		std::string instanceName = implementedBy.substr( pos + 1,
				implementedBy.size() );

		return new UserDataType( this, moduleName, instanceName );
	}
};

static UserMetaDataType s_USER_TYPE_metaDataType;


// -----------------------------------------------------------------------------
// Section: BlobDataType
// -----------------------------------------------------------------------------

/**
 *	This class is used to represent a blob data type. It is similar to the
 *	string data type but writes itself differently to sections.
 */
class BlobDataType : public StringDataType
{
public:
	/**
	 *	Constructor.
	 */
	BlobDataType( MetaDataType * pMeta ) : StringDataType( pMeta )
	{
	}

	virtual void addToSection( PyObject * pNewValue, DataSectionPtr pSection )
		const
	{
		int size = PyString_Size( pNewValue );
		std::string newString( PyString_AsString( pNewValue ), size );
		pSection->setBlob( newString );
	}

	virtual PyObjectPtr createFromSection( DataSectionPtr pSection ) const
	{
		std::string blobStr = pSection->asBlob();
		return PyObjectPtr(
			PyString_FromStringAndSize( blobStr.data(), blobStr.length() ),
			PyObjectPtr::STEAL_REFERENCE );
	}


	virtual bool fromStreamToSection( BinaryIStream & stream,
			DataSectionPtr pSection, bool isPersistentOnly ) const
	{
		std::string value;
		stream >> value;
		if (stream.error()) return false;

		pSection->setBlob( value );
		return true;
	}

	virtual bool fromSectionToStream( DataSectionPtr pSection,
				BinaryOStream & stream, bool isPersistentOnly ) const
	{
		stream << pSection->asBlob();
		return true;
	}

	virtual void addToMD5( MD5 & md5 ) const
	{
		md5.append( "Blob", sizeof( "Blob" ) );
	}
};

SIMPLE_DATA_TYPE( BlobDataType, BLOB )


// -----------------------------------------------------------------------------
// Section: MailBoxDataType
// -----------------------------------------------------------------------------

typedef SmartPointer<PyObject> PyObjectPtr;

/**
 *	This class is used to represent a mailbox data type.
 *
 *	@ingroup entity
 */
class MailBoxDataType : public DataType
{
public:
	MailBoxDataType( MetaDataType * pMeta ) : DataType( pMeta ) { }

protected:
	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::isSameType
	 */
	virtual bool isSameType( PyObject * pValue )
	{
		return PyEntityMailBox::reducibleToRef( pValue );
	}

	/**
	 *	This method sets the default value for this type.
	 *
	 *	@see DataType::setDefaultValue
	 */
	virtual void setDefaultValue( DataSectionPtr pSection )
	{
		pDefaultValue_ = Py_None;
		if (pSection)
			WARNING_MSG( "MailBoxDataType::setDefaultValue: Default value for "
						"mailbox not supported\n" );
		// __kyl__ (14/06/2006) The following line causes a circular dependency
		// as PyEntityMailBox::constructFromRef() depends on EntityType being
		// initialised, which we are in the middle of doing.
//		pDefaultValue_ = (pSection) ? this->createFromSection( pSection ) :
//							Py_None;
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::pDefaultValue
	 */
	virtual PyObjectPtr pDefaultValue() const
	{
		return pDefaultValue_;
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::addToStream
	 */
	virtual void addToStream( PyObject * pNewValue,
			BinaryOStream & stream, bool /*isPersistentOnly*/ ) const
	{
		EntityMailBoxRef mbr;
		MailBoxDataType::from( pNewValue, mbr );
		MailBoxDataType::to( mbr, stream );
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::createFromStream
	 */
	virtual PyObjectPtr createFromStream( BinaryIStream & stream,
			bool /*isPersistentOnly*/ ) const
	{
		EntityMailBoxRef mbr;
		MailBoxDataType::from( stream, mbr );

		if (stream.error())
		{
			ERROR_MSG( "MailBoxDataType::createFromStream: "
					   "Not enough data on stream to read EntityMailBoxRef\n" );
			return NULL;
		}

		PyObjectPtr pob;
		MailBoxDataType::to( mbr, pob );
		return pob;
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::addToSection
	 */
	virtual void addToSection( PyObject * pNewValue, DataSectionPtr pSection )
		const
	{
		EntityMailBoxRef mbr;
		MailBoxDataType::from( pNewValue, mbr );
		MailBoxDataType::to( mbr, pSection );
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::createFromSection
	 */
	virtual PyObjectPtr createFromSection( DataSectionPtr pSection ) const
	{
		EntityMailBoxRef mbr;
		MailBoxDataType::from( pSection, mbr );

		PyObjectPtr pob;
		MailBoxDataType::to( mbr, pob );
		return pob;
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::fromStreamToSection
	 */
	virtual bool fromStreamToSection( BinaryIStream & stream,
			DataSectionPtr pSection, bool /*isPersistentOnly*/ ) const
	{
		EntityMailBoxRef mbr;
		MailBoxDataType::from( stream, mbr );
		MailBoxDataType::to( mbr, pSection );
		return true;
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::fromSectionToStream
	 */
	virtual bool fromSectionToStream( DataSectionPtr pSection,
			BinaryOStream & stream, bool /*isPersistentOnly*/ ) const
	{
		if (!pSection) return false;
		EntityMailBoxRef mbr;
		MailBoxDataType::from( pSection, mbr );
		MailBoxDataType::to( mbr, stream );
		return true;	// if missing then just added empty mailbox
	}


	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::addToMD5
	 */
	virtual void addToMD5( MD5 & md5 ) const
	{
		md5.append( "MailBox", sizeof( "MailBox" ) );
	}

	virtual bool operator<( const DataType & other ) const
	{
		if (this->DataType::operator<( other )) return true;
		if (other.DataType::operator<( *this )) return false;

		const MailBoxDataType& otherMB =
				static_cast< const MailBoxDataType& >( other );
		return (Script::compare( pDefaultValue_.getObject(),
			otherMB.pDefaultValue_.getObject() ) < 0);
	}

private:
	PyObjectPtr pDefaultValue_;

	/// Helper method from Python
	static void from( PyObject * pObject, EntityMailBoxRef & mbr )
	{
		mbr = PyEntityMailBox::reduceToRef( pObject );
	}

	/// Helper method to Python
	static void to( const EntityMailBoxRef & mbr, PyObjectPtr & pObject )
	{
		pObject = PyObjectPtr( PyEntityMailBox::constructFromRef( mbr ), true );
	}

	/// Helper method from Stream
	static void from( BinaryIStream & stream, EntityMailBoxRef & mbr )
	{
		stream >> mbr;
	}

	/// Helper method to Stream
	static void to( const EntityMailBoxRef & mbr, BinaryOStream & stream )
	{
		stream << mbr;
	}

	/// Helper method from Section
	static void from( DataSectionPtr pSection, EntityMailBoxRef & mbr )
	{
		mbr.id = pSection->readInt( "id" );
		mbr.addr.ip = pSection->readInt( "ip" );
		(uint32&)mbr.addr.port = pSection->readInt( "etc" );
	}

	/// Helper method to Section
	static void to( const EntityMailBoxRef & mbr, DataSectionPtr pSection )
	{
		pSection->writeInt( "id", mbr.id );
		pSection->writeInt( "ip", mbr.addr.ip );
		pSection->writeInt( "etc", (uint32&)mbr.addr.port );
	}
};

/// Datatype for MailBoxes.
SIMPLE_DATA_TYPE( MailBoxDataType, MAILBOX )


#if 0
// TODO: Should consider whether we want this datatype. The main issue with it
// at the moment is similar to the list. If a modification is made to it, it
// is not propagated. Only when the top level property is set to a new
// dictionary.

// -----------------------------------------------------------------------------
// Section: DictionaryDataType
// -----------------------------------------------------------------------------

/**
 *	This class is used for dictionary data types.
 */
class DictionaryDataType : public DataType
{
public:
	DictionaryDataType( DataType & keyType, DataType & valueType ) :
		keyType_( keyType ),
		valueType_( valueType )
	{
		this->typeName( "DICT: " +
				keyType_.typeName() + valueType_.typeName() );
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::isSameType
	 */
	virtual bool isSameType( PyObject * pValue )
	{
		if (!PyDict_Check( pValue ))
		{
			ERROR_MSG( "DictionaryDataType::isSameType: Not a dictionary.\n" );
			if (pValue != NULL)
			{
				PyObject * pAsStr = PyObject_Str( pValue );

				if (pAsStr)
				{
					ERROR_MSG( "\tpValue = %s\n",
							PyString_AsString( pAsStr ) );
					Py_DECREF( pAsStr );
				}
			}

			return false;
		}

		int pos = 0;
		PyObject * pKey;
		PyObject * pEntry;

		while (PyDict_Next( pValue, &pos, &pKey, &pEntry ))
		{
			if (pKey && pValue)
			{
				if (!keyType_.isSameType( pKey ))
				{
					ERROR_MSG( "DictionaryDataType::isSameType: Bad key.\n" );
					return false;
				}

				if (!valueType_.isSameType( pEntry ))
				{
					ERROR_MSG( "DictionaryDataType::isSameType: Bad value.\n" );
					return false;
				}
			}
		}

		return true;
	}

	virtual void reloadScript()
	{
		keyType_.reloadScript();
		valueType_.reloadScript();
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::pDefaultValue
	 */
	virtual PyObjectPtr pDefaultValue()
	{
		return PyObjectPtr( PyDict_New(), PyObjectPtr::STEAL_REFERENCE );
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::addToStream
	 */
	virtual void addToStream( PyObject * pNewValue,
			BinaryOStream & stream, bool isPersistentOnly )
	{
		int size = PyDict_Size( pNewValue );
		stream << size;

		int pos = 0;
		PyObject * pKey;
		PyObject * pValue;

		while (PyDict_Next( pNewValue, &pos, &pKey, &pValue ))
		{
			if (pKey && pValue)
			{
				keyType_.addToStream( pKey, stream, isPersistentOnly );
				valueType_.addToStream( pValue, stream, isPersistentOnly );
			}
		}
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::createFromStream
	 */
	virtual PyObjectPtr createFromStream( BinaryIStream & stream,
			bool isPersistentOnly ) const
	{
		int size;
		stream >> size;

		if (size > stream.remainingLength())
		{
			ERROR_MSG( "DictionaryDataType::createFromStream: "
					   "Size (%d) is greater than remainingLength() (%d)\n",
					   size, stream.remainingLength() );
			return NULL;
		}

		PyObject * pDict = PyDict_New();

		for (int i = 0; i < size; i++)
		{
			PyObject * pKey =
				keyType_.createFromStream( stream, isPersistentOnly );

			if (pKey)
			{
				PyObject * pValue =
					valueType_.createFromStream( stream, isPersistentOnly );

				if (pValue)
				{
					if (PyDict_SetItem( pDict, pKey, pValue ) == -1)
					{
						ERROR_MSG( "DictionaryDataType::createFromStream: "
								"Insert failed.\n" );
						return NULL;
					}

					Py_DECREF( pValue );
				}
				else
				{
					ERROR_MSG( "DictionaryDataType::createFromStream: "
							"Bad key.\n" );
					return NULL;
				}

				Py_DECREF( pKey );
			}
			else
			{
				ERROR_MSG( "DictionaryDataType::createFromStream: Bad key.\n" );
				return NULL;
			}
		}

		return PyObjectPtr( pDict, PyObjectPtr::STEAL_REFERENCE );
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::addToSection
	 */
	virtual void addToSection( PyObject * pNewValue, DataSectionPtr pSection )
		const
	{
		int pos = 0;
		PyObject * pKey;
		PyObject * pValue;

		while (PyDict_Next( pNewValue, &pos, &pKey, &pValue ))
		{
			if (pKey && pValue)
			{
				DataSectionPtr pItemSection = pSection->newSection( "item" );
				DataSectionPtr pKeySection = pItemSection->newSection( "key" );
				keyType_.addToSection( pKey, pKeySection );

				DataSectionPtr pValueSection =
					pItemSection->newSection( "value" );
				valueType_.addToSection( pValue, pValueSection );
			}
		}
	}

	/**
	 *	Overrides the DataType method.
	 *
	 *	@see DataType::createFromSection
	 */
	virtual PyObjectPtr createFromSection( DataSectionPtr pSection )
	{
		if (!pSection)
		{
			ERROR_MSG( "ArrayDataType::createFromSection: "
					"Section is NULL.\n" );
			return NULL;
		}

		PyObject * pDict = PyDict_New();

		DataSection::iterator iter = pSection->begin();

		while (iter != pSection->end())
		{
			DataSectionPtr pItem = *iter;

			if (pItem->sectionName() == "item")
			{
				DataSectionPtr pKeySection = pItem->findChild( "key" );

				if (pKeySection)
				{
					DataSectionPtr pValueSection = pItem->findChild( "value" );

					if (pValueSection)
					{
						PyObject * pKey =
							keyType_.createFromSection( pKeySection );
						PyObject * pValue =
							valueType_.createFromSection( pValueSection );

						if (pKey && pValue)
						{
							PyDict_SetItem( pDict, pKey, pValue );
						}

						Py_XDECREF( pKey );
						Py_XDECREF( pValue );
					}
					else
					{
						ERROR_MSG( "DictionaryDataType::createFromSection: "
								"No value section\n" );
					}
				}
				else
				{
					ERROR_MSG( "DictionaryDataType::createFromSection: "
							"No key section\n" );
				}
			}
			else
			{
				ERROR_MSG( "DictionaryDataType::createFromSection: "
						"Bad section '%s'\n", (*iter)->sectionName().c_str() );
			}

			++iter;
		}

		return PyObjectPtr( pDict, PyObjectPtr::STEAL_REFERENCE );
	}


	virtual bool fromStreamToSection( BinaryIStream & stream,
			DataSectionPtr pSection, bool isPersistentOnly )
	{
		int size;
		stream >> size;

		for (int i = 0; i < size; i++)
		{
			DataSectionPtr pItemSection = pSection->newSection( "item" );
			DataSectionPtr pKeySection = pItemSection->newSection( "key" );
			keyType_.fromStreamToSection( stream, pKeySection,
											isPersistentOnly );

			DataSectionPtr pValueSection = pItemSection->newSection( "value" );
			valueType_.fromStreamToSection( stream, pValueSection,
											isPersistentOnly );
		}

		return true;
	}

	virtual void addToMD5( MD5 & md5 ) const
	{
		md5.append( "Dict", sizeof( "Dict" ) );
		keyType_.addToMD5( md5 );
		valueType_.addToMD5( md5 );
	}

private:
	DataType & keyType_;
	DataType & valueType_;
};

class DictionaryMetaDataType : public MetaDataType
{
public:
	DictionaryMetaDataType() : MetaDataType( "DICTIONARY" ) {}

	virtual DataTypePtr getType( DataSectionPtr pSection )
	{
		DataType * pKeyType =
			DataType::fromDataSection( pSection->openSection( "key" ) );

		if (!pKeyType)
		{
			ERROR_MSG( "DictionaryMetaDataType::getType: Bad key type.\n" );
			return NULL;
		}

		DataType * pValueType =
			DataType::fromDataSection( pSection->openSection( "value" ) );

		if (!pValueType)
		{
			ERROR_MSG( "DictionaryMetaDataType::getType: Bad value type.\n" );
			return NULL;
		}

		return new DictionaryDataType( *pKeyType, *pValueType );
	}
};

static DictionaryMetaDataType s_DICT_metaDataType;

#endif

// data_types.cpp

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

#include "chunk/chunk_item.hpp"
#include "chunk/editor_chunk_item.hpp"
#include "properties_helper.hpp"
#include "array_properties_helper.hpp"
#include "resmgr/xml_section.hpp"
#include "entitydef/base_user_data_object_description.hpp"
#include "entitydef/data_types.hpp"


/**
 *	The constructor for this class only initialises it's pointer members.
 */
PropertiesHelper::PropertiesHelper() :
	BasePropertiesHelper(),
	pType_( NULL ),
	pDict_( NULL )
{
}


/**
 *	This method initialises the helper with the values it needs to be able
 *	to get/set the properties.
 *
 *	@param pItem			Chunk item corresponding to the entity.
 *	@param pType			Type desctiption of the entity.
 *	@param pDict			Dictionary of properties of the entity.
 *	@param changedCallback	Optional callback, called when a property changes.
 */
void PropertiesHelper::init(
	EditorChunkItem* pItem,
	BaseUserDataObjectDescription* pType,
	PyObject* pDict,
	BWBaseFunctor1<int>* changedCallback /*=NULL*/ )
{
	pItem_ = pItem;
	pType_ = pType;
	pDict_ = pDict;
	changedCallback_ = changedCallback;
}


/**
 *	This method returns the type info of the owner of this property helper.
 *
 *	@return		Type info of the owner of this property helper
 */
BaseUserDataObjectDescription* PropertiesHelper::pType() const
{
	return pType_;
}


/**
 *	This method returns the name of the property at the passed index.
 *
 *	@param	index	The index of the property in pType.
 *	@return	The name of the property at the passed index in pType.
 */
std::string PropertiesHelper::propName( PropertyIndex index )
{
	if ( pType_ == NULL || index.empty() )
		return "";

	MF_ASSERT( index.valueAt(0) < (int) pType_->propertyCount() );
	PyObjectPtr ob( propGetPy( index.valueAt(0) ), PyObjectPtr::STEAL_REFERENCE );
	DataDescription* pDD = pType_->property( index.valueAt(0) );

	std::string result = pDD->name();
	if ( PySequence_Check( ob.getObject() ) && index.count() > 1 )
	{
		SequenceDataType* dataType = static_cast<SequenceDataType*>( pDD->dataType() );
		ArrayPropertiesHelper propArray;
		propArray.init( pItem(), &(dataType->getElemType()), ob.getObject() );
		PropertyIndex arrayIndex = index.tail();
		result = result + propArray.propName( arrayIndex );
	}

	return result;
}


/**
 *	This method returns the number of properties in pType.
 *
 *	@return	The number of properties in pType.
 */
int PropertiesHelper::propCount() const
{
	if ( pType_ == NULL )
		return 0;

    return (int)pType_->propertyCount();
}


/**
 *	This method returns the index of the named property.
 *
 *	@param	name	The name of the property.
 *	@return	The index of the named property.
 */
PropertyIndex PropertiesHelper::propGetIdx( const std::string& name ) const
{
	// find brackets, in case it's an array element, and if so, extract the
	// type name from it.
	uint32 bracket = name.find_first_of( '[' );
	if ( bracket == std::string::npos )
		bracket = name.size();
	std::string typeName = name.substr( 0, bracket );

	// get the property
	PropertyIndex result;
	DataDescription* pDD = pType_->findProperty( typeName );
	if (pDD)
	{
		result.append( pDD->index() );
	}

	// if no brackets, we can return as it's not an array.
	if ( bracket == name.size() )
		return result;

	// find out if it's a valid python sequence
	PyObjectPtr ob( const_cast<PropertiesHelper*>(this)->propGetPy( result.valueAt(0) ),
		PyObjectPtr::STEAL_REFERENCE );

	if ( PySequence_Check( ob.getObject() ) )
	{
		// it's a sequence, so call propGetIdx in the array using the
		// ArrayPropertiesHelper, and append the return value to the result
		// PropertyIndex object.
		SequenceDataType* dataType = static_cast<SequenceDataType*>( pDD->dataType() );
		ArrayPropertiesHelper propArray;
		propArray.init( pItem(), &(dataType->getElemType()), ob.getObject() );
		PropertyIndex arrayResult = propArray.propGetIdx( name );
		if ( !arrayResult.empty() )
		{
			result.append( arrayResult );
		}
	}

	return result;
}


/**
 *	This method returns the property in PyObject form (New Reference).
 *
 *	@param	index	The index of the property in pType.
 *	@return	The property in PyObject form (New Reference).
 */
PyObject* PropertiesHelper::propGetPy( PropertyIndex index )
{
	if ( pType_ == NULL || index.empty() )
		return NULL;

	MF_ASSERT( index.valueAt(0) < (int) pType_->propertyCount() );
	DataDescription * pDD = pType_->property( index.valueAt(0) );

	PyObject* pValue = PyDict_GetItemString( pDict_,
			const_cast<char*>( pDD->name().c_str() ) );

	if ( index.count() > 1 && PySequence_Check( pValue ) )
	{
		// requesting an array element, so call propGetPy on the array using
		// the ArrayPropertiesHelper.
		SequenceDataType* dataType = static_cast<SequenceDataType*>( pDD->dataType() );
		ArrayPropertiesHelper propArray;
		propArray.init( pItem(), &(dataType->getElemType()), pValue );
		PropertyIndex arrayIndex = index.tail();
		pValue = propArray.propGetPy( arrayIndex );
	}
	else
	{
		// must return a new reference
		Py_XINCREF( pValue );
	}

	return pValue;
}


/**
 *	This method sets the PyObject form of a property.
 *
 *	@param	index	The index of the property in pType.
 *	@param	pObj	The PyObject of the property.
 *	@return	Boolean success or failure.
 */
bool PropertiesHelper::propSetPy( PropertyIndex index, PyObject * pObj )
{
	if ( pType_ == NULL || index.empty() )
		return false;

	MF_ASSERT( index.valueAt(0) < (int) pType_->propertyCount() );
	DataDescription * pDD = pType_->property( index.valueAt(0) );

	if ( index.count() == 1 )
	{
		propUsingDefault( index.valueAt( 0 ), false );

		// requesting the top-level property directly, even if it's an array
		// (sequence), so set it directly.
		if (PyDict_SetItemString( pDict_,
			const_cast<char*>( pDD->name().c_str() ), pObj ) == -1)
		{
			ERROR_MSG( "PropertiesHelper::propSetPy: Failed to set value\n" );
			PyErr_Print();
			return false;
		}
	}
	else
	{
		
		PyObjectPtr ob( propGetPy( index.valueAt(0) ), PyObjectPtr::STEAL_REFERENCE );
		if ( PySequence_Check( ob.getObject() ) )
		{
			SequenceDataType* dataType = static_cast<SequenceDataType*>( pDD->dataType() );
			ArrayPropertiesHelper propArray;
			propArray.init( pItem(), &(dataType->getElemType()), ob.getObject() );
			PropertyIndex arrayIndex = index.tail();
			if ( !propArray.propSetPy( arrayIndex, pObj ) )
			{
				ERROR_MSG( "PropertiesHelper::propSetPy: Failed to set value in array\n" );
				PyErr_Print();
				return false;
			}
		}
		else
		{
			ERROR_MSG( "PropertiesHelper::propSetPy: Failed to set value in array (not a sequence)\n" );
			PyErr_Print();
			return false;
		}
	}

	return true;
}


/**
 *	This method returns the property in datasection form.
 *
 *	@param	index	The index of the property in pType.
 *	@return	The property in datasection form.
 */
DataSectionPtr PropertiesHelper::propGet( PropertyIndex index )
{
	if ( pType_ == NULL || index.empty() )
		return NULL;

	MF_ASSERT( index.valueAt(0) < (int) pType_->propertyCount() );
	DataDescription * pDD = pType_->property( index.valueAt(0) );

	PyObjectPtr pValue( propGetPy( index ), PyObjectPtr::STEAL_REFERENCE );

	if (pValue == NULL)
	{
		PyErr_Clear();

		PyObject * pStr = PyObject_Str( pDict_ );
		Py_XDECREF( pStr );

		return NULL;
	}

	DataSectionPtr pTemp = new XMLSection( "temp" );

	if ( index.count() > 1 )
	{
		PyObjectPtr ob( propGetPy( index.valueAt(0) ), PyObjectPtr::STEAL_REFERENCE );
		if ( PySequence_Check( ob.getObject() ) )
		{
			// requesting and array item, so return it.
			SequenceDataType* dataType = static_cast<SequenceDataType*>( pDD->dataType() );
			dataType->getElemType().addToSection( pValue.getObject(), pTemp );
			return pTemp;
		}
	}
	
	pDD->addToSection( pValue.getObject(), pTemp );

	return pTemp;
}


/**
 *	This method sets the datasection form of a property.
 *
 *	@param	index	The index of the property in pType.
 *	@param	pTemp	The datasection form of property.
 *	@return	Boolean success or failure.
 */
bool PropertiesHelper::propSet( PropertyIndex index, DataSectionPtr pTemp )
{
	if ( pType_ == NULL || index.empty() )
		return false;

	MF_ASSERT( index.valueAt(0) < (int) pType_->propertyCount() );
	DataDescription * pDD = pType_->property( index.valueAt(0) );

	if ( index.count() == 1 )
	{
		propUsingDefault( index.valueAt( 0 ), false );

		PyObjectPtr pNewVal = pDD->createFromSection( pTemp );
		if (!pNewVal)
		{
			PyErr_Clear();
			return false;
		}

		PyDict_SetItemString( pDict_,
			const_cast<char*>( pDD->name().c_str() ), &*pNewVal );
		Py_INCREF( &*pNewVal );
	}
	else
	{
		PyObjectPtr ob( propGetPy( index.valueAt(0) ), PyObjectPtr::STEAL_REFERENCE );
		if ( PySequence_Check( ob.getObject() ) )
		{
			SequenceDataType* dataType = static_cast<SequenceDataType*>( pDD->dataType() );
			ArrayPropertiesHelper propArray;
			propArray.init( pItem(), &(dataType->getElemType()), ob.getObject() );
			PropertyIndex arrayIndex = index.tail();
			if ( !propArray.propSet( arrayIndex, pTemp ) )
			{
				ERROR_MSG( "PropertiesHelper::propSet: Failed to set value in array\n" );
				PyErr_Print();
				return false;
			}
		}
		else
		{
			ERROR_MSG( "PropertiesHelper::propSet: Failed to set value in array (not a sequence)\n" );
			PyErr_Print();
			return false;
		}
	}

	if ( changedCallback_ != NULL )
		(*changedCallback_)( index.valueAt(0) );

	return true;
}


/**
 *	This method returns the default value for a property.
 *
 *	@param	index	The index of the property in pType.
 *	@return	The default vaule as a PyObject.
 */
PyObjectPtr PropertiesHelper::propGetDefault( int index )
{
	return pType()->property(index)->dataType()->pDefaultValue();
}


/**
 *	This method sets the property to its default value.
 *
 *	@param	index	The index of the property in pType.
 */
void PropertiesHelper::propSetToDefault( int index )
{
	propSetPy( index, propGetDefault(index).getObject());
}


/**
 *	This method determines which properties are in list names.
 *
 *	@param	names		The list of property names.
 *	@param	allowEdit	The vector of editable propertie flags.
 */
void PropertiesHelper::setEditProps(
	const std::list<std::string>& names, std::vector<bool>& allowEdit )
{
	clearEditProps( allowEdit );

	if (pType_ == NULL)
		return;

	allowEdit.resize( pType_->propertyCount() );

	for (uint i=0; i < pType_->propertyCount(); i++)
	{
		DataDescription * pDD = pType_->property(i);

		allowEdit[ i ] = std::find( names.begin(), names.end(), pDD->name() ) 
								!= names.end();
	}
}


/**
 *	Clears the list of editable properties.
 *
 *	@param	allowEdit	The vector of editable propertie flags.
 */
void PropertiesHelper::clearEditProps( std::vector<bool>& allowEdit )
{
	allowEdit.clear();
}


/**
 *	Clears all properties.
 */
void PropertiesHelper::clearProperties()
{
	pType_ = NULL;
}


/**
 *	Clears the properties section from pOwnSect.
 *
 *	@param	pOwnSect	The datasection to clear.
 */
void PropertiesHelper::clearPropertySection( DataSectionPtr pOwnSect )
{
	DataSectionPtr propertiesSection = pOwnSect->openSection( "properties" );
	
	if (propertiesSection)
		propertiesSection->delChildren();
}


/**
 *	Finds out if a property is a UserDataObject-to-UserDataObject link.
 *
 *	@param index	Property index.
 *	@return			true if it's a UserDataObject-to-UserDataObject link.
 */
bool PropertiesHelper::isUserDataObjectLink( int index )
{
	return pType_->property( index )->dataType()->typeName() == "UDO_REF";
}


/**
 *	Finds if a property is an array of UserDataObject-to-UserDataObject links.
 *
 *	@param index	Property index.
 *	@return			true if it's an array of UserDataObject-to-UserDataObject links.
 */
bool PropertiesHelper::isUserDataObjectLinkArray( int index )
{
	DataDescription* pDD = pType_->property( index );

	if ( pDD->dataType()->typeName().substr(0,8) == "ARRAY of" )
	{
		SequenceDataType* dataType = static_cast<SequenceDataType*>( pDD->dataType() );
		if ( dataType->getElemType().typeName() == "UDO_REF" )
			return true;
	}
	return false;
}


/**
 *	Builds a list of commands for the right click on Editor Chunk Item operation.
 *
 *	@return Returns the list of commands.
 */
std::vector<std::string> PropertiesHelper::command()
{
	propMap_.clear();

	std::vector<std::string> links;
	int index = 0;
	for (int i=0; i < propCount(); i++)
	{
		DataDescription* pDD = pType()->property( i );
		if (!pDD->editable())
			continue;

		if ( isUserDataObjectLink(i) )
		{
			PyObjectPtr ob( propGetPy( i ), PyObjectPtr::STEAL_REFERENCE );
			
			std::string uniqueId = PyString_AsString( PyTuple_GetItem( ob.getObject(), 0 ) );
			if ( !uniqueId.empty() )
			{
				links.push_back( "#"+propName(i) );
				links.push_back( "#"+uniqueId );
				links.push_back( "Delete Link" );
				propMap_[index++] = PropertyIndex( i );
				links.push_back( "##" );
				links.push_back( "##" );
			}
		}
		else if ( isUserDataObjectLinkArray(i) )
		{
			PyObjectPtr ob( propGetPy( i ), PyObjectPtr::STEAL_REFERENCE );

			SequenceDataType* dataType =
				static_cast<SequenceDataType*>( pDD->dataType() );
			ArrayPropertiesHelper propArray;
			propArray.init( pItem(), &(dataType->getElemType()), ob.getObject());

			int numProps = propArray.propCount();
			if ( numProps > 0 )
			{
				links.push_back( "#"+propName(i) );
				links.push_back( "Delete All" );
				propMap_[index++] = PropertyIndex( i );
				links.push_back( "" );
				// Iterate through the array of links
				for(int j = 0; j < numProps; j++)
				{
					PyObjectPtr link( propArray.propGetPy( j ), PyObjectPtr::STEAL_REFERENCE );
					std::string uniqueId = PyString_AsString( PyTuple_GetItem( link.getObject(), 0 ) );
					if ( !uniqueId.empty() )
					{
						links.push_back( "#"+uniqueId );
						links.push_back( "Delete Link" );
						PropertyIndex pi( i );
						pi.append( j );
						propMap_[index++] = pi;
						links.push_back( "##" );
					}
				}
				links.push_back( "##" );
			}
		}
	}	
	return links;
}

/**
 *	This function returns the PropertyIndex that corresponds
 *	to the provided index.
 *
 *	@param index The index from the right click menu
 *	@return The PropertyIndex
 */
PropertyIndex PropertiesHelper::commandIndex( int index )
{
	if ( propMap_.find( index ) != propMap_.end() )
		return propMap_[index];
	return PropertyIndex(-1);
}

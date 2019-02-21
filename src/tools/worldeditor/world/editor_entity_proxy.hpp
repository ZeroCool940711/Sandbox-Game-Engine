/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef EDITOR_ENTITY_PROXY_HPP
#define EDITOR_ENTITY_PROXY_HPP


#include "worldeditor/config.hpp"
#include "worldeditor/forward.hpp"
#include "worldeditor/world/editor_entity_array_properties.hpp"
#include "common/array_properties_helper.hpp"
#include "gizmo/general_properties.hpp"
#include "cstdmf/smartpointer.hpp"


/**
 *	A helper class to access entity INT properties
 */
class EntityIntProxy : public UndoableDataProxy<IntProxy>
{
public:
	enum IntType
	{
		SINT8 = 0,
		UINT8,
		OTHER
	};

	EntityIntProxy( BasePropertiesHelper* props, int index,
		IntType intType = OTHER );

	virtual bool getRange( int& min, int& max ) const;

	virtual uint32 EDCALL get() const;

	virtual void EDCALL setTransient( uint32 i );

	virtual bool EDCALL setPermanent( uint32 i );

	virtual std::string EDCALL opName();

private:
	IntType							intType_;
	int								min_;
	int								max_;
	bool							ranged_;
	BasePropertiesHelper*			props_;
	int								index_;
	uint32							transientValue_;
	bool							useTransient_;
};


/**
 *	A helper class to access entity FLOAT properties
 */
class EntityFloatProxy : public UndoableDataProxy<FloatProxy>
{
public:
	EntityFloatProxy( BasePropertiesHelper* props, int index );

	virtual float EDCALL get() const;

	virtual void EDCALL setTransient( float f );

	virtual bool EDCALL setPermanent( float f );

	virtual std::string EDCALL opName();

private:
	BasePropertiesHelper*			props_;
	int								index_;
	float							transientValue_;
	bool							useTransient_;
};


/**
 *	A helper class to access entity ENUM FLOAT properties
 */
class EntityFloatEnumProxy : public UndoableDataProxy<IntProxy>
{
public:
	EntityFloatEnumProxy( BasePropertiesHelper* props, int index,
		std::map<float,int> enumMap );

	virtual uint32 EDCALL get() const;

	virtual void EDCALL setTransient( uint32 i );

	virtual bool EDCALL setPermanent( uint32 i );

	virtual std::string EDCALL opName();

private:
	BasePropertiesHelper*			props_;
	int								index_;
	uint32							transientValue_;
	bool							useTransient_;
	std::map<float,int>				enumMapString_;
	std::map<int,float>				enumMapInt_;
};


/**
 *	A helper class to access entity STRING properties
 */
class EntityStringProxy : public UndoableDataProxy<StringProxy>
{
public:
	EntityStringProxy( BasePropertiesHelper* props, int index );

	virtual std::string EDCALL get() const;

	virtual void EDCALL setTransient( std::string v );

	virtual bool EDCALL setPermanent( std::string v );

	virtual std::string EDCALL opName();

private:
	BasePropertiesHelper*			props_;
	int								index_;
};


/**
 *	A helper class to access entity ARRAY properties
 */
class EntityArrayProxy : public ArrayProxy
{
public:
	EntityArrayProxy( BasePropertiesHelper* props, DataTypePtr dataType,
		int index );
	virtual ~EntityArrayProxy();

	virtual void elect( GeneralProperty* parent );
	virtual void expel( GeneralProperty* parent );
	virtual void select( GeneralProperty* parent );

	virtual BasePropertiesHelper* propsHelper();
	virtual int index();
	virtual ArrayPropertiesHelper* arrayPropsHelper();

	virtual bool addItem();
	virtual bool delItems();

private:
	BasePropertiesHelper*			props_;
	ArrayPropertiesHelper			array_;
	SequenceDataTypePtr				dataType_;
	int								index_;
	GizmoPtr						gizmo_;
	bool							alwaysShowGizmo_;
	std::vector<GeneralProperty*>	properties_;

	virtual bool deleteArrayItem( int index );
	virtual void delItemNotification( int index );
	virtual void createProperties( GeneralProperty* parent );
	virtual void clearProperties();
};
typedef SmartPointer<EntityArrayProxy> EntityArrayProxyPtr;


/**
 *	A helper class to manage linking through the link gizmo
 */
class EntityArrayLinkProxy : public LinkProxy
{
public:
	EntityArrayLinkProxy( EntityArrayProxy* arrayProxy, EditorChunkItem* item, const std::string& propName );

	virtual LinkType EDCALL linkType() const;

	virtual MatrixProxyPtr EDCALL createCopyForLink();

    virtual TargetState EDCALL canLinkAtPos(ToolLocatorPtr locator) const;

    virtual void EDCALL createLinkAtPos(ToolLocatorPtr locator);

    virtual ToolLocatorPtr EDCALL createLocator() const;    

private:
	EntityArrayProxy* arrayProxy_;
	EditorChunkItem* item_;
	std::string propName_;
};


/**
 *	A helper class to access entity ENUM STRING properties
 */
class EntityStringEnumProxy : public UndoableDataProxy<IntProxy>
{
public:
	EntityStringEnumProxy( BasePropertiesHelper* props, int index,
		std::map<std::string,int> enumMap );

	virtual uint32 EDCALL get() const;

	virtual void EDCALL setTransient( uint32 i );

	virtual bool EDCALL setPermanent( uint32 i );

	virtual std::string EDCALL opName();

private:
	BasePropertiesHelper*			props_;
	int								index_;
	uint32							transientValue_;
	bool							useTransient_;
	std::map<std::string,int>		enumMapString_;
	std::map<int,std::string>		enumMapInt_;
};


/**
 *	A helper class to access the entity specific properties
 */
class EntityPythonProxy : public UndoableDataProxy<PythonProxy>
{
public:
	EntityPythonProxy( BasePropertiesHelper* props, int index );

	virtual PyObjectPtr EDCALL get() const;

	virtual void EDCALL setTransient( PyObjectPtr v );

	virtual bool EDCALL setPermanent( PyObjectPtr v );

	virtual std::string EDCALL opName();

private:
	BasePropertiesHelper*			props_;
	int								index_;
};


#endif // EDITOR_ENTITY_PROXY_HPP

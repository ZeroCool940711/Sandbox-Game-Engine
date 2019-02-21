/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef PY_TEXTURE_PROVIDER_HPP
#define PY_TEXTURE_PROVIDER_HPP

#pragma warning( disable:4786 )

#include "moo/managed_texture.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"


/*~ class BigWorld.PyTextureProvider
 *
 *	This class exposes a texture.  There are several instances where a
 *	texture is required, for example by SimpleGUIComponent.  This has
 *	two attributes, one called textureName, which takes the filepath of a
 *	texture file, the other called texture, which accepts a PyTextureProvider.
 *
 *	A PyTextureProvider can be created using the BigWorld.PyTextureProvider
 *	function, which creates a PyTextureProvider from a texture file.  This can
 *	be useful if a class requires a PyTextureProvider, rather than a texture
 *	name.
 *
 *	Several classes (for example PyModelRenderer and PySceneRenderer)
 *	have an attribute which exposes a PyTextureProvider.  This allows them
 *	to expose a dynamic texture, which gets updated on a tick by tick basis.
 */
/**
 *	This class allows a texture to be passed through python
 */
class PyTextureProvider : public PyObjectPlus
{
	Py_Header( PyTextureProvider, PyObjectPlus )

public:
	PyTextureProvider( PyObject * pOwner, Moo::BaseTexturePtr pTexture,
			PyTypePlus * pType = &s_type_ ) :
		PyObjectPlus( pType ), pOwner_( pOwner ), pTexture_( pTexture ) { }
	PyTextureProvider( PyTypePlus * pType = &s_type_ ) :
		PyObjectPlus( pType ) { }
	~PyTextureProvider() { }

	PyObject *			pyGetAttribute( const char * attr );
	int					pySetAttribute( const char * attr, PyObject * value );

	PY_RO_ATTRIBUTE_DECLARE( pOwner_, owner )
	PyObject * pyGet_width();
	PY_RO_ATTRIBUTE_SET( width );
	PyObject * pyGet_height();
	PY_RO_ATTRIBUTE_SET( height );
	PY_FACTORY_DECLARE()

	virtual Moo::BaseTexturePtr texture() { return pTexture_; }

protected:
	SmartPointer<PyObject>	pOwner_;
	Moo::BaseTexturePtr		pTexture_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyTextureProvider )

#endif // PY_TEXTURE_PROVIDER_HPP

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
#include "matrix_gui_shader.hpp"

#include "simple_gui_component.hpp"
#include "moo/render_context.hpp"	// for frameTimestamp

#ifndef CODE_INLINE
#include "matrix_gui_shader.ipp"
#endif

// -----------------------------------------------------------------------------
// Section: WorldToClipMP
// -----------------------------------------------------------------------------


/**
 *	This class is mainly for the matrix GUI shader, and provides a clip-space
 *	position given a world-space matrix provider.
 */
class WorldToClipMP : public MatrixProvider
{
	Py_Header( WorldToClipMP, MatrixProvider )

public:
	WorldToClipMP( PyTypePlus * pType = &s_type_ ) : MatrixProvider( &s_type_ )
		{ }

	virtual void matrix( Matrix & m ) const;

	PyObject *	pyGetAttribute( const char * attr );
	int			pySetAttribute( const char * attr, PyObject * value );

	PY_RW_ATTRIBUTE_DECLARE( target_, target )

	static WorldToClipMP * New()	{ return new WorldToClipMP(); }
	PY_AUTO_FACTORY_DECLARE( WorldToClipMP, END )

private:
	MatrixProviderPtr	target_;
};

/*~	class GUI.WorldToClipMP
 *
 *	This utility class is a MatrixProvider that provides a 
 *	clip-space position given a world-space matrix provider.
 *	It is primarily used by the MatrixGUIShader.
 */
PY_TYPEOBJECT( WorldToClipMP )

PY_BEGIN_METHODS( WorldToClipMP )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( WorldToClipMP )

	/*~	attribute WorldToClipMP.target
	 *
	 *	@type MatrixProviderPtr	The target MatrixProvider is the matrix that is used 
	 *							to provide the clip-space for this WorldToClipMP.
	 */
	PY_ATTRIBUTE( target )

PY_END_ATTRIBUTES()

/*~	function GUI.WorldToClipMP
 *
 *	Used primarily with the MatrixGUIShader, this returns a new WorldToClip MatrixProider object.
 */
PY_FACTORY( WorldToClipMP, GUI )


/**
 *	WorldToClipMP matrix method
 */
void WorldToClipMP::matrix( Matrix & m ) const
{
	if (target_)
	{
		Vector4 clipPos;
		Matrix tgt;
		target_->matrix( tgt );
		Moo::rc().viewProjection().applyPoint( clipPos, tgt.applyToOrigin() );
		if ( !almostZero( clipPos.w, 0.0001f ) )
		{
			float oow = 1.f / fabsf( clipPos.w );
			m.setTranslate( Vector3( clipPos.x * oow, clipPos.y * oow, 0.f ) );
		}
		else
		{
			//position off the screen
			m.setTranslate( Vector3( -100000.f,-100000.f, 0.f ) );
		}

	}
	else
	{
		m = Matrix::identity;
	}

}


/**
 *	Get python attribute
 */
PyObject * WorldToClipMP::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();
	return MatrixProvider::pyGetAttribute( attr );
}

/**
 *	Set python attribute
 */
int WorldToClipMP::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();
	return MatrixProvider::pySetAttribute( attr, value );
}


// -----------------------------------------------------------------------------
// Section: MatrixGUIShader
// -----------------------------------------------------------------------------

PY_TYPEOBJECT( MatrixGUIShader )

PY_BEGIN_METHODS( MatrixGUIShader )
	PY_METHOD( reset )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( MatrixGUIShader )
	/*~ attribute MatrixGUIShader.target
	 *
	 *	This attribute specifies the MatrixProvider that is used to transform
	 *	the components that this is attached to.  The transformation is applied
	 *	within clip space ( (-1,-1) is the bottom left of the screen, (1,1) is
	 *	the top right).
	 *
	 *	If blend is 0 (false) then any changes to the target MatrixProvider
	 *	are reflected instantaneously in the transformation of the components.  
	 *
	 *	If blend is non-zero (true), then the the components will move from
	 *	their current transformations to the transform specified by the target
	 *	in eta seconds.
	 *
	 *	It defaults to None.
	 *
	 *	@type		MatrixProvider
	 */
	PY_ATTRIBUTE( target )
	/*~ attribute MatrixGUIShader.eta
	 *
	 *	This attribute specifies how many seconds it will take to blend from
	 *	the current transformation of the components to the target transform.
	 *	This is only used if the blend attribute is non-zero.
	 *
	 *	It defaults to 0.0.
	 *
	 *	@type	float
	 */
	PY_ATTRIBUTE( eta )
	/*~ attribute MatrixGUIShader.blend
	 *
	 *	This attribute specifies whether to blend from the current transform to
	 *	that specified by the target MatrixProvider.  If this attribute is
	 *	non-zero (true) then blending is performed, over eta seconds.  If this
	 *	attribute is zero (false) then the target transform is just applied
	 *	instantly.
	 *
	 *	It defaults to 1 (true).
	 *
	 *	@type	integer (treated as boolean)
	 */
	PY_ATTRIBUTE( blend )
PY_END_ATTRIBUTES()

/*~ function GUI.MatrixShader
 *
 *	This function creates a new MatrixGUIShader.  This can be used to transform
 *	the position of one or more GUI Components within clipping space.  
 *
 *	@param	target (optional)	The initial target MatrixProvider.
 *
 *	@return				a new MatrixGUIShader.
 */
PY_FACTORY_NAMED( MatrixGUIShader, "MatrixShader", GUI )

SHADER_FACTORY( MatrixGUIShader )


/**
 *	Constructors.
 */
MatrixGUIShader::MatrixGUIShader( PyTypePlus * pType ) :
	GUIShader( pType ),
	current_( Matrix::identity ),
	target_( NULL ),
	eta_( 0.f ),
	blend_( true ),
	lastUpdated_( -1 )
{
}


MatrixGUIShader::MatrixGUIShader( MatrixProviderPtr pTrn, PyTypePlus * pType ):
	GUIShader( pType ),
	current_( Matrix::identity ),
	target_( pTrn ),
	eta_( 0.f ),
	blend_( false ),
	lastUpdated_( -1 )
{
}

/**
 *	Destructor.
 */
MatrixGUIShader::~MatrixGUIShader()
{
}


/**
 *	Get python attribute
 */
PyObject * MatrixGUIShader::pyGetAttribute( const char * attr )
{
	PY_GETATTR_STD();

	return GUIShader::pyGetAttribute( attr );
}

/**
 *	Set python attribute
 */
int MatrixGUIShader::pySetAttribute( const char * attr, PyObject * value )
{
	PY_SETATTR_STD();

	return GUIShader::pySetAttribute( attr, value );
}


/**
 *	Python factory method
 */
PyObject * MatrixGUIShader::pyNew( PyObject * args )
{
	if (PyTuple_Size( args ) != 0)
	{
		PyErr_SetString( PyExc_TypeError, "GUI.MatrixShader()"
			" takes no arguments" );
		return NULL;
	}

	return new MatrixGUIShader();
}


/**
 *	Apply ourselves to the given component
 */
bool MatrixGUIShader::processComponent( SimpleGUIComponent& component,
	float dTime )
{
	Matrix transformer;

	// change the matrix if desired
	// (and if we haven't yet done so this frame)
	uint32 frameNow = Moo::rc().frameTimestamp();
	if (target_)
	{
		if ( blend_ )
		{		
			if ( lastUpdated_ != frameNow )
			{
				Matrix tgtVal;
				target_->matrix( tgtVal );

				BlendTransform tgtBlend( tgtVal );

				if (eta_ > 0.f)
				{
					current_.blend( dTime / eta_, tgtBlend );
					eta_ = max( eta_ - dTime, 0.f );
				}
				else
				{
					current_ = tgtBlend;
				}
			}

			lastUpdated_ = frameNow;

			// set up the matrix to apply
			current_.output( transformer );
		}
		else
		{
			target_->matrix( transformer );
		}

		// and apply it to the runTimeTransform.
		Matrix m( component.runTimeTransform() );
		m.preMultiply( transformer );
		component.runTimeTransform( m );
	}

	// easy!
	return false;	//don't need to update children - runTimeTransform already persists
}


/*~ function MatrixGUIShader.reset
 *
 *	This method sets the current matrix to the target matrix, immediately.
 */
/**
 *	This method sets the current matrix to the target matrix, immediately.
 */
void MatrixGUIShader::reset()
{
	eta_ = 0.f;
	Matrix tgtVal;
	target_->matrix( tgtVal );
	BlendTransform tgtBlend( tgtVal );
	current_ = tgtBlend;
	lastUpdated_ = Moo::rc().frameTimestamp();
}


/**
 *	Load method
 */
bool MatrixGUIShader::load( DataSectionPtr pSect )
{
	if (!GUIShader::load( pSect )) return false;

	DataSectionPtr tsect = pSect->openSection( "target" );
	if (!tsect)
	{
		target_ = NULL;
	}
	else
	{
		Matrix tgtVal = Matrix::identity;
		if (target_) target_->matrix( tgtVal );
		tgtVal = tsect->asMatrix34( tgtVal );

		PyMatrix * pyM = new PyMatrix();
		pyM->set( tgtVal );
		target_ = pyM;
		Py_DECREF( pyM );
	}

	eta_ = pSect->readFloat( "eta", eta_ );
	blend_ = pSect->readBool( "blend", blend_ );

	return true;
}

/**
 *	Save method
 */
void MatrixGUIShader::save( DataSectionPtr pSect )
{
	GUIShader::save( pSect );

	if (target_)
	{
		Matrix tgtVal;
		target_->matrix( tgtVal );
		pSect->writeMatrix34( "target", tgtVal );
	}

	pSect->writeFloat( "eta", eta_ );
	pSect->writeBool( "blend", blend_ );
}

// matrix_gui_shader.cpp

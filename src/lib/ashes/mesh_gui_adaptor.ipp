/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

// mesh_gui_adaptor.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

// mesh_gui_adaptor.ipp

INLINE const Matrix & MeshGUIAdaptor::getMatrix() const
{
	static Matrix m;
	if ( transform_ )
		transform_->matrix(m);
	return m;
}
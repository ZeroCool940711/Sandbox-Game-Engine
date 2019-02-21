/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

// node.ipp

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif

namespace Moo
{


/**
 *	This method visits this node as if its parent had the input transform.
 */
INLINE void Node::visitSelf( const Matrix& parent )
{
	worldTransform_.multiply( this->transform(), parent );
	transformInBlended_ = false;
}


/**
 * Get the nodes relative transform
 *
 * @return reference to the relative node transform
 *
 */
INLINE
Matrix& Node::transform( )
{
#ifdef _WIN32
	if (transformInBlended_)
	{
		blendTransform_.output( transform_ );
		transformInBlended_ = false;
	}
#endif
	return transform_;
}

/**
 * Get the nodes relative transform
 *
 * @return const reference to the relative node transform
 *
 */
INLINE
const Matrix& Node::transform( ) const
{
#ifdef _WIN32
	if (transformInBlended_)
	{
		blendTransform_.output( const_cast<Node*>(this)->transform_ );
		const_cast<Node*>(this)->transformInBlended_ = false;
	}
#endif
	return transform_;
}

/**
 * Set the nodes relative transform
 *
 * @param m the nodes relative transform
 *
 */
INLINE
void Node::transform( const Matrix& m )
{
	transform_ = m;
	transformInBlended_ = false;
}

/**
 * Get the nodes worldtransform
 *
 * @return the nodes world transform, only valid after the node has been traversed
 */

INLINE
const Matrix& Node::worldTransform( ) const
{
	return worldTransform_;
}


/**
 * Set the world transform
 *
 * @param	w	the nodes world transform.
 */
INLINE
void Node::worldTransform( const Matrix& w )
{
	worldTransform_ = w;
	transformInBlended_ = false;
}


/**
 * Get the nodes parent
 *
 * @return smartpointer to the nodes parent
 */
INLINE
NodePtr Node::parent( ) const
{
	return parent_;
}

/**
 * Get the number of children under this node.
 *
 * @return the number of children attached to this node
 */
INLINE
uint32 Node::nChildren( ) const
{
	return children_.size();
}

/**
 * Get the child indexed by i.
 *
 * @return smartpointer to the child
 */
INLINE
NodePtr Node::child( uint32 i )
{
	IF_NOT_MF_ASSERT_DEV( i < children_.size() )
	{
		return NULL;
	}
	return children_[ i ];
}

/**
 * Get the blendFactor for this node.
 *
 * @param blendCookie cookie to determine if this is the first blend of this node or not.
 *
 * @return the blendRatio
 */
INLINE
float Node::blend( int blendCookie )
{
	return (blendCookie == blendCookie_) ? blendRatio_ : 0.f;
}

/**
 * Set the blendFactor and blend cookie for this node.
 *
 * @param blendCookie the current blendCookie
 * @param blendRatio the current blendratio for this node
 */
INLINE
void Node::blend( int blendCookie, float blendRatio )
{
	blendCookie_ = blendCookie;
	blendRatio_ = blendRatio;
	transformInBlended_ = true;
}


/**
 *	Explicilty set the blend cookie and a simple transform for this node
 */
INLINE void Node::blendClobber( int blendCookie, const Matrix & m )
{
	blendCookie_ = blendCookie;
	blendRatio_ = 1.0f;
	transform_ = m;
	transformInBlended_ = false;
}


/**
 * Get this nodes identifier
 *
 * @return identifier of the node
 */
INLINE
const std::string& Node::identifier( ) const
{
	return identifier_;
}

#ifdef _WIN32
/**
 *	Get a reference to the blend transform for this node
 */
INLINE BlendTransform & Node::blendTransform()
{
	return blendTransform_;
}
#endif


}

// node.ipp

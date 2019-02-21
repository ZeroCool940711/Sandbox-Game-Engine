/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef SKELETON_COLLIDER_HPP
#define SKELETON_COLLIDER_HPP

#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/py_data_section.hpp"
#include "box_attachment.hpp"


/**
 *	This class is a utility class designed to help
 *	implement collisions on dynamic skeletal objects.
 *
 *	It has a list of BoxAttachment objects, and provides
 *	a C++ and python interface to coordinate collision
 *	testing for that list as a whole.
 */
class SkeletonCollider : public PyObjectPlus
{
	Py_Header( SkeletonCollider, PyObjectPlus )

public:
	typedef SmartPointer<BoxAttachment> ColliderPtr;
	typedef std::vector<ColliderPtr> Colliders;

	SkeletonCollider( PyTypePlus * pType = &s_type_ );
	~SkeletonCollider();

	PY_FACTORY_DECLARE()

	void addCollider( ColliderPtr collider ) { colliders_.push_back( collider ); }
	BoxAttachment* getCollider( uint32 n ) { return n < colliders_.size() ? colliders_[n].getObject() : NULL ; }
	uint32 nColliders() { return colliders_.size(); }

	bool	collide( const Vector3& start, const Vector3& direction, float& distance );

	bool doCollide( Vector3 start, Vector3 direction )
	{
		float distance = 0;
		return collide( start, direction, distance );
	}

	void save( DataSectionPtr pSection );
	void load( DataSectionPtr pSection );

	void save( PyDataSectionPtr pSection ) { this->save( pSection->pSection() ); }
	void load( PyDataSectionPtr pSection ) { this->load( pSection->pSection() ); }

	PY_AUTO_METHOD_DECLARE( RETVOID, addCollider, ARG( ColliderPtr, END ) )
	PY_AUTO_METHOD_DECLARE( RETDATA, getCollider, ARG( uint32, END ) )
	PY_AUTO_METHOD_DECLARE( RETDATA, nColliders,  END )
	PY_AUTO_METHOD_DECLARE( RETDATA, doCollide, ARG( Vector3, ARG(Vector3, END ) ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, save, ARG( PyDataSectionPtr, END ) )
	PY_AUTO_METHOD_DECLARE( RETVOID, load, ARG( PyDataSectionPtr, END ) )
	PY_RO_ATTRIBUTE_DECLARE( impactPoint_, impactPoint )
	PY_RO_ATTRIBUTE_DECLARE( impactReflection_, impactReflection )
	PY_RO_ATTRIBUTE_DECLARE( impactCollider_, impactCollider )
    
	PyObject * pyGetAttribute( const char * attr );
	int pySetAttribute( const char * attr, PyObject * value );
private:
	Colliders	colliders_;
	Vector3		impactPoint_;
	Vector3		impactReflection_;
	ColliderPtr	impactCollider_;

	SkeletonCollider( const SkeletonCollider& );
	SkeletonCollider& operator=( const SkeletonCollider& );
};



#endif // SKELETON_COLLIDER_HPP

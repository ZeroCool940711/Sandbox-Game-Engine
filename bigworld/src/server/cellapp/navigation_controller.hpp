/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef NAV_CONTROLLER_HPP
#define NAV_CONTROLLER_HPP

#include "controller.hpp"
#include "network/basictypes.hpp"
#include "updatable.hpp"
#include "waypoint/navigator.hpp"
#include <vector>

typedef SmartPointer<Entity> EntityPtr;

/**
 * This controller moves an entity to the destination point along the navigation mesh.
 */
class NavigationController : public Controller, public Updatable
{
	DECLARE_CONTROLLER_TYPE( NavigationController )
public:
	NavigationController( const Position3D& destination = Position3D(0, 0, 0),
							float velocity = 0.f,
							bool faceMovement = true,
							float maxDistance = 500.f,
							float girth = 0.5f,
							float closeEnough = 0.01f);

protected:
	virtual void	startReal( bool isInitialStart );
	virtual void	stopReal( bool isFinalStop );

	int				move();
	void			writeRealToStream( BinaryOStream& stream );
	bool 			readRealFromStream( BinaryIStream& stream );
	void			update();

	float metresPerTick_;
	float maxDistance_;
	float girth_;
	float closeEnough_;
	bool  faceMovement_;
	Position3D nextPosition_;
	Position3D destination_;
	NavLoc* pDstLoc_;
	std::vector<Vector3> path_;
	int currentNode_;
};

#endif //NAV_CONTROLLER_HPP

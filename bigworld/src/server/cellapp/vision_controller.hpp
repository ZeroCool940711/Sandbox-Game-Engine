/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef VISION_CONTROLLER_HPP
#define VISION_CONTROLLER_HPP

#include "controller.hpp"
#include "updatable.hpp"

typedef SmartPointer<Entity> EntityPtr;

class VisionRangeTrigger;

/**
 *  This class is the controller for an entity's vision.
 */
class VisionController : public Controller, public Updatable
{
	DECLARE_CONTROLLER_TYPE( VisionController )

public:
	 // period in ticks. defaults to 10 ticks (1 second)
	VisionController( float visionAngle = 1.f, float visionRange = 20.f,
		float seeingHeight = 2.f, int updatePeriod = 10 );

	virtual void	startReal( bool isInitialStart );
	virtual void	stopReal( bool isFinalStop );

	virtual float getYawOffset();

	void	writeRealToStream( BinaryOStream& stream );
	bool 	readRealFromStream( BinaryIStream& stream );

	void	update();

	float	fieldOfView() const		{ return visionAngle_; }	// Deprecated
	float	visionAngle() const		{ return visionAngle_; }	
	float	visionRange() const		{ return visionRange_; }

	void setVisionRange( float visionAngle, float range );

	float seeingHeight() const		{ return seeingHeight_; };
	void seeingHeight( float h );

private:
	float	visionAngle_;
	float	visionRange_;

	float	seeingHeight_;
	int		updatePeriod_;
	int		tickSinceLast_;

	VisionRangeTrigger* pVisionTrigger_;

	std::vector< EntityID > * pOnloadedVisible_;
};

#endif // VISION_CONTROLLER_HPP

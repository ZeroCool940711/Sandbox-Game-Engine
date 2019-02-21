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

#include "current_general_properties.hpp"


std::vector<GenPositionProperty*> PropertyCollator<GenPositionProperty>::properties_;
CurrentPositionProperties::ViewEnroller CurrentPositionProperties::s_viewEnroller;

std::vector<GenScaleProperty*> PropertyCollator<GenScaleProperty>::properties_;
CurrentScaleProperties::ViewEnroller CurrentScaleProperties::s_viewEnroller;

std::vector<GenRotationProperty*> PropertyCollator<GenRotationProperty>::properties_;
CurrentRotationProperties::ViewEnroller CurrentRotationProperties::s_viewEnroller;


Vector3 CurrentPositionProperties::averageOrigin()
{
	Vector3 avg = Vector3::zero();

	if (properties_.empty())
		return Vector3::zero();

	std::vector<GenPositionProperty*>::iterator i = properties_.begin();
	for (; i != properties_.end(); ++i)
	{
		Matrix m;
		(*i)->pMatrix()->getMatrix( m );
		avg += m.applyToOrigin();
	}

	return avg / (float) properties_.size();
}

Vector3 CurrentPositionProperties::centrePosition()
{
	BoundingBox bb( Vector3::zero(), Vector3::zero() );

	std::vector<GenPositionProperty*>::iterator i = properties_.begin();
	if (i != properties_.end())
	{
		Matrix m;
		(*i)->pMatrix()->getMatrix( m );
		Vector3 v = m.applyToOrigin();

		bb.setBounds( v, v );
		++i;
	}

	for (; i != properties_.end(); ++i)
	{
		Matrix m;
		(*i)->pMatrix()->getMatrix( m );
		Vector3 v = m.applyToOrigin();

		bb.addBounds( v );
	}

	return (bb.minBounds() + bb.maxBounds()) / 2.f;
}


Vector3 CurrentRotationProperties::averageOrigin()
{
	Vector3 avg = Vector3::zero();

	if (properties_.empty())
		return Vector3::zero();

	std::vector<GenRotationProperty*>::iterator i = properties_.begin();
	for (; i != properties_.end(); ++i)
	{
		Matrix m;
		(*i)->pMatrix()->getMatrix( m );
		avg += m.applyToOrigin();
	}

	return avg / (float) properties_.size();
}

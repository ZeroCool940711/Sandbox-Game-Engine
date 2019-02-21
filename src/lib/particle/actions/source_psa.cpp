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

#pragma warning (disable:4355)	// 'this' : used in base member initialiser list
#pragma warning (disable:4503)	// 'identifier' : decorated name length exceeded, name was truncated
#pragma warning (disable:4786)	// 'identifier' : identifier was truncated to 'number' characters in the debug information

#include "source_psa.hpp"
#include "particle/particle_system.hpp"
#include "chunk/chunk_obstacle.hpp"
#include "vector_generator.hpp"

DECLARE_DEBUG_COMPONENT2( "Particle", 0 )

#ifndef CODE_INLINE
#include "source_psa.ipp"
#endif

ParticleSystemActionPtr SourcePSA::clone() const
{
    return ParticleSystemAction::clonePSA(*this);
}


/**
 *	This is the destructor for SourcePSA.
 */
SourcePSA::~SourcePSA()
{
	BW_GUARD;
	if ( pPositionSrc_ != NULL )
	{
		delete pPositionSrc_;
		pPositionSrc_ = NULL;
	}
	if ( pVelocitySrc_ != NULL )
	{
		delete pVelocitySrc_;
		pVelocitySrc_ = NULL;
	}

	pGS_ = NULL;
}


/**
 *	This is the serialiser for SourcePSA properties.
 */
void SourcePSA::serialiseInternal(DataSectionPtr pSect, bool load)
{
	BW_GUARD;
	const std::string PositionSourceString("pPositionSrc");
	const std::string VelocitySourceString("pVelocitySrc");

	// manage the vector generators
	if (load)
	{
		DataSectionPtr pVGSect = pSect->openSection( PositionSourceString );
		if (pVGSect)
		{
			// create a new one, use iterators for convenience
			DataSectionIterator it;
			for (it = pVGSect->begin(); it != pVGSect->end(); it++)
			{
				DataSectionPtr pDS = *it;
				std::string vgType = pDS->sectionName();
				pPositionSrc_ = VectorGenerator::createGeneratorOfType(vgType);
				MF_ASSERT_DEV(pPositionSrc_);
				if( pPositionSrc_ )
					pPositionSrc_->serialise( pDS, load );
			}
		}

		pVGSect = pSect->openSection( VelocitySourceString );
		if (pVGSect)
		{
			// create a new one
			DataSectionIterator it;
			for (it = pVGSect->begin(); it != pVGSect->end(); it++)
			{
				DataSectionPtr pDS = *it;
				std::string vgType = pDS->sectionName();
				pVelocitySrc_ = VectorGenerator::createGeneratorOfType(vgType);
				MF_ASSERT_DEV(pVelocitySrc_);
				if( pVelocitySrc_ )
					pVelocitySrc_->serialise( pDS, load );
			}
		}
	}
	else
	{
		if (pPositionSrc_)
		{
			DataSectionPtr pVGSect = pSect->newSection( PositionSourceString );
			pPositionSrc_->serialise(pVGSect, load);
		}

		if (pVelocitySrc_)
		{
			DataSectionPtr pVGSect = pSect->newSection( VelocitySourceString );
			pVelocitySrc_->serialise(pVGSect, load);
		}
	}

    if (!load && sleepPeriod_ > sleepPeriodMax_)
        std::swap(sleepPeriod_, sleepPeriodMax_);

	SERIALISE(pSect, motionTriggered_, Bool, load);
	SERIALISE(pSect, timeTriggered_, Bool, load);
	SERIALISE(pSect, grounded_, Bool, load);
	SERIALISE(pSect, dropDistance_, Float, load);
	SERIALISE(pSect, rate_, Float, load);
	SERIALISE(pSect, sensitivity_, Float, load);
	SERIALISE(pSect, maxSpeed_, Float, load);
	SERIALISE(pSect, activePeriod_, Float, load);
	SERIALISE(pSect, sleepPeriod_, Float, load);
	SERIALISE(pSect, sleepPeriodMax_, Float, load);
	if (load)
		currentSleepPeriod_ = generateSleepPeriod();

    if (load && sleepPeriod_ > sleepPeriodMax_ && sleepPeriod_ >= 0.0f && sleepPeriodMax_ >= 0.0f)
        std::swap(sleepPeriod_, sleepPeriodMax_);

	SERIALISE(pSect, minimumSize_, Float, load);
	SERIALISE(pSect, maximumSize_, Float, load);
	SERIALISE(pSect, forcedUnitSize_, Int, load);
	SERIALISE(pSect, allowedTimeInSeconds_, Float, load);
	if (load)
		allowedTime(allowedTimeInSeconds_);

	SERIALISE(pSect, initialRotation_, Vector2, load);
	SERIALISE(pSect, randomInitialRotation_, Vector2, load);
	SERIALISE(pSect, initialColour_, Vector4, load);
	SERIALISE(pSect, randomSpin_, Bool, load );
	SERIALISE(pSect, minSpin_, Float, load );
	SERIALISE(pSect, maxSpin_, Float, load );
	SERIALISE(pSect, ignoreRotation_, Bool, load);
	SERIALISE(pSect, inheritVelocity_, Float, load);
}


// -----------------------------------------------------------------------------
// Section: Accessors to SourcePSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the Set-Accessor to the motionTrigger flag.
 *
 *	@param flag		The new value for the motionTrigger flag.
 */
void SourcePSA::motionTriggered( bool flag )
{
	motionTriggered_ = flag;
	if ( motionTriggered_ || timeTriggered_ )
	{
		firstUpdate_ = true;
	}
}


/**
 *	This is the Set-Accessor to the timeTriggered flag.
 *
 *	@param flag		The new value for the timeTriggered flag.
 */
void SourcePSA::timeTriggered( bool flag )
{
	timeTriggered_ = flag;
	if ( motionTriggered_ || timeTriggered_ )
	{
		firstUpdate_ = true;
	}
	if ( !timeTriggered_ )
	{
		periodTime_ = 0.0f;
	}
}


/**
 *	This is the Set-Accessor for the sensitivity property.
 *
 *	Note: Sensitivity cannot be set to zero - it means infinite particles
 *	would be generated on any finite displacement. If sensitivity is set
 *	below a threshold of 0.0001, it will be forced to that minimum threshold.
 *
 *	@param value	The number of metres moved before a particle is generated.
 */
void SourcePSA::sensitivity( float value )
{
	sensitivity_ = value;
	if ( almostZero( sensitivity_, 0.0001f ))
	{
		sensitivity_ = 0.0001f;
	}
}


/**
 *	This is the Set-Accessor for the minimumSize property.
 *
 *	@param newScale		The new value for the minimum size.
 */
void SourcePSA::minimumSize( float newScale )
{
	minimumSize_ = newScale;
	if ( minimumSize_ > maximumSize_ )
	{
		maximumSize_ = minimumSize_;
	}
}


/**
 *	This is the Set-Accessor for the maximumSize property.
 *
 *	@param newScale		The new value for the maximum size.
 */
void SourcePSA::maximumSize( float newScale )
{
	maximumSize_ = newScale;
	if ( maximumSize_ < minimumSize_ )
	{
		minimumSize_ = maximumSize_;
	}
}


// -----------------------------------------------------------------------------
// Section: Methods for SourcePSA.
// -----------------------------------------------------------------------------


/**
 *	This sets a vector generator as the new generator for positions of the
 *	particles.
 *
 *	@param pPositionSrc		The new source for positions.
 */
void SourcePSA::setPositionSource( VectorGenerator *pPositionSrc )
{
	BW_GUARD;
	if ( pPositionSrc != pPositionSrc_ )
	{
		if ( pPositionSrc_ != NULL )
		{
			delete pPositionSrc_;
			pPositionSrc_ = NULL;
		}
		pPositionSrc_ = pPositionSrc;
	}
}


/**
 *	This sets a vector generator as the new generator for velocities of the
 *	particles.
 *
 *	@param pVelocitySrc		The new source for velocities.
 */
void SourcePSA::setVelocitySource( VectorGenerator *pVelocitySrc )
{
	BW_GUARD;
	if ( pVelocitySrc != pVelocitySrc_ )
	{
		if ( pVelocitySrc_ != NULL )
		{
			delete pVelocitySrc_;
			pVelocitySrc_ = NULL;
		}
		pVelocitySrc_ = pVelocitySrc;
	}
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------

PROFILER_DECLARE( SourcePSA_execute, "PSA Source Execute" );

/**
 *	This method executes the action for the given frame of time. The dTime
 *	parameter is the time elapsed since the last call.
 *
 *	@param particleSystem	The particle system on which to operate.
 *	@param dTime			Elapsed time in seconds.
 */
void SourcePSA::execute( ParticleSystem &particleSystem, float dTime )
{
	BW_GUARD_PROFILER( SourcePSA_execute );

	// Do nothing if the dTime passed was zero or if the particle system
	// is not quite old enough to be active.
	if ( ( age_ < delay() ) || ( dTime <= 0.0f ) )
	{
		age_ += dTime;
		return;
	}

	// Before any createParticles() calls are made, store the current
	// timestamp value for this frame.
	currentTime_ = timestamp();	

	// Calculate particle system velocity and displacement
	Matrix objectToWorld = particleSystem.objectToWorld();

	Vector3 positionOfPS = objectToWorld.applyToOrigin();
	Vector3 displacement = positionOfPS - ( firstUpdate_ ? positionOfPS :
		lastPositionOfPS_ );
	lastPositionOfPS_ = positionOfPS;
	firstUpdate_ = false;
	velocityOfPS_ = displacement / dTime;

	if (ignoreRotation_)
	{
		objectToWorld.setTranslate( objectToWorld.applyToOrigin() );
	}


	// Check for forced particles.
	if ( forcedTotal_ > 0 )
	{
		int frameTotal = forcedTotal_ * forcedUnitSize();

		if ( frameTotal + particleSystem.size() > particleSystem.capacity() )
		{
			frameTotal = particleSystem.capacity() - particleSystem.size();
		}

		// Make the forced particles.
		while ( frameTotal > 0 )
		{
			// don't make too many
			int forceSize = min(forcedUnitSize(), frameTotal);

			createParticles( particleSystem, objectToWorld, forceSize,
				Vector3( 0.0f, 0.0f, 0.0f ) );
			frameTotal -= forceSize;
		}

		forcedTotal_ = 0;
	}


	// Check for particle creation that is dependent on the particle system's
	// previous position.
	if ( motionTriggered() || timeTriggered() || ( queuedTotal_ > 0 ) )
	{
		float distance = displacement.length();
		int frameTotal = 0;

		// Check for motion triggered particles.
		if ( motionTriggered() )
		{
			float speed = distance / dTime;
			if ( speed > sensitivity() )
			{
				// Sensitivity cannot be zero or less. The Set-Accessor
				// ensures this is the case.
				frameTotal = int( ( 1 + 2 * ( ( speed - sensitivity() ) /
					sensitivity() ) ) * dTime );

				if ( frameTotal + particleSystem.size() >
					particleSystem.capacity() )
				{
					frameTotal = particleSystem.capacity() -
						particleSystem.size();
				}
			}

			// Make the motion triggered particles.
			if ( frameTotal > 0 )
			{
				Vector3 displacementPerParticle = displacement / float(frameTotal);
				createParticles( particleSystem, objectToWorld, frameTotal,
					displacementPerParticle );
			}
		}

		// Check for time triggered/queued particles.
		if ( timeTriggered() || ( queuedTotal_ > 0 ) )
		{
			if ( rate() > 0.0f )
			{
				if ( currentSleepPeriod_ > 0.0f )
				{
					float period = activePeriod() + currentSleepPeriod_;

					// Find out where we are in our active/sleep cycle.
					float lastPeriodTime = periodTime_;
					periodTime_ += dTime;
					if ( ( periodTime_ > period ) && ( period > 0.0f ) )
					{
						periodTime_ = fmodf( periodTime_, period );
						lastPeriodTime = 0.0f;

						// generate a new sleep period
						currentSleepPeriod_ = generateSleepPeriod();
					}

					// If we are active, accumulate only the active time of
					// the nearest period, ie. this period, for the purposes
					// of determining how many particles to produce.
					if ( periodTime_ <= activePeriod() )
					{
						float timeBetweenParticles = 1.0f / rate();
						accumulatedTime_ += periodTime_ - lastPeriodTime;

						if ( accumulatedTime_ > timeBetweenParticles )
						{
							frameTotal = int( accumulatedTime_ /
								timeBetweenParticles );
							accumulatedTime_ -= frameTotal *
								timeBetweenParticles;
							queuedTotal_ -= frameTotal;
						}
					}
				}
				else
				{
					// Here we are guaranteed to be active all the time.
					float timeBetweenParticles = 1.0f / rate();
					accumulatedTime_ += dTime;

					if ( accumulatedTime_ > timeBetweenParticles )
					{
						frameTotal = int( accumulatedTime_ /
							timeBetweenParticles );
						accumulatedTime_ -= frameTotal * timeBetweenParticles;
						queuedTotal_ -= frameTotal;
					}
				}
			}

			// Capacity is a hard limit on the number of particles produced.
			// Particles exceeding the limit are simply vanished - not queued
			// further.
			if ( frameTotal + particleSystem.size() >
				particleSystem.capacity() )
			{
				frameTotal = particleSystem.capacity() - particleSystem.size();
			}

			// TODO : This feature added for GDC Austin, but only with xml support
			// Implement fully in BigWorld 2.0 but no harm in leaving it here.
			// Hard limit time trigged particles if maximum velocity is set.
			if ( frameTotal > 0 && maxSpeed_ > 0.f )
			{
				float speed = distance / dTime;
				if (speed > maxSpeed_)
					frameTotal = 0;
			}

			// Make the time triggered/queued particles.
			if ( frameTotal > 0 )
			{
				Vector3 displacementPerParticle = displacement / float(frameTotal);
				createParticles( particleSystem, objectToWorld, frameTotal,
					displacementPerParticle );
			}
		}
	}
}


/**
 *	This method determines the memory footprint of the action
 */
size_t SourcePSA::sizeInBytes() const
{
	return sizeof(SourcePSA) + pPositionSrc_->sizeInBytes() + pVelocitySrc_->sizeInBytes(); 
}


// -----------------------------------------------------------------------------
// Section: Helper Methods for the SourcePSA.
// -----------------------------------------------------------------------------


/**
 *	This method iterates through the required particles to be created, adding
 *	them to the ParticleSystem supplied.
 *
 *	@param particleSystem		The Particle System to be used.
 *	@param objectToWorld		The pre-calculated objectToWorld matrix.
 *	@param numberOfParticles	The number of particles to make.
 *	@param dispPerParticle		The spacing between particles.
 */
void SourcePSA::createParticles( ParticleSystem &particleSystem,
		const Matrix &objectToWorld,
		int numberOfParticles, 
		const Vector3 &dispPerParticle )
{
	BW_GUARD;
	// Make the collider if necessary
	if (!pGS_)
		pGS_ = new ChunkRompCollider();

	float sizeDifference = maximumSize() - minimumSize();

	// If allowed time is zero, then it is ignored.
	for (int count = 0; (count < numberOfParticles) && 
        ((allowedTime_ == 0.0f) || 
        (timestamp() - currentTime_ < allowedTime_)); ++count)
	{
		// Generate our particle's position.
		Vector3 posInLS( 0.0f, 0.0f, 0.0f );
		if ( pPositionSrc_ != NULL )
		{
			pPositionSrc_->generate( posInLS );
		}

		// Get our position in world-space and add any accumulated
		// displacements.
		Vector3 position;
		objectToWorld.applyPoint( position, posInLS );
		position -= float( numberOfParticles - 1 - count ) * dispPerParticle;

		// Generate our particle's velocity.
		Vector3 velInLS( 0.0f, 0.0f, 0.0f );
		if ( pVelocitySrc_ != NULL )
		{
			pVelocitySrc_->generate( velInLS );
		}

		// Get our velocity in world-space.
		Vector3 velocity;
		objectToWorld.applyVector( velocity, velInLS );

		// Add the particle system's current velocity.
		velocity += ( inheritVelocity_ * velocityOfPS_ );

		// If the particles are to be set upon the ground, check with
		// the chunk space for where it collides with the ground directly
		// below it.
		if (grounded())
		{ 
			if (pGS_)
			{
				// Make it a tiny bit higher in case it is currently snapped to the ground
				// This needs to be done since the ChunkRompCollider will not find a collision
				// if it starts at or below the ground. Notice that the drop distance is
				// offset by this amount as well.

				position.y += 0.05f; 

				position.y = pGS_->ground( position, dropDistance() + 0.05f, true );
			}
		}

        if (position.y != RompCollider::NO_GROUND_COLLISION)
        {
		    // Generate our particle's size.
		    float size;
		    size = unitRand() * sizeDifference + minimumSize();

		    // Get the world pitch and yaw from the forward vector of the psystem
			// For sprite particles, pitch means spin speed, and yaw means initial orientation
		    float pitch = initialRotation_.x + randomInitialRotation_.x * (unitRand() - 0.5f);
		    float yaw = initialRotation_.y + randomInitialRotation_.y * (unitRand() - 0.5f);

		    Vector4 colour( initialColour_ );
            Vector3 spinAxis;
            float spinSpeed = 0.0f;
		    if ( randomSpin_ )
		    {
			    // get a random spin axis
			    spinAxis.set( unitRand(), unitRand() , unitRand() );
			    spinAxis.normalise();
 			    // give a random spin amount too.
			    spinSpeed = unitRand() * (maxSpin_-minSpin_) + minSpin_;

		    }

            if (particleSystem.pRenderer() && particleSystem.pRenderer()->isMeshStyle())
            {
				pitch += objectToWorld.applyToUnitAxisVector(2).pitch();
				yaw += objectToWorld.applyToUnitAxisVector(2).yaw();

		        particleSystem.addParticle
                ( 
                    Particle 
                    (
			            position,
			            velocity,
			            pitch,
			            yaw,
                        spinAxis,
                        spinSpeed
                    ),
                    true
                );
            }
            else
            {
		        particleSystem.addParticle
                ( 
                    Particle 
                    (
			            position,
			            velocity,
			            colour,
			            size,
			            pitch,
			            yaw 
                    ),
                    false
                );
            }
	    }
    }
}


// -----------------------------------------------------------------------------
// Section: Python Interface to the PySourcePSA.
// -----------------------------------------------------------------------------


PY_TYPEOBJECT( PySourcePSA )

/*~ function Pixie.SourcePSA
 *	Factory function to create and return a new PySourcePSA object. SourcePSA is a
 *	ParticleSystemAction that creates particles within a ParticleSystem. 
 *	@param position_source VectorGenerator as a position source for the particles.
 *	@param velocity_source VectorGenerator as a velocity source for the particles.
 *	@return A new PySourcePSA object.
 */
PY_FACTORY_NAMED( PySourcePSA, "SourcePSA", Pixie )

PY_BEGIN_METHODS( PySourcePSA )
	/*~ function PySourcePSA.create
	 *	Queue number particles into the particle creation queue. Particles will 
	 *	be created using the normal creation algorithms for the SourcePSA.
	 *	@param number Integer. Number of particles to add to creation queue.
	 */
	PY_METHOD( create )
	/*~ function PySourcePSA.force
	 *	Force creation of number units (see forcedUnitSize attribute) of 
	 *	particles on the next update.
	 *	@param number Integer. Nomber of units to create.
	 */
	PY_METHOD( force )
	/*~ function PySourcePSA.setPositionSource
	 *	Sets a new VectorGenerator as a position source for the particles.
	 *	@param generator VectorGenerator. Position source for the particles.
	 */
	PY_METHOD( setPositionSource )
	/*~ function PySourcePSA.setVelocitySource
	 *	Sets a new VectorGenerator as a velocity source for the particles.
	 *	@param generator VectorGenerator. Velocity source for the particles.
	 */
	PY_METHOD( setVelocitySource )
	/*~ function PySourcePSA.getPositionSourceMaxRadius
	 *	Gets the maximum radius for the position source VectorGenerator.
	 *	@return Float. Maximum radius.
	 */
	PY_METHOD( getPositionSourceMaxRadius )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( PySourcePSA )
	/*~ attribute PySourcePSA.motionTriggered
	 *	Flag for motion triggered particle creation. Particles are created when
	 *	the speed of movement for the particle system exceeds the sensitivity
	 *	value. The number of particles created is dependant upon the difference
	 *	between speed and sensitivity divided by the sensitivity.
	 *	@type Integer as boolean. 0 or 1.
	 */
	PY_ATTRIBUTE( motionTriggered )
	/*~ attribute PySourcePSA.timeTriggered
	 *	Flag for time triggered particle creation.
	 *	@type Integer as boolean. 0 or 1.
	 */
	PY_ATTRIBUTE( timeTriggered )
	/*~ attribute PySourcePSA.grounded
	 *	This attribute if set specifies that the source position's y coordinate
	 *	if within dropDistance of the ground is set to the ground's y value. Ie
	 *	it starts on the ground. Default is 0 (false).
	 *	@type Integer as boolean. 0 or 1.
	 */
	PY_ATTRIBUTE( grounded )
	/*~ attribute PySourcePSA.dropDistance
	 *	Distance downwards to check for a grounded particle (see grounded
	 *	attribute). Default is 15.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( dropDistance )
	/*~ attribute PySourcePSA.rate
	 *	Rate of particle creation for time triggered particle creation.
	 *	@type Float. Particles per second.
	 */
	PY_ATTRIBUTE( rate )
	/*~ attribute PySourcePSA.sensitivity
	 *	Sensitivity to movement for motion triggered particle creation. Forced
	 *	minimum value of 0.0001f. If the speed of movement of the particle
	 *	system exceeds the sensitivity, then particles are generated (see
	 *	motionTriggered for more information).
	 *	@type Float. Must be greater than or equal to 0.0001f.
	 */
	PY_ATTRIBUTE( sensitivity )
	/*~ attribute PySourcePSA.activePeriod
	 *	activePeriod along with sleepPeriod determine an on/off cycle for time 
	 *	created particles.
	 *	@type Float in seconds.
	 */
	PY_ATTRIBUTE( activePeriod )
	/*~ attribute PySourcePSA.sleepPeriod
	 *	activePeriod along with sleepPeriod determine an on/off cycle for time 
	 *	created particles.
	 *	@type Float in seconds.
	 */
	PY_ATTRIBUTE( sleepPeriod )
	/*~ attribute PySourcePSA.sleepPeriodMax
	 *	sleepPeriodMax facilitates a random behaviour to sleepPeriod
	 *	@type Float in seconds.
	 */
	PY_ATTRIBUTE( sleepPeriodMax )
	/*~ attribute PySourcePSA.minimumSize
	 *	Smallest scale for generated particle size. Default 1.0.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( minimumSize )
	/*~ attribute PySourcePSA.maximumSize
	 *	Largest scale for generated particle size. Default 1.0.
	 *	@type Float.
	 */
	PY_ATTRIBUTE( maximumSize )
	/*~ attribute PySourcePSA.forcedUnitSize
	 *	The unit size for determining number of particles created when forcing
	 *	particle creation (see force method).
	 *	@type Integer. 
	 */
	PY_ATTRIBUTE( forcedUnitSize )
	/*~ attribute PySourcePSA.initialRotation
	 *	Used by Mesh particle systems. Initial Pitch/Yaw. Default is (0.0, 0.0).
	 *	@type Sequence of 2 floats.
	 */
	PY_ATTRIBUTE( initialRotation )
	/*~ attribute PySourcePSA.randomInitialRotation
	 *	Used by Mesh particle systems. Randomise the initial Pitch/Yaw. Default is (0.0, 0.0).
	 *	@type Sequence of 2 floats.
	 */
	PY_ATTRIBUTE( randomInitialRotation )
	/*~ attribute PySourcePSA.initialColour
	 *	RGBA quad of floats for particle colour. Interpreted by mesh particle 
	 *	systems as spin. Default is (0.5, 0.5, 0.5, 1.0).
	 *	@type Sequence of 4 floats.
	 */
	PY_ATTRIBUTE( initialColour )
	/*~ attribute PySourcePSA.ignoreRotation
	 *	If set particle systems ignore the rotation of the particle system's
	 *	world tranform's rotation. Default is 0 (false).
	 *	@type Integer as boolean. 0 or 1.
	 */
	PY_ATTRIBUTE( ignoreRotation )
	/*~	attribute PySourcePSA.inheritVelocity
	 *	
	 *	Specifies how much velocity particles should inherit from SourcePSA when spawned, eg, 
	 *	a value of 0.0 will add no extra velocity to spawning particles and a value of 2.0
	 *	will add double the SourcePSA's velocity to the spawning particles.
	 *
	 *	@type float
	 */
	PY_ATTRIBUTE( inheritVelocity )
	/*~ attribute PySourcePSA.randomSpin
	 *	If set particle systems will be imbued with a random amount of angular velocity,
	 *	and a random rotation axis.  This property is exclusively for mesh particles.
	 */
	 PY_ATTRIBUTE( randomSpin )
	 /*~ attribute PySourcePSA.minSpin
	 *	When randomSpin is set, the particle is given a random amount of spin, between
	 *	minSpin and maxSpin.  [The units are 20 radians per second]
	 */
	 PY_ATTRIBUTE( minSpin )
	 /*~ attribute PySourcePSA.maxSpin
	 *	When randomSpin is set, the particle is given a random amount of spin, between
	 *	minSpin and maxSpin.  [The units are 20 radians per second]
	 */
	 PY_ATTRIBUTE( maxSpin )
PY_END_ATTRIBUTES()


/**
 *	This is an automatically declared method for any derived classes of
 *	PyObjectPlus. It connects the readable attributes to their corresponding
 *	C++ components and searches the parent class' attributes if not found.
 *
 *	@param attr		The attribute being searched for.
 */
PyObject *PySourcePSA::pyGetAttribute( const char *attr )
{
	BW_GUARD;
	PY_GETATTR_STD();

	return PyParticleSystemAction::pyGetAttribute( attr );
}


/**
 *	This is an automatically declared method for any derived classes of
 *	PyObjectPlus. It connects the writable attributes to their corresponding
 *	C++ components and searches the parent class' attributes if not found.
 *
 *	@param attr		The attribute being searched for.
 *	@param value	The value to assign to the attribute.
 */
int PySourcePSA::pySetAttribute( const char *attr, PyObject *value )
{
	BW_GUARD;
	PY_SETATTR_STD();

	return PyParticleSystemAction::pySetAttribute( attr, value );
}

/**
 *	This is a static Python factory method. This is declared through the
 *	factory declaration in the class definition.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be two lists (vector generator descriptions for position
 *					and velocity.) Both arguments are optional.
 */
PyObject *PySourcePSA::pyNew( PyObject *args )
{
	BW_GUARD;
	PyObject *positionDesc = NULL;
	PyObject *velocityDesc = NULL;

	if ( PyArg_ParseTuple( args, "OO", &positionDesc, &velocityDesc ) )
	{
		SourcePSAPtr pAction = new SourcePSA(
			VectorGenerator::parseFromPython( positionDesc ),
			VectorGenerator::parseFromPython( velocityDesc ) );
		return new PySourcePSA(pAction);
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "SourcePSA:"
			"Expected two lists describing vector generators." );
		return NULL;
	}
}

/**
 *	This Python method allows the script to tell the SourcePSA to create
 *	N particles as part of its cycle.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be an integer.
 */
PyObject *PySourcePSA::py_create( PyObject *args )
{
	BW_GUARD;
	int number;

	if ( PyArg_ParseTuple( args, "i", &number ) )
	{
		pAction_->create( number );
		Py_Return;
	}

	PyErr_SetString( PyExc_TypeError, "SourcePSA.create: "
		"integer expected." );
	return NULL;
}

/**
 *	This Python method allows the script to force the SourcePSA to create
 *	N particles in the next update.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be an integer.
 */
PyObject *PySourcePSA::py_force( PyObject *args )
{
	BW_GUARD;
	int number;

	if ( PyArg_ParseTuple( args, "i", &number ) )
	{
		pAction_->force( number );
		Py_Return;
	}

	PyErr_SetString( PyExc_TypeError, "SourcePSA.force: "
		"integer expected." );
	return NULL;
}

/**
 *	This Python method allows the script to specify the parameters for a
 *	new vector generator to the particle positions.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be a list describing a vector generator.
 */
PyObject *PySourcePSA::py_setPositionSource( PyObject *args )
{
	BW_GUARD;
	PyObject *positionDesc = NULL;

	if ( PyArg_ParseTuple( args, "O", &positionDesc ) )
	{
		pAction_->setPositionSource( VectorGenerator::parseFromPython( positionDesc ) );
		Py_Return;
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "SourcePSA.setPositionSource: "
			"Expected a list describing vector generators." );
		return NULL;
	}
}

/**
 *	This Python method allows the script to specify the parameters for a
 *	new vector generator to the particle velocities.
 *
 *	@param args		The list of parameters passed from Python. This should
 *					be a list describing a vector generator.
 */
PyObject *PySourcePSA::py_setVelocitySource( PyObject *args )
{
	BW_GUARD;
	PyObject *velocityDesc = NULL;

	if ( PyArg_ParseTuple( args, "O", &velocityDesc ) )
	{
		pAction_->setVelocitySource( VectorGenerator::parseFromPython( velocityDesc ) );
		Py_Return;
	}
	else
	{
		PyErr_SetString( PyExc_TypeError, "SourcePSA.setVelocitySource: "
			"Expected a list describing vector generators." );
		return NULL;
	}
}

/**
 *	This Python method allows the script to determine the max size of the position
 *  vector generator.
 */
PyObject *PySourcePSA::py_getPositionSourceMaxRadius( PyObject *args )
{
	BW_GUARD;
	return Script::getData( pAction_->getPositionSource()->maxRadius() );
}


PY_SCRIPT_CONVERTERS( PySourcePSA )

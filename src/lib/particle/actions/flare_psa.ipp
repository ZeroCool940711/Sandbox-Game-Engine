/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifdef CODE_INLINE
#define INLINE inline
#else
#define INLINE
#endif


// -----------------------------------------------------------------------------
// Section: Constructor(s) for FlarePSA.
// -----------------------------------------------------------------------------


INLINE FlarePSA::FlarePSA( const std::string& flareName, int step, int col, int size ) : 
	flareStep_( step ),
	colourize_( !!col ),
	useParticleSize_( !!size )
{
	flareName_ = flareName;
	this->loadLe();
}


// -----------------------------------------------------------------------------
// Section: Methods for the FlarePSA.
// -----------------------------------------------------------------------------


/**
 *	This is the Get-Accessor for the flareName attribute. 
 *
 *	@return The resource used for flares. 
 */
INLINE const std::string & FlarePSA::flareName() 
{ 
	return flareName_; 
}


/**
 *	This is the Set-Accessor for the flareName attribute. 
 *
 *	@return The resource to use for flares. 
 */
INLINE void FlarePSA::flareName( const std::string & name )
{
	flareName_ = name;
	this->loadLe();
}


/**
 *	This is the Get-Accessor for the flareStep attribute. 0 means that the
 *	oldest particle will have the flare. 1 means all particles have flares.
 *	2..x means every x particles has a flare.
 *
 *	@return The flare step. 
 */
INLINE int FlarePSA::flareStep() 
{ 
	return flareStep_; 
}


/**
 *	This is the Set-Accessor for the flareStep attribute. 0 means that the
 *	oldest particle will have the flare. 1 means all particles have flares.
 *	2..x means every x particles has a flare.
 *
 *	@param step	Sets the flare step. 
 */
INLINE void FlarePSA::flareStep( int step ) 
{ 
	flareStep_ = step; 
}


/**
 *	This is the Get-Accessor for the colourize attribute. 
 *
 *	@return Whether the flares are coloured. 
 */
INLINE bool FlarePSA::colourize() 
{ 
	return colourize_; 
}


/**
 *	This is the Set-Accessor for the colourize attribute.
 *
 *	@param	colour Sets whether the flares should be coloured.
 */
INLINE void FlarePSA::colourize( bool colour ) 
{ 
	colourize_ = colour; 
}


/**
 *	This is the Get-Accessor for the useParticleSize attribute. 
 *
 *	@return Whether the flares are should be the same size as the particles. 
 */
INLINE bool FlarePSA::useParticleSize() 
{ 
	return useParticleSize_; 
}


/**
 *	This is the Set-Accessor for the useParticleSize attribute.
 *
 *	@param	size	Sets the flares should be the same size as the particles.
 */
INLINE void FlarePSA::useParticleSize( bool size ) 
{ 
	useParticleSize_ = size; 
}
	
	
// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the ParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int FlarePSA::typeID( void ) const
{
	return typeID_;
}

INLINE std::string FlarePSA::nameID( void ) const
{
	return nameID_;
}


// -----------------------------------------------------------------------------
// Section: Constructor(s) for PyFlarePSA.
// -----------------------------------------------------------------------------


/**
 *	This is the constructor for PyFlarePSA.
 *
 *	@param elasticity			The elasticity to use.
 */
INLINE PyFlarePSA::PyFlarePSA( FlarePSAPtr pAction, PyTypePlus *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction )
{
}

// -----------------------------------------------------------------------------
// Section: Methods for the PyFlarePSA.
// -----------------------------------------------------------------------------


/**
 *	This is the Get-Accessor for the flareName attribute. 
 *
 *	@return The resource used for flares. 
 */
INLINE const std::string & PyFlarePSA::flareName() 
{ 
	return pAction_->flareName(); 
}


/**
 *	This is the Set-Accessor for the flareName attribute. 
 *
 *	@return The resource to use for flares. 
 */
INLINE void PyFlarePSA::flareName( const std::string & name )
{
	pAction_->flareName( name );
}


/**
 *	This is the Get-Accessor for the flareStep attribute. 0 means that the
 *	oldest particle will have the flare. 1 means all particles have flares.
 *	2..x means every x particles has a flare.
 *
 *	@return The flare step. 
 */
INLINE int PyFlarePSA::flareStep() 
{ 
	return pAction_->flareStep(); 
}


/**
 *	This is the Set-Accessor for the flareStep attribute. 0 means that the
 *	oldest particle will have the flare. 1 means all particles have flares.
 *	2..x means every x particles has a flare.
 *
 *	@param Sets the flare step. 
 */
INLINE void PyFlarePSA::flareStep( int step ) 
{ 
	pAction_->flareStep( step ); 
}


/**
 *	This is the Get-Accessor for the colourize attribute. 
 *
 *	@return Whether the flares are coloured. 
 */
INLINE bool PyFlarePSA::colourize() 
{ 
	return pAction_->colourize(); 
}


/**
 *	This is the Set-Accessor for the colourize attribute.
 *
 *	@param Sets whether the flares should be coloured.
 */
INLINE void PyFlarePSA::colourize( bool colour ) 
{ 
	pAction_->colourize( colour ); 
}


/**
 *	This is the Get-Accessor for the useParticleSize attribute. 
 *
 *	@return Whether the flares are should be the same size as the particles. 
 */
INLINE bool PyFlarePSA::useParticleSize() 
{ 
	return pAction_->useParticleSize(); 
}


/**
 *	This is the Set-Accessor for the useParticleSize attribute.
 *
 *	@param Sets the flares should be the same size as the particles.
 */
INLINE void PyFlarePSA::useParticleSize( bool size ) 
{ 
	pAction_->useParticleSize( size ); 
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Py Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the PyParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int PyFlarePSA::typeID( void ) const
{
	return pAction_->typeID();
}

/* flare_psa.ipp */

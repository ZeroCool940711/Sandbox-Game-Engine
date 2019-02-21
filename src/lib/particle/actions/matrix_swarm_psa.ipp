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
// Section: Constructor(s) and Destructor for MatrixSwarmPSA.
// -----------------------------------------------------------------------------

/**
 *	This is the constructor for MatrixSwarmPSA.
 */
INLINE MatrixSwarmPSA::MatrixSwarmPSA()
{
}


/**
 *	This is the destructor for MatrixSwarmPSA.
 */
INLINE MatrixSwarmPSA::~MatrixSwarmPSA()
{
	for (Matrices::iterator iter = targets_.begin();
		iter != targets_.end();
		iter++)
	{
		Py_DECREF( *iter );
	}
}


// -----------------------------------------------------------------------------
// Section: Accessors to MatrixSwarmPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This method returns the matrix targets of MatrixSwarmPSA.
 *
 *	@return A vector of MatrixProvider* objects.
 */
INLINE Matrices& MatrixSwarmPSA::targets( void ) 
{
	return targets_;
}


// -----------------------------------------------------------------------------
// Section: Overrides to the Particle System Action Interface.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the ParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int MatrixSwarmPSA::typeID( void ) const
{
	return typeID_;
}


INLINE std::string MatrixSwarmPSA::nameID( void ) const
{
	return nameID_;
}



// -----------------------------------------------------------------------------
// Section: Constructor(s) and Destructor for PyMatrixSwarmPSA.
// -----------------------------------------------------------------------------

/**
 *	This is the constructor for PyMatrixSwarmPSA.
 */
INLINE PyMatrixSwarmPSA::PyMatrixSwarmPSA( MatrixSwarmPSAPtr pAction, PyTypePlus *pType ) :
	PyParticleSystemAction( pAction, pType ),
	pAction_( pAction ),
	targetHolders_( pAction_->targets(), this, true )
{
}


// -----------------------------------------------------------------------------
// Section: Accessors to PyMatrixSwarmPSA properties.
// -----------------------------------------------------------------------------


/**
 *	This is the override to the PyParticleSystemAction typeID method.
 *
 *	@return	The typeID for this particular class.
 */
INLINE int PyMatrixSwarmPSA::typeID( void ) const
{
	return pAction_->typeID();
}


/* matrix_swarm_psa.ipp */

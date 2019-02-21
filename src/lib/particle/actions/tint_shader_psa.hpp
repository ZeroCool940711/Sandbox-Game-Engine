/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef TINT_SHADER_PSA_HPP
#define TINT_SHADER_PSA_HPP

#include "particle_system_action.hpp"

/**
 *	This action tints a particle according to its age. If given mulitple tints
 *	over time, it will blend the particle's tint over time to each of the
 *	specified R, G, B, and Alpha components.
 *
 *	A tint is supplied with an age time (in seconds.) 
 */
class TintShaderPSA : public ParticleSystemAction
{
public:
	///	@name Public Types for the TintShader.
	//@{
	typedef std::map<float, Vector4> Tints;
	//@}

	///	@name Constructor(s) and Destructor.
	//@{
	TintShaderPSA();
	//@}

    ParticleSystemActionPtr clone() const;

	///	@name Operations on the TintShaderPSA.
	//@{
	void addTintAt( float time, const Vector4 &tint );
	void clearAllTints( void );
	//@}

	///	@name Accessors to TintShaderPSA properties.
	//@{
	bool repeat( void ) const;
	void repeat( bool flag );

	Vector4ProviderPtr modulator( void ) const;
	void modulator( Vector4ProviderPtr vector );

	float period() const;
	void period( float p );

	float fogAmount( void ) const;
	void fogAmount( float f );

	Tints & tintSet() { return tints_; }
	//@}

	///	@name Overrides to the Particle System Action Interface.
	//@{
	void execute( ParticleSystem &particleSystem, float dTime );
	int typeID( void ) const;
	std::string nameID( void ) const;

	virtual size_t sizeInBytes() const;
	//@}

	static const std::string nameID_;

protected:
	void serialiseInternal(DataSectionPtr pSect, bool load);

private:
	///	@name Auxiliary Variables for the TintShaderPSA.
	//@{
	static int typeID_;			///< TypeID of the TintShaderPSA.

	Tints tints_;				///< The map of Age to Tints.
	bool repeat_;				///< Repeat cycle of Tints.
	float period_;				///< Length of tint cycle.
	float fogAmount_;			///< Amount of fog to blend in.
	Vector4ProviderPtr	modulator_;	///< Global multiplier for the tints.
	//@}
};

typedef SmartPointer<TintShaderPSA> TintShaderPSAPtr;


/*~ class Pixie.TintShaderPSA
 *
 *	TintShaderPSA is a PyParticleSystemAction that tints a particle according
 *	to its age. The initial colour of a particle is ( 0.5, 0.5, 0.5, 1.0 ) in
 *	RGBA at time 0 and tinting occurs linearly from one tint to the next tint
 *	over the time interval between which they are set. Tints can be set at
 *	particular times using the addTintAt function.
 *
 *	A new TintShaderPSA is created using Pixie.TintShaderPSA function.
 */
class PyTintShaderPSA : public PyParticleSystemAction
{
	Py_Header( PyTintShaderPSA, PyParticleSystemAction )
public:
	PyTintShaderPSA( TintShaderPSAPtr pAction, PyTypePlus *pType = &s_type_ );

	int typeID( void ) const;

	///	@name Accessors to PyTintShaderPSA properties.
	//@{
	bool repeat( void ) const;
	void repeat( bool flag );

	float period( void ) const;
	void period( float p );

	float fogAmount( void ) const;
	void fogAmount( float f );

	Vector4ProviderPtr modulator( void ) const;
	void modulator( Vector4ProviderPtr vector );
	//@}

	///	@name Python Interface to the PyStreamPSA.
	//@{
	PyObject *pyGetAttribute( const char *attr );
	int pySetAttribute( const char *attr, PyObject *value );

	PY_FACTORY_DECLARE()

	PY_METHOD_DECLARE( py_addTintAt )
	PY_METHOD_DECLARE( py_clearAllTints )

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( bool, repeat, repeat )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, period, period )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, fogAmount, fogAmount )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( Vector4ProviderPtr, modulator, modulator )
	//@}
private:
	TintShaderPSAPtr pAction_;
};

PY_SCRIPT_CONVERTERS_DECLARE( PyTintShaderPSA )


#ifdef CODE_INLINE
#include "tint_shader_psa.ipp"
#endif

#endif


/* tint_shader_psa.hpp */

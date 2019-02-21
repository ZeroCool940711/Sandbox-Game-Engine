/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef PARTICLE_SYSTEM_ACTION_HPP
#define PARTICLE_SYSTEM_ACTION_HPP

// Standard MF Library Headers.
#include "moo/moo_math.hpp"
#include "pyscript/pyobject_plus.hpp"
#include "pyscript/script.hpp"
#include "pyscript/script_math.hpp"
#include "pyscript/stl_to_py.hpp"

// Standard Library Headers.
#include <map>
#include <string>
#include <set>

// Forward Class Declarations.
class ParticleSystem;
class ParticleSystemAction;
class VectorGenerator;


#define PSA_SOURCE_TYPE_ID			 1
#define PSA_SINK_TYPE_ID			 2
#define PSA_BARRIER_TYPE_ID			 3
#define PSA_FORCE_TYPE_ID			 4
#define PSA_STREAM_TYPE_ID			 5
#define PSA_JITTER_TYPE_ID			 6
#define PSA_SCALAR_TYPE_ID			 7
#define PSA_TINT_SHADER_TYPE_ID		 8
#define PSA_NODE_CLAMP_TYPE_ID		 9
#define PSA_ORBITOR_TYPE_ID			10
#define PSA_FLARE_TYPE_ID			11
#define PSA_COLLIDE_TYPE_ID			12
#define PSA_MATRIX_SWARM_TYPE_ID	13
#define PSA_MAGNET_TYPE_ID			14
#define PSA_SPLAT_TYPE_ID			15

#define PSA_TYPE_ID_MAX				16

typedef SmartPointer<ParticleSystemAction> ParticleSystemActionPtr;
/**
 *	This is the base class defining the interface shared by all particle
 *	system actions.
 *
 *	All actions have a delay attribute. The action will not perform its
 *	actions until after the specified time as passed. There is also a
 *	minimum age attribute. The action will affect only the particles that
 *	are over that minimum age.
 */
class ParticleSystemAction : public ReferenceCount
{
public:
	///	@name Constructor(s) and Destructor.
	//@{
	ParticleSystemAction();
	virtual ~ParticleSystemAction();
	//@}

	static ParticleSystemActionPtr clonePSA(ParticleSystemAction const &action);
    virtual ParticleSystemActionPtr clone() const = 0;

	void serialise(DataSectionPtr pSect, bool load);

	static int nameToType(const std::string & name);
	static const std::string & typeToName(int type);
	static ParticleSystemActionPtr createActionOfType(int type);
	
	// Return the resources required for a ParticleSystemAction
	static void prerequisitesOfType( DataSectionPtr pSection, std::set<std::string>& output );
	static void prerequisites( DataSectionPtr pSection, std::set<std::string>& output ){};

	virtual size_t sizeInBytes() const = 0;

	///	@name Interface Accessors for ParticleSystemAction.
	//@{
	float delay( void ) const;
	void delay( float seconds );

	float minimumAge( void ) const;
	void minimumAge( float seconds );

    std::string const &name() const;
    void name(std::string const &str); 

	virtual void setFirstUpdate() {}

	bool enabled( void ) const;
    void enabled( bool state );
	//@}

	///	@name Interface Methods to the ParticleSystemAction.
	//@{
	virtual void execute( ParticleSystem &particleSystem, float dTime )=0;
	virtual int typeID( void ) const=0;
	virtual std::string nameID( void ) const=0;
	//@}

protected:
	virtual void serialiseInternal(DataSectionPtr pSect, bool load) = 0;

	float delay_;		///< Minimum time in seconds before activation.
	float minimumAge_;	///< Minimum age in seconds for an acted particle.
	float age_;			///< Elapsed time in seconds before activation.
	bool  enabled_;		///< Whether or not the action should be executed.
    std::string name_;
};


PY_SCRIPT_CONVERTERS_DECLARE( ParticleSystemAction )


class PyParticleSystemAction;
typedef SmartPointer<PyParticleSystemAction> PyParticleSystemActionPtr;

/*~ class Pixie.PyParticleSystemAction
 *	PyParticleSystemAction is the base class of all actions that modify the
 *	behaviour of a particle system over time. All actions can have a delay
 *	associated with them (the time an action will wait before affecting the
 *	particle system) and minimumAge (the age a particle must be before it can be
 *	affected by the action).
 *
 *	Multiple PyParticleSystemAction's can be stacked onto the particle system
 *	to create more complex behaviour.
 *
 *	Each PyParticleSystemAction has a typeID integer attribute to represent its type.
 *	They are as follows:
 *	
 *	@{
 *		PSA_SOURCE_TYPE_ID			 1
 *		PSA_SINK_TYPE_ID			 2
 *		PSA_BARRIER_TYPE_ID			 3
 *		PSA_FORCE_TYPE_ID			 4
 *		PSA_STREAM_TYPE_ID			 5
 *		PSA_JITTER_TYPE_ID			 6
 *		PSA_SCALAR_TYPE_ID			 7
 *		PSA_TINT_SHADER_TYPE_ID		 8
 *		PSA_NODE_CLAMP_TYPE_ID		 9
 *		PSA_ORBITOR_TYPE_ID			10
 *		PSA_FLARE_TYPE_ID			11
 *		PSA_COLLIDE_TYPE_ID			12
 *		PSA_MATRIX_SWARM_TYPE_ID	13
 *		PSA_MAGNET_TYPE_ID			14
 *		PSA_SPLAT_TYPE_ID			15
 *	@}
 */
class PyParticleSystemAction : public PyObjectPlus
{
	Py_Header( PyParticleSystemAction, PyObjectPlus )
public:
	PyParticleSystemAction( ParticleSystemActionPtr pAction, PyTypePlus *pType = &s_type_ );
	
	ParticleSystemActionPtr pAction() { return pA_; }

	static PyParticleSystemActionPtr createPyAction(ParticleSystemAction * pAction);

	virtual int typeID( void ) const =0;

	///	@name Python Interface to the ParticleSystemAction.
	//@{
	PyObject *pyGetAttribute( const char *attr );
	int pySetAttribute( const char *attr, PyObject *value );
	//@}

	float delay( void ) const;
	void delay( float seconds );

	float minimumAge( void ) const;
	void minimumAge( float seconds );

	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, delay, delay )
	PY_RW_ACCESSOR_ATTRIBUTE_DECLARE( float, minimumAge, minimumAge )
	PY_RO_ATTRIBUTE_DECLARE( typeID(), typeID )

private:
	ParticleSystemActionPtr pA_;
};


typedef SmartPointer<PyParticleSystemAction> PyParticleSystemActionPtr;


PY_SCRIPT_CONVERTERS_DECLARE( PyParticleSystemAction )


#ifdef CODE_INLINE
#include "particle_system_action.ipp"
#endif

#endif


/* particle_system_action.hpp */

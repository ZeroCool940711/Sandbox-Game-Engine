/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef VECTOR_GENERATOR_HPP
#define VECTOR_GENERATOR_HPP


// Standard MF Library Headers.
#include "moo/moo_math.hpp"
#include "pyscript/pyobject_plus.hpp"
//#include "pyscript/script.hpp"
#include "resmgr/datasection.hpp"

/*~ class BigWorld.VectorGenerator
 *	VectorGenerator is a class that is used by some actions in the
 *	ParticleSystem (most notably SourcePSA and JitterPSA) to specify possible
 *	(or modifications to) positions and velocities for particles.
 *
 *	This class is not instantiated directly, but uses a python sequence to pass
 *	through creation parameters for this class to the underlying C++ objects.
 *	The C++ object looks at the string specifying the type of vector generator
 *	and passes the arguments to the appropriate factory function for that type
 *	of VectorGenerator.
 *
 *	Example (see also SourcePSA):
 *	@{
 *	source = Pixie.SourcePSA( [ "Point", ( 0, 0, 0 ) ],
 *						      [ "Sphere", ( 0, 0, 0 ), 0.4, 0.2 ] )
 *	@}
 *
 *	Possible initial generators are:
 *	
 *	Format: [ "Point", location of the point ]
 *	Point: [ "Point", ( 0, 0, 0 ) ]
 *	Format: [ "Line", start of line, end of line ]
 *	Line: [ "Line", ( 0, 0, 0 ), ( 0, 0.1, 0 ) ]
 *	Format: [ "Sphere", center of sphere, maxRadius, minRadius ]
 *	Sphere: [ "Sphere", ( 0, 0, 0 ), 0.4, 0.2 ]
 *	Box: Format: [ "Box", corner, opposite corner ]
 *	[ "Box", ( 0, 0, 0 ), ( 0.1, 0.1, 0.1 ) ]
 *	Format: [ "Cylinder", center of the first circle endpoint
 *	, center of the second circle endpoint, radius of first circle endpoint
 *	, radius of second circle endpoint ]
 *	Cylinder: [ "Cylinder", ( 0, 0, 0 ), ( 0, 0.1, 0 ), 0.2, 0.4 ]
 *	
 */

/**
 *	This class is the virtual base class for classes used to generate random
 *	3D vectors.
 */
class VectorGenerator
{
public:
	virtual ~VectorGenerator();

	/// This method sets the input vector to a random value.
	virtual void generate( Vector3 &vector ) const = 0;

	/// This method creates a vector generator from Python arguments.
	static VectorGenerator *parseFromPython( PyObject *args );

	// This method saves or restores state via an xml section.
	virtual void serialise( DataSectionPtr pSect, bool load );
	static VectorGenerator * createGeneratorOfType(const std::string & type);

	// the name of the type of vector generator
	virtual const std::string & nameID() = 0;

	virtual float maxRadius() const { return 0.0f; }

	virtual size_t sizeInBytes() const = 0;

protected:
	virtual void serialiseInternal( DataSectionPtr pSect, bool load ) = 0;
};


/**
 *	This child class of VectorGenerator is used to generate 3D vectors at a
 *	fixed point.
 */
class PointVectorGenerator : public VectorGenerator
{
public:
	PointVectorGenerator() :  // BC: added initialization
	  position_(0, 0, 0) 
	{} // for serialisation

	PointVectorGenerator( const Vector3 &point );

	virtual void generate( Vector3 &vector ) const;
	static PointVectorGenerator *parsePythonTuple( PyObject *args );

	const std::string & nameID() { return nameID_; }
	static const std::string nameID_;

	// accessors for editor gizmos
	Vector3 position() const { return position_; }
	void position(const Vector3 & pos) { position_ = pos; }

	virtual size_t sizeInBytes() const { return sizeof(PointVectorGenerator); }

protected:
	virtual void serialiseInternal( DataSectionPtr pSect, bool load );

	Vector3 position_;	///< Point from where the vectors are generated.
};


/**
 *	This child class of VectorGenerator is used to generate 3D vectors along
 *	a line. The line is specified by its start and end points.
 */
class LineVectorGenerator : public VectorGenerator
{
public:
	LineVectorGenerator() :  // BC: added initialization
		origin_(0, 0, 0),
		direction_(0, 0, 0)
	{}

	LineVectorGenerator( const Vector3 &start,
		const Vector3 &end );

	virtual void generate( Vector3 &vector ) const;
	static LineVectorGenerator *parsePythonTuple( PyObject *args );

	const std::string & nameID() { return nameID_; }
	static const std::string nameID_;

	// accessors for editor gizmos
	Vector3 start() const;
	void start(const Vector3 & pos);

	Vector3 end() const;
	void end(const Vector3 & end);

	virtual size_t sizeInBytes() const { return sizeof(LineVectorGenerator); }

protected:
	virtual void serialiseInternal( DataSectionPtr pSect, bool load );

private:
	Vector3 origin_;	///< Origin of the line.
	Vector3 direction_;	///< Direction and length of the line.
};


/**
 *	This child class of VectorGenerator is used to generate 3D vectors in a
 *	cylinder. The line is specified by the centres of its two ends (the
 *	source and destination,) and a maximum and minimum radii. A non-zero
 *	minimum radius means that the cylinder will be hollow.
 */
class CylinderVectorGenerator : public VectorGenerator
{
public:
	CylinderVectorGenerator() : // BC: added initialization
		origin_( 0, 0, 0 ),
		direction_( 0, 0, 0 ),
		maxRadius_( 0 ),
		minRadius_( 0 ),	
		basisU_(0, 0, 0),	
		basisV_(0, 0, 0) 
	{}

	CylinderVectorGenerator( const Vector3 &source,
		const Vector3 &destination,
		float maxRadius,
		float minRadius = 0.0f );

	virtual void generate( Vector3 &vector ) const;
	static CylinderVectorGenerator *parsePythonTuple( PyObject *args );

	const std::string & nameID() { return nameID_; }
	static const std::string nameID_;

	// accessors for editor gizmos
	Vector3 origin() const { return origin_; }
	void origin(const Vector3 & pos);

	Vector3 destination() const { return origin_ + direction_; }
	void destination(const Vector3 & destination);

	virtual float maxRadius() const { return maxRadius_; }
	void maxRadius(float radius) { maxRadius_ = radius; checkRadii(); }

	float minRadius() const { return minRadius_; }
	void minRadius(float radius) { minRadius_ = radius; checkRadii(); }

	virtual size_t sizeInBytes() const { return sizeof(CylinderVectorGenerator); }

protected:
	virtual void serialiseInternal( DataSectionPtr pSect, bool load );

private:
	void constructBasis();
	void checkRadii();

	Vector3 origin_;	///< Origin of the line between the endpoint centres.
	Vector3 direction_;	///< Direction and length of that line.
	float maxRadius_;	///< Maximum radius of the cylinder.
	float minRadius_;	///< Minimum radius of the cylinder.

	Vector3 basisU_;	///< First cross-section basis vector.
	Vector3 basisV_;	///< Second cross-section basis vector.
};


/**
 *	This child class of VectorGenerator is used to generate 3D vectors
 *	in a sphere. The sphere is specified by a centre and a maximum and minimum
 *	radii. A non-zero minimum radius means that the sphere will be hollow.
 */
class SphereVectorGenerator : public VectorGenerator
{
public:
	SphereVectorGenerator() :  // BC: added initialization
		centre_(0, 0, 0),
		maxRadius_(0),
		minRadius_(0)
	{}

	SphereVectorGenerator( const Vector3 &centre,
		float maxRadius,
		float minRadius = 0.0f );

	virtual void generate( Vector3 &vector ) const;
	static SphereVectorGenerator *parsePythonTuple( PyObject *args );

	const std::string & nameID() { return nameID_; }
	static const std::string nameID_;

	virtual float maxRadius() const { return maxRadius_; }

	// accessors for the editor gizmos
	Vector3 centre() const { return centre_; }
	void centre(const Vector3 & pos) { centre_ = pos; }

	void maxRadius(float radius) { maxRadius_ = radius; checkRadii(); }

	float minRadius() const { return minRadius_; }
	void minRadius(float radius) { minRadius_ = radius; checkRadii(); }

	virtual size_t sizeInBytes() const { return sizeof(SphereVectorGenerator); }

protected:
	virtual void serialiseInternal( DataSectionPtr pSect, bool load );

private:
	void checkRadii();	///< check max and min radius are appropriate values

	Vector3 centre_;	///< Origin of the sphere.
	float maxRadius_;	///< Maximum radius of the sphere.
	float minRadius_;	///< Minimum radius of the sphere.
};


/**
 *	This child class of VectorGenerator is used to generate 3D vectors in a
 *	box whose basis vectors are parallel with the x,y,z basis vectors. The
 *	box is specified by two points being its opposing corners.
 */
class BoxVectorGenerator : public VectorGenerator
{
public:
	BoxVectorGenerator() :  // BC: added initialization
		corner_(0, 0, 0),
		opposite_(0, 0, 0)
	{}

	BoxVectorGenerator( const Vector3 &corner,
		const Vector3 &oppositeCorner );

	virtual void generate( Vector3 &vector ) const;
	static BoxVectorGenerator *parsePythonTuple( PyObject *args );

	const std::string & nameID() { return nameID_; }
	static const std::string nameID_;

	// accessors for editor gizmos
	Vector3 corner() const { return corner_; }
	void corner(const Vector3 & pos) { corner_ = pos; }

	Vector3 oppositeCorner() const { return opposite_; }
	void oppositeCorner(const Vector3 & pos) { opposite_ = pos; }

	virtual size_t sizeInBytes() const { return sizeof(BoxVectorGenerator); }

protected:
	virtual void serialiseInternal( DataSectionPtr pSect, bool load );

private:
	Vector3 corner_;	///< One corner of the box.
	Vector3 opposite_;	///< The opposite corner of the box.
};


#ifdef CODE_INLINE
#include "vector_generator.ipp"
#endif

#endif


/* vector_generator.hpp */

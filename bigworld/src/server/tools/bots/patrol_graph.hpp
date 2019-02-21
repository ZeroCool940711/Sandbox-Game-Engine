/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef PATROL_GRAPH_HPP
#define PATROL_GRAPH_HPP

#include "movement_controller.hpp"

#include "math/vector3.hpp"
#include <map>
#include <vector>
#include <map>
class DataSection;

namespace Patrol
{

class Graph;

/**
 *	 This class implements a patrol node that is used by the patrol graph.
 */
class Node
{
public:
	class Properties
	{
	public:
		Properties() :
			minStay_( 0.f ),
			maxStay_( 0.f ),
			radius_( 0.f ),
			minSpeed_( 0.f ),
			maxSpeed_( 0.f )
		{}

		void init( DataSection * pSection );

		float minStay() const	{ return minStay_; }
		float maxStay() const	{ return maxStay_; }
		float radius() const	{ return radius_; }
		float minSpeed() const	{ return minSpeed_; }
		float maxSpeed() const	{ return maxSpeed_; }

	private:
		float minStay_;
		float maxStay_;
		float radius_;
		float minSpeed_;
		float maxSpeed_;

		friend class Graph;
	};

	Node();

	typedef std::map< std::string, int > NodeMap;

	bool init1( int index, DataSection * pSection,
			const Properties & defaultProps );
	bool init2( const NodeMap & map, DataSection * pSection );

	float flatDistTo( const Vector3 & pos ) const;
	float flatDistSqrTo( const Vector3 & pos ) const;

	int nextNode() const;
	int randomisePosition( Vector3 & pos, const Graph & graph ) const;

	Vector3 generateDestination() const;
	float generateStayTime() const;
	float generateSpeed( float ospeed ) const;

	const std::string & name() const	{ return name_; }
	const Properties & props() const	{ return props_; }

	const Vector3 & position() const	{ return position_; }

private:
	typedef std::vector< int > Adjacencies;
	Adjacencies toNodes_;
	Vector3 position_;
	int id_;
	std::string name_;

	Properties props_;

	friend class Graph;
};


/**
 *	 This class implements a patrol graph that bots can follow.
 */
class Graph
{
public:
	Graph();
	bool init( DataSection * pSection, float scalePos, float scaleSpeed );

	int findClosestNode( const Vector3 & position ) const;
	const Node * node( int index ) const;

	int size() const		{ return nodes_.size(); }

private:
	typedef std::vector< Node > Nodes;
	Nodes nodes_;
};


/**
 *	This class is used to manage a collection of graphs.
 */
class Graphs
{
public:
	Graphs();
	Graph * getGraph( const std::string & graphName,
		float scalePos, float scaleSpeed );

private:
	typedef std::map< std::string, Graph > Map;
	Map graphs_;
};


/**
 *	This class is used to traverse a patrol graph.
 */
class GraphTraverser : public MovementController
{
public:
	GraphTraverser( const Graph & graph, float & speed,
			Vector3 & startPosition, bool randomise, bool snap );
	virtual bool nextStep( float & speed, float dTime,
			Vector3 & pos, Direction3D & dir );

protected:
	int destinationNode_;
	Vector3 destinationPos_;
	float stayTime_;
	const Graph & graph_;
};

} // namespace Patrol

#endif // PATROL_GRAPH_HPP

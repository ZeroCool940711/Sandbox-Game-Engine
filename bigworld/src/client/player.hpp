/******************************************************************************
BigWorld Technology
Copyright BigWorld Pty, Ltd.
All Rights Reserved. Commercial in confidence.

WARNING: This computer program is protected by copyright law and international
treaties. Unauthorized use, reproduction or distribution of this program, or
any portion of this program, may result in the imposition of civil and
criminal penalties as provided by law.
******************************************************************************/

#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "entity.hpp"
#include "physics.hpp"
#include "particle/py_meta_particle_system.hpp"

class MatrixProvider;
typedef SmartPointer<MatrixProvider> MatrixProviderPtr;
class EntityPhotonOccluder;

namespace Moo {
typedef SmartPointer< class RenderTarget > RenderTargetPtr;
};

/**
 *	This class stores a pointer to the player entity, and other
 *	player specific objects such as the current physics controller.
 */
class Player
{
public:
	Player();
	~Player();

	bool setPlayer( Entity * newPlayer );

	static void poseUpdateNotification( Entity * pEntity );

	Entity *	i_entity()		{ return pEntity_; }

	MatrixProviderPtr camTarget();

	static Player & instance();
	void updateWeatherParticleSystems( class PlayerAttachments & pa );

	static Entity *		entity()			{ return instance().i_entity(); }

	Chunk * findChunk();

	bool drawPlayer( Moo::RenderTargetPtr renderTexture, bool useDepth = false );

private:

	struct AttachedPS
	{
		SmartPointer<PyModelNode> pNode_;
		PyMetaParticleSystem* pAttachment_;
	};

	std::vector< AttachedPS > attachedPSs_;

	Entity		* pEntity_;

	EntityPhotonOccluder * playerFlareCollider_;
};

#endif // PLAYER_HPP
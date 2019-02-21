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

#pragma warning(disable:4786)	// remove "truncated identifier" warnings from STL

#include "action_matcher.hpp"

#include "entity.hpp"
#include "duplo/pymodel.hpp"
#include "duplo/action_queue.hpp"
#include "player.hpp"

#include "cstdmf/debug.hpp"
#include "cstdmf/dogwatch.hpp"
#include "app.hpp"

DECLARE_DEBUG_COMPONENT2( "Entity", 0 )


bool ActionMatcher::globalEntityCollision_ = false;
bool ActionMatcher::debug_ = false;
bool ActionMatcher::matchBots_ = true;


PY_TYPEOBJECT( ActionMatcher )

PY_BEGIN_METHODS( ActionMatcher )
PY_END_METHODS()

PY_BEGIN_ATTRIBUTES( ActionMatcher )
	/*~ attribute ActionMatcher.matchCaps
	 *
	 *	This attribute specifies which user defined capabilities the
	 *	ActionMatcher is matching at the moment.  It is a list of numbers
	 *	between 0 and 31.  It is checked against the &lt;capsOn> and
	 *	&lt;capsOff> fields (in the .model file definition of each action) for
	 *	each action, and the ActionMatcher will only match those actions for
	 *	which all caps in the &lt;capsOn >field are on in matchCaps	and all
	 *	those in &lt;capsOff> are NOT on.
	 *
	 *	Read-Write List of Integers (0-31)
	 */
	PY_ATTRIBUTE( matchCaps )
	/*~ attribute ActionMatcher.entityCollision
	 *
	 *	Specifies whether the model responds to collisions between entities.
	 *	If this is true then when the parent entity collides with another
	 *	entity, then one model, or the other, or both, will be moved away from
	 *	other model so that they dont overlap.   Which model moves is
	 *	determined by the collisionRooted flag on the ActionMatcher on both
	 *	models.  If collisionRooted is set on either model, then that model
	 *	will NOT move away from the entity, leaving the onus for movement on
	 *	the other model.  If both have it set, then neither will move.  If it
	 *	is clear on both of them then they will both move half as far.  It
	 *	defaults to 0.
	 *	Note that it is the model that moves, rather than than the entity,
	 *	making this ta client side effect only.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( entityCollision )
	/*~ attribute ActionMatcher.collisionRooted
	 *
	 *	This attribute controls how the model responds to entity to entity
	 *	collisions.  The entityCollision attribute controls whether the entity
	 *	will contribute towards collisions at all, collisionRooted controls
	 *	whether this model will move in response to collisions.  This model
	 *	will move directly away from the entity it has collided with, until its
	 *	bounding box no longer overlaps that of the model belonging to the
	 *	collided entity.  Note that it is the model	that moves, rather than the
	 *	entity, making this a client side effect only.
	 *
	 *	If this is non-zero, then it will NOT move in response to collisions,
	 *	otherwise it will.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( collisionRooted )
	/*~ attribute ActionMatcher.matcherCoupled
	 *
	 *	This attribute switches the ActionMatcher on and off. If the attribute
	 *	is false (0), then the ActionMatcher is	decoupled, otherwise, it is
	 *	coupled.  It defaults to 1.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( matcherCoupled )
	/*~ attribute ActionMatcher.inheritOnRecouple
	 *
	 *	This attribute is used for player entities to smooth their motion when
	 *	the ActionMatcher is switched on after being off (which is done by
	 *	setting matcherCoupled to 1).  If inheritOnRecouple is true, then the
	 *	Entities position is set to the models position, otherwise the model is
	 *	moved immediately to the entities position, which can cause a visual
	 *	jar.  It defaults to 1.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( inheritOnRecouple )
	/*~ attribute ActionMatcher.turnModelToEntity
	 *
	 *	This attribute controls whether or not the ActionMatcher turns the
	 *	model to track direction changes in the Entity.  If it is true, then
	 *	the models yaw will be set to the Entities yaw.  If it is false, then
	 *	the action matcher doesn't control the model's yaw at all.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( turnModelToEntity )
	/*~ attribute ActionMatcher.matchModelScale
	 *
	 *	This attribute controls whether the model�s scale is taken into 
	 *	account when choosing an action. Note: Only the Z scale is used 
	 *	as in the following entity.model.scale[2]
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( matchModelScale )
	/*~ attribute ActionMatcher.useEntityPitchAndRoll
	 *
	 *	This attribute controls whether the model should use the pitch and
	 *	roll of the entity.
	 *
	 *	@type Read-Write Boolean.
	 */
	PY_ATTRIBUTE( useEntityPitchAndRoll )
	/*~ attribute ActionMatcher.maxCollisionLod
	 *
	 *	If the LOD (level of detail) of the model is less than this amount,
	 *	then the model will be moved to respond to entity-entity collisions.
	 *	If it is greater than this amount, then the model won't respond to
	 *	collisions.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( maxCollisionLod )
	/*~ attribute ActionMatcher.bodyTwistSpeed
	 *
	 *	The angular speed at which the model will turn to line up with the
	 *	entity in radians per second.  Defaults to 2pi.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( bodyTwistSpeed )
	/*~ attribute ActionMatcher.footTwistSpeed
	 *
	 *	The angular speed at which the model can twist to match its
	 *	unitRotation in	radians per second. Defaults to 3pi.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( footTwistSpeed )
	/*~	attribute ActionMatcher.fallNotifier
	 *
	 *	The callback function to call when the entity transitions between
	 *	falling and not-falling.  On callback the function will receive one
	 *	argument, falling, which is 1 if the entity has started to fall, and 0
	 *	if it has just stopped falling.  The ActionMatcher decides if the
	 *	entity is falling based on the action that it chooses.  If the
	 *	minEntitySpeed of the trigger is less than zero, then it assumes that
	 *	the entity is falling. This will be the case when the falling checkbox is selected
	 *  in the model editor for an action. 
	 *
	 *	@type	Function object
	 */
	PY_ATTRIBUTE( fallNotifier )
	/*~ attribute ActionMatcher.fallSelected
	 *
	 *	This attribute is set to true if the ActionMatcher chose a falling
	 *	action in its most recent choice.  The ActionMatcher decides if the
	 *	entity is falling based on the action that it chooses.  If the
	 *	minEntitySpeed of the trigger is less than zero, then it assumes that
	 *	the entity is falling.
	 *
	 *	@type	Integer
	 */
	PY_ATTRIBUTE( fallSelected )
	/*~ attribute ActionMatcher.lastMatch
	 *
	 *	This attribute is set to the name of the last action chosen by the
	 *	action matcher.
	 *
	 *	@type	Read-Only String
	 */
	PY_ATTRIBUTE( lastMatch )
	/*~ attribute ActionMatcher.fuse
	 *
	 *	Fuse is the attribute that tracks how long (in seconds) a model has
	 *	been bored (playing the same action).  Once it exceeds patience, the
	 *	boredNotifier callback is called.  This works for all animations, not
	 *	just idle animations.  It is up	to the callback function to process
	 *	different types of animation.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( fuse )
	/*~ attribute ActionMatcher.patience
	 *
	 *	Patience is how long the model can continue playing the same action
	 *	for, before it becomes bored.  Fuse keeps track	of how long the same
	 *	animation has been playing. Once fuse exceeds patience, the
	 *	boredNotifier callback is called.
	 *
	 *	@type	Float
	 */
	PY_ATTRIBUTE( patience )
	/*~ attribute ActionMatcher.boredNotifier
	 *
	 *	The boredNotifer is the callback function to call once fuse exceeds patience.
	 *	As an animation plays, fuse will increase.  When a new animation is played,
	 *	fuse will be reset.  The patience parameter is set by script.  If 0, the
	 *	boredNotifier will not be triggered.  The callback can be defined as follows:
	 *
	 *	@{
	 *		def onBored( self, actionName )
	 *	@}
	 *
	 *	The name of the action is that described in the .model file under
	 *	<action>/<name>.  It can be set in the Action panel of ModelViewer.
	 *
	 *
	 *	@type	Function object
	 */
	PY_ATTRIBUTE( boredNotifier )
	/*~ attribute ActionMatcher.startMovingNotifier
	 *
	 *	The startMovingNotifier is a callback function to call when the entity
	 *	changes from a stationary to a moving state. This callback's purpose is
	 *	to provide the opportunity to the script programmer to clear the action
	 *	queue in the even that it had been set explicitly in script. The
	 *	callback can be defined as follows:
	 *
	 *	@{
	 *		def onStartMoving( self, actionName ):
	 *			for actionName in self.model.queue:
	 *				self.model.action(actionName).stop()
	 *			pass
	 *
	 *		self.am.startMovingNotifier = onStartMoving
	 *	@}
	 *
	 *	The name of the action is that described in the .model file under
	 *	<action>/<name>.  It can be set in the Action panel of ModelViewer.
	 *
	 *
	 *	@type	Function object
	 */
	PY_ATTRIBUTE( startMovingNotifier )
	/*~ attribute ActionMatcher.useEntityModel
	 *
	 *	The useEntityModel attribute makes the action matcher use the entity's
	 *	model to check for movement and rotation, instead of the action
	 *	matcher's owner's model.
	 *	
	 *	This is particularly useful if the action matcher is constructed with
	 *	and entity that is different than the owner of the model that this
	 *	action matcher is used by.
	 *
	 *	@type	Boolean
	 */
	PY_ATTRIBUTE( useEntityModel )
PY_END_ATTRIBUTES()

/*~	function BigWorld.ActionMatcher
 *
 *	This function returns a new ActionMatcher Motor object, which is used to match the
 *	movements and orientation of an entity to the actions defined on its primary model.
 *	Note that the primary Model will be assigned an ActionMatcher Motor by default.
 *
 *	@return new ActionMatcher object
 */
PY_FACTORY( ActionMatcher, BigWorld )

/**
 *	Constructor
 */
ActionMatcher::ActionMatcher( Entity * pEntity, PyTypePlus * pType ) :
	Motor( pType ),
	pEntity_( pEntity ),
	oldSpeed_( 0 ),
	oldSpeedDelta_( 0 ),
	oldSpeedFor_( 0 ),
	oldFootTwist_( 0 ),
	entityCollision_( false ),
	collisionRooted_( true ),
	matcherCoupled_( true ),
	inheritOnRecouple_( true ),
	turnModelToEntity_( false ),
	matchModelScale_( true ),
	useEntityModel_( false ),
	useEntityPitchAndRoll_( false ),
	maxCollisionLod_( 100.f ),
	bodyTwistSpeed_( DEG_TO_RAD( 360.0f ) ),
	footTwistSpeed_( DEG_TO_RAD( 540.0f ) ),
	fallSelected_( false ),
	fuse_( 0.f ),
	patience_( 0.f ),
	lastVehicleID_( 0 )
{
	BW_GUARD;
}


/**
 *	Destructor.
 */
ActionMatcher::~ActionMatcher()
{
	BW_GUARD;
	MF_ASSERT_DEV( pOwner_ == NULL );
}


/**
 *	This method is called by our base class after it attaches to a model.
 *
 *	We override it to get information out the model we attach to, and
 *	also set the world transform of the model to be that of the entity.
 */
void ActionMatcher::attached()
{
	BW_GUARD;
	MF_ASSERT_DEV( pActions_.empty() );

	// gather the list of matchable actions
	if (pOwner_->pSuperModel())
	{
		pOwner_->pSuperModel()->getMatchableActions( pActions_ );
	}
	lastAction_ = pActions_.end();
	fuse_ = 0.f;
	if (patience_ < 0.f) patience_ = -patience_;

	// Note: we don't reinitialise all the 'oldXXX' variables here...
	//  because if we're detached from somewhere and reattached elsewhere,
	//  chances are these are exactly the things the user wants to keep.

	// set the the model's transform to the entity's
	Matrix	world;
	if (pEntity_)
	{
		world.setTranslate( pEntity_->position() );
		world.preRotateY( pEntity_->auxVolatile()[0] );
		if (pActions_.size() == 0)
		{
			world.preRotateX( pEntity_->auxVolatile()[1] );
			world.preRotateZ( pEntity_->auxVolatile()[2] );
		}
	}
	else
	{
		world.setIdentity();
	}
	pOwner_->worldTransform( world );

	this->rev( static_cast<float>( App::instance().getTimeDelta() ) );
}

/**
 *	This method is called by our base class after it detaches from a model.
 *
 *	We override it to clean stuff up, releasing references to the supermodel.
 */
void ActionMatcher::detached()
{
	BW_GUARD;
	if (!pActions_.empty())	// empty <=> capacity zero for us
	{						// (i.e. no mem block if empty)
		pActions_.clear();
	}
}



/**
 *	Static python factory method
 */
PyObject * ActionMatcher::pyNew( PyObject * args )
{
	BW_GUARD;
	PyObject * pObject;
	if ( !PyArg_ParseTuple( args, "O", &pObject ) ||
		!Entity::Check( pObject ))
	{
		PyErr_SetString( PyExc_TypeError,
			"BigWorld.ActionMatcher: Expected an entity" );
		return NULL;
	}

	return new ActionMatcher( (Entity*)pObject );
}


/**
 *	Get attribute method.
 */
PyObject * ActionMatcher::pyGetAttribute( const char * attr )
{
	BW_GUARD;
	PY_GETATTR_STD();

	return Motor::pyGetAttribute( attr );
}

/**
 *	Set attribute method.
 */
int ActionMatcher::pySetAttribute( const char * attr, PyObject * value )
{
	BW_GUARD;
	PY_SETATTR_STD();

	return Motor::pySetAttribute( attr, value );
}


/**
 *	Specialised set method for the 'matcherCoupled' attribute
 */
int ActionMatcher::pySet_matcherCoupled( PyObject * value )
{
	BW_GUARD;
	bool isPlayer = (pEntity_.good() && Player::entity() == &*pEntity_);

	bool	oldMC = matcherCoupled_;
	int ret = Script::setData( value, matcherCoupled_, "matcherCoupled" );
	if (isPlayer && ret == 0)
	{
		if(pOwner_ != NULL && pEntity_.good() && matcherCoupled_ != oldMC)
		{
			// If we just turned ON the matcher coupling from being off,
			if (matcherCoupled_)
			{
				// and if we're the player and want to be smooth,
				if (inheritOnRecouple_)
				{
					// then copy the model's world position into ours.
					pEntity_->position( pOwner_->worldTransform().applyToOrigin() );
					// Scripts should really call teleport on physics before
					// this time to give other clients the chance to have the
					// right position by the time they've finished the action.
				}
				else
				{
					// Otherwise immediately set the model's world position to
					// the entity's, since it's going to pop next rev anyway
					// (when we will be turned on), and we don't want to match
					// some weird action to that pop (such as a deadly fall :)
					Matrix world = pOwner_->worldTransform();
					world.translation( pEntity_->position() );
					pOwner_->worldTransform( world );
				}
			}
			else
			{
				// cancel matched action
				PyModel & model = *pOwner_;
				model.actionQueue().setMatch(NULL);
			}
		}

		inheritOnRecouple_ = true;
	}
	return ret;
}








/**
 * TODO: to be documented.
 */
struct NiceV3
{
public:
	NiceV3( const Vector3 & v ) : v_(v) {}
	const Vector3 & v_;
};

std::ostream& operator <<( std::ostream& o, const NiceV3& t )
{
	return o << '(' << t.v_[0] << ',' << t.v_[1] << ',' << t.v_[2] << ')';
}


static DogWatch dwMatch("Match");
static DogWatch dwMatchLoop("Loop");
/*
static DogWatch dwMatchPreLoop("PreLoop");
static DogWatch dwMatchZero("Zero");
static DogWatch dwMatchStart("Start");
static DogWatch dwMatchStart2("Start2");
static DogWatch dwMatchStart3("Start3");
static DogWatch dwMatchEnd("End");
static DogWatch dwMatchBored("Bored");
static DogWatch dwMatchFall("Fall");
*/


/**
 *	This function implements the raison d'etre of this class.
 *
 *	@see ActionMatcher
 */
void ActionMatcher::rev( float dTime )
{
	BW_GUARD;
	MF_ASSERT_DEV( pOwner_ != NULL );

	if (!matcherCoupled_) return;

	Entity * pEntity = &*pEntity_;
	if (!pEntity) return;

	dwMatch.start();

	// dwMatchStart.start();
	PyModel & model = *pOwner_;

	// get current entity and model poses
	Vector3		newEntityPos = pEntity->position();
	float		newEntityYaw = pEntity->auxVolatile()[0];
	float		newEntityPitch = pEntity->auxVolatile()[1];
	float		newEntityRoll = pEntity->auxVolatile()[2];

	// dwMatchStart3.start();
	Matrix world;
	if ( useEntityModel_ )
	{
		if (pEntity->pPrimaryModel())
		{
			world = pEntity->pPrimaryModel()->worldTransform();
		}
		else
		{
			world.setRotate( newEntityYaw, newEntityPitch, newEntityRoll );
			world.postTranslateBy( newEntityPos );
		}
	}
	else
	{
		world = model.worldTransform();	
	}
	// dwMatchStart3.stop();

	// Check for Entity-to-Entity collision here.
	if (globalEntityCollision_ && entityCollision_ && !collisionRooted_ &&
		(model.pSuperModel() == NULL || maxCollisionLod_ <= 0.f ||
			model.pSuperModel()->lastLod() < maxCollisionLod_))
	{
		this->doEntityCollisions( newEntityPos );
	}
	// dwMatchStart.stop();

	// if this model doesn't have any movement actions, get out now
	if (pActions_.empty())
	{
		// dwMatchZero.start();
		Matrix newWorld;
		newWorld.setRotate( newEntityYaw, newEntityPitch, newEntityRoll );
		newWorld.postTranslateBy( newEntityPos );

		// check this because setting the world transform is expensive
		if (newWorld != world)
		{
			model.worldTransform( newWorld );
		}

		// dwMatchZero.stop();
		dwMatch.stop();
		return;
	}

	Vector3		oldModelPos = world.applyToOrigin();
	float		oldModelYaw = world.yaw();
	float		oldModelPitch = world.pitch();
	float		oldModelRoll = world.roll();

	// fiddle with the old position if we were on a vehicle
	// TODO: 1. Make it work for moving spaces.
	// TODO: 2. Do it all a better way! (e.g. store auto/selfDisplacement_ in
	//  Entity when 'pos' called? careful of transitions...)
	if (pEntity->isOnVehicle())
	{
		EntityID currVehicleID = pEntity->pVehicle()->id();
		const Vector3 & currVehiclePos = pEntity->pVehicle()->position();
		float currVehicleYaw = pEntity->pVehicle()->auxVolatile()[0];
		if (lastVehicleID_ == currVehicleID)
		{
			oldModelPos += currVehiclePos - lastVehiclePos_;
			oldModelYaw += currVehicleYaw - lastVehicleYaw_;
		}
		else
		{
			lastVehicleID_ = currVehicleID;
		}
		lastVehiclePos_ = currVehiclePos;
		lastVehicleYaw_ = currVehicleYaw;
	}
	else
	{
		lastVehicleID_ = 0;
	}

	// initially the model isn't going anywhere
	Vector3		newModelPos = oldModelPos;
	float		newModelYaw = oldModelYaw;
	float		newModelPitch = oldModelPitch;
	float		newModelRoll = oldModelRoll;

	float		newFootTwist = 0.f;


	// find the change in speed that this entails
	Vector3		velocity;


	if( pEntity->velocity().lengthSquared() > 0 )
	{
		velocity = pEntity->velocity();
		velocity.normalise();
		velocity *= ( newEntityPos - oldModelPos ).length() * ( 1.0f / dTime );
	}
	else
		velocity = ( newEntityPos - oldModelPos ) * ( 1.0f / dTime );

	float	deltaFall = velocity[1];
	Vector3	horizontalVelocity = velocity;
	horizontalVelocity[1] = 0.f;


	const bool startedMoving =	oldSpeed_ < 0.1f &&
								horizontalVelocity.length() >= 0.1f;

	// if entity is player and no impacting actions are affecting its position,
	// then estimate the eventual speed as a multiplier of current speed.
	float		blendingSpeedFactorEstimate = 1.f;
	if (pEntity == Player::entity() &&
		model.actionQueue().lastPromotion().lengthSquared() < 0.1f)
	{
		// If this entity is the player then we compensate for its
		// velocity smoothing (i.e. momentum) to determine the speed
		// it's building up/down to, so we know what action to blend in to

		// i.e. when we detect a change in velocity (an acceleration)
		// we pretend it's going to change smoothly at that rate for
		// the standard blendInTime (0.3) seconds.

		// Another way to do this (for every entity) could be to say
		// 'OK, we're doing this set of movement actions at these blend
		// ratios which make for a certain apparent velocity. Now what's the
		// best action we can select to make that velocity look like the
		// entity's velocity, bearing in mind that if we select a different
		// action it will only add as much as it can blend in over this
		// next frame.' And no, I can't find the words to express that
		// more concisely right now :)

		oldSpeedFor_ += dTime;

		float		newSpeed = horizontalVelocity.length();
		float		newSpeedDelta = newSpeed - oldSpeed_;
		if (fabs(newSpeedDelta) > 1.0f*dTime)	// change of more than 1m/s/s
		{
			// have we already seen such a change?
			if (fabs(newSpeedDelta - oldSpeedDelta_) > 0.2f ||
				oldSpeedFor_ > 0.3f)
			{		// nope, remember this as the start (or close to it)
				oldSpeedFor_ = 0.f;
			}

			// ok, scale this delta up as if it took 0.3s
			float estimateSpeed = oldSpeed_ +
				newSpeedDelta * ((0.3f-oldSpeedFor_) + dTime)/dTime;
			// don't let it get too small
			if (estimateSpeed < 0) estimateSpeed = 0;
			// don't let it get too big
			if (estimateSpeed > 10.f && estimateSpeed > newSpeed)
				estimateSpeed = newSpeed;

			if (newSpeed > 0.001f)
			{
				blendingSpeedFactorEstimate = estimateSpeed/newSpeed;
			}
		}

		oldSpeed_ = newSpeed;
		oldSpeedDelta_ = newSpeedDelta;
	}
	else
	{
		oldSpeed_ = horizontalVelocity.length();
	}

	// dwMatchPreLoop.start();
	// express this displacement from the model's point of view
	Matrix	rotor;
	rotor.setRotateY( -oldModelYaw );
	Vector3 deltaRot = rotor.applyPoint( horizontalVelocity );

	float modelScaleFactor = 1.0f;
	if( matchModelScale_ && pEntity->pPrimaryModel() )
	{
		modelScaleFactor = pEntity->pPrimaryModel()->worldTransform().applyToUnitAxisVector(2).length();
	}


	float rLen = deltaRot.length() * blendingSpeedFactorEstimate * modelScaleFactor;
	float rYaw = deltaRot.yaw();
	float rAux1 = Angle( newEntityYaw - newModelYaw );
	Capabilities rCaps = matchCapabilities_;
	// dwMatchPreLoop.stop();

	dwMatchLoop.start();

	// and find a matching action!
	SMActionVector::iterator i;
	for (i = pActions_.begin(); i != pActions_.end(); i++)
	{
		const MatchInfo & mi = (*i)->pSource_->matchInfo_;

		// does this action trigger?
		if (mi.trigger.satisfies( rCaps, rLen, rYaw, rAux1 ))
		{
			break;			// yep
		}

		bool trigCapsOK = rCaps.match( mi.trigger.capsOn, mi.trigger.capsOff );
		// does it trigger by falling?
		if (trigCapsOK && mi.trigger.minEntitySpeed < 0.f &&
			deltaFall < mi.trigger.minEntitySpeed &&
			deltaFall > mi.trigger.maxEntitySpeed)
		{
			break;			// yep
		}

		// were we doing this action last time?
		if (lastAction_ != i)
		{
			continue;		// nope
		}

		// have we gone on too long?
		if (mi.oneShot)
		{
			float	matchCycleCount = model.actionQueue().matchCycleCount() +
				(*i)->pSource_->blendOutTime_ / (*i)->pFirstAnim_->duration_;

			if (matchCycleCount >= 1.f)
			{
				continue;	// yep
			}
		}

		// so does it cancel?
		if (mi.cancel.satisfies( rCaps, rLen, rYaw, rAux1 ))
		{
			continue;		// yep
		}

		// so shall we keep doing this one then?
		break;				// yep
	}
	dwMatchLoop.stop();

	// dwMatchEnd.start();

	// ok, what did we find to match that movement?
	SMActionVector::iterator	origI = i;
	bool fallSelected = false;
	float inTime = -1.0f;
	if (i == pActions_.end())
	{
		newModelPos = newEntityPos;		// just teleport the model
		newModelYaw = newEntityYaw;
		newModelPitch = newEntityPitch;
		newModelRoll = newEntityRoll;

		//DEBUG_MSG( "AM: rLen %g, rYaw %g: None\n", rLen, rYaw );
		model.actionQueue().setMatch( NULL );
	}
	else
	{
		if ((*i)->pSource_->matchInfo_.scalePlaybackSpeed)
		{
			const ModelAnimation * pAnim = (*i)->pFirstAnim_;
			float	dbLen = pAnim->flagFactorBit( 0 ).applyToOrigin().length() /
				pAnim->duration_;

			//float	drLen = desiredOffset.length();
			Vector3 speedOffset = velocity * dTime;
			speedOffset[1] = max(0.f,speedOffset[1]);
			float drLen = speedOffset.length();

			if (dbLen != 0.0f)
			{
				// nickl TODO

				// inTime = dTime * ( drLen / (dbLen * dTime) );
				inTime = model.moveScale() * drLen / dbLen;	// cancelled out 'dTime'
			}
		}

		newModelPos = newEntityPos;

		bool turnModelToEntity = turnModelToEntity_;
		if ((*i)->pSource_->matchInfo_.feetFollowDirection)
		{
			// point the feet in the direction of movement
			float naturalYaw = 0.f;
			if ((*i)->pSource_->isMovement_)
			{
				const Vector3 naturalDir =
					model.initialItinerantContextInverse().applyVector(
					(*i)->pFirstAnim_->flagFactorBit( 0 ).applyToOrigin() )*-1.f;
				naturalYaw = atan2f(naturalDir[0],naturalDir[2]);
			}

			newFootTwist = Angle( rYaw - naturalYaw );

			turnModelToEntity = true;
		}
		if (turnModelToEntity)
		{
			// turn the model to the yaw of the entity
			//  (pretty quick smart too)
			const float bodyTwistMax = bodyTwistSpeed_ * dTime;
			newModelYaw = oldModelYaw + Math::clamp( bodyTwistMax,
				float(Angle(newEntityYaw - oldModelYaw)) );

			if (useEntityPitchAndRoll_)
			{
				newModelPitch = oldModelPitch + Math::clamp( bodyTwistMax,
					float(Angle(newEntityPitch - oldModelPitch)) );

				newModelRoll = oldModelRoll + Math::clamp( bodyTwistMax,
					float(Angle(newEntityRoll - oldModelRoll)) );
			}
		}

		// smooth the foot twist, regardless of who set it
		const float footTwistMax = footTwistSpeed_ * dTime;
		newFootTwist = oldFootTwist_ + Math::clamp( footTwistMax,
			newFootTwist - oldFootTwist_ );

		//DEBUG_MSG( "AM: rLen %g, rYaw %g, rAux1 %g: %s\n", rLen, rYaw, rAux1,
		//	(*i)->pSource->name_.c_str() );
		model.actionQueue().setMatch( *i, dTime, inTime,
			(*i)->pSource_->matchInfo_.promoteMotion );

		if (!model.actionQueue().lastMatchAccepted())
		{
//			dprintf( "Match not accepted\n" );
			i = pActions_.end();
		}
		else
		{
			// record if we sucessfully (as far as we can tell)
			// suggested a falling action
			fallSelected =
				(*i)->pSource_->matchInfo_.trigger.minEntitySpeed < 0.f;
		}
	}

	// dwMatchFall.start();
	// if we've changed whether or not we're falling then call the notifier
	// (this system should really be a bit more generic)
	if (fallSelected_ != fallSelected)
	{
		fallSelected_ = fallSelected;
		if (fallNotifier_)
		{
			Py_INCREF( &*fallNotifier_ );
			Script::call(
				&*fallNotifier_,
				Py_BuildValue( "(i)", int( fallSelected ) ),
				"ActionMatcher fall notifier callback: " );
		}
	}
	// dwMatchFall.stop();



	if (startedMoving && startMovingNotifier_)
	{
		//MF_ASSERT_DEV(PyCallable_Check(startMovingNotifier_));

		Py_INCREF( &*startMovingNotifier_ );

		if (i != pActions_.end())
		{
			Script::call(
				&*startMovingNotifier_,
				Py_BuildValue( "(s)", (*i)->pSource_->name_.c_str() ),
				"ActionMatcher start moving notifier on: " );
		}
		else
		{
			Script::call(
				&*startMovingNotifier_,
				Py_BuildValue( "(O)", Py_None ),
				"ActionMatcher start moving notifier on: " );
		}
	}


	// dwMatchBored.start();
	// see if we're bored
	if (boredNotifier_)
	{
		if (patience_ > 0.f)
		{
			if (i == lastAction_ && i != pActions_.end())
			{
				fuse_ += dTime;
				if (fuse_ >= patience_)
				{
					patience_ = -patience_;
					boredAction_ = i;
					Py_INCREF( &*boredNotifier_ );
					Script::call(
						&*boredNotifier_,
						Py_BuildValue( "(s)", (*i)->pSource_->name_.c_str() ),
						"ActionMatcher bored notifier on: " );
				}
			}
			else
			{
				fuse_ = 0.f;
			}
		}
		else if (patience_ < 0.f)
		{
			if (origI != boredAction_)
			{
				patience_ = -patience_;
				Py_INCREF( &*boredNotifier_ );
				Script::call(
					&*boredNotifier_,
					Py_BuildValue( "(O)", Py_None ),
					"ActionMatcher bored notifier off: " );
			}
		}
	}
	// dwMatchBored.stop();
/*
	if (lastAction_ != i)
	{
		if (i == pActions_.end())
		{
			dprintf( "Choose no action because rLen %f, rYaw %f rAux1 %f\n",
				rLen, rYaw, rAux1 );
		}
		else
		{
			dprintf( "Choose new action %s\n", (*i)->pSource->name_.c_str() );
		}
	}
*/
	lastAction_ = i;


	// apply the changes to the model's pose
	world.preRotateY( newModelYaw - oldModelYaw );
	if (useEntityPitchAndRoll_)
	{
		world.preRotateX( newModelPitch - oldModelPitch );
		world.preRotateZ( newModelRoll - oldModelRoll );
	}

	world.translation( newModelPos );
	model.worldTransform( world );


	oldFootTwist_ = newFootTwist;


	model.unitRotation( newFootTwist );



	if (ActionMatcher::debug_)
	{
		std::stringstream		amDebug;

		amDebug << "desOff " << NiceV3(velocity * dTime);
		amDebug << ", rLen " << rLen << " rYaw " << rYaw << " rAux1 " << rAux1;
		amDebug << " -> ";
		if (i == pActions_.end())
		{
			amDebug << "<None>";
		}
		else if (model.actionQueue().lastMatchAccepted())
		{
			amDebug << (*i)->pSource_->name_ << " @ " << inTime;
		}
		else
		{
			amDebug << "<Overridden>";
		}
		amDebug << std::ends;
		model.setDebugString( "AM", amDebug.str() );
	}

	// dwMatchEnd.stop();
	dwMatch.stop();
}

// this function (above) gets called from tick, and tick only gets called
// when we're in the world, so its OK to add actions to the action queue.
// (and since we're not using callbacks it's ok anyway)



/**
 *	This function collides the entity that we control with other
 *	entities around it.
 *
 *	@param newEntityPos		Entity position on input and output
 */
void ActionMatcher::doEntityCollisions( Vector3 & newEntityPos )
{
	BW_GUARD;
	Entity * pEntity = &*pEntity_;
	if (!pEntity) return;

	static DogWatch dwCollisions("Collide");
	dwCollisions.start();

	// TODO: Our neighbours should be attached ActionMatchers, not entities
	Entity::Neighbour iter	= pEntity->begin( 3.0f );
	Entity::Neighbour end	= pEntity->end( 3.0f );
	for ( ; iter != end; iter++ )
	{
		// Check that the Entity is not a cached Entity.
		if ( iter->enterCount() <= 0 ) continue;

		PyModel * pModel = iter->pPrimaryModel();
		if (pModel == NULL) continue;

		Motor * pMotor = pModel->motorZero();
		if (!pMotor || !ActionMatcher::Check( pMotor )) continue;

		ActionMatcher * pAM = (ActionMatcher*)pMotor;

		// For each other entity, check that it does not collide with it.
		if ( pAM->entityCollision_ )
		{
			// 1. If they are too far from the camera, don't bother with
			//	  collision detection either, since the player will not
			//	  be able to see it.
			// JWD: Actually, we can't do this due to action matching methinks

			// 2. Check if they are close enough to affect each other
			//	  in the y height.
			float otherHeight = iter->position().y;
			float newHeight = newEntityPos.y;

			// TODO: The magic eight ball says 1.8 metres is a good height.
			//		The magic eight ball is never to be trusted. Fix this.
			const float entityHeight = 1.8f;
			if ( ( otherHeight > newHeight + entityHeight ) ||
				( newHeight > otherHeight + entityHeight ) )
			{
				// The two Entities are not vertically spaced to
				// affect each other. Move on to the next Entity.
				continue;
			}

			// 3. Check if they are close enough in the x-z plane.

			// Limit the size of things to a diametre of 10 metres.
			const double maxDiametre = 10.0;
			double xDist = fabs( iter->position().x - newEntityPos.x );
			if ( xDist >= maxDiametre )
			{
				continue;
			}
			double zDist = fabs( iter->position().z - newEntityPos.z );
			if ( zDist >= maxDiametre )
			{
				continue;
			}

			// Set up the vectors in 2D for cylinders.
			Vector2 otherPos( iter->position().x, iter->position().z );
			Vector2 newPos( newEntityPos.x, newEntityPos.z );
			Vector2 lookAt = otherPos - newPos;

			// TODO: The magic eight ball says 0.6 metre is the answer.
			//		The magic eight ball is never to be trusted. Fix this.
			const float bubbleRadius = 0.6f;

			// 4. Apply the 'electro-static repulsion cylindrical' bubble
			// to the current Entity.
			float distSQ = lookAt.lengthSquared();
			if ( distSQ < bubbleRadius * bubbleRadius )
			{
				if ( distSQ == 0.0f )
				{
					lookAt = Vector2( 0.0f,
						(iter->id() < pEntity->id()) ? 1.0f : -1.0f );
				}
				else
				{
					lookAt.normalise();
				}

				if ( pAM->collisionRooted_ )
				{
					lookAt *= ( bubbleRadius - sqrtf( distSQ ) );
				}
				else
				{
					lookAt *= ( bubbleRadius - sqrtf( distSQ ) ) / 2;
				}
				Vector3 lookAtIn3D( lookAt.x, 0.0f, lookAt.y );
				newEntityPos -= lookAtIn3D ;
			}
		}
	}

	dwCollisions.stop();
}


/**
 *	Get the string name of the last action matched, or None if no match
 */
PyObject * ActionMatcher::pyGet_lastMatch()
{
	BW_GUARD;
	if (pOwner_ == NULL)
	{
		PyErr_SetString( PyExc_ValueError, "ActionMatcher.lastMatch: "
			"ActionMatcher is not attached to a Model" );
		return NULL;
	}

	if (lastAction_ == pActions_.end())
	{
		Py_Return;
	}

	return Script::getData( (*lastAction_)->pSource_->name_ );
}

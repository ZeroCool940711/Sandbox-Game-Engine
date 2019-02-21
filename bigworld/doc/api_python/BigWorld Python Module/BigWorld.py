


class ActionMatcher(Motor.PyObjectPlus):
    """ActionMatcher is a Motor that matches the movements and
    orientation of an entity to the actions defined on its primary model. 

    The ActionMatcher takes account of such information as: 
    
    - the speed of the Entity 
    
    - the angle between the Model and the Entity 
    
    - the angle between the Model and the Entity's velocity 
    
    - user-defined state flags 
    
    It also reads configuration information from Actions in the .model file,
    in order to work out which action to play and how fast to play it. The ActionMatcher
    starts at the top, and checks each action in order, until it finds one that matches
    the current criteria. The relevent parts of the action definition are within the <match> section. 
    
    - <trigger> contains the criteria for picking a particular action. For an action
    to be chosen, all of the following must be true: 
    
    - The speed (magnitude of the velocity) of the Enitity is betweeen <minEntitySpeed>
    and <maxEntitySpeed>. If <minEntitySpeed> is less than zero, then the engine assumes
    the entity is falling when it plays this animation. Note that minEntitySpeed should
    always be closer to zero, regardless of sign, eg, min = -2, max = -7, min = 2, max = 7 
    
    - The angle (entityYaw - modelYaw)is between <minEntityAux1> and <maxEntityAux1>. 
    
    - The angle (entityVelocity - modelYaw) is between <minEntityYaw> and <maxEntityYaw>.
    To match a model walking backward, give it a a yaw range of between, 90 and 270. 
    
    - The ActionMatcher must have all of the caps specified in <capsOn> switched on. These 
    caps are bitpositions in a bitfield, so range between 0 and 31. The caps have no BigWorld
    specific meanings, so it is up to the game to assign meaning to specific numbers. 
    
    - The ActionMatcher must have all of the caps specified in <capsOff> switched off. 
    
    - <scalePlaybackSpeed>, if set to true, specifies that the rate the animation is played
    at will be scaled to match the movement rate. If, for example, the animation was designed
    in 3dsmax to be played at 30 frames a second and move at 2 m/s. If the entity is moving at 3.5 m/s,
    and the valid range for this particular animation is 1 m/s to 5 m/s then the animation will play
    3.5/2.0 = 1.75 times faster than the animator built it to play. 
    
    - <feetFollowDirection>, if set to true, causes the model to turn to track the DirectionCursor 
    
    By default, an ActionMatcher motor is automatically created for a primary Model. To create
    additional ActionMatchers, use BigWorld.ActionMatcher function. """
    
    
    actions = None              #Read-Only Tuple  
    bodyTwistSpeed  = None      #Float  
    boredNotifier = None   #Function object  
    collisionRooted = None  #Integer  
    debug  = None  
    debugWorldVelocity = None
    entityCollision = None  #Integer  
    fallNotifier = None  #Function object  
    fallSelected = None  #Integer  
    footTwistSpeed = None  #Float  
    fuse = None  #Float  
    inheritOnRecouple = None  #Integer  
    lastMatch = None  #Read-Only String  
    matchCaps = None   
    matchModelScale = None  #Integer  
    matchNotifier = None  #Function object  
    matcherCoupled = None  #Integer  
    maxCollisionLod = None  #Float  
    patience = None  #Float  
    startMovingNotifier = None  #Function object  
    turnModelToEntity = None  #Integer  
    useEntityModel = None  #Boolean  
    useEntityPitchAndRoll = None  #Read-Write Boolean.  
    velocityProvider = None   
    
    """actions 
    This attribute contains the name of all matchable actions in use by the action matcher 
    Type: Read-Only Tuple  
    
    
    bodyTwistSpeed 
    The angular speed at which the model will turn to line up with the
    entity in radians per second. Defaults to 2pi. 
    Type: Float  
    
    
    boredNotifier 
    The boredNotifer is the callback function to call once fuse exceeds patience. 
    As an animation plays, fuse will increase. When a new animation is played, fuse 
    will be reset. The patience parameter is set by script. If 0, the boredNotifier 
    will not be triggered. The callback can be defined as follows: 

    
    def onBored( self, actionName )


    
    The name of the action is that described in the .model file under /. It
    can be set in the Action panel of ModelViewer. 
    Type: Function object  
    
    
    collisionRooted 
    This attribute controls how the model responds to entity to entity collisions.
    The entityCollision attribute controls whether the entity will contribute towards
    collisions at all, collisionRooted controls whether this model will move in response
    to collisions. This model will move directly away from the entity it has collided with,
    until its bounding box no longer overlaps that of the model belonging to the
    collided entity. Note that it is the model that moves, rather than the entity,
    making this a client side effect only. 
    
    If this is non-zero, then it will NOT move in response to collisions, otherwise it will. 
    Type: Integer  
    
    
    debug 
    The debug attribute allows you to view internal action matcher variables that are 
    contributing towards matching, including information about the currently matched action, if any. 
    debugWorldVelocity 
    The debug attribute exposes the internal world velocity that was calculated and 
    used by the action matcher to select actions. 
    entityCollision 
    Specifies whether the model responds to collisions between entities. If this is true
    then when the parent entity collides with another entity, then one model, or the other,
    or both, will be moved away from other model so that they dont overlap. Which model 
    moves is determined by the collisionRooted flag on the ActionMatcher on both models. 
    If collisionRooted is set on either model, then that model will NOT move away from the
    entity, leaving the onus for movement on the other model. If both have it set, then
    neither will move. If it is clear on both of them then they will both move half as
    far. It defaults to 0. Note that it is the model that moves, rather than than the 
    entity, making this ta client side effect only. 
    Type: Integer  
    
    
    fallNotifier 
    The callback function to call when the entity transitions between falling and not-falling.
    On callback the function will receive one argument, falling, which is 1 if the entity has
    started to fall, and 0 if it has just stopped falling. The ActionMatcher decides if the entity
    is falling based on the action that it chooses. If the minEntitySpeed of the trigger is less 
    than zero, then it assumes that the entity is falling. This will be the case when the falling
    checkbox is selected in the model editor for an action. 
    Type: Function object  
    
    
    fallSelected 
    This attribute is set to true if the ActionMatcher chose a falling action in its most
    recent choice. The ActionMatcher decides if the entity is falling based on the action
    that it chooses. If the minEntitySpeed of the trigger is less than zero,
    then it assumes that the entity is falling. 
    Type: Integer  
    
    
    footTwistSpeed 
    The angular speed at which the model can twist to match its unitRotation
    in radians per second. Defaults to 3pi. 
    Type: Float  
    
    
    fuse 
    Fuse is the attribute that tracks how long (in seconds) a model has been
    bored (playing the same action). Once it exceeds patience, the boredNotifier
    callback is called. This works for all animations, not just idle animations.
    It is up to the callback function to process different types of animation. 
    Type: Float  
    
    
    inheritOnRecouple 
    This attribute is used for client controlling entities to smooth their motion
    when the ActionMatcher is switched on after being off (which is done by setting
    matcherCoupled to 1). If inheritOnRecouple is true, then the Entities position
    is set to the models position, otherwise the model is moved immediately to the
    entities position, which can cause a visual jar. It defaults to 1. 
    Type: Integer  
    
    
    lastMatch 
    This attribute is set to the name of the last action chosen by the action matcher. 
    Type: Read-Only String  
    
    
    matchCaps 
    This attribute specifies which user defined capabilities the ActionMatcher
    is matching at the moment. It is a list of numbers between 0 and 31. It is
    checked against the <capsOn> and <capsOff> fields (in the .model file definition
    of each action) for each action, and the ActionMatcher will only match those
    actions for which all caps in the <capsOn >field are on in matchCaps and all 
    those in <capsOff> are NOT on. 
    
    Read-Write List of Integers (0-31) 
    matchModelScale 
    This attribute controls whether the model's scale is taken into account when
    choosing an action. Note: Only the Z scale is used as in the following entity.model.scale[2] 
    Type: Integer  
    
    
    matchNotifier 
    The callback function to call when the entity transitions between one action-matched
    action and another. On callback the function will receive one argument, the name of the new action. 
    Type: Function object  
    
    
    matcherCoupled 
    This attribute switches the ActionMatcher on and off. If the attribute is false (0),
    then the ActionMatcher is decoupled, otherwise, it is coupled. It defaults to 1. 
    Type: Integer  
    
    
    maxCollisionLod 
    If the LOD (level of detail) of the model is less than this amount, then the model 
    will be moved to respond to entity-entity collisions. If it is greater than this
    amount, then the model won't respond to collisions. 
    Type: Float  
    
    
    patience 
    Patience is how long the model can continue playing the same action for, before
    it becomes bored. Fuse keeps track of how long the same animation has been playing.
    Once fuse exceeds patience, the boredNotifier callback is called. 
    Type: Float  
    
    
    startMovingNotifier 
    The startMovingNotifier is a callback function to call when the entity changes
    from a stationary to a moving state. This callback's purpose is to provide the
    opportunity to the script programmer to clear the action queue in the even that
    it had been set explicitly in script. The callback can be defined as follows: 
    
    
    def onStartMoving( self, actionName ):
            for actionName in self.model.queue:
                    self.model.action(actionName).stop()
            pass
    
    self.am.startMovingNotifier = onStartMoving



    The name of the action is that described in the .model file under /.
    It can be set in the Action panel of ModelViewer. 
    Type: Function object  
    
    
    turnModelToEntity 
    This attribute controls whether or not the ActionMatcher turns the model
    to track direction changes in the Entity. If it is true, then the models
    yaw will be set to the Entities yaw. If it is false, then the action
    matcher doesn't control the model's yaw at all. 
    Type: Integer  
    
    
    useEntityModel 
    The useEntityModel attribute makes the action matcher use the entity's model 
    to check for movement and rotation, instead of the action matcher's owner's model. 
    
    This is particularly useful if the action matcher is constructed with and entity
    that is different than the owner of the model that this action matcher is used by. 
    Type: Boolean  
    
    
    useEntityPitchAndRoll 
    This attribute controls whether the model should use the pitch and roll of the entity. 
    Type: Read-Write Boolean.  
    
    
    velocityProvider 
    The velocityProvider attribute allows you to use any Vector4Provider to match 
    against, instead of the entity velocity. This is particularly useful with 
    player physics Vector4Providers, which allow you to remove the effect of collision corrections. 
    """
    
    return

class ActionQueuer (PyObjectPlus):
    """An ActionQueuer is an action that can be queued on a model. 
    It has many read only attributes which are defined in the .model file.
    An ActionQueuer can be obtained by either calling the PyModel.action 
    function, or treating the name of the action as a property of a PyModel.
    The only two methods that actually effect the state of the ActionQueuer
    are ActionQueuer.stop and calling the actionqueuer as a function object,
    which makes it play. 

    Actions play on a track, that is specified in the .model file. Actions that
    are on different tracks play concurrently. Actions that are on the same track 
    either overwrite the current action, or queue up behind it. Actions can be queued
    by creating subsequence ActionQueuer's via an an existing ActionQueuer that
    is either queued or playing. 
    
    This class inherits from PyFashion. 

    Example: 
    model = BigWorld.Model( "objects/models/characters/biped.model" )
    
    # These two lines are equivalent.
    aq = model.action( "Walk" )
    aq = model.Walk
    
    # Calling the aq object executes the action.
    model.Walk()
    
    # This gets an ActionQueuer for the "Run" action that will be queued
    # up to start playing after the "Walk" action completes
    model.Run().Walk()
    
    # Here is an example of queueing up a set of actions by string names
    def queueActions( model, actionNames ):
            aq = model.action( actionNames[0] )()
            for name in actionNames[1:]:
                    aq = aq.action( name )()
    ...
    queueActions( self.model, ["Walk", "Run", "Jump"] )

    """
    
    def Get_seek( self ): 
        """This attribute returns the seek position for another entity wishing
        to co-ordinate using this action's counterpart on its own model with this action on this model. 
        Returns: A Vector4 which is the position and rotation that the corresponding action should be played at.  """
        return
    
    
    def Get_seekInv( self ): 
        """This attribute returns the seek position for another entity wishing
        to co-ordinate using this action on its own model with this action's counterpart on this model (got that?). 
        Returns: A Vector4 which is the position and rotation that this action should be played at.  
        """
        return
    
    def __call__( self, afterTime, callBack, promoteMotion ): 
        """By calling the ActionQueuer itself as a function, you initiate the action that the ActionQueuer refers to. 
        
        For example: 
        # Start the walk action immediatly.
        self.model.Walk()
        # Start the climb action after a short delay, call onClimbFinished on
        # completion, and apply animation transforms to the model transform.
        self.model.Climb(0.5, self.onClimbFinished, 1)
        
        Parameters: afterTime  A float which is the time in seconds to
        delay before starting to play the action. Default is 0.  
        callBack  A script functor that will be called when the action finishes playing. Default is None.  
        promoteMotion  An integer specifying a boolean value (0 or 1) that
        determines whether the animation transforms are applied to the model's transform. Default is 0 (false).  
        
        
        Returns: The ActionQueuer object on success, None on failure.  """
        return
    
    
    def action( self, aname, errIfNotFound ): 
        """This function returns the specified action as an ActionQueuer object.
        If an action is found that matches the action name parameter then it is added into the ActionQueue. 
        
        For example: 
        # Start the walk action.
        self.model.action("Walk")()
        
        Parameters: aname  The name of the action to return as an ActionQueuer object.  
        errIfNotFound  An optional bool value specifying whether to return
        an error, if no action was found. Default value is True.  
        
        
        Returns: Returns an ActionQueuer object of the specified action.  """
        return
    
    
    def getSeek( self, posYaw ): 
        """This method returns the seek position and yaw (in world space )
        for another entity wishing to co-ordinate using this action on the 
        provided position and yaw with this action's counterpart on this model. 
        
        This method is different from the seek attribute. The seek attribute 
        specifies the position relative to this model. This function returns
        the position relative to a specified postition. 
        Parameters: posYaw  A Vector4 which is the position and yaw that this action will be played at  
        
        
        Returns: A Vector4 which is the position and rotation that the corresponding action should be played at.  
        """
        return
    
    def getSeekInv( self, posYaw ): 
        """This method returns the seek position and yaw for another entity
        wishing to co-ordinate using this action's counterpart on
        this position and yaw with this action on its own model. 
        
        This method is different from the seekInv attribute. The seekInv
        attribute specifies the position relative to this model. This
        function returns the position relative to a specified postition. 
        Parameters: posYaw  A Vector4 which is the position and 
        yaw that the corresponding action will be played at  
        
        
        Returns: A Vector4 which is the position and rotation that this action should be played at.  
        """
        return
    
    def stop( self ): 
        """This function stops all instances of the input action on
        the queue, by starting to blend them out (excepting matched actions) 
        """
    return


class ArmTrackerNodeInfo (BaseNodeInfo.PyObjectPlus):
    """This is a subclass of BaseNodeInfo. This describes a two bone arm 
    system used to either reach a specific direction or point in a 
    specific direction. The values are all provided to the factory 
    method used to create this (BigWorld.ArmTrackerNodeInfo). 

    Refer to the Tracker class documentation for an overview of the
    tracker system and related code samples. """
    
    maxElbowYaw = None  #float  
    maxShoulderPitch = None  #float  
    maxShoulderRoll = None  #float  
    maxShoulderYaw = None  #float  
    minElbowYaw = None  #float  
    minShoulderPitch = None  #float  
    minShoulderRoll = None  #float  
    minShoulderYaw = None  #float  
    pointingAxis = None  #integer  
    
    """maxElbowYaw 
    Describes the maximum yaw variation in the positive direction (to the right) that a tracker can apply, in degrees. 
    Type: float  
    
    
    maxShoulderPitch 
    Describes the maximum pitch variation in the positive direction (upwards) that a tracker can apply, in degrees. 
    Type: float  
    
    
    maxShoulderRoll 
    Describes the maximum roll variation in the positive direction (anticlockwise) that a tracker can apply, in degrees. 
    Type: float  
    
    
    maxShoulderYaw 
    Describes the maximum yaw variation in the positive direction (to the right) that a tracker can apply, in degrees. 
    Type: float  
    
    
    minElbowYaw 
    Describes the maximum yaw variation in the negative direction (to the left) that a tracker can apply, in degrees. 
    Type: float  
    
    
    minShoulderPitch 
    Describing the maximum pitch variation in the negative direction (downwards) that a tracker can apply, in degrees. 
    Type: float  
    
    
    minShoulderRoll 
    Describes the maximum roll variation in the negative direction (clockwise) that a tracker can apply, in degrees. 
    Type: float  
    
    
    minShoulderYaw 
    Describes the maximum yaw variation in the negative direction (to the left) that a tracker can apply, in degrees. 
    Type: float  
    
    
    pointingAxis 
    Specifies the pointing axis of node (Xaxis, Yaxis or Zaxis). 
    Type: integer  
    
    
    """
    
    return



class AutoAim (PyObjectPlus):
    """AutoAim is made up of essentially two components. The first is friction
    and defines to what degree a Camera will 'stick' to its target and the
    second is adhesion, which describes to what degree the target reticle
    (eg, crosshairs, target box, etc) will follow the target. Each has a field
    of view where the friction and adhesion will be the maximum specified and
    a falloff field of view, where the values linearly blend to 0. 

    The object is contained within the BigWorld.Targeting object. Defaults
    for any number of values can be provided in engine_config.xml under <targeting>/<autoAim>, eg; 
    
    
    <root>
            ...
            <targeting>
                    ...
                    <autoAim>
                            ...
                            <friction> .5 </friction>
                            ...
                    </autoAim>
                    ...
            </targeting>
            ...
    </root>
    
    AutoAim Object can be accessed through BigWorld.autoAim function.
    Note that this is only valid for hand-held controllers...
    """
    adhesionDistance = None  #float  
    adhesionFalloffDistance = None  #float  
    adhesionHorizontalAngle = None  #float  
    adhesionHorizontalFalloffAngle = None  #float  
    adhesionPitchToYawRatio = None  #float  
    adhesionVerticalAngle = None  #float  
    adhesionVerticalFalloffAngle = None  #float  
    forwardAdhesion = None  #float  
    friction = None  #float  
    frictionDistance = None  #float  
    frictionFalloffDistance = None  #float  
    frictionHorizontalAngle = None  #int  
    frictionHorizontalFalloffAngle = None  #float  
    frictionMinimumDistance = None  #float  
    frictionVerticalAngle = None  #float  
    frictionVerticalFalloffAngle = None  #float  
    reverseAdhesionStyle = None  #int  
    strafeAdhesion = None  #float  
    turnAdhesion = None  #float  
    
    """
    adhesionDistance
    
    
    This attribute specifies the distance within which the adhesion
    will be the maximum specified for the current movement.
    
    
    
    Type:
    float  
    
    
    
    adhesionFalloffDistance
    
    
    This attribute specifies the distance outside which the adhesion will be 0.
    The adhesion will linearly deplete from adhesionDistance to adhesionFalloffDistance.
    
    
    
    Type:
    float  
    
    
    
    adhesionHorizontalAngle
    
    
    This attribute specifies the left and right field of view angles, in radians, within which the adhesion
    will be the maximum specified for the current movement.
    
    
    
    Type:
    float  
    
    
    
    adhesionHorizontalFalloffAngle
    
    
    This attribute specifies the left and right field of view angles, in radians, outside which the adhesion
    will be 0.  The adhesion will linearly deplete from adhesionHorizontalAngle to adhesionHorizontalFalloffAngle.
    
    
    
    Type:
    float  
    
    
    
    adhesionPitchToYawRatio
    
    
    This value is used in calculations to handle pitch and yaw.  Defaults to 0.5.
    
    
    
    Type:
    float  
    
    
    
    adhesionVerticalAngle
    
    
    This attribute specifies the upper and lower field of view angles, in radians, within which the adhesion
    will be the maximum specified for the current movement.
    
    
    
    Type:
    float  
    
    
    
    adhesionVerticalFalloffAngle
    
    
    This attribute specifies the upper and lower field of view angles, in radians, outside which the adhesion
    will be 0.  The adhesion will linearly deplete from adhesionVerticalAngle to adhesionVerticalFalloffAngle.
    
    
    
    Type:
    float  
    
    
    
    forwardAdhesion
    
    
    This describes to what degree the target reticle will 'stick' to a target as the targeter moves forward.
    The value is between 0.0 and 1.0. 0 would mean the target reticle is not affected, whereas 1 would cause
    the target reticle to follow the target until it slides out of view.  Defaults to 0.05.
    
    
    
    Type:
    float  
    
    
    
    friction
    
    
    This describes to what degree the Camera will 'stick' to a target.  It is on a scale from 0.0 to 1.0.
    A value of 0.0 would mean there was no friction at all, so the camera would move freely when a target
    is present and a value of 1.0 would mean that the camera would lock onto the target until it reaches the
    friction falloff angle/distance.  Defaults to 0.5.
    
    
    
    Type:
    float  
    
    
    
    frictionDistance
    
    
    This attribute specifies the distance within which the friction will be maximum specified by friction.
    
    
    
    Type:
    float  
    
    
    
    frictionFalloffDistance
    
    
    This attribute specifies the distance outside which the friction will be 0.
    The friction will linearly deplete from frictionDistance to frictionFalloffDistance.
    
    
    
    Type:
    float  
    
    
    
    frictionHorizontalAngle
    
    
    This attribute specifies the left and right field of view angles, in radians, within which the friction
    will be the maximum specified by friction.
    
    
    
    Type:
    int  
    
    
    
    frictionHorizontalFalloffAngle
    
    
    This attribute specifies the left and right field of view angles, in radians, outside which the friction
    will be 0.  The friction will linearly deplete from frictionHorizontalAngle to frictionHorizontalFalloffAngle.
    
    
    
    Type:
    float  
    
    
    
    frictionMinimumDistance
    
    
    This attribute specifies the distance before friction will come into effect,
    ie, if the target is too close, 0 friction will be used...
    
    
    
    Type:
    float  
    
    
    
    frictionVerticalAngle
    
    
    This attribute specifies the upper and lower field of view angles, in radians, within which the friction
    will be maximum specified by friction.
    
    
    
    Type:
    float  
    
    
    
    frictionVerticalFalloffAngle
    
    
    This attribute specifies the upper and lower field of view angles, in radians, outside which the friction
    will be 0.  The friction will linearly deplete from frictionVerticalAngle to frictionVerticalFalloffAngle.
    
    
    
    Type:
    float  
    
    
    
    reverseAdhesionStyle
    
    
    Adjusts the forwardAdhesion for backwards movement.  The default, -1 will perform the usually desired
    effect, 0 will deactivate it.  Other values will adjust it proportionally.
    
    
    
    Type:
    int  
    
    
    
    strafeAdhesion
    
    
    This describes to what degree the target reticle will 'stick' to a target as the targeter moves sideways.
    The value is between 0.0 and 1.0. 0 would mean the target reticle is not affected, whereas 1 would cause
    the target reticle to follow the target until it slides out of view.  Defaults to 0.2.
    
    
    
    Type:
    float  
    
    
    
    turnAdhesion
    
    
    This describes to what degree the target reticle will 'stick' to a target as the targeter turns.
    The value is between 0.0 and 1.0. 0 would mean the target reticle is not affected, whereas 1 would cause
    the target reticle to follow the target until it slides out of view.  Defaults to 0.1.
    
    
    
    Type:
    float  
    
    """
    
    return

class AvatarDropFilter (AvatarFilter.Filter.PyObjectPlus ):
    """This class inherits from AvatarFilter. It is nearly exactly
    the same as its parent, except that a findDropPoint is used to place each input point on the ground. 

    An AvatarDropFilter is created using the BigWorld.AvatarDropFilter function. """
    alignToGround = None  #bool  
    groundNormal = None  #GroundNormalProvider  
    
    return
class AvatarFilter (Filter.PyObjectPlus ):
    """This class inherits from Filter. It implements a filter which 
    tracks the last 8 entity updates from the server, and linearly interpolates between them. 

    Linear interpolation is done at ( time - latency ), where time is the current
    engine time, and latency is how far in the past the entity currently is. 
    
    Latency moves from its current value in seconds to the "ideal latency" which is the
    time between the two most recent updates if an update has just arrived, 
    otherwise it is 2 * minLatency. Lantency moves at velLatency seconds per second. 
    
    An AvatarFilter is created using the BigWorld.AvatarFilter function.
    The constructor takes in an optional AvatarFilter to copy for example: 
    
    
    # create an avatar filter
    filter = BigWorld.AvatarFilter()
    
    # create an avatar filter from another avatar filter
    filter = BigWorld.AvatarFilter(oldFilter)
    
    """
    latency = None  #float  
    latencyCurvePower = None  #float  
    latencyFrames =None   #float  
    minLatency = None  #float  
    velLatency = None  #float  
    
    def callback( self, whence, fn, extra, passMissedBy ): 
        """This method adds a callback function to the filter. This function will 
        be called at a time after the event specified by the whence argument. The
        amount of time after the event is specified, in seconds, by the extra argument. 
    
        If whence is -1, then the event is when the filter's timeline reaches the
        penultimate update (i.e. just before the current update position starts
        influencing the entity). If whence is 0, then the event is when the
        current update is reached. If whence is 1 then the event is when the 
        timeline reaches the time that the callback was specified. 
        
        The function will get one argument, which will be zero if the 
        passMissedBy argumenty is zero, otherwise it will be the amount of
        time that passed between the time specified for the callback and the time it was actually called at. 
        
        Note: When the callback is made, the entity position will already have
        been set for the frame (so it'll be at some spot after the desired callback time),
        but the motors will not yet have been run on the model to move it. The positions
        of other entities may or may not yet have been updated for the frame. 
        
        Note: If a callback function returns '1', the entity position will be snapped to
        the position that the entity would have had at the exact time the callback
        should have been called. 
        Parameters: whence  an integer (-1, 0, 1). Which event to base the callback on.  
        fn  a callable object. The function to call. It should take one integer argument.  
        extra  a float. The time after the event specified by whence tha the function should be called at.  
        passMissedBy  whether or not to pass to the function the time it missed its ideal call-time by.  
        """
        return
    
    def debugMatrixes( self ): 
        """This function returns a tuple of matrix providers that when applied to a unit cube visualises the received position data being used by the filter. 
    
        Note: These matrix providers hold references to the filter that issued them. 
        Returns: tuple([MatrixProvider,]) A tuple of newly created matrix providers""" 
        return  
    
    return


# ================================================================================================
# functions for the bigworld module

def ActionMatcher( ): 
    """"This function returns a new ActionMatcher Motor object, which is used to match the movements
    and orientation of an entity to the actions defined on its primary model. Note that the primary
    Model will be assigned an ActionMatcher Motor by default. 
    Returns: new ActionMatcher object  
    """
    return

def ArmTrackerNodeInfo( model, pUpperArmNode, pForeArmNode, minShoulderPitch, maxShoulderPitch,
                        minShoulderYaw, maxShoulderYaw, minShoulderRoll, maxShoulderRoll, minElbowPitch,
                        maxElbowPitch, minPitch, maxPitch, minYaw, maxYaw, pointingAxis, angularVelocity,
                        angularThreshold, angularHalflife ): 
    """Creates a new instance of the ArmTrackerNodeInfo class. This describes how PyModelNode instances
    are affected by a two bone IK Tracker. 
    Parameters: model  The PyModel that owns the nodes which trackers using this will affect.  
    pUpperArmNode  A string containing the name of the upper arm node.  
    pForeArmNode  A string containing the name of the forearm node.  
    minShoulderPitch  A float describing the maximum pitch variation in the negative direction (downwards) that a tracker can apply, in degrees.  
    maxShoulderPitch  A float describing the maximum pitch variation in the positive direction (upwards) that a tracker can apply, in degrees.  
    minShoulderYaw  A float describing the maximum yaw variation in the negative direction (to the left) that a tracker can apply, in degrees.  
    maxShoulderYaw  A float describing the maximum yaw variation in the positive direction (to the right) that a tracker can apply, in degrees.  
    minShoulderRoll  A float describing the maximum roll variation in the negative direction (clockwise) that a tracker can apply, in degrees.  
    maxShoulderRoll  A float describing the maximum roll variation in the positive direction (anticlockwise) that a tracker can apply, in degrees.  
    minElbowPitch  A float describing the maximum pitch variation in the negative direction (downwards) that a tracker can apply, in degrees.  
    maxElbowPitch  A float describing the maximum pitch variation in the positive direction (upwards) that a tracker can apply, in degrees.  
    minPitch  A float describing the maximum pitch variation in the negative direction (downwards) that a tracker can apply, in degrees.  
    maxPitch  A float describing the maximum pitch variation in the positive direction (upwards) that a tracker can apply, in degrees.  
    minYaw  A float describing the maximum yaw variation in the negative direction (to the left) that a tracker can apply, in degrees.  
    maxYaw  A float describing the maximum yaw variation in the positive direction (to the right) that a tracker can apply, in degrees.  
    pointingAxis  An int specifying the pointing axis of node (Xaxis, Yaxis or Zaxis).  
    angularVelocity  A float describing the speed at which a tracker will turn the PyModelNode instances that it influences, in degrees per second.  
    angularThreshold  A float describing the angle to check if a decay or simple interpolation is used to blend the tracker
    node into the correct angle, specified in degrees. For more information check BaseNodeInfo.  
    angularHalflife  A float describing the angular rate of decay to blend the tracker node, specified in seconds.
    For more information check BaseNodeInfo.  

    
    Returns: The new ArmTrackerNodeInfo.  
    """
    return

def AvatarDropFilter( an ): 
    """This function creates a new AvatarDropFilter, which is used to move avatars around the same as an AvatarFilter
    does, but also to keep them on the ground beneath their specified positions. 
    Parameters: an  optional AvatarFilter to initialise the filter with  
    
    
    Returns: a new AvatarDropFilter  """
    return

def AvatarFilter( an ): 
    """"This function creates a new AvatarFilter, which is used to interpolate between the position
    and yaw updates from the server for its owning entity. 
    Parameters: an  optional AvatarFilter to initialise the filter with  
    
    
    Returns: a new AvatarFilter  
    """
    return

def AxisEvent( axis, value, dTime ): 
    """This function creates a new PyAxisEvent object. This can be used to create artificial
    events which did not originate from the engine's input system itself. 
    Parameters: axis  The axis index, as defined in Keys.py  
    value  A float value between -1.0 and +1.0.  
    dTime  A value representing the time since the last time the axis was processed.  
    

    Returns: the newly created axis event.  
    """
    return

def BoidsFilter( ): 
    """This function creates a new BoidsFilter. This is used to filter the movement of an 
    Entity which consists of several models (boids) for which flocking behaviour is desired. 
    Returns: a new BoidsFilter  
    """
    return

def Bouncer( ): 
    """Bouncer is a factory function to create a Bouncer Motor. A Bouncer is a Motor that 
    starts with an initial velocity, and bounces according to the laws of physics, finally 
    coming to rest. This Motor could be used for thrown objects, such as grenades. 
    Returns: A new Bouncer object.  
    """
    return

def BoxAttachment( ): 
    """This method creates a BoxAttachment and returns it. 
    Returns: Returns a new BoxAttachment object.  
    """
    return

def CursorCamera( ): 
    """This function creates a new CursorCamera. A CursorCamera looks at
    a specified target, in the direction of a specified source. 
    Returns: a new CursorCamera object.  
    """
    return

def DiffDirProvider( source, target ): 
    """Creates a DiffDirProvider. This provides a matrix representing the direction 
    between the points specified by two matrices. 
    
    Code Example: 
    # returns a new DiffDirProvider which provides a matrix which points from
    # matrix e1 to matrix e2
    BigWorld.DiffDirProvider( e1.matrix, e2.matrix )

    Parameters: source  The MatrixProvider that gives the position from which the matrix this provides points.  
    target  The MatrixProvider that gives the position towards which the matrix this provides points.  

    
    Returns: The new DiffDirProvider  
    """
    return

def DumbFilter( ): 
    """This function creates a new DumbFilter, which is the simplest filter,
    which just sets the position of the owning entity to the most recently received position from the server. 
    Returns: A new DumbFilter object  
    """
    return

def EntityDirProvider( entity, pitchIndex, yawIndex ): 
    """Create a new EntityDirProvider. This is a MatrixProvider with the direction of a specified entity. 
    
    Code Example: 
    # return a new EntityDirProvider which provides the direction
    # faced by the entity ent.
    BigWorld.EntityDirProvider( ent )
    
    Parameters: entity  The entity whose facing direction this provides.  
    pitchIndex  An integer which describes which of the entity's axis are to be
    interpreted as pitch (0:yaw, 1:pitch, 2:roll).  
    yawIndex  An integer which describes which of the entity's axis are to
    be interpreted as yaw (0:yaw, 1:pitch, 2:roll).  
    
    
    Returns: The new EntityDirProvider.  
    """
    return

def FlexiCam( ): 
    """This function creates a new FlexiCam, which can be used to look at a target from a specified point relative to the target. 
    Returns: a new FlexiCam  
    """
    return

def Font( name ): 
    """Parameters: name  the resource id.  

    
    Returns: A new PyFont object.
    """
    return

def FootTrigger( Boolean, string ): 
    """This function creates a new FootTrigger object. This can be attached to PyModelNodes
    to test for them hitting the ground, and calling a function when they do. 
    Parameters: Boolean  left [False] foot or right [True] foot  
    string  [optional] sound trigger prefix  
    
    
    Returns: a new FootTrigger object.  
    """
    return

def FreeCamera( ): 
    """This function creates a new FreeCamera. This is a camera that is controlled
    by the mouse and arrow keys, and moves around independently of entities and geometry,
    making it useful for debugging and exploring the world. 
    Returns: a new FreeCamera object.  
    """

def GraphicsSetting( String, String, Int, Boolean, Boolean, PyObject ): 
    """This function creates a PyGraphicsSetting. 
    
    For example: 
    def onOptionSelected( idx ):
        print "graphics setting option %d selected" % (idx,)
        
    gs = BigWorld.GraphicsSetting( "label", "description", -1, False, False, onOptionSelected )
    gs.addOption( "High", "High Quality", True )
    gs.addOption( "Low", "Low Quality", True )
    gs.registerSetting()	#always add after the options have been added.


    Parameters: String  Short label to display in the UI.  
    String  Longer description to display in the UI.  
    Int  Active option index. When this setting is added to the registry, the active option 
    will be either reset to the first supported option or to the one restored from the options
    file, if the provided active option index is -1. If, instead, the provided activeOption is
    a non negative value, it will either be reset to the one restored from the options file
    (if this happens to be different from the value passed as parameter) or not be reset at all.  
    Boolean  Delayed - apply immediately on select, or batched up.  
    Boolean  Requires Restart - game needs a restart to take effect.  
    PyObject  Callback function.  
    
    
    Returns: a new PyGraphicsSetting  
    """
    return

def Homer( ): 
    """Homer is a factory function to create a Homer Motor. A Homer is a Motor
    that moves a model towards a target (MatrixProvider). 
    Returns: A new Homer object.  
    """
    return

def InvViewMatrix( ): 
    """This function returns a new InvViewMatrixProvider which is a MatrixProvider
    that can be used to access the rendering engine's inverse view matrix. 
    """
    return
    
def KeyEvent( key, state, modifiers, character, cursorPos ): 
    """This function creates a new PyKeyEvent object. This can be used to create artificial 
    events which did not originate from the engine's input system itself. 
    Parameters: key  The key code.  
    state  The state of the key, where -1 indicates up and >= 0 indicates the repeat
    count (so 0 means the key has just been pressed).  
    modifiers  The state of the modifier keys (ALT, CONTROL, SHIFT).  
    character  A unicode string or None.  
    cursorPos  The clip-space position of the mouse cursor.  
    
    
    Returns: the newly created key event.  
    """
    return
def LatencyInfo( ): 
    """The function LatencyInfo is a factory function to create and return a 
    LatencyInfo object. A LatencyInfo object is a Vector4Provider with an overloaded
    value attribute. This attribute (for a LatencyInfo object) is overloaded to return
    a 4 float sequence consisting of ( 1, minimum latency, maximum latency, average
    latency ) if the client is connected to a server. If the client is not connected
    to the server then the value attribute returns ( 0, 0, 0, 0 ). 
    Returns: A new LatencyInfo object (see also Vector4Provider).  
    """
    return

def LinearHomer( ): 
    """This function returns a new LinearHomer object, which is Motor designed
    to drive a Model towards it's target in a straight line. It has a different
    set of features to the Homer Motor. 
    Returns: A new LinearHomer object  
    """
    return

def Loft( ): 
    """This function creates and returns a new PyLoft object. """
    return

def Material( effectName, diffuseMap, DataSection ): 
    """Note that the parameter list is complicated, so although three parameters
    are listed below, the PyMaterial constructor takes : Either the name of an .fx file,
    (and optionally the name of a diffuse map.) Or a PyDataSection containing a material section. 
    
    There are no listed attributes for a PyMaterial, however on creation, the attribute
    dictionary will be filled with all the "artist editable" variables contained in the effect file. 
    Parameters: effectName  Name of the effect file (*.fx)  
    diffuseMap  Name of the diffuse map for the .fx file  
    DataSection  DataSection for a material section.  
    
    
    Returns: A new PyMaterial object.  
    """
    return

def Model( *modelPath ): 
    """Model is a factory function that creates and returns a new PyModel object
    (None if there are problems loading any of the requested resources). PyModels
    are renderable objects which can be moved around the world, animated, dyed
    (have materials programatically assigned) and contain points to which objects can be attached. 
    
    The parameters for the function are one or more model resources. Each of these
    individual models is loaded, and combined into a "supermodel". 
    
    This function will block the main thread to load models in the event that they
    were not specified as prerequisites. To load models asynchronously from script
    use the method BigWorld.loadResourceListBG(). 
    
    For example, to load a body and a head model into one supermodel: 
    
    
    model = BigWorld.Model( "models/body.model", "models/head.model" )
    
    Parameters: *modelPath  String argument(s) for the model.  
    
    
    Returns: A reference to the PyModel specified.  """
    return
    

def MouseEvent( dx, dy, dz, cursorPos ): 
    """This function creates a new PyMouseEvent object. This can be used to
    create artificial events which did not originate from the engine's input system itself. 
    Parameters: dx  A signed integer representing the delta movement on the X-axis.  
    dy  A signed integer representing the delta movement on the Y-axis.  
    dz  A signed integer representing the mouse wheel.  
    cursorPos  The clip-space position of the mouse cursor.  
    
    
    Returns: the newly created mouse event.  """
    return


def MouseTargettingMatrix( ): 
    """Creates and returns a new MouseTargettingMatrix provider. This matrix provider
    produces a world-space transform (position and direction) from the unprojected
    mouse position. The translation component is positioned on the near plane of the camera. 
    
    While this provider is intended to be used with the BigWorld targeting system,
    you can also use it to produce world position and direction vectors from Python.
    For example, these vectors could be used as parameters to BigWorld.collide: 
    
    
    mtm = Math.Matrix( BigWorld.MouseTargetting() )
    src = mtm.applyToOrigin()
    dir = mtm.applyToAxis(2)"""
    return
    
def Orbitor( ): 
    """Orbitor is a factory function to create a Orbitor Motor. A Orbitor
    is a Motor that moves a model towards a target (MatrixProvider) and orbits. 
    Returns: A new Orbitor object.  
    """
    return

def OrthogonalNodeInfo( model, pitchNode, yawNode, directionNode, minPitch, maxPitch,
                        minYaw, maxYaw, yawAxis, pitchAxis, angularVelocity, angularThreshold,
                        angularHalflife ): 
    """This creates a new instance of the OrthogonalNodeInfo class which describes how
    PyModelNode instances can be affected by a Tracker. Note that the attributes specified 
    by this function are not made accessable by script in the object it creates. Instances
    of this particular NodeInfo class can be used by a Tracker to apply pitch changes to one 
    model node, and yaw changes to another. This can be used very effectively with models which 
    represent machinery that turns on swivel joints. 
    Parameters: model  The PyModel owning the nodes that will be affected by any Tracker
    which makes use of the returned OrthogonalNodeInfo.  
    pitchNode  A string containing the name of the pitch node. This is the node to which
    a Tracker applies pitch changes. If this does not match the name of a node of the model
    specified by the model argument then a ValueError is thrown.  
    yawNode  A string containing the name of the yaw node. This is the node to which yaw changes
    are applied. If this does not match the name of a node in the model specified by the model
    argument then a ValueError is thrown.  
    directionNode  A string containing the name of a node that can be assumed to have undergone
    both the pitch and yaw translations after they've been applied. This should be a descendent 
    of both the pitchNode and the yawNode. If this does not match the name of a node in the model
    specified by the model argument then a ValueError is thrown.  
    minPitch  A float describing the maximum pitch variation (in degrees) in the negative direction
    (downwards) that can be applied by a Tracker.  
    maxPitch  A float describing the maximum pitch variation (in degrees) in the positive direction 
    (upwards) that can be applied by a Tracker.  
    minYaw  A float describing the maximum yaw variation (in degrees) in the negative direction
    (to the left) that can be applied by a Tracker.  
    maxYaw  A float describing the maximum yaw variation (in degrees) in the positive direction
    (to the right) that can be applied by a Tracker.  
    yawAxis  An int describing which axis to rotate along to change yaw 1=x-axis, 2=y-axis, 3=z-axis,
    negative values means invert the axis.  
    pitchAxis  An int describing which axis to rotate along to change pitch 1=x-axis, 2=y-axis,
    3=z-axis, negative values means invert the axis.  
    angularVelocity  A float describing the speed (in degrees per second) that a Tracker will rotate
    any PyModelNode instances that it influences.  
    angularThreshold  A float describing the angle to check if a decay or simple interpolation is used 
    to blend the tracker node into the correct angle, specified in degrees. For more information check
    BaseNodeInfo.  
    angularHalflife  A float describing the angular rate of decay to blend the tracker node, specified
    in seconds. For more information check BaseNodeInfo.  
    
    
    Returns: The new OrthogonalNodeInfo.  """
    return


def Oscillator( yaw, period, amplitude, offset ): 
    """Oscillator is a factory function that creates and returns a new Oscillator object. An Oscillator
    is a Motor that applies an oscillating rotational component to an objects world transform. 
    
    See the Oscillator class for more information on the parameters below. 
    Parameters: yaw  Float. Specifies the initial yaw (in radians) of the rotation.  
    period  Float. Specifies the period (in seconds) of rotation.  
    amplitude  Float. Specifies the maximum angle (in radians) of rotation.  
    offset  Float. Specifies the offset (in seconds) to the start of the period of oscillation.  
    
    
    Returns: A new Oscillator object.  
    """
    return

def Oscillator2( yaw, period, amplitude, offset ): 
    """Oscillator2 is a factory function that creates and returns a new Oscillator2 object. An Oscillator2 extends Oscillator (Motor) and applies an oscillating rotational component around the origin to a model's world transform, instead of rotating it around its own centre. 
    
    See the Oscillator class for more information on the parameters below. 
    Parameters: yaw  Float. Specifies the initial yaw (in radians) of the rotation.  
    period  Float. Specifies the period (in seconds) of rotation.  
    amplitude  Float. Specifies the maximum angle (in radians) of rotation.  
    offset  Float. Specifies the offset to the start of the period of oscillation.  
    
    
    Returns: A new Oscillator2 object.  """
    return
    

def PlayerAvatarFilter( ): 
    """This function creates a new PlayerAvatarFilter. This updates the position and
    yaw of the entity to those specified by the most recent server update. 
    Returns: a new PlayerAvatarFilter  
    """
    return

def PlayerMatrix( ): 
    """This function returns a new PlayerMProv which is a MatrixProvider that can
    be used to access the player entity's position. """
    return

def Propellor( ): 
    """Creates and returns a new Propellor Motor, which is used to move a model 
    by applying a point source of force. """
    return

def PyChunkLight( ): 
    """"Creates a new PyChunkLight. This is a script controlled chunk omni light
    which can be used as a diffuse and/or specular light source for models and shells. """
    return

def PyChunkModel( modelName, spaceID, matrix ): 
    """"NOTE: This function has been deprecated - Use a static PyModelObstacle instead 
    
    Creates an instance of PyChunkModel using a given model, with a given transform. A
    PyChunkModel is a PyObjectPlus wrapper around a simple model which is treated by
    BigWorld as a part of the level geometry. 
    
    Code Example: 
    # Import Math library
    import Math
    
    # Place a PyChunkModel in the world at ( 0, 10, 0 )
    modelName = "objects\models\items\Trees & Plants\oak4.model"
    matrix = Math.Matrix()
    matrix.setTranslate( ( 0, 10, 0 ) )
    newPyChunkModel = BigWorld.PyChunkModel( modelName, $p.spaceID, matrix )
    
    Parameters: modelName  A string containing the name of the model to use.  
    spaceID  The ID of the space in which to place the PyChunkModel.  
    matrix  The MatrixProvider which gives the PyChunkModel's transform.
    Note that the PyChunkModel's transform is static. It does does not dynamically
    change with this MatrixProvider.  
    

    Returns: The new PyChunkModel.  """
    return


def PyChunkSpotLight( ): 
    """Creates a new PyChunkSpotLight. This is a script controlled chunk spot light
    which can be used as a diffuse and/or specular light source for models and shells. """
    return

def PyModelObstacle( modelNames, matrix, dynamic ): 
    """Creates and returns a new PyModelObstacle object, which is used to integrate
    a model into the collision scene. 

    Example: 
    self.model = BigWorld.PyModelObstacle("models/body.model", "models/head.model", self.matrix, 1)
    
    
    
    Once created and assigned (self.model = PyModelObstacle), the PyObstacleModel becomes
    attached, hence its dynamic nature cannot be changed. If left as static, the position
    of the PyObstacleModel will not be updated as the MatrixProvider changes. PyModelObstacles
    are reasonably expensive, hence should only be used when truly required. 
    Parameters: modelNames  Any number of ModelNames can be used to build a SuperModel, which
    is a PyModel made of other PyModels ( see BigWorld.Model() ). Each modelName should be
    a complete .model filename, including resource path.  
    matrix  (optional) The transform Matrix for the resultant PyModelObstacle, used to
    update (if dynamic) or set (if static) the SuperModel's position.  
    dynamic  (optional) If true, PyModelObstacle will be treated as dynamic, otherwise
    static. Defaults to false (static).  """
    return
    

def PyModelRenderer( width, height ): 
    """This function creates a new PyModelRenderer, which can be used to render
    models into a PyTextureProvider which can supply a texture to other things,
    such as a SimpleGUIComponent, which need a texture to render. 
    Parameters: width  the width of the texture to render to  
    height  the height of the texture to render to  
    """
    return

def PyMorphControl( ): 
    """This function creates a new PyMorphControl. This can be applied to a model,
    and used to blend between a specified list of morph targets using weights
    supplied in a MatrixProvider. 
    Returns: a new PyMorphControl  
    """
    return

def PySceneRenderer( width, height ): 
    """This function creates a new PySceneRenderer. The function takes two arguments,
    which are the width and height of the texture to render to. PySceneRenderers render
    one or more cameras to a texture, which can then be applied to anything which
    can use a PyTextureProvider. 
    Parameters: width  an Integer. The width of the texture to render to (in Pixels).  
    height  an Integer. The height of the texture to render to (in Pixels).  
    
    
    Returns: a new PySceneRenderer.  """
    return
    

def PyShimmerCountProvider( ): 
    """This function creates a PyShimmerCountProvider. It records the number of
    shimmer objects currently visible. It can be used to optimise Post-Processing
    effects that use the shimmer channel (alpha of the back buffer). 
Returns: a new PyShimmerCountProvider.  """


def PyTextureProvider( resourceId ): 
    """This function creates a PyTextureProvider. It takes a texture filename
    as an argument. A PyTextureProvider is used to supply a texture to classes
    which need a texture, such as SimpleGUIComponent. 

    For example: 
    tex = BigWorld.PyTextureProvider( "maps/fx/picture.bmp" )
    
    In the example, tex is set to be a PyTextureProvider which exposes the picture.bmp texture. 
    Parameters: resourceId  the filename of the texture to load.  
    

    Returns: a new PyTextureProvider.  
    
    """
    return

def PyWaterVolume( ): 
    """Factory function to create and return a new PyWaterVolume object. 
    Returns: A new PyWaterVolume object.  
    """
    return

def RenderTarget( name, width, height, reuseZ, format ): 
    """Note that width and height can be specified either in absolute pixels,
    or they can represents multipliers of the screen size. These dimensions
    automatically adjust when the screen size changes. 

    w,h = 0 : use the screen size w,h = -1 : screen size / 2 w,h = -2 : screen
    size / 4 w,h = -n : screen size / pow(2,n) 
    Parameters: name  Name of the render target  
    width  Desired Width. 0, ... -n have special meaning (see above)  
    height  Desired Height. 0, ... -n have special meaning (see above)  
    reuseZ  (optional)Re-use the back-buffer's Z-buffer, or False to
    create a unique z-buffer for this render target.  
    format  (optional)D3DFORMAT enum value.  
    
    
    Returns: A new PyRenderTarget object.  
    """
    return

def ScanDirProvider( amplitude, period, offset, callback ): 
    """Creates a new ScanDirProvider. This is a MatrixProvider whose direction
    oscillates around the y axis, and may be instructed to call a function
    when it hits it's leftmost and rightmost limits. 
    
    Code Example: 
    # return a new ScanDirProvider which swings left and right by pi/2 radians
    # every 10 seconds, offset by 0 percent of its period from time 0. It calls
    # limitFunct( 4 ) when it hits it's leftmost limit, and limitFunct( 5 )
    # when it hits it's rightmost limit.
    BigWorld.ScanDirProvider( math.pi/2, 10, 0, limitFunct )
    
    Parameters: amplitude  A float representing the angle about 0 to which
    this should swing around the y axis, in radians.  
    period  A float giving the time in seconds for a full sweep left & right.  
    offset  An optional float giving the offset of the sweep cycle from time 0,
    as a proportion of the period.  
    callback  An optional function with a single argument. This function is 
    called with value 4 when this hits it's leftmost limit, and 5 when it
    hits it's rightmost limit.  
    

    Returns: The new ScanDirProvider  
    """
    return

def Servo( signal ): 
    """Creates and returns a new Servo Motor, which sets the transform of a model
    to the given MatrixProvider. As the MatrixProvider is updated, the model will move. 
    Parameters: signal  MatrixProvider that model will follow  
    """
    return

def Splodge( ): 
    """Creates a new PySplodge object which is ready to be attached to a node on
    a model. This draws a "splodge" shadow on the ground below the point where
    it is attached. 
    Returns: A new PySplodge.  
    """
    return

def ThirdPersonTargettingMatrix( source ): 
    """Creates and returns a new ThirdPersonTargettingMatrix provider. This matrix
    provider implements a special-case matrix designed for targetting while using 
    a 3rd person camera. It takes camera matrix and moves it forward along its z-axis
    until it lines up with the position given by the user-specified source matrix
    (for example, the player entity matrix). 
    Parameters: source  MatrixProvider to help calculate the look from matrix.  
    """
    return

def Tracker( ): 
    """Creates a new instance of the Tracker class. This is a subclass of PyFashion
    which manipulates yaw and pitch of the PyModelNode instances in a PyModel so
    as to make them face towards (or "track") a target. 
    Returns: The new Tracker object.  
    """
    return

def TrackerNodeInfo( model, primaryNodeName, secondaryNodesList, pointingNodeName,
                     minPitch, maxPitch, minYaw, maxYaw, angularVelocity,
                     angularThreshold, angularHalflife ): 
    """Creates a new instance of the TrackerNodeInfo class. This describes how PyModelNode
    instances are affected by a Tracker. Note that the attributes specified by this
    function are not made accessable by script in the object it creates. 
    Parameters: model  The PyModel that owns the nodes which trackers using this will affect.  
    primaryNodeName  A string containing the name of the primary node. The direction change
    applied to the primary node is not weighted.  
    secondaryNodesList  A list of tuples of the form ( nodeName, weight ), where nodeName is
    a string containing the name of a node to be affected by a tracker, and weight is a float 
    describing the influence a tracker has over it.  
    pointingNodeName  A string containing the name of a node provides the frame of reference
    used for tracker manipulations. If the node specified is not present, the model's scene root is used instead.  
    minPitch  A float describing the maximum pitch variation in the negative direction
    (upwards) that a tracker can apply, in degrees.  
    maxPitch  A float describing the maximum pitch variation in the positive direction
    (downwards) that a tracker can apply, in degrees.  
    minYaw  A float describing the maximum yaw variation in the negative direction (to the left)
    that a tracker can apply, in degrees.  
    maxYaw  A float describing the maximum yaw variation in the positive direction (to the right)
    that a tracker can apply, in degrees.  
    angularVelocity  A float describing the speed at which a tracker will turn the PyModelNode
    instances that it influences, in degrees per second.  
    angularThreshold  (optional) A float describing the angle to check if a decay or simple
    interpolation is used to blend the tracker node into the correct angle, specified in degrees.
    For more information check BaseNodeInfo. Defaults to -1.  
    angularHalflife  (optional) A float describing the angular rate of decay to blend the tracker 
    node, specified in seconds. For more information check BaseNodeInfo. Defaults to 0.  
    

    Returns: The new TrackerNodeInfo.  
    """
    return

def VOIP.command( data ): 
    """This is a generic function for handling implementation specific commands
    not exposed in the general VoIP interface. 
    Parameters: data  (dictionary) Additional information in the form of string value pairs  
    """
    return

def VOIP.createChannel( channelName, password, data ): 
    """Creates a new channel with the given name. The creating user automatically joins the
    newly created channel. The channel will be destroyed when all users leave the channel.
    Default password is "". 
    Parameters: channelName  (string) The string that uniquely identifies the new channel  
    password  (string) Optional: The password that will be needed to access the channel  
    data  (dictionary) Optional: Additional information in the form of string value pairs  
    
    
    Returns: (none)  
    """
    return

def VOIP.disableActiveChannel( channelName, data ): 
    """Disables voice transmission on the specified channel. 
    Parameters: channelName  (string) The string identifying the channel  
    data  (dictionary) Optional: Additional information in the form of string value pairs  
    
    
    Returns: (none)  
    """
    return

def VOIP.disableMicrophone( data ): 
    """Disables microphone input. 
    Parameters: data  (dictionary) Optional: Additional information in the form of string value pairs  
    
    
    Returns: (none)  
    """
    return

def VOIP.disablePositional( data ): 
    """Disables positional audio filtering 
    Parameters: data  (dictionary) Optional: Additional information in the form of string value pairs  
    

    Returns: (none)  
    """
    return

def VOIP.enableActiveChannel( channelName, data ): 
    """Enables voice transmission on the specified channel. 
    Parameters: channelName  (string) The string identifying the channel  
    data  (dictionary) Optional: Additional information in the form of string value pairs  
    
    
    Returns: (none)  
    """
    return

def VOIP.enableMicrophone( data ): 
    """Enables microphone input. 
    Parameters: data  (dictionary) Optional: Additional information in the form of string value pairs  


    Returns: (none)  
    """
    return

def VOIP.enablePositional( data ): 
    """"Enables positional audio filtering 
    Parameters: data  (dictionary) Optional: Additional information in the form of string value pairs  
    

    Returns: (none)  
    """
    return

def VOIP.finalise( data ): 
    """This function instructs the VoIP client to clean up all its connections and resources. 
    Parameters: data  (dictionary) Optional: Additional information in the form of string value pairs  
    
    
    Returns: (none)  
    """
    return

def VOIP.getStatistics( ): 
    """Requests an implementation specific a statistics report that will be sent back to
    the application via the response callback mechanism. """
    return

def VOIP.initialise( data ): 
    """This function initialises the VoIP client. 
    Parameters: data  (dictionary) Optional: Additional information in the form of string value pairs  
    
    
    Returns: (none)  
    """
    return

def VOIP.inviteUser( userName, channelName, data ): 
    """Sends an invite to another VoIP user to join a channel. 
    Parameters: userName  (string) The string identifying the user's VoIP account  
    channelName  (string) The string identifying the channel  
    data  (dictionary) Optional: Additional information in the form of string value pairs  
    
    
    Returns: (none)  
    """
    return

def VOIP.joinChannel( channelName, password, data ): 
    """Joins an existing channel. Default password is "". 
    Parameters: channelName  (string) The string identifying the channel  
    password  (string) Optional: The password that will be needed to access the channel  
    data  (dictionary) Optional: Additional information in the form of string value pairs  
    
    
    Returns: (none)  
    """
    return

def VOIP.kickUser( userName, channelName, data ): 
    """Kicks a user from a channel 
    Parameters: userName  (string) The string identifying the user's VoIP account  
    channelName  (string) The string identifying the channel  
    data  (dictionary) Optional: Additional information in the form of string value pairs  
    
    
    Returns: (none)  
    """
    return

def VOIP.leaveChannel( channelName, data ): 
    """Leaves a channel that the client is already connected to. 
    Parameters: channelName  (string) The string identifying the channel  
    data  (dictionary) Optional: Additional information in the form of string value pairs  
    

    Returns: (none)  
    """
    return

def VOIP.login( username, password, data ): 
    """"This function logs the client into the VoIP service. 
    Parameters: username  (string) The string that uniquely identifies the user's account  
    password  (string) The corresponding password to access the account  
    data  (dictionary) Optional: Additional information in the form of string value pairs  
    
    
    Returns: (none)  
    """
    return

def VOIP.logout( data ): 
    """"This function logs the client out of the VoIP service. 
    Parameters: data  (dictionary) Optional: Additional information in the form of string value pairs  
    
    
    Returns: (none)  
    """
    return

def VOIP.queryChannels( ): 
    """Returns the list of channels the client is connected to. 
    Returns: [(string),...] The list of channel names  
    """
    return

def VOIP.setChannelVolume( attenuation, data ): 
    """Sets the volume at which voice communications on the given channel play. This
    attenuation is in addition to that applied by the master volume. 
    Parameters: attenuation  (float) The new speaker volume attenuation in decibels(dB)  
    data  (dictionary) Optional: Additional information in the form of string value pairs  
    
    
    Returns: (none)  
    """
    return

def VOIP.setHandler( callback ): 
    """Sets a callback function that can be used by the VOIPClient implementation. The
    function augments will be of the form (int, dictionary). 
    Parameters: callback  (callable) A callable object that can handle arguments of the form (int, dictionary)  
    """
    return

def VOIP.setMasterVolume( attenuation, data ): 
    """Sets the overall volume at which voice communications will be played. 
    Parameters: attenuation  (float) The new speaker volume attenuation in decibels(dB)  
    data  (dictionary) Optional: Additional information in the form of string value pairs  
    

    Returns: (none)  
    """
    return

def VOIP.setMicrophoneVolume( attenuation, data ): 
    """Sets the attenuation applied to microphone capture. 
    Parameters: attenuation  (float) The new microphone attenuation in decibels(dB)  
    data  (dictionary) Optional: Additional information in the form of string value pairs  
    
    
    Returns: (none)  """
    return
    

def addAlwaysUpdateModel( pModel ): 
    """This function adds a PyModel to a list of models that have their nodes updated
    even when they are not visible. 
    Parameters: pModel  the model to always update  
    """
    return

def addDecal( start, end, size, type ): 
    """This function attempts to add a decal to the scene, given a world ray. If
    the world ray collides with scene geometry, a decal is placed over the intersecting
    region (clipped against the underlying collision geometry). Try not to have too
    large an extent for the decal collision ray, for reasons of performance. 
    Parameters: start  The start of the collision ray.  
    end  The end of the collision ray.  
    size  The size of the resultant decal.  
    type  The index of the decal texture.  
    """
    return

def addFogEmitter( position, density, innerRadius, outerRadius, colour, localised ): 
    """This function adds a fog emitter to the world at the specified location, with
    the specified density, radii and colour. 
    
    There are two types of emitters, non-localised and localised. A non-localised
    fog emitter adds fog to the entire world, causing all cameras to render their
    views with fog of the specified density. The emitters position, innerRadius
    and outerRadius values are ignored. 
    
    A localised fog emitter causes a camera to add fog to a scene if it is within
    the outer radius of the fog emitter. The density of the fog (try numbers between
    2 and 10, with 10 being denser) tapers off linearly between the specified density
    at the inner radius to zero at the outer radius. Note that a localised fog emitter
    does not actually appear in the camera as a patch of fog, if the camera is outside
    it. It merely applies a uniform fog to any cameras which are within it. 
    
    Fog emitters are identified by an integer id, which is returned by this function.
    This can be passed to the delFogEmitter function to remove that emitter. 
    Parameters: position  a Vector3. This is the position for the fogEmitter. Ignored
    if localised is zero (false).  
    density  a float. This is how dense to make the fog. Try numbers between 2 and 10,
    with 10 being denser.  
    innerRadius  a float. This is the inner radius for the fog emitter. Ignored if localised
    is zero (false). If the camera is within this radius of the emitter, then this emitter
    will apply fog of the specified density.  
    outerRadius  a float. This is the outer radius for the fog emitter. Ignored if localised
    is zero (false). If the camera is between the inner radius and the outer radius then the
    density of the fog applied to the camera will be linearly interpolated between the specified
    density and zero.  
    colour  an integer. This is the colour of the fog to apply. It is in packed rgb format.  
    localised  An integer, treated as a boolean. If this is true, then the fogEmitter will be
    localised, otherwise it will be non-localised.  
    
    
    Returns: an integer. This is the id of this fog emitter.  
    """
    return

def addFunctionWatcher( path, function, argumentList, exposedHint, desc ): 
    """This function interacts with the watcher debugging system, allowing 
    the creation of watcher variables. 
    
    The addFunctionWatcher function in Python allows the creation of a watcher
    path that can be queried as well as set with a variable length argument list. 
    
    For example: 
    
    BigWorld.addFunctionWatcher( "command/addGuards", util.patrollingGuards,
    [("Number of guards to add", int)], BigWorld.EXPOSE_LEAST_LOADED, "Add an
    arbitrary number of patrolling guards into the world.") 
    
    Raises a TypeError if the "function" argument is not callable. 
    Parameters: path  The path the watcher will be created as. Function watchers
    are generally placed under the "command/" path structure.  
    function  The function to call to with the provided argument list.  
    argumentList  The argument list (provided as a tuple) of types and descriptions that the function accepts. See the example for more details about its structure.  
    exposedHint  The default exposure hint to be used when forwarding the watcher call from Managers to Components.  
    desc  (Optional) A description of the function watcher behaviour and usage. While this field is optional, it is strongly recommended to provide this description in order to allow usage through WebConsole to be more intuitive.  
    """
    return

def addMat( source, callback, target ): 
    """This method adds a matrix trap to the active list. Rather than having
    a Player Entity trigger the trap, as in a pot (Player-Only Trap) it uses
    a MatrixProvider to trigger the trap. It could use the Player's matrix to
    perform the exact same task as a pot, however a mat has the added flexibility
    of being able to use any point in the world as the triggering element (be it
    an Entity or otherwise). If the target matrix is not provided, the MatrixProvider
    of the current Camera will be used. The scale of the source MatrixProvider
    represents the size of the mat. 
    
    When the matrix trap is triggered, it will notfiy the given callback function, defined as; 
    
    def callback( self, in, matID ): 
    
    where in will be 1 if the target MatrixProvider has entered the mat and 0 if
    it has left. The ID of the mat that was triggered is also provided 
    Parameters: source  The centre and size of the mat, as given by a MatrixProvider
    (size = scale of matrix)  
    callback  The script function to be called when the mat is triggered (to receive
    in flag and mat ID)  
    target  The optional MatrixProvider that will trigger the mat (uses current Camera 
    if not provided)  
    
    
    Returns: integer ID of the created and active matrix trap  
    """
    return

def addModel( pModel, spaceID ): 
    """This function adds a model to the global model list. It allows models to be
    drawn and ticked without being attached to either an entity or another model. This
    function actually allows any PyAttachment to be added, however it is called addModel
    to be consistent with the Entity method addModel. That method also allows any
    PyAttachment to be added. 
    Parameters: pModel  A PyAttachment to be added to the global models list.  
    spaceID  [optional] space ID. If not set, the current camera space is used.  
    """
    return

def addPot( centre, radius, callback ): 
    """This function adds a player-only trap to the active list of traps. This trap
    will trigger if the player entity moves either into or out of the sphere of
    specified radius and centre. When it triggers, it will call the specified callable
    object, with two arguments, the first of which is 0 if the player moved out of
    the sphere, and 1 if the player moved into the sphere. The second is the handle
    of the trap, which is the number returned by the addPot function. See Entity.addTrap for
    a trap that works on all entities. 

    For example: 
    # the callback function
    def hitPot( enteredTrap, handle ):
        if enteredTrap:
            print "The player entered the sphere for trap number ", handle
        else:
            print "The player left the sphere for trap number ", handle
    
    # Note: player.matrix is the players current position
    BigWorld.addPot( BigWorld.player().matrix, 5.0, hitPot )
    
    In this example, the hitPot function will get called whenever the player enters
    or leaves a stationary sphere with a radius of 5 metres (the position of the sphere
    is centred around where the player was when the addPot function was called). 
    Parameters: centre  a MatrixProvider. This is the centre of the sphere.  
    radius  a float. This is the radius of the sphere to trap.  
    callback  a callable object. This is the function that gets called when the trap is triggered.  
    
    
    Returns: an integer. The handle of the trap.  """
    return


def addShadowEntity( Entity ): 
    """Adds the given entity to the dynamic shadow manager. The manager holds onto a weak reference to the entity. 
    Parameters: Entity  entity to be added to the shadow manager.  
    """
    return

def addSkyBox( a, p ): 
    """This function registers a PyAttachment and Vector4Provider pair. The output
    of the provider sets various values on the sky box, in particular the alpha value
    controls the blend in/out amount. 
    
    The Vector4Provider is set onto the sky box's effect file using the SkyBoxController semantic. 
    Parameters: a  the PyAttchment to use as a python sky box  
    p  the Vector4Provider to set on the sky box  
    """
    return

def addSpaceGeometryMapping( spaceID, matrix, filepath ): 
    """This function maps geometry into the given client space ID. It cannot be
    used with spaces created by the server since the server controls the geometry
    mappings in those spaces. 
    
    The given transform must be aligned to the chunk grid. That is, it should be
    a translation matrix whose position is in multiples of 100 on the X and Z axis.
    Any other transform will result in undefined behaviour. 
    
    Any extra space mapped in must use the same terrain system as the first, with
    the same settings, the behaviour of anything else is undefined. 
    
    Raises a ValueError if the space for the given spaceID is not found. 
    Parameters: spaceID  The ID of the space  
    matrix  The transform to apply to the geometry. None may be passed in if 
    no transform is required (the identity matrix).  
    filepath  The path to the directory containing the space data  
    
    
    Returns: (integer) handle that is used when removing mappings using
    BigWorld.delSpaceGeometryMapping().  
    """
    return

def addTextureFeed( identifier, texture ): 
    """This adds a new TextureProvider to BigWorld, identified by a given label. This
    label can then be used to refer to the TextureProvider in other parts of BigWorld. 
    Parameters: identifier  A string label to identify this TextureProvider  
    texture  A PyTextureProviderPtr that is the TextureProvider to link to this label  
    """
    return

def addWatcher( path, getFunction, setFunction ): 
    """This function interacts with the watcher debugging system, allowing the creation of watcher variables. 
    
    The addWatcher function in Python allows the creation of a particular watcher variable. For example: 

    
    >>> maxBandwidth = 20000
    >>> def getMaxBps( ):
    >>>     return str(maxBandwidth)
    >>> def setMaxBps( bps ):
    >>>     maxBandwidth = int(bps)
    >>>     setBandwidthPerSecond( int(bps) )
    >>>
    >>> BigWorld.addWatcher( "Comms/Max bandwidth per second", getMaxBps, setMaxBps )
    
    

    This adds a watcher variable under "Comms" watcher directory. The function getMaxBps
    is called to obtain the watcher value and setMaxBps is called when the watcher is modified. 
    
    Raises a TypeError if getFunction or setFunction is not callable. 
    Parameters: path  the path to the item to create.  
    getFunction  the function to call when retrieving the watcher variable. This function
    takes no argument and returns a string representing the watcher value.  
    setFunction  (optional)the function to call when setting the watcher variable This
    function takes a string argument as the new watcher value. This function does not
    return a value. If this function does not exist the created watcher is a read-only watcher variable.  
    """
    return

def addWaterVolumeListener( dynamicSource, callback ): 
    """Parameters: dynamicSource  A MatrixProvider providing the listener's location.  
    callback  A callback fn that takes one boolean argument and one PyWaterVolume argument.  
    """
    return

def ambientController( p ): 
    """This function registers a vector4 provider to provide further control over the
    ambient lighting. It is interpreted as an (r,g,b,reserved) multiplier on the
    existing time-of-day ambient colour. 
    Parameters: p  the Vector4Provider to set  
    """
    return

def autoDetectGraphicsSettings( ): 
    """Automatically detect the graphics settings based on the client's system properties. """
    return

def axisDirection( axis ): 
    """This function returns the direction the specified joystick is pointing in. 
    
    The return value indicates which direction the joystick is facing, as follows: 
    
    
    - 0 down and left
    - 1 down
    - 2 down and right
    - 3 left
    - 4 centred
    - 5 right
    - 6 up and left
    - 7 up
    - 8 up and right
    
    Parameters: axis  This is one of AXIS_LX, AXIS_LY, AXIS_RX, AXIS_RY, with the first
    letter being L or R meaning left thumbstick or right thumbstick, the second, X or Y 
    being the direction.  
    
    
    Returns: An integer representing the direction of the specified thumbstick, as listed above.  
    """
    return

def callback( time, function ): 
    """Registers a callback function to be called after a certain time, but not before
    the next tick. The callback is executed once and is not automatically repeated, so
    reset the callback again if regular updates are needed. If multiple callbacks are
    due to be executed within a single frame, the chronological order in which they were
    added is maintained. 
    Parameters: time  A float describing the delay in seconds before function is called.
    If a time of 0 is given, then it is guaranteed to be called on the next frame.  
    function  Function to call. This function must take 0 arguments.  
    
    
    Returns: int A handle that can be used to cancel the callback.  
    """
    return

def camera( cam ): 
    """This function is used to get and set the camera that is used by BigWorld to render
    the world. It can be called with either none or one arguments. If it has no arguments,
    then it returns the camera currently in use. If it is called with one argument, then
    that argument should be a camera and it is set to be the active camera. In this case
    the function returns None. 
    Parameters: cam  a Camera object, which is set to be the current camera. If this is
    not specified, the current camera will be returned.  
    
    
    Returns: A Camera object or None  
    """
    return

def cameraSpaceID( spaceID ): 
    """Parameters: spaceID  (optional) Sets the space that the camera exists in.  


    Returns: spaceID The spaceID that the camera currently exists in. If the
    camera is not in any space then 0 is returned.  
    """
    return

def cancelCallback( int ): 
    """Cancels a previously registered callback. 
    Parameters: int  An integer handle identifying the callback to cancel.  

    
    Returns: None.  
    """
    return

def changeFullScreenAspectRatio( ratio ): 
    """Changes screen aspect ratio for full screen mode. 
    Parameters: ratio  the desired aspect ratio: float (width/height).  
    """
    return

def changeVideoMode( new, windowed ): 
    """This function allows you to change between fullscreen and windowed mode. If
    switching to fullscreen mode, the video mode index is used to determine the new
    resolution. If switching to windowed mode, this parameter is ignored. 

    The exception to the above is if you set the modeIndex to -1, then both parameters
    are ignored, and the device will simply be reset and remain with its current settings. 
    
    The video mode index is reported via the listVideoModes function. 
    Parameters: new  int - fullscreen video mode to use.  
    windowed  bool - True windowed mode is desired.  
    
    
    Returns: bool - True on success. False otherwise.  
    """
    return

def chunkTransform( chunkNMapping, spaceID ): 
    """Parameters: chunkNMapping  chunkNMapping is a string containing
    the name of the chunk whose transform is to be returned.  
    spaceID  spaceID is the id of the space in which the chunk resides.  
    
    
    Returns: A Matrix which describes the chunk's transform.  
    """
    return

def clearAllSpaces( keepClientOnlySpaces ): 
    """As the name implies, clears all currently loaded spaces. If the
    optional Boolean parameter is True client only spaces will not be
    cleared. Default is False. 
    Parameters: keepClientOnlySpaces  If true this function will leave
    spaces created locally intact.  
    """
    return

def clearDecals( ): 
    """This function clears the current list of decals. Only the decals
    on the space currently being viewed by the camera will be cleared. 

    @note Before version 1.9, this needed to be called when changing spaces,
    so that decals from another space didn't draw in the current one. From 1.9
    onwards, decals are stored per space and calling this function is no
    longer a requirement. """
    return

def clearEntitiesAndSpaces( ): 
    """As the name implies, removes all entities (including the player) and
    clear all currently loaded spaces. Equivalent to calling BigWorld.resetEntityManager
    followed by BigWorld.clearAllSpaces. """
    return

def clearSpace( spaceID ): 
    """This function clears out the given client space ID, including unmapping
    all geometry in it. Naturally there should be no entities inside the space
    when this is called or else they will be stranded without chunks. It cannot
    be used with spaces created by the server since the server controls the
    geometry mappings in those spaces. 

    Raises a ValueError if the space for the given spaceID is not found. 
    Parameters: spaceID  The ID of the space  
    """
    return

def collide( spaceID, src, dst ): 
    """This function performs a collision test along a line segment from
    one point to another through the collision scene. If the line intersects
    nothing then the function returns None. Otherwise, it returns a tuple
    consisting of the point the first collision occurred at, the triangle
    that was hit (specified by its three points, as Vector3s), and the material
    type of the poly that was hit (an integer between 0 and 255). This material
    type corresponds to the material or object Kind specified in ModelViewer
    (see Object->Properties and Material->Material Properties panes). 

    For example: 
        >>> BigWorld.collide( spaceID, (0, 10, 0), (0,-10,0) )
        ((0.0000, 0.0000, 0.0000), ( (0.0000, 0.0000, 0.0000),
                                     (4.0000, 0.0000, 0.0000), (4.0000, 0.0000, 4.0000)), 0)
    
    In this example the line goes from 10 units above the origin to 10 units below
    it. It intersects with a triangle at the origin, where the triangles three
    points are (0,0,0), (4,0,0) and (4,0,4). The material is of type 0. 
    Parameters: spaceID  The space in which to perform the collision.  
    src  The point to project from.  
    dst  The point to project to.  
    
    
    Returns: A 3-tuple of the hit point, the triangle it hit, and the
    material type or None if nothing was hit.  
    """
    return

def commandLineLoginInfo( ):    
    """This function returns the username and password that where specified
    on the command line as a tuple. If none were specified, None is returned. 
    
    The command line flags are --username and --password. (-u and -p can also be used). """
    return
    
def commitPendingGraphicsSettings( ): 
    """This function commits any pending graphics settings. Some graphics settings,
    because they may block the game for up to a few minutes when coming into effect,
    are not committed immediately. Instead, they are flagged as pending and require
    commitPendingGraphicsSettings to be called to actually apply them. 
    """
    return

def connect( server, loginParams, progressFn ): 
    """This function initiates a connection between the client and a specified server.
    If the client is not already connected to a server and is able to reach the network
    then all entities which reside on the client are destroyed in preparation for
    entering the server's game. This function can be used to tell the client to
    run offline, or to pass username, password, and other login data to the server. 
    The client can later be disconnected from the server by calling BigWorld.disconnect. 
    Parameters: server  This is a string representation of the name of the server
    which the client is to attempt to connect. This can be in the form of an IP
    address (eg: "11.12.123.42"), a domain address (eg: "server.coolgame.company.com"),
    or an empty string (ie: "" ). If an empty string is passed in then the client will 
    run in offline mode. If a connection is successfully established then subsequent 
    calls to BigWorld.server will return this value.  
    loginParams  This is a Python object that may contain members which provide login
    data. If a member is not present in this object then a default value is used instead. 
    These members are:  
    progressFn  This is a Python function which is called by BigWorld to report on the
    status of the connection. This function must take three arguments, where the first
    is interpreted as an integer indicating where in the connection process the report
    indicated, the second is interpreted as a string indicating the status at that point
    and the third is a string message that the server may have sent.  
    
    
    Returns: None  
    """
    return

def connectedEntity( ): 
    """This method returns the entity that this application is connected to. The connected
    entity is the server entity that is responsible for collecting and sending data to
    this client application. It is also the only client entity that has an Entity.base property. 
    """
    return

def consumerBuild( ): 
    """Check if this is a consumer build. 
    Returns: true if this is a consumer build.  
    """
    return

def controlEntity( entity, beControlled ): 
    """Sets whether the movement of an entity should be controlled locally by physics. 
    
    When shouldBeControlled is set to True, the entity's physics attribute becomes
    accessible. Each time this is called with its shouldBeControlled attribute as
    True, the entity's physics attribute is set to None. As this is also made
    accessible, it can then be set to a different value. 
    
    When shouldBeControlled is set to False, attempts to access the entity's
    physics object raise an AttributeError. 
    
    This function only works for client-side entities. The server decides
    who controls server-side entities. A TypeError is raised if the given
    entity is not a client-side entity. 
    Parameters: entity  The entity to control/uncontrol.  
    beControlled  Whether the entity should be controlled.  
    """
    return

def createEntity( className, spaceID, vehicleID, position, direction, state ): 
    """Creates a new entity on the client and places it in the world. The
    resulting entity will have no base or cell part. 
    Parameters: className  The string name of the entity to instantiate.  
    spaceID  The id of the space in which to place the entity.  
    vehicleID  The id of the vehicle on which to place the entity (0 for no vehicle).  
    position  A Vector3 containing the position at which the new entity is to be spawned.  
    direction  A Vector3 containing the initial orientation of the new
    entity (roll, pitch, yaw).  
    state  A dictionary describing the initial states of the entity's
    properties (as described in the entity's .def file). A property will
    take on it's default value if it is not listed here.  
    
    
    Returns: The ID of the new entity, as an integer.  
    """
    return

def createSpace( ): 
    """The space is held in a collection of client spaces for safe keeping,
    so that it is not prematurely disposed. 
    Returns: spaceID The ID of the client-only space that has been created  
    """
    return

def createTranslationOverrideAnim( baseAnim, translationReferenceAnim, noOverrideChannels, outputAnim ): 
    """This function is a tool which can be used to alter skeletal animations so that
    they can be used with models which have skeletons of different proportions. This
    is achieved by creating a new animation which is based on a given animation, but 
    replaces the translation component for each node with that of the beginning of the
    same node in a reference animation. As the translation should not change in a skeletal
    system (bones do not change size or shape), this effectively re-fits the animation on
    to a differently proportioned model. This operates by creating a new animation file,
    and is not intended for in-game use. 
    Parameters: baseAnim  A string containing the name (including path) of the animation
    file on which the new file is to be based.  
    translationReferenceAnim  A string containing the name (including path) of the animation
    file whose first frame contains the translation which will be used for the new animation.
    This should have the same proportions as are desired for the new animation.  
    noOverrideChannels  A list of strings containing the names of the nodes that shouldn't 
    have their translation overridden. These nodes will not be scaled to the proportions
    provided by translationReferenceAnim in the new animation.  
    outputAnim  A string containing the name (including path) of the animation file to
    which the new animation will be saved.  
    
    
    Returns: None  
    """
    return

def darkenConsoleBackground( enable ): 
    """Sets the dark-console-background flag. When true, a dark background will be
    rendered underneat the current active console (if any), making the console text
    easier to read. If false, console text will be rendered directly over the current
    contents of the color buffer. Some console may inhibit the renderting of the dark
    background (e.g. the histogram console). 
    Parameters: enable  (optional) new value to be assigned to the dark-console-background flag.  
    
    
    Returns: the current value (before the call is processed).  
    """
    return

def dcursor( ): 
    """This function returns the direction cursor which is used to control
    which way the user is looking with the mouse. 
    Returns: the current direction cursor.  
    """
    return

def debugAQ( ): 
    """This function controls the Action Queue debugging graph. It takes one argument
    which is either a PyModel, or None. None switches the debugging graph off. If the
    argument is a model, then a graph is displayed on the screen. It is a line graph,
    which shows what actions are currently playing on that model, and what blend weight
    each action has. 

    Each line represents one currently playing action. It will have an arbitrary
    colour assigned to it, and have the Actions name printed below it in the same
    colour. The height of the line represents what percentage of the total blended
    Actions this Action makes up. 
    
    If one model is currently being graphed, and this function is called again with
    another model, the graph for the first model stops, and the new model takes over. """
    return

def debugModel( ): 
    """This function is used to turn on/off the drawing of the models * skeleton.
    This can also be done through the watcher located in * Render/Draw Skeletons 
    """
    return
def decalTextureIndex( textureName ): 
    """This function returns the texture index that is needed to pass into addDecal.
    It takes the name of the texture, and returns the index, or -1 if none was found. 
    Parameters: textureName  The name of the texture to look up.  
    
    
    Returns: The index of the texture  
    """
    return

def delAlwaysUpdateModel( pModel ): 
    """This function removes a model from the update list. 
    Parameters: pModel  the model to remove from the update list  
    """
    return

def delFogEmitter( id ): 
    """This function removes a fog emitter which was previously created by addFogEmitter. 
    Parameters: id  An integer. The id of the fog emitter to delete. This is
    returned by addFogEmitter when a fog emitter is created.  
    """
    return

def delMat( matID ): 
    """Removes the mat from active duty as described by the given mat handle. 
    Parameters: matID  The integer ID of the mat to remove. Corresponds to the handle returned by BigWorld.addMat()  
    """
    return

def delModel( pModel ): 
    """This function deletes a model from the global model list. 

    This function actually works with any PyAttachment, however it is called
    delModel to be consistent with the Entity method delModel. That method 
    also allows any PyAttachment to be deleted. 
    Parameters: pModel  A PyAttachment to be added to the global models list.  
    """
    return

def delPot( handle ): 
    """This method deletes a trap from the active list. This stops
    it responding to the player entity entering or leaving its sphere. 

    The trap is specified by its handle, which was returned by addPot
    when the trap was created. 
    Parameters: handle  an integer. The handle of the trap to remove.  
    """
    return

def delShadowEntity( Entity ): 
    """Removes the given entity from the dynamic shadow manager. Does
    nothing if the entity was not previously added. 
    Parameters: Entity  entity to be removed from the shadow manager.  
    """
    return

def delSkyBox( m, p ):  
    """This function registers a PyAttachment and Vector4Provider pair.
    The output of the provider sets various values on the sky box, in
    particular the alpha value controls the blend in/out amount. 
    
    The Vector4Provider is set onto the sky box's effect file using
    the SkyBoxController semantic. 
    Parameters: m  the PyAttachment to use as a python sky box  
    p  the Vector4Provider to set on the sky box  
    """
    return

def delSpaceGeometryMapping( spaceID, handle ): 
    """This function unmaps geometry from the given client space ID. 
    
    It cannot be used with spaces created by the server since the server
    controls the geometry mappings in those spaces. 
    
    Raises a ValueError if the space for the given spaceID is not found,
    or if the handle does not refer to a mapped space geometry. 
    Parameters: spaceID  The ID of the space  
    handle  An integer handle to the space that was returned when created  
    """
    return

def delStaticSkyBoxes( ): 
    """"This function removes the static sky boxes added via Worldeditor. 
    This is usually used when you want script control and dynamic sky boxes,
    in which case you'd use this method as well as addSkyBox / delSkyBox. """
    return

def delTextureFeed( identifier ): 
    """This function removes a previously added TextureProvider. The TextureProvider
    can still be used in script, but cannot be referred to by its previously associated label anymore. 
    Parameters: identifier  A string label to identity the TextureProvider to remove  
    """
    return

def delWatcher( path ):         
    """Parameters: path  the path of the watcher to delete.  
    """
    return

def delWaterVolumeListener( id ): 
    """Parameters: id  id formerly returned by addWaterVolumeListener  
    """
    return

def destroyEntity( id ): 
    """Destroys an exiting client-side entity. 
    Parameters: id  The id of the entity to destroy.  
    """
    return

def disconnect( ): 
    """If the client is connected to a server then this will terminate the connection. Otherwise it does nothing. This function does not delete any entities, however it does stop the flow of data regarding entity status to and from the server. 
    
    The client can be connected to a server via the BigWorld.connect function. 
    Returns: None  
    """
    return

def dumpMemCounters( ): 
    """This debugging function prints out the current value of memory
    watchers found in "Memory/" watcher directory. 
    """
    return
    
def dumpRefs( ): 
    """Dumps references to each object visible from the entity tree.
    This does not include all objects, and may not include all references. 
    """
    return

def entity( id, lookInCache ): 
    """Returns the entity with the given id, or None if not found. This function
    can search all entities known to the client, or only entities which are not
    cached. An entity only becomes cached if in an online game the server indicates
    to the client that the entity has left the player's area of interest. 
    Parameters: id  An integer representing the id of the entity to return.  
    lookInCache  An optional boolean which instructs the function to search for
    the entity amongst the entities which are cached. Otherwise, this function
    will only search amongst non-chached entities. This argument defaults to false.  
    
    
    Returns: The entity corresponding to the id given, or None if no such entity is found.  
    """
    return

def fetchModel( modelPath, onLoadCompleted ): 
    """Deprecated: It is recommended to use BigWorld.loadResourceListBG in the following manner instead: 
    # load list of models in background
    myList = [ "a.model", "b.model" ]
    BigWorld.loadResourceListBG( myList, self.callbackFn )
    def callbackFn( self, resourceRefs ):
        if resourceRefs.failedIDs:
            ERROR_MSG( "Failed to load %s" % ( resourceRefs.failedIDs, ) )
        else:
            # construct supermodel
            myModel = BigWorld.Model( *resourceRefs.keys() )
    


            fetchModel is a function that creates and returns a 
            new PyModel object through the callback function provided (None if there
            are problems loading any of the requested resources). PyModels are renderable
            objects which can be moved around the world, animated, dyed (have materials
            programatically assigned) and contain points to which objects can be attached. 

            The parameters for the function are one or more model resources. Each of these
            individual models is loaded, and combined into a "supermodel". 
            
            This function is similar to the BigWorld.Model function. However, it schedules
            the model loading as a background task so that main game script execution would
            not be affected. After model loading is completed, callback function will be
            triggered with the valid PyModel. 
            
            For example, to load a body and a head model into one supermodel: 
            

            BigWorld.fetchModel( "models/body.model", "models/head.model", onLoadCompleted )
            
            Parameters: *modelPath  String argument(s) for the model.  
            onLoadCompleted  The function to be called when the model resource loading is
            completed. An valid PyModel will be returned. When the model loading fails. A
            None is passed back.  


            Returns: No return values.  """
    return


def findChunkFromPoint( point, spaceID ): 
    """Parameters: point  point is a Vector3 describing the location to search for a chunk, in world space.  
    spaceID  spaceID is the id of the space to search in.  
    
    
    Returns: The name of the chunk found, as a string.  
    """
    return

def findDropPoint( spaceID, vector3 ): 
    """Finds the point directly beneath the start point that collides with
    the collision scene and terrain (if present in that location) 
    Parameters: spaceID  The ID of the space you want to do the raycast in  
    vector3  Start point for the collision test  
    
    
    Returns: A pair of the drop point, and the triangle it hit, or None if nothing was hit.  
    """
    return

def flashBangAnimation( p ): 
    """This function adds a vector4 provider for the flash bang animation. The output
    of the flashbanganimation is modulated with the previous frame of the game to 
    create a saturated frame buffer. There can be any number of flash bang animations running at the same time. 
    Parameters: p  the Vector4Provider to add  
    """
    return

def floraReset( ): 
    """This method will re-initialise the flora vertex buffer, i.e. repopulating
    all flora objects in the vicinity of the camera. This needs to be called if
    for example you have changed the height map and need the flora to be re-seeded 
    onto the new terrain mesh. 
    """
    return

def floraVBSize( Number ): 
    """This method sets the vertex buffer size for the Flora. 
    Parameters: Number  of bytes as an integer.  

    """
    return

def flyThroughRunning( ): 
    """This function returns true if a fly through is currently running 
    Returns: True if a fly through is currently running  
    """
    return

def fogController( p ): 
    """This function registers a vector4 provider to provide further control over
    the fogging. (x,y,z) is a multiplier on the existing fog colour. (w) is a multiplier
    on the fog density. 
    Parameters: p  the Vector4Provider to set  
    """
    return

def getFullScreenAspectRatio( ): 
    """This function returns an estimate of the amount of memory the application is
    currently using. 
    """
    return

def getGraphicsSetting( label ): 
    """Gets graphics setting option. 
    
    Raises a ValueError if the given label does not name a graphics setting, if
    the option index is out of range, or if the option is not supported. 
    Parameters: label  string - label of setting to be retrieved.  
    
    
    Returns: optionIndex int - index of option.  
    """
    return


def getMaterialKinds( ): 
    """This method returns a list of id and section tuples of the material
    kind list, in the order in which they appear in the XML file 
    Returns: The list of (id, DataSection) tuples of material kinds  
    """
    return

def getTextureFeed( identifier ): 
    """This function returns a previously added TextureProvider. 
    Parameters: identifier  A string label to identity the TextureProvider to return  
    """
    return

def getWatcher( path ): 
    """"This function interacts with the watcher debugging system, allowing the retrieval of watcher variables. 
    
    The watcher debugging system allows various variables to be displayed and updated from within the runtime. 
    
    Watchers are specified in the C-code using the MF_WATCH macro. This takes three arguments,
    the first is a string specifying the path (parent groups followed by the item name, separated
    by forward slashes), the second is the expression to watch. The third argument specifies such
    things as whether it is read only or not, and has a default of read-write. 
    
    The getWatcher function in Python allows the getting of a particular value. For example: 

    
    >>>	BigWorld.getWatcher( "Client Settings/Terrain/draw" )
    1
    
    
    
    Raises a TypeError if the watcher was not found. 
    Parameters: path  the path to the item to modify.  
    
    
    Returns: the value of that watcher variable.  
    """
    return

def getWatcherDir( path ): 
    """This function interacts with the watcher debugging system, allowing the inspection of a directory watcher. 

    Raises a TypeError if not found. 
    Parameters: path  the path to the item to modify.  
    
    
    Returns: a list containing an entry for each child watcher. Each entry is a tuple of
    child type, label and value. The child type is 1 for read-only, 2 for read-write and 3 for a directory.  
    """
    return

def graphicsSettings( ): 
    """"Returns list of registered graphics settings 
    Returns: list of 4-tuples in the form (label : string, index to active option in
    options list: int, options : list, desc : string). Each option entry is a 3-tuple
    in the form (option label : string, support flag : boolean, desc : string).  
    """
    return
    
def graphicsSettingsNeedRestart( ): 
    """This function returns true if any recent graphics setting change requires the client
    to be restarted to take effect. If that's the case, restartGame can be used to restart
    the client. The need restart flag is reset when this method is called. 
    """
    return

def hasPendingGraphicsSettings( ): 
    """This function returns true if there are any pending graphics settings. Some graphics
    settings, because they may block the game for up to a few minutes when coming into effect, 
    are not committed immediately. Instead, they are flagged as pending and require
    commitPendingGraphicsSettings to be called to actually apply them. 
    """
    return

def isKeyDown( key ): 
    """The 'isKeyDown' method allows the script to check if a particular key has been pressed 
    and is currently still down. The term key is used here to refer to any control with an
    up/down status; it can refer to the keys of a keyboard, the buttons of a mouse or even
    that of a joystick. The complete list of keys recognised by the client can be found
    in the Keys module, defined in Keys.py. 
    
    The return value is zero if the key is not being held down, and a non-zero 
    value is it is being held down. 
    Parameters: key  An integer value indexing the key of interest.  
    
    
    Returns: True (1) if the key is down, false (0) otherwise.  
    """
    return

def isTripleBuffered( ): 
    """Returns current display's triple buffering status. 
    Returns: bool - True if triple buffering is on. False if it is off.  
    """
    return

def isVideoVSync( ): 
    """Returns current display's vertical sync status. 
    Returns: bool - True if vertical sync is on. False if it is off.  
    """
    return

def isVideoWindowed( ): 
    """Queries current video windowed state. 
    Returns: bool True is video is windowed. False if fullscreen.  
    """
    return

def keyToString( key ): 
    """The 'keyToString' method converts from a key index to its corresponding
    string name. The string names returned by the integer index can be found in 
    the keys.py file. If the index supplied is out of bounds, an empty string will be returned. 
    Parameters: key  An integer representing a key index value.  
    
    
    Returns: A string containing the name of the key supplied.  
    """
    return

def listVideoModes( ): 
    """Lists video modes available on current display device. 
    Returns: list of 5-tuples (int, int, int, int, string). (mode index, width, height, BPP, description)  
    """
    return
    
def loadResourceListBG( resourceList, callbackFn, priority ): 
    """This function loads a list of resources in the background thread and is designed
    to be use as an extension to the Entity prerequisites mechanism, for times when you
    don't know ahead if time what resources an entity requires, or if the entity changes
    in such a way that requires new resources to load. 
    
    The function takes a tuple of resource IDs, a callback function and an optional
    priority number as arguments. The resource list is loaded in the background thread
    and then the callback function is called with a new ResourceRefs instance as the
    argument. You can use the resources immediately, or you can hold onto the ResourceRefs
    object; the loaded objects are tied to its lifetime. Nothing is returned from this
    function call immediately. The priority number defines which resources are need to
    be loaded before others where a higher number indicates a higher priority. 
    
    
    
    For example: 
    # In the example, a new item is obtained; the item is one of many available
    # and thus there is no way to know before-hand what resources will be
    # required.  When the item is obtained, a ResourceRefs object is
    # constructed; it loads the resources in the background, and calls back the
    # function when done.  While the resourceRefs object is in existence, it
    # will hold onto the resources making sure they aren't unloaded.  In this
    # case a model is immediately constructed and a gui texture is immediately
    # set - thus it is unnecessary to hold onto the extra resource refs at all.
    # There is an alternative example not being called, onLoad2, which does not
    # use the resources immediately, it uses the ResourceRefs object to hold
    # onto them until needed later.
    
    self.obtainItem( "maps/items/icon_item4.tga","models/items/item4.model" )
    
    def obtainItem( self, iconName, modelName ):
        resName = (iconName, modelName)
        BigWorld.loadResourceListBG(resources, partial(self.onLoad, resName))
    
    # In this case we will construct the objects we needed, so we don't
    # have to keep a hold of the resource refs object.  Note we check
    # isDestroyed, as in this example self is an entity, and the entity
    # may have left the world during our resource loading.
    def onLoad( self, resName, resourceRefs ):
        assert isinstance( self, BigWorld.Entity )
        if self.inWorld:
            failed = resourceRefs.failedIDs
            guiName = resourceRefs[ resName[0] ]
            modelName = resourceRefs[ resName[1] ]
    
            #now you can construct models or set gui textures or whatever
            #without causing resource loads in the main thread.
    
            if guiName not in failed:
                self.itemGUI.textureName = guiName
            else:
                ERROR_MSG( "Could not load gui texture %s" % (guiName,) )
    
            if modelName not in failed:
                self.newItemModel = resourceRefs[modelName]
            else:
                ERROR_MSG( "Could not load model %s" % (modelName,) )
    
    # In this case we won't use the resources straight away, so
    # we keep a hold of the resource refs until we need them.
    def onLoad2( self, resourceRefs ):
        if not self.inWorld:
            self.resourcesHolder = resourceRefs
    
    def discardItem( self ):
        #release the ResourceRefs object, freeing the references
        #to all associated resources.  If nobody else is using these
        #resources they will be freed.
        del self.resourcesHolder
    
    Parameters: resourceList  a list of resources to load. This is similar to the
    list returned by entity prerequisites.  
    callbackFn  a function to be called back when the load of all resources is complete. 
    The callback function takes no arguments.  
    priority  [optional] integer to indicate the priority number (defaults to 64).  
    
    
    Returns: None """ 
    return


def localeInfo( ): 
    """Returns a tuple containing information about the currently activated input. locale.
    The locale changes when the user selects a new language from the language bar in the 
    operating system. Use the BWPersonality.handleInputLangChange event handler to determine
    when this happens. 

    The tuple returned is of the form: 
    
    
    (country, abbreviatedCountry, language, abbreviatedLanguage)
    
    Returns: A 4-tuple of unicode strings that contain containing locale information..  
    """
    return

def memUsed( ): 
    """This function returns an estimate of the amount of memory the application is currently using. 
    """
    return

def models( Any ): 
    """This function sets or gets the global model list. 
    Parameters: Any  sequence of PyAttachments, or None.  

    
    Returns: The global models list, if no arguments were passed in, or None.  
    """
    return

def noPartialLocks( ): 
    """This function is deprecated. Use BigWorld.target.noPartial attribute instead. 
    """
    return

def playMovie( ): 
    """Placeholder for deprecated functionality. 
    """
    return

def player( entity ): 
    """This sets the player entity, or returns the current player entity if the entity 
    argument is not provided. The BigWorld engine assumes that there is no more than one
    player entity at any time. Changing whether an entity is the current player entity
    involves changing whether it is an instance of its normal class, or its player class.
    This is a class whose name equals the entity's current class name, prefixed with the
    word "Player". As an example, the player class for Biped would be PlayerBiped. 



    The following occurs if a new entity is specified: 
    
    The onBecomeNonPlayer function is called on the old player entity. 
    
    The old player entity becomes an instance of its original class, rather than its player class. 
    
    The reference to the current player entity is set to None. 
    
    A player class for the new player entity is sought out. If the player class for the
    new player entity is found, then the entity becomes an instance of this class. Otherwise, 
    the function immediately returns None. 
    
    The onBecomePlayer function is called on the new player entity. 
    
    The reference to the current player entity is set the new player entity. 
    Parameters: entity  An optional entity. If supplied, this entity becomes the current player.  
    
    
    Returns: If the entity argument is supplied then this is None, otherwise it's the current player entity.  
    """
    return

def playerDead( isDead ): 
    """This method sets whether or not the player is dead. This information
    is used by the snow system to stop snow falling if the player is dead. 
    Parameters: isDead  an integer as boolean. This is 0 if the player is alive,
    otherwise it is non-zero.  
    """
    return

def probe( server, progressFn ): 
    """The function probe is used to determine the status of a particular server.
    This has not yet been implemented. 
    Parameters: server  String. Specifies the name or address of server.  
    progressFn  Callback Function (python function object, or an instance of a
    python class that implements the __call__ interface). The function to be
    called when information on the status for a particular server is available.  
    """
    return

def projection( ): 
    """This function retrieves the ProjectionAccess object for the current
    camera, allowing access to such things as the fov and the clip planes. 
    Returns: ProjectionAccess  
    """
    return

def quit( ): 
    """"Ask the application to quit. 
    """
    return

def releaseSpace( spaceID ): 
    """This function releases the given client space ID. 

    Note that the space will not necessarily be immediately deleted
    - that must wait until it is no longer referenced (including by chunk dir mappings). 
    
    Raises a ValueError if the space for the given spaceID is not found. 
    Parameters: spaceID  The ID of the space to release.  
    """
    return

def reloadChunks( ):    
    """Unload all chunks, purge all ".chunk" data sections and begin reloading the space. 
    """
    return

def reloadTextures( ): 
    """This function recompresses all textures to their .dds equivalents, without restarting
    the game. The procedures may take a while, depending on how many textures are currently
    loaded by the engine. This is especially useful if you have updated the
    texture_detail_levels.xml file, for example to change normal maps from 32 bit to 16 bits per pixel. 
    """
    return

def removeFlashBangAnimation( p ): 
    """This function removes a Vector4Provider from the list of flash bang animations 
    Parameters: p  the Vector4Provider to remove  
    """
    return

def resetEntityManager( keepPlayerAround, keepClientOnly ): 
    """Resets the entity manager's state. After disconnecting, the entity will retain
    all of it's state. Allowing the world to still be explored offline, even with 
    entity no longer being updated by the server. Calling this function resets the 
    entity manager, removing all entities from the space. 
    Parameters: keepPlayerAround  (type bool) True if player should be spared. False
    if player should also be removed together with all entities.  
    keepClientOnly  (type bool) True to clear all server entities and keep client only
    entities.  
    """
    return
    
def resizeWindow( width, height ): 
    """Sets the size of the application window (client area) when running
    in windowed mode. Does nothing when running in fullscreen mode. 
    Parameters: width  The desired width of the window's client area.  
    height  The desired height of the window's client area.  
    """
    return

def restartGame( ): 
    """This function restarts the game. It can be used to restart the game after certain graphics options have been changed that require a restart. 
    """
    return

def rollBackPendingGraphicsSettings( ): 
    """This function rolls back any pending graphics settings. 
    """
    return

def saveAllocationsToFile( filename ): 
    """Save all recorded memory allocations into the text file for
    further analysis File format description: 
    
    number_of_callstack_hashes 64bit hash;callstack 64bit hash;callstack .... 
    64bit hash;callstack slotId;64bit callstack hash;allocation size ... 
    slotId;64bit callstack hash;allocation size 
    
    Use condense_mem_allocs.py script to convert saved file into the 
    following format: slot id;allocationSize;number of allocations;callstack ... 
    Parameters: filename  The name of file to save data  
    """
    return

def saveFontCacheMap( identifier ): 
    """This saves the current cache texture map for the given font out
    to disk as a .dds file with the same name as the font file. 
    
    Additionally it saves out a .generated file of the same name, which
    is a binary file with the dimensions and positions of each glyph in
    that map (note that the font will continue to operate dynamically until it is next loaded). 
    
    Finally it saves another DDS file that has colour-coded blocks 
    representing glyph dimensions, the effect margin and texture margin
    to aid artists with generating their own font texture maps. 
    
    Please see the client programming guide for more information. 
    Parameters: identifier  A string label to identify the font  
    """
    return

def savePreferences( ): 
    """Saves the current preferences (video, graphics and script) into a
    XML file. The name of the file to the writen is defined in the field in engine_config.xml. 
    Returns: bool True on success. False on error.  
    """
    return

def screenHeight( ): 
    """Returns the height of the current game window. 
    Returns: float  
    """
    return

def screenShot( format, name ): 
    """This method takes a screenshot and writes the image to disk. The output 
    folder is configured by the 'screenShot/path' section in engine_config.xml. 
    Parameters: format  Optional string. The format of the screenshot to be outputed,
    can be on of "bmp", "jpg", "tga", "png" or "dds". The default comes from resources.xml.  
    name  Optional string. This is the root name of the screenshot to generate.
    A unique number will be postpended to this string. The default comes from resources.xml.  
    """
    return

def screenSize( ): 
    """Returns the width and height of the current game window as a tuple. 
    Returns: (float, float)  
    """
    return

def screenWidth( ): 
    """Returns the width of the current game window. 
    Returns: float  
    """
    return

def server( ): 
    """This function returns the name of the server to which the client is currently
    connected. The name is identical to the string which would have been passed in as
    the first argument to the BigWorld.connect function to establish the connection. 
    Usually this is a string representation of an IP address. If the client is running
    in offline mode, then this is an empty string. Otherwise, if the client is neither
    connected to a server nor running in offline mode, then this returns None. 
    Returns: The name of the server to which the client is connected, an empty string 
    if the client is running in offline mode, or None if the client is not connected.  
    """
    return

def serverTime( ): 
    """This function returns the server time. This is the time that all entities are
    at, as far as the server itself is concerned. This is different from the value
    returned by the BigWorld.time() function, which is the on the client. 
    Returns: a float. This is the current time on the server.  
    """
    return

def setCursor( cursor ): 
    """Sets the active cursor. The active cursor will get all mouse, keyboard and
    axis events forwarded to it. Only one cursor can be active at a time. 
    Parameters: cursor  the new active cursor, or None to simply deactivate the current one.  
    """
    return

def setGraphicsSetting( label, optionIndex ): 
    """"Sets graphics setting option. 
    
    Raises a ValueError if the given label does not name a graphics setting, 
    if the option index is out of range, or if the option is not supported. 
    Parameters: label  string - label of setting to be adjusted.  
    optionIndex  int - index of option to set.  
    """
    return

def setTripleBuffering( doTripleBuffering ): 
    """Turns triple buffering on/off. 
    Parameters: doTripleBuffering  bool - True to turn triple buffering on. False to turn if off.  
    """
    return

def setVideoVSync( doVSync ): 
    """Turns vertical sync on/off. 
    Parameters: doVSync  bool - True to turn vertical sync on. False to turn if off.  
    """
    return

def setWatcher( path, val ): 
    """This function interacts with the watcher debugging system, allowing the setting of watcher variables. 
    
    The watcher debugging system allows various variables to be displayed and updated from within the runtime. 
    
    Watchers are specified in the C-code using the MF_WATCH macro. This takes three arguments, 
    the first is a string specifying the path (parent groups followed by the item name, separated
    by forward slashes), the second is the expression to watch. The third argument specifies 
    such things as whether it is read only or not, and has a default of read-write. 
    
    The setWatcher function in Python allows the setting of a particular read-write value. For example: 
    

    >>>	BigWorld.setWatcher( "Client Settings/Terrain/draw", 0 )
    
    
    
    Raises a TypeError if the watcher failed to be set. Raises a TypeError if args cannot be converted into a string. 
    Parameters: path  the path to the item to modify.  
    val  the new value for the item.  
    """
    return

def sinkKeyEvents( key, [optional] ): 
    """Adds a global event handler for the given key. The handler will exist up to and
    including the next "key up" event for the given key. This is useful if you process
    the key down event and want to stop all subsequent key events for a particular key 
    from occuring. For example, it is useful when the GUI state is changed in the GUI 
    component's handleKeyDown and you don't want the new GUI state to receive the 
    subsequent char or key up events (i.e. the user would have to fully let go of 
    the key and press it again before anything receives more events from that key). 
    
    The handler should be a class instance with methods "handleKeyEvent" and "handleCharEvent". 
    The handler methods should return True to override the event and stop it from being passed 
    to any other handleres, or False to allow the event to continue as per normal. 
    
    If no event handler is specified, then it will sink all events up to and including the next key-up event. 
    
    When the 
    Parameters: key  The key code to be routed to the sink.  
    [optional]  sink The class instance that will process the key events. If not specified, 
    it will sink all events for the given key up to and including the next key up.  
    """
    return

def spaceLoadStatus( (optional) ): 
    """This function queries the chunk loader to see how much of the current camera * 
    space has been loaded. It queries the chunk loader to see the distance * to the 
    currently loading chunk ( the chunk loader loads the closest chunks * first ). A 
    percentage is returned so that scripts can use the information * to create for example a teleportation progress bar. 
    Parameters: (optional)  distance The distance to check for. By default this is * set to the current far plane.  
    
    
    Returns: float Rough percentage of the loading status  
    """
    return

def spaceTimeOfDay( spaceID, time ): 
    """Gets and sets the time of day in 24 hour time, as used by the environment system. 
    Parameters: spaceID  The spaceID of the space to set/get the time.  
    time  Optional string. If provided, the time of day is set to this. This can 
    be in the format "hour:minute" (eg "01:00", "1:00", "1:0", "1:" ), or as a 
    string representation of a float (eg "1", "1.0"). Note that an incorrectly 
    formatted string will not throw an exception, nor will it change the time of day.  
    
    
    Returns: A string representing the time of day at the end of the function call in the form "hh:mm" (eg "01:00" ).  
    """
    return

def stime( ): 
    return

def stringToKey( string ): 
    """The 'stringToKey' method converts from the name of a key to its corresponding
    key index as used by the 'isKeyDown' method. The string names for a key can be
    found in the keys.py file. If the name supplied is not on the list defined, the
    value returned is zero, indicating an error. This method has a inverse method, 
    'keyToString' which does the exact opposite. 
    Parameters: string  A string argument containing the name of the key.  


    Returns: An integer value for the key with the supplied name.  
    """
    return

def sunlightController( p ): 
    """This function registers a vector4 provider to provide further control
    over the sunlight. It is interpreted as an (r,g,b,reserved) multiplier
    on the existing time-of-day sunlight colour. 
    Parameters: p  the Vector4Provider to set  
    """
    return

def target( ): 
    """BigWorld.target is actually an attribute that is callable. 
    See PyTarget.__call__ for more information. 
    """
    return

def time( ): 
    """This function returns the client time. This is the time that the player's
    entity is currently ticking at. Other entities are further back in time, 
    the further they are from the player entity. 
    Returns: a float. The time on the client.  
    """
    return

def timeOfDay( time ): 
    """Gets and sets the time of day in 24 hour time, as used by the environment system.
    If the camera is not currently in a space, then this function will not do anything,
    and will return an empty string. 
    
    This can also be changed manually via the following key combinations: 
    
    DEBUG + "[": Rewind time of day by 10 minutes 
    
    DEBUG + Shift + "[": Rewind time of day by 1 hour 
    
    DEBUG + "]": Advance time of day by 10 minutes 
    
    DEBUG + Shift + "]": Advance time of day by 1 hour 
    Parameters: time  Optional string. If provided, the time of day is set to this. This 
    can be in the format "hour:minute" (eg "01:00", "1:00", "1:0", "1:" ), or as a string
    representation of a float (eg "1", "1.0"). Note that an incorrectly formatted string 
    will not throw an exception, nor will it change the time of day.  
    
    
    Returns: A string representing the time of day at the end of the function call in
    the form "hh:mm" (eg "01:00" ). Returns an empty string if the camera is not currently in a space.  
    """
    return

def totalPhysicalMemory( ): 
    """Return the amount of physical RAM in the machine. This can help when a game 
    automatically chooses graphics settings for a user. There are a few graphics 
    settings that affect the amount of system RAM used. Two examples are: - texture 
    detail (managed textures are used, and they are mirrored in RAM) - the far plane 
    distance directly affects how many chunks are loaded and thus how many unique 
    assets will be loaded at any time. 
    Returns: Integer The amount of physical RAM, in bytes.  
    """
    return

def totalVirtualMemory( ): 
    """Return the amount of virtual memory, a.k.a the amount of address space available 
    to the process. Used in conjunction with the totalPhysicalMemory function, you should
    be able to determine whether or not the user running a 64 bit client and therefore 
    whether or not your game has access to the full amount of physical RAM installed. 

    This can help a game automatically choose graphics settings for a user. There are a
    few graphics settings that affect the amount of system RAM used. Two examples are: 
    - texture detail (managed textures are used, and they are mirrored in RAM) - the far
    plane distance directly affects how many chunks are loaded and thus how many unique 
    assets will be loaded at any time. 
    Returns: Integer The amount of virtual RAM, in bytes.  
    """
    return

def videoModeIndex( ): 
    """"Retrieves index of current video mode. 
    Returns: int Index of current video mode or zero if render context has not yet been initialised.  
    """
    return

def weather( spaceID ): 
    """This function returns the unique Weather object which is used
    to control the weather on the client. 
    Parameters: spaceID  [optional] The id of the space to retrieve
    the weather object from, or the current camera space if no argument is passed in.  
    

    Returns: the unique Weather object.  
    """
    return

def weatherController( p ): 
    """This function registers a vector4 provider to provide further
    control over the weather. It is interpreted as (extraRain, reserved, 
    reserved, reserved) The output of the provider sets various values on the weather. 
    Parameters: p  the Vector4Provider to set  
    """
    return

def windowSize( ): 
    """Returns size of application window when running in windowed mode.
    This is different to the screen resolution when running in fullscreen
    mode (use listVideoModes and videoModeIndex functions to get the screen resolution in fullscreen mode). 

    This function is deprecated. Use screenSize, instead. 
    Returns: 2-tuple of floats (width, height)  
    """
    return

def worldDrawEnabled( newValue ): 
    """Sets and gets the value of a flag used to control if the world is drawn. 
    Note: the value of this flag will also be used to turn the watching of files 
    being loaded in the main thread on or off. That is, if enabled, warnings 
    will be issued into the debug output whenever a file is accessed from the main thread. 
    Parameters: newValue  (optional) True enables the drawing of the world, False disables.  
    
    
    Returns: Bool If the no parameters are passed the current value of the flag is returned.  
    """
    return

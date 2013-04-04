#pragma once

/// @file IStateController.h interface for activating/de-activating states.

#include "types.h"

///@interface IStateController IStateController.h. 
///@brief  Defines the actions available to enable/disable states.
struct IStateController {
    /// Utility class to make it easier to enable/disable states and
    /// make code easier to understand
	struct StateEnabler {
        IStateController& sc_;
        StateEnabler( IStateController& sc ) : sc_( sc ) {}
        StateEnabler operator()( StateID sid, bool enable ) {
        	if( enable ) sc_.EnableState( sid );
        	else sc_.DisableState( sid );
        	return StateEnabler( *this );
        }
	};
    /// Enable state
    virtual void EnableState( StateID ) = 0;
    /// Disable state
    virtual void DisableState( StateID ) = 0;
    /// Enable all states
    virtual void EnableAllStates() = 0;
    /// IStateController* sc;
    /// ...
    /// sc->ChangeStates()
    ///         ( STATE_3, true  )
    ///         ( STATE_4, false )
    ///         ( STATE_5, false )
    ///         ( STATE_6 ,false );
    StateEnabler ChangeStates() {
    	return StateEnabler( *this );
    }
    virtual ~IStateController() {}
};


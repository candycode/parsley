#pragma once

/// @file IStateController.h interface for activating/de-activating states.

#include "types.h"

///@interface IStateController IStateController.h. Defines the actions available
///to enable/disable states.
struct IStateController {
	struct StateEnabler {
        IStateController& sc_;
        StateEnabler( IStateController& sc ) : sc_( sc ) {}
        StateEnabler operator()( StateID sid, bool enable ) {
        	if( enable ) sc_.EnableState( sid );
        	else sc_.DisableState( sid );
        	return StateEnabler( *this );
        }
	};
    virtual void EnableState( StateID ) = 0;
    virtual void DisableState( StateID ) = 0;
    virtual void EnableAllStates() = 0;
    StateEnabler ChangeStates() {
    	return StateEnabler( *this );
    }
    virtual ~IStateController() {}
};


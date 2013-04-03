#ifndef ISTATE_CONTROLLER_H_
#define ISTATE_CONTROLLER_H_

/// @file IStateController.h interface for activating/de-activating states.

#include "types.h"

///@interface IStateController IStateController.h. Defines the actions available
///to enable/disable states.
struct IStateController {
    virtual void EnableState( StateID ) = 0;
    virtual void DisableState( StateID ) = 0;
    virtual void EnableAllStates() = 0;
    virtual ~IStateController() {}
};

#endif //ISTATE_CONTROLLER_H_


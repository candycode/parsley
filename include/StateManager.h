#pragma once
////////////////////////////////////////////////////////////////////////////////
//Copyright (c) 2010-2013, Ugo Varetto
//All rights reserved.
//
//Redistribution and use in source and binary forms, with or without
//modification, are permitted provided that the following conditions are met:
//    * Redistributions of source code must retain the above copyright
//      notice, this list of conditions and the following disclaimer.
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//    * Neither the name of the copyright holder nor the
//      names of its contributors may be used to endorse or promote products
//      derived from this software without specific prior written permission.
//
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL UGO VARETTO BE LIABLE FOR ANY
//DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////

/// @file StateManager 

#include <list>
#include "Any.h"
#include "SmartPtr.h"

#include "types.h"

namespace parsley {
//------------------------------------------------------------------------------
/// @interface IStateManager StateManager.h State manager interface: handles 
/// parsed values, errors and enable/disable states of an IStateController 
/// instance. Methods of this interface are invoked by ParserManager::Apply 
/// during the parsing process.
struct IStateManager {
    /// Type of parsed values.
    typedef std::map< ValueID, Any > Values;
    /// Handle values for a specific state. Implementations of this method
    /// shall handle value storage and semantic validation end return @c true
    /// or @c false accordingly.
    /// @param sid state id of the current parsing state.
    /// @param values extracted values.
    /// @return @c true if values validated, @c false otherwise. The return 
    ///         value of this method s used by ParserManager::Apply to decide 
    ///         if parsing should stop or continue.
    virtual bool HandleValues( StateID sid, const Values& values ) = 0;
    /// Handle error condition. This method is called when an error condition 
    /// is detected by ParserManager::Apply.
    /// @param sid state identifier of state in which error condition found. 
    ///        Note that when using implementations of this interface with 
    ///        ParserManager the state id passed to this function is the 
    ///        identifier of the last state that was parsed correctly.
    /// @param lineNum line number at which parsed stopped.
    virtual void HandleError( StateID sid, int lineNum ) = 0;
    /// Returns copy of current object.
    virtual IStateManager* Clone() const = 0;
    /// Updates states to enable/disable specific transitions. 
    /// Invoked by ParserManager::Apply after a parser is found and successfully
    /// applied to a specific state.
    /// @param sc state controller reference.
    /// @param curState state identifier of current parsing state.
    virtual void UpdateState( IStateController& sc, 
                              StateID curState ) const = 0;
    /// (Required) virtual destructor.
    virtual ~IStateManager() {}
};

//------------------------------------------------------------------------------
/// @brief Implementation of IStateManager interface to support polymorphic 
/// value semantics through pImpl idiom.
/// @ingroup MainClasses
class StateManager : public IStateManager {
public:
    /// Default constructor.
    StateManager() {}
    /// Copy constructor: builds new copy of contained implementation.
    StateManager( const StateManager& v ) 
        : pImpl_( v.pImpl_ ? v.pImpl_->Clone() : 0 ) {}
    /// Swap.
    StateManager& Swap( StateManager& v ) { 
        parsley::Swap( pImpl_, v.pImpl_ ); return *this; 
    } 
    /// Assignment from StateManager.
    StateManager& operator=( const StateManager& v ) { 
        StateManager( v ).Swap( *this ); return *this;  
    }
    /// Assignment from IStateManager.
    StateManager( const IStateManager& v ) : pImpl_( v.Clone() ) {}
    /// Implementation of IStateManager::HandleValues.
    bool HandleValues( StateID sid,  const Values& v ) {
        CheckPointer();
        //will fail in release mode if NULL pointer exceptions off
        return pImpl_->HandleValues( sid, v ); 
    }
    /// Implementation of IStateManager::HandleError.
    void HandleError( StateID sid, int line ) {
        CheckPointer();
        //will fail in release mode if NULL pointer exceptions off
        pImpl_->HandleError( sid, line ); 
    }
    /// Implementation of IStateManager::Clone.
    StateManager* Clone() const { return new StateManager( *this ); }
    /// Implementation of IStateManager::UpdateState.
    void UpdateState( IStateController& sc, StateID sid ) const {
        assert( pImpl_ != 0 && "NULL StateManager pointer" );
        pImpl_->UpdateState( sc, sid );
    }
private:
    /// Check if pointer to implementation is @c NULL.
    bool CheckPointer() {
        assert( pImpl_ );
        if( !pImpl_ ) throw std::runtime_error( "NULL StateManager pointer" );
        if( !pImpl_ ) return false; //release mode, no exceptions
        return true;
    }
    /// Pointer to implementation.
    SmartPtr< IStateManager > pImpl_;
};

} //namespace

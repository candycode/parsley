#ifndef STATE_MANAGER_H_
#define STATE_MANAGER_H_

/// @file StateManager 

#include <list>
#include "Any.h"
#include "SmartPtr.h"

#include "types.h"

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
    /// @return @c true if values validated, @c false otherwise. The return value of this method
    /// is used by ParserManager::Apply to decide if parsing should stop or continue.
    virtual bool HandleValues( StateID sid, const Values& values ) = 0;
    /// Handle error condition. This method is called when an error condition is detected
    /// by ParserManager::Apply.
    /// @param sid state identifier of state in which error condition found. Note that
    /// when using implementations of this interface with ParserManager the state id
    /// passed to this function is the identifier of the last state that was parsed correctly.
    /// @param lineNum line number at which parsed stopped.
    virtual void HandleError( StateID sid, int lineNum ) = 0;
    /// Returns copy of current object.
    virtual IStateManager* Clone() const = 0;
    /// Updates states to enable/disable specific transitions. Invoked by ParserManager::Apply after
    /// a parser is found and successfully applied to a specific state.
    /// @param sc state controller reference.
    /// @param curState state identifier of current parsing state.
    virtual void UpdateState( IStateController& sc, StateID curState ) const = 0;
    /// (Required) virtual destructor.
    virtual ~IStateManager() {}
};

//------------------------------------------------------------------------------
/// @brief Implementation of IStateManager interface to support polymorphic value semantics
/// through pImpl idiom.
/// @ingroup MainClasses
class StateManager : public IStateManager {
public:
    /// Default constructor.
    StateManager() {}
    /// Copy constructor: builds new copy of contained implementation.
    StateManager( const StateManager& v ) : pImpl_( v.pImpl_ ? v.pImpl_->Clone() : 0 ) {}
    /// Swap.
    StateManager& Swap( StateManager& v ) { ::Swap( pImpl_, v.pImpl_ ); return *this; } 
    /// Assignment from StateManager.
    StateManager& operator=( const StateManager& v ) { StateManager( v ).Swap( *this ); return *this;  }
    /// Assignment from IStateManager.
    StateManager( const IStateManager& v ) : pImpl_( v.Clone() ) {}
    /// Implementation of IStateManager::HandleValues.
    bool HandleValues( StateID sid,  const Values& v ) {
        CheckPointer();
        return pImpl_->HandleValues( sid, v ); //will fail in release mode if NULL pointer exceptions off
    }
    /// Implementation of IStateManager::HandleError.
    void HandleError( StateID sid, int line ) {
        CheckPointer();
        pImpl_->HandleError( sid, line ); //will fail in release mode if NULL pointer exceptions off
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

//template < class DerivedT >
//class TableStateManager : public IStateManager
// {
//public:
//  typedef bool ( DerivedT::*StateManagerMethod )( StateID, const Values& );
//  typedef bool ( DerivedT::*ErrorHandlerMethod )( StateID );
//  bool HandleValues( StateID id, const Values& v )
// {
//      return ( this->* )valueHandlers_[ id ]( id, v );
//  }
//  bool HandleError( StateID id )
// {
//      return ( this->* )errorHandlers_[ id ]( id );
//  }
//  bool SetStateManagerMethod( StateID id, StateManagerMethod hm )
// {
//      bool replaced = false;
//      if( valueHandlers_.find( id ) != valueHandlers_.end() ) replaced = true;
//      valueHandlers_[ id ] = hm;
//      return replaced;
//  }
//  bool SetErrorHandlerMethod( StateID id, ErrorHandlerMethod hm )
// {
//      bool replaced = false;
//      if( errorHandlers_.find( id ) != errorHandlers_.end() ) replaced = true;
//      errorHandlers_[ id ] = hm;
//      return replaced;
//  }
//};


#endif //STATE_MANAGER_H_


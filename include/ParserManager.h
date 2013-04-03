#ifndef PARSER_MANAGER_H_
#define PARSER_MANAGER_H_

/// @file ParserManager implementation of state machine for selecting
/// parser depending on state.

#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <string>

#include "types.h"

#include "Parser.h"

#include "IStateController.h"

#include "StateManager.h"

struct ITransitionCBack {
    void operator()( const Values& v, StateID from, StateID to ) { Apply( v, from, to ); }
    virtual void Apply( const Values&, StateID, StateID ) = 0;
    virtual ITransitionCBack* Clone() const = 0;
    virtual ~ITransitionCBack() {}
#ifdef USE_TRANSITION_CBACKS
    virtual bool Enable( const prevValues&, StateID, StateID ) = 0;
    virtual bool Validate( const prevValues&,  StateID, StateID ) = 0;
    virtual void OnError( StateID, StateID, int lines ) = 0;
#endif        
};

class TransitionCBack : public ITransitionCBack {
public:
    TransitionCBack() {}
    TransitionCBack( const TransitionCBack& other ) : pImpl_( other.pImpl_ ? other.pImpl_->Clone() : 0 ) {}
    TransitionCBack( ITransitionCBack* ptr ) : pImpl_( ptr ) {}
    TransitionCBack& Swap( TransitionCBack& v ) { ::Swap( pImpl_, v.pImpl_ ); return *this; } 
    TransitionCBack& operator=( const TransitionCBack& v ) { TransitionCBack( v ).Swap( *this ); return *this;  }
    void Apply( const Values& v, StateID from, StateID to) { 
        if( pImpl_ ) pImpl_->Apply( v, from, to );
    }
    TransitionCBack* Clone() const { return new TransitionCBack( *this ); }
#ifdef USE_TRANSITION_CBACKS
    bool Enable( const prevValues& pv, StateID cur, StateID next ) { return pImpl_->Enable( pv, cur, next ); }
    bool Validate( const prevValues&,  StateID prev, StateID cur ) { return pImpl_->Validate( pv, prev, cur ); }
    void OnError( StateID prev, StateID cur, int lines ) { pImpl_->OnError( prev, cur, lines ); }
#endif        
private:
    SmartPtr< ITransitionCBack > pImpl_;
};


//------------------------------------------------------------------------------
/// @brief State machine implementation.
///
/// Each state is matched by a specific parser.
/// At each invocation of the ParserManager::Apply method
/// the first parser to match one of the possible next states
/// is used to identify the next current state and
/// StateManager::HandleValue is invoked passing to it the extracted
/// values and the state id of the matched state.
/// @ingroup MainClasses
class ParserManager : public IStateController
{
    typedef std::list< StateID > States;
    typedef std::map< StateID, States > StateMap;
    typedef std::map< StateID, Parser > StateParserMap;
    typedef std::map< ParserID, StateID > ParserStateMap;
    typedef std::set< StateID > DisabledStates;
    typedef std::map< StateID, 
                      std::map< StateID, TransitionCBack > > TransitionCBackMap;

public:
    /// Constructor.
    /// @param sm state manager to handle parsed values.
    ParserManager( const StateManager& sm ) : stateManager_( sm ) {}
    /// Default constructor.
    ParserManager()  {}
    /// Links a state to a parser.
    /// @param sid state id.
    /// @param p parser.
    void SetParser( StateID sid, const Parser& p ) { stateParserMap_[ sid ] = p; }

    class ParserAdder {
        ParserManager& pm_;
    public:          
        ParserAdder( ParserManager& pm ) : pm_( pm ) {}
        ParserAdder operator()( StateID state, const Parser& p ) {
            pm_.SetParser( state, p );
            return ParserAdder( *this );
        }
    };
    ParserAdder SetParsers() { return ParserAdder( *this ); }
    /// Adds a new state.
    /// @param s state id.
    void AddState( StateID s )
    {
        if( stateMap_.find( s ) != stateMap_.end() ) 
        {
            throw std::logic_error( "State id already present" );
        }
        else stateMap_[ s ] = States();
    }
    void SetTransitionCBack( StateID prev, StateID next, const TransitionCBack& cback ) {
        transitionCBack_[ prev ][ next ] = cback;
    }
    /// Adds a transition from a state to another. If the previous state
    /// doesn't exist it is automatically added.
    /// Each state can have an unspecified number of next possible states.
    /// The transition is determined  by testing the all the parsers associated
    /// to each state, stopping at the firs parser that validates the input.
    /// @param prev previous state id
    /// @param next next state id
    /// @return reference to @c *this.
    ParserManager& AddState( StateID prev, StateID next )
    {
        if( stateMap_.find( prev ) == stateMap_.end() ) AddState( prev );
        stateMap_[ prev ].push_back( next );
        return *this;
    }
private:
    template < StateID invalidState >
    class StateAdder {
        ParserManager& pm_;
        StateID prevState_;
        StateID nextState_;
    public:          
        StateAdder( ParserManager& pm ) : pm_( pm ), prevState_( invalidState ), nextState_( invalidState ) {}
        StateAdder operator()( StateID prev, StateID next ) {
            pm_.AddState( prev, next );
            prevState_ = prev;
            nextState_ = next;
            return StateAdder( *this );
        }
        StateAdder operator()( StateID prev, 
                               StateID next1, 
                               StateID next2,
                               StateID next3  = invalidState,
                               StateID next4  = invalidState,
                               StateID next5  = invalidState,
                               StateID next6  = invalidState,
                               StateID next7  = invalidState,
                               StateID next8  = invalidState,
                               StateID next9  = invalidState,
                               StateID next10 = invalidState ) {
            pm_.AddState( prev, next1 );
            pm_.AddState( prev, next2 );
            if( next3  != invalidState ) pm_.AddState( prev, next3 );
            if( next4  != invalidState ) pm_.AddState( prev, next4 );
            if( next5  != invalidState ) pm_.AddState( prev, next5 );
            if( next6  != invalidState ) pm_.AddState( prev, next6 );
            if( next7  != invalidState ) pm_.AddState( prev, next6 );
            if( next8  != invalidState ) pm_.AddState( prev, next6 );
            if( next9  != invalidState ) pm_.AddState( prev, next6 );
            if( next10 != invalidState ) pm_.AddState( prev, next6 );
            return StateAdder( *this );
        }
        StateAdder operator[]( const TransitionCBack& cback ) {
            if( prevState_ != invalidState && nextState_ != invalidState ) {
                pm_.SetTransitionCBack( prevState_, nextState_, cback );
                prevState_ = invalidState;
                nextState_ = invalidState;
            }
            return StateAdder( *this );
        }
    };
public:
    template < StateID sid >
    StateAdder< sid > AddStates() {
        return StateAdder< sid >( *this );
    }
    /// Set initial state.
    void SetInitialState( StateID sid )
    { 
        if( stateMap_.find( sid ) == stateMap_.end() ) stateMap_[ sid ] = States();
        curState_ = sid;
    }
    /// Set last state. 
    void SetEndState( StateID sid )
    {
        if( stateMap_.find( sid ) == stateMap_.end() ) stateMap_[ sid ] = States();
        endState_ = sid;
    }
    /// Convenience method to set begin and end states in one call.
    void SetBeginEndStates( StateID begin, StateID end )
    {
        SetInitialState( begin );
        SetEndState( end );
    }
    /// Given a current state, it iterates over the list of possivle subsequent states
    /// and applies the corresponding parsers until a validating parser
    /// is found or returns @c false if no parser/state found.
    /// After a (parser,state) pair is found the StateManager::HandleValues method
    /// and StateManager::UpdateState are invoked to handle values and optionally
    /// enable/disable states depending on the current state and parsed values;
    /// unless the new current state is a terminal state the method is recursively
    /// invoked passing to it the new current state.
    /// @param is input stream.
    /// @param curState current state.
    /// @return @c true if a new state has been selected or the termination state reached,
    ///         @c false otherwise.
    bool Apply( InStream& is, StateID curState
#ifdef USE_TRANSITION_CBACKS
        , const Values& prevValues)
#endif     
        )
    {
#ifndef USE_TRANSITION_CBACKS // use transition cbacks instead of state manager        
        //if( is.fail() || is.bad() ) return false;
        const States& states = stateMap_[ curState ];
        ///@todo investigate the option of supporting hierarchical state controllers
        ///allowing to assign state conteollers to specific states instead of parsers
        /// e.g. 
        /// <code>
        /// if( states.empty() ) {
        ///   controller = controllers_[ curstate ];
        ///   controller.Apply( is, curstate );
        /// </code> 
        if( curState == endState_ || states.empty() ) return true;
        const StateID prevState = curState;
        States::const_iterator i;
        bool validated = false;
        for( i = states.begin(); i != states.end(); ++i )
        {
            if( disabledStates_.count( *i ) > 0 ) continue;
            Parser& l = stateParserMap_[ *i ];
            if( l.Parse( is ) )
            {
                curState = *i;
                if( !stateManager_.HandleValues( *i, l.GetValues() ) )
                {
                    validated = false;
                    break;
                }
                else 
                {
                    validated = true;
                    stateManager_.UpdateState( *this, curState  );
                    if( !transitionCBack_.empty() 
                        && transitionCBack_.find( prevState ) != transitionCBack_.end() ) {
                        if( transitionCBack_[ prevState ].find( curState ) != transitionCBack_[ prevState ].end() ) {
                            transitionCBack_[ prevState ][ curState ].Apply( l.GetValues(), prevState, curState );
                        }
                    }
                    break;
                }
            }
        }
        if( i == states.end() || !validated )
        {
            stateManager_.HandleError( curState, is.get_lines() );
            return false;
        }
        return Apply( is, curState );
#else
// use transition cbacks instead of state manager 
       
        SharedState sstate;
        TransitionCBack state1ToState2( new S1S2( sstate ) );
        TransitionCBack state2ToState3( new S1S3( sstate ) );

        const States& states = stateMap_[ curState ];
        if( curState == endState_ || states.empty() ) return true;
        const StateID prevState = curState;
        States::const_iterator i;
        for( i = states.begin(); i != states.end(); ++i )
        {
            curState = *i;
            TransitionCBack& tcback = transitionCBack_[ prevState ][ curState ];
            if( !tback
                .Enabled( prevValues, prevState, curState ) ) continue;
            Parser& l = stateParserMap_[ curState ];     
            if( l.Parse( is ) )
            {   
                if( !tback.Validate( l.GetValues(), prevState, curState ) )
                {
                    validated = false;
                    break;
                }
                else 
                {
                    validated = true; 
                    tcback.Apply( l.GetValues(), prevState, curState );                
                }
                break;
            }
        }
        if( i == states.end() || !validated )
        {
            tcback.OnError( prevState, curState, is.get_lines() );
            return false;
        }
        return Apply( is, curState, prevValues );

#endif        
    }
    /// Set state manger.
    void SetManager( const StateManager& sm ) { stateManager_ = sm; }
    /// IStateController::DisableState implementation.
    /// Adds a state to the set of disabled states which are then not considered
    /// for possible transitions in the ParserManager::Apply method.
    void DisableState( StateID id )
    {
        disabledStates_.insert( id );
    }
    /// IStateController::EnableState implementation.
    /// Removes the state from the set of disabled states making it available
    /// againg as a target for possible transitions.
    void EnableState( StateID id )
    {
        if( disabledStates_.count( id ) == 0 ) return;
        disabledStates_.erase( id );
    }
    /// IStateController::EnableAllStates implementation.
    /// Empties the set of disabled states.
    void EnableAllStates()
    {
        disabledStates_.clear();
    }

private:
    /// Current State; 0 -> invalid state
    StateID curState_;
    /// Maps state to list of possible next states
    StateMap stateMap_;
    /// Maps state to parser
    StateParserMap stateParserMap_;
    /// Maps parser id (string) to state id
    ParserStateMap parserStateMap_;
    /// StateID counter
    mutable StateID stateIDCounter_;
    /// Disabled states
    DisabledStates disabledStates_;
    /// End state
    StateID endState_;
#ifndef USE_TRANSITION_CBACKS    
    /// State manager. Its methods are invoked from withing the
    /// ParserManager::Apply method to handle parsed values and enable/disable
    /// states depending on specific state and state values.
    StateManager stateManager_;
#endif    
    /// Transition callback
    TransitionCBackMap transitionCBack_;
};


#endif //PARSER_MANAGER_H_


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

/// @file ParserManager implementation of state machine for selecting
/// parser depending on state.

#include <list>
#include <map>
#include <set>
#include <stdexcept>
#include <string>
#include <sstream>

#include "types.h"

#include "Parser.h"

#include "IStateController.h"

#include "StateManager.h"

namespace parsley {

/// @brief Interface for transition callbacks functors invoked each time
/// a transition takes place. 
/// In case @c USE_TRANSITION_CBACKS is @c #defined then transition 
/// callbacks are used to validate and enable/disable transitions 
/// in place of IStateManager
struct ITransitionCBack {
    /// Invoke Apply method
    void operator()( const Values& v, StateID prev, StateID cur ) { 
        Apply( v, prev, cur ); 
    }
    /// Invoked whenever a state transition take place.
    /// @param values values parsed in @c current state
    /// @param prev source state
    /// @param cur current state
    virtual void Apply( const Values& values, StateID prev, StateID cur ) = 0;
    /// Crete copy of current instance
    virtual ITransitionCBack* Clone() const = 0;
    /// Virtual destructor to allow overrides
    virtual ~ITransitionCBack() {}
#ifdef USE_TRANSITION_CBACKS
    /// Returns @c true if transition enabled; @c false otherwise
    /// @param prevValues values parsed in @c from state
    /// @param from source state
    /// @param to target state
    /// @return @c true if transition enabled; @c false otherwise
    virtual bool Enabled( const Values& prevValues, 
                          StateID from , StateID to ) const = 0;
    /// Validation: given parsed values, previous state and current
    /// state return @c true or @c false
    /// @param values values parsed in current state
    /// @param prev previous state
    /// @param cur current state 
    /// @return @c true if state valid; @c false otherwise
    virtual bool Validate( const Values& values, 
                           StateID prev, StateID cur ) const = 0;
    /// Invoked when a parsing error occurs
    /// @param from source state
    /// @param to target state
    /// @param lines error line
    virtual void OnError( StateID from, StateID to, int lines ) = 0;
#endif        
};

/// @brief Convenience class with default implementations of virtual methods
struct TransitionCBackDefault : ITransitionCBack {
    virtual void Apply( const Values&, StateID, StateID ) {}
#ifdef USE_TRANSITION_CBACKS
    virtual bool Enabled( const Values&, StateID, StateID ) const {
        return true;
    }
    virtual bool Validate( const Values&,  StateID, StateID ) const {
        return true;
    }
    virtual void OnError( StateID , StateID cur, int lineno ) {
        std::ostringstream oss;
        oss << "\n\t[" << cur << "] PARSER ERROR AT LINE " 
                  << lineno << std::endl;
        std::cerr << oss.str() << std::endl;          
        throw std::logic_error( oss.str() );          
    }
#endif       
};

/// @brief Value semantics through pImpl idiom. 
/// Instances of this class
/// hold a reference to an ITransitionCBack implementation to which
/// methodds are forwarded. Copy and assignment operators are supported
/// through the Clone() method.
class TransitionCBack : public ITransitionCBack {
public:
    /// Default constructor; required to initialize data inside STL
    /// data structures.
    TransitionCBack() {}
    TransitionCBack( const TransitionCBack& other ) 
        : pImpl_( other.pImpl_ ? other.pImpl_->Clone() : 0 ) {}
    TransitionCBack( ITransitionCBack* ptr ) : pImpl_( ptr ) {}
    TransitionCBack& Swap( TransitionCBack& v ) { 
        parsley::Swap( pImpl_, v.pImpl_ ); return *this; 
    } 
    TransitionCBack& operator=( const TransitionCBack& v ) { 
        TransitionCBack( v ).Swap( *this ); return *this;  
    }
    void Apply( const Values& v, StateID from, StateID to) {
        if( pImpl_ ) pImpl_->Apply( v, from, to );
    }
    TransitionCBack* Clone() const { return new TransitionCBack( *this ); }
#ifdef USE_TRANSITION_CBACKS
    bool Enabled( const Values& pv, StateID cur, StateID next ) const { 
        return pImpl_->Enabled( pv, cur, next ); 
    }
    bool Validate( const Values& pv,  StateID prev, StateID cur ) const { 
        return pImpl_->Validate( pv, prev, cur ); 
    }
    void OnError( StateID prev, StateID cur, int lines ) { 
        pImpl_->OnError( prev, cur, lines ); 
    }
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
class ParserManager : public IStateController {
    typedef std::list< StateID > States;
    typedef std::map< StateID, States > StateMap;
    typedef std::map< StateID, Parser > StateParserMap;
    typedef std::map< ParserID, StateID > ParserStateMap;
    typedef std::set< StateID > DisabledStates;
    typedef std::map< StateID, 
                      std::map< StateID, TransitionCBack > > TransitionCBackMap;

public:
#ifndef USE_TRANSITION_CBACKS    
    /// Constructor.
    /// @param sm state manager to handle parsed values.
    ParserManager( const StateManager& sm ) : stateManager_( sm ) {}
#endif    
    /// Default constructor.
    //ParserManager()  {}
    /// Links a state to a parser.
    /// @param sid state id.
    /// @param p parser.
    void SetParser( StateID sid, const Parser& p ) { 
        stateParserMap_[ sid ] = p; 
    }
private:    
    /// Utility class to simplify the addition of parsers.
    /// usage:
    /// <code>
    /// ParserManager pM;
    /// pM.SetParsers()
    ///    ( STATE_1, State1Parser )
    ///    ( STATE_2, State2Parser ); 
    /// </code>
    class ParserAdder {
        ParserManager& pm_;
    public:
        /// Constructor receiving a reference to parser manager
        /// to operate on          
        ParserAdder( ParserManager& pm ) : pm_( pm ) {}
        /// Add parser to parser manager
        /// @param state state
        /// @param p parser associated to state @c state
        ParserAdder operator()( StateID state, const Parser& p ) {
            pm_.SetParser( state, p );
            return ParserAdder( *this );
        }
    };
public:    
    /// Utility method to make the addition of parsers easier
    /// and make code easier to understand.
    /// usage:
    /// ParserManager pM;
    /// pM.SetParsers()
    ///    ( STATE_1, State1Parser )
    ///    ( STATE_2, State2Parser ); 
    /// </code>
    ParserAdder SetParsers() { return ParserAdder( *this ); }
    /// Adds a new state.
    /// @param s state id.
    void AddState( StateID s ) {
        if( stateMap_.find( s ) != stateMap_.end() ) {
            throw std::logic_error( "State id already present" );
        }
        else stateMap_[ s ] = States();
    }
    /// Assign transition callback to state transition
    /// @param prev source state
    /// @param next target state
    /// @param cback transition callback
    void SetTransitionCBack( StateID prev, StateID next, 
                             const TransitionCBack& cback ) {
        transitionCBack_[ prev ][ next ] = cback;
    }
    /// Set callback to all transitions from any state to target state
    /// @param target target state: all transitions from any states to
    ///               this state will invoke the same callback type
    /// @param cback transition callback
    void SetTargetTransitionCBack( StateID target, 
                                   const TransitionCBack& cback ) {
        for( StateMap::const_iterator i = stateMap_.begin(); 
             i != stateMap_.end(); 
             ++i ) {
            for( States::const_iterator s = i->second.begin();
                 s != i->second.end(); ++s ) {
                if( *s == target ) {
                    SetTransitionCBack( i->first, *s, cback );
                }
            }
        }
    }
    /// Set callback from all source transitions to any state
    /// @param src source state: all transitions from this state to
    ///            any other state will invoke the same callback type
    /// @param cback transition callback
    void SetSourceTransitionCBack( StateID src, 
                                   const TransitionCBack& cback ) {
        for( StateMap::const_iterator i = stateMap_.begin(); 
             i != stateMap_.end(); 
             ++i ) {
            if( src != i->first ) continue;
            for( States::const_iterator s = i->second.begin();
                 s != i->second.end(); ++s ) {
                SetTransitionCBack( i->first, *s, cback );
            }
        }
    }
    /// Set same transition cback to all transitions
    /// @param cback transition callback
    void SetAllTransitionsCBack( const TransitionCBack& cback ) {
        for( StateMap::const_iterator i = stateMap_.begin(); 
             i != stateMap_.end(); 
             ++i ) SetSourceTransitionCBack( i->first, cback );   
    }
    /// Adds a transition from a state to another. If the previous state
    /// doesn't exist it is automatically added.
    /// Each state can have an unspecified number of next possible states.
    /// The transition is determined  by testing the all the parsers associated
    /// to each state, stopping at the firs parser that validates the input.
    /// @param prev previous state id
    /// @param next next state id
    /// @return reference to @c *this.
    ParserManager& AddState( StateID prev, StateID next ) {
        if( stateMap_.find( prev ) == stateMap_.end() ) AddState( prev );
        stateMap_[ prev ].push_back( next );
        return *this;
    }
    // void MoveTo( InStream& is,  StateID sid, bool rewind = false ) {
    //     if( rewind ) is.seekg( 0 );
    //     streampos spos = is.tellg();
    //     while( !is.eof() && !Apply( is, sid ) ) {
    //         spos = is.tellg();
    //         is.get();
    //     }
    //     if( !is.eof() && is.good() ) {
    //         is.seekg( 0 );
    //         is.seekg( spos );
    //     }
    // }
    // void MoveTo( InStream& is, const Parser& p, bool rewind = false ) {
    //     if( rewind ) is.seekg( 0 );
    //     while( !is.eof() && ! p.Parse( is ) ) is.get();
    // }
private:    
    /// @brief Utility class to make adding transition callbacks easier and 
    /// make code easier to understand.
    class CBackSetter {
        ParserManager& pm_;
    public:
        /// Constructor accepting a reference to the parser manager to operate
        /// on   
        CBackSetter( ParserManager& pm ) : pm_( pm ) {}
        /// Assign transition callback to all transitions with a the same
        /// target state
        /// @param targetState transition target:
        ///        invokes ParserManager::SetTargetTransitionCBack
        /// @param cback transition callback
        CBackSetter operator()( StateID targetState, 
                               const TransitionCBack& cback ) {
            pm_.SetTargetTransitionCBack( targetState, cback );
            return CBackSetter( *this );
        }
        /// Assign transition callback to transition
        /// @param sourceState source state
        /// @param targetState target state
        /// @param cback transition callback
        CBackSetter operator()( StateID sourceState, StateID targetState, 
                                const TransitionCBack& cback ) {
            pm_.SetTransitionCBack( sourceState, targetState, cback );
            return CBackSetter( *this );
        }  
    };
public:    
    /// Utility method to simplify adding transition callbacks through
    /// CBackSetter instances.
    /// usage:
    /// <code>
    /// State sharedState;
    /// ParserManager pM;
    /// pM.SetCBacks()
    ///     ( aSTATE, new aSTATECBack( &sharedState ) );
    /// </code>
    /// @return CBackSetter instance
    CBackSetter SetCBacks() { return CBackSetter( *this ); }
private:
    /// @brief Utility class to make adding states easier and code easier to
    /// understand.
    /// @tparam invalidState invalid state id
    template < StateID invalidState >
    class StateAdder {
        ParserManager& pm_;
        StateID prevState_;
        StateID nextState_;
    public:
        /// Constructor accepting a parser manger reference          
        StateAdder( ParserManager& pm ) 
            : pm_( pm ), 
              prevState_( invalidState ), 
              nextState_( invalidState ) {}
        /// Add transition
        /// @param prev source state
        /// @param next target state
        StateAdder operator()( StateID prev, StateID next ) {
            pm_.AddState( prev, next );
            prevState_ = prev;
            nextState_ = next;
            return StateAdder( *this );
        }
        /// Add transition from previously specified source state to target
        /// state
        /// @param sid target state
        StateAdder operator()( StateID sid ) {
            prevState_ = invalidState;
            nextState_ = sid;
            return StateAdder( *this );
        }
        /// Add transitions from source state to a set of target states
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
        /// Add transition callback while adding state
        StateAdder operator[]( const TransitionCBack& cback ) {
            if( prevState_ != invalidState && nextState_ != invalidState ) {
                pm_.SetTransitionCBack( prevState_, nextState_, cback );
            } else if( nextState_ != invalidState ) {
                pm_.SetTargetTransitionCBack( nextState_, cback );
            }
            prevState_ = invalidState;
            nextState_ = invalidState;
            return StateAdder( *this );
        }
    };
public:
    /// Utility function to add states through StateAdder instances
    /// usage:
    /// <code>
    /// enum { INVALID_STATE, START, STATE_1, ... };
    /// ParserManager pM;
    /// pM.AddStates< INVALID_STATE >()
    ///    ( START, // from
    ///          STATE_1 ) // to 
    ///    ( STATE_1, // from
    ///          STATE_2, END_STATE ) // to any of these
    /// </code>
    template < StateID sid >
    StateAdder< sid > AddStates() {
        return StateAdder< sid >( *this );
    }
    /// Set initial state.
    void SetInitialState( StateID sid ) {
        if( stateMap_.find( sid ) == stateMap_.end() ) {
            stateMap_[ sid ] = States();
        }
        curState_ = sid;
    }
    /// Set last state. 
    void SetEndState( StateID sid ) {
        if( stateMap_.find( sid ) == stateMap_.end() ) { 
            stateMap_[ sid ] = States();
        }
        endState_ = sid;
    }
    /// Convenience method to set begin and end states in one call.
    void SetBeginEndStates( StateID begin, StateID end ) {
        SetInitialState( begin );
        SetEndState( end );
    }
    /// Given a current state, it iterates over the list of possivle subsequent 
    /// states and applies the corresponding parsers until a validating parser
    /// is found or returns @c false if no parser/state found.
    /// After a (parser,state) pair is found the StateManager::HandleValues 
    /// method and StateManager::UpdateState are invoked to handle values and 
    /// optionally enable/disable states depending on the current state and 
    /// parsed values; unless the new current state is a terminal state the 
    /// method is recursively invoked passing to it the new current state.
    /// @param is input stream.
    /// @param curState current state.
    /// @return @c true if a new state has been selected or the termination 
    ///         state reached, @c false otherwise.
    bool Apply( InStream& is, StateID curState
#ifdef USE_TRANSITION_CBACKS
        , const Values& prevValues = Values()
#endif     
        ) {
#ifndef USE_TRANSITION_CBACKS // use transition cbacks instead of state manager        
        //if( is.fail() || is.bad() ) return false;
        const States& states = stateMap_[ curState ];
        ///@todo investigate the option of supporting hierarchical state 
        ///controllers allowing to assign state conteollers to specific states 
        ///instead of parsers
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
        for( i = states.begin(); i != states.end(); ++i ) {
            if( disabledStates_.count( *i ) > 0 ) continue;
            Parser& l = stateParserMap_[ *i ];
            if( l.Parse( is ) ) {
                curState = *i;
                if( !stateManager_.HandleValues( *i, l.GetValues() ) ) {
                    validated = false;
                    break;
                }
                else {
                    validated = true;
                    stateManager_.UpdateState( *this, curState  );
                    if( !transitionCBack_.empty() 
                        && transitionCBack_.find( prevState ) != 
                           transitionCBack_.end() ) {
                        if( transitionCBack_[ prevState ].find( curState ) 
                            != transitionCBack_[ prevState ].end() ) {
                            transitionCBack_[ prevState ][ curState ]
                            .Apply( l.GetValues(), prevState, curState );
                        }
                    }
                    break;
                }
            }
        }
        if( i == states.end() || !validated ) {
            stateManager_.HandleError( curState, is.get_lines() );
            return false;
        }
        return Apply( is, curState );
#else
// use transition cbacks instead of state manager 
        const States& states = stateMap_[ curState ];
        if( curState == endState_ || states.empty() ) return true;
        const StateID prevState = curState;
        States::const_iterator i;
        bool validated = false;
        TransitionCBack* tcback = 0;
        StateID nextState = curState;
        for( i = states.begin(); i != states.end(); ++i ) {
            nextState = *i;
            if( !TransitionCBackExists( prevState, nextState ) ) {
                std::ostringstream os;
                os << "ERROR: No transition between " << prevState  << " and "
                   << nextState; 
                throw std::logic_error( os.str() );
            }
            tcback = &transitionCBack_[ prevState ][ nextState ];
            if( !tcback
                  ->Enabled( prevValues, prevState, nextState ) ) continue;
            Parser& l = stateParserMap_[ nextState ];     
            if( l.Parse( is ) ) {
                if( !tcback->Validate( l.GetValues(), prevState, nextState ) ) {
                    validated = false;
                    break;
                }
                else {
                    validated = true; 
                    tcback->Apply( l.GetValues(), prevState, nextState );                
                }
                break;
            }
        }
        if( i == states.end() || !validated ) {
            tcback->OnError( prevState, nextState, is.get_lines() );
            return false;
        }
        return Apply( is, nextState, prevValues );

#endif        
    }
#ifndef USE_TRANSITION_CBACKS    
    /// Set state manger.
    void SetManager( const StateManager& sm ) { stateManager_ = sm; }
#endif    
    /// IStateController::DisableState implementation.
    /// Adds a state to the set of disabled states which are then not considered
    /// for possible transitions in the ParserManager::Apply method.
    void DisableState( StateID id ) {
        disabledStates_.insert( id );
    }
    /// IStateController::EnableState implementation.
    /// Removes the state from the set of disabled states making it available
    /// againg as a target for possible transitions.
    void EnableState( StateID id ) {
        if( disabledStates_.count( id ) == 0 ) return;
        disabledStates_.erase( id );
    }
    /// IStateController::EnableAllStates implementation.
    /// Empties the set of disabled states.
    void EnableAllStates() {
        disabledStates_.clear();
    }
    /// Check if transition between two states exists.
    /// @param from source state
    /// @param to target state
    bool TransitionCBackExists( StateID from, StateID to ) {
        return transitionCBack_.find( from ) != transitionCBack_.end() 
               && transitionCBack_.find( from )->second.find( to ) !=
                  transitionCBack_.find( to )->second.end(); 
    }
private:
    /// Current State
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

} //namespace

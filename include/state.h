#pragma once
////////////////////////////////////////////////////////////////////////////////
//Parsley - parsing framework
//Copyright (c) 2010-2015, Ugo Varetto
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
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL UGO VARETTO BE LIABLE FOR ANY
//DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////

#include <unordered_map>
#include <memory>
#include <cassert>
#include <functional>

#include "InStream.h"
#include "Parser.h"

namespace parsley {

//----------------------------------------------------------------------------
// State machine
//----------------------------------------------------------------------------
template < typename InStreamT,
           typename StateMapT,
           typename ParserMapT,
           typename ActionMapT >
struct IState {
    using InStream = InStreamT;
    using StateMap = StateMapT;
    using ParserMap = ParserMapT;
    using Parser = typename ParserMap::value_type::second_type;
    using ActionMap = ActionMapT;
    using Action = typename ActionMap::value_type::second_type;
    using ID = StateMap::value_type::first_type;
    virtual bool Eval(InStream& is, 
                      ActionMap& c, 
                      StateMap& map,
                      ParserMap& parsers) = 0;
    virtual IState* Clone() = 0;
    virtual ~IState() {}
};

template < typename MapT >
bool Exists(const MapT& m, 
            const typename MapT::value_type::first_type& key) {
#ifdef CHECK_KEY
    return m.find(key) != m.end();
#endif
}


class State {
public:
    template < typename T >
    State(const T& impl) : pImpl_(new Impl< T >(impl)) {}
    template < typename T >
    State(T&& impl) : pImpl_(new Impl< T >(std::forward(impl))) {}
    /// Copy constructor.
    State(const State& s) : pImpl_(s.pImpl_->Clone()) {}
    State(State&& s) = default;
    ///Evaluate state.
    template < typename InStreamT,
               typename StateMapT, 
               typename ParserMap,
               typename ActionMap>
    bool Eval(InStream& is, 
              StateMapT& states, 
              ParserMapT& parsers,
              ActionMap& actions) {
        assert(pImpl_);
        return pImpl_->Eval(is, states, parsers, actions); 
    }
private:
    struct Instance {
        template < typename InStream,
                   typename StateMap,
                   typename ParserMap,
                   typename ActionMap >
        bool Eval(InStream& is, StateMap& sm, ParserMap& pm, ActionMap& am) const {
            using S = IState< InStream,
                              StateMap,
                              ParserMap,
                              ActionMap >;
            return static_cast< const Impl< S >* >(this)->impl_.Eval(is, sm, pm, am);                  
        } 
        virtual Instance* Clone() const = 0;
    };
    template < typename T >
    struct Impl : Instance {
        Impl(const T& impl) : impl_(impl) {}
        Impl(T&& impl) : impl_(std::move(impl)) {}
        Impl(const Impl&) = default;
        Impl(Impl&&) = default;
        Impl* Clone() const {
            return new Impl(*this);
        }
        T impl_;
    };
private:
    /// Implementation
    std::unique_ptr< Instance > pImpl_;
};


/// Terminal state: invoke parser, to be put inside a State instance,
/// optionally invoke callback
template < typename InStreamT,
           typename StateMapT,
           typename ParserMapT,
           typename ActionMapT >
class TerminalState : public IState< InStreamT, StateMapT, ParserMapT, ActionMapT > {
public:    
    using Base = IState< InStreamT, StateMapT, ParserMapT >;
    using ID = Base::ID;
    using InStream = Base::InStream;
    using StateMap = Base::StateMap;
    using ParserMap = Base::ParserMap;
    using Parser = typename ParserMap::value_type::second_type;
    using ActionMap = Base::ActionMap;
    using Action = typename ActionMap::value_type::second_type;
    TerminalState() = delete;
    TerminalState(ID id) : id_(id) {}
    TerminalState(const TerminalState&) = default;
    TerminalState(TerminalState&&) = default;
    virtual bool Eval(InStream& is, 
                      ActionMap& actions, 
                      StateMap& map,
                      ParserMap& parsers) {
        if(!Exists(parsers, id_)) { 
            throw std::logic_error("No parser found - terminal "
                                   "state MUST have an associated parser");

            return false;
        }
        return parsers[id_].Parse(is) 
               && Exists(actions, id_) ? actions[id_](parsers[id_].GetValues(), id)
               : true;
    }
    virtual Base* Clone() { return new TerminalState(*this); }
private:
    ID id_;    
};

/// Non-terminal state: do not invoke parser, to be put inside a State instance,
/// optionally invoke callback
template < typename InStreamT,
           typename StateMapT,
           typename ParserMapT,
           typename ActionMapT >
class NonTerminalState : public IState< InStreamT, StateMapT, ParserMapT, ActionMapT > {
public:    
    using Base = IState< InStreamT, StateMapT, ParserMapT >;
    using ID = Base::ID;
    using InStream = Base::InStream;
    using StateMap = Base::StateMap;
    using ParserMap = Base::ParserMap;
    using Parser = typename ParserMap::value_type::second_type;
    using ActionMap = Base::ActionMap;
    using Action = typename ActionMap::value_type::second_type;
    NonTerminalState() = delete;
    NonTerminalState(ID id) : id_(id) {}
    NonTerminalState(const NonTerminalState&) = default;
    NonTerminalState(NonTerminalState&&) = default;
    virtual bool Eval(InStream& is, 
                      ActionMap& actions, 
                      StateMap& map,
                      ParserMap& parsers) {
        if(!Exists(map, id_)) {
            throw std::logic_error("Missing next state: Non terminal "
                                   "state must have a next state");
            return false;
        }
        return map[id_].Eval(is, actions, map, parsers);
        && Exists(actions, id_) ? actions[id_](parsers[id_].GetValues(), id_)
        : true;
    }
    
    virtual Base* Clone() { return new TerminalState(*this); }
private:
    ID id_;    
};

/// Non-terminal state: do not invoke parser, to be put inside a State instance,
/// optionally invoke callback
template < typename InStreamT,
           typename StateMapT,
           typename ParserMapT,
           typename ActionMapT >
class GreedyState : public IState< InStreamT, StateMapT, ParserMapT > {
public:    
    using Base = IState< InStreamT, StateMapT, ParserMapT >;
    using ID = Base::ID;
    using InStream = Base::InStream;
    using StateMap = Base::StateMap;
    using ParserMap = Base::ParserMap;
    using Parser = typename ParserMap::value_type::second_type;
    using ActionMap = Base::ActionMap;
    using Action = typename ActionMap::value_type::second_type;
    GreedyState() = delete;
    GreedyState(ID id) : id_(id) {}
    GreedyState(const GreedyState&) = default;
    GreedyState(GreedyState&&) = default;
    virtual bool Eval(InStream& is, 
                      ActionMap& actions, 
                      StateMap& map,
                      ParserMap& parsers) {
        if(!Exists(map, id_)) {
            throw std::logic_error("Missing next state: Non terminal "
                                   "state must have a next state");
            return false;
        }
        const bool v = map[id_].Eval(is, actions, map, parsers);
        while(map[id_].Eval(is, actions, map, parsers));
        return v && Exists(actions, id_) ? actions[id_](Any(), id)
        : v;
    }
    
    virtual Base* Clone() { return new TerminalState(*this); }
private:
    ID id_;    
};


template < typename IDT, 
           template < typename, typename > class MapT = std::unordered_map >
struct StateTypes {
    using KeyType = IDT;
    using ActionType = std::function< bool (InStream&, const Values&, KeyType) >;
    using ParserType = Parser;
    using StateType = State;
    using ParserMapType = MapT< KeyType, ParserType >; 
    using ActionMapType = MapT< KeyType, ActionType >;
    using StateMapType = MapT< Key, StateType >
    using IStateType = IState< InStream, StateMapType, 
                               ParserMapType, ActionMapType >;
    using NonTerminalStateType = NonTerminalState< InStream, StateMapType, 
                               ParserMapType, ActionMapType >; 
    using TerminalStateType = TerminalState< InStream, StateMapType, 
                               ParserMapType, ActionMapType >;
    using OrStateType = OrTerminalState< InStream, StateMapType, 
                               ParserMapType, ActionMapType >;
    using AndStateType = AndTerminalState< InStream, StateMapType, 
                               ParserMapType, ActionMapType >;
    using NotStateType = NotTerminalState< InStream, StateMapType, 
                               ParserMapType, ActionMapType >;
    using GreedyStateType = GreedyState< InStream, StateMapType, 
                               ParserMapType, ActionMapType >;                                                                                                                                                                      
};



/// Non-terminal state: do not invoke parser, to be put inside a State instance,
/// optionally invoke callback
template < typename InStreamT,
           typename StateMapT,
           typename ParserMapT,
           typename ActionMapT >
class OrState : public IState< InStreamT, StateMapT, ParserMapT > {
public:    
    using Base = IState< InStreamT, StateMapT, ParserMapT >;
    using ID = Base::ID;
    using InStream = Base::InStream;
    using StateMap = Base::StateMap;
    using ParserMap = Base::ParserMap;
    using Parser = typename ParserMap::value_type::second_type;
    using ActionMap = Base::ActionMap;
    using Action = typename ActionMap::value_type::second_type;
    OrState() = delete;
    OrState(ID id) : id_(id) {}
    template < typename...Args >
    OrState(Args...args) : terms_{args} {} 
    GreedyState(const GreedyState&) = default;
    GreedyState(GreedyState&&) = default;
    virtual bool Eval(InStream& is, 
                      ActionMap& actions, 
                      StateMap& map,
                      ParserMap& parsers) {
        for(auto& i: terms_) {
        	const bool r = i.Eval(is, actions, map, parsers);
        	if(r) {
        		return v && Exists(actions, id_) ? 
        		actions[id](parsers[id].GetValues(), id)
        	}
        }
        const bool v = map[id_].Eval(is, actions, map, parsers);
        while(map[id_].Eval(is, actions, map, parsers));
        
        : v;
    }
    
    virtual Base* Clone() { return new TerminalState(*this); }
private:
    std::vector< State > terms_;   
};

// using T = ...//terminal state
// using NT = ...//non-terminal state
// enum {FLOAT, PLUS, MINUS, POPEN, PCLOSE, EXPR};
// map[EXPR]  = T{FLOAT} |
//              T{PLUS} & NT{EXPR} |
//              T{MINUS} & NT{EXPR} |
//              NT{EXPR} & (T{PLUS} | T{MINUS}) & NT{EXPR}) | 
//              T{POPEN} & NT{EXPR} & T{PCLOSE};

class OrState : ... {

    bool Eval(...) {
        for(auto& i: states_) {
            const bool v = i.Eval(...);
            if(v) return true;
        }
        return false;
    }    
};

class AndState : ... {
    bool Eval(...) {
        for(auto& i: states_) {
            if(!i.Eval(...)) return false;
        }
        return true;
    }
};

class NotState : ... {
    bool Eval(...) {
        if(!state_.Eval(...)) return false;
        return true;
    }
};
}
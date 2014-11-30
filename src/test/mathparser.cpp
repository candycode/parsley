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

/// @file recursive.cpp sample code.
/// Implementation of callback and recursive parsers.
#include <iostream>
#include <cassert>
#include <sstream>
#include <vector>
#include <memory>
#include <type_traits>

#include "types.h"
#include "InStream.h"
#include "parsers.h"
#include "parser_operators.h"
#include "typedefs.h"

#define log(x) std::cout << __LINE__ << ": " << x << std::endl;

class Node;

using NodePtr = std::shared_ptr< Node >;
using NodeRef = std::weak_ptr< Node >;
using Nodes   = std::vector< NodePtr >;

using namespace parsley;

enum TYPE {VALUE = 1,
           PLUS,
           MINUS,
           MUL,
           DIV,
           POPEN,
           PCLOSE,
           EMPTY};


class Node : public std::enable_shared_from_this< Node > {
public:    
    Node(const Any& data, TYPE type, 
         NodeRef parent = NodeRef(), NodeRef scope = NodeRef()) 
    : data_(data), type_(type), parent_(parent), scope_(scope)
    {}
    Node() = default;
    Node(const Node&) = default;
    Node(Node&&) = default;
    template < typename T >
    const T& Get() const { return data_.Get< T >(); }
    void Add(NodePtr n) {
        children_.push_back(n);
        n->SetParent(shared_from_this());
    }
    void SetParent(NodePtr p) {
        if(!parent_.expired()) {
            NodePtr t = parent_.lock(); 
            Nodes::iterator i = t->children_.begin();
            for(;
                i != t->children_.end(); ++i) {
                if(i->get() == this) break; 
            }
            if(i != t->children_.end()) {
                NodePtr self = *i;
                p->Add(self);
                t->children_.erase(i);
            }
        }
        parent_ = p;
    }
    NodePtr Parent() const { 
        return parent_.expired() ? NodePtr() :
               parent_.lock(); 
    }
    const Node& First() const {
        return *children_.front();
    }
    const Node& Last() const {
        return *children_.back();
    }
    NodePtr TakeLastChild() { 
        NodePtr n = children_.back();
        children_.resize(children_.size() - 1);
        return n;
    }
    TYPE Type() const { return type_; }
    std::size_t NumChildren() const { return children_.size(); }
private:    
    Any data_;
    TYPE type_;
    
    Nodes children_;
    NodeRef parent_;
    NodeRef scope_;    
};

bool Operator(TYPE t) { 
    return t == PLUS
           || t == MINUS
           || t == MUL
           || t == DIV; 
}

bool ScopeDelimiter(TYPE t) {
    return t == POPEN || t == PCLOSE;
}

class Context {
public:
    Context() : root_(new Node(Any(), EMPTY)),
                cursor_(root_), scope_(root_) {}
    bool Add(const Any& v, TYPE id) {
        NodePtr cursor = cursor_.lock();
        NodePtr n;
        TYPE t = EMPTY;
        switch(id) {
        case VALUE: 
            n = NodePtr(new Node(v, id, NodeRef(), scope_)); 
            cursor->Add(n);
            if(cursor->NumChildren() < 2) cursor_ = n;
            break;
        case POPEN:
            n = NodePtr(new Node(v, id, NodeRef(), scope_)); 
            cursor->Add(n);
            scope_ = n;
            cursor_ = n;
            //log(n->Type());
            break;
        case PCLOSE:
            cursor = scope_.lock();
            //log(cursor->Type());
            assert(ScopeDelimiter(cursor->Type()));
            scope_ = cursor->Parent();
            cursor_ = scope_;
            break;
        case PLUS:
        case MINUS:
        case MUL:
        case DIV:
            n = NodePtr(new Node(v, id, NodeRef(), scope_)); 
            t = cursor->Type();
            if(true) {
                NodePtr c = cursor->Parent()->TakeLastChild();
                cursor->Parent()->Add(n);
                n->Add(c);
            }
            else
                cursor->Add(n);     
            cursor_ = n;
        break;
        case EMPTY: break;
        default: break;    
        }
        return true;
    }
    const Node& Root() const { return *root_; }
private:
    NodePtr root_; 
    NodeRef scope_;
    NodeRef cursor_;

} ctx_G;

double Traverse(const Node& n) {
    double v = double();
    switch(n.Type()) {
    case VALUE:
        v = n.Get< double >();
        break;
    case POPEN:
        assert(n.NumChildren() > 0);
        v = Traverse(n.First());
        break;
    case PLUS:
        v = n.NumChildren() == 1 ? double(0)
                         : Traverse(n.First());
        v = v + Traverse(n.Last());
        break;                 
    case MINUS:
        v = n.NumChildren() == 1 ? double(0)
                         : Traverse(n.First());
        v = v - Traverse(n.Last());
        break;                            
    case MUL:
        assert(n.NumChildren() > 1);
        v = Traverse(n.First()) * Traverse(n.Last());
        break;
    case DIV:
        assert(n.NumChildren() > 1);
        v = Traverse(n.First()) / Traverse(n.Last());
        break;
    case EMPTY:
        if(n.NumChildren() > 0) v = Traverse(n.First());    
    default: break;    
    }
    
    return v;
}


template < typename CBT, typename P, typename Ctx, typename Id >
CBackParser< CBT, P, Ctx, Id > MakeCBackParser( const CBT& cb,
                                            const P& p, 
                                            Ctx& c,
                                            const Id& id) {
    return CBackParser< CBT, P, Ctx, Id >( p, cb, c, id );
}

Parser CB(Parser p, TYPE id) {
    return MakeCBackParser([](const Values& v, Context& ctx, TYPE id  ){
        log(v.begin()->second);
            return ctx.Add(v.begin()->second, id); }, p, ctx_G, id);
}

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
    virtual ID Id() const = 0;
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
    return m.find(key) != m.end();
}


class State {
public:
    template < typename T >
    State(T& impl) : pImpl_(new Impl< T >(impl)) {}
    template < typename T >
    State(T&& impl) : pImpl_(new Impl< T >(std::move(impl))) {}
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
        bool Eval() const {
            using S = IState< InStream,
                              StateMap,
                              ParserMap,
                              ActionMap >;
            return static_cast< const S* >(this)->Eval(is, sm, pm, am);                  
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
class TerminalState : public IState< InStreamT, StateMapT, ParserMapT > {
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
            return parsers[id].Parse(is) 
                   && Exists(actions, id) ? actions[id](parsers[id].GetValues(), id)
                   : true;
        }
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
class NonTerminalState : public IState< InStreamT, StateMapT, ParserMapT > {
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
                    if(!Exists(map, id_))
                        throw std::logic_error("Missing next state: Non terminal "
                                               "state must have a next state");


                   return false;
                   return map[id_].Eval(is, actions, map, parsers);
                   && Exists(actions, id_) ? actions[id](parsers[id].GetValues(), id)
                   : true;
    }
    
    virtual Base* Clone() { return new TerminalState(*this); }
private:
    ID id_;    
};

template < typename IDT, 
           typename ActionT
           template < typename, typename > class MapT = std::unordered_map >
struct Types {
    using KeyType = IDT;
    using ActionType = ActionT;
    using ParserType = Parserl
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



using T = ...//terminal state
using NT = ...//non-terminal state
enum {FLOAT, PLUS, MINUS, POPEN, PCLOSE, EXPR};
map[EXPR]  = T{FLOAT} |
             T{PLUS} & NT{EXPR} |
             T{MINUS} & NT{EXPR} |
             NT{EXPR} & (T{PLUS} | T{MINUS}) & NT{EXPR}) | 
             T{POPEN} & NT{EXPR} & T{PCLOSE};

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

class GreedyState : ... {
    bool Eval(...) {
        States::iterator s == states_.end();
        for(States::iterator i = states_.begin(); i != states_.end(); ++i) {
            if(i->Eval(...)) s = i;
        } 
        return s != states_.end();
    }
};



void TestMathParser() {

    Parser f = CB(F(), VALUE),
           plus = CB(C("+"), PLUS),
           minus = CB(C("-"), MINUS),
           open  = CB(C("("), POPEN),
           close = CB(C(")"), PCLOSE),
           eof = EofParser(); 
    Parser expr;
    Parser rexpr = RefParser( expr );
    Parser term =  f / (open, rexpr, close);
    Parser rterm = RefParser(term);      
    expr = ((rterm, (plus / minus), 
             rexpr) / rterm);
    Parser mathExpr = (expr, eof);
    std::istringstream iss("(2)");
    InStream is(iss);
    if(!expr.Parse( is )) {
        std::cerr << "ERROR AT " << is.tellg() <<  std::endl;
    } else {
        std::cout << "OK" << std::endl;
        const double res = Traverse(ctx_G.Root());
        std::cout << res << std::endl;
    }
}

int main( int, char** ) {
    TestMathParser();
    return 0;
}



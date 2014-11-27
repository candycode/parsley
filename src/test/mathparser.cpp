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

class Node;

using NodePtr = std::shared_ptr< Node >;
using NodeRef = std::weak_ptr< Node >;
using Nodes   = std::vector< NodePtr >;

using namespace parsley;

enum TYPE {VALUE = 0,
           PLUS,
           MINUS,
           MUL,
           DIV,
           POPEN,
           PCLOSE,
           EMPTY};


class Node : std::enable_shared_from_this< Node > {
public:    
    Node(const Any& data, TYPE type, NodePtr parent = NodePtr(), NodePtr scope = NodePtr()) 
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
    void Insert(NodePtr n) {
        n->SetParent(children_.back()->Parent());
        children_.back()->SetParent(n);
        std::swap(children_.back(), n);
    }
    void SetParent(NodePtr p) { parent_ = p; }
    NodePtr Parent() const { return NodePtr(parent_); }
    const Node& First() const {
        return *children_.front();
    }
    const Node& Last() const {
        return *children_.back();
    }
private:    
    Any data_;
    TYPE type_;
    
    Nodes children_;
    NodeRef parent_;
    NodeRef scope_;    
};

bool Operator(TYPE) { return true; }

class Context {
public:
    Context() : root_(new Node(Any(), EMPTY)),
                cursor_(root_), scope_(root_) {}
    bool Add(const Any& v, TYPE id) {
        switch(id) {
        case VALUE: 
            if(v.Empty()) return false;
            cursor_.lock()->Add(new Node(v, id, NodePtr(), scope_));
            cursor_ = cursor_->children.back();
            break;
        case POPEN:
            cursor_->Add(new Node(Any(), id, NodePtr(), scope_));
            cursor_ = cursor_->LastChild();
            scope_ = cursor_;
            break;
        case PCLOSE:
            cursor_ = scope_->Parent();
            break;
        case PLUS:
        case MINUS:
        case MUL:
        case DIV:
            NodePtr n(new Node(Any(), id, NodePtr(), scope_));
            if(cursor_->Type() != POPEN
               && !Operator(cursor->Type())) {
                cursor_->Insert(n);
            } else {
                cursor_->Add(n);
            }
            cursor_ = n;
        }
        return true;
    }
    const Node& Root() const { return *root_; }
private:
    NodePtr root_; 
    NodeRef scope_;
    NodeRef cursor_;  
};

float Traverse(const Node& n, float prevValue = float(0)) {
    switch n.Type() {
    case VALUE:
        return n.Get< float >();
        break;
    case POPEN:
        assert(n.NumChildren() > 0);
        return Traverse(n.First());
        break;
    case PLUS:
        const float v = n.NumChildren() == 1 ? float(0)
                         : Traverse(n.First());
        return v + Traverse(n.Last());
        break;                 
    case MINUS:
        const float v = n.NumChildren() == 1 ? float(0)
                         : Traverse(n.First());
        return v - Traverse(n.Last());
        break;                            
    case MUL:
        assert(n.NumChildren() > 1);
        return Traverse(n.First()) * Traverse(n.Last());
        break;
    case DIV:
        assert(n.NumChildren() > 1);
        return Traverse(n.First()) / Traverse(n.Last());
        break;
    default: break;    
    }
}


template < typename CBT, typename P, typename Ctx, typename Id >
CBackParser< CBT, P, Ctx > MakeCBackParser( const CBT& cb,
                                            const P& p, 
                                            Ctx& c,
                                            const Id& id) {
    return CBackParser< CBT, P, Ctx, Id >( p, cb, c, id );
}

Parser CB(Parser p, int id) {
    return MakeCBackParser([](const Values& v, Context& ctx, int id  ){
            return ctx.Add(v.begin()->second, id); }, p, ctx_G, id);
}

//----------------------------------------------------------------------------

void TestMathParser() {
    Parser f = CB(F(), 1),
           plus = CB(C("+"), 2),
           minus = CB(C("-"),3),
           open  = CB(C("("), 4),
           close = CB(C(")"), 5); 
    Parser expr;
    Parser rexpr = RefParser( expr );
    Parser term =  (open, rexpr, close) / f;
    Parser rterm = RefParser(term);      
    expr = (rterm, (plus / minus), 
             rexpr) / rterm;
    std::istringstream iss("1+(2+3)-1-(23+23.5)");
    InStream is(iss);
    if(!expr.Parse( is ) || !is.eof()) {
        std::cout << "ERROR AT " << is.tellg() <<  std::endl;
        return 1;
    } else {
        std::cout << "OK" << std::endl;
        const float res = Traverse(ctx_G.Root());
        std::cout << res << std::endl;
    }

int main( int, char** ) {
    TestMathParser();
    return 0;
}



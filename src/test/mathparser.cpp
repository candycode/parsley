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

#include "types.h"
#include "InStream.h"
#include "parsers.h"
#include "parser_operators.h"
#include "typedefs.h"

using namespace parsley;

enum TYPES = {VALUE = 0,
              PLUS,
              MINUS,
              MUL,
              DIV,
              POPEN,
              PCLOSE,
              EMPTY};


class Node : std::enable_shared_from_this< Node > {
    Any data_;
    TYPES type_;
    using Nodes = std::vector< std::unique_ptr< Node > >;
    std::vector< std::shared_ptr< Node > > children_;
    std::weak_ptr< Node > parent_;
public:    
    Node(const Any& data, TYPES types) 
    : data_(data), type_(type),
        parent_(this)) {}
    Node() = default;
    Node(const Node&) = default;
    Node(Node&&) = default;
    template < typename T >
    const T& Get() const { return data_.Get< T >(); }
    void Add(std::shared_ptr< Node > n) {
        children_.push_back(n);
        n->SetParent(std::weak_ptr< Node >(shared_from_this()));
    }
    void Insert(std::shared_ptr< Node > n) {
        n->SetParent(nodes_.back()->Parent());
        nodes.back()->SetParent(n);
        std::swap(nodes_.back(), n);
    }
    void SetParent(std::shared_ptr< Node > p) { parent_->p; }
    std::shared_ptr< Node > Parent() const { return parent_; }
};

class Context {
    bool Add(const Values& v, int id) {
        switch(id) {
        case VALUE: 
            assert(v.size());
            cursor_->Add(new Node(v.front(), id, scope_));
            cursor_ = cursor_->children.back();
            break;
        case POPEN:
            cursor_->Add(new Node(Any(), id, scope_));
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
            std::shared_ptr< Node > n(new Node(Any(), id, scope));
            if(cursor_->Type() != POPEN
               && !Operator(cursor->Type())) {
                cursor_->Insert(n);
            } else {
                cursor_->Add(n);
            }
            cursor_ = n;
        }
    }
private:
    std::shared_ptr< Node > cursor_;
    std::shared_ptr< Node > context_;
    std::shared_ptr< Node > root_;   
};

bool CBack( const Values& v, Context&  ) {
    std::cout << *v.begin() << std::endl;
    return true;
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
            std::cout << v.begin()->second << ' ' << id << std::endl;
            return true;}, p, ctx_G, id);
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
    }
    else std::cout << "OK" << std::endl;
}

int main( int, char** ) {
    TestMathParser();
    return 0;
}



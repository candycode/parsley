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

bool CBack( const Values& v ) {
    return true;
    std::cout << *v.begin() << std::endl;
    return true;
}

template < class CB, class P >
CBackParser< CB, P > MakeCBackParser( const CB& cb, const P& p ) {
    return CBackParser< CB, P >( cb, p );
}


void TestRecursiveParser() {
    typedef bool (*CBackType)(const Values& );
    typedef CBackParser< CBackType, Parser > CBP;
    Parser expr;
    Parser rexpr = RefParser( expr );
    Parser value = ( C("("), rexpr ,C(")") ) / F();
    expr = ( RefParser( value ), ( C("+") / C("-") ), 
             RefParser( expr ) ) / RefParser( value );
    std::istringstream iss( "1+(2+3)-1-(23+23.5)" );
    InStream is( iss );
    if( !expr.Parse( is ) || !is.eof() ) {
        std::cout << "ERROR AT " << is.tellg() <<  std::endl;
    }
    else std::cout << "OK" << std::endl;
}

int main( int, char** ) {
    TestRecursiveParser();
    return 0;
}



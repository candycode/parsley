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

/// @file compositeparsers.cpp test/sample code.
/// Test parsers built from combining other parsers, i.e.
/// lexers whuich validate expressions composed of multiple
/// tokens parsed with basic parser types. 

#include <iostream>
#include <cassert>
#include <sstream>

#include "types.h"
#include "InStream.h"
#include "parsers.h"
#include "parser_operators.h"
#include "ParserManager.h"
#include "typedefs.h"

static SkipBlankParser __; // skip blanks static BlankParser _; // parse blank
static SkipToNextLineParser endl_; // skip to next line
static EofParser eof_; // parse EOF
static const bool SKIP_BLANKS = true;
static const bool DONT_SKIP_BLANKS = !SKIP_BLANKS;
static const bool SKIP_TO_NEXT_LINE = true;
static const bool DONT_SKIP_TO_NEXT_LINE = !SKIP_TO_NEXT_LINE;

// TEST COMPOSITE PARSERS
void TestCompositeParsers() {
    std::cout << "\nCOMPOSITE PARSERS TEST" << std::endl;
    std::cout << "========================" << std::endl;
    int count = 0;
    //1 AND lexer
    std::istringstream iss( "c 1 6 0.4 -2.3 1.1" );
    InStream is( iss );
    AndParser al( A("n") > U("#") > U("e") > F("x") > F("y") > F("z") );
    assert( al.Parse( is ) );
    assert( al[ "n" ] == String( "c" ) );
    assert( al[ "#" ] == unsigned( 1 ) );
    assert( al[ "e" ] == unsigned( 6 ) );
    assert( al[ "x" ] == 0.4 );
    assert( al[ "y" ] == -2.3 );
    assert( al[ "z" ] == 1.1 );
    std::cout << ++count << ' ';
    //2 TUPLE
    std::istringstream iss2( " 2.1   2.2  2.3 " );
    InStream is2( iss2 );
    TupleParser< 3 > tl( FloatParser(), "coord" );
    assert( tl.Parse( is2 ) ); 
    std::vector< Any > v = tl[ "coord" ];
    assert( v[ 0 ] == 2.1 && v[ 1 ] == 2.2 && v[ 2 ] == 2.3 );
    std::cout << ++count << ' ';
    //3 TUPLE
    std::istringstream iss3( " 2.1   2.2  2.3 4" );
    InStream is3( iss3 );
    TupleParser<> tl2( FloatParser(), "coord" );
    assert( tl2.Parse( is3 ) );
    std::vector< Any > v2 = tl2[ "coord" ];
    assert( v2[ 0 ] == 2.1 && v2[ 1 ] == 2.2 
            && v2[ 2 ] == 2.3 && v2[ 3 ] == 4.0 );
    std::cout << ++count << ' ';
    //4 TUPLE
    std::istringstream iss4( " string1 |  string2  | string3 | string4" );
    InStream is4( iss4 );
    Parser tl3( *( ( AL(DONT_SKIP_BLANKS) >= __ >= A("") >= __ >= C("|") ) 
         /*OR*/  / ( AL(DONT_SKIP_BLANKS) >= __ >= A("") >=  eof_ ) 
              ) );     
    assert( tl3.Parse( is4 ) );
    std::list< Any > vs = tl3[ "" ];
    assert( *(vs.begin()) == S("string1") 
         && *(++vs.begin()) == S("string2") 
         && *(++++vs.begin() ) == S("string3")
         && *( vs.rbegin() ) == S("string4" ) );
    std::cout << ++count << ' ';
    //5 TUPLE
    std::istringstream 
        iss5( "| string 1 |  string 2 | string 3 | string 4 |  gfdgfd" );
    InStream is5( iss5 );
    AndParser separator(DONT_SKIP_BLANKS ); 
    separator >= __ >= C( "|" ) ;
    TupleParser<> tl4( NotParser<OrParser>( separator / NextLineParser() ), 
                       "strings", separator > __, separator, 
                       PassThruParser(), SKIP_BLANKS );
    assert( tl4.Parse( is5 ) );
    std::vector< Any > vs2 = tl4[ "strings" ];
    assert( vs2.size() == 5 );
    //using a pass-through as the end lexer means we always get something added 
    //as the last element
    assert( vs2[ 0 ] == S("string 1") );
    assert( vs2[ 1 ] == S("string 2") );
    assert( vs2[ 2 ] == S("string 3") );
    assert( vs2[ 3 ] == S("string 4") ); 
    std::cout << ++count << ' ';
    //6 OR lexer
    std::istringstream oriss1( "c 1 6 0.4 -2.3 1.1" );
    std::istringstream oriss2( " 2.1   2.2  2.3 " );
    InStream oris1( oriss1 );
    InStream oris2( oriss2 );
    OrParser ol = al / tl;
    assert( ol.Parse( oris1 ) );
    assert( ol[ "n" ] == String( "c" ) );
    assert( ol[ "#" ] == unsigned( 1 ) );
    assert( ol[ "e" ] == unsigned( 6 ) );
    assert( ol[ "x" ] == 0.4 );
    assert( ol[ "y" ] == -2.3 );
    assert( ol[ "z" ] == 1.1 );
    assert( ol.Parse( oris2 ) );
    std::vector< Any > v4 = ol[ "coord" ];
    assert( v4[ 0 ] == 2.1 && v4[ 1 ] == 2.2 && v4[ 2 ] == 2.3 );
    std::cout << ++count << ' ';
    std::cout << std::endl;
}

int main( int, char** ) {
    TestCompositeParsers();
    return 0;
}



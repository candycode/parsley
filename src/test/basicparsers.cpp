/// @file basicparsers.cpp test/sample code.
/// Test all the token parsers i.e. parsers used to
/// extract individual tokens.

#include <iostream>
#include <cassert>
#include <sstream>

#include "types.h"
#include "InStream.h"
#include "parsers.h"
#include "parser_operators.h"
#include "ParserManager.h"

// TEST BASIC PARSERS
void TestBasicParsers() {
    std::cout << "BASIC PARSERS TEST" << std::endl;
    std::cout << "==================" << std::endl;
    int count = 0;
    //1 int
    std::istringstream iss( "-123" );
    InStream is( iss );
    IntParser il( "n" );
    assert( il.Parse( is ) );   
    assert( il.GetValues().find( "n" )->second == -123 );
    std::cout << ++count << ' ';
    //2 unsigned int
    std::istringstream iss2( "123" );
    InStream is2( iss2 );
    UIntParser il2( "n" );
    assert( il2.Parse( is2 ) ); 
    assert( il2.GetValues().find( "n" )->second == unsigned( 123 ) );
    std::cout << ++count << ' ';
    //3 float 
    std::istringstream iss3( "12.3" );
    InStream is3( iss3 );
    FloatParser il3( "n" );
    assert( il3.Parse( is3 ) ); 
    assert( il3.GetValues().find( "n" )->second == 12.3 );
    std::cout << ++count << ' ';
    //4 float 
    std::istringstream iss4( "-12" );
    InStream is4( iss4 );
    FloatParser il4( "n" );
    assert( il4.Parse( is4 ) ); 
    assert( il4.GetValues().find( "n" )->second == -12.0 );
    std::cout << ++count << ' ';
    //5 float 
    std::istringstream iss5( "-.12" );
    InStream is5( iss5 );
    FloatParser il5( "n" );
    assert( il5.Parse( is5 ) ); 
    assert( il5.GetValues().find( "n" )->second == -.12 );
    std::cout << ++count << ' ';
    //6 float 
    std::istringstream iss6( "-.12E-01" );
    InStream is6( iss6 );
    FloatParser il6;
    assert( il6.Parse( is6 ) ); 
    assert( il6.GetValues().find( "" )->second == -.012 );
    std::cout << ++count << ' ';
    //7 float 
    std::istringstream iss7( ".12E1abc" );
    InStream is7( iss7 );
    FloatParser il7;
    assert( il7.Parse( is7 ) ); 
    assert( il7.GetValues().find( "" )->second == 1.2 );
    assert( is7.get() == 'a' );
    assert( is7.get() == 'b' );
    assert( is7.get() == 'c' );
    std::cout << ++count << ' ';
    //8 float 
    std::istringstream iss8( ".12E+abc" );
    InStream is8( iss8 );
    FloatParser il8( "n" );
    assert( il8.Parse( is8 ) ); 
    assert( il8.GetValues().find( "n" )->second == 0.12 );
    assert( is8.get() == 'E' );
    assert( is8.get() == '+' );
    assert( is8.get() == 'a' );
    assert( is8.get() == 'b' );
    assert( is8.get() == 'c' );
    std::cout << ++count << ' ';
    //9 float 
    std::istringstream iss9( "+1.D2" );
    InStream is9( iss9 );
    FloatParser il9( "n" );
    assert( il9.Parse( is9 ) ); 
    assert( il9.GetValues().find( "n" )->second == 100.0 );
    std::cout << ++count << ' ';
    //10 float 
    std::istringstream iss10( "+1." );
    InStream is10( iss10 );
    FloatParser il10( "n" );
    assert( il10.Parse( is10 ) );   
    assert( il10.GetValues().find( "n" )->second == 1.0 );
    std::cout << ++count << ' ';
    //11 float 
    std::istringstream iss11( "+.E1" );
    InStream is11( iss11 );
    FloatParser il11( "n" );
    assert( il11.Parse( is11 ) );
    assert( il11.GetValues().find( "n" )->second == 0.0 );
    std::cout << ++count << ' ';
    //12 float 
    std::istringstream iss12( "+.E" );
    InStream is12( iss12 );
    FloatParser il12( "n" );
    assert( il12.Parse( is12 ) == false );
    assert( il12.GetValues().empty() );
    assert( is12.get() == '+' );
    assert( is12.get() == '.' );
    assert( is12.get() == 'E' );
    std::cout << ++count << ' ';
    //13 string 
    std::istringstream iss13( "Hello123" );
    InStream is13( iss13 );
    SequenceParser<> il13;
    assert( il13.Parse( is13 ) );
    assert( il13[ "" ] == String( "Hello123" ) );
    std::cout << ++count << ' ';
    //14 string
    std::istringstream iss14( "Hello123" );
    InStream is14( iss14 );
    SequenceParser< ConstStringValidator > il14( "hello123" );
    assert( il14.Parse( is14 ) == false );
    std::cout << ++count << ' ';
    //assert( il13.GetValues().find( "n" )->second == String( "Hello123" ) );
    std::cout << std::endl; 
}


int main( int, char** ) {
    TestBasicParsers();
    return 0;
}




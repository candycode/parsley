/// @file recursive.cpp sample code.
/// Implementation of callback and recursive parsers.
#include <iostream>
#include <cassert>
#include <sstream>

#include "types.h"
#include "InStream.h"
#include "parsers.h"
#include "parser_operators.h"
#include "ParserManager.h"
#include "typedefs.h"

//==============================================================================
// RECURSIVE & CALLBACK PARSERS
//==============================================================================
//------------------------------------------------------------------------------
class RefParser : public IParser
{
public:
    typedef IParser* IParserPtr; //smart pointers ?
    typedef IParser::Values Values;
    RefParser() : ref_( 0 ), memStartPos_( 0 ), memEndPos_( 0 ) {}
    RefParser( IParser& ip ) : ref_( &ip ), memStartPos_( 0 ), memEndPos_( 0 ) {}
    const Values& GetValues() const { return values_; }
    const ValueType& operator[]( const KeyType& k ) const { return ref_->operator[]( k ); }
    bool Parse( InStream& is )
    {
        assert( ref_ && "NULL PARSER REFERENCE" );
        // little memoization logic: if text already parsed simply skip to
        // end of parsed text
        if( memStartPos_ == is.tellg() && !(ref_->GetValues().empty()) )
        {
            is.seekg( memEndPos_ );
            return true;
        }
        const StreamPos p = is.tellg();
        const bool ok = ref_->Parse( is );
        // if parsed record begin and end of parsed text
        if( ok )
        {
            memStartPos_ = p;
            memEndPos_ = is.tellg();
            values_ = ref_->GetValues();
            //for( Values::const_iterator i = values_.begin(); i != values_.end(); ++i )
            //{
            //  std::cout << i->second << ' ';
            //}
            //std::cout << std::endl;
        }
        return ok;
    }
    RefParser* Clone() const { return new RefParser( *this ); }
private:
    StreamPos memStartPos_;
    StreamPos memEndPos_;
    mutable Values values_;
    IParserPtr ref_;
};

template < class CBackT, class ParserT >
class CBackParser : public IParser
{
public:
    typedef IParser::Values Values;
    typedef CBackT CallBackType;
    typedef ParserT ParserType;
    CBackParser( const ParserType& p, const CallBackType& cback ) : p_( p ), cback_( cback ) {}
    const Values& GetValues() const { return p_.GetValues(); }
    const ValueType& operator[]( const KeyType& k ) const { return p_.operator[]( k ); }
    bool Parse( InStream& is ) { if( p_.Parse( is ) ) return cback_( p_.GetValues() ); return false; }
    CBackParser* Clone() const { return new CBackParser( *this ); }
private:
    ParserType p_;
    CallBackType cback_;
};

bool CBack( const IParser::Values& v )
{
    return true;
    std::cout << *v.begin() << std::endl;
    return true;
}

template < class CB, class P >
CBackParser< CB, P > MakeCBackParser( const CB& cb, const P& p )
{
    return CBackParser< CB, P >( cb, p );
}


void TestRecursiveParser()
{
    typedef bool (*CBackType)(const IParser::Values& );
    typedef CBackParser< CBackType, Parser > CBP;
    Parser expr;
    Parser rexpr = RefParser( expr );
    Parser value = ( C("("), rexpr ,C(")") ) / F();
    expr = ( RefParser( value ), ( C("+") / C("-") ), RefParser( expr ) ) / RefParser( value );
    std::istringstream iss( "1+(2+3)-1-(23+23.5)" );
    InStream is( iss );
    if( !expr.Parse( is ) || !is.eof() )
    {
        std::cout << "ERROR AT " << is.tellg() <<  std::endl;
    }
    else std::cout << "OK" << std::endl;
}

int main( int, char** ) {
    TestRecursiveParser();
    return 0;
}



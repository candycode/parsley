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

#include <string>
#include <map>
#include <list>
#include <vector>
//#include <locale> in case support for locale-dependent decimal format needed

#include "Parser.h"
#include "InStream.h"
#include "types.h"

typedef std::string String;

namespace parsley {

/// @file parsers.h
/// Implementation of many common parsers to perform parsing and validation of
/// nuumbers, alphanumeric strings and tuples.

//-----------------------------------------------------------------------------
/// @brief Skips all blanks until next line.
/// @ingroup BlankParsers
class NextLineParser : public IParser {
public:
    typedef Values::value_type::second_type ValueType;
    typedef Values::key_type KeyType;
    const Values& GetValues() const {
        static const Values dummy;
        return dummy;
    }
    const ValueType& operator[]( const KeyType&  ) const {
        throw std::logic_error( "Not implemented" );
    }
    bool Parse( InStream& is, bool rewind = false ) {
        if( is.eof() ) return true;
        if( !is.good() ) return false;
        Char c = is.get(); 
        if( is.eof() ) return true;
        while( !is.eof() && IsBlank( c ) ) c = is.get();
        if( c == Eol() ) return true;
        if( is.good() ) is.unget();
        return false;
    }
    NextLineParser* Clone() const { return new NextLineParser( *this ); }
private:
    bool IsBlank( Char c ) const {
        return parsley::IsSpace( c ) != 0 && c != '\n';
    }
    Char Eol() const { return '\n'; }
};


//-----------------------------------------------------------------------------
/// @brief Advance to the next non-blank character.
/// @ingroup BlankParsers
class SkipBlankParser : public IParser {
public:
    typedef Values::value_type::second_type ValueType;
    typedef Values::key_type KeyType;
    const Values& GetValues() const {
        static const Values dummy;
        return dummy;
    }
    const ValueType& operator[]( const KeyType&  ) const {
        throw std::logic_error( "Not implemented" );
    }
    bool Parse( InStream& is, bool rewind = false ) {
        if( is.eof() ) return true;
        if( !is.good() ) return false;
        Char c = is.get();
        while( !is.eof() && IsBlank( c ) ) c = is.get();
        if( is.good() ) is.unget(); 
        return true;
    }
    SkipBlankParser* Clone() const { return new SkipBlankParser( *this ); }
private:
    bool IsBlank( Char c ) const {
        return parsley::IsSpace( c ) != 0;
    }
};

//-----------------------------------------------------------------------------
/// @brief Advance to next line skipping all blank and non blank characters.
/// @ingroup BlankParsers
class SkipToNextLineParser : public IParser {
public:
    typedef Values::value_type::second_type ValueType;
    typedef Values::key_type KeyType;
    const Values& GetValues() const {
        static const Values dummy;
        return dummy;
    }
    const ValueType& operator[]( const KeyType&  ) const {
        throw std::logic_error( "Not implemented" );
    }
    bool Parse( InStream& is, bool rewind = false ) {
        //Always returns true: used to skip blanks
        if( is.eof() ) return true;
        if( !is.good() ) return false;
        Char c = is.get();
        while( !is.eof() && c != '\n' ) c = is.get();
        if( is.good() ) is.unget(); 
        return true;
    }
    SkipToNextLineParser* Clone() const { 
        return new SkipToNextLineParser( *this ); 
    }
};

//-----------------------------------------------------------------------------
/// @brief Parse blanks.
/// @ingroup BlankParsers
class BlankParser : public IParser {
public:
    typedef Values::value_type::second_type ValueType;
    typedef Values::key_type KeyType;
    const Values& GetValues() const {
        static const Values dummy;
        return dummy;
    }
    const ValueType& operator[]( const KeyType&  ) const {
        throw std::logic_error( "Not implemented" );
    }
    /// @return @c false if first character is not blank or input not in good 
    /// state, @c true otherwise.
    bool Parse( InStream& is, bool rewind = false ) {
        if( !is.good() && !is.eof() ) return false;
        if( is.eof() ) return true;
        Char c = is.get();
        while( !is.eof() && IsBlank( c ) ) c = is.get();
        if( is.good() ) is.unget(); 
        return true;
    }
    BlankParser* Clone() const { return new BlankParser( *this ); }
private:
    bool IsBlank( Char c ) const {
        return parsley::IsSpace( c ) != 0;
    }
};

//-----------------------------------------------------------------------------
/// @brief Parse end of file character.
/// @ingroup BlankParsers
class EofParser : public IParser {
public: 
    typedef Values::value_type::second_type ValueType;
    typedef Values::key_type KeyType;
    const Values& GetValues() const {
        static const Values dummy;
        return dummy;
    }
    const ValueType& operator[]( const KeyType&  ) const {
        throw std::logic_error( "Not implemented" );
    }
    /// @return @c true if input stream @c eof flag set; @c false otherwise.
    bool Parse( InStream& is, bool rewind = true ) {
        return is.eof();  
    }
    EofParser* Clone() const { return new EofParser( *this ); }
};


//------------------------------------------------------------------------------
/// @brief Parse unsigned int.
/// @ingroup NumberParsers
class UIntParser : public IParser {
public:
    typedef Values::value_type::second_type ValueType;
    typedef Values::key_type KeyType;
    typedef InStream::char_type Char;
    UIntParser( const KeyType& name = KeyType() ) : name_( name ) {}
    UIntParser( const UIntParser& p ) 
        : name_( p.name_ ), token_( p.token_ ), valueMap_( p.valueMap_ ) {}
    /// Parses che characters composing an unsigned integer value. 
    /// The characters are stored inside a member variable for further 
    /// conversion to <tt>unsigned int</tt>.
    /// @return @c true if unsigned integer parsed; @c false otherwise. 
    bool Parse( InStream& is, bool rewind = false ) {
        token_.clear();
        valueMap_.clear();
        if( !is.good() ) return false;
        Char c = is.get();
        if( !is.good() ) return false;
        if( parsley::IsDigit( c ) == 0 ) {
            is.unget();
            return false;
        }
        token_.push_back( c );
        GetNumber( is );
        return true;
    }
    
    /// Performs text to number conversion and returns parsed value.
    /// @return value map composed of one (name,value) pair where the name is 
    /// the one assigned in the constructor and the value is the parsed 
    /// unsigned int value.
    const Values& GetValues() const {
        if( valueMap_.empty() && !token_.empty() ) {
            valueMap_.insert( 
                std::make_pair( 
                    name_, unsigned( parsley::ToInt( token_.c_str() ) ) ) );
        }
        return valueMap_;
    }

    /// Performs text to number conversion if needed and returns parsed value 
    /// if key found.
    /// @return Any instance containing unsigned integer value.
    const ValueType& operator[]( const KeyType& k ) const {
        const Values& v = GetValues();
        Values::const_iterator i = v.find( k );
        if( i == v.end() ) throw std::logic_error( "Cannot find value" );
        return i->second;
    }

    /// @return textual representation of parsed number.
    const String& GetText() const { return token_; }

    UIntParser* Clone() const { return new UIntParser( *this ); }

private:
    void GetNumber( InStream& is ) {
        if( !is.good() ) return;
        Char c = is.get();
        if( !is.good() ) return;
        while( is.good() && parsley::IsDigit( c ) != 0 ) {
            token_.push_back( c );
            c = is.get();
        }
        if( is.good() ) is.unget();
    }

    /// Name of parsed value in value map.
    KeyType name_;
    /// Parsed text.
    String token_;
    /// Name  -\> Value map. Declared @c mutable to allow for lazy behavior:
    /// number is converted and stored into map only when UIntParser::GetValues
    /// called.
    mutable Values valueMap_;
};


//------------------------------------------------------------------------------
/// @brief Parse signed integer values.
///
/// Makes use of UIntParser.
/// @ingroup NumberParsers
class IntParser : public IParser {
public: 
    typedef Values::value_type::second_type ValueType;
    typedef Values::key_type KeyType;
    typedef InStream::char_type Char;
    IntParser( const KeyType& name = KeyType() ) : name_( name ) {}
    IntParser( const IntParser& p ) 
        : name_( p.name_ ), token_( p.token_ ), valueMap_( p.valueMap_ ) {}
    /// Parses che characters composing an  integer value. The characters are 
    /// stored inside an member variable for further conversion to <tt>int</tt>.
    /// @return @c true if integer parsed; @c false otherwise.  
    bool Parse( InStream& is, bool rewind = false ) {
        token_.clear();
        valueMap_.clear();
        if( !is.good() ) return false;
        bool ok = false; {
        REWIND r( ok, is );
        Char c = is.get();
        if( !is.good() ) return false;
        if( c == '+' || c == '-' ) {
            UIntParser uil;
            if( uil.Parse( is, rewind ) ) { token_ = c + uil.GetText(); ok = true; }
            else return false;          
        }
        else {
            UIntParser uil;
            if( uil.Parse( is, rewind ) ) { token_ = uil.GetText(); ok = true; }
            else return false;
        }
        ok = true;
        }
        return ok;
    }
    
    /// Performs text to number conversion and returns parsed value.
    /// @return value map composed of one (name,value) pair where the name is 
    /// the one assigned in the constructor and the value is the parsed 
    /// integer value.
    const Values& GetValues() const {
        //valueMap_[]= won't work on Apple gcc 4.0.x
        if( valueMap_.empty() && !token_.empty() ) {
            valueMap_.insert( 
                std::make_pair( 
                    name_, int( parsley::ToInt( token_.c_str() ) ) ) );
        }
        return valueMap_;
    }

    /// Performs text to number conversion if needed and returns parsed value 
    /// if key found.
    /// @return Any instance containing integer value.
    const ValueType& operator[]( const KeyType& k ) const {
        const Values& v = GetValues();
        Values::const_iterator i = v.find( k );
        if( i == v.end() ) throw std::logic_error( "Cannot find value" );
        return i->second;
    }

    IntParser* Clone() const { return new IntParser( *this ); } 

private:
    /// Name of parsed value in value map.
    KeyType name_;
    /// Parsed text.
    String token_;
    /// Name -> Value map. Declared @c mutable to allow for lazy behavior:
    /// number is converted and stored into map only when IntParser::GetValues
    /// called.
    mutable Values valueMap_;
};


//------------------------------------------------------------------------------
/// @brief Float parser.
///
/// Parses number in signed/unsigned floating point number in
/// plain or exponential format.
/// Supports both @c E and @c D as exponent identifier.
/// The decimal separator is assumed to be the '.' character. Can be extended
/// to be locale-aware by reading the decimal separator through a call to
/// @code 
/// std::use_facet< std::numpunct< Char > >( is.getloc() ).decimal_point ();
/// @endcode
/// @ingroup NumberParsers
class FloatParser : public IParser {
public:
    typedef Values::value_type::second_type ValueType;
    typedef Values::key_type KeyType;
    FloatParser( const FloatParser& p ) 
        : name_( p.name_ ), valueMap_( p.valueMap_ ) {}
    FloatParser( const KeyType& name = KeyType() ) : name_( name ) {} 
    virtual const String& GetText() const { return token_ ; }
    /// Reads a floating point number and stores the sequence of parsed 
    /// characters into a member variable. No conversion to a float number is 
    /// performed during parsing.
    virtual bool Parse( InStream& is, bool rewind = false ) {
        token_.clear();
        valueMap_.clear();
        if( !is.good() ) return false;
        // in case support for locale-dependent decimal separator is needed:
        //Char decimalSeparator_ = 
        //    std::use_facet< std::numpunct< Char > >( is.getloc() )
        //    .decimal_point ();
        bool ok = false; {
        REWIND r( ok, is );
        const Char c = is.get();
        if( !is.good() ) return false;  
        if( c == '+' || c == '-' ) {
            if( Apply( &FloatParser::MatchUnsigned, is, c ) ) {
                token_ = c + token_;
            }
        }
        else if( c == '.' ) {
            if( Apply( &FloatParser::MatchFractional, is, c ) ) {
                token_ = c + token_;
            }
        }
        else if( parsley::IsDigit( c ) != 0 ) {
            is.unget();
            Apply( &FloatParser::MatchUnsigned, is, 0 );
        }
        else return false;
        ok = token_.length() > 0;
        }
        return ok;
    }
    /// Performs text to float conversion and returns parsed value.
    /// @return value map composed of one (name,value) pair where the name is 
    ///         the one assigned in the constructor and the value is the parsed 
    ///         float value.
    const Values& GetValues() const {
        if( valueMap_.empty() && !token_.empty() ) {
            valueMap_.insert( 
                std::make_pair( 
                    name_, parsley::ToFloat( token_.c_str() ) ) );
        }
        return valueMap_;
    }
    /// Performs text to float conversion if needed and returns parsed value if 
    /// key found.
    /// @return Any instance containing float value. 
    const ValueType& operator[]( const KeyType& k ) const {
        const Values& v = GetValues();
        Values::const_iterator i = v.find( k );
        if( i == v.end() ) throw std::logic_error( "Cannot find value" );
        return i->second;
    }

    virtual FloatParser* Clone() const {
        return new FloatParser( *this );
    }


private:
    typedef bool( FloatParser::*MatchMethod )( InStream& ); 

    bool Apply( MatchMethod f, InStream& is, Char last ) {
        if( !is.good() ) return false;
        const String tmp = token_; // save current value
        const StreamPos pos = is.tellg();
        const bool m = (this->*f)( is );
        if( !m ) {
            //restore previous value
            token_ = tmp;
            //restore stream pointer
            is.seekg( pos );
        }
        //last == 0 is a signal that enough input has been validated to make
        //the token a valid float 
        return last == 0 || m;
    }

    bool MatchUnsigned( InStream& is ) {
        if( !is.good() ) return false;
        const Char c = is.get();
        if( !is.good() ) return false;
        if( parsley::IsDigit( c ) != 0 ) { 
            token_.push_back( c ); 
            Apply( &FloatParser::MatchUnsigned, is, 0 ); 
            return true; 
        }
        else if( c == '.' ) { 
            token_.push_back( c ); 
            return Apply( &FloatParser::MatchFractional, is, c ); 
        }
        is.unget();
        return false;
    }

    bool MatchFractional( InStream& is ) {
        if( !is.good() ) return false;
        const Char c = is.get();
        if( !is.good() ) return false;
        if( parsley::IsDigit( c ) ) {
            token_.push_back( c ); 
            Apply( &FloatParser::MatchFractional, is, 0 );
            return true;
        }
        else if( c == 'E' || c == 'D' || 
                 c == 'e' || c == 'd' ) {
             // (as of 2009) problem: on windows 'D' (with both vc++ and migw) 
             // is properly understood but not on linux/mac: need to force a 
             // 'D' -> 'E' translation
             token_.push_back( 'E' );
             return Apply( &FloatParser::MatchExponent, is, c );
        }
        is.unget();
        return false;
    }
    bool MatchExponent( InStream& is ) {
        if( !is.good() ) return false;
        const Char c = is.get();
        if( !is.good() ) return false;
        if( c == '+' || c == '-' ) {
            token_.push_back( c ); 
            return Apply( &FloatParser::MatchExponent, is, c );
        }
        else if( parsley::IsDigit( c ) ) {
            token_.push_back( c ); 
            Apply( &FloatParser::MatchExponent, is,  0 ); 
            return true;
        }
        is.unget();
        return false;
    }
    /// Name of parsed value in value map.
    KeyType name_;
    /// Parsed text.
    String token_;
    /// @brief Name -\> Value map.
    /// Declared @c mutable to allow for lazy behavior:
    /// number is converted and stored into map only when FloatParser::GetValues
    /// called.
    mutable Values valueMap_;
};

//------------------------------------------------------------------------------
/// @brief Validator for alphanumeric strings.
/// @ingroup validators 
struct AlphaNumValidator {
    /// Validates any alphanumeric value
    bool Validate( const String&, Char newChar ) const {
        return parsley::IsAlnum( newChar ) != 0;
    }
    
    void Reset() {}
};

/// @brief Validator for alphanumeric string starting with an alphabetical 
/// character.
/// @ingroup validators
struct FirstAlphaNumValidator {
    /// Validates a new character only if it's an alphabetical value or if
    /// it's an alphanumeric value and the validated string is not empty.
    /// @param s currently parsed characters.
    /// @param newChar current character to validate.
    /// @return @c true if new character is alphabetical or is a digit but not
    /// the first character to be parsed.
    bool Validate( const String& s, Char newChar ) const {
        return s.empty() ? 
               parsley::IsAlpha( newChar ) != 0 
                                 || parsley::IsDigit( newChar ) != 0
            : parsley::IsAlpha( *s.begin() ) != 0 && 
              ( parsley::IsAlpha( newChar ) != 0 
                || parsley::IsDigit( newChar ) != 0 );
    }
    
    void Reset() {}
};

/// @brief Validates a constant string with support for case-insensitive 
/// validation.
/// @ingroup validators
/// @todo use stream's facilities to perform uppercase \<-\> lowercase 
/// conversion since this is much better for future @e locale and @e Unicode
/// support.
class ConstStringValidator {
public:
    /// Constructor.
    /// @param s constant string to validate.
    /// @param ignoreCase enable/disable case-insensitive comparison.
    ConstStringValidator( const String& s, bool ignoreCase = false )
        : value_( s ), ignoreCase_( ignoreCase ), count_( 0 ) {}
    ConstStringValidator() : ignoreCase_( false ), count_( 0 ) {}
    ConstStringValidator( const ConstStringValidator& csv )
        : value_( csv.value_ ), ignoreCase_( csv.ignoreCase_ ), 
          count_( csv.count_ ) {}
    /// Validates newly read character.
    /// For this function to work for constant strings the partially read
    /// string should be passes at each invocation of the method; in order
    /// to decrease memory usage and execution time an internal index
    /// pointing to the current character to match in the constant string
    /// is incremented each time this method is invoked and can be reset
    /// through the ConstStringValidator::Reset method.
    /// @param newChar last read character.
    /// @return @c true if character matches corresponding constant string
    /// character, @c false otherwise.
    bool Validate( const String& , Char newChar ) const {
        if( count_ >= value_.length() ) return false;
        if( ignoreCase_ ) {
            return parsley::ToLower( value_[ count_++ ] ) 
                        == parsley::ToLower( newChar );
        }
        return value_[ count_++ ] == newChar;
    }
    /// Resets internal character index.
    void Reset() { count_ = 0; }
private:
    /// Constant string value.
    String value_;
    /// Ignore case flag.
    bool ignoreCase_;
    /// Index pointing at the next character to validate in 
    /// ConstStringValidator::value_.
    mutable String::size_type count_;
};


/// @brief Parser for strings.
///
/// It can be configured with validators having the following
/// compile time interface:
/// @code
/// struct ... 
/// {
///   bool Validate( const String& currentlyValidated, Char newChar ) const;
///   void Reset();
/// };
/// @endcode
/// @ingroup StringParsers
template < class ValidatorT = AlphaNumValidator >
class SequenceParser : public IParser {
public:
    typedef Values::value_type::second_type ValueType;
    typedef Values::key_type KeyType;
    typedef InStream::char_type Char;
    typedef ValidatorT Validator;
    /// Templated constructor.
    /// @tparam T type of value passed to validator constructor.
    /// @param v value passed to validator constructor.
    /// @param name name associated to retrieved value(s); set as key in value 
    ///        map
    template < class T >
    SequenceParser( const T& v, const ValueID& name = ValueID() ) 
        : validator_( v ), name_( name ) {} 
    /// Constructor.
    /// @param name name associated to retrieved value(s); set as key in value 
    /// map 
    SequenceParser( const ValueID& name = ValueID() ) : name_( name ) {}
    /// Copy constructor.
    SequenceParser( const SequenceParser& l ) 
        : name_( l.name_ ), token_( l.token_ ), valueMap_( l.valueMap_ ), 
          validator_( l.validator_ ) {}
    /// Overridden IParser::Parse method.
    bool Parse( InStream& is, bool rewind = false ) {
        token_.clear();
        valueMap_.clear();
        validator_.Reset();
        if( !is.good() ) return false;
        Char c = 0;
        while( !is.eof() ) {
            c = is.get();
            if( !is.good() ) {
                return token_.length() > 0;
            }
            if( !validator_.Validate( token_, c ) ) break;
            token_.push_back( c );
        }
        if( is.good() ) is.unget();
        return token_.length() > 0;
    }
    /// Overridden IParser::GetValues method.   
    const Values& GetValues() const {
        if( valueMap_.empty() & !token_.empty() ) {
            valueMap_.insert( std::make_pair( name_, token_ ) );
        }
        return valueMap_;
    }
    /// Overridden IParser::operator[] .
    const ValueType& operator[]( const KeyType& k ) const {
        const Values& v = GetValues();
        Values::const_iterator i = v.find( k );
        if( i == v.end() ) throw std::logic_error( "Cannot find value" );
        return i->second;
    }
    /// Overridden IParser::Clone method.
    SequenceParser* Clone() const { return new SequenceParser( *this ); }   
    /// @return last parsed text
    const String& GetText() const { return token_; }
private:
    /// Name associated to retrieved value(s); set as key in value map 
    ValueID name_;
    /// Parsed text.
    String token_;
    /// Parsed values.
    mutable Values valueMap_;
    /// Validator used by SequenceParser::Parse method to validate input.
    Validator validator_;
};

//------------------------------------------------------------------------------
/// @brief Convenience class that implements a parser for constant strings.
///
/// Contains a @c SequenceParser<ConstStringValidator> to perform the actual 
/// parsing.
/// @ingroup StringParsers
class ConstStringParser : public IParser {
public:
    ConstStringParser( const String& s, const ValueID& name = ValueID(), 
                       bool ignoreCase = true ) :
      csv_( ConstStringValidator( s, ignoreCase ), name ) {}
    bool Parse( InStream& is, bool rewind = false ) { return csv_.Parse( is, rewind ); }
    const Values& GetValues() const { return csv_.GetValues(); }
    const ValueType& operator[]( const KeyType& k ) const { return csv_[ k ]; }
    ConstStringParser* Clone() const { return new ConstStringParser( *this ); } 
private:
    SequenceParser< ConstStringValidator > csv_;
};

//------------------------------------------------------------------------------
/// @brief Convenience class that implements a parser for alphanumeric strings.
///
/// Contains/ a @c SequenceParser<AlphaNumValidator> to perform the actual 
/// parsing.
/// @ingroup StringParsers
class AlphaNumParser : public IParser {
public:
    AlphaNumParser( const String& name ) : anl_( name ) {}
    bool Parse( InStream& is, bool rewind = true ) { return anl_.Parse( is, rewind ); }
    const Values& GetValues() const { return anl_.GetValues(); }
    const ValueType& operator[]( const KeyType& k ) const { return anl_[ k ]; }
    AlphaNumParser* Clone() const { return new AlphaNumParser( *this ); }
private:
    SequenceParser< AlphaNumValidator > anl_;
};

//------------------------------------------------------------------------------
/// @brief Convenience class that implements a parser for alphanumeric strings 
/// that start with a letter.
///
/// Contains a @c SequenceParser<AlphaNumValidator> to validate 
/// characters past the first character.
/// @ingroup StringParsers
class FirstAlphaNumParser : public IParser {
public:  
    typedef Values::value_type::second_type ValueType;
    typedef Values::key_type KeyType;
    /// Cosntructor.
    /// @param name name assigned to parsed value in returned value map
    FirstAlphaNumParser( const String& name ) : name_( name ) {}
    /// IParser::Parse implementation: checks if the first character is a letter
    /// then uses the included SequenceParser to parse additional input.
    bool Parse( InStream& is, bool rewind = true ) {
        token_.clear();
        valueMap_.clear();
        if( !is.good() ) return false;
        Char c = is.get();
        if( parsley::IsAlpha( c ) != 0 ) {
            token_ += c;
            if( anl_.Parse( is, rewind ) ) {
                token_ += anl_.GetText();
            }
            return true;
        }
        is.unget();
        return false;
    }
    /// IParser::GetValues implementation.
    const Values& GetValues() const {
        if( valueMap_.empty() ) {
            valueMap_.insert( std::make_pair( name_, token_ ) );
        }
        return valueMap_;
    }
    /// IParser::operator[] implementation.
    const ValueType& operator[]( const KeyType& k ) const {
        const Values& v = GetValues();
        Values::const_iterator i = v.find( k );
        if( i == v.end() ) throw std::logic_error( "Cannot find value" );
        return i->second;
    }
    /// IParser::Clone implementation.
    FirstAlphaNumParser* Clone() const { 
        return new FirstAlphaNumParser( *this ); 
    } 
private:
    String name_;
    String token_;
    mutable Values valueMap_;
    SequenceParser< AlphaNumValidator > anl_;
};

//------------------------------------------------------------------------------
/// @brief Parser for sequences of values of the same type separated by a 
/// separator expression and bound with a start/end expression.
/// @ingroup Parsers
template < int SIZE = -1 > // 
class TupleParser : public IParser {
public:  
    typedef Values::value_type::second_type ValueType;
    typedef Values::key_type KeyType;
    typedef InStream::char_type Char;
    typedef std::list< Parser > Parsers;
    TupleParser() {}
    /// Constructor.
    /// @param v value parser: used to parse individual tuple values
    /// @param name name assigned to parsed value array in returned value map
    /// @param b begin parser: parses expression identifying the start of the 
    /// tuple
    /// @param s separator parser: parses text used to separate adjacent values
    /// @param e end parser: parses expression identifying end of tuple
    /// @param blanks skip blanks flag: if @c true blanks are skipped before 
    /// applying each parser
    TupleParser( const Parser& v, const ValueID& name = ValueID(), 
                 const Parser& b = BlankParser(),
                 const Parser& s = BlankParser(), 
                 const Parser& e = BlankParser(), bool blanks = false ) 
                 : skipBlanks_( blanks ),
                 beginParser_( b ), valueParser_( v ), 
                 separatorParser_( s ), endParser_( e ), name_( name ) {}
    TupleParser( const TupleParser& l ) 
        : skipBlanks_( l.skipBlanks_ ), name_( l.name_ ), 
          beginParser_( l.beginParser_ ), 
          valueParser_( l.valueParser_ ), 
          separatorParser_( l.separatorParser_ ),
          endParser_( l.endParser_ ), values_( l.values_ ), 
          valueMap_( l.valueMap_ )
    {}
    /// IParser::Parse implementation: applies begin parser, loops over 
    /// values/separators then
    /// applies end parser. Note that it will always parse as many values as 
    /// possible to make it possible to report an error when the number of 
    /// parsed values exceeds the number of required values.
    bool Parse( InStream& is, bool rewind = true ) {
        values_.clear();
        valueMap_.clear();
        if( !is.good() ) return false;
        int counter = 0;
        bool ok = false; {
        // record current file pointer and rewind when exiting scope in case
        // parser not applied; nested scope required if local variable used
        // as signal
        REWIND r( ok, is );
        // apply begin parser and return false if it fails
        if( skipBlanks_ ) SkipBlanks( is );
        if( !beginParser_.Parse( is, rewind ) ) return false;
        bool endReached = false;
        std::vector< ValueType >::size_type s = 0;
        // start applying value parser
        if( skipBlanks_ ) SkipBlanks( is );
        while( valueParser_.Parse( is, rewind ) ) {
            // parsed values: add values to value array
            AddValue( valueParser_.GetValues() );
            // increment value counter
            ++counter;
            // apply separator parser and exit loop if it fails
            if( !separatorParser_.Parse( is, rewind ) ) break;
            // value & separator parser applied, if required skip blanks 
            // before next value or end separator
            if( skipBlanks_ ) SkipBlanks( is );
        }
        // exited from loop: either end separator or wrong expression found  
        
        // apply end separator: if it succeeds it means the end separator was
        // parsed successfully and the end of the tuple was reached
        if( skipBlanks_ ) SkipBlanks( is );
        endReached = endParser_.Parse( is, rewind );
        
        // if end reached and number of parsed values is correct return true, 
        // false otherwhise
        ok = endReached && ( counter == Size() 
                             || ( counter > 0 && Size() < 0 ) );
        }
        return ok;
    }
    /// @return size of tuple.
    static int Size() { return SIZE; }
    /// IParser::GetValues implementation.
    const Values& GetValues() const {
        if( valueMap_.empty() && !values_.empty() ) {
            valueMap_.insert( std::make_pair( name_, values_ ) );
        }
        return valueMap_;
    }
    /// IParser::operator[] implementation
    const ValueType& operator[]( const KeyType& k ) const {
        const Values& v = GetValues();
        Values::const_iterator i = v.find( k );
        if( i == v.end() ) throw std::logic_error( "Cannot find value" );
        return i->second;
    }
    /// IParser::Clone implementation.
    TupleParser* Clone() const { return new TupleParser( *this ); } 

private:
    /// Advances to first non-blank character or @c EOF.
    void SkipBlanks( InStream& is ) {
        if( is.good() ) {
            Char c = is.get();
            while( is.good() && parsley::IsSpace( c ) != 0  ) c = is.get();
            if( is.good() ) is.unget();
        }
    }

    /// Adds value to parsed value array.
    std::vector< ValueType >::size_type AddValue( const Values& v ) {
        std::vector< ValueType >::size_type bs = values_.size();
        for( Values::const_iterator i = v.begin();
             i != v.end();
             ++i ) values_.push_back( i->second );
        return values_.size() - bs;
    }
    /// Skip blanks flag: if @c true the TupleParser::Parse method skips
    /// all blanks before parsing input.
    bool skipBlanks_;
    /// Name assigned to values in value map.
    String name_;
    /// Parser that identifies the start of the tuple.
    Parser beginParser_;
    /// Parser that parses individual tuple components.
    Parser valueParser_;
    /// Parser that parses the expression separating two adjacent values.
    Parser separatorParser_;
    /// Parser that parses the expression identifying the end of the tuple.
    Parser endParser_;
    /// Parsed values.
    std::vector< ValueType > values_;
    /// Value map.
    mutable Values valueMap_;
};


/// @brief Pass through parser: always validates the input.
///
/// Sample usage:
/// @code
/// std::istringstream 
/// iss5( "| string 1 |  string 2 | string 3 | string 4 |  gfdgfd" );
/// InStream is5( iss5 );
/// AndParser separator(DONT_SKIP_BLANKS ); 
/// separator >= __ >= C( "|" ) ;
/// TupleParser<> tl4( 
///   NotParser<OrParser>( separator / NextLineParser() ), 
///       "strings", separator > __,
///   separator,
///   PassThruParser(),
///   SKIP_BLANKS ); // end parser always succeeds i.e. no end parser separator
///                  // for tuple
/// assert( tl4.Parse( is5 ) );
/// std::vector< Any > vs2 = tl4[ "strings" ];
/// assert( vs2.size() == 5 );
/// @endcode
/// @ingroup Parsers
class PassThruParser : public IParser {
public:
    bool Parse( InStream&,bool rewind = true ) { return true; }
    const Values& GetValues() const { static const Values v; return v; }
    const ValueType& operator[]( const KeyType&  ) const { 
        static const ValueType v; 
        return v; 
    }
    PassThruParser* Clone() const { return new PassThruParser( *this ); }   
};

} //namespace

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

/// @file types.h Common types and wrappers.

//common types, other option is to parameterize all classes with template
//parameters

#include <iosfwd>
#include <string>
#include <cctype>
#include <map>
#include "InStream.h"
#include "Any.h"

namespace parsley {

typedef std::string String;
typedef InChStream< std::istream > InStream;
typedef InStream::streampos StreamPos;
typedef InStream::streamoff StreamOff;
typedef InStream::char_type Char;
typedef String ValueID;
typedef String ParserID;
typedef std::map< ValueID, Any > Values;

// wrappers useful for future unicode support
///@todo consder using the std::sto* functions
inline double ToFloat( const char* str ) { return ::atof( str ); }
inline int ToInt( const char* str ) { return ::atoi( str ); }
inline int IsSpace( int c ) { return ::isspace( c ); }
inline int ToLower( int c ) { return ::tolower( c ); }
inline int IsAlpha( int c ) { return ::isalpha( c ); }
inline int IsAlnum( int c ) { return ::isalnum( c ); }
inline int IsDigit( int c ) { return ::isdigit( c ); }

} //namespace

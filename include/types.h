#ifndef TYPES_H_
#define TYPES_H_

/// @file types.h Common types and wrappers.

//common types, other option is to parameterize all classes with template parameters
#include <iosfwd>
#include <string>
#include <cctype>
#include "InStream.h"

typedef std::string String;
typedef InChStream< std::istream > InStream;
typedef InStream::streampos StreamPos;
typedef InStream::streamoff StreamOff;
typedef InStream::char_type Char;
typedef String ValueID;
typedef int StateID;
typedef String ParserID;
enum {EMPTY_STATE = -1};

// wrappers useful for future unicode support
inline double ToFloat( const char* str ) { return ::atof( str ); }
inline int ToInt( const char* str ) { return ::atoi( str ); }
inline int IsSpace( int c ) { return ::isspace( c ); }
inline int ToLower( int c ) { return ::tolower( c ); }
inline int IsAlpha( int c ) { return ::isalpha( c ); }
inline int IsAlnum( int c ) { return ::isalnum( c ); }
inline int IsDigit( int c ) { return ::isdigit( c ); }

#endif


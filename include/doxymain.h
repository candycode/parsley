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
// *** NOONE SHOULD INCLUDE THIS PAGE INTO ANYTHING, IT'S ONLY FOR USE WITH DOXYGEN ***

/// @file doxymain.h Doxygen source code for documentation pages.
//==============================================================================
// MAIN PAGE
//==============================================================================

/*! \mainpage Parsley
 *
 * \section intro_sec Introduction
 *
 * Parsley is a multiplatform parsing framework originally developed to extract values from text data files. It can
 * be used as it is or easily extended to support <em>Parsing Expression Grammar</em> it was built to be used in conjuntion with
 * the provided state machine manager to ease the parsing of different sections of text files.
 * It doesn't have any dependency other than STL and is developed and tested on Linux, Windows and Mac OS.
 * 
 * \section sample_usage Sample usage
 * \subsection extract_int Integer parser
 * \code
 *  // 1) create input stream to read from
 *  std::istringstream iss( "-123" );
 *  InStream is( iss );
 *  // 2) create parser and set the name for the returned values to 'number'
 *  IntParser il( "number" );
 *  // 3) apply parser: returns true on success
 *  assert( il.Parse( is ) );
 *  // 4) extract parsed value by name; returned values
 *  //    are instances of type Any which can be cast to any other type
 *  //    provided the conversion is possible 
 *  assert( il.GetValues().find( "number" )->second == -123 );
 * \endcode
 *  
 * \section depend_sec Compilation 
 * \subsection dependencies Dependencies
 * No dependencies, the only C++ library used is STL.
 * \subsection build Building
 * Building is performed through <em>CMake</em>. Should work with any version; tested with versions @c 2.6 and @c 2.8.
 */

//==============================================================================
// GROUPS (MODULES)
//==============================================================================
//------------------------------------------------------------------------------
//TODO
/// @defgroup todo Todo
//------------------------------------------------------------------------------
//MAIN CLASSES
/// @defgroup MainClasses Main Classes
//------------------------------------------------------------------------------
//UTILITY
/// @defgroup utility Utility
//------------------------------------------------------------------------------
//PARSER OPERATORS
/// @defgroup operators Parser operators
/// Overloaded operators used to ease the construction of composite parsers.
//------------------------------------------------------------------------------
//PRE BUILT PARSERS
/// @defgroup Parsers Parsers
/// Pre-built parsers for the most common data types.

/// @defgroup BlankParsers Blank Parsers
/// @ingroup Parsers
/// Blank parsers used to parse or skip sequences of blanks.

/// @defgroup NumberParsers Number Parsers
/// @ingroup Parsers
/// Parsers for numerical values

/// @defgroup StringParsers String Parsers
/// @ingroup Parsers
/// Parsers for strings
//------------------------------------------------------------------------------
//VALIDATORS used by string parsers
/// @defgroup validators Text validators
/// Validators must impelemt the following compile time interface:
/// @code
/// struct ... 
/// {
///	  bool Validate( const String& currentlyValidated, Char newChar ) const;
///	  void Reset();
/// };
/// @endcode
/// @ingroup StringParsers

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
 * No dependencies other than @c STL.
 * \subsection build Building
 * Building is performed through <em>CMake</em>. Should work with any version; tested with version @c 2.6.
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

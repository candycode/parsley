#pragma once
///@file typedefs.h various typedefs used throughout the examples/tests.
typedef AlphaNumParser A; ///< "12abc"
typedef FirstAlphaNumParser FA; ///< "A123"
typedef UIntParser U; ///< "123"
typedef FloatParser F; ///< "12.2E03"
typedef String S; ///< "abc"
/// only returns true if the string is equal to a constant value
typedef ConstStringParser C;
/// parses expressions NOT equal to const value 
typedef NotParser<ConstStringParser> NC; 
/// parses expressions NOT equal to a sequence of expressions
typedef NotParser< AndParser > NAL; 
/// 'AND' Lexer: true iff all sub-expressions are parsed correctly 
typedef AndParser AL; 
/// 'OR'  Lexer: true if any of the sub-expression is parsed correctly
typedef OrParser OL;  
typedef TupleParser<-1> T; ///< parse sequence
typedef TupleParser<3> T3; ///< parse sequence of 3 elements
typedef TupleParser<2> T2; ///< parse sequence of 2 elements

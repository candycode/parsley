#ifndef PARSER_OPERATORS_H_
#define PARSER_OPERATORS_H_

/// @file parser_operators.h
/// Contains the implementation of a number of convenience operators that
/// make it easier and more readable to define complex parsers.

//------------------------------------------------------------------------------
/// @defgroup AndParserOperators AndParser operators
/// @ingroup operators

/// @ingroup AndParserOperators
inline AndParser operator,( AndParser al1, const Parser& al2 ) {
    al1.Add( al2 );
    return al1;
}

/// @ingroup AndParserOperators
inline AndParser operator,( const Parser& al1, AndParser al2 ) {
    al2.AddFront( al1 );
    return al2;
}

/// @ingroup AndParserOperators
inline AndParser operator,( const Parser& al1, const Parser& al2 ) {
    AndParser al;
    al.Add( al1 ).Add( al2 );
    return al;
}

/// @ingroup AndParserOperators
inline AndParser operator>( AndParser al1, const Parser& al2 ) {
    al1.Add( al2 );
    return al1;
}

/// @ingroup AndParserOperators
inline AndParser operator>( const Parser& al1, AndParser al2 ) {
    al2.AddFront( al1 );
    return al2;
}

/// @ingroup AndParserOperators
inline AndParser operator>( const Parser& al1, const Parser& al2 ) {
    AndParser al;
    al.Add( al1 ).Add( al2 );
    return al;
}

/// @ingroup AndParserOperators
inline AndParser operator&( const Parser& al1, const Parser& al2 ) {
    AndParser al;
    al.Add( al1 ).Add( al2 );
    return al;
}

//------------------------------------------------------------------------------

/// @defgroup OrParserOperators OrParser operators
/// @ingroup operators

/// @ingroup OrParserOperators
inline OrParser operator/( const Parser& ol1, OrParser& ol2 ) {
    ol2.Add( ol1 );
    return ol2;
}

/// @ingroup OrParserOperators
inline OrParser operator/( OrParser& ol1, const Parser& ol2 ) {
    ol1.Add( ol2 );
    return ol1;
}

/// @ingroup OrParserOperators
inline OrParser operator/( const Parser& al1, const Parser& al2 ) {
    OrParser al;
    al.Add( al1 ).Add( al2 );
    return al;
}

#endif //PARSER_OPERATORS_H_

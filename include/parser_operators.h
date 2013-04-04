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

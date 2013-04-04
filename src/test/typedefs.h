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

using namespace parsley;

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

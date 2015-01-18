#pragma once
////////////////////////////////////////////////////////////////////////////////
//Parsley - parsing framework
//Copyright (c) 2010-2015, Ugo Varetto
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
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
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
#include <vector>
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
//@todo consider using an unordered_map or making it configurable
typedef std::map< ValueID, Any > Values;
template < typename MapT >
    const typename MapT::value_type::second_type&
Get(const MapT& v, typename MapT::value_type::first_type vid) {
    assert(v.find(vid) != v.end());
    return v.find(vid)->second;
}
template < typename MapT >
const typename MapT::value_type::second_type&
Get(const MapT& v) {
    assert(!v.empty());
    return v.begin()->second;
}
template < typename MapT >
bool In(const typename MapT::value_type::first_type& k, const MapT& m) {
    return m.find(k) != m.end();
}
    
template < typename T, typename W, typename M = std::map< T, W > >
M GenWeightedTerms(const std::vector< std::vector< T > >& vt,
                   const std::vector< T >& scope,
                   W start = W(0),
                   const W& step = W(1000)) {
    std::map< T, W > m;
    for(auto& i: vt) {
        for(auto& j: i) {
            m.insert(std::make_pair(j, start));
        }
        start += step;
    }
    for(auto& i: scope) {
        m.insert(std::make_pair(i, start));
    }
    return m;
}

template < typename T, typename W, typename M >
M InsertWeightedTerms(const M& m,
                      T term, //insert before term
                      const std::vector< T >& v
                                     ) {
    assert(m.find(term) != m.end());
    const W w = m.find(term).second;
    const W f = m.begin()->second;
    W l = f;
    for(auto i: m) {
        if(i.second != f) {
            l = i.second;
            break;
        }
    }
    const W step = l - f;
    W c = m.begin().second;
    std::vector< T > t;
    std::vector< std::vector< T > > vt;
    bool added = false;
    for(auto i: m) {
        if(i.second == w && !added) {
            vt.push_back(v);
            added = true;
        }
        if(i.second == c) {
            t.push_back(i.first);
        } else {
            vt.push_back(t);
            t.resize(0);
            c = i.second;
            t.push_back(i.first);
        }
    }
    return GenWeightedTerms< T, W, M >(vt, std::vector< T >(), f, step);
}
    
    
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

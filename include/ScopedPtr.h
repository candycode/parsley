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

/// @file ScopedPtr.h Implementaion of scoped smart pointer class for automatic 
/// memory management.

#include <memory>
#include <cassert>

namespace parsley {

//------------------------------------------------------------------------------
/// @brief Scoped pointer implementation; uses default memory de-allocator.
/// @ingroup utility
template < class T >
class ScopedPtr {
    /// Type used in the implementation of the the unspecified bool idiom: i.e.
    /// perform automatic conversion to bool type from member function pointer.
    typedef void ( ScopedPtr< T >::*BoolType )() const;
    /// Method used in thte unspecified bool idiom.
    void BoolTypeImpl() const {}
public:
    typedef T ElementType;
    typedef ElementType* PointerType;
    /// Default constructor.
    ScopedPtr() : rawPtr_( 0 ) {}
    /// Explicit constructor from pointer.
    template < class U > explicit SmartPtr(U* p ) {
        rawPtr_ = dynamic_cast< PointerType >( p );
    }
    /// Copy constructor.
    ScopedPtr( const ScopedPtr& sp ) {
        rawPtr_ = Clone( sp );
    }
    /// Copy constructor from smart pointer of different type.
    template < class U >
    SmartPtr( const ScopedPtr< U >& sp ) {
        rawPtr_ = Clone( sp );
    }
    /// De-reference operator.
    ElementType& operator*() const { return *rawPtr_; }
    /// Overloaded member access operator. 
    PointerType operator->() const { return rawPtr_; }
    /// Assignment operator.
    template < class U >
    ScopedPtr& operator=( const ScopedPtr< U >& sp ) {
        if( GetPointer( sp ) == rawPtr_ ) return *this;
        Release();
        rawPtr_ = Clone( sp );
        return *this;
    }
    /// Destructor. Drecrement ref counter, delete object and
    /// counter if ref count == 0;
    ~ScopedPtr() { Release(); }
    /// Conversion operator: returns address of member function for further
    /// conversion to e.g. bool. Intended for use in conditional expressions 
    /// such as testing for @c NULL pointers.
    operator BoolType() const { 
        return rawPtr_ != 0 ? &ScopedPtr::BoolTypeImpl : 0; 
    }
private:
    /// Returns raw pointer.
    template < class P >
    friend  typename ScopedPtr< P >::PointerType 
        GetPointer( const ScopedPtr<P>& ); 
    /// Swaps two smart pointers.
    template < class P >
    friend void Swap( ScopedPtr<P>&, ScopedPtr<P>& );
    template < class P >
    friend typename ScopedPtr< P >::PointerType Clone( const ScopedPtr< P >& );
    /// Decrements the ref. counter, deletes object if count is 0
    void Release() {
        delete rawPtr_;
        rawPtr_ = 0;
    }
private:
    /// Shared object.
    PointerType rawPtr_;
};


template < class P >
inline typename ScopedPtr< P >::PointerType 
    GetPointer( const ScopedPtr<P>& sp ) { return sp.rawPtr_; } 

template < class P >
inline void Release( const SmartPtr<P>& sp ) { sp.Release(); }

template < class P >
typename ScopedPtr< P >::PointerType Clone( const ScopedPtr< P >& p ) {
    //Clone method required
    return p.Clone();
}

template < class P > 
inline void Swap( ScopedPtr< P >& p1, ScopedPtr< P >& p2 ) {
    std::swap( p1.rawPtr_, p2.rawPtr_ );
    std::swap( p1.count_,  p2.count_ );
}   

} //namespace

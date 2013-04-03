#ifndef SCOPED_PTR_
#define SCOPED_PTR_

/// @file ScopedPtr.h Implementaion of scoped smart pointer class for automatic memory management.

#include <memory>
#include <cassert>

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
    /// conversion to e.g. bool. Intended for use in conditional expressions such
    /// as testing for @c NULL pointers.
    operator BoolType() const { return rawPtr_ != 0 ? &ScopedPtr::BoolTypeImpl : 0; }
private:
    /// Returns raw pointer.
    template < class P >
    friend  typename ScopedPtr< P >::PointerType GetPointer( const ScopedPtr<P>& ); 
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
inline typename ScopedPtr< P >::PointerType GetPointer( const ScopedPtr<P>& sp ) { return sp.rawPtr_; } 

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

#endif // SCOPED_PTR_


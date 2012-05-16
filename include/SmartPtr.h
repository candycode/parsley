#ifndef SMART_PTR_
#define SMART_PTR_

/// @file SmartPtr.h Implementaion of smart pointer class for automatic memory management.

#include <memory>
#include <cassert>

///@brief Reference counter
class RefCounter 
{
public:
    RefCounter( unsigned c = 1 ) : count_(c) {}
    unsigned Count() const { return count_; }
    void Inc() { ++count_; }
    void Dec() { --count_; }
private:
    RefCounter operator=( const RefCounter& );
    unsigned count_; 
};

//------------------------------------------------------------------------------
/// @brief Smart pointer implementation; uses default memory de-allocator.
/// @ingroup utility
template < class T >
class SmartPtr
{
    /// Type used in the implementation of the the unspecified bool idiom: i.e.
    /// perform automatic conversion to bool type from member function pointer.
    typedef void ( SmartPtr< T >::*BoolType )() const;
    /// Method used in thte unspecified bool idiom.
    void BoolTypeImpl() const {}
public:
    typedef T ElementType;
    typedef ElementType* PointerType;
    /// Default constructor.
    SmartPtr() : rawPtr_( 0 ), count_( 0 ) {}
    /// Explicit constructor from pointer.
    template < class U > explicit SmartPtr(U* p )
    {
        count_ = new RefCounter;
        assert( count_ && "NULL counter" );
        assert( count_->Count() == 1 && "Invalid ref count" );
        rawPtr_ = dynamic_cast< PointerType >( p );
    }
    /// Copy constructor.
    SmartPtr( const SmartPtr& sp )
    {
        rawPtr_ = GetPointer( sp );
        Acquire( GetCounter( sp ) );
    }
    /// Copy constructor from smart pointer of different type.
    template < class U >
    SmartPtr( const SmartPtr< U >& sp )
    {
        rawPtr_ = GetPointer( sp );
        Acquire( GetCounter( sp ) );
    }
    /// De-reference operator.
    ElementType& operator*() const { return *rawPtr_; }
    /// Overloaded member access operator. 
    PointerType operator->() const { return rawPtr_; }
    /// Assignment operator.
    template < class U >
    SmartPtr& operator=( const SmartPtr< U >& sp )
    {
        if( GetPointer( sp ) == rawPtr_ ) return *this;
        Release();
        rawPtr_ = GetPointer( sp );
        Acquire( GetRefCounter( sp ) );
        return *this;
    }
    /// Destructor. Drecrement ref counter, delete object and
    /// counter if ref count == 0;
    ~SmartPtr() { Release(); }
    /// Conversion operator: returns address of member function for further
    /// conversion to e.g. bool. Intended for use in conditional expressions such
    /// as testing for @c NULL pointers.
    operator BoolType() const { return rawPtr_ != 0 ? &SmartPtr::BoolTypeImpl : 0; }
private:
    /// Returns raw pointer.
    template < class P >
    friend  typename SmartPtr< P >::PointerType GetPointer( const SmartPtr<P>& ); 
    /// Returns pointer to reference counter.
    template < class P >
    friend RefCounter* GetCounter( const SmartPtr<P>& );
    /// Returns current reference counter value.
    template < class P >
    friend unsigned GetCount( const SmartPtr<P>& );
    /// Swaps two smart pointers.
    template < class P >
    friend void Swap( SmartPtr<P>&, SmartPtr<P>& );
    /// Increments the reference counter.
    void Acquire( RefCounter* c ) 
    { 
        count_ = c;
        if( c ) c->Inc();
    }
    /// Decrements the ref. counter, deletes object if count is 0
    void Release()
    { 
        if( count_ ) 
        {           
            count_->Dec();  

            if( count_->Count() == 0 ) 
            {
                delete count_;          
                delete rawPtr_;
                count_ = 0;
                rawPtr_ = 0; 
            }
        }

    }
private:
    /// Shared object.
    PointerType rawPtr_;
    /// Reference counter. Shared by all instances of smart pointers pointing
    /// at the same object.
    RefCounter* count_;
};


template < class P >
inline typename SmartPtr< P >::PointerType GetPointer( const SmartPtr<P>& sp ) { return sp.rawPtr_; } 

template < class P >
inline RefCounter* GetCounter( const SmartPtr<P>& sp ) { return sp.count_; }

template < class P >
inline unsigned GetCount( const SmartPtr<P>& sp ) { return sp.count_->Count(); }

template < class P >
inline void Release( const SmartPtr<P>& sp ) { sp.Release(); }

template < class P > 
inline void Swap( SmartPtr< P >& p1, SmartPtr< P >& p2 )
{ 
    std::swap( p1.rawPtr_, p2.rawPtr_ );
    std::swap( p1.count_,  p2.count_ );
}   

#endif // SMART_PTR_


#ifndef ANY_H_
#define ANY_H_

/// @file Any.h Implementation of class to hold instances of any type.

#include <typeinfo>
#include <vector>
#include <iterator>

//------------------------------------------------------------------------------
/// @brief Class that can hold instances of any type.
/// @ingroup utility
class Any
{
public:
    /// Type used by Any::Type() method to signal an empty @c Any instance. 
    struct EMPTY_ {};
    /// Default constructor, sets the internal pointer to @c NULL
    Any() : pval_( 0 ) {}
    /// Constructor accepting a parameter copied into internal type instance.
    template < class ValT >
    Any( const ValT& v ) : pval_( new ValHandler< ValT >( v ) ) {} 
    /// Copy constructor.
    Any( const Any& a ) : pval_( a.pval_ ? a.pval_->Clone() : 0 ) {}
    /// Destructor: deletes the contained data type.
    ~Any() { delete pval_; }
public:
    /// Returns @c true if instance empty.
    bool Empty() const { return pval_ == 0; }
    /// Returns type of contained data or Any::EMPTY_ if instance empty.
    const std::type_info& Type() const
    { 
        //note gcc requires typeid(C) with C != void; compiles on vc++ 2008
        return !Empty() ? pval_->GetType() : typeid( EMPTY_ ); // 
    } 
    /// Swap two Any instances by swapping the internal pointers.
    Any& Swap( Any& a ) { std::swap( pval_, a.pval_ ); return *this; }
    /// Assignment
    Any& operator=( const Any& a ) { Any( a ).Swap( *this ); return *this; }
    /// Assignment from non - @c Any value.
    template < class ValT > 
    Any& operator=( const ValT& v ) { Any( v ).Swap( *this ); return *this; }
    /// Equality: check by converting value to contained value type then
    /// invoking equality operator on converted type.
    template < class ValT > 
    bool operator==( const ValT& v ) const
    { 
        CheckAndThrow< ValT >(); 
        return (static_cast< ValHandler<ValT>* >( pval_ )->val_ ) == v;
    }
public:
     ///Convert to const reference.
    template < class ValT > operator const ValT&() const
    { 
        CheckAndThrow< ValT >();    
        return (static_cast< ValHandler<ValT>* >( pval_ )->val_ );
    }
    ///Convert to reference.
    template < class ValT > operator ValT&() const
    { 
        CheckAndThrow< ValT >();
        return ( static_cast< ValHandler<ValT>* >( pval_ )->val_ );
    }
    ///Convert to pointer.
    template < class ValT > operator ValT*() const
    { 
        CheckAndThrow< ValT >();
        return &( static_cast< ValHandler<ValT>* >( pval_ )->val_ );
    }
    ///Convert to const pointer.
    template < class ValT > operator const ValT*() const
    { 
        CheckAndThrow< ValT >();
        return &( static_cast< ValHandler<ValT>* >( pval_ )->val_ );
    }
private:
    /// Check if contained data is convertible to specific type.
    template < class ValT > void CheckAndThrow() const
    {

#ifdef ANY_CHECK_TYPE
        if( typeid( ValT ) != Type() )
            if( !typeid( ValT ).before( Type() ) ) 
                throw std::logic_error( 
                        (std::string( " Attempt to convert from ") 
                        + Type().name()
                        + std::string( " to " )
                        + typeid( ValT ).name() ).c_str() );
#endif
    }
    /// @interface HandlerBase Wrapper for data storage.
    struct HandlerBase // hint: use small object allocator
    {
        virtual const std::type_info& GetType() const = 0;
        virtual HandlerBase* Clone() const = 0;
        virtual ~HandlerBase() {}
        virtual std::ostream& Serialize( std::ostream& os ) const = 0;
    };

    /// HandlerBase actual data container class.
    template < class T > struct ValHandler :  HandlerBase
    {
        typedef T Type;
        T val_;
        ValHandler( const T& v ) : val_( v ) {}
        const std::type_info& GetType() const { return typeid( T ); }
        ValHandler* Clone() const { return new ValHandler( val_ ); }
        std::ostream& Serialize( std::ostream& os ) const
        {
            os << val_;
            return os;
        }
    };

    ///Pointer to contained data: deleted when Any instance deleted.
    HandlerBase* pval_; 

    ///Overloaded operator to serialize data to output streams.
    friend inline std::ostream& operator<<( std::ostream& os, const Any& any )
    {
        if( any.Empty() ) return os;
        return any.pval_->Serialize( os );
    }

};

///Utility function to print the content of an std::vector of Any objects.
std::ostream& operator<<( std::ostream& os, const std::vector< Any >& av )
{
    std::copy( av.begin(), av.end(), std::ostream_iterator< Any >( os, ", " ) );
    return os;
}




#endif // ANY_H_


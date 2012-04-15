/// @file cppscratch.cpp driver program to experiment with c++ techniques

#include <vector>
#include <iostream>
#include <sstream>
#include <deque>
#include <iterator>
#include <algorithm>
#include <cassert>




#if 0
//==============================================================================

//------------------------------------------------------------------------------
template < class T >
struct PrintAction
{
	void operator()( T t ) const { std::cout << t << std::endl; }  
};


template <class SeqT> struct SeqPrintAction
{
	void operator()( const SeqT& v )
	{
		std::copy( v.begin(), v.end(),
			std::ostream_iterator< typename SeqT::value_type >( std::cout, "\n" ) );
	}
};


struct IP
{
	virtual void Print() const  = 0;
	virtual IP* Clone() const = 0;
};

struct P : IP
{
 int i_;
 P() : i_( 412 ) {}
 void Print() const { std::cout << i_ << ' '; }
 P* Clone() const { return new P; }
};

struct P2 : IP
{
	void Print() const { std::cout << "Ciao" << ' '; }
	P2* Clone() const { return new P2; }
};

struct OrP : IP
{
	IP* p1_;
	IP* p2_;
	OrP( const OrP& p ) { p1_ = p.p1_->Clone(); p2_ = p.p2_->Clone(); } 
	OrP( const IP& p1, const IP& p2 ) : p1_( p1.Clone() ), p2_( p2.Clone() ) {}
	void Print() const { assert( p1_ && p2_); p1_->Print(); p2_->Print(); } 
	OrP* Clone() const { assert( p1_ && p2_); return new OrP( *p1_->Clone(), *p2_->Clone() ); }
};


template < class T > struct Factory
{
	static T* Create() { return new T; }
	static T* Create( const T& t ) { return new T( t ); }
	static T* Clone( const T* pt ) { return pt ? Create( *pt ) : 0; } 
};

template < class T = IP, class Ptr = IP*, template < class > class FactoryT = Factory > 
class ValueIP : public T
{
public:
	ValueIP( Ptr pImpl ) : pImpl_( pImpl ) {}
	template < class ObjT >
	ValueIP( const ObjT& obj ) : pImpl_( FactoryT< ObjT >::Create( obj ) ) {}
	void Print() const { assert( pImpl_ ); pImpl_->Print(); }
	ValueIP* Clone() const { return new ValueIP( pImpl_->Clone() ); }
protected:
	Ptr GetImpl() const { return pImpl_; }
private:
	Ptr pImpl_;
};


OrP operator|( const IP& p1, const IP& p2 )
{
	OrP orp( p1, p2 );
	return orp;
}


#include "Any.h"
#include <map>
struct Data
{
	std::map< std::string, Any > data_;
	Any& operator[]( const std::string& k ) { return data_[ k ]; }
	const Any& operator[]( const std::string& k ) const
	{ 
		if( data_.find( k ) == data_.end() ) throw std::logic_error( "Element not in map" );
		return data_.find( k )->second;
	}
};



//==============================================================================
struct IFace
{
	virtual void Bar( int i ) const = 0;
	template < class T > 
	void Foo( T t )
	{
		Bar( t );
	}
};

struct Face : IFace
{
	void Bar( int i ) const { std::cout << i << std::endl; }
};


template < class T >
struct Node
{
	std::string name;
	std::vector< Node< T > > childrens;
	Node() {}
	Node( const std::string& n ) : name( n ) {} 
};

void TestRecursiveTypes()
{
	Node< int > n;
	Face f;
	IFace& i = f;
	i.Foo( 2 );
}
//==============================================================================
typedef std::string String;
template < class T, class SkipT > class Tokenizer;

template < class IT > class InChStream
{
public:
	InChStream( IT& is ) : isp_( &is ), lines_( 0 ), lineChars_( 0 ), chars_( 0 ) {}
	typedef  typename IT::char_type char_type;
	char_type get() 
	{
		assert( isp_ != 0 && "NULL STREAM POINTER" );
		// characters available: get from queue
		char_type c = 0;
		if( !queue_.empty() )
		{
			char_type c = queue_.front();
			queue_.pop_front();
		}
		else if( isp_->good() )
		{
			c = isp_->get();
		}
		// read from stream
		else
		{
			throw std::logic_error( "Attempt to read from invalid stream" );
			return 0; // in case exception handling disabled
		}
		if( c == '\n' )
		{
			++lines_;
			lineChars_ = 0;
		}
		else ++lineChars_;
		++chars_;
		return c;
	}
	void putback( char_type c )
	{
		if( c == '\n' ) --lines_;
		--chars_;
		queue_.push_front( c );
	}
	void putback( const String& s )
	{
		std::copy( s.rbegin(), s.rend(), std::front_inserter( queue_ ) );	
	}
	bool good() const { assert( isp_ != 0 && "NULL STREAM POINTER" ); return isp_->good(); }

	int get_total_chars() const { return chars_; }
	int get_lines() const { return lines_; }
	int get_line_chars() const { return lineChars_; }

private:
	IT* isp_;
	std::deque< char_type > queue_;
	int lines_;
	int lineChars_;
	int chars_;
};

typedef InChStream< std::istream > InCharStream;


//------------------------------------------------------------------------------
struct SkipBlanks
{
	typedef InCharStream InStream;
	typedef InStream::char_type Char;
	void Skip( InStream& is ) const
	{
		if( !is.good() ) return;
		std::istream::char_type c = is.get();
		while( ::isspace( c ) != 0 ) c = is.get();
		if( is.good() ) is.putback( c );
	}
	bool IsBlank( Char c ) const
	{
		return ::isspace( c ) != 0;
	}
};

//------------------------------------------------------------------------------
template < class SkipT > 
class Tokenizer< unsigned, SkipT > : public SkipT
{
public:
	typedef InCharStream InStream;
	typedef typename InStream::char_type Char;
	bool NextToken( InStream& is )
	{
		if( !is.good() ) return false;
		token_.clear();
		Skip( is ); // skip blanks
		if( !is.good() ) return false;
		Char c = is.get();
		if( ::isdigit( c ) == 0 )
		{
			is.putback( c );
			return false;
		}
		token_.push_back( c );
		Validate( is );
		return !token_.empty();
	}
	
	const String& GetText() const { return token_; } 

	operator unsigned()	const { return unsigned( ::atoi( token_.c_str() ) ); }

private:
	void Validate( InStream& is ) 
	{
		if( !is.good() ) return;
		Char c = is.get();
		while( is.good() && ::isdigit( c ) != 0 )
		{
			token_.push_back( c );
			c = is.get();
		}
		if( is.good() )
		{
			if( !IsBlank( c ) )
			{
				is.putback( token_ );
				token_.clear();
			}
			else is.putback( c );
		}
	}

	String token_;
};


//------------------------------------------------------------------------------
template  < class SkipT > 
class Tokenizer< int, SkipT  > : public SkipT
{
public:
	typedef InCharStream InStream;
	typedef typename InStream::char_type Char;
	bool NextToken( InStream& is )
	{
		if( !is.good() ) return false;
		token_.clear();
		Skip( is );
		if( !is.good() ) return false;
		const Char c = is.get();
		if( c != '+' && c != '-' && ( ::isdigit( c ) == 0 ) )
		{
			is.putback( c );
			return false;
		}
		if( ::isdigit( c ) != 0 ) is.putback( c );
		Tokenizer< unsigned, SkipT > ut;
		const bool b = ut.NextToken( is );
		if(  c == '+' && b  ) token_ = ut.GetText();
		else if( c == '-' && b ) token_ = "-" + ut.GetText();
		else if( b ) token_ = ut.GetText();
		return b;
	}
	
	const String& GetText() const { return token_; }

	operator int() const { return ::atoi( token_.c_str() );	}
private:
	std::string token_;
};

//------------------------------------------------------------------------------
template < class T >
struct PrintAction
{
	void operator()( T t ) const { std::cout << t << std::endl; }  
};

//------------------------------------------------------------------------------
template < 
		   class T, 
		   class SkipBlanksT,
		   class ActionT,
		   template < class, class > class TokenizerT = Tokenizer
		 >
class Parser
{
public:
	typedef InCharStream InStream;
	bool Parse( InStream& i )
	{
		if( tokenizer_.NextToken( i ) )
		{
			action_( tokenizer_ );
			return true;
		}
		return false;
	}
private:
	TokenizerT< T, SkipBlanksT > tokenizer_;
	ActionT action_;
};

#endif 

//==============================================================================
int main(int argc, char* argv[])
{
	//const String in( "   54" );
	//Parser< int, SkipBlanks, PrintAction< int > > p;
	//std::istringstream iss( in );
	//InCharStream ics( iss );
	//p.Parse( ics );
	std::cin.get();
	return 0;
}


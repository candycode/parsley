#ifndef HPARSER_H_
#define HPARSER_H_
#include <iostream>
#include <list>
#include <cassert>
#include <string>
#include <cctype>
#include <algorithm>
#include <iterator>
#include <vector>
#include <map>
#include <queue>

typedef std::istream InStream; // consider using a TextInStream class to be able to get and put back lines
typedef std::string String;

class InText
{
public:
	bool Valid() const { return isp_ != 0 && isp_->good(); }
	bool GetLine( std::string& s )
	{
		if( isp_ == 0 || !isp_->good() ) return false;
		if( queue_.empty() ) 
		{
			std::getline( *isp_, line_ );
			queue_.push( line_ );
		}
		
		s = queue_.front();
		queue_.pop();
		return isp_->good();
	}
	bool Empty() const { return queue_.empty(); }
	void PushLine( const String& s )
	{
		if( queue_.empty() ) queue_.push( s );
	}
	void SetInStream( std::istream& is ) { isp_ = &is; }
	std::istream& GetStream() { assert( isp_ && "NULL STREAM" ); return *isp_; }
	InText() : isp_( 0 ), locked_( false ) {}
	InText( std::istream& is ) : isp_( &is ), locked_( false ) {} 
private:
	String line_;
	String prevLine_;
	std::istream* isp_;
	bool locked_;
	std::queue< String > queue_;
};

//------------------------------------------------------------------------------
//interface ValueManagerT 
//{
//	void StorageBegin( const String& name );
//	void Store( const String& key, const String& value );
//	void Store( const String& key, double value );
//	void Store( const String& key, int value );
//	void Store( const String& key, unsigned int value );
//	template < class FwdIt > void Store( const String& key, FwdIt b, FwdIt e );
//	void StorageEnd();
//}
template < class ValueManagerT /*, class StateT*/ >
struct IHParser
{
	virtual void SetInText( InText* /*is*/ ) = 0;
	virtual IHParser* AddParser( IHParser* /*p*/ ) = 0;
	virtual bool Parse( const String& /*trigger*/, ValueManagerT&  ) = 0;
	virtual IHParser* AddLineParser( IHParser* /*p*/ ) { throw std::logic_error( "AddLineParser not implemented" ); return 0; } 
	virtual void Clear() = 0;
	virtual const String& GetError() const = 0;
	virtual const String& GetWarning() const = 0; 
	virtual ~IHParser() {}
	virtual bool Enabled() const = 0;
	virtual void Enable( bool /*enable*/ ) = 0;
	virtual const String& GetUnparsedText() const = 0;
	virtual const String& GetIdentifier() const  = 0;
//protected:
//	virtual StateT& GetState() = 0;
};

//==============================================================================
class ValueManager 
{
public:
	class ValueNode
	{
	public:
		typedef std::vector< String > ValueStrings;
		typedef std::vector< int > ValueInts;
		typedef std::vector< unsigned int > ValueUInts;
		typedef std::vector< double > ValueDoubles;
		enum Type { INVALID = -1,
					BEGIN,
					END,
					EMPTY,
					STRING,
					DOUBLE,
					INT,
					UINT,
					STRING_SEQ,
					DOUBLE_SEQ,
					INT_SEQ,
					UINT_SEQ };
		ValueNode() : type_( INVALID ) {}
		ValueNode( Type type ) : type_( type ) {}
		ValueNode( const String& name, Type type ) : key_( name ), type_( type ) {}
		ValueNode( const String& name ) : key_( name ), type_( EMPTY ) {}
		ValueNode( const String& k, const String& v )
			: key_( k ), valueString_( v ), type_( STRING ) {}
		ValueNode( const String& k, double v )
			: key_( k ), valueDouble_( v ), type_( DOUBLE ) {}
		ValueNode( const String& k, int v )
			: key_( k ), valueInt_( v ), type_( INT ) {}
		ValueNode( const String& k, unsigned int v )
			: key_( k ), valueUInt_( v ), type_( UINT ) {}
		template < class FwdIt >
		ValueNode( const String& k, FwdIt b, FwdIt e  )
			: key_( k )
		{
			Construct( b, e, std::iterator_traits< FwdIt >::value_type() );
		}
		Type GetType() const { return type_; }
		const String& GetName() const { return key_; }
	private:
		template < class FwdIt > void Construct( FwdIt b, FwdIt e, const String& )
		{
			valueStringSeq_ = ValueStrings( b, e );
			type_ = STRING_SEQ;
		}
		template < class FwdIt > void Construct( FwdIt b, FwdIt e, const int& )
		{
			valueIntSeq_ = ValueInts( b, e );
			type_ = INT_SEQ;
		}
		template < class FwdIt > void Construct( FwdIt b, FwdIt e, const unsigned int& )
		{
			valueUIntSeq_ = ValueUInts( b, e );
			type_ = UINT_SEQ;
		}
		template < class FwdIt > void Construct( FwdIt b, FwdIt e, const double& )
		{
			valueDoubleSeq_ = ValueDoubles( b, e );
			type_ = DOUBLE_SEQ;
		}
	private:
		inline friend std::ostream& operator<<( std::ostream& os, const ValueNode& vn )
		{
			switch( vn.GetType() )
			{
			case ValueNode::STRING: os << vn.key_ << " = " << vn.valueString_;
									break;
			case ValueNode::INT:    os << vn.key_ << " = " << vn.valueInt_;
									break;
			case ValueNode::UINT:   os << vn.key_ << " = " << vn.valueUInt_;
									break;
			case ValueNode::DOUBLE: os << vn.key_ << " = " << vn.valueDouble_;
									break;
			default: break;
			}
			return os;
		}
		Type type_;
		String key_;
		int valueInt_;
		unsigned int valueUInt_;
		double valueDouble_;
		String valueString_;
		ValueInts valueIntSeq_;
		ValueUInts valueUIntSeq_;
		ValueDoubles valueDoubleSeq_;
		ValueStrings valueStringSeq_;
	};


public:
	typedef std::vector< ValueNode > Values;
	virtual void StorageBegin( const String& name )
	{
		values_.push_back( ValueNode( name, ValueNode::BEGIN ) );
		tmpName_ = name;
	}
	virtual void Store( const String& key, const String& value ) { Push( key, value ); } 
	virtual void Store( const String& key, double value ) { Push( key, value ); } 
	virtual void Store( const String& key, int value ) { Push( key, value ); } 
	virtual void Store( const String& key, unsigned int value ) { Push( key, value ); }
	virtual void StorageEnd()
	{
		values_.push_back( ValueNode( tmpName_, ValueNode::END ) );
	}
	template < class FwdIt >
	void Store( const String& key, FwdIt b, FwdIt e ) { Push( key, b, e ); }

	typedef Values::const_iterator ConstValueIterator;

	ConstValueIterator ValuesBegin() const { return values_.begin(); }
	ConstValueIterator ValuesEnd()   const { return values_.end();   }

private:
	template < class V > void Push( const String& k, const V& v )
	{
		values_.push_back( ValueNode( k, v ) );
	}
	template < class FwdIt > void Push( const String& k, FwdIt b, FwdIt e )
	{
		values_.push_back( ValueNode( k, b, e ) );
	}	
	String tmpName_;
	Values values_;
};


//-----------------------------------------------------------------------------
template< class FwdIt > 
inline bool IsBlank( FwdIt b, FwdIt e )
{
	return
		std::find_if( b, e,
			std::not1( 
				std::ptr_fun( ::isspace )
				)
			) == e;
}


//==============================================================================
template < class StateT, class VMT = ValueManager >
class HParser : public IHParser< VMT >
{
public:
	
	bool Enabled() const { return enabled_ && !parsers_.empty(); }

	void Enable( bool enable ) { enabled_ = enable; }

	HParser( const String& name = "Parser",
			 bool ownMemory = false ) 
			 : in_( 0 ), ownMemory_( ownMemory ), enabled_( true ), identifier_( name ), lineParser_( 0 ) {}
	
	HParser( const String& name, InText* it, bool ownMemory  ) : 
		in_( it ), ownMemory_( ownMemory ), enabled_( true ), identifier_( "name" ), lineParser_( 0 ) {}
	
	void SetInStream( InStream& is ) { in_->SetInStream( is ); }

	void SetInText( InText* it ) { in_ = it; }
	
	HParser* AddParser( IHParser* p )
	{ 
		assert( p != 0 && "NULL parser" );
		if( p == 0 )
		{
			throw std::logic_error( "NULL parser" );
			return 0; // in case exception handling not enabled
		}
		p->SetInText( in_ );
		parsers_.push_back( p );
		return this;
	}

	HParser* AddParser( IHParser* p, int count )
	{ 
		assert( p != 0 && "NULL parser" );
		if( p == 0 )
		{
			throw std::logic_error( "NULL parser" );
			return 0; // in case exception handling not enabled
		}
		p->SetInText( in_ );
		parsers_.push_back( p );
		parsersCount_[ p ] = count;
		return this;
	}

	const String& GetUnparsedText() const { return GetBuffer(); }

	HParser* AddLineParser( HParser* lp ) { lineParser_ = lp; return this; }

	bool Parse( const String& tr, VMT& vm )
	{
		bool readLine = tr.empty();
		if( !tr.empty() ) buffer_ = tr;
		vm.StorageBegin( GetIdentifier() );
		ParserPtrList::iterator it = parsers_.begin();
		int c = 0;
		bool ret = false;
		while( ValidStream() || !readLine )
		{
			if( readLine ) GetLine();
			readLine = true;
			const bool m = ( *it )->Parse( GetBuffer(), vm );
			if( !m ) 
			{ 
				//if child parser has not succeeded in parsing then
				//the text buffer contains the text that was not parsed
				//so reset the 
				if( !EmptyBuffer()  )
				{
					c = 0;
					it = parsers_.begin();
					continue;
				}
				PushLine();
				break;
			}
			++c;
			if( c >= parsersCount_[ *it ] &&  parsersCount_[ *it ] > 0 )
			{
				++it;
				c = 0;
			}
			if( it == parsers_.end() ) break;
		}
		vm.StorageEnd();
		for( ParserPtrList::iterator it = parsers_.begin();
			 it != parsers_.end();
			 ++it )
		{
			if( (*it)->GetError().length()   != 0 ) errorMsg_    += (*it)->GetError()   + '\n';
			if( (*it)->GetWarning().length() != 0 ) warningMsg_  += (*it)->GetWarning() + '\n';
		}
		return ( it == parsers_.end() );
	}

	const String& GetIdentifier() const { return identifier_; }

	const String& GetBuffer() const { return buffer_; }

	void Clear()
	{
		for( ParserPtrList::iterator it = parsers_.begin();
			 it != parsers_.end();
			 ++it )
		{
			( *it )->Clear();
			delete *it;
		}
	}
	const String& GetError() const { return errorMsg_; }

	const String& GetWarning() const { return warningMsg_; }
	
	~HParser() { if( ownMemory_ ) Clear(); }

private:
	
	bool EmptyBuffer() const { return in_->Empty(); }

	HParser( const HParser& );
	
	HParser& operator=( const HParser& );

	bool ValidStream() const
	{
		return in_->Valid();
	}

	bool GetLine()
	{
		while( in_->GetLine( buffer_ ) &&
			   IsBlank( buffer_.begin(), buffer_.end() ) );	 
		return in_->Valid();
	}

	void PushLine() { in_->PushLine( buffer_ ); }

private:

	InText* in_;
	bool ownMemory_;
	bool enabled_;
	typedef std::list< IHParser* > ParserPtrList;
	ParserPtrList parsers_;
	std::map< IHParser*, int > parsersCount_;
	String buffer_;
	String errorMsg_;
	String warningMsg_;
	String identifier_;
	IHParser* lineParser_;
};


//==============================================================================
template < class ValueManagerT = ValueManager >
class TokenParser : public HParser< ValueManagerT >
{
public:
	TokenParser( const String& name = "TokenParser",
				 bool ownMemory = false ) : HParser( name, ownMemory ), match_( false ) {}
	TokenParser( const String& name,
				 InText* i,
				 bool ownMemory  ) : HParser( name, i, ownMemory ) {}
	TokenParser* AddToken( const String& t ) { tokens_.push_back( t ); return this; }
	bool Match( const String& tr )
	{
		if( tr.empty()  ) return false;
		String::size_type offset = String::size_type();
		Tokens::const_iterator ti = tokens_.begin();
		while( ti != tokens_.end() )
		{
			String::size_type b = tr.find( *ti, offset );
			if( b == String::npos) break;
			offset = b + ti->length();
			++ti;
		}
		match_ = ti == tokens_.end();
		return match_;
	}
	bool Parse( const String& tr, ValueManagerT& /*vm*/ ) { return Match( tr ); }
	
	virtual bool ParseValue() const { return false; }

private:
	typedef std::list< String > Tokens;
	Tokens tokens_;
	bool match_;
};

//==============================================================================
template < class ValueManagerT = ValueManager >
class PassThruParser : public HParser< ValueManagerT >
{
public:
	PassThruParser( const String& name = "TokenParser",
				 bool ownMemory = false ) : HParser( name, ownMemory ){}
	PassThruParser( const String& name,
				 InText* i,
				 bool ownMemory  ) : HParser( name, i, ownMemory ) {}
	bool Parse( const String& tr, ValueManagerT& /*vm*/ ) { return true; }
	
	virtual bool ParseValue() const { return false; }
};


//==============================================================================
struct TypedToken
{
	typedef String::size_type Index;
	virtual const type_info& GetType() const = 0;
	virtual const String& GetToken() const = 0;
	virtual Index Match( const String& /*text*/ ) const = 0;
	virtual String::size_type Parse( const String& /*text*/ ) = 0;
	virtual TypedToken* Clone() const = 0; // this method should allow to pass an allocator object
	virtual bool ParseValue() const = 0;
	virtual const String& GetIdentifier() const  = 0;
};

class ConstStringToken : public TypedToken
{
public:
	ConstStringToken( const String& st ) : token_( st ) {}
	virtual const type_info& GetType() const { return typeid( String ); }
	virtual const String& GetToken() const { return token_; }
	virtual Index Match( const String& s ) const
	{
		if( s.find( token_, 0 ) != String::npos ) return token_.length();
		else return 0;
	}
	virtual String::size_type Parse( const String& s )
	{
		 return Match( s );
	}
	virtual ConstStringToken* Clone() const
	{
		return new ConstStringToken( token_ );

	}
	virtual bool ParseValue() const { return false; }
	virtual const String& GetIdentifier() const { return token_; }
private:
	String token_;
};

class AlphaNumStringToken : public TypedToken
{
public:
	AlphaNumStringToken( const String& id = typeid( String ).name() ) : identifier_( id ) {}
	virtual const type_info& GetType() const { return typeid( String ); }
	virtual const String& GetToken() const { return token_; }
	virtual Index Match( const String& s ) const 
	{
		String::const_iterator i = s.begin();
		for( ; i != s.end() && ::isalnum( *i );  ++i );
		return i - s.begin(); // String::difference_type to String::size_type

	}
	virtual String::size_type Parse( const String& s )
	{
		if( !::isalpha( *s.begin() ) ) return 0;
		String::const_iterator i = s.begin();
		for( ; i != s.end() && ::isalnum( *i ); ++i );
		token_ = String( s.begin(), i );
		return token_.length();
	}
	virtual AlphaNumStringToken* Clone() const 
	{ 
		AlphaNumStringToken* ant = new AlphaNumStringToken;
		ant->token_ = token_;
		return ant;
	}
	virtual bool ParseValue() const { return true; }
	virtual const String& GetIdentifier() const { return identifier_; }
private:
	String token_;
	String identifier_;
};

class UIntToken : public TypedToken
{
public:
	UIntToken( const String& id = typeid( unsigned int ).name() ) : identifier_( id ) {}
	virtual const type_info& GetType() const { return typeid( unsigned int ); }
	virtual const String& GetToken() const { return token_; }
	virtual Index Match( const String& s ) const 
	{
		if( s.empty() || !::isdigit( *s.begin() ) ) return 0;
		String::const_iterator i = s.begin();
		for( ; i != s.end() && ::isdigit( *i ) ; ++i );
		return i - s.begin();
	}
	virtual String::size_type Parse( const String& s )
	{
		token_ = "";
		if( s.empty() || !::isdigit( *s.begin() ) ) return 0;
		String::const_iterator i = s.begin();
		for( ; i != s.end() && ::isdigit( *i ) ; ++i );
		token_ = String( s.begin(), i );
		return token_.length();
	}
	virtual UIntToken* Clone() const
	{
		UIntToken* uit = new UIntToken;
		uit->token_ = token_;
		return uit;
	}
	virtual bool ParseValue() const { return true; }
	virtual const String& GetIdentifier() const { return identifier_; }
private:
	String token_;
	String identifier_;
};

class DoubleToken : public TypedToken
{
public:
	DoubleToken( const String& id = typeid( double ).name()) : identifier_( id ) {} 
	virtual const type_info& GetType() const { return typeid( double ); }
	virtual const String& GetToken() const { return token_; }
	virtual Index Match( const String& s ) const 
	{
		if( *s.begin() == '+' || *s.begin() == '-' ) CheckAndApply( &DoubleToken::MatchUnsigned, s );
		else if( *s.begin() == '.' ) CheckAndApply( &DoubleToken::MatchFractional, s );
		else if( ::isdigit( *s.begin() ) ) Apply( &DoubleToken::MatchUnsigned, s );
		return tmpToken_.length();
	}
	virtual String::size_type Parse( const String& s )
	{
		/*if( tmpToken_.length() == 0 )*/ Match( s );
		token_ = tmpToken_;
		if( token_.find( '.' ) == String::npos ) token_ = "";
		tmpToken_.clear();
		return token_.length();		
	}
	virtual DoubleToken* Clone() const
	{
		DoubleToken* dt = new DoubleToken;
		dt->token_ = token_;
		return dt;
	}
	virtual bool ParseValue() const { return true; }
	virtual const String& GetIdentifier() const { return identifier_; }

private:
	typedef bool( DoubleToken::*MatchMethod )( const String& ) const; 

	bool Apply( MatchMethod f, const String& s ) const
	{
		tmpToken_ += *s.begin();
		const String tmp = tmpToken_;
		if( !(this->*f)( String( ++s.begin(), s.end() ) ) ) tmpToken_ = tmp;
		return true;
	}

	bool CheckAndApply( MatchMethod f, const String& s ) const
	{
		const String tmp = tmpToken_;
		tmpToken_ += *s.begin();
		const bool m = (this->*f)( String( ++s.begin(), s.end() ) );
		if( !m ) tmpToken_ = tmp;
		return m;
	}

	bool MatchUnsigned( const String& s ) const
	{
		if( s.empty() ) return false;
		if( ::isdigit( *s.begin() ) ) return Apply( &DoubleToken::MatchUnsigned, s );
		else if( *s.begin() == '.' ) return CheckAndApply( &DoubleToken::MatchFractional, s );
		return false;
	}
	bool MatchFractional( const String& s ) const
	{
		if( s.empty() ) return false;
		if( ::isdigit( *s.begin() ) ) return Apply( &DoubleToken::MatchFractional, s );
		else if( *s.begin() == 'E' || *s.begin() == 'D' || 
			     *s.begin() == 'e' || *s.begin() == 'd' )
		{
			return CheckAndApply( &DoubleToken::MatchExponent, s );
		}		
		return false;
	}
	bool MatchExponent( const String& s ) const
	{
		if( s.empty() ) return false;
		if( *s.begin() == '+' || *s.begin() == '-' ) return CheckAndApply( &DoubleToken::MatchExponent, s );
		else if( ::isdigit( *s.begin() ) ) return Apply( &DoubleToken::MatchExponent, s );
		return false;
	}
	String token_;
	mutable String tmpToken_; //used by match
	String identifier_;
};

template < class ValueManagerT = ValueManager > 
class TypedTokenParser : public HParser< ValueManagerT >
{
public:
	TypedTokenParser( const String& name = "TypedTokenParser", 
					  int c = 1,
					  bool ownMemory = false ) : HParser( name, ownMemory ), counter_( c ) {}
	
	TypedTokenParser( const String& name,
					  InText* i,
					  int c,
					  bool ownMemory  ) : HParser( name, i, ownMemory ), counter_( c ) {}
	
	void SetCount( int c ) { counter_ = c; }
	
	TypedTokenParser* AddToken( TypedToken* tt )
	{ 
		tokens_.push_back( tt );
		return this;
	}
	const String& GetUnparsedText() const { return unparsedText_; }
	bool Parse( const String& tr, ValueManagerT& vm ) 
	{
		if( tr.empty() ) return false;
		unparsedText_ = tr;
		String::const_iterator i = tr.begin();
		TypedTokensPtr::const_iterator ti = tokens_.begin();
		vm.StorageBegin( GetIdentifier() );
		while( i != tr.end() &&  ti != tokens_.end() )
		{
			if( IsBlank( *i ) )
			{
				++i;
				continue;
			}
			const String::size_type e = (*ti)->Parse( String( i, tr.end() ) ); 
			if( e != 0  )//&& (  != tr.end() || IsBlank( *( i + e ) ) ) ) 
			{			
				if( (*ti)->ParseValue() ) parsedTokens_.push_back( (*ti)->Clone() );
				i += e;
				++ti;
			}
			else break;
		}
		vm.StorageEnd();
		//--counter_;
		//Enable( true );
		const bool m = ti == tokens_.end();
		if( m )
		{
			unparsedText_.clear();
			for( ti = parsedTokens_.begin(); ti != parsedTokens_.end(); ++ti )
			{
				if( (*ti)->GetType() == typeid( unsigned int ) )
				{
					vm.Store( (*ti)->GetIdentifier(), ( unsigned int ) ::atoi( (*ti)->GetToken().c_str() ) ) ;
				}
				else if ( (*ti)->GetType() == typeid( int ) )
				{
					vm.Store( (*ti)->GetIdentifier(), ( int ) ::atoi( (*ti)->GetToken().c_str() ) );
				}
				else if ( (*ti)->GetType() == typeid( double ) )
				{
					vm.Store( (*ti)->GetIdentifier(), ( double ) ::atof( (*ti)->GetToken().c_str() ) );
				}
				else if ( (*ti)->GetType() == typeid( String ) )
				{
					vm.Store( (*ti)->GetIdentifier(), (*ti)->GetToken() );
				}
			}
		}
		parsedTokens_.clear();
		return m;
	}
	void Clear()
	{
		for( TypedTokensPtr::iterator ti = tokens_.begin();
			  ti != tokens_.end();
			  ++ti ) delete *ti;
		parsedTokens_.clear();
		HParser::Clear();
	}

	typedef std::list< TypedToken* > TypedTokensPtr;

	TypedTokensPtr::const_iterator GetParsedTokensPtrBegin() const { return parsedTokens_.begin(); }

	TypedTokensPtr::const_iterator GetParsedTokensPtrEnd() const { return parsedTokens_.begin(); }
	

private:
	bool IsBlank( String::value_type c ) { return ::isspace( c ) != 0; }
	TypedTokensPtr tokens_;
	TypedTokensPtr parsedTokens_;
	int counter_;
	String unparsedText_;
};


#endif // HPARSER_H_
/// @file parsertest.cpp Test/sample code.

#include <iostream>
#include <cassert>
#include <sstream>

#include "types.h"
#include "InStream.h"
#include "parsers.h"
#include "parser_operators.h"
#include "ParserManager.h"


typedef AlphaNumParser A;
typedef FirstAlphaNumParser FA;
typedef UIntParser U;
typedef FloatParser F;
typedef String S;
typedef ConstStringParser C;
typedef NotParser<ConstStringParser> NC;
typedef NotParser< AndParser > NAL;
typedef AndParser AL;
typedef OrParser OL;
typedef TupleParser<-1> T;
typedef TupleParser<3> T3;
typedef TupleParser<2> T2;
static SkipBlankParser __;
static BlankParser _;
static SkipToNextLineParser endl_;
static EofParser eof_;
const bool SKIP_BLANKS = true;
const bool DONT_SKIP_BLANKS = !SKIP_BLANKS;
const bool SKIP_TO_NEXT_LINE = true;
const bool DONT_SKIP_TO_NEXT_LINE = !SKIP_TO_NEXT_LINE;

//==============================================================================
//                               BASIC TESTS
//==============================================================================
// TEST LEXERS & TOKENIZERS
void TestTokenizers()
{
	std::cout << "TOKENIZER TEST" << std::endl;
	std::cout << "==============" << std::endl;
	int count = 0;
	//1 int
	std::istringstream iss( "-123" );
	InStream is( iss );
	IntParser il( "n" );
	assert( il.Parse( is ) );	
	assert( il.GetValues().find( "n" )->second == -123 );
	std::cout << ++count << ' ';
	//2 unsigned int
	std::istringstream iss2( "123" );
	InStream is2( iss2 );
	UIntParser il2( "n" );
	assert( il2.Parse( is2 ) );	
	assert( il2.GetValues().find( "n" )->second == unsigned( 123 ) );
	std::cout << ++count << ' ';
	//3 float 
	std::istringstream iss3( "12.3" );
	InStream is3( iss3 );
	FloatParser il3( "n" );
	assert( il3.Parse( is3 ) );	
	assert( il3.GetValues().find( "n" )->second == 12.3 );
	std::cout << ++count << ' ';
	//4 float 
	std::istringstream iss4( "-12" );
	InStream is4( iss4 );
	FloatParser il4( "n" );
	assert( il4.Parse( is4 ) );	
	assert( il4.GetValues().find( "n" )->second == -12.0 );
	std::cout << ++count << ' ';
	//5 float 
	std::istringstream iss5( "-.12" );
	InStream is5( iss5 );
	FloatParser il5( "n" );
	assert( il5.Parse( is5 ) );	
	assert( il5.GetValues().find( "n" )->second == -.12 );
	std::cout << ++count << ' ';
	//6 float 
	std::istringstream iss6( "-.12E-01" );
	InStream is6( iss6 );
	FloatParser il6;
	assert( il6.Parse( is6 ) );	
	assert( il6.GetValues().find( "" )->second == -.012 );
	std::cout << ++count << ' ';
	//7 float 
	std::istringstream iss7( ".12E1abc" );
	InStream is7( iss7 );
	FloatParser il7;
	assert( il7.Parse( is7 ) );	
	assert( il7.GetValues().find( "" )->second == 1.2 );
	assert( is7.get() == 'a' );
	assert( is7.get() == 'b' );
	assert( is7.get() == 'c' );
	std::cout << ++count << ' ';
	//8 float 
	std::istringstream iss8( ".12E+abc" );
	InStream is8( iss8 );
	FloatParser il8( "n" );
	assert( il8.Parse( is8 ) );	
	assert( il8.GetValues().find( "n" )->second == 0.12 );
	assert( is8.get() == 'E' );
	assert( is8.get() == '+' );
	assert( is8.get() == 'a' );
	assert( is8.get() == 'b' );
	assert( is8.get() == 'c' );
	std::cout << ++count << ' ';
	//9 float 
	std::istringstream iss9( "+1.D2" );
	InStream is9( iss9 );
	FloatParser il9( "n" );
	assert( il9.Parse( is9 ) );	
	assert( il9.GetValues().find( "n" )->second == 100.0 );
	std::cout << ++count << ' ';
	//10 float 
	std::istringstream iss10( "+1." );
	InStream is10( iss10 );
	FloatParser il10( "n" );
	assert( il10.Parse( is10 ) );	
	assert( il10.GetValues().find( "n" )->second == 1.0 );
	std::cout << ++count << ' ';
	//11 float 
	std::istringstream iss11( "+.E1" );
	InStream is11( iss11 );
	FloatParser il11( "n" );
	assert( il11.Parse( is11 ) );
	assert( il11.GetValues().find( "n" )->second == 0.0 );
	std::cout << ++count << ' ';
	//12 float 
	std::istringstream iss12( "+.E" );
	InStream is12( iss12 );
	FloatParser il12( "n" );
	assert( il12.Parse( is12 ) == false );
	assert( il12.GetValues().empty() );
	assert( is12.get() == '+' );
	assert( is12.get() == '.' );
	assert( is12.get() == 'E' );
	std::cout << ++count << ' ';
	//13 string 
	std::istringstream iss13( "Hello123" );
	InStream is13( iss13 );
	SequenceParser<> il13;
	assert( il13.Parse( is13 ) );
	assert( il13[ "" ] == String( "Hello123" ) );
	std::cout << ++count << ' ';
	//14 string
	std::istringstream iss14( "Hello123" );
	InStream is14( iss14 );
	SequenceParser< ConstStringValidator > il14( "hello123" );
	assert( il14.Parse( is14 ) == false );
	std::cout << ++count << ' ';
	//assert( il13.GetValues().find( "n" )->second == String( "Hello123" ) );
	std::cout << std::endl;	
}

//==============================================================================
// TEST COMPOSITE PARSERS
void TestParsers()
{
	std::cout << "\nLEXER TEST" << std::endl;
	std::cout << "==============" << std::endl;
	int count = 0;
	//1 AND lexer
	std::istringstream iss( "c 1 6 0.4 -2.3 1.1" );
	InStream is( iss );
	AndParser al( A("n") > U("#") > U("e") > F("x") > F("y") > F("z") );
	assert( al.Parse( is ) );
	assert( al[ "n" ] == String( "c" ) );
	assert( al[ "#" ] == unsigned( 1 ) );
	assert( al[ "e" ] == unsigned( 6 ) );
	assert( al[ "x" ] == 0.4 );
	assert( al[ "y" ] == -2.3 );
	assert( al[ "z" ] == 1.1 );
	std::cout << ++count << ' ';
	//std::cout << String( al[ "n" ] ) << ' ' << unsigned( al[ "#" ] ) << ' ' << unsigned( al[ "e" ] )
	//		  << ' ' << double( al[ "x" ] ) << ' ' << double( al[ "y" ] ) << ' ' << double( al[ "z" ] );
	//2 TUPLE
	std::istringstream iss2( " 2.1   2.2  2.3 " );
	InStream is2( iss2 );
	TupleParser< 3 > tl( FloatParser(), "coord" );
	assert( tl.Parse( is2 ) ); 
	std::vector< Any > v = tl[ "coord" ];
	assert( v[ 0 ] == 2.1 && v[ 1 ] == 2.2 && v[ 2 ] == 2.3 );
	std::cout << ++count << ' ';
	//3 TUPLE
	std::istringstream iss3( " 2.1   2.2  2.3 4" );
	InStream is3( iss3 );
	TupleParser<> tl2( FloatParser(), "coord" );
	assert( tl2.Parse( is3 ) );
	std::vector< Any > v2 = tl2[ "coord" ];
	assert( v2[ 0 ] == 2.1 && v2[ 1 ] == 2.2 && v2[ 2 ] == 2.3 && v2[ 3 ] == 4.0 );
	std::cout << ++count << ' ';
	//4 TUPLE
	std::istringstream iss4( " string1 |  string2  | string3 | string4" );
	InStream is4( iss4 );
	//TupleParser<> tl3( A("string"), "strings", __, AndParser( DONT_SKIP_BLANKS ) >= __ >= C( "|" ) >= __, eof_ | __ );
	Parser tl3( *( (  AL(DONT_SKIP_BLANKS) >= __ >= A("") >= __ >= C("|") ) / ( AL(DONT_SKIP_BLANKS) >= __ >= A("") >=  eof_ ) ) ); 	
	assert( tl3.Parse( is4 ) );
	std::list< Any > vs = tl3[ "" ];
	//assert( vs[ 0 ] == S("string1") && vs[ 1 ] == S("string2") && vs[ 2 ] == S("string3") && vs[ 3 ] == S("string4" ) );
	assert( *(vs.begin()) == S("string1") && *(++vs.begin()) == S("string2") && *(++++vs.begin() ) == S("string3"));// && vs[ 3 ] == S("string4" ) );
	std::cout << ++count << ' ';
	//5 TUPLE
	std::istringstream iss5( "| string 1 |  string 2 | string 3 | string 4 |  gfdgfd" );
	InStream is5( iss5 );
	AndParser separator(DONT_SKIP_BLANKS ); 
	separator >= __ >= C( "|" ) ;
	TupleParser<> tl4( NotParser<OrParser>( separator / NextLineParser() ), "strings", separator > __, separator, PassThruParser(), SKIP_BLANKS );
	assert( tl4.Parse( is5 ) );
	std::vector< Any > vs2 = tl4[ "strings" ];
	assert( vs2.size() == 5 );
	//using a pass-through as the end lexer means we always get something added as the last element
	assert( vs2[ 0 ] == S("string 1") );
	assert( vs2[ 1 ] == S("string 2") );
	assert( vs2[ 2 ] == S("string 3") );
	assert( vs2[ 3 ] == S("string 4") ); 
	std::cout << ++count << ' ';
	//6 OR lexer
	std::istringstream oriss1( "c 1 6 0.4 -2.3 1.1" );
	std::istringstream oriss2( " 2.1   2.2  2.3 " );
	InStream oris1( oriss1 );
	InStream oris2( oriss2 );
	OrParser ol = al / tl;
	assert( ol.Parse( oris1 ) );
	assert( ol[ "n" ] == String( "c" ) );
	assert( ol[ "#" ] == unsigned( 1 ) );
	assert( ol[ "e" ] == unsigned( 6 ) );
	assert( ol[ "x" ] == 0.4 );
	assert( ol[ "y" ] == -2.3 );
	assert( ol[ "z" ] == 1.1 );
	assert( ol.Parse( oris2 ) );
	std::vector< Any > v4 = ol[ "coord" ];
	assert( v4[ 0 ] == 2.1 && v4[ 1 ] == 2.2 && v4[ 2 ] == 2.3 );
	std::cout << ++count << ' ';
	std::cout << std::endl;
}

//==============================================================================
// TEST PARSER FOR EXPRESSION EVALUATION
namespace eval
{
	enum { START, VALUE, OPERATOR, EXPRESSION_BEGIN, EXPRESSION_END, EOF_ };
	class Print : public IStateManager
	{
		enum {VAL, ADD, SUB, MUL, DIV, LEFT_P, RIGHT_P };
	public:
	void UpdateState( IStateController& sc, StateID currentState ) const
	{
	}
	void EnableState( StateID ) {}
	void DisableState( StateID ) {}
	void EnableState( const ParserID& ) {}
	void DisableState( const ParserID& ) {}
	void EnableAllStates() {}
	typedef std::map< ValueID, Any > Values;
	bool HandleValues( StateID sid, const Values& v )
	{
		switch( sid )
		{
		case VALUE: state_->program_.push_back( OP( VAL, v.find("operand")->second ) );
					break;
		case OPERATOR: 
			{
				const String op = v.find( "operator" )->second;
				if( op == "+" ) state_->program_.push_back( OP( ADD ) );
				else if( op == "-" ) state_->program_.push_back( OP( SUB ) );
				else if( op == "*" ) state_->program_.push_back( OP( MUL ) );
				else if( op == "/" ) state_->program_.push_back( OP( DIV ) );
			}
			break;
		case eval::EOF_:  state_->Evaluate( state_->program_.begin() );
					break;
		default:	break;	
		}
		Values::const_iterator i;
		for( i = v.begin(); i != v.end(); ++i )
		{
			std::cout << '(' << i->first << " = " << i->second << ')' << ' ';
		}
		std::cout << '\n';
		return true;
	}
	void HandleError( StateID sid, int /*line*/ )
	{
		std::cerr << "\n\t[" << sid << "] PARSER ERROR" <<  std::endl;
	}
	Print* Clone() const { return new Print( *this ); }

	

	private:
		struct OP
		{
			int code_;
			double value_;
			OP() : code_( -1 ), value_( 0.0 ) {}
			OP( int code, double value = 0.0 ) : code_( code ), value_( value ) {}
		};
	public:	
		struct State 
		{
			typedef std::list< OP > Program;
			Program program_;
			void Evaluate( Program::iterator i )
			{
				if( i == program_.end() ) return;
				if( i->code_ == LEFT_P )
				{
					Evaluate( ++i );
					return;
				}
				//subsitute all mul operations
				Program::iterator start = i;
				while( i != program_.end() && i->code_ != RIGHT_P  )
				{
					if( i->code_ == MUL )
					{
						Program::iterator op1 = i; --op1;
						Program::iterator op2 = i; ++op2;
						i->code_ = VAL;
						i->value_ = op1->value_ * op2->value_;
						i = op2; ++i;
						program_.erase( op1 );
						program_.erase( op2 );
					}
					else if( i->code_ == DIV )
					{
						Program::iterator op1 = i; --op1;
						Program::iterator op2 = i; ++op2;
						i->code_ = VAL;
						i->value_ = op1->value_ / op2->value_;
						i = op2; ++i;
						program_.erase( op1 );
						program_.erase( op2 );

					}
					else ++i;
				}

				i = start;
				double v = i->value_;
				while( i !=  program_.end() && i->code_ != RIGHT_P )
				{
					if( i->code_ == ADD )
					{
						Program::iterator op2 = i; ++op2;
						v += op2->value_;
					}
					else if( i->code_ == SUB )
					{
						Program::iterator op2 = i; ++op2;
						v -= op2->value_;
					}
					++i;
				}

				start->code_ = VAL;
				start->value_ = v;
				i = i != program_.end() ? ++i : i;
				Program::iterator b = start; ++b;
				program_.erase( b, i );

				if( program_.size() > 1 ) Evaluate( start );

				std::cout << "\nEVAL: " << v << std::endl;
			}
		};
		Print( const SmartPtr< State >& s ) : state_( s ) {} 
	private:
		SmartPtr< State > state_;

	};
} //namespace eval

void TestExpressionEvaluator()
{
	std::cout << "\nEXPRESSION EVALUATOR" << std::endl;
	std::cout << "==============" << std::endl;

	std::istringstream iss( "1+3*4-1/3.3" );
	InStream is( iss );
	SmartPtr< eval::Print::State > state( new eval::Print::State );
	StateManager PRINT( ( eval::Print( state ) ) );
	ParserManager pm( PRINT );
	pm.SetBeginEndStates( eval::START, eval::EOF_ );
	pm.AddState( eval::START, eval::VALUE );
	pm.AddState( eval::START, eval::EXPRESSION_BEGIN );
	pm.AddState( eval::EXPRESSION_BEGIN, eval::EXPRESSION_BEGIN );
	pm.AddState( eval::EXPRESSION_BEGIN, eval::VALUE );
	pm.AddState( eval::VALUE, eval::EXPRESSION_END );
	pm.AddState( eval::EXPRESSION_END, eval::OPERATOR );
	pm.AddState( eval::EXPRESSION_END, eval::EXPRESSION_END );
	pm.AddState( eval::EXPRESSION_END, eval::EOF_ );
	pm.AddState( eval::VALUE, eval::OPERATOR ).AddState( eval::VALUE, eval::EOF_ );
	pm.AddState( eval::OPERATOR, eval::VALUE );

	pm.SetParser( eval::VALUE, ( AL( DONT_SKIP_BLANKS ) >= __ >= F("operand") ) );
	pm.SetParser( eval::OPERATOR,  ( AL( DONT_SKIP_BLANKS ) >= __ >= ( C("+","operator") /
																	   C("-","operator") /
																	   C("/","operator") /
																	   C("*","operator") ) ) );
	pm.SetParser( eval::EXPRESSION_BEGIN, ( __ > C("(","open") ) );
	pm.SetParser( eval::EXPRESSION_END,   ( __ > C(")","closed") ) );
	//pm.SetParser( MUL,   Parser( AL( DONT_SKIP_BLANKS ) >= __ >= C("*","operator"), PRINT ) );
	pm.SetParser( eval::EOF_, ( __ > eof_ ) );
	
	
	pm.Apply( is, eval::START );
}


//==============================================================================
//                              ADDITIONAL TESTS
//==============================================================================

//==============================================================================
// READ MOLDEN DATA FROM STRING STREAM AND ACTUAL FILES WITH WIN AND UNIX EOL
//==============================================================================
#if 1

enum { START, MOLDEN_FORMAT, TITLE, TITLE_DATA, ATOMS, ATOM_DATA, GTO,
		   GTO_ATOM_NUM, GTO_SHELL_INFO, GTO_COEFF_EXP, EOL_, EOF_, SKIP_LINE };

class Print : public IStateManager
{
	
public:
	void UpdateState( IStateController& sc, StateID currentState ) const
	{
		if( currentState == GTO_SHELL_INFO )
		{
			sc.EnableState( GTO_COEFF_EXP );
			sc.DisableState( GTO_SHELL_INFO );
			sc.DisableState( GTO_ATOM_NUM );
			sc.DisableState( EOF_ );
		}
		else if( currentState == GTO_COEFF_EXP )
		{
			if( state_->basisFunctionCounter_ == state_->basisFunctionNum_ )
			{
				sc.DisableState( GTO_COEFF_EXP );
				sc.EnableState( GTO_SHELL_INFO );
				sc.EnableState( GTO_ATOM_NUM );
				sc.EnableState( EOF_ );
				state_->basisFunctionCounter_ = 0;
				state_->basisFunctionNum_ = 0;
			}
		}
	}
	void EnableState( StateID ) {}
	void DisableState( StateID ) {}
	void EnableState( const ParserID& ) {}
	void DisableState( const ParserID& ) {}
	void EnableAllStates() {}
	typedef std::map< ValueID, Any > Values;
	bool HandleValues( StateID sid, const Values& v )
	{
		if( sid == EOF_ ) return true;
		//each parser matches one line
		switch( sid )
		{
		case GTO_SHELL_INFO: 
							 state_->basisFunctionNum_ = v.find( "num basis" )->second;
							 state_->basisFunctionCounter_ = 0;
							 break;
		case GTO_COEFF_EXP:  ++state_->basisFunctionCounter_;
							 break;
		default: break;
		}
	
		std::cout << state_->line_ << "> [" << sid << "]: ";
		Values::const_iterator i = v.begin();
		for( ; i != v.end(); ++i )
		{
			std::cout << '(' << i->first << " = " << i->second << ')' << ' ';
		}
		std::cout << '\n';
		return true;
	}
	void HandleError( StateID sid, int lineno )
	{
		std::cerr << "\n\t[" << sid << "] PARSER ERROR AT LINE " << lineno << std::endl;
	}
	Print* Clone() const { return new Print( *this ); }

	///Shared state
	struct State
	{
		unsigned basisFunctionNum_;
		unsigned basisFunctionCounter_;
		unsigned line_;
		State() : basisFunctionNum_( 0 ), basisFunctionCounter_( 0 ), line_( 0 ) {}
	};


	Print( State* s ) : state_( s ) {}

	

private:
	SmartPtr< State > state_;
};

#include <fstream>
//==============================================================================
// TEST:  >>>START
// ...
// [Molden Format] >>>MOLDEN_FORMAT
// [ <Title> ]     >>>TITLE 
// [Atoms] <AU>    >>>ATOMS
//
// c     1    6 -.12795847981101E+01 -.97661648325429E+00 -.23657630908143E+01 >>>ATOM_DATA
// c     2    6 -.21389493998520E+01 0.61566557723072E+00 -.12265666276112E+00 >>>ATOM_DATA
// ...
// [GTO] >>> GTO <= STOP HERE 
void TestMoldenChunk1( /*std::istream& is*/ )
{
	
	std::cout << "\nPARSER TEST" << std::endl;
	std::cout << "==============" << std::endl;

	std::ostringstream oss;
	oss << "fdsafdsafdsafdas\nfdsa\n fdf\n\n\n" //<- garbage: can be skipped with START->SKIP_LINE->START or MOLDEN_FORMAT state loop 
		<< "  [Molden Format]" << '\n'
	    << "  [Title]"		   << '\n'
	    << "  Sample title"    << '\n'
	    << "  [Atoms] AU"      << '\n'
	    << "  c     1    6 1.12795847981101E+01 -.97661648325429E+00 -.23657630908143E+01" << '\n'
	    << "  c     2    6 -.21389493998520E+01 0.61566557723072E+00 -.12265666276112E+00" << '\n'
		<< "  c     2    6 -.21389493998520E+01 0.61566557723072E+00 -.12265666276112E+00" << '\n'
		<< "  c     2    6 -.21389493998520E+01 0.61566557723072E+00 -.12265666276112E+00" << '\n'
		<< "\n\n"
	    << "[GTO]" << '\n'
		<< "  1 0" << '\n' 
		<<  "s    5 1.00" << '\n'
		<<	"0.12384016938000E+04 0.54568832082000E-02\n"
			"0.18629004992000E+03 0.40638409211000E-01\n"
			"0.42251176346000E+02 0.18025593888000E+00\n"
			"0.11676557932000E+02 0.46315121755000E+00\n"
			"0.35930506482000E+01 0.44087173314000E+00\n"
			"s    1 1.00\n"
			"0.40245147363000E+00 0.10000000000000E+01\n"
			"s    1 1.00\n"
			"0.13090182668000E+00 0.10000000000000E+01\n"
			"p    3 1.00\n"
			"0.94680970621000E+01 0.38387871728000E-01\n"
			"0.20103545142000E+01 0.21117025112000E+00\n"
			"0.54771004707000E+00 0.51328172114000E+00\n"
			"p    1 1.00\n"
			"0.15268613795000E+00 0.10000000000000E+01\n"
			"d    1 1.00\n"
			"0.80000000000000E+00 0.10000000000000E+01\n"
			"\n"
			"2 0\n"
			"s    5 1.00\n"
			"0.12384016938000E+04 0.54568832082000E-02\n"
			"0.18629004992000E+03 0.40638409211000E-01\n"
			"0.42251176346000E+02 0.18025593888000E+00\n"
			"0.11676557932000E+02 0.46315121755000E+00\n"
			"0.35930506482000E+01 0.44087173314000E+00\n"
			"s    1 1.00\n"
			"0.40245147363000E+00 0.10000000000000E+01\n"
			"s    1 1.00\n"
			"0.13090182668000E+00 0.10000000000000E+01\n"
			"p    3 1.00\n"
			"0.94680970621000E+01 0.38387871728000E-01\n"
			//"0.20103545142000E+01 0.21117025112000E+00\n" //<- will rise an error if commented
			"0.54771004707000E+00 0.51328172114000E+00\n"
			"p    1 1.00\n"
			"0.15268613795000E+00 0.10000000000000E+01\n"
			"d    1 1.00\n"
			"0.80000000000000E+00 0.10000000000000E+01\n";

	std::istringstream iss( oss.str() );
#ifdef WIN32
	std::ifstream ifs1( "c:\\projects\\parser\\trunk\\data\\molden.input" );
	std::ifstream ifs2( "c:\\projects\\parser\\trunk\\data\\molden.input.win" );
#else
	std::string dir( ::getenv( "HOME" ) );
	std::string lpath = dir + "/projects/parser/trunk/data/molden.input";
	std::string wpath = dir + "/projects/parser/trunk/data/molden.input.win";
	std::ifstream ifs1( lpath.c_str() );
	std::ifstream ifs2( wpath.c_str() );
#endif
	
	InStream is1( ifs1 );
	InStream is2( ifs2 );

	Print::State* state = new Print::State;

	StateManager PRINT( ( Print( state ) ) );
	
	ParserManager pm( PRINT );
	pm.SetBeginEndStates( START, EOF );
	pm.AddState( /*previous*/ START, /*current*/ MOLDEN_FORMAT ).
	   AddState( /*previous*/ START, /*current*/ SKIP_LINE ).
	   AddState( /*previous*/ SKIP_LINE, /*current*/ MOLDEN_FORMAT ).
	   AddState( /*previous*/ SKIP_LINE, /*current*/ START ).
       AddState( /*previous*/ MOLDEN_FORMAT, /* current */ TITLE ).
	   AddState( /*previous*/ TITLE, /*current*/ ATOMS ).
	   AddState( /*previous*/ TITLE, /*current*/ TITLE_DATA ).
	   AddState( /*previous*/ TITLE_DATA, /*current*/ ATOMS ).
	   AddState( /*previous*/ ATOMS, /*current*/ ATOM_DATA ).
	   AddState( /*previous*/ ATOM_DATA, /*current*/ ATOM_DATA ).
	   AddState( /*previous*/ ATOM_DATA, /*current*/ GTO ).
	   AddState( /*previous*/ GTO, /*current*/ GTO_ATOM_NUM ).
	   AddState( GTO_ATOM_NUM, GTO_SHELL_INFO).
	   AddState( GTO_SHELL_INFO, GTO_COEFF_EXP ).
	   AddState( GTO_COEFF_EXP, GTO_SHELL_INFO ).
	   AddState( GTO_COEFF_EXP, GTO_COEFF_EXP ).
	   AddState( GTO_COEFF_EXP, GTO_ATOM_NUM ).
	   AddState( GTO_COEFF_EXP, EOF_ );
	
	//AL BRS; BRS >= C("[") >= NC("]") >= C("]");
	TupleParser<> coord( FloatParser(), "coord", __, _, endl_ );
	pm.SetParser( MOLDEN_FORMAT, ( C("[") > C("Molden Format") > C("]") )  );
	pm.SetParser( START, NotParser< AL >( C("[") > C("Molden Format") > C("]") ) );
	pm.SetParser( SKIP_LINE, endl_ );
	pm.SetParser( TITLE, ( C("[") > C("Title") > C("]") ) );
	pm.SetParser( TITLE_DATA, ( __ > NC("\n") ) );
	pm.SetParser( ATOMS, AL(DONT_SKIP_BLANKS) >= __ >= C("[") >= __ >= C("Atoms") >= __ >= C("]") >= __ >= A("unit") >= endl_ );
	//pm.SetParser( ATOM_DATA, Parser( A("name") > U("#") > U("element") > coord, PRINT ) );
	AL atomDataParser(false);
	atomDataParser >= __ >= A("name") >= _ >= U("#") >= _  >= U("element") >= _ >= F("x") >= _ >= F("y") >= _ >= F("z") >= endl_;
	//atomDataParser >= __ >= A("name") >= _ >= U("#") >= _  >= U("element") >= __ >= coord >= endl_;
	pm.SetParser( ATOM_DATA, atomDataParser );
	//GTO
	pm.SetParser( GTO, ( C("[") > C("GTO") > C("]") ) );
	
	//GTO_ATOM_NUM
	pm.SetParser( GTO_ATOM_NUM, AL( DONT_SKIP_BLANKS ) >= __ >= U("gto atom #") >= endl_  );
	pm.SetParser( GTO_SHELL_INFO, ( AL( DONT_SKIP_BLANKS ) >= __ >= FA("shell") >= _ >= U("num basis" ) >= endl_ ) );
	pm.SetParser( GTO_COEFF_EXP, ( AL( DONT_SKIP_BLANKS ) >= __ >= F("exp") >= _ >= F("coeff") >= endl_ ) ); 
	pm.SetParser( EOF_, ( __ > eof_ ) );
	std::cout << "+++++++++++++++++++++++++++++++++++++++++++++\n";
	std::cout << "++++++++++ UNIX/MacOS X LINE ENDINGS ++++++++\n";
	pm.Apply( is1, START );
	std::cout << "+++++++++++++++++++++++++++++++++++++\n";
	std::cout << "++++++++ WINDOWS LINE ENDINGS +++++++\n";
	pm.Apply( is2, START );

}
#endif

//==============================================================================
// RECURSIVE & CALLBACK PARSERS
//==============================================================================

//------------------------------------------------------------------------------
class RefParser : public IParser
{
public:
	typedef IParser* IParserPtr; //smart pointers ?
	typedef IParser::Values Values;
	RefParser() : ref_( 0 ), memStartPos_( 0 ), memEndPos_( 0 ) {}
	RefParser( IParser& ip ) : ref_( &ip ), memStartPos_( 0 ), memEndPos_( 0 ) {}
	const Values& GetValues() const { return values_; }
	const ValueType& operator[]( const KeyType& k ) const { return ref_->operator[]( k ); }
	bool Parse( InStream& is )
	{
		assert( ref_ && "NULL PARSER REFERENCE" );
        // little memoization logic: if text already parsed simply skip to
        // end of parsed text
		if( memStartPos_ == is.tellg() && !(ref_->GetValues().empty()) )
		{
			is.seekg( memEndPos_ );
			return true;
		}
		const StreamPos p = is.tellg();
		const bool ok = ref_->Parse( is );
		// if parsed record begin and end of parsed text
        if( ok )
		{
			memStartPos_ = p;
			memEndPos_ = is.tellg();
			values_ = ref_->GetValues();
			//for( Values::const_iterator i = values_.begin(); i != values_.end(); ++i )
			//{
			//	std::cout << i->second << ' ';
			//}
			//std::cout << std::endl;
		}
		return ok;
	}
	RefParser* Clone() const { return new RefParser( *this ); }
private:
	StreamPos memStartPos_;
	StreamPos memEndPos_;
	mutable Values values_;
	IParserPtr ref_;
};

template < class CBackT, class ParserT >
class CBackParser : public IParser
{
public:
    typedef IParser::Values Values;
	typedef CBackT CallBackType;
	typedef ParserT ParserType;
	CBackParser( const ParserType& p, const CallBackType& cback ) : p_( p ), cback_( cback ) {}
	const Values& GetValues() const { return p_.GetValues(); }
	const ValueType& operator[]( const KeyType& k ) const { return p_.operator[]( k ); }
	bool Parse( InStream& is ) { if( p_.Parse( is ) ) return cback_( p_.GetValues() ); return false; }
	CBackParser* Clone() const { return new CBackParser( *this ); }
private:
	ParserType p_;
	CallBackType cback_;
};

bool CBack( const IParser::Values& v )
{
    return true;
	std::cout << *v.begin() << std::endl;
	return true;
}

template < class CB, class P >
CBackParser< CB, P > MakeCBackParser( const CB& cb, const P& p )
{
    return CBackParser< CB, P >( cb, p );
}


void TestRecursion()
{
	typedef bool (*CBackType)(const IParser::Values& );
	typedef CBackParser< CBackType, Parser > CBP;
	Parser expr;
	Parser rexpr = RefParser( expr );
	Parser value = ( C("("), rexpr ,C(")") ) / F();
	expr = ( RefParser( value ), ( C("+") / C("-") ), RefParser( expr ) ) / RefParser( value );
	std::istringstream iss( "1+(2+3)-1-(23+23.5)" );
	InStream is( iss );
	if( !expr.Parse( is ) || !is.eof() )
	{
		std::cout << "ERROR AT " << is.tellg() <<  std::endl;
	}
	else std::cout << "OK" << std::endl;
}



//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    
	TestRecursion();
	//std::wistringstream iss(L"");
	//assert( iss.get() == WEOF ); //WEOF is windows only
	//assert( iss.get() == WEOF ); 
	//std::cout << ([3]sss) << std::endl;
	try
	{
#if 1
		TestTokenizers();
		TestParsers();
		TestExpressionEvaluator();
#endif
		TestMoldenChunk1();
		
	} catch( const std::exception& e )
	{
		std::cerr << e.what();
		std::cout << "\nPRESS <ENTER>" << std::endl;
		std::cin.get();
		return 1;

	}
	std::cout << "\nPRESS <ENTER>" << std::endl;
	std::cin.get();
	return 0;
}


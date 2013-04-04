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

/// @file parsertest.cpp test/sample code.
/// Implementation of a parser for mathematical expressions.

#ifdef USE_TRANSITION_CBACKS
#undef USE_TRANSITION_CBACKS
#endif

#include <iostream>
#include <cassert>
#include <sstream>
#include <algorithm>
#include <list>
#include <stack>

#include "types.h"
#include "InStream.h"
#include "parsers.h"
#include "parser_operators.h"
#include "ParserManager.h"
#include "typedefs.h"
#include "parser_operators.h"

static SkipBlankParser __; // skip blanks static BlankParser _; // parse blank
static SkipToNextLineParser endl_; // skip to next line
static EofParser eof_; // parse EOF
static const bool SKIP_BLANKS = true;
static const bool DONT_SKIP_BLANKS = !SKIP_BLANKS;
static const bool SKIP_TO_NEXT_LINE = true;
static const bool DONT_SKIP_TO_NEXT_LINE = !SKIP_TO_NEXT_LINE;

// TEST PARSER FOR EXPRESSION EVALUATION USING TERM REWRITING
namespace eval {
    enum { START, VALUE, OPERATOR, EXPRESSION_BEGIN, EXPRESSION_END, EOF_ };
    class Print : public IStateManager {
        struct OP {
            int code_;
            double value_;
            int level_;
            bool operator==( const OP& other ) const {
                return other.code_  == code_  && 
                       other.value_ == value_ && 
                       other.level_ == level_; 
            }
            OP() : code_( -1 ), value_( 0.0 ), level_( 0 ) {}
            OP( int code, double value = 0.0, int level = 0 ) 
                : code_( code ), value_( value ), level_( level ) {}
        };
        typedef std::list< OP > Program;
        
        enum { VAL, ADD, SUB, MUL, DIV, LEFT_P, RIGHT_P };
    public:
        void UpdateState( IStateController& sc, StateID currentState ) const {
        }
	void EnableState( StateID ) {}
	void DisableState( StateID ) {}
	void EnableState( const ParserID& ) {}
	void DisableState( const ParserID& ) {}
	void EnableAllStates() {}
	typedef std::map< ValueID, Any > Values;
	bool HandleValues( StateID sid, const Values& v ) {
            Program::iterator levelBegin, levelEnd;
	    switch( sid ) {
	    case EXPRESSION_BEGIN:
		 state_->program_.push_back( OP( LEFT_P ) );
		 levelStack_.push( --( state_->program_.end() ) );
		 break;
	    case EXPRESSION_END: {
		     state_->program_.push_back( OP( RIGHT_P ) );
		     levelEnd = --( state_->program_.end() );
                     levelBegin = levelStack_.top();
                     levelStack_.pop();
		     const double V = state_->Evaluate( ++levelBegin, levelEnd );
		     state_->program_.erase( --levelBegin, ++levelEnd );
		     state_->program_.push_back( OP( VAL, V ) );    
		     break;
		 } 
	    case VALUE: state_->program_.push_back( 
            OP( VAL, v.find("operand")->second ) ) ;
			break;
	    case OPERATOR: {
		    const String op = v.find( "operator" )->second;
		    if( op == "+" ) state_->program_.push_back( OP( ADD ) );
		    else if( op == "-" ) state_->program_.push_back( OP( SUB ) );
		    else if( op == "*" ) state_->program_.push_back( OP( MUL ) );
		    else if( op == "/" ) state_->program_.push_back( OP( DIV ) );
		}
		break;
	    case EOF_:  if( !levelStack_.empty() ) {
			    throw std::logic_error( "Unmatched parenthesis" );
			    return false; //in case no exception handling enabled
			}  
			std::cout << state_->Evaluate( 
                state_->program_.begin(), state_->program_.end() );
			break;
	    default:    break;  
	    }
	    Values::const_iterator i;
	    for( i = v.begin(); i != v.end(); ++i ) {
		std::cout << '(' << i->first << " = " << i->second << ')' << ' ';
	    }
	    std::cout << '\n';
	    return true;
	}
	void HandleError( StateID sid, int /*line*/ ) {
	    std::cerr << "\n\t[" << sid << "] PARSER ERROR" <<  std::endl;
	}
	Print* Clone() const { return new Print( *this ); }
    public: 
        struct State {
            Program program_;
            double Evaluate( Program::iterator b, Program::iterator e ) {
                // termination condition  
                Program::iterator ei = e;
                if( b == --ei ) return b->value_;  
                Program::iterator i = std::find( b, e, OP( ADD ) );
                Program::iterator l;
                Program::iterator r;
                if( i != e ) {
                    l = i;
                    r = i;
                    if( i == b ) return 0 + Evaluate( ++r, e );
                    else return Evaluate( b, l ) + Evaluate( ++r, e );
                }
                i = std::find( b, e, OP( SUB ) );
                if( i != e ) {
                    l = i;
                    r = i;
                    if( i == b ) return 0 - Evaluate( ++r, e );
                    else return Evaluate( b, l ) - Evaluate( ++r, e );
                }
                i = std::find( b, e, OP( MUL ) );
                if( i != e ) {
                    l = i;
                    r = i;
                    return Evaluate( b, l ) * Evaluate( ++r, e );
                }
                i = std::find( b, e, OP( DIV ) );
                if( i != e ) {
                    l = i;
                    r = i;
                    // could catch division by zero exception
                    return Evaluate( b, l ) / Evaluate( ++r, e ); 
                }
                throw std::logic_error( "Invalid expression" ); 
            }
        };
        Print( SmartPtr< State > s ) : state_( s ) {} 
    private:
        SmartPtr< State > state_;
	std::stack< Program::iterator > levelStack_;

    };
} //namespace eval

void TestExprEval() {
    std::cout << "EXPRESSION EVALUATOR" << std::endl;
    std::cout << "====================" << std::endl;

    const String EXPR  = "1+3*(4-(2-1/3.3))"; 
    std::cout << "Expression: " << EXPR << " = " << 1+3*(4-(2-1/3.3)) 
              << std::endl;
    std::istringstream iss( EXPR );
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
    pm.AddState( eval::EXPRESSION_END, eval::EXPRESSION_END );
    pm.AddState( eval::EXPRESSION_END, eval::EOF_ );
    pm.AddState( eval::EXPRESSION_END, eval::OPERATOR );
    pm.AddState( eval::VALUE, eval::OPERATOR )
        .AddState( eval::VALUE, eval::EOF_ );
    pm.AddState( eval::OPERATOR, eval::EXPRESSION_BEGIN );
    pm.AddState( eval::OPERATOR, eval::VALUE );
    
    pm.SetParser( eval::VALUE, 
                  ( AL( DONT_SKIP_BLANKS ) >= __ >= F("operand") ) );
    pm.SetParser( eval::OPERATOR, 
                  ( AL( DONT_SKIP_BLANKS ) >= __ >= 
                    ( C("+","operator") /
                                           C("-","operator") /
                                           C("/","operator") /
                                           C("*","operator") ) ) );
    pm.SetParser( eval::EXPRESSION_BEGIN, ( __ > C("(","open") ) );
    pm.SetParser( eval::EXPRESSION_END,   ( __ > C(")","closed") ) );
    pm.SetParser( eval::EOF_, ( eof_ ) );
    
    pm.Apply( is, eval::START );
}

int main( int, char** ) {
    TestExprEval();
    return 0;
}



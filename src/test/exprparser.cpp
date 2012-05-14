/// @file parsertest.cpp Test/sample code.

#include <iostream>
#include <cassert>
#include <sstream>

#include "types.h"
#include "InStream.h"
#include "parsers.h"
#include "parser_operators.h"
#include "ParserManager.h"
#include "typedefs.h"

static SkipBlankParser __; // skip blanks static BlankParser _; // parse blank
static SkipToNextLineParser endl_; // skip to next line
static EofParser eof_; // parse EOF
static const bool SKIP_BLANKS = true;
static const bool DONT_SKIP_BLANKS = !SKIP_BLANKS;
static const bool SKIP_TO_NEXT_LINE = true;
static const bool DONT_SKIP_TO_NEXT_LINE = !SKIP_TO_NEXT_LINE;

// TEST PARSER FOR EXPRESSION EVALUATION USING TERM REWRITING
namespace eval
{
    enum { START, VALUE, OPERATOR, EXPRESSION_BEGIN, EXPRESSION_END, EOF_ };
    class Print : public IStateManager
    {
        enum { VAL, ADD, SUB, MUL, DIV, LEFT_P, RIGHT_P };
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
        case EXPRESSION_BEGIN:
             state_->program_.push_back( OP( LEFT_P ) );
             break;
        case EXPRESSION_END:
             state_->program_.push_back( OP( RIGHT_P ) );
             break; 
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
        case EOF_:  state_->Evaluate( state_->program_.begin() );
                    break;
        default:    break;  
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
                    Program::iterator c = i;
                    ++c;
                    program_.erase( i );
                    Evaluate( c );
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
                if( i->code_ == RIGHT_P ) program_.erase( i ); 
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
        Print( SmartPtr< State > s ) : state_( s ) {} 
    private:
        SmartPtr< State > state_;

    };
} //namespace eval

void TestExprEval()
{
    std::cout << "EXPRESSION EVALUATOR" << std::endl;
    std::cout << "====================" << std::endl;

    const String EXPR  = "1+3*4-1/3.3"; 
    std::cout << "Expression: " << EXPR << " = " << (1+3*4-1/3.3) << std::endl;
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
    pm.AddState( eval::EXPRESSION_END, eval::OPERATOR );
    pm.AddState( eval::EXPRESSION_END, eval::EXPRESSION_END );
    pm.AddState( eval::EXPRESSION_END, eval::EOF_ );
    pm.AddState( eval::VALUE, eval::OPERATOR ).AddState( eval::VALUE, eval::EOF_ );
    pm.AddState( eval::OPERATOR, eval::EXPRESSION_BEGIN );
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

int main( int, char** ) {
    TestExprEval();
    return 0;
}



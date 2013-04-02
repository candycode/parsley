///@file moldenformat test/sample code.
///Prser for a subset of the Molden format

#include <iostream>
#include <cassert>
#include <sstream>
#include <fstream>

#include "types.h"
#include "InStream.h"
#include "parsers.h"
#include "parser_operators.h"
#include "ParserManager.h"
#include "typedefs.h"


static SkipBlankParser __;
static BlankParser _;
static SkipToNextLineParser endl_;
static EofParser eof_;
const bool SKIP_BLANKS = true;
const bool DONT_SKIP_BLANKS = !SKIP_BLANKS;
const bool SKIP_TO_NEXT_LINE = true;
const bool DONT_SKIP_TO_NEXT_LINE = !SKIP_TO_NEXT_LINE;

//==============================================================================
// READ MOLDEN DATA FROM STRINGSTREAM AND ACTUAL FILES WITH WIN AND UNIX EOL
//==============================================================================


enum { START, MOLDEN_FORMAT, TITLE, TITLE_DATA, ATOMS, ATOM_DATA, GTO,
       GTO_ATOM_NUM, GTO_SHELL_INFO, GTO_COEFF_EXP, EOL_, EOF_, SKIP_LINE, INVALID_STATE };

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
        << "  [Title]"         << '\n'
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
        <<  "0.12384016938000E+04 0.54568832082000E-02\n"
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

//FILE PATHS
#ifdef WIN32
    std::ifstream ifs1( "c:\\projects\\parsley\\data\\molden.input" );
    std::ifstream ifs2( "c:\\projects\\parsley\\data\\molden.input.win" );
#else
    std::string dir( ::getenv( "HOME" ) );
    std::string lpath = dir + "/projects/parsley/data/molden.input";
    std::string wpath = dir + "/projects/parsley/data/molden.input.win";
    std::ifstream ifs1( lpath.c_str() );
    std::ifstream ifs2( wpath.c_str() );
#endif
    
    InStream is1( ifs1 );
    InStream is2( ifs2 );

    Print::State* state = new Print::State;

    StateManager PRINT( ( Print( state ) ) );
    
    ParserManager pm( PRINT );
    pm.SetBeginEndStates( START, EOF_ );
    // pm.AddState( /*previous*/ START, /*current*/ MOLDEN_FORMAT ).
    //    AddState( /*previous*/ START, /*current*/ SKIP_LINE ).
    //    AddState( /*previous*/ SKIP_LINE, /*current*/ MOLDEN_FORMAT ).
    //    AddState( /*previous*/ SKIP_LINE, /*current*/ START ).
    //    AddState( /*previous*/ MOLDEN_FORMAT, /* current */ TITLE ).
    //    AddState( /*previous*/ TITLE, /*current*/ ATOMS ).
    //    AddState( /*previous*/ TITLE, /*current*/ TITLE_DATA ).
    //    AddState( /*previous*/ TITLE_DATA, /*current*/ ATOMS ).
    //    AddState( /*previous*/ ATOMS, /*current*/ ATOM_DATA ).
    //    AddState( /*previous*/ ATOM_DATA, /*current*/ ATOM_DATA ).
    //    AddState( /*previous*/ ATOM_DATA, /*current*/ GTO ).
    //    AddState( /*previous*/ GTO, /*current*/ GTO_ATOM_NUM ).
    //    AddState( GTO_ATOM_NUM, GTO_SHELL_INFO).
    //    AddState( GTO_SHELL_INFO, GTO_COEFF_EXP ).
    //    AddState( GTO_COEFF_EXP, GTO_SHELL_INFO ).
    //    AddState( GTO_COEFF_EXP, GTO_COEFF_EXP ).
    //    AddState( GTO_COEFF_EXP, GTO_ATOM_NUM ).
    //    AddState( GTO_COEFF_EXP, EOF_ ). //end of file
    //    AddState( GTO_COEFF_EXP, EOL_ ); //end of line: will stop a the end of GTO input, e.g. before [MO] tag
    // pm.AddStates<-1>()
    //    ( /*previous*/ START, /*current*/ MOLDEN_FORMAT )
    //    ( /*previous*/ START, /*current*/ SKIP_LINE )
    //    ( /*previous*/ SKIP_LINE, /*current*/ MOLDEN_FORMAT )
    //    ( /*previous*/ SKIP_LINE, /*current*/ START )
    //    ( /*previous*/ MOLDEN_FORMAT, /* current */ TITLE )
    //    ( /*previous*/ TITLE, /*current*/ ATOMS )
    //    ( /*previous*/ TITLE, /*current*/ TITLE_DATA )
    //    ( /*previous*/ TITLE_DATA, /*current*/ ATOMS )
    //    ( /*previous*/ ATOMS, /*current*/ ATOM_DATA )
    //    ( /*previous*/ ATOM_DATA, /*current*/ ATOM_DATA )
    //    ( /*previous*/ ATOM_DATA, /*current*/ GTO )
    //    ( /*previous*/ GTO, /*current*/ GTO_ATOM_NUM )
    //    ( GTO_ATOM_NUM, GTO_SHELL_INFO )
    //    ( GTO_SHELL_INFO, GTO_COEFF_EXP )
    //    ( GTO_COEFF_EXP, GTO_SHELL_INFO )
    //    ( GTO_COEFF_EXP, GTO_COEFF_EXP )
    //    ( GTO_COEFF_EXP, GTO_ATOM_NUM )
    //    ( GTO_COEFF_EXP, EOF_ ) //end of file
    //    ( GTO_COEFF_EXP, EOL_ ); //end of line: will stop a the end of GTO input, e.g. before [MO] tag
    pm.AddStates< INVALID_STATE >()
       ( START, MOLDEN_FORMAT, SKIP_LINE )
       ( SKIP_LINE, MOLDEN_FORMAT, START )
       ( MOLDEN_FORMAT, TITLE )
       ( TITLE, ATOMS, TITLE_DATA )
       ( TITLE_DATA, ATOMS )
       ( ATOMS, ATOM_DATA )
       ( ATOM_DATA, ATOM_DATA, GTO)
       ( GTO, GTO_ATOM_NUM )
       ( GTO_ATOM_NUM, GTO_SHELL_INFO )
       ( GTO_SHELL_INFO, GTO_COEFF_EXP )
       ( /* from */ GTO_COEFF_EXP, 
          /*to*/ GTO_SHELL_INFO, /*or*/ GTO_COEFF_EXP, /*or*/ GTO_ATOM_NUM, /*or*/ EOF_, /*or*/ EOL_ );
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
    pm.SetParser( EOL_, endl_ );
    std::cout << "+++++++++++++++++++++++++++++++++++++++++++++\n";
    std::cout << "++++++++++ UNIX/MacOS X LINE ENDINGS ++++++++\n";
    pm.Apply( is1, START );
    std::cout << "+++++++++++++++++++++++++++++++++++++\n";
    std::cout << "++++++++ WINDOWS LINE ENDINGS +++++++\n";
    pm.Apply( is2, START );

}

//------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
    TestMoldenChunk1();
    return 0;
}


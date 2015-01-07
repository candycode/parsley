#include <map>
#include <string>
#include <cassert>
#include <iostream>


#include <PTree.h>


namespace {
//==============================================================================
enum TERM {EXPR = 1, OP, CP, VALUE, PLUS, MINUS, MUL, DIV, SUM, PRODUCT,
           NUMBER, POW, POWER};

std::map< TERM, int > weights = {
    {NUMBER, 10},
    {OP, 100},
    {CP, 100},
    {POW, 8},
    {MUL, 7},
    {DIV, 7},
    {MINUS, 6},
    {PLUS, 5}
};

bool ScopeBegin(TERM t) { return t == OP; }
bool ScopeEnd(TERM t) { return t == CP; }

std::string TermToString(TERM t) {
    std::string ret;
    switch(t) {
        case EXPR: ret = "EXPR";
            break;
        case OP: ret = "(";
            break;
        case CP: ret = ")";
            break;
        case VALUE: ret = "VALUE";
            break;
        case PLUS: ret = "+";
            break;
        case MINUS: ret = "-";
            break;
        case MUL: ret = "*";
            break;
        case DIV: ret = "/";
            break;
        case SUM: ret = "SUM";
            break;
        case PRODUCT: ret = "PROD";
            break;
            //        case NUMBER: ret = "";
            //            break;
        case POW: ret = "^";
            break;
        case POWER: ret = "POW";
            break;
        default: break;
    }
    return ret;
};

}
using namespace std;

void TreeTest() {
    std::vector< int > result;
    const std::vector< int > CORRECT =
    {MUL, NUMBER, OP, PLUS, NUMBER, NUMBER, CP};
    parsley::STree< TERM > stree;
    //"6*((2+-3)+7^9)"
    
    //2*(5+6)
    //MUL -> NUMBER
    //    -> OP -> PLUS -> NUMBER
    //                  -> NUMBER -> CP
    stree.Add(NUMBER, weights[NUMBER])
//    .Add(MUL, weights[MUL])
//    .Add(OP, weights[OP])
//    .Add(OP, weights[OP] + weights[OP])
    //.Add(NUMBER, weights[NUMBER] + 2*weights[OP])
    .Add(MUL, weights[MUL])
    .Add(OP, weights[OP])
    .Add(NUMBER, weights[NUMBER])
    .Add(PLUS, weights[PLUS])
    .Add(NUMBER, weights[NUMBER])
    .Add(CP, weights[CP]);
//    .Add(CP, weights[CP] + weights[OP])
//    .Add(PLUS, weights[PLUS]+ weights[OP])
//    .Add(NUMBER, weights[NUMBER]+ weights[OP])
//    .Add(POW, weights[POW]+ weights[OP])
//    .Add(NUMBER, weights[NUMBER]+ weights[OP])
//    .Add(CP, weights[CP]);
    stree.Apply([&result](TERM t) { result.push_back(t); });
    assert(result == CORRECT);
}







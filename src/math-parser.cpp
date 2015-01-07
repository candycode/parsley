
#include <map>
#include <iostream>
#include <sstream>
#include <string>
#include <parsers.h>
#include <InStream.h>
#include <PTree.h>
#include <peg.h>
#ifdef HASH_MAP
#include <unordered_map>
#else
#include <map>
#endif


using namespace std;

namespace {
//==========================================================================
using real_t = double;

enum TERM {EXPR = 1, OP, CP, VALUE, PLUS, MINUS, MUL, DIV, SUM, PRODUCT,
           NUMBER, POW, POWER, START};
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

struct Term {
    TERM type;
    real_t value;
    Term(TERM t, real_t v) : type(t), value(v) {}
};

bool ScopeBegin(TERM t) { return t == OP; }
bool ScopeEnd(TERM t) { return t == CP; }

using namespace parsley;
using AST = parsley::STree< Term, std::map< TERM, int > >;

using Ctx = AST;
    
bool HandleTerm(TERM t, const Values& v, Ctx& ast, EvalState es) {
    if(es == EvalState::BEGIN || v.empty()) return true;
    if(es == EvalState::FAIL) return false;
    if(t == NUMBER) ast.Add({t, v.begin()->second});
    else ast.Add({t, real_t()});
    return true;
};

#define DEFINE_HASH(T) \
namespace std { \
template <> \
struct hash< T > \
{ \
std::size_t operator()(T k) const \
{ \
using std::size_t; \
using std::hash; \
return std::hash<int>()(int(k)); \
} \
}; \
}

using ActionFun = std::function< bool (TERM, const Values&, Ctx&, EvalState) >;
    
#ifdef HASH_MAP
    DEFINE_HASH(TERM)
#endif
#ifdef HASH_MAP
    using Map = std::unordered_map< TERM, EvalFun >;
    using ActionMap = std::unordered_map< TERM, ActionFun >;
#else
    using ParsingRules = std::map< TERM, EvalFun >;
    using ActionMap = std::map< TERM, ActionFun  >;
#endif
    
   
ParsingRules GenerateParser(ActionMap& am, Ctx& ctx) {
    
    ParsingRules g; // grammar
    auto n  = MAKE_CALL(TERM, g, am, ctx);
    auto mt = MAKE_EVAL(TERM, g, am, ctx);
    
    //grammar definition
    //non-terminal - note the top-down definition through deferred calls using
    //the 'c' helper function
    
    g[START]   = n(EXPR);
    g[EXPR]    = n(SUM);
    g[SUM]     = (n(PRODUCT), *((n(PLUS) / n(MINUS)), n(PRODUCT)));
    g[PRODUCT] = (n(POWER), *((n(MUL) / n(DIV) / n(POW)), n(VALUE)));
    g[POWER]   = (n(VALUE), *(n(POW), n(VALUE)));
    g[VALUE]   = n(NUMBER) / (n(OP), n(EXPR), n(CP));
    using FP = FloatParser;
    using CS = ConstStringParser;
    //terminal
    g[NUMBER] = mt(NUMBER, FP());
    g[OP]     = mt(OP,     CS("("));
    g[CP]     = mt(CP,     CS(")"));
    g[PLUS]   = mt(PLUS,   CS("+"));
    g[MINUS]  = mt(MINUS,  CS("-"));
    g[MUL]    = mt(MUL,    CS("*"));
    g[DIV]    = mt(DIV,    CS("/"));
    g[POW]    = mt(POW,    CS("^"));
    
    return g;
}
    
}

void MathParser(const string& expr) {
    istringstream iss(expr);
    InStream is(iss);
    AST ast(weights);
    ActionMap am;
    Set(am, HandleTerm, NUMBER,
        EXPR, OP, CP, PLUS, MINUS, MUL, DIV, POW, SUM, PRODUCT, POWER, VALUE);
    ParsingRules g = GenerateParser(am, ast);
    g[START](is);
    
    auto getData = [](const Term& t) { return t.value; };
    auto getType = [](const Term& t) { return t.type;  };
    auto init    = [](TERM tid) { return tid == MUL
        || tid == DIV ? real_t(1) : real_t(0); };
    
    cout << "RESULT: " << ast.Eval< real_t >(am, init, getData, getType)
         << endl;
    
}


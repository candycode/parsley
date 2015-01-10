
#include <map>
#include <iostream>
#include <sstream>
#include <string>
#include <parsers.h>
#include <functional>
#include <cmath>

#include <peg.h>
#ifdef HASH_MAP
#include <unordered_map>
#else
#include <map>
#endif
#include <InStream.h>
#include <PTree.h>
using namespace std;


namespace {
//==========================================================================
using real_t = double;

enum TERM {EXPR = 1, OP, CP, VALUE, PLUS, MINUS, MUL, DIV, SUM, PRODUCT,
           NUMBER, POW, POWER, START, VAR, ASSIGN, ASSIGNMENT};
std::map< TERM, int > weights = {
    {NUMBER, 10},
    {VAR, 10},
    {ASSIGN, 9 },
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

//required customization points
bool ScopeBegin(const Term& t) { return t.type == OP; }
bool ScopeEnd(const Term& t) { return t.type == CP; }
real_t GetData(const Term& t) { return t.value; }
real_t Init(TERM tid) {
    if(tid == ASSIGN) return std::numeric_limits<real_t>::quiet_NaN();
    return tid == MUL
    || tid == DIV
    || tid == POW ? real_t(1) : real_t(0); };
TERM GetType(const Term& t) { return t.type; }
//in case functions implementing operators are not of standard
//C function type and do not define 'return_type' it is required to also
//specialize Return<T> to allow for return type deduction
//namespace parsley {
//template <>
//struct Return< MyCallableType > {
//    using Type = AReturnType;
//};
//}
    
    
using namespace parsley;
using AST = parsley::STree< Term, std::map< TERM, int > >;

//LVALUE handling: only using evaluation function supporting numbers
//use var -> real key -> real value
using VarToKey = std::map< string, real_t >;
using KeyToVaue = std::map< real_t, real_t >;

    
struct Ctx {
    AST ast;
    VarToKey varkey;
    KeyToVaue keyvalue;
    real_t assignKey;
};

real_t GenKey() {
    static real_t k = real_t(0);
    return k++;
}
    
bool HandleTerm(TERM t, const Values& v, Ctx& ctx, EvalState es) {
    if(es == EvalState::BEGIN || v.empty()) return true;
    if(es == EvalState::FAIL) return false;
    if(t == VAR) {
        if(ctx.varkey.find(v.begin()->second) != ctx.varkey.end()) {
            const real_t k = ctx.varkey.find(v.begin()->second)->second;
            ctx.ast.Add({t, k});
        } else {
            const real_t k = GenKey();
            ctx.varkey[v.begin()->second] = k;
            ctx.keyvalue[k] = real_t();
            ctx.ast.Add({t, k});
        }
    } else if(t == NUMBER) ctx.ast.Add({t, v.begin()->second});
    else if(t == CP) return true; //not adding marker of scope end
    else ctx.ast.Add({t, Init(t)});
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
    g[VALUE]   = n(ASSIGNMENT) / n(NUMBER) / (n(OP), n(EXPR), n(CP));
    g[ASSIGNMENT]  = (n(VAR), ZO((n(ASSIGN), n(EXPR))));
    ///TODO support for multi-arg functions:
    //redefinitions of open and close parethesis are used to signal
    //the begin/end of argument list;
    //eval function of ',' separator takes care of adding result of expression
    //into Context::ArgumentArray
    //eval function of FOP ('(') puts result of first child (if any) into
    //Context::ArgumentArray
    //eval function of FUNNAME invokes function passing Context::ArgumentArray
    //to function, usually a wrapper which forwards calls to actual math
    //function f(const vector<duble>& v) -> atan(v[0], v[1]);
    //it is possible to have one arg array per function and therefore also
    //memoize by caching last used parameters
    //g[FUN]         = (n(FUNNAME), n(FOP), ZO((n(EXPR), *(n(ARGSEP), (EXPR)))),
    //                   n(FCP));
    using FP = FloatParser;
    using CS = ConstStringParser;
    using VS = FirstAlphaNumParser;
    //terminal
    g[NUMBER] = mt(NUMBER, FP());
    g[OP]     = mt(OP,     CS("("));
    g[CP]     = mt(CP,     CS(")"));
    g[PLUS]   = mt(PLUS,   CS("+"));
    g[MINUS]  = mt(MINUS,  CS("-"));
    g[MUL]    = mt(MUL,    CS("*"));
    g[DIV]    = mt(DIV,    CS("/"));
    g[POW]    = mt(POW,    CS("^"));
    g[ASSIGN] = mt(ASSIGN, CS("<-"));
    g[VAR]    = mt(VAR, VS());
    
    return g;
}
 

}


real_t MathParser(const string& expr, Ctx& ctx) {
    istringstream iss(expr);
    InStream is(iss, [](Char c) {return c == ' ' || c == '\t';});
    ctx.ast.SetWeights(weights);
    ActionMap am;
    Set(am, HandleTerm, NUMBER,
        EXPR, OP, CP, PLUS, MINUS, MUL, DIV, POW, SUM, PRODUCT, POWER, VALUE,
        ASSIGN, ASSIGNMENT, VAR);
    ParsingRules g = GenerateParser(am, ctx);
    g[START](is);
    if(is.tellg() < expr.size()) cerr << "ERROR AT: " << is.tellg() << endl;
    using Op = std::function< real_t (real_t, real_t) >;
    using Ops = std::map< TERM, Op >;
    Ops ops = {
        {NUMBER, [](real_t n, real_t) { return n; }},
        {PLUS, [](real_t v1, real_t v2) { return v1 + v2; }},
        {MINUS, [](real_t v1, real_t v2) { return v1 - v2; }},
        {MUL, [](real_t v1, real_t v2) { return v1 * v2; }},
        {DIV, [](real_t v1, real_t v2) { return v1 / v2; }},
        {POW, [](real_t v1, real_t v2) { return pow(v1,v2); }},
        {OP, [](real_t v, real_t a) { return v; }},
        {CP, [](real_t v, real_t ) { return v; }},
        //HACK TO AVOID USING AN EVAL FUNCTION THAT TAKES FULL Term
        //INSTANCES AND SUPPORTS DATA OTHER THAN double
        {ASSIGN, [&ctx](real_t key, real_t value ) {
            ctx.keyvalue[ctx.assignKey] = value;
            return value;
        }},
        {VAR, [&ctx](real_t k, real_t v) {
            ctx.assignKey = k;
            return ctx.keyvalue[k];
        }}
    };
    
    const real_t ret = ctx.ast.Eval(ops);
    return ret;
}

int main(int, char**) {
    //REPL, exit when input is empty
    string me;
    Ctx ctx;
    while(getline(cin, me)) {
        if(me.empty()) break;
        cout << MathParser(me, ctx) << endl;
        ctx.ast.Reset();
    }
    return 0;
}




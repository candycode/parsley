
#include <map>
#include <iostream>
#include <sstream>
#include <string>
#include <parsers.h>
#include <functional>
#include <cmath>
#include <stack>

#include <peg.h>
#include <unordered_map>
#include <map>
#include <InStream.h>
#include <PTree.h>
#include <types.h>
//following is required for parser composition: (P1,P2) -> P1 & P2 -> AndParser
#include <parser_operators.h>

#include <tuple>

using namespace std;

namespace {
//==============================================================================
using real_t = double;

///Term type
enum TERM {EXPR = 1, OP, CP, VALUE, PLUS, MINUS, MUL, DIV, SUM, PRODUCT,
           NUMBER, POW, START, VAR, ASSIGN, ASSIGNMENT, SUMTERM, PRODTERM
};


///for debugging purposes only
std::map< TERM, string > T2S =
    {{PLUS, "PLUS"}, {MINUS, "MINUS"}, {MUL, "MUL"}, {DIV, "DIV"}};

//operator arity, required only for non syntax tree based approaches
std::map< TERM, int > ARITY =
    {{PLUS, 2}, {MINUS, 2}, {MUL, 2}, {DIV, 2}, {POW, 2}, {ASSIGN, 2}};


using namespace parsley;

///operator priority
///order by priority low -> high and specify scope operators '(', ')'
std::map< TERM, int > weights
    = GenWeightedTerms< TERM, int >(
        {{PLUS},
        {MINUS},
        {MUL, DIV},
        {POW},
        {ASSIGN},
        {VAR, NUMBER}},
        //scope operators
        {OP, CP});
    
///Term data
struct Term {
    TERM type;
    real_t value;
    Term(TERM t, real_t v) : type(t), value(v) {}
    Term() = default;
};

//required customization points
///Returns @c true if term represent the beginning of a new scope
bool ScopeBegin(const Term& t) { return t.type == OP; }
///Returns @c true if term represent the beginning of a new scope
bool ScopeEnd(const Term& t) { return t.type == CP; }
real_t GetData(const Term& t) { return t.value; }
real_t Init(TERM tid) {
    if(tid == ASSIGN)
        return std::numeric_limits<real_t>::quiet_NaN();
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
    
//Syntax tree type used by HandleTerm callback only
using AST = STree< Term, std::map< TERM, int > >;

//Variables
//LVALUE handling: only using evaluation function supporting numbers
//use var -> real key -> real value
using VarToKey = std::map< string, real_t >;
using KeyToValue = std::map< real_t, real_t >;
using FunToKey = std::map< string, real_t >;
using Function = real_t (real_t, real_t);
using KeyToFun = std::map< real_t, Function >;
using Invokable = std::function< Function >;
using VarKey = real_t;
using FunKey = real_t;
    

//generate new key used for variable->value mapping
real_t GenKey() {
    static real_t k = real_t(0);
    return k++;
}

//Parser callback 1, simply add terms to syntax tree
bool HandleTerm(TERM t, const Values& v, Ctx& ctx, EvalState es) {
    if(es == EvalState::BEGIN) return true;
    if(es == EvalState::FAIL) return false;
    
    if(t == VAR) {
        if(In(Get(v), ctx.varkey)) {
            const real_t k = Get(ctx.varkey, Get(v));
            ctx.ast.Add({t, k});
        } else {
            const real_t k = GenKey();
            ctx.varkey[Get(v)] = k;
            ctx.keyvalue[k] = std::numeric_limits<real_t>::quiet_NaN();
            ctx.ast.Add({t, k});
        }
    } else if(t == NUMBER) {
        ctx.ast.Add({t, Get(v)});
    }
    else if(t == CP) return true;
    else if(!v.empty()) ctx.ast.Add({t, Init(t)});
    return true;
};

///True when term is the id matching the open scope
bool ScopeOpen(TERM t)  { return t == OP; }
///True then term is the id matching the closing scope
bool ScopeClose(TERM t) { return t == CP; }
///True if term is an operation
bool Op(TERM t) {
    return t == PLUS
           || t == MINUS
           || t == MUL
           || t == DIV
           || t == POW
           || t == ASSIGN
           || ScopeOpen(t);
}
//Note on variable handling: var name is parsed but nothing is added into
//the value/function arrays, only the key matching the variable is stored
//into the context

    
///In case a hash map is used it is required to provide a hash  key generation
///function
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

///Parser callback type
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
    
///Generate parser table map: each element represent a parsing rule
ParsingRules GenerateParser(ActionMap& am, Ctx& ctx) {
    
    ParsingRules g; // grammar
    auto n  = MAKE_CALL(TERM, g, am, ctx);
    auto c  = MAKE_CBCALL(TERM, g, am, ctx);
    auto mt = MAKE_EVAL(TERM, g, am, ctx);
    
    //grammar definition
    //non-terminal - note the top-down definition through deferred calls using
    //the 'c' helper function
    // c -> callback invoked also for non-terminal states
    // n -> callback invoked only for non terminal states
    // 'c' is required to captures operations such as 1 + 2: callback is invoked
    // after parsing 1 + 2 and can therefore be used to directly evaluate term
    // once both operands and operation type have been stored into context
    //pattern is:
    //grammar[STATE_ID] = c(OPERATION_TO_HANDLE_WITH_CALLBAK)... | ... (...,...)
    //grammar[STATE_TO_HANDLE_WITH_CALLBAK] = ...
    //this way you have a callback invoked after all the operations associated
    //with STATE_TO_HANDLE_WITH_CALLBAK have been completed it is therefore
    //possible to handle operations after all operands and operators have been
    //parsed instead of having a callback invoked when an operator is found
    //with no knowledge of what lies on its right side
    g[START]   = n(EXPR);
    g[EXPR]    = n(SUM);
    //c(SUMTERM) and c(PRODTERM) are only required for non-tree based evaluation
    //because a callback needs to be invoked each time a "+ 3"
    //expression is parsed in order to be able to properly add a function or
    //compute a value each time an operator is encountered
    g[SUM]      = (n(PRODUCT), *c(SUMTERM));
    g[SUMTERM]  = ((n(PLUS) / n(MINUS)), n(PRODUCT));
    g[PRODUCT]  = (n(VALUE), *c(PRODTERM));
    g[PRODTERM] = ((n(MUL) / n(DIV) / n(POW)), n(VALUE));
    g[VALUE]    = (n(OP), n(EXPR), n(CP)) / c(ASSIGNMENT) / n(NUMBER);
    g[ASSIGNMENT]  = (n(VAR), ZO((n(ASSIGN), n(EXPR))));
    ///@todo support for multi-arg functions:
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



class EvalFrame {
private:
    using Args = std::vector< real_t >;
    using Function = std::function< real_t (const Args&) >;
    using FunStack = std::stack< Function >;
    using FunLUT = std::unordered_map< real_t, Function >;
    using VarLUT = std::unordered_map< real_t, real_t >;
    using FLUTRef = std::reference_wrapper< FunLUT >;
    using VLUTRef = std::reference_wrapper< VarLUT >;
public:
    EvalFrame(FunLUT& f, VarLUT& v) : flut_(f), vlut_(v) {}
    EvalFrame(const EvalFrame&) = default;
    EvalFrame(EvalFrame&&) = default;
    EvalFrame operator()(const Term& t, APPLY stage) {
        if(stage == APPLY::BEGIN) {
            switch(t.type) {
                case NUMBER:
                    args_.push_back(t.value);
                    f_.push([](const Args& args) { return args[0]; } );
                    break;
                case VAR:
                    if(last_ == ASSIGN) {
                        if(vlut_.get().find(t.value) != vlut_.get().end())
                            args_.push_back(t.value);
                        else args_.push_back(GenKey());
                    }
                    else args_.push_back(vlut_.get()[t.value]);
                    f_.push([this](const Args& args) {
                        return args[0];
                    });
                    break;
                default: f_.push(flut_.get()[t.value]);
                    break;
            }
            last_ = t.type;
        } else if(stage == APPLY::END) {
            if(t.type == ASSIGN) {
                vlut_.get()[args_[0]] = args_[1];
                args_[0] = args_[1];
                args_.resize(1);
            } else {
                const real_t ret = f_.top()(args_);
                f_.pop();
                args_.resize(0);
                args_.push_back(ret);
            }
        }
        return *this;
    }
    const Args& Values() const { return args_; }
private:
    TERM last_ = TERM();
    Args args_;
    FunStack f_;
    FLUTRef flut_;
    VLUTRef vlut_;
};

using Args = std::vector< real_t >;
using F = std::function< real_t (const Args&) >;
using FunLUT = std::unordered_map<real_t, F >;
using VarLUT = std::unordered_map<real_t, F >;
using Name2Key = std::unordered_map< std::string, real_t >;
using Op2Key = std::unordered_map< TERM, real_t >;

Name2Key fn = {
    {"sin", 0},
    {"cos", 1},
    {"pow", 2}
};

FunLUT fl = {
    {0, [](const Args& args) { return args[0] + args[1]; }},
    {1, [](const Args& args) { return args[0] - args[1]; }},
    {2, [](const Args& args) { return args[0] * args[1]; }},
    {3, [](const Args& args) { return args[0] / args[1]; }},
    {4, [](const Args& args) { return sin(args[0]); }},
    {5, [](const Args& args) { return cos(args[0]); }},
    {6, [](const Args& args) { return pow(args[0], args[1]); }}
};

Op2Key ops = {
    {PLUS, 0},
    {MINUS, 1},
    {MUL, 2},
    {DIV, 3},
    {POW, 4}
};


//Context
struct Ctx {
    AST ast;
};

//restore context to initial state
void Reset(Ctx& c) {
    c.ast.Reset();
    c.values.clear();
    c.functions.clear();
    c.opstack = stack< TERM >();
}



///Parse expression and return evaluated result
real_t MathParser(const string& expr, Ctx& ctx) {
    istringstream iss(expr);
    InStream is(iss);//, [](Char c) {return c == ' ' || c == '\t';});
    ctx.ast.SetWeights(weights);
    ActionMap am;
#if defined(INLINE_EVAL)
    auto HT = HandleTerm2;
#elif defined(FUN_EVAL)
    auto HT = HandleTerm3;
#else
    auto HT = HandleTerm;
#endif
    Set(am, HT, NUMBER,
        EXPR, OP, CP, PLUS, MINUS, MUL, DIV, POW, SUM, PRODUCT, VALUE,
        ASSIGN, ASSIGNMENT, VAR, SUMTERM, PRODTERM);
    ParsingRules g = GenerateParser(am, ctx);
    g[START](is);
    if(is.tellg() < expr.size()) cerr << "ERROR AT: " << is.tellg() << endl;
    
    return ctx.ast.ScopedEval(EvalFrame(ctx.funlut, ctx.varlut));
}




///Entry point
int main(int, char**) {

    //REPL, exit when input is empty
    string me;
    Ctx ctx;
    ctx.funlut = fl;
    while(getline(cin, me)) {
        if(me.empty()) break;
        cout << "> " << MathParser(me, ctx) << endl;
        Reset(ctx);
    }
    return 0;
}

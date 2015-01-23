
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

#include <tuple>

using namespace std;

namespace {
//==============================================================================
using real_t = double;

///Term type
enum TERM {EXPR = 1, OP, CP, VALUE, PLUS, MINUS, MUL, DIV, SUM, PRODUCT,
           NUMBER, POW, START, VAR, ASSIGN, ASSIGNMENT, SUMTERM, PRODTERM,
           FUNCTION, ARG, ARGS, FBEGIN, FEND, FNAME, FSEP
};


///for debugging purposes only
std::map< TERM, string > T2S =
    {{PLUS, "PLUS"}, {MINUS, "MINUS"}, {MUL, "MUL"}, {DIV, "DIV"}};

//operator arity, required only for non syntax tree based approaches
std::map< TERM, int > ARITY =
    {{PLUS, 2}, {MINUS, 2}, {MUL, 2}, {DIV, 2}, {POW, 2}, {ASSIGN, 2}};


//Tree evaluation is performed through function composition of previous
//traversal result with new result:
// for c in children
//    r = f(r, eval(c))
using F1Arg = double (double);
template < typename F, typename T, int... I >
std::function< T(T, T) > MkFun(const F& f) {
    std::vector< T > args;
    args.reserve(sizeof...(I));
    const size_t ARITY = sizeof...(I);
    return  [args, f, ARITY](T r, T n) mutable {
        if(args.empty()) args.push_back(r);
        args.push_back(n);
        if(args.size() == ARITY) {
            n = f(args[I]...);
            args.resize(0);
        }
        return n;
    };
}
    

using namespace parsley;

///operator priority
///order by priority low -> high and specify scope operators '(', ')'
std::map< TERM, int > weights
    = GenWeightedTerms< TERM, int >(
        {{FSEP},
        {PLUS},
        {MINUS},
        {MUL, DIV},
        {POW},
        {ASSIGN},
        {VAR, NUMBER}},
        //scope operators
        {OP, CP, FBEGIN, FEND});
    
///Term data
struct Term {
    TERM type;
    real_t value;
    Term(TERM t, real_t v) : type(t), value(v) {}
};

//required customization points
///Returns @c true if term represent the beginning of a new scope
bool ScopeBegin(const Term& t) { return t.type == OP; }
///Returns @c true if term represent the beginning of a new scope
bool ScopeEnd(const Term& t) { return t.type == CP; }
real_t GetData(const Term& t) { return t.value; }
real_t Init(TERM tid) {
    if(tid == ASSIGN || tid == FBEGIN)
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
    
//Context
struct Ctx {
    AST ast;
    VarToKey varkey;
    KeyToValue keyvalue;
    real_t assignKey;
    FunToKey fkey;
    KeyToValue fkeyvalue;
    
    //Only required for just-in-time evaluation
    //performed from within HandleTerm2 function
    //stores partial evaluation results, at the end of the evaluation
    //result is stored into first element
    std::vector< real_t > values;
    
    //Only required for deferred evaluation function built
    //in HandleTerm3
    //stores functions performing partial evaluation
    //function evaluating entire expression is found in first element of array
    using EvaluateFunction = std::function< real_t () >;
    std::vector< EvaluateFunction > functions;
    
    //Required by both HandleTerm 2 and 3
    stack< TERM > opstack;
};
//restore context to initial state
void Reset(Ctx& c) {
    c.ast.Reset();
    c.values.clear();
    c.functions.clear();
    c.opstack = stack< TERM >();
}

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
    } else if(t == FBEGIN){
        const std::string fname = v.find("name")->second;
        const real_t k = Get(ctx.fkey, fname);
        ctx.ast.Add({t, k});
    } else if(t == FSEP) {
        ctx.ast.Rewind(FBEGIN);
    } else if(t == NUMBER) {
        ctx.ast.Add({t, Get(v)});
    }
    else if(t == CP) return true; //not adding marker of scope end
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

///Parser callback 2: Just in time evaluation
bool HandleTerm2(TERM t, const Values& v, Ctx& ctx, EvalState es) {
    auto reset = [](real_t r,
                    std::vector< real_t >& a,
                    size_t scopeLevel) {
        assert(a.size() > scopeLevel);
        a[scopeLevel] = r;
        a.resize(scopeLevel + 1);
    };
    if(es == EvalState::BEGIN) return true;
    if(es == EvalState::FAIL) return false;
    if(Op(t)) ctx.opstack.push(t);
    if(t == VAR) {
        if(In(Get(v), ctx.varkey)) {
            const real_t k = Get(ctx.varkey,Get(v));
            ctx.assignKey = k;
            ctx.values.push_back(ctx.keyvalue[k]);
        } else {
            const real_t k = GenKey();
            ctx.varkey[Get(v)] = k;
            ctx.keyvalue[k] = real_t();
            ctx.assignKey = k;
        }
    } else if(t == NUMBER) {
        const real_t r = Get(v);
        ctx.values.push_back(r);
    } else if(ScopeClose(t)) {
        if(ctx.opstack.empty() || !ScopeOpen(ctx.opstack.top())) return false;
        ctx.opstack.pop();
    } else if(t == SUMTERM) {
        if(ctx.values.size() > 0 && !ctx.opstack.empty()) {
            const TERM op = ctx.opstack.top();
            const size_t start = In(op, ARITY) ?
            ctx.values.size() - ARITY[op]
            : 0;
            if(op == PLUS) {
                real_t r = ctx.values[start];
                for(size_t i = start + 1; i < ctx.values.size(); ++i)
                    r += ctx.values[i];
                reset(r, ctx.values, start);
                ctx.opstack.pop();
            } else if(op == MINUS) {
                real_t r = ctx.values[start];
                for(size_t i = start + 1; i < ctx.values.size(); ++i)
                    r -= ctx.values[i];
                reset(r, ctx.values, start);
                ctx.opstack.pop();
            }
        }
        
    } else if(t == PRODTERM) {
        if(ctx.values.size() > 0 && !ctx.opstack.empty()) {
            const TERM op = ctx.opstack.top();
            const size_t start = In(op, ARITY) ?
            ctx.values.size() - ARITY[op]
            : 0;
            if(op == MUL) {
                real_t r = ctx.values[start];
                for(size_t i = start + 1; i < ctx.values.size(); ++i)
                    r *= ctx.values[i];
                reset(r, ctx.values, start);
                ctx.opstack.pop();
            } else if(op == DIV) {
                real_t r = ctx.values[start];
                for(size_t i = start + 1; i < ctx.values.size(); ++i)
                    r /= ctx.values[i];
                reset(r, ctx.values, start);
            } else if(op == POW) {
                real_t r = ctx.values[start];
                for(size_t i = start + 1; i < ctx.values.size(); ++i)
                    r = pow(r, ctx.values[i]);
                reset(r, ctx.values, start);
            }
        }
        
    } else if(t == ASSIGNMENT) {
        //assignment is defined as a single variable or variable + assignment
        //expression, in case of single variable (i.e. no ASSIGN on stack)
        //there is no further action to be taken
        if(!ctx.opstack.empty()
           && ctx.opstack.top() == ASSIGN) {
            assert(ctx.values.size() > 0);
            const real_t k = ctx.assignKey;
            ctx.keyvalue[k] = ctx.values.back();
        }
    }
    return true;
}

///Parser callback 3: deferred evaluation function
///Generate expression evaluation function through pattern:
///    f = [f, fi]() {
///        return f() + fi();
///    };
///where fi is the function at position i in the function array;
///traversal path gets unrolled into functions+closured, therefore no
///loop is executed during actual evaluation
bool HandleTerm3(TERM t, const Values& v, Ctx& ctx, EvalState es) {
    auto freset = [](const Ctx::EvaluateFunction& f,
                     std::vector< Ctx::EvaluateFunction >& a,
                     size_t scopeLevel) {
        assert(a.size() > scopeLevel);
        a[scopeLevel] = f;
        a.resize(scopeLevel + 1);
    };
    if(es == EvalState::BEGIN) return true;
    if(es == EvalState::FAIL) return false;
    if(Op(t)) ctx.opstack.push(t);
    if(t == VAR) {
        if(In(Get(v), ctx.varkey)) {
            const real_t k = Get(ctx.varkey,Get(v));
            ctx.assignKey = k;
            std::reference_wrapper< const real_t > ref(ctx.keyvalue[k]);
            ctx.functions.push_back([ref](){ return ref.get(); });
        } else {
            const real_t k = GenKey();
            ctx.varkey[Get(v)] = k;
            ctx.keyvalue[k] = real_t();
            ctx.assignKey = k;
        }
    } else if(t == NUMBER) {
        const real_t r = Get(v);
        ctx.functions.push_back([r]() { return r; });
    } else if(ScopeClose(t)) {
        if(ctx.opstack.empty() || !ScopeOpen(ctx.opstack.top())) return false;
           ctx.opstack.pop();
    } else if(t == SUMTERM) {
        if(ctx.functions.size() > 0 && !ctx.opstack.empty()) {
            const TERM op = ctx.opstack.top();
            const size_t start = In(op, ARITY) ?
                ctx.functions.size() - ARITY[op]
                : 0;
            if(op == PLUS) {
                Ctx::EvaluateFunction f = ctx.functions[start];
                for(size_t i = start + 1; i < ctx.functions.size(); ++i) {
                    auto fi = ctx.functions[i];
                    f = [f, fi]() {
                        return f() + fi();
                    };
                }
                freset(f, ctx.functions, start);
                ctx.opstack.pop();
            } else if(op == MINUS) {
                Ctx::EvaluateFunction f = ctx.functions[start];
                for(size_t i = start + 1;
                    i < ctx.functions.size(); ++i) {
                    auto fi = ctx.functions[i];
                    f = [f, fi](){ return f() - fi(); };
                }
                freset(f, ctx.functions, start);
                ctx.opstack.pop();
            }
        }
        
    } else if(t == PRODTERM) {
        if(ctx.functions.size() > 0 && !ctx.opstack.empty()) {
            const TERM op = ctx.opstack.top();
            const size_t start = In(op, ARITY) ?
                ctx.functions.size() - ARITY[op]
                : 0;
            if(op == MUL) {
                Ctx::EvaluateFunction f = ctx.functions[start];
                for(size_t i = start + 1; i < ctx.functions.size(); ++i) {
                    auto fi = ctx.functions[i];
                    f = [f, fi]() {
                        return f() * fi();
                    };
                }
                freset(f, ctx.functions, start);
                ctx.opstack.pop();
            } else if(op == DIV) {
                Ctx::EvaluateFunction f = ctx.functions[start];
                for(size_t i = start + 1; i < ctx.functions.size(); ++i) {
                    auto fi = ctx.functions[i];
                    f = [f, fi]() { return f() / fi(); };
                }
                freset(f, ctx.functions, start);
                ctx.opstack.pop();
            } else if(op == POW) {
                Ctx::EvaluateFunction f = ctx.functions[start];
                for(size_t i = start + 1; i < ctx.functions.size(); ++i) {
                    auto fi = ctx.functions[i];
                    f = [f, fi]() { return pow(f(), fi()); };
                }
                freset(f, ctx.functions, start);
                ctx.opstack.pop();
            }
        }
        
    } else if(t == ASSIGNMENT) {
        //assignment is defined as a single variable or variable + assignment
        //expression, in case of single variable (i.e. no ASSIGN on stack)
        //there is no further action to be taken
        if(!ctx.opstack.empty()
           && ctx.opstack.top() == ASSIGN) {
            assert(ctx.functions.size() > 0);
            const real_t k = ctx.assignKey;
            const size_t start = ctx.functions.size() - 1;
            const Ctx::EvaluateFunction v = ctx.functions[start];
            std::reference_wrapper<real_t> ref = ctx.keyvalue[k];
            Ctx::EvaluateFunction f = [ref, k, v]() {
                const real_t r = v();
                ref.get() = r;
                return r;
            };
            freset(f, ctx.functions, start);
            ctx.opstack.pop();
        }
    }
    return true;
}

    
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
    g[SUM]     = (n(PRODUCT), *c(SUMTERM));
    g[SUMTERM] = ((n(PLUS) / n(MINUS)), n(PRODUCT));
    g[PRODUCT] = (n(VALUE), *c(PRODTERM));
    g[PRODTERM] = ((n(MUL) / n(DIV) / n(POW)), n(VALUE));
    g[VALUE]   = (n(OP), n(EXPR), n(CP)) / c(ASSIGNMENT) / n(NUMBER);
    g[ASSIGNMENT]  = (n(VAR), ZO((n(ASSIGN), n(EXPR))));
    g[FUNCTION] = (n(FBEGIN), ZO((c(ARG),*c(ARGS))), n(FEND));
    g[ARGS]     = (n(FSEP), c(ARG));
    g[ARG]      = n(EXPR);
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
    g[FBEGIN] = mt(FBEGIN, (VS("name"), CS("(")));
    g[FEND]   = mt(FEND, CS(")"));
    g[FSEP]   = mt(FSEP, CS(","));
    
    
    return g;
}

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
        }},
        {FBEGIN, [&ctx](real_t k, real_t v) {
            ctx.fkey.push(k);
            ctc.fargs.push(std::vector< real_t >());
        }},
        {ARG, [&ctx](real_t v, real_t) {
            ctx.fargs.top().push_back(v);
        }},
        {FEND, [&ctx](real_t, real_t) {
            ctx.functions[ctx.fkey](ctx.args);
            ctx.fkey.pop();
            ctx.args.pop();
        }}
            
    };

   // const real_t ret = ctx.ast.Eval(ops);
#if defined(INLINE_EVAL)
    return ctx.values.front();
#elif defined(FUN_EVAL)
    return ctx.functions.front()();
#else
    return ctx.ast.Eval(ops);
#endif
}


//template < typename F, typename R, typename...ArgsT >
//std::function< R(ArgsT...) > MakeComposableTFun(const F& f) {
//    std::tuple< ArgsT... > args;
//    int count = 0;
//    const size_t ARITY = sizeof...(ArgsT);
//    return  [args, f, ARITY](auto r, auto n) mutable {
//        if(count == 0) std::g
//        if(count == ARITY) {
//            n = f(args[I]...);
//            count = 0;
//        }
//        return n;
//    };
//}


///Entry point
int main(int, char**) {
   
    return 0;
    //REPL, exit when input is empty
    string me;
    Ctx ctx;
    while(getline(cin, me)) {
        if(me.empty()) break;
        cout << "> " << MathParser(me, ctx) << endl;
        Reset(ctx);
    }
    return 0;
}

//ops[ast.Root()->Data().type](ast.Root(), map);
//ops[SUM] = [&m](WTRee* p) {
//    for(auto i: p->children_) {
//        
//    }
//}
//using one additional level of indirection it is possible to capture
//operations and perform real-time evaluation
//
//[SUM] = n(CSUM)
//[CSUM] = (n(PRODUCT), *((n(PLUS) / n(MINUS)), n(PRODUCT)));
//callback[SUM] is called when SUM->CSUM validates and if [NUMBER] loads
//e.g. into the context then sum can directly evaluate the stored numbers



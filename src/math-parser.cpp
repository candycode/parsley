
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
           NUMBER, POW, POWER, START, VAR, ASSIGN, ASSIGNMENT
};


template < typename T, typename W >
std::map< T, W > GenWeightedTerms(const std::vector< std::vector< T > >& vt,
                                  const std::vector< T >& scope,
                                  W start = W(0),
                                  const W& step = W(1000)) {
    std::map< T, W > m;
    for(auto& i: vt) {
        for(auto& j: i) {
            m.insert(j, start);
        }
        start += step;
    }
    for(auto& i: scope) {
        m.insert(i, start);
    }
    return m;
}

//template < typename T, typename W >
//    std::map< T, W > InsertWeightedTerms(const std::map< T, W >& m,
//                                         W w,
//                                         const std::vector< T >& v
//                                     ) {
//        const W f = m.begin()->second;
//        W l = f;
//        for(auto i: m) {
//            if(i.second != f) {
//                l = i.second;
//                break;
//            }
//        }
//        const W step = l - f;
//        W c = m.begin().second;
//        for(auto i: m) {
//            if(i.second == c) {
//                t.push_back(i.first)
//            } else {
//                vt.push_back(t);
//                t.resize(0);
//                c = i.second;
//                t.push_back(i.first);
//            }
//        }
//        
//}
    
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
    
    //The following two members are only required for just-in-time evaluation
    //performed from within HandleTerm2 function
    std::vector< real_t > values;
    TERM op = TERM();
    
    //The following is onlu required for deferred evaluation function built
    //in HandleTerm3
    using EvaluateFunction = std::function< real_t () >;
    std::vector< EvaluateFunction > functions;
};

void Reset(Ctx& c) {
    c.ast.Reset();
    c.values.clear();
    c.functions.clear();
    c.op = TERM();
}
    
real_t GenKey() {
    static real_t k = real_t(0);
    return k++;
}
    
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
    else if(t == CP) return true; //not adding marker of scope end
    else if(!v.empty()) ctx.ast.Add({t, Init(t)});
    return true;
};

    
bool Op(TERM t) {
    return t == PLUS
           || t == MINUS
           || t == MUL
           || t == DIV
           || t == POW
           || t == ASSIGN;
}
//build both tree and perform real-time evaluation using captures of
//SUM, PRODUCT etc.: values are stored into the context as they are read
//and each time an operation is encountered the first element of the
//value array is replaced with the result of the operation applied to
//the value array
bool HandleTerm2(TERM t, const Values& v, Ctx& ctx, EvalState es) {
    auto reset = [](real_t r, std::vector< real_t >& a) {
        a[0] = r;
        a.resize(1);
    };
    if(es == EvalState::BEGIN) return true;
    if(es == EvalState::FAIL) return false;
    ctx.op = Op(t) ? t : ctx.op;
    if(t == VAR) {
        if(In(Get(v), ctx.varkey)) {
            const real_t k = Get(ctx.varkey,Get(v));
            ctx.ast.Add({t, k});
            ctx.assignKey = k;
            ctx.values.push_back(ctx.keyvalue[k]);
        } else {
            const real_t k = GenKey();
            ctx.varkey[Get(v)] = k;
            ctx.keyvalue[k] = real_t();
            ctx.ast.Add({t, k});
            ctx.assignKey = k;
            ctx.values.push_back(std::numeric_limits<real_t>::quiet_NaN());
        }
    } else if(t == NUMBER) {
        ctx.values.push_back(Get(v));
        ctx.ast.Add({t, Get(v)});
    }
    else if(t == CP); //not adding marker of scope end
    else if(t == SUM) {
        if(ctx.values.size() > 0) {
            real_t r = real_t(0);
            if(ctx.op == PLUS) {
                for(auto i: ctx.values) r += i;
                reset(r, ctx.values);
            } else if(ctx.op == MINUS) {
                r = ctx.values[0];
                for(std::vector< real_t >::iterator i = ++ctx.values.begin();
                    i != ctx.values.end();
                    ++i)
                    r -= *i;
                reset(r, ctx.values);
            }
        }
        
    } else if(t == PRODUCT) {
        if(ctx.values.size() > 0) {
            real_t r = real_t(1);
            if(ctx.op == MUL) {
                for(auto i: ctx.values) r *= i;
                reset(r, ctx.values);
            } else if(ctx.op == DIV) {
                r = ctx.values[0];
                for(std::vector< real_t >::iterator i = ++ctx.values.begin();
                    i != ctx.values.end();
                    ++i)
                    r /= *i;
                reset(r, ctx.values);
            } else if(ctx.op == POW) {
                r = ctx.values[0];
                for(std::vector< real_t >::iterator i = ++ctx.values.begin();
                    i != ctx.values.end();
                    ++i)
                    r = pow(r, *i);
                reset(r, ctx.values);
            }
        }

    } else if(t == ASSIGNMENT) {
        if(ctx.op == ASSIGN) {
            assert(ctx.values.size() > 1);
            ctx.keyvalue[ctx.assignKey] = ctx.values[1];
            reset(ctx.values[1], ctx.values);
        }
    }
    else if(!v.empty()) ctx.ast.Add({t, Init(t)});
    return true;
}
    
//build both tree and perform real-time evaluation using captures of
//SUM, PRODUCT etc.: values are stored into the context as they are read
//and each time an operation is encountered the first element of the
//value array is replaced with the result of the operation applied to
//the value array
bool HandleTerm3(TERM t, const Values& v, Ctx& ctx, EvalState es) {
    auto freset = [](const Ctx::EvaluateFunction& f,
                     std::vector< Ctx::EvaluateFunction >& a) {
        a[0] = f;
        a.resize(1);
    };
    if(es == EvalState::BEGIN) return true;
    if(es == EvalState::FAIL) return false;
    ctx.op = Op(t) ? t : ctx.op;
    if(t == VAR) {
        if(In(Get(v), ctx.varkey)) {
            const real_t k = Get(ctx.varkey,Get(v));
            ctx.ast.Add({t, k});
            ctx.assignKey = k;
            ctx.functions.push_back([&ctx, k](){ return ctx.keyvalue[k]; });
        } else {
            const real_t k = GenKey();
            ctx.varkey[Get(v)] = k;
            ctx.keyvalue[k] = real_t();
            ctx.ast.Add({t, k});
            ctx.assignKey = k;
            ctx.functions.push_back([](){
                return std::numeric_limits< real_t >::quiet_NaN();
            });
        }
    } else if(t == NUMBER) {
        ctx.ast.Add({t, Get(v)});
        const real_t r = Get(v);
        ctx.functions.push_back([r]() { return r; });
    }
    else if(t == CP); //not adding marker of scope end
    else if(t == SUM) {
        if(ctx.functions.size() > 0) {
            if(ctx.op == PLUS) {
                Ctx::EvaluateFunction f = [](){ return real_t(0); };
                for(auto j: ctx.functions) f = [&ctx, f, j]() {
                    return f() + j();
                };
                freset(f, ctx.functions);
            } else if(ctx.op == MINUS) {
                Ctx::EvaluateFunction f = ctx.functions.front();
                for(std::vector< Ctx::EvaluateFunction >::iterator i
                    = ++ctx.functions.begin();
                    i != ctx.functions.end();
                    ++i)
                    f = [&ctx, f, i](){ return f() - (*i)(); };
                freset(f, ctx.functions);
            }
        }
        
    } else if(t == PRODUCT) {
        if(ctx.functions.size() > 0) {
            if(ctx.op == MUL) {
                Ctx::EvaluateFunction f = []() { return real_t(1); };
                for(auto i: ctx.functions) f = [&ctx, f, i]() {
                    return f() * i();
                };
                freset(f, ctx.functions);
            } else if(ctx.op == DIV) {
                Ctx::EvaluateFunction f = ctx.functions[0];
                for(std::vector< Ctx::EvaluateFunction >::iterator i
                    = ++ctx.functions.begin();
                    i != ctx.functions.end();
                    ++i)
                    f = [&ctx, f, i]() { return f() / (*i)(); };
                freset(f, ctx.functions);
            } else if(ctx.op == POW) {
                Ctx::EvaluateFunction f = ctx.functions[0];
                for(std::vector< Ctx::EvaluateFunction >::iterator i
                    = ++ctx.functions.begin();
                    i != ctx.functions.end();
                    ++i)
                    f = [&ctx, f, i]() { return pow(f(), (*i)()); };
                freset(f, ctx.functions);
            }
        }
        
    } else if(t == ASSIGNMENT) {
        if(ctx.op == ASSIGN) {
            assert(ctx.functions.size() > 1);
            const real_t k = ctx.assignKey;
            const Ctx::EvaluateFunction v = ctx.functions[1];
            Ctx::EvaluateFunction f = [&ctx, k, v]() {
                const real_t r = v();
                ctx.keyvalue[k] = r;
                return r;
            };
            freset(f, ctx.functions);
        }
    }
    else if(!v.empty()) ctx.ast.Add({t, Init(t)});
    return true;
}

    
    
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
    g[EXPR]    = c(SUM);
    g[SUM]     = (c(PRODUCT), *((n(PLUS) / n(MINUS)) & c(PRODUCT)));
    g[PRODUCT] = (c(POWER), *((n(MUL) / n(DIV) / n(POW)), n(VALUE)));
    g[POWER]   = (n(VALUE), *(n(POW), n(VALUE)));
    g[VALUE]   = c(ASSIGNMENT) / n(NUMBER) / (n(OP), n(EXPR), n(CP));
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
    InStream is(iss);//, [](Char c) {return c == ' ' || c == '\t';});
    ctx.ast.SetWeights(weights);
    ActionMap am;
#ifdef INLINE_EVAL
    auto HT = HandleTerm2;
#elif FUN_EVAL
    auto HT = HandleTerm3;
#else
    auto HT = HandleTerm;
#endif
    Set(am, HT, NUMBER,
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
#ifdef INLINE_EVAL
    assert(ctx.values[0] == ret);
#elif FUN_EVAL
    assert(ctx.functions[0]() == ret);
#endif
    return ret;
}


int main(int, char**) {
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




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
           NUMBER, POW, START, VAR, ASSIGN, ASSIGNMENT,
           FUNCTION, FBEGIN, FEND, FSEP
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
        {{ASSIGN},
        {PLUS},
        {MINUS},
        {MUL, DIV},
        {POW},
        {FBEGIN},
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
    
    

//in case functions implementing operators are not of standard
//C function type and do not define 'return_type' it is required to also
//specialize Return<T> to allow for return type deduction
//namespace parsley {
//template <>
//struct Return< MyCallableType > {
//    using Type = AReturnType;
//};
//}

//using unordered_map with TERM type as key => hash function needed
struct TERMHash {
    std::size_t operator()(TERM k) const {
        using std::size_t;
        using std::hash;
        return hash<int>()(int(k));
    }
};
    
//Syntax tree type used by HandleTerm callback only
using AST = STree< Term, std::map< TERM, int > >;

using Args = std::vector< real_t >;
using F = std::function< real_t (const Args&) >;
using FunLUT = std::unordered_map<real_t, F >;
using VarLUT = std::unordered_map<real_t, real_t >;
using Name2Key = std::unordered_map< std::string, real_t >;
using Op2Key = std::unordered_map< TERM, real_t, TERMHash >;
    
//Context
struct Ctx {
    AST ast;
    FunLUT fl = {
        {0, [](const Args& args)
            { real_t r = 0;
                for(auto i: args) {cout << i << ' '; r += i;}
                cout << endl;
              return r;}},
        {1, [](const Args& args) { return args[0] - args[1]; }},
        {2, [](const Args& args) { return args[0] * args[1]; }},
        {3, [](const Args& args) { return args[0] / args[1]; }},
        {4, [](const Args& args) { return sin(args[0]); }},
        {5, [](const Args& args) { return cos(args[0]); }},
        {6, [](const Args& args) { return pow(args[0], args[1]); }}
    };
    Name2Key fn = {
        {"sin", 4},
        {"cos", 5},
        {"pow", 6},
        {"sum", 0}
    };
    Op2Key ops = {
        {PLUS, 0},
        {MINUS, 1},
        {MUL, 2},
        {DIV, 3},
        {POW, 6}
    };
    Name2Key varkey;
    VarLUT vl;
};
    
    
//generate new key used for variable->value mapping
real_t GenKey() {
    static real_t k = real_t(0);
    return k++;
}

///True if term is an operation
bool Op(TERM t) {
    return t == PLUS
    || t == MINUS
    || t == MUL
    || t == DIV
    || t == POW;
}
    
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
    
    
//Parser callback: simply add terms to syntax tree
bool HandleTerm(TERM t, const Values& v, Ctx& ctx, EvalState es) {
    if(es == EvalState::BEGIN) return true;
    if(es == EvalState::FAIL) return false;
    if(Op(t)) {
        ctx.ast.Add({t, ctx.ops[t]});
    } else if(t == VAR) {
        if(In(Get(v), ctx.varkey)) {
            const real_t k = Get(ctx.varkey, Get(v));
            ctx.ast.Add({t, k});
        } else {
            const real_t k = GenKey();
            ctx.varkey[Get(v)] = k;
            ctx.vl[k] = std::numeric_limits<real_t>::quiet_NaN();
            ctx.ast.Add({t, k});
        }
    } else if(t == NUMBER) {
        ctx.ast.Add({t, Get(v)});
    } else if(t == OP) {
        ctx.ast.OffsetInc(OP);
        return true;
    } else if(t == CP) {
        ctx.ast.OffsetDec(CP);
        return true;
    //function evaluation: f(a, b, c) -> f( (a) (b) (c) )
    } else if(t == FBEGIN) {
        assert(In(v.find("name")->second, ctx.fn));
        ctx.ast.Add({t, Get(ctx.fn, v.find("name")->second)});
        ctx.ast.OffsetInc(OP, 2);
        return true;
    } else if(t == FEND) {
        ctx.ast.OffsetDec(CP, 2);
        return true;
    } else if(t == FSEP) {
        //ctx.ast.OffsetDec(CP);
        return true;
    } else if(!v.empty()) {
        ctx.ast.Add({t, Init(t)});
    }
    return true;
};
    
    
///Parser callback type
using ActionFun = std::function< bool (TERM, const Values&, Ctx&, EvalState) >;
    

using ParsingRules = std::map< TERM, EvalFun >;
using ActionMap = std::map< TERM, ActionFun  >;

///Generate parser table map: each element represent a parsing rule
ParsingRules GenerateParser(ActionMap& am, Ctx& ctx) {
    
    ParsingRules g; // grammar
    auto n  = MAKE_CALL(TERM, g, am, ctx);
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
    g[SUM]      = (n(PRODUCT), *((n(PLUS) / n(MINUS)), n(PRODUCT)));
    g[PRODUCT]  = (n(VALUE), *((n(MUL) / n(DIV) / n(POW)), n(VALUE)));
    //warning: function has the same name format as a VAR, parse before!
    g[VALUE]    = (n(OP), n(EXPR), n(CP)) / n(FUNCTION)
                  / n(ASSIGNMENT) / n(NUMBER);
    g[FUNCTION] = (n(FBEGIN), ZO(n(EXPR) & *(n(FSEP) & n(EXPR))),n(FEND));
    g[ASSIGNMENT]  = (n(VAR), ZO((n(ASSIGN), n(EXPR))));
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


///Per-node evaluation function: @todo consider putting context in class or
///using a context hierarchy, used as stack-frame data holder
///A stack of arg arrays is used to keep separate per-tree-level arguments, this
///approach allows to be arity-free i.e. no need to specify the arity of each
///operator/function: each function accepts a list of arguments as a vector
///Alternative and more efficient option is to use a single vector to hold
///all the arguments, in this case though the functions have to be modified
///from
/// process elaments 0, ...N
///to
/// process elements size - arity, ...size - arity + N
///also the EvalFrame class needs to be aware of the arity of each function
///to properly resize the array after each function application:
///from
/// args_.pop();
/// args_.top().push_back(ret);
///to
/// args_.resize(args_.size() - arity_[function id]);
/// args_.push_back(ret);
class EvalFrame {
private:
    using Args = std::vector< real_t >;
    using ArgStack = std::stack< Args, vector< vector< real_t > > >;
    using Function = std::function< real_t (const Args&) >;
    using FunStack = std::stack< Function >;
    using FunLUT = std::unordered_map< real_t, Function >;
    using VarLUT = std::unordered_map< real_t, real_t >;
    using FLUTRef = std::reference_wrapper< FunLUT >;
    using VLUTRef = std::reference_wrapper< VarLUT >;
public:
    EvalFrame(FunLUT& f, VarLUT& v) : flut_(f), vlut_(v) { args_.push(Args()); }
    EvalFrame(const EvalFrame&) = default;
    EvalFrame(EvalFrame&&) = default;
    EvalFrame& operator=(const EvalFrame&) = default;
    EvalFrame& operator=(EvalFrame&&) = default;
    EvalFrame operator()(const Term& t, APPLY stage) {
        if(stage == APPLY::BEGIN) {
            switch(t.type) {
                case NUMBER:
                    args_.top().push_back(t.value);
                    break;
                case VAR:
                    if(last_ == ASSIGN) {
                        if(vlut_.get().find(t.value) != vlut_.get().end())
                            args_.top().push_back(t.value);
                        else args_.top().push_back(GenKey());
                    }
                    else args_.top().push_back(vlut_.get()[t.value]);
                    break;
                default: {
                    args_.push(Args());
                    f_.push(flut_.get()[t.value]);
                }
                break;
            }
            last_ = t.type;
        } else if(stage == APPLY::END
                  && t.type != NUMBER
                  && t.type != VAR) {
            if(t.type == ASSIGN) {
                const real_t ret = args_.top()[1];
                vlut_.get()[args_.top()[0]] = ret;
                args_.pop();
                args_.top().push_back(ret);
            } else {
                const real_t ret = f_.top()(args_.top());
                f_.pop();
                args_.pop();
                args_.top().push_back(ret);
            }
        }
        return *this;
    }
    const Args& Values() const { return args_.top(); }
    const real_t Result() const {
        assert(!args_.empty());
        assert(!args_.top().empty());
        return args_.top().front();
    }
private:
    TERM last_ = TERM();
    ArgStack args_;
    FunStack f_;
    FLUTRef flut_;
    VLUTRef vlut_;
};


//restore context to initial state
void Reset(Ctx& c) {
    c.ast.Reset();
}

///Parse expression and return evaluated result
real_t MathParser(const string& expr, Ctx& ctx) {
    istringstream iss(expr);
    InStream is(iss);//, [](Char c) {return c == ' ' || c == '\t';});
    ctx.ast.SetWeights(weights);
    ActionMap am;
    auto HT = HandleTerm;
    Set(am, HT, NUMBER,
        EXPR, OP, CP, PLUS, MINUS, MUL, DIV, POW, SUM, PRODUCT, VALUE,
        ASSIGN, ASSIGNMENT, VAR, FBEGIN, FEND, FSEP);
    ParsingRules g = GenerateParser(am, ctx);
    g[START](is);
    
    if(is.tellg() < expr.size()) cerr << "ERROR AT: " << is.tellg() << endl;
#ifdef PRINT_TREE
    int s = 0;
    cout << '\n';
    ctx.ast.Apply([s](const Term& t) mutable {
        if(t.type != NUMBER) s += 2;
        else if(ScopeEnd(t)) s -= 2;
        assert(s >= 0);
        cout << string(s, '.') << t.type << ' ' << t.value << endl;
    });
#endif

    return ctx.ast.ScopedApply(EvalFrame(ctx.fl, ctx.vl)).Result();
}

} //anonymous namespace


///Entry point
int main(int, char**) {

    //REPL, exit when input is empty
    string me;
    Ctx ctx;
//    cout << MathParser("1+(2+3)", ctx);
//    return 0;
//    MathParser("sum(1,2)", ctx);
    while(getline(cin, me)) {
        if(me.empty()) break;
        cout << "> " << MathParser(me, ctx) << endl;
        Reset(ctx);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////////////
//Parsley - parsing framework
//Copyright (c) 2010-2015, Ugo Varetto
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
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
//IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL UGO VARETTO BE LIABLE FOR ANY
//DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////

///WORK IN PROGRESS: MINIMAL NEW IMPLEMENTATION OF RECURSIVE PEG PARSER
//(WITH INFINITE LOOKAHEAD)
//MEMOIZATION: PARSERS ARE APPLIED ONCE AND ONLY ONCE PER STREAM POSITION
//(A LA PACKRAT), DATA STORED IN CLOSURE
//note that parsing is *not* context-free because advancement depends on both
//the grammar *and* the return value of the context-aware callback function

#ifdef HASH_MAP
#include <unordered_map>
#else
#include <map>
#endif

#include <iostream>
#include <sstream>
#include <functional>
#include <string>
#include <algorithm>
#include <limits>
#include <array>

#include <parsers.h>
#include <InStream.h>
#include <types.h>

using namespace parsley;
using namespace std;

using NonTerminal = Parser;

enum class EvalState {BEGIN, PASS, FAIL};

using EvalFun = std::function< bool (InStream&) >;


///@todo add operator () to parser so that function is independent from
///Parser class
template < typename InStreamT,
typename KeyT,
typename EvalMapT,
typename ActionMapT,
typename ContextT >
typename EvalMapT::value_type::second_type
MakeTermEval(KeyT k,
             const EvalMapT& em,
             Parser p, //use an empty parser to specify non-terminal term
             //.., NonTerminal(),...
             ActionMapT& am,
             ContextT& c,
             bool cback = true) {
    std::size_t sp = std::numeric_limits<std::size_t>::max();
    bool last = false;
    return [cback, last, sp, k, &em, p, &am, &c](InStreamT& is)
            mutable -> bool {
        if(is.tellg() == sp) return last;
        assert(em.find(k) != em.end());
        assert(am.find(k) != am.end());
        bool ret = true;
        std::size_t i = is.tellg();
        if(cback) {
            ret = am[k](k, Values(), c, EvalState::BEGIN);
        }
        if(!p.Empty()) {
            ret = ret && p.Parse(is);
            if(cback) {
                const EvalState s = ret ? EvalState::PASS : EvalState::FAIL;
                ret = ret && am[k](k, p.GetValues(), c, s);
            }
        } else {
            ret = ret && em.find(k)->second(is);
            if(cback) {
                const EvalState s = ret ? EvalState::PASS : EvalState::FAIL;
                ret = ret && am[k](k, Values(), c, s);
            }
        }
        sp = i;
        last = ret;
        return ret;
    };
}

EvalFun OR() {
    return [](InStream&) { return false; };
}

template < typename F, typename...Fs >
EvalFun OR(F f, Fs...fs) {
    return [=](InStream& is)  {
        bool pass = false;
        pass = f(is);
        if(pass) return true;
        return OR(fs...)(is);
    };
}


EvalFun AND() {
    return [](InStream& is) { return true; };
}

template < typename F, typename...Fs >
EvalFun AND(F f, Fs...fs) {
    return [=](InStream& is) {
        bool pass = false;
        REWIND r(pass, is);
        pass = f(is);
        if(!pass) return false;
        pass = AND(fs...)(is);
        return pass;
    };
}

template < typename F >
EvalFun ZM(F f) {
    return [=](InStream& is) {
        while(!is.eof() && is.good() && f(is));
        return true;
    };
}

template < typename F >
EvalFun OM(F f) {
    return [=](InStream& is) {
        if(!f(is)) return false;
        return ZM(f)(is);
    };
}

template < typename F >
EvalFun NOT(F f) {
    return [f](InStream& is) {
        Char c = 0;
        StreamPos pos = is.tellg();
        bool pass = false;
        while(is.good() && !f(is)) {
            pass = true;
            c = is.get();
            if(!is.good()) break;
            pos = is.tellg();
        }
        //go back to position before successful
        //parser applied
        if(is.good()) is.seekg( pos );
        return pass;
    };
}

template< typename F >
EvalFun GREEDY(F f) {
    return [f](InStream& is) {
        bool r = f(is);
        while(r) r = f(is);
        return r;
    };
}

//Returns a function which forwards calls to other function in grammar
//map
template < typename KeyT, typename EvalMapT, typename ActionMapT,
           typename ContextT >
EvalFun Call(KeyT key, const EvalMapT& em, ActionMapT& am, ContextT& c,
             bool cback) {
    return MakeTermEval<InStream>(key, em, Parser(), am, c, cback);
}

#define MAKE_CALL(K, G, AM, CTX) \
[& G, & AM, & CTX](K t) { return Call(t, G, AM, CTX, false); }

#define MAKE_CBCALL(K, G, AM, CTX) \
[& G, & AM, & CTX](K t) { return Call(t, G, AM, CTX, true); }

#define MAKE_EVAL(K, G, AM, CTX) \
[& G, & AM, & CTX](TERM t, Parser p) { \
return MakeTermEval<InStream>(t, g, p, am, ctx); \
}

EvalFun operator&(const EvalFun& e1, const EvalFun& e2) {
    return AND(e1, e2);
}

EvalFun operator/(const EvalFun& e1, const EvalFun& e2) {
    return OR(e1, e2);
}

EvalFun operator*(const EvalFun& e) {
    return ZM(e);
}

EvalFun operator+(const EvalFun& e) {
    return OM(e);
}

EvalFun operator!(const EvalFun& e) {
    return NOT(e);
}

EvalFun operator,(const EvalFun& e1, const EvalFun& e2) {
    return AND(e1, e2);
}


template < typename F, typename KeyT, typename CB, typename Ctx >
F Invoke(const EvalFun& f, const CB& cb, Ctx& c,
         KeyT k = KeyT(), const Values& def = Values()) {
    return [cb, f, k, &c, def](InStream& is) {
        cb(k, def, EvalState::BEGIN);
        bool ret = f(is);
        if(ret) cb(k, def, c, ret ? EvalState::PASS : EvalState::FAIL);
    };
}

template < typename F, typename KeyT, typename MapT, typename Ctx >
F InvokeMapped(const EvalFun& f, KeyT k, const MapT& m, Ctx& c,
               const Values& def = Values()) {
    return [f, k, &m, &c, def](InStream& is) {
        m.find(k)->second(k, def, c, EvalState::BEGIN);
        bool ret = f(is);
        if(ret) m.find(k)->second(k, def, c,
                                  ret ? EvalState::PASS : EvalState::FAIL);
    };
}

template < typename M >
void Set(M& m, typename M::value_type::second_type ) {}

template < typename M, typename KeyT, typename...KeysT >
void Set(M& m, typename M::value_type::second_type f, KeyT k, KeysT...ks) {
    m[k] = f;
    Set(m, f, ks...);
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


//==============================================================================
enum TERM {EXPR = 1, OP, CP, VALUE, PLUS, MINUS, MUL, DIV, SUM, PRODUCT,
           NUMBER, POW, POWER};

std::map< TERM, int > weights = {
    {NUMBER, 0},
    {OP, 0},
    {CP, 0},
    {POW, 1},
    {MUL, 2},
    {DIV, 2},
    {MINUS, 3},
    {PLUS, 4}
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


#ifdef HASH_MAP
DEFINE_HASH(TERM)
#endif

struct Ctx {};
using ActionFun = std::function< bool (TERM, const Values&, Ctx&, EvalState) >;
#ifdef HASH_MAP
using Map = std::unordered_map< TERM, EvalFun >;
using ActionMap = std::unordered_map< TERM, ActionFun >;
#else
using Map = std::map< TERM, EvalFun >;
using ActionMap = std::map< TERM, ActionFun  >;
#endif

void Go() {
    Ctx ctx;
    ActionMap am;
    int indent = 0;
    auto handleTerm = [&indent](TERM t, const Values& v, Ctx&, EvalState es) {
        if(es == EvalState::BEGIN) {
            ++indent;
            return true;
        } else if(es == EvalState::FAIL) {
            --indent;
            return false;
        }
        if(!v.empty())
        cout << std::string(indent, '`') << ' ' << TermToString(t);
        if(t == NUMBER) {
            if(!v.empty()) cout << v.begin()->second;
        }
        if(!v.empty()) cout << endl;
        --indent;
        return true;
    };
    auto handleWeightedTerm = [&indent](TERM t, const Values& v, Ctx&,
                                        EvalState es) {
        if(es == EvalState::BEGIN) return true;
        else if(es == EvalState::FAIL) return false;
        if(ScopeBegin(t)) ++indent;
        if(!v.empty())
            cout << std::string((indent + weights[t]), ' ')
            << TermToString(t);
        if(t == NUMBER) {
            if(!v.empty()) cout << v.begin()->second;
        }
        if(ScopeEnd(t)) --indent;
        if(!v.empty()) cout << endl;
        return true;
    };

    Set(am, handleWeightedTerm, NUMBER,
        EXPR, OP, CP, PLUS, MINUS, MUL, DIV, POW, SUM, PRODUCT, POWER, VALUE);
    //callback function also receives key: it is possible to have same
    //callbak handle multiple keys; to assign the same callback to multiple
    //keys in action map use Set helper:
    //    ActionMap A;
    //    Set(A,
    //        [](TERM t, const Values&, Ctx&) {
    //           cout << t << endl; return true; },
    //        OP, CP, MINUS, MUL);
    Map g; // grammar
    auto c = MAKE_CBCALL(TERM, g, am, ctx);
    auto n = MAKE_CALL(TERM, g, am, ctx);
    auto mt = MAKE_EVAL(TERM, g, am, ctx);
    //grammar definition
    //non-terminal - note the top-down definition through deferred calls using
    //the 'c' helper function
    g[EXPR]    = n(SUM);
    //Option 1:
    //    g[SUM]     = AND(c(PRODUCT), ZM(AND(OR(c(PLUS), c(MINUS)), c(PRODUCT))));
    //    g[PRODUCT] = AND(c(VALUE), ZM(AND(OR(c(MUL), c(DIV), c(POW), c(VALUE))));
    //    g[VALUE]   = OR(c(NUMBER), AND(c(OP),c(EXPR), c(CP)));
    //Option 2:
    //    g[SUM]     = c(PRODUCT) & *((c(PLUS) / c(MINUS)) & c(PRODUCT));
    //    g[PRODUCT] = c(VALUE) & *((c(MUL) / c(DIV) / c(POW)) & c(VALUE));
    //    g[VALUE]   = c(NUMBER) / (c(OP) & c(EXPR) & c(CP));
    //Option 3: commas for ANDs, parenthesis required, if not they are parsed
    //          with standard rules
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
    //text
    istringstream is("6*((2+-3)+7^9)");
    InStream ins(is);
    //invoke parser
    g[EXPR](ins);
    //NOTE: EOF is set AFTER TRYING TO READ PAST THE END OF THE STREAM!
    //check
    if(!ins.eof()) std::cout << "ERROR AT: " << ins.get_lines() << ", "
                             << ins.get_line_chars() << std::endl;
}

//------------------------------------------------------------------------------
int main(int, char**) {
    //Go();
    void TreeTest();
    TreeTest();
   	return 0;
}

//LEFTOVERS
//******************************************************************************
//#if 0
//namespace std {
//    template <>
//    struct hash<TERM>
//    {
//        std::size_t operator()(TERM k) const
//        {
//            using std::size_t;
//            using std::hash;
//            
//            // Compute individual hash values for first,
//            // second and third and combine them using XOR
//            // and bit shifting:
//            
//            return std::hash<int>()(int(k));
//        }
//    };
//}
//#endif
//
//
//template < typename T, std::size_t N > class CircularBuffer {
//public:
//    using value_type = T;
//    static size_t Size() { return  N; }
//    CircularBuffer& Put(const T& v) {
//        buf_[p_] = v;
//        p_ = (p_ + 1) % N;
//        return *this;
//    }
//    const T& Get() const { return buf_[p_]; }
//    T& Get() { return buf_[p_]; }
//    bool Same(const T& v) const {//returns true iff all elements are the same
//        for(auto i: buf_) if(i != v) return false;
//        return true;
//    }
//    void Back() {
//        p_ = p_ - 1;
//        p_ = p_ < 0 ? N - p_ : p_;
//    }
//    void Dup() {
//        Put(Get());
//    }
//    template < typename F >
//    F Apply(F f) {
//        for(auto i: buf_) f(i);
//        return f;
//    }
//private:
//    std::array<T, N > buf_;
//    std::size_t p_ = 0;
//};
//
//#if 0
//template < typename KeyT, typename EvalMapT, typename ActionMapT, typename
//ContextT >
//struct C {
//    C(CircularBuffer<KeyT, 2>& l, KeyT k, const EvalMapT& e, ActionMapT& a,
//      ContextT& c) : last(l), terminate(false),
//    key(k), em(std::cref(e)), am(std::ref(a)), ctx(std::ref(c)),
//    prev(std::numeric_limits<std::size_t>::max()) {}
//    bool operator()(InStream& is) const {
//        const EvalMapT& m = em.get();
//        //if(last.get().Get() == key) return false;
//        ActionMapT& a = am.get();
//        assert(m.find(key) != m.end());
//        //last.get().Put(key);
//        //last.get().Put(key);
//        const bool pass = m.find(key)->second(is);
//        //assert(a.find(key) != a.end());
//        //if(pass) a.find(key)->second(Values(), ctx);
//        return pass;
//    }
//    mutable bool terminate = false;
//    mutable std::size_t prev;
//    KeyT key;
//    std::reference_wrapper<const EvalMapT> em;
//    mutable std::reference_wrapper<ActionMapT> am;
//    mutable std::reference_wrapper<ContextT> ctx;
//    mutable std::reference_wrapper< CircularBuffer<KeyT, 2> > last;
//};
//#endif
//
////User must implement function
//template < typename KeyT >
//KeyT InitKey();
//template <>
//TERM InitKey<TERM>() { return TERM(); }
//#if 0
//{{NUMBER, [](TERM t, const Values& v, Ctx&, EvalState) {
//    std::cout << "NUMBER: ";
//    if(!v.empty()) cout << v.begin()->second;
//    cout << std::endl;
//    return true;}},
//    {EXPR, [](TERM t, const Values& , Ctx&, EvalState) {
//        std::cout << "EXPR" << std::endl; return true;}},
//    {OP, [](TERM t, const Values& , Ctx&, EvalState) {
//        std::cout << "OP" << std::endl; return true;}},
//    {CP, [](TERM t, const Values& , Ctx&, EvalState) {
//        std::cout << "CP" << std::endl; return true;}},
//    {PLUS, [](TERM t, const Values& , Ctx&, EvalState) {
//        std::cout << "+" << std::endl; return true;}},
//    {MINUS, [](TERM t, const Values& , Ctx&, EvalState) {
//        std::cout << "-" << std::endl; return true;}},
//    {MUL, [](TERM t, const Values& , Ctx&, EvalState) {
//        std::cout << "*" << std::endl; return true;}},
//    {DIV, [](TERM t, const Values& , Ctx&, EvalState) {
//        std::cout << "/" << std::endl; return true;}},
//    {POW, [](TERM t, const Values& , Ctx&, EvalState) {
//        std::cout << "^" << std::endl; return true;}},
//    {SUM, [](TERM t, const Values& , Ctx&, EvalState) {
//        std::cout << "SUM" << std::endl; return true;}},
//    {PRODUCT, [](TERM t, const Values& , Ctx&, EvalState) {
//        std::cout << "PRODUCT" << std::endl; return true;}},
//    {POWER, [](TERM t, const Values& , Ctx&, EvalState) {
//        std::cout << "POWER" << std::endl; return true;}},
//    {VALUE, [](TERM t, const Values& , Ctx&, EvalState) {
//        std::cout << "VALUE" << std::endl; return true;}}
//    };
//#endif



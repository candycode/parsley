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
//MEMOIZATION: PARSERS ARE APPLIED ONCE AND ONLY ONCE (A LA PACKRAT), DATA
//STORED IN CLOSURE
//note that parsing is *not* context-free because advancement depends on both
//the grammar *and* the return value of the context-aware callback function

#include <map>
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
             ContextT& c) {
    std::size_t sp = std::numeric_limits<std::size_t>::max();
    bool last = false;
    return [last, sp, k, &em, p, &am, &c](InStreamT& is) mutable -> bool {
        if(is.tellg() == sp) return last;
        assert(em.find(k) != em.end());
        assert(am.find(k) != am.end());
        bool ret = false;
        std::size_t i = is.tellg();
        if(!p.Empty()) {
            ret = p.Parse(is);
            ret = ret && am[k](p.GetValues(), c);
        } else {
            ret = em.find(k)->second(is);
            ret = ret && am[k](Values(), c);
        }
        sp = i;
        last = ret;
        return ret;
    };
}

using EvalFun = std::function< bool (InStream&) >;

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

//currently callbacks are called only for items parsed through
//MakeTermEval: can modify to *not* call callback from within MakeTermEval
//and call callback from Call only, but this would force client code
//to *always* use Call; other option configure MakeTermEval with parameter
//to enable/disable callback invocation upon successful parsing
template < typename KeyT, typename EvalMapT, typename ActionMapT,
           typename ContextT >
EvalFun Call(KeyT key, const EvalMapT& em, ActionMapT& am, ContextT& c) {
    std::size_t sp = std::numeric_limits<std::size_t>::max();
    return [sp, &em, &am, key, &c](InStream& is) mutable {
        if(is.tellg() == sp) return true;
        assert(em.find(key) != em.end());
        std::size_t i = is.tellg();
        const bool ret = em.find(key)->second(is);
        if(ret) sp = i;
        return ret;
    };
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

//==============================================================================
enum TERM {EXPR = 1, OP, CP, VALUE, PLUS, MINUS, MUL, DIV, SUM, PRODUCT,
           NUMBER};

struct Ctx {};
using Map = std::map< TERM, EvalFun >;
using ActionMap = std::map< TERM, std::function< bool (const Values&, Ctx&) > >;


void Go() {
    Ctx ctx;
    ActionMap am  {{NUMBER, [](const Values& v, Ctx&) {
                        std::cout << "VALUE: ";
                        if(!v.empty()) cout << v.begin()->second;
                        cout << std::endl;
                        return true;}},
        {EXPR, [](const Values& , Ctx&) {
            std::cout << "EXPR " << std::endl; return true;}},
        {OP, [](const Values& , Ctx&) {
            std::cout << "OP " << std::endl; return true;}},
        {CP, [](const Values& , Ctx&) {
            std::cout << "CP " << std::endl; return true;}},
        {PLUS, [](const Values& , Ctx&) {
            std::cout << "+ " << std::endl; return true;}},
        {MINUS, [](const Values& , Ctx&) {
            std::cout << "- " << std::endl; return true;}},
        {MUL, [](const Values& , Ctx&) {
            std::cout << "* " << std::endl; return true;}},
        {DIV, [](const Values& , Ctx&) {
            std::cout << "/ " << std::endl; return true;}}};
    
    
#define MAKE_CALL(K, G, AM, CTX) \
     [& G, & AM, & CTX](K t) { return Call(t, G, AM, CTX); };
    Map g;
    auto c = MAKE_CALL(TERM, g, am, ctx);
    //auto c = [&g, &am, &ctx](TERM t) { return Call(t, g, am, ctx); };
#define MAKE_EVAL(K, G, AM, CTX) \
    [& G, & AM, & CTX](TERM t, Parser p) { \
        return MakeTermEval<InStream>(t, g, p, am, ctx); \
    };
    auto mt = MAKE_EVAL(TERM, g, am, ctx);

    //terminal
    g[NUMBER] = mt(NUMBER, FloatParser());
    g[OP]     = mt(OP, ConstStringParser("("));
    g[CP]     = mt(CP, ConstStringParser(")"));
    g[PLUS]   = mt(PLUS, ConstStringParser("+"));
    g[MINUS]  = mt(MINUS, ConstStringParser("-"));
    g[MUL]    = mt(MUL, ConstStringParser("*"));
    g[DIV]    = mt(DIV, ConstStringParser("/"));
    
    //non-terminal
    g[EXPR]    = c(SUM);
//    g[SUM]     = AND(c(PRODUCT), ZM(AND(OR(c(PLUS), c(MINUS)), c(PRODUCT))));
//    g[PRODUCT] = AND(c(VALUE), ZM(AND(OR(c(MUL), c(DIV)), c(VALUE))));
//    g[VALUE]   = OR(c(NUMBER), AND(c(OP),c(EXPR), c(CP)));
    g[SUM]     = c(PRODUCT) & *((c(PLUS) / c(MINUS)) & c(PRODUCT));
    g[PRODUCT] = c(VALUE) & *((c(MUL) / c(DIV)) & c(VALUE));
    g[VALUE]   = c(NUMBER) / (c(OP) & c(EXPR) & c(CP));

    //text
    istringstream is("6+((2+-3)+1)");
    InStream ins(is);
    
    //invoke parser
    g[EXPR](ins);
    
    //NOTE: EOF is set AFTER TRYING TO READ PAST THE END OF THE STREAM!
    //check
    if(!ins.eof()) std::cout << "ERROR AT: " << ins.tellg() << std::endl;
}

//------------------------------------------------------------------------------
int main(int, char**) {
    Go();
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


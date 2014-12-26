////////////////////////////////////////////////////////////////////////////////
//Copyright (c) 2010-2014, Ugo Varetto
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
//THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
//ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
//DISCLAIMED. IN NO EVENT SHALL UGO VARETTO BE LIABLE FOR ANY
//DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
//(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
//LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
//ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
//(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
//SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
////////////////////////////////////////////////////////////////////////////////

///WORK IN PROGRESS: MINIMAL NEW IMPLEMENTATION OF RECURSIVE PEG PARSER (WITH INFINITE LOOKAHEAD)

#include <map>
#include <iostream>
#include <sstream>
#include <functional>
#include <string>
#include <algorithm>
#include <limits>

#include <parsers.h>
#include <InStream.h>
#include <types.h>

using namespace parsley;
using namespace std;

using NonTerminal = Parser;


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
    return [k, &em, p, &am, &c](InStreamT& is) mutable -> bool {
        assert(em.find(k) != em.end());
        assert(am.find(k) != am.end());
        bool ret = false;
        if(!p.Empty()) {
            ret = p.Parse(is)
                  && am[k](p.GetValues(), c);
        } else {
            ret = em.find(k)->second(is)
                  && am[k](Values(), c);
        }
        return ret;
    };
}


using EvalFun = std::function< bool (InStream&) >;




EvalFun OR() {
    return [](InStream&) { return false; };
}

template < typename F, typename...Fs >
EvalFun OR(F f, Fs...fs) {
    return [=](InStream& is) {
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
    return [f, fs...](InStream& is) {
        bool pass = false;
        REWIND r(pass, is);
        pass = f(is);
        if(!pass) return false;
        pass = AND(fs...)(is);
        return pass;
    };
}

////the following works only if the parsers DO NOT REWIND
//template < typename F >
//EvalFun NOT(F f) {
//    return [f](InStream& is, bool ) {
//        Char c = 0;
//        StreamPos pos = is.tellg();
//        bool pass = false;
//        while(is.good() && !f(is)) {
//            pass = true;
//            c = is.get();
//            if(!is.good()) break;
//            pos = is.tellg();
//        }
//        if(is.good()) is.seekg( pos );
//        return pass;
//    };
//}
//
//template< typename F >
//EvalFun GREEDY(F f) {
//    return [f](InStream& is, bool) {
//        bool r = f(is);
//        while(r) r = f(is);
//        return r;
//    };
//}

#if 0
template < typename KeyT, typename EvalMapT, typename ActionMapT, typename ContextT >
struct C {
    C(KeyT k, const EvalMapT& e, ActionMapT& a, ContextT& c) : terminate(false),
    key(k), em(std::cref(e)), am(std::ref(a)), ctx(std::ref(c)) {}
    bool operator()(KeyT k, InStream& is) const {
        const EvalMapT& m = em.get();
        ActionMapT& a = am.get();
        assert(m.find(key) != m.end());
        if(terminate) {
            terminate = false;
            return false;
        }
        if(k == key) terminate = true;
        const bool pass = m.find(key)->second(key, is);
        assert(a.find(key) != a.end());
        if(pass) a.find(key)->second(Values(), ctx);
        return pass;
    }
    mutable bool terminate = false;
    KeyT key;
    std::reference_wrapper<const EvalMapT> em;
    mutable std::reference_wrapper<ActionMapT> am;
    mutable std::reference_wrapper<ContextT> ctx;
};
#endif

template < typename KeyT, typename EvalMapT, typename ActionMapT, typename ContextT >
EvalFun Call(KeyT key, const EvalMapT& em, ActionMapT& am, ContextT& c) {
    KeyT P = KeyT();
    return [P, &em, &am, key, &c](InStream& is) mutable {
        if(P == key) {
            P = KeyT();
            return false;
        }
        P = key;
        assert(em.find(key) != em.end());
        const bool ret = em.find(key)->second(is);
        if(ret) {
            assert(am.find(key) != am.end());
            am[key](Values(), c);
        }
        return ret;
    };
    //return C< KeyT, EvalMapT, ActionMapT, ContextT >(key, em, am, c);
}

enum TERM {EXPR = 1, EXPR_, OP, CP, VALUE, PLUS, MINUS, MUL, DIV, T, EOS};

#if 0
namespace std {
    template <>
    struct hash<TERM>
    {
        std::size_t operator()(TERM k) const
        {
            using std::size_t;
            using std::hash;
            
            // Compute individual hash values for first,
            // second and third and combine them using XOR
            // and bit shifting:
            
            return std::hash<int>()(int(k));
        }
    };
}
#endif

struct Ctx {};
using Map = std::map< TERM, EvalFun >;
using ActionMap = std::map< TERM, std::function< bool (const Values&, Ctx&) > >;


void Go() {
    Ctx ctx;
    ActionMap am  {{VALUE, [](const Values& v, Ctx&) {
                        std::cout << "VALUE: ";
                        if(!v.empty()) cout << v.begin()->second;
                        cout << std::endl;
                        return true;}},
        //{EXPR_, [](const Values& , Ctx& ) {throw std::logic_error("NEVER CALLED"); return true;}},
        {EXPR, [](const Values& , Ctx&) {std::cout << "EXPR " << std::endl; return true;}},
        {OP, [](const Values& , Ctx&) {std::cout << "OP " << std::endl; return true;}},
        {CP, [](const Values& , Ctx&) {std::cout << "CP " << std::endl; return true;}},
        {PLUS, [](const Values& , Ctx&) {std::cout << "+ " << std::endl; return true;}},
        {MINUS, [](const Values& , Ctx&) {std::cout << "- " << std::endl; return true;}},
        {MUL, [](const Values& , Ctx&) {std::cout << "* " << std::endl; return true;}},
        {DIV, [](const Values& , Ctx&) {std::cout << "/ " << std::endl; return true;}}};
    
    Map g;
    
    auto c = [&g, &am, &ctx](TERM t) { return Call(t, g, am, ctx); };
    
    g[EXPR] = OR(AND(c(EXPR), c(PLUS), c(EXPR)),
                 AND(c(OP), c(EXPR), c(CP)),
                 c(VALUE));
    
    g[VALUE] = MakeTermEval<InStream>(VALUE, g, FloatParser(), am, ctx);
    g[OP]    = MakeTermEval<InStream>(OP, g, ConstStringParser("("), am, ctx);
    g[CP]    = MakeTermEval<InStream>(CP, g, ConstStringParser(")"), am, ctx);
    g[PLUS]  = MakeTermEval<InStream>(PLUS, g, ConstStringParser("+"), am, ctx);
    g[MINUS]  = MakeTermEval<InStream>(MINUS, g, ConstStringParser("-"), am, ctx);
    g[MUL]  = MakeTermEval<InStream>(MUL, g, ConstStringParser("*"), am, ctx);
    g[DIV]  = MakeTermEval<InStream>(DIV, g, ConstStringParser("/"), am, ctx);
    //alternate solution bottom-up
    // g[EXPR_] = MakeTermEval<InStream>(EXPR, g, NonTerminal(), am, ctx);
    // g[EXPR] = OR(g[VALUE],
    //              AND(g[OP], g[EXPR_], g[CP]));
    istringstream is("(2+-3)+1");
    InStream ins(is);

    g[EXPR](ins);
    //EOF is set AFTER TRYING TO READ PAST THE END OF THE STREAM!
    ins.get();
    if(!ins.eof()) std::cout << "ERROR AT: " << ins.tellg() << std::endl;
    assert(ins.eof());
}


int main(int, char**) {
    Go();
   	return 0;
}

//catch recursive call in AND: use counter in closure to check recursive call before even applying first AND clause
//reset counter on exit (wheather true of false)

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
    return [k, &em, p, &am, &c](KeyT prev, InStreamT& is) mutable -> bool {
        assert(em.find(k) != em.end());
        assert(am.find(k) != am.end());
        bool ret = false;
        if(!p.Empty()) {
            ret = p.Parse(is)
                  && am[k](p.GetValues(), c);
        } else {
            ret = em.find(k)->second(prev,is)
                  && am[k](Values(), c);
        }
        return ret;
    };
}

template < typename KeyT > struct EvalFun {
    using Type = std::function< bool (KeyT, InStream&) >;
};


template < typename K >
typename EvalFun< K >::Type OR() {
    return [](K, InStream&) { return false; };
}

template < typename K, typename F, typename...Fs >
typename EvalFun< K >::Type OR(F f, Fs...fs) {
    return [=](K k, InStream& is) {
        bool pass = false;
        pass = f(k, is);
        if(pass) return true;
        return OR<K>(fs...)(k, is);
    };
}

template < typename K >
typename EvalFun< K >::Type AND() {
    return [](K, InStream& is) { return true; };
}

template < typename K, typename F, typename...Fs >
typename EvalFun< K >::Type AND(F f, Fs...fs) {
    return [f, fs...](K k, InStream& is) {
        bool pass = false;
        REWIND r(pass, is);
        pass = f(k, is);
        if(!pass) return false;
        pass = AND<K>(fs...)(k, is);
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


template < typename KeyT, typename EvalMapT, typename ActionMapT, typename ContextT >
typename EvalFun< KeyT >::Type Call(KeyT key, const EvalMapT& em, ActionMapT& am, ContextT& c) {
    KeyT P = KeyT();
    return [P, &em, &am, key](KeyT k, InStream& is) mutable {
        if(P == key) {
            P = KeyT();
            return false;
        }
        P = key;
        assert(em.find(key) != em.end());
        return em.find(key)->second(key, is);
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
using Map = std::map< TERM, typename EvalFun< TERM >::Type >;
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
        {DIV, [](const Values& , Ctx&) {std::cout << "/ " << std::endl; return true;}},
        {EOS, [](const Values& , Ctx&) {std::cout << "EOF " << std::endl; return true;}},
        {T, [](const Values&, Ctx&) {std::cout << "TERM " << std::endl; return true;}}};
    Map g;
    
    auto c = [&g, &am, &ctx](TERM t) { return Call(t, g, am, ctx); };
    
    using K = TERM;
    g[EXPR] = OR<K>(
                    AND<K>(c(EXPR), c(PLUS), c(EXPR)),
                    AND<K>(c(OP), c(EXPR), c(CP)),
                    c(VALUE)
                    );
    
    g[VALUE] = MakeTermEval<InStream>(VALUE, g, FloatParser(), am, ctx);
    g[OP]    = MakeTermEval<InStream>(OP, g, ConstStringParser("("), am, ctx);
    g[CP]    = MakeTermEval<InStream>(CP, g, ConstStringParser(")"), am, ctx);
    g[PLUS]  = MakeTermEval<InStream>(PLUS, g, ConstStringParser("+"), am, ctx);
    g[MINUS]  = MakeTermEval<InStream>(MINUS, g, ConstStringParser("-"), am, ctx);
    g[MUL]  = MakeTermEval<InStream>(MUL, g, ConstStringParser("*"), am, ctx);
    g[DIV]  = MakeTermEval<InStream>(DIV, g, ConstStringParser("/"), am, ctx);
    g[EOS] = MakeTermEval<InStream>(EOS, g, EofParser(), am, ctx);
    
    //alternate solution bottom-up
    // g[EXPR_] = MakeTermEval<InStream>(EXPR, g, NonTerminal(), am, ctx);
    // g[EXPR] = OR(g[VALUE],
    //              AND(g[OP], g[EXPR_], g[CP]));
    istringstream is("(2+-3)+1");
    InStream ins(is);

    g[EXPR](EXPR, ins);
    //EOF is set AFTER TRYING TO READ PAST THE END OF THE STREAM!
    ins.get();
    if(!ins.eof()) std::cout << "ERROR AT: " << ins.tellg() << std::endl;
    assert(ins.eof());
}


int main(int, char**) {
    Go();
   	return 0;
}
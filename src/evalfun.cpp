#include <map>
#include <iostream>
#include <sstream>
#include <functional>
#include <string>
#include <algorithm>

#include <parsers.h>
#include <InStream.h>
#include <types.h>

using namespace parsley;
using namespace std;

using NonTerminal = Parser;

//rewind = rewind stream pointer in case rule doesn't match
//rewind is required for NOT type parsers: if rewind is on
//by default 

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
    return [k, &em, p, &am, &c](InStreamT& is, bool rewind) mutable -> bool {
        assert(em.find(k) != em.end());
        assert(am.find(k) != am.end());
        bool ret = false;
        if(!p.Empty()) {
            ret = p.Parse(is, rewind)
                  && am[k](p.GetValues(), c);
        } else ret = em.find(k)->second(is, rewind) 
                      && am[k](Values(), c);
        return ret;
    };
}

using EvalFun = std::function< bool (InStream&, bool) >;




EvalFun OR() {
  return [](InStream&, bool) { return false; };
}

template < typename F, typename...Fs > 
EvalFun OR(F f, Fs...fs) {
    return [=](InStream& is, bool rewind = true) {
        bool pass = false; 
        REWIND(rewind, is);
        pass = f(is, rewind) || OR(fs...)(is, rewind);
        rewind = pass || !rewind;
        return pass;
    };
}

template< typename F >
EvalFun AND(F f) {
    return [f](InStream& is, bool rewind = true) { return f(is, rewind); };
}

template < typename F, typename...Fs > 
EvalFun AND(F f, Fs...fs) {
    return [=](InStream& is, bool rewind = true) {
        bool pass = false; 
        REWIND(rewind, is);
        pass = f(is, rewind) && AND(fs...)(is, rewind);
        rewind = pass || !rewind;
        return pass;
    };
}

//the following works only if the parsers DO NOT REWIND
template < typename F > 
EvalFun NOT(F f) {
    return [f](InStream& is, bool rewind = true) {
        bool pass = false; 
        REWIND(rewind, is);
        pass = !f(is, false);
        rewind = pass || !rewind;
        return pass;
    };
}

template< typename F >
EvalFun GREEDY(F f) {
    return [f](InStream& is, bool rewind = true) {
        bool r = f(is, rewind);
        while(r) r = f(is, rewind);
        return r;
    }; 
}

template < typename EvalMapT, typename KeyT >
EvalFun Call(const EvalMapT& em, KeyT key) {
    return [&em, key](InStream& is, bool rewind) {
        assert(em.find(key) != em.end());
        return em.find(key)->second(is, rewind);  
    };
}

enum TERM {EXPR, EXPR_, OP, CP, VALUE};

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
using Map = std::map< TERM, std::function< bool (InStream&, bool) > >;
using ActionMap = std::map< TERM, std::function< bool (const Values&, Ctx&) > >;


void Go() {
  Ctx ctx;
  ActionMap am  {{VALUE, [](const Values& v, Ctx& ) {std::cout << "VALUE: " << v.begin()->second << std::endl; return true;}},
                  //{EXPR_, [](const Values& , Ctx& ) {throw std::logic_error("NEVER CALLED"); return true;}},
                  {EXPR, [](const Values& , Ctx& ) {std::cout << "EXPR" << std::endl; return true;}},
                  {OP, [](const Values& , Ctx& ) {std::cout << "OP" << std::endl; return true;}},
                  {CP, [](const Values& , Ctx& ) {std::cout << "CP" << std::endl; return true;}}};
  Map g;
  // g[EXPR] = OR(g[VALUE],
  //              AND(g[OP], g[EXPR], g[CP]));
  auto c = [&g](TERM t) { return Call(g, t); };
  g[EXPR]  = OR(c(VALUE),
                AND(c(OP), c(EXPR), c(CP)));
  g[VALUE] = MakeTermEval<InStream>(VALUE, g, FloatParser(), am, ctx);
  g[OP]    = MakeTermEval<InStream>(OP, g, ConstStringParser("("), am, ctx);
  g[CP]    = MakeTermEval<InStream>(CP, g, ConstStringParser(")"), am, ctx);
  //alternate solution bottom-up
  // g[EXPR_] = MakeTermEval<InStream>(EXPR, g, NonTerminal(), am, ctx);
  // g[EXPR] = OR(g[VALUE], 
  //              AND(g[OP], g[EXPR_], g[CP]));
  istringstream is("((1.2))");
  InStream ins(is);
  g[EXPR](ins, true);
  assert(ins.eof());
}


int main(int, char**) {
  Go();  
	return 0;
}
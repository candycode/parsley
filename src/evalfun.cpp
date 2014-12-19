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
    std::size_t prev = std::numeric_limits< std::size_t >::max(); 
    return [prev, k, &em, p, &am, &c](InStreamT& is, bool fail) mutable -> bool {
        static std::size_t prev = std::numeric_limits< std::size_t >::max();
        //if(prev == is.tellg()) return false;
        prev = is.tellg();
        assert(em.find(k) != em.end());
        assert(am.find(k) != am.end());
        if(fail) {
          am[k](Values(), c, fail);
          return false;
        }
        bool ret = false;
        if(!p.Empty()) {
            ret = p.Parse(is) 
                  && am[k](p.GetValues(), c, fail);
        } else ret = em.find(k)->second(is, fail)  
                      && am[k](Values(), c, fail);

        return ret;
    };
}

using EvalFun = std::function< bool (InStream&, bool) >;

EvalFun OR() {
  return [](InStream&, bool) { return false; };
}

template < typename F, typename...Fs > 
EvalFun OR(F f, Fs...fs) {
    return [=](InStream& is, bool ) {
        bool pass = false; 
        pass = f(is, false);
        if(pass) return true;
        return OR(fs...)(is, false);
    };
}


EvalFun AND() {
    return [](InStream& is, bool) { return true; };
}

template < typename F, typename...Fs > 
EvalFun AND(F f, Fs...fs) {
    return [=](InStream& is, bool ) {
        bool pass = false;
        REWIND r(pass, is);
        pass = f(is, false);
        if(!pass) return false;
        pass = AND(fs...)(is, false);
        if(!pass) f(is, true);
        return pass;
    };
}

//the following works only if the parsers DO NOT REWIND
template < typename F > 
EvalFun NOT(F f) {
    return [f](InStream& is, bool ) {
        Char c = 0;
        StreamPos pos = is.tellg();
        bool pass = false;
        while(is.good() && !f(is)) {
            pass = true;
            c = is.get();
            if(!is.good()) break;
            pos = is.tellg();
        }
        if(is.good()) is.seekg( pos );
        return pass;
    };
}

template< typename F >
EvalFun GREEDY(F f) {
    return [f](InStream& is, bool) {
        bool r = f(is);
        while(r) r = f(is);
        return r;
    }; 
}

template < typename EvalMapT, typename KeyT >
EvalFun Call(const EvalMapT& em, KeyT key) {
    std::size_t prev = std::numeric_limits< std::size_t >::max();
    return [&prev, &em, key](InStream& is, bool) mutable {
        //if(prev == is.tellg()) return false;
        prev = is.tellg();
        assert(em.find(key) != em.end());
        return em.find(key)->second(is, false);
    };
}

enum TERM {EXPR, EXPR_, OP, CP, VALUE, PLUS, MINUS, MUL, DIV, T, EOS};

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
using ActionMap = std::map< TERM, std::function< bool (const Values&, Ctx&, bool) > >;


void Go() {
  Ctx ctx;
  ActionMap am  {{VALUE, [](const Values& v, Ctx&, bool ) {std::cout << "VALUE: " << v.begin()->second << std::endl; return true;}},
                  //{EXPR_, [](const Values& , Ctx& ) {throw std::logic_error("NEVER CALLED"); return true;}},
                  {EXPR, [](const Values& , Ctx&, bool ) {std::cout << "EXPR" << std::endl; return true;}},
                  {OP, [](const Values& , Ctx&, bool ) {std::cout << "OP" << std::endl; return true;}},
                  {CP, [](const Values& , Ctx&, bool ) {std::cout << "CP" << std::endl; return true;}},
                  {PLUS, [](const Values& , Ctx&, bool ) {std::cout << "+" << std::endl; return true;}},
                  {MINUS, [](const Values& , Ctx&, bool ) {std::cout << "-" << std::endl; return true;}},
                  {MUL, [](const Values& , Ctx&, bool ) {std::cout << "*" << std::endl; return true;}},
                  {DIV, [](const Values& , Ctx&, bool ) {std::cout << "/" << std::endl; return true;}},
                  {EOS, [](const Values& , Ctx&, bool ) {std::cout << "EOF" << std::endl; return true;}},
                  {T, [](const Values&, Ctx&, bool ) {std::cout << "TERM" << std::endl; return true;}}};
  Map g;
  // g[EXPR] = OR(g[VALUE],
  //              AND(g[OP], g[EXPR], g[CP]));
  auto c = [&g](TERM t) { return Call(g, t); };
  

  g[EXPR] = OR(
               AND(c(OP), c(EXPR), c(CP)),
               AND(c(T), c(PLUS), c(T)),
               c(VALUE)
               );
  g[T] = OR(c(),
            c(T));
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
  istringstream is("(1+2)+3");
  InStream ins(is);
  g[EXPR](ins, false);
  if(!ins.eof()) std::cout << Char(ins.get()) << std::endl;
  assert(ins.eof());
}


int main(int, char**) {
  Go();  
	return 0;
}
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
        } else ret = em.find(k)->second(is)  
                      && am[k](Values(), c);
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
        {
        bool pass = false; 
        pass = f(is);
        if(pass) return true;
        }
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
        else pass = AND(fs...)(is);
        return pass;
    };
}

//the following works only if the parsers DO NOT REWIND
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

template < typename EvalMapT, typename KeyT >
EvalFun Call(const EvalMapT& em, KeyT key) {
    return [&em, key](InStream& is) {
        assert(em.find(key) != em.end());
        return em.find(key)->second(is);  
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
using ActionMap = std::map< TERM, std::function< bool (const Values&, Ctx&) > >;


void Go() {
  Ctx ctx;
  ActionMap am  {{VALUE, [](const Values& v, Ctx& ) {std::cout << "VALUE: " << v.begin()->second << std::endl; return true;}},
                  //{EXPR_, [](const Values& , Ctx& ) {throw std::logic_error("NEVER CALLED"); return true;}},
                  {EXPR, [](const Values& , Ctx& ) {std::cout << "EXPR" << std::endl; return true;}},
                  {OP, [](const Values& , Ctx& ) {std::cout << "OP" << std::endl; return true;}},
                  {CP, [](const Values& , Ctx& ) {std::cout << "CP" << std::endl; return true;}},
                  {PLUS, [](const Values& , Ctx& ) {std::cout << "+" << std::endl; return true;}},
                  {MINUS, [](const Values& , Ctx& ) {std::cout << "-" << std::endl; return true;}},
                  {MUL, [](const Values& , Ctx& ) {std::cout << "*" << std::endl; return true;}},
                  {DIV, [](const Values& , Ctx& ) {std::cout << "/" << std::endl; return true;}},
                  {EOS, [](const Values& , Ctx& ) {std::cout << "EOF" << std::endl; return true;}},
                  {T, [](const Values&, Ctx& ) {std::cout << "TERM" << std::endl; return true;}}};
  Map g;
  // g[EXPR] = OR(g[VALUE],
  //              AND(g[OP], g[EXPR], g[CP]));
  auto c = [&g](TERM t) { return Call(g, t); };
  

  g[EXPR] = OR(//c(EOS),
               c(VALUE),
               AND(c(OP), c(EXPR), c(CP)),
               AND(c(VALUE), c(PLUS), c(EXPR))
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
  istringstream is("(1+2)");
  InStream ins(is);
  g[EXPR](ins);
  if(!ins.eof()) std::cout << Char(ins.get()) << std::endl;
  assert(ins.eof());
}


int main(int, char**) {
  Go();  
	return 0;
}
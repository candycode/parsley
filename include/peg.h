#include <limits>
#include "InStream.h"
#include "Parser.h"

namespace parsley {

using NonTerminal = Parser;

enum class EvalState {BEGIN, PASS, FAIL};

using EvalFun = std::function< bool (InStream&) >;


///@todo add operator () to parser so that function is can eventually
///be made independent of Parser class and ivoke operator() instead
///of Parse() method
 template < typename InStreamT,
   typename KeyT,
   typename EvalMapT,
   typename ActionMapT,
   typename ContextT >
   typename EvalMapT::value_type::second_type
   MakeTermEval(KeyT k,
                const EvalMapT& em,
                Parser p, //use an empty parser to specify 
                          //non-terminal term
                //.., NonTerminal(),...
                ActionMapT& am,
                ContextT& c,
                bool cback = true) {
     std::size_t sp = std::numeric_limits<std::size_t>::max();
     bool last = false;
     return [cback, last, sp, k, &em, p, &am, &c](InStreamT& is)
       mutable -> bool { //mutable required: sp and last to be
                         //modified by function 
       //~memoization: if evaluation already performed at stream
       //position return result (true or false)
       //at stream position
       if(is.tellg() == sp) return last;
       //make sure key is in map
       assert(em.find(k) != em.end());
       //if callback requested make sure callback available in
       //action map
       if(cback) assert(am.find(k) != am.end());
       bool ret = true;
       //record stream position before parsing
       std::size_t i = is.tellg();
       //Callback: BEGIN
       if(cback) {
         ret = am[k](k, Values(), c, EvalState::BEGIN);
       }
       //If parser not empty actually parse values and 
       //invoke action callback with values
       if(!p.Empty()) { //parser: parse values
         ret = ret && p.Parse(is);
         if(cback) { //parser + callback: parse values then invoke
                     //action callback with parsed values
           const EvalState s = ret ? EvalState::PASS 
                                   : EvalState::FAIL;
           //short circuit evaluation, use am[...] as first arg
           //if not callback not called with FAIL if ret is false
           ret = am[k](k, p.GetValues(), c, s) && ret;
         }
       } else { //no parser: simply invoke function at key = k in
                //eval map
         ret = ret && em.find(k)->second(is);
         if(cback) {
           const EvalState s = ret ? EvalState::PASS 
                                   : EvalState::FAIL;
           //short circuit evaluation, use am[...] as first arg
           //if not callback not called with FAIL if ret is false
           ret = am[k](k, Values(), c, s) && ret;
         }
       }
       //record stream position and result in clojure
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
        //REWIND r(pass, is);
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
EvalFun ZO(F f) {
    return [=](InStream& is) {
      //return true if f fails (zero)
      //or if f succeeds (one) and following
      //call to f fails:
      //if first call f(is) succeeds !f(s) is false
      //and the second term is evaluated: if second term
      //fails then !false == true is returned meaning
      //only first f(is) succeeded
      return !f(is) || !f(is);
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

//Returns a function which forwards calls to other function in grammar
//map
template < typename KeyT, typename EvalMapT, typename ActionMapT,
typename ContextT >
EvalFun Call(KeyT key, const EvalMapT& em, ActionMapT& am, 
             ContextT& c, bool cback) {
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
        if(ret) cb(k, def, c, ret ? EvalState::PASS 
                                  : EvalState::FAIL);
    };
}

template < typename F, typename KeyT, typename MapT, typename Ctx >
F InvokeMapped(const EvalFun& f, KeyT k, const MapT& m, Ctx& c,
               const Values& def = Values()) {
    return [f, k, &m, &c, def](InStream& is) {
        m.find(k)->second(k, def, c, EvalState::BEGIN);
        bool ret = f(is);
        if(ret) m.find(k)->second(k, def, c,
                                  ret ? EvalState::PASS 
                                      : EvalState::FAIL);
    };
}

template < typename M >
void Set(M& m, typename M::value_type::second_type ) {}

template < typename M, typename KeyT, typename...KeysT >
void Set(M& m, 
         typename M::value_type::second_type f, KeyT k, KeysT...ks) {
    m[k] = f;
    Set(m, f, ks...);
}

}

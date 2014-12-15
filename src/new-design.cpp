


///@todo add context type everywhere

namespace parsley {
template < typename MapT >
bool Exists(typename MapT::value_type::first_type key,
		    const MapT& m) {
	return m.find(key) != m.end();
}

template < typename InStreamT,
           typename EvalMapT,
           typename ParserMapT,
           typename ActionMapT >
struct ParsingTypes {
	using InStream = InStreamT;
    using Key = typename EvalMapT::value_type::first_type;
    using EvalFun = typename EvalMapT::value_type::second_type;
    using Parser = typename ParserMapT::value_type::second_type;
    using Action = typename ActionMapT::value_type::second_type;
    using EvalMap = EvalMapT;
    using ParserMap = ParserMapT;
    using ActionMap = ActionMapT;
    
};        


template < typename InStreamT,
           typename EvalMapT,
           typename ParserMapT,
           typename ActionMapT,
           template < template < typename > class F > 
struct EvalFunctionType {
    using ReturnType = bool;
    using Key = ParsingTypes< InStreamT, EvalMapT, ParserMapT,
                              ActionMapT >::Key;                          
    using Type = F< ReturnType (Key, InStreamT&
                                EvalMapT&, ParserMapT&, ActionMapT&);

};

//eval function
template < typename KeyT,
           typename InStreamT,
           typename EvalMapT,
           typename ParserMapT,
           typename ActionMapT >
bool ParseTerminal(
            InStreamT& is,
            EvalMapT& em,
            ParserMapT& pm,
            ActionMapT& am) {
	return pm[id].Parse(is) && am[id](id, pm.Parse().GetValues());
}
template < typename KeyT,
           typename InStreamT,
           typename EvalMapT,
           typename ParserMapT,
           typename ActionMapT >
bool ParseNonTerminal(KeyT id,
            InStreamT& is,
            EvalMapT& em,
            ParserMapT& pm,
            ActionMapT& am) {
	return Eval(id, is, em, pm, am) && am[id](id, pm.Parse().GetValues());
}


//generate simple eval function
template < typename KeyT,
           typename InStreamT,
           typename EvalMapT,
           typename ParserMapT,
           typename ActionMapT >
ParsingTypes< InStreamT, EvalMapT, ParserMapT, ActionMapT >
::EvalFunType
GenerateEvalTerm(KeyT id) {
	return [id](InStreamT& is,
				EvalMapT& em,
				ParseMapT& pm,
				ActionMapT& am) {
		///@todo remove Exists; replace with bool
		///unordered map: key = id, value = true if
		///term has parser, false if not
		return Exists(id, pm) ? 
			   ParseTerminal(id, is, em, pm, am)
			   : ParseNonTerminal(id, em, pm, am);
	}
}

//combinators
//generate simple eval function
template < typename InStreamT,
           typename EvalMapT,
           typename ParserMapT,
           typename ActionMapT >
ParsingTypes< InStreamT, EvalMapT, ParserMapT, ActionMapT >
::EvalFunType
OR(ParsingTypes< InStreamT, EvalMapT, ParserMapT, ActionMapT >
   ::EvalFunType f1, 
   ParsingTypes< InStreamT, EvalMapT, ParserMapT, ActionMapT >
   ::EvalFunType f2) {
    return [=](InStreamT& is,
               EvalMapT& em,
               ParserMapT& pm,
               ActionMapT& am) {
        return f1(is, em, pm, am) || f2(is, em, pm, am);
    };
}


template < typename InStreamT,
           typename EvalMapT,
           typename ParserMapT,
           typename ActionMapT >
ParsingTypes< InStreamT, EvalMapT, ParserMapT, ActionMapT >
::EvalFunType
AND(ParsingTypes< InStreamT, EvalMapT, ParserMapT, ActionMapT >
   ::EvalFunType f1, 
   ParsingTypes< InStreamT, EvalMapT, ParserMapT, ActionMapT >
   ::EvalFunType f2) {
    return [=](InStreamT& is,
               EvalMapT& em,
               ParserMapT& pm,
               ActionMapT& am) {
        return f1(is, em, pm, am) && f2(is, em, pm, am);
    };
}

template < typename InStreamT,
           typename EvalMapT,
           typename ParserMapT,
           typename ActionMapT >
ParsingTypes< InStreamT, EvalMapT, ParserMapT, ActionMapT >
::EvalFunType
NOT(ParsingTypes< InStreamT, EvalMapT, ParserMapT, ActionMapT >
   ::EvalFunType f) {
    return [=](InStreamT& is,
               EvalMapT& em,
               ParserMapT& pm,
               ActionMapT& am) {
        return !f(is, em, pm, am);
    };
}

template < typename KeyT,
           typename InStreamT,
           typename EvalMapT,
           typename ParserMapT,
           typename ActionMapT >
ParsingTypes< InStreamT, EvalMapT, ParserMapT, ActionMapT >
::EvalFunType
GREEDY(KeyT key) {
    return [=](InStreamT& is,
               EvalMapT& em,
               ParserMapT& pm,
               ActionMapT& am) {
        int count = 0;
        while(em[key](is, em, pm, am)) ++count;
        return count > 0 && am[key(id, Values()); 
    };
}


template < typename T, typename R, typename F >
std::function< bool (T, T) > Compose(R r, F f) {
    return [=](T v1, T v2) {
        return f(v1, v2);
    };
}

template < typename InStreamT,
           typename EvalMapT,
           typename ParserMapT,
           typename ActionMapT,
           typename C,
           typename F,
           typename... Fs >
ParsingTypes< InStreamT, EvalMapT, ParserMapT, ActionMapT >
::EvalFunType
 Compose(C c F f, Fs...fs) {
    return [](InStreamT& is,
              EvalMapT& em,
              ParserMapT&  pm,
              ActionMapT& am) {
        return c(f(is, em, pm, am), 
                 Compose< InStreamT, EvalMapT, ParserMapT,
                  ActionMapT, C >(c, fs...))(is, em, pm, am);
    };
}

struct GrammarTypes {
    using EvalFun = ParsingTypes< InStreamT, EvalMapT, ParserMapT, ActionMapT >::EvalFun;
    using Key = ParsingTypes< InStreamT, EvalMapT, ParserMapT, ActionMapT >::Key;
    static EvalFun GenOR(EvalFun f1, EvalFun f2) {
        return GenerateOrTerm< InStream, EvalMap, ParserMap, ActionMap >(f1, f2);
    }   
    static EvalFunType EvalTerm(Key id) {
        return GenerateEvalTerm< InStream, EvalMap, ParserMap, ActionMap >(id);
    }
    template < typename...ArgsT >
    static EvalFunType EvalOR(ArgsT...args) {
        return Compose< ....>(GenOR, args...);
        
    } 
    static EvalFunType EvalAND(EvalFunType f1, EvalFunType f2) {
    template < typename...ArgsT >
    static EvalFunType EvalOR(ArgsT...args) {
        return Compose< ....>(GenAnd, args...);
        
    } 

};

GrammarTypes< InStream, EvalMap, ParserMap, ActionMap > {

};
grammar[EXPR] = OR(T(VALUE),
                   AND(T(POPEN),T(EXPR), T(PCLOSE)));
grammar[VALUE] = T(VALUE)
grammar[POPEN] = T(POPEN)
grammar[PCLOSE] = T(PCLOSE)
return grammar[EXPR](instream, grammar, parserMap, actionMap, context);m



}




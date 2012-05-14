typedef AlphaNumParser A; // "12abc"
typedef FirstAlphaNumParser FA; //"A123"
typedef UIntParser U; // "123"
typedef FloatParser F; // "12.2E03"
typedef String S; // "abc"
typedef ConstStringParser C; // only returns true if the string is equal to a constant value
typedef NotParser<ConstStringParser> NC; // parses expressions NOT equal to const value
typedef NotParser< AndParser > NAL; // parses expressions NOT equal to a sequence of expressions
typedef AndParser AL; // 'AND' Lexer: true iff all sub-expressions are parsed correctly 
typedef OrParser OL;  // 'OR'  Lexer: true if any of the sub-expression is parsed correctly
typedef TupleParser<-1> T; // parse sequence
typedef TupleParser<3> T3; // parse sequence of 3 elements
typedef TupleParser<2> T2; // parse sequence of 2 elements

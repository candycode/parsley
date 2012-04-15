cmake_minimum_required(VERSION 2.4)

PROJECT( parsley )

SET( SCRATCH_SRCS cppscratch.cpp )

SET( PARSER_TEST_SRCS parsertest.cpp
     Any.h
     InStream.h
     IStateController.h
     Parser.h
     ParserManager.h
     SmartPtr.h
     types.h
     StateManager.h
     parsers.h
     parser_operators.h
     doxymain.h
   )



ADD_EXECUTABLE( scratch ${SCRATCH_SRCS} )

ADD_ExECUTABLE( parsertest ${PARSER_TEST_SRCS} )
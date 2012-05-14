cmake_minimum_required(VERSION 2.6)

project( parsley )

set( IDIR include )

set( INCLUDES
     ${IDIR}/Any.h
     ${IDIR}/InStream.h
     ${IDIR}/IStateController.h
     ${IDIR}/Parser.h
     ${IDIR}/ParserManager.h
     ${IDIR}/SmartPtr.h
     ${IDIR}/types.h
     ${IDIR}/StateManager.h
     ${IDIR}/parsers.h
     ${IDIR}/parser_operators.h
     ${IDIR}/doxymain.h
   )

include_directories( include )
add_executable( basicparsers src/test/basicparsers.cpp ${INCLUDES} )
add_executable( compositeparsers src/test/compositeparsers.cpp ${INCLUDES} )
add_executable( exprprarser src/test/exprparser.cpp ${INCLUDES} )

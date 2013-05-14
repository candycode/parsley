#pragma once

#include "types.h"

namespace parsley {

template < typename S >
struct StateInfo {
  StateInfo() : empty( true ) {}
  StateInfo( S sid ) : src( sid ), empty( false ) {}
  StateInfo( S s1, S s2 ) : src( s1 ), empty( false ) {
    targets.push_back( s2 );
  }
  StateInfo( const StateInfo& s1,
             const StateInfo& s2,
             const StateInfo& s3 = StateInfo(),
             const StateInfo& s4 = StateInfo(),
             const StateInfo& s5 = StateInfo(),
             const StateInfo& s6 = StateInfo(),
             const StateInfo& s7 = StateInfo(),
             const StateInfo& s8 = StateInfo(),
             const StateInfo& s9 = StateInfo(),
             const StateInfo& s10 = StateInfo() ) : src( s1.src ) {
    empty =false;
    src = s1.src;
    targets.push_back( s2 );
    if( s3.empty ) return;
    targets.push_back( s3 );
    if( s4.empty ) return;
    targets.push_back( s4 );
    if( s5.empty ) return;
    targets.push_back( s5 );
    if( s6.empty ) return;
    targets.push_back( s6 );
    if( s7.empty ) return;
    targets.push_back( s7 );
    if( s8.empty ) return;
    targets.push_back( s8 );
    if( s9.empty ) return;
    targets.push_back( s9 );
    if( s10.empty ) return;
    targets.push_back( s10 );
  }
  S src;
  std::vector< StateInfo< S > > targets;
  bool empty;
};
} // namespace 
#ifdef PARSLEY_DECL_STATEINFO_OPERATORS
#error "Macro PARSLEY_DECL_STATEINFO_OPERATORS already defined"
#endif
#define PARSLEY_DECL_STATEINFO_OPERATORS( SID ) \
parsley::StateInfo< SID > operator,( SID s1, SID s2 ) { \
  return parsley::StateInfo< SID >( s1, s2 ); \
} \
parsley::StateInfo< SID > operator,(SID sid, \
                                    const parsley::StateInfo< SID >& si ) { \
  return parsley::StateInfo< SID >( sid, si ); \
} \
parsley::StateInfo< SID > operator,( const parsley::StateInfo< SID >& si1, \
                                     const parsley::StateInfo< SID >& si2 ) { \
  parsley::StateInfo< SID > si( si1 ); \
  si.targets.push_back( si2 ); \
  return si; \
} \
parsley::StateInfo< SID > operator( const parsley::StateInfo< SID >& si, \
                                    SID sid ) { \
  parsley::StateInfo< SID > sir( si ); \
  sir.targets.push_back( sid ); \
  return sir; \
}

    // StateInfo si =
    // (START, 
    //   (MOLDEN_FORMAT,
    //     (START_SKIP_LINE,
    //        MOLDEN_FORMAT,
    //        START),
    //     (TITLE,
    //       (TITLE_DATA,
    //         ATOMS),
    //       (ATOMS,
    //         (ATOM_DATA,
    //           ATOM_DATA,
    //           (GTO,
    //             (GTO_ATOM_NUM
    //               (GTO_SHELL_INFO,
    //                 (GTO_COEFF_EXP,
    //                   GTO_SHELL_INFO,
    //                   GTO_COEFF_EXP,
    //                   GTO_ATOM_NUM,
    //                   EOF_,
    //                   EOL_))))))))); 
    

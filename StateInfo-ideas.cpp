struct StateInfo {
  StateInfo() : empty( true ) {}
  StateInfo( StateID sid ) : src( sid ) {
    empty = false;
  }
  StateInfo( const StateInfo& s1,
             const StateInfo& s2,
             const StateInfo& s3 = StateInfo() ) : src( s1.src ) {
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
  StateID src;
  std::vector< StateInfo > targets;

  void SetStates( StateMap& sm, Queue q = Queue() ) {
    for( std::vector< StateInfo >::iterator i = targets.begin(),
         i != targets.end();
         ++i ) {
        sm[ src ].push_back( i->src );
    }
    
  }


};


typedef std::queue< StateInfo* > StateInfoQueue;

void UpdataStateMapImpl( StateMap& sm, StateInfoQueue& q) {
  if( q.empty() ) return;
  StateInfo* si = q.front();
  q.pop();
  for( std::vector< StateInfo >::iterator i = si->targets.begin(),
         i != si->targets.end();
         ++i ) {
        sm[ si->src ].push_back( (*i)->src );
        q.push( &(*i) ); 
  }
  UpdateStateMapImpl( sm, q );
}
void UpdateStates( StateMap& sm ) {
  StateInfoQueue q;
  q.push( &sm );
  UpdateStateMapImpl( sm, q );

}


StateInfo operator( StateID s1, StateID s2 ) {
  return StateInfo( s1, s2 );
}
StateInfo operator,(StateID sid, const StateInfo& si ) {
  return StateInfo( sid, si );
}
StateInfo operator,( const StateInfo& si1, const StateInfo& si2 ) {
  StateInfo si( si1 );
  si.targets.push_back( si2.src );
  std::back_insert_iterator< std::vector< StateInfo > > bi(si.targets);
  std::copy( si2.targets.begin(), si2.targets.end(), bi );
  return si;
}
StateInfo operator( const StateInfo& si, StateID sid ) {
  StateInfo sir( si );
  si.targets.push_back( sid );
}


    StateInfo si =
    (START, 
      (MOLDEN_FORMAT,
        (START_SKIP_LINE,
           MOLDEN_FORMAT,
           START),
        (TITLE,
          (TITLE_DATA,
            ATOMS),
          (ATOMS,
            (ATOM_DATA,
              ATOM_DATA,
              (GTO,
                (GTO_ATOM_NUM
                  (GTO_SHELL_INFO,
                    (GTO_COEFF_EXP,
                      GTO_SHELL_INFO,
                      GTO_COEFF_EXP,
                      GTO_ATOM_NUM,
                      EOF_,
                      EOL_))))))))); 
    

              
const bool CLEAR_STATES_OPTION = true;               
parserManager.SetStates( si, CLEAR_STATES_OPTION );

SetStates method implements the UpdateStates functions.
/*
 * Copyright (C) 2024 Commissariat à l'énergie atomique et aux énergies alternatives (CEA)

 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at

 *    http://www.apache.org/licenses/LICENSE-2.0 

 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
*/

#include "quantum.hpp"

namespace vpsim {


ParallelQuantumKeeper::ParallelQuantumKeeper( unsigned int quantum):
	forceSyncCount(0),
	syncCount(0)
{
	cout<<"Setting global quantum to "<<sc_time(quantum, SC_NS)<<endl;
	set_global_quantum( sc_time(quantum, SC_NS) );
}

ParallelQuantumKeeper::ParallelQuantumKeeper(){
}

ParallelQuantumKeeper::~ParallelQuantumKeeper(){
}

//#define ENABLE_PARALLEL_QUANTUM

//!Synchronisation with systemc time using regular quantum steps (multiples of quantum)
//!to be used by default
void ParallelQuantumKeeper::sync(){
#ifdef ENABLE_PARALLEL_QUANTUM
	while(need_sync()){ // local time is higher than the remaining quantum
		sc_time q =  m_next_sync_point-sc_core::sc_time_stamp();
		//sc_time q = compute_local_quantum();(identical to previous line but less efficient)

		//cout << endl << "quantum.cpp: ParallelQuantumKeeper: sync: wait: q =" << q << endl;

		sc_core::wait(q); //we wait for the remaining quantum to align on specific time points (multiples of quantum)

		//we reset ParallelQuantumKeeper, i.e we compute the new quantum, and we update the local time
	    m_local_time -= q;
	    m_next_sync_point = sc_core::sc_time_stamp() + compute_local_quantum();

	    //update stats
	    syncCount++;
	}
#else
	if (need_sync()) forceSync();
#endif

}

//! forced synchronisation at time stamps unaligned with quantum
//! to be used when absolutely necessary for synchronization purposes (adds some synchronization points)
void ParallelQuantumKeeper::forceSync(){

	//cout << endl << "quantum.cpp: ParallelQuantumKeeper: ForceSync: wait: m_local_time =" << m_local_time << endl;

	sc_core::wait(m_local_time);


  reset();
}

//! convenience proxy to clarify the set function defined by tlm_quantumkeeper
//! and be more consistent with existing tlm_quantumkeeper::get_local_time
void ParallelQuantumKeeper::set_local_time(const sc_core::sc_time& t)
{
  this->set(t);
}

ParallelQuantumKeeper& ParallelQuantumKeeper::operator+=( sc_time const t){
	this->inc(t);
	return *this;
}

uint32_t ParallelQuantumKeeper::getSyncCount(){ return syncCount;};
uint32_t ParallelQuantumKeeper::getForceSyncCount(){ return forceSyncCount;};
uint32_t ParallelQuantumKeeper::getTotalSyncCount(){ return syncCount+forceSyncCount;};

}//end namespace vpsim

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

#ifndef QUANTUM_HPP_
#define QUANTUM_HPP_

#include "global.hpp"

namespace vpsim
{
	//! ParallelQuantumKeeper leverages tlm_utils::tlm_quantumkeeper to provide synchronous time events between
	//! LT initiators. It makes sure that synchronization occur on specific time stamps and do not induce initiators
    //! to run at different time events (though committing waits every quantum in average)
	class ParallelQuantumKeeper : public tlm_utils::tlm_quantumkeeper
	{
	private:
		//stats
		uint32_t forceSyncCount;
		uint32_t syncCount;

	public:
		//!Constructor
		ParallelQuantumKeeper();
		ParallelQuantumKeeper(unsigned int quantum);

		//!Destructor
		virtual ~ParallelQuantumKeeper();


		//! Synchronization with systemc time using regular quantum steps (multiples of quantum)
		//! to be used by default
		virtual void sync();

		sc_time getNextSyncPoint() { return m_next_sync_point; }

		//! forced synchronization at time stamps unaligned with quantum
		//! to be used when absolutely necessary for synchronization purposes (adds some synchronization poitns)
		void forceSync();

		//! convenience proxy to clarify the set function defined by tlm_quantumkeeper
		//! and be more consistent with existing tlm_quantumkeeper::get_local_time
		void set_local_time(const sc_core::sc_time& t);

		ParallelQuantumKeeper& operator+=( sc_time const t);

		uint32_t getSyncCount();
		uint32_t getForceSyncCount();
		uint32_t getTotalSyncCount();
	};
}

#endif /* QUANTUM_HPP_ */


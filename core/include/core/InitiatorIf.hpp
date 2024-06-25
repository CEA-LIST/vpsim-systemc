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

#ifndef INITIATOR_IF_HPP_
#define INITIATOR_IF_HPP_

#include "global.hpp"
#include "logger.hpp"

namespace vpsim
{
	class InitiatorIf : public tlm::tlm_bw_transport_if<>, public Logger
	{
	private:
		//Other local variables
		std::vector < tlm::tlm_dmi > mCachedDmiRegions;
		bool mDmiEnable;
		string mName;
		DIAG_LEVEL mDiagnosticLevel;
		bool mForceLt;
		bool mTlmActive;
		uint32_t mNbPort;

	public:
		//---------------------------------------------------
		//Constructor
		InitiatorIf ( string Name, uint32_t NbPort );
		InitiatorIf ( string Name, unsigned int Quantum, uint32_t NbPort );
		InitiatorIf ( string Name, unsigned int Quantum, bool Active, uint32_t NbPort );

		//Destructor
		~InitiatorIf ();

		//---------------------------------------------------
		//Other functions
		void tlm_error_checking ( const tlm::tlm_response_status status );

		tlm::tlm_response_status target_mem_access ( uint32_t port, uint64_t addr, uint32_t length, unsigned char * data, ACCESS_TYPE rw, sc_time &delay, uint32_t id=0 );

		uint32_t target_dbg_access ( uint32_t port, uint64_t addr, uint32_t length, unsigned char * data, ACCESS_TYPE rw );

		//---------------------------------------------------
		//Set functions
		void setDiagnosticLevel(DIAG_LEVEL DiagnosticLevel);
		void setForceLt(bool ForceLt);
		void setQuantumKeeper(tlm_utils::tlm_quantumkeeper QuantumKeeper);
		void setDmiEnable(bool DmiEnable);

		//Get functions
		DIAG_LEVEL getDiagnosticLevel();
		bool getForceLt();
		uint32_t getNbPort();
		bool getTlmActive();
		bool getDmiEnable();

		tlm::tlm_generic_payload trans;

		//!
		//! @return InitiatorIf module mName
		//!
		string getName();

		tlm_utils::simple_initiator_socket<InitiatorIf> ** getInitiatorSocket();

		//---------------------------------------------------
		//Communication interface
		tlm_utils::simple_initiator_socket<InitiatorIf> ** mInitiatorSocket;

		tlm::tlm_sync_enum nb_transport_bw ( tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t );

		void invalidate_direct_mem_ptr ( sc_dt::uint64 start, sc_dt::uint64 end );
	};

	/* Explanation: Multiprocessor GIC implementations require that CPUs be connected to different CPU interfaces.
	 * These interfaces have the same Address with respect to the CPU, read/writes to the interface identify the accessing CPU.
	 * Therefore, we need a way to tell the GIC which CPU is performing the write, to avoid mixing up registers connected to
	 *  different CPUs.
	 */
	struct GicCpuExtension : public tlm::tlm_extension<GicCpuExtension> {
		uint32_t cpu_id;

		virtual tlm::tlm_extension_base* clone() const {
			GicCpuExtension* copy = new GicCpuExtension;
			*copy=*this;
			return copy;
		}
		virtual void copy_from(tlm::tlm_extension_base const &ext) {
			*this=dynamic_cast<GicCpuExtension const &>(ext);
		}

	};
}

#endif /* INITIATOR_HPP_ */

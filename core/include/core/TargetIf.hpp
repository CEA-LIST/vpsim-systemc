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

#ifndef _TargetIf_HPP_
#define _TargetIf_HPP_

#include "LatencyIf.hpp"
#include "AddrSpace.hpp"
#include "logger.hpp"

namespace vpsim
{

	//!
	//!	the TargetIf<TYPE> class provides the necessary interfaces that any TLM Slave in VPSim shall provide
	//! in order to implement all TLM 2.0 accesses types
	//! it also provides default implementation for most generalizable interfaces
	//! @tparam TYPE the type of the memory allocd array that can be accessed by DMI
	//!
	template < typename TYPE>
	class TargetIf : public LatencyIf, public AddrSpace, public tlm::tlm_fw_transport_if<>, public Logger	{
	protected:

		string mName; //!< the name of the TargetIf instance to be used for debugging purposes
		bool mByteEnable; //!< a boolean flag to inform that byte level accesses are supported by the target
		bool mDmiEnable; //!< a boolean flag to inform that DMI accesses are supported by the target

		Callback_t * mReadCallback; //!< callback function pointer to a TLM read function implemented by inheriting slave
		Callback_t * mWriteCallback; //!< callback function pointer to a TLM write function implemented by inheriting slave

		TYPE * mLocalMem;//!< Internal shared memory space

		//------------
		//Statistics
		//------------

		uint64_t mReadCount; //!< statistic counter for the number of read accesses to the TLM slave
		uint64_t mWriteCount; //!< statistic counter for the number of write accesses to the TLM slave


	public:
		using REG_T = TYPE;
        static constexpr auto REG_SIZE = sizeof(REG_T);

		uint64_t getReadCount() { return mReadCount; }
		uint64_t getWriteCount() { return mWriteCount; }
		//---------------------------------------------------
		//Constructors
		//---------------------------------------------------

		//!
		//! constructor that disables the support of DMI communications
		//! @param [in] name : a unique string to represent the TargetIf instance in standard output
		//! @param [in] size : the size of the memory address range that this TargetIf will encompass
		//!
		TargetIf ( string Name, uint64_t Size );

		//!
		//! constructor that allows to enable the support of DMI communications
		//! @param [in] name : a unique string to represent the TargetIf instance in standard output
		//! @param [in] size : the size of the memory address range that this TargetIf will encompass
		//! @param [in] byte_enable : activates byte masked communication if set to true
		//! @param [in] dmi_enable : activates the support of DMI accesses if set to true
		//!
		TargetIf ( string Name, uint64_t Size, bool ByteEnable, bool DmiEnable );

		//!
		//! destructor
		//!
		~TargetIf ();


		//---------------------------------------------------
		//Set functions
		//---------------------------------------------------

		//!
		//! sets the value of mByteEnable to that of val
		//! @param [in] val
		//!
		void setByteEnable ( bool val );

		//!
		//! sets the value of mDmiEnable to that of val
		//! @param [in] val
		//!
		void setDmiEnable ( bool val );

		//!
		//! sets the value of mDiagnosticLevel to that of val
		//! @param [in] val
		//!
		void setDiagnosticLevel ( DIAG_LEVEL val );

		//---------------------------------------------------
		//Get functions
		//---------------------------------------------------

		//!
		//! @return the value of mByteEnable
		//!
		bool getByteEnable ( );

		//!
		//! @return the value of mDmiEnable
		//!
		bool getDmiEnable ( );

		//!
		//! @return the value of mDiagnosticLevel
		//!
		DIAG_LEVEL getDiagnosticLevel ( );

		//!
		//! @return the value of mBaseAddress
		//!
		//uint64_t getBaseAddress ( );

		//!
		//! @return TargetIf module mName
		//!
		string getName ( );

		//!
		//! @return TargetIf module mLocalMem
		//!
		TYPE * getLocalMem ();

		//!
		//! @return TargetIf module mTargetSocket
		//!
		tlm_utils::simple_target_socket<TargetIf<TYPE>> * getTargetSocket();


		//Statistics

		//!
		//! prints the instance statistics to the statistics output file
		//!
		void PrintStatistics ( );


		//---------------------------------------------------
		//Helper functions

		//!
		//! Sets the TargetIf local_mem memory to point to mem.
		//! throws if the local_mem is already allocated
		//! @param [in] mem : a  pointer to an array of TYPE
		//!
		void RegisterMemorySpace ( TYPE * mem );


		//!
		//! Registers a read function to be used for every TLM read access except DMI (i.e. b_transport, nb_transport_fw or transport_dbg)
		//! @note This scheme is intended to allow dynamic switching of read implementation to support dynamic variable precision.
		//! TODO this feature was not tested yet
		//! @param [in] callback : a function pointer that implements the read of the TargetIf memory
		//!
		void RegisterReadAccess ( Callback_t * callback );

		//!
		//! Registers a write function to be used for every TLM read access except DMI (i.e. b_transport, nb_transport_fw or transport_dbg)
		//! @note This scheme is intended to allow dynamic switching of write implementation to support dynamic variable precision.
		//! TODO this feature was not tested yet
		//! @param [in] callback : a function pointer that implements the write of the TargetIf memory
		//!
		//void register_write_access ( sc_module * mod, tlm::tlm_response_status (*ptr) ( payload_t&, sc_core::sc_time& ) );
		void RegisterWriteAccess ( Callback_t * callback );


		//---------------------------------------------------
		//core tlm function
		//---------------------------------------------------

		//!
		//! Reference implementation for non-blocking TLM communication thread implementation.
		//! To be used or overidden by classes inheriting from TargetIf
		//! @note the implementation relies on read_access_function and write_access_function calls
		//! @param [in,out] payload : a VPSim simplified TLM packet
		//! @param [in,out] delay : the accumulated delay vs the current simulation time
		//!
		tlm::tlm_response_status CoreFunction ( payload_t &payload, sc_time& delay );

		//!
		//! Wrapper function that performs the actual read access of the TargetIf by forwarding it to
		//! registered read callback function pointed to by m_read_callback.
		//! @param [in,out] payload : a VPSim simplified TLM packet
		//! @param [in,out] delay : the accumulated delay vs the current simulation time
		//!
		tlm::tlm_response_status ReadAccessFunction ( payload_t &payload, sc_time& delay );

		//!
		//! Wrapper function that performs the actual write access of the TargetIf by forwarding it to
		//! registered write callback function pointed to by m_write_callback.
		//! @param [in,out] payload : a VPSim simplified TLM packet
		//! @param [in,out] delay : the accumulated delay vs the current simulation time
		//!
		tlm::tlm_response_status WriteAccessFunction ( payload_t &payload, sc_time& delay );

		//---------------------------------------------------
		//Communication interface
		//---------------------------------------------------

		tlm_utils::simple_target_socket<TargetIf> mTargetSocket; //!< a standard TLM socket that is linked to the TargetIf TLM functions and allows to easily bind to the TargetIf

		//!
		//! Provides reference implementation for TLM 2.0 standard blocking transport interface
		//! @param [in,out] trans : a TLM packet using the tlm_generic_payload structure provided by TLM standard implementation
		//! @param [in,out] delay : the accumulated delay vs the current simulation time accumulated throughout TLM blocking calls
		//!
		void b_transport( tlm::tlm_generic_payload& trans, sc_time& delay );

		//!
		//! Targets to provide reference implementation for TLM 2.0 standard non-blocking forward transport interface
		//! implementation is minimal : do not use
		//! @param [in,out] trans : a TLM packet using the tlm_generic_payload structure provided by TLM standard implementation
		//! @param [in,out] phase : the current phase of the communication
		//! @param [in,out] t : ???? the accumulated delay vs the current simulation time accumulated throughout TLM blocking calls
		//! @return : a TLM standard return status containing the result state of the forward communication
		//!
		tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t);

		//!
		//! Provides reference implementation for TLM 2.0 standard Direct Memory Interface (DMI) interface
		//! @param [in,out] trans : a TLM packet using the tlm_generic_payload structure provided by TLM standard implementation and containing address space that one want to get DMI access to
		//! @param [out] dmi_data : an allocated dmi structure used to return information on what DMI features the TargetIf supports for the asked memory space
		//!
		bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data);

		//!
		//! Provides reference implementation for TLM 2.0 standard debug interface (debug interface is used to read/write target memory without any side effect such as time update)
		//! @param [in,out] trans : a TLM packet using the tlm_generic_payload structure provided by TLM standard implementation and containing address space that one want to get DMI access to
		//! @return : the size of the read data, 0 if failure
		//!
		unsigned int transport_dbg(tlm::tlm_generic_payload& trans);
	};

}

#endif /* _TargetIf_HPP_ */

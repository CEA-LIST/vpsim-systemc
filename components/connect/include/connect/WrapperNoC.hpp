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

#ifndef WRAPPERNOC_HPP
#define WRAPPERNOC_HPP

#include "systemc.h"
//#include "ac_tlm_protocol.H"
#include "NoCBasicTypes.hpp"

#include <algorithm>

namespace vpsim
{

//this wrapper updates the id field in incoming ac_tlm_req so that they match the router ID
class C_BasicWrapperMasterNoC: public sc_module, public ac_tlm_transport_if
{
		unsigned int ID;

	public: 
		sc_export<ac_tlm_transport_if> MasterIn;
		sc_port<ac_tlm_transport_if> MasterOut;

		C_BasicWrapperMasterNoC(sc_module_name name, unsigned int _ID);

		//ac_tlm_rsp transport(ac_tlm_req const &  req);
		void b_transport( tlm::tlm_generic_payload& trans, sc_time& delay );
};



class C_WrapperMasterNoCToFifo: public sc_module, public ac_tlm_transport_if
{
		unsigned int ID;

		//NoC speed parameters
		float FrequencyScaling;
		//sc_time NoCCycle;
		unsigned int LinkSizeInBytes;
		 unsigned int volatile ParallelAccessCount;

		T_MemoryMap* MemMap;

		//to find the Target_ID for a given address
		T_TargetID GetTargetIDFromAddress(T_MemoryAddress MemoryAddress);


		//we use the request address to address the response address
		// the response address is null as long as no response has been received,
		// i.e it is the response status
		std::map<tlm::tlm_generic_payload*,bool> ResponseReceived;
		std::map<tlm::tlm_generic_payload*,bool> DEBUG_CurRequests;

	public:
		sc_in_clk clk;
		sc_export<ac_tlm_transport_if> MasterIn;
		sc_fifo_out<NoCFlit> FifoOut; //to send flits on the NoC
		sc_fifo_in<NoCFlit> FifoIn; //to receive response flits

		C_WrapperMasterNoCToFifo(sc_module_name name, unsigned int _ID, unsigned int _LinkSizeInBytes, float _FrequencyScaling, bool NoTiming);

		SC_HAS_PROCESS(C_WrapperMasterNoCToFifo);

		void SetMemoryMap(T_MemoryMap* _MemMap);

		//the transport interface that creates NoCFlit messages and send them to the wrapper Fifo
		//then waits for a response handled by the RouteBW thread
		//ac_tlm_rsp transport(ac_tlm_req const &  req);
		void b_transport( tlm::tlm_generic_payload& trans, sc_time& delay );

		//thread handling response Flits for requests sent by the transport IF
		void RouteBW();

};

class C_WrapperSlaveFifoToNoC: public sc_module
{
		unsigned int ID;

		//NoC speed parameters
		float FrequencyScaling;
		//sc_time NoCCycle;
		unsigned int LinkSizeInBytes;

	public:
		sc_in_clk clk;
		//std::map<T_PortID, sc_port<ac_tlm_transport_if>*> SlaveOuts;
		sc_port<ac_tlm_transport_if> SlaveOut;

		sc_fifo_out<NoCFlit> FifoOut; //to send response flits on the NoC
		sc_fifo_in<NoCFlit> FifoIn; //to receive request flits on the NoC

		//Constructor
		C_WrapperSlaveFifoToNoC(sc_module_name name, unsigned int _ID, unsigned int _LinkSizeInBytes, float _FrequencyScaling, bool NoTiming);

		SC_HAS_PROCESS(C_WrapperSlaveFifoToNoC);

		//binding function that instantiates as many slave ports as required
		void bind(sc_export<ac_tlm_transport_if> & SlavePort);

		//The slave wrapper thread that deals with input NoCFlit to send a TLM request to slave
		//and re-emmit Backward NoCFlit response
		void RouteFW();
};

};//namespace vpsim

#endif

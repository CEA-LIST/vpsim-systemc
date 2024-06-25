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

#ifndef NOCIF_HPP
#define NOCIF_HPP

#include "NoCBasicTypes.hpp"

namespace vpsim{

//!
//! The C_NoCIF is a pure virtual class that defines the interface necessary
//! to describe a NoC topology as well as its binding to slaves and masters
//! on the network
//! Implementation of these function is dependent on the level of abstraction
//! of the generated mesh.
//!
class C_NoCIF
{
	public :

	virtual void AddRouter(unsigned RouterID)=0;

	//virtual void AddLink(unsigned int RouterSrcID, unsigned int RouterSrcPortID, unsigned int RouterDestID, unsigned int RouterDestPortID=0 /*unused*/, bool debug=false)=0;
	virtual void AddLink(unsigned int RouterSrcID, unsigned int RouterDestID, bool debug=false)=0;

	virtual  bool AddRouting(unsigned int RouterSrcID, unsigned int TargetID, unsigned int OutPortID, bool debug=false)=0;

	virtual  void BuildDefaultRoutingBidirectional(bool debug=false)=0;

	virtual void CheckMemoryMap()=0;

	virtual T_TargetID GetTargetIDFromAddress(T_MemoryAddress MemoryAddress)=0;

	virtual T_MemoryAddress GetBaseAddressFromTargetID(T_TargetID TargetID)=0;

	//connect a master to a router
	virtual void bind( sc_port<ac_tlm_transport_if>* MasterPort, T_RouterID RouterID)=0;

	//connect a slave to a router with its address map (this creates a new targetID)
	virtual void bind( sc_export<ac_tlm_transport_if>* SlavePort, T_RouterID RouterID, T_MemoryRegion MemRegion)=0;

	//binding functions to connect to CABA NoC directly to traffic generators
	T_TargetID BindBiDir(sc_fifo_in<NoCFlit>* Slave, sc_fifo_out<NoCFlit>* Master, T_RouterID RouterID);

	//connect a bidirectional traffic generator to the router
	//virtual void AddBidirTrafficEndPoint( T_RouterID RouterID)=0;

	virtual void SetFrequencyScaling(float _FrequencyScaling)=0;

	virtual void SetNoCLinkSize(unsigned int _LinkSizeInBytes)=0;

	virtual void SetTimingActivation(bool _TimingActivation = false)=0;

	virtual void SetTraceActivation(bool _TraceActivation = false)=0;

	//Note: To implement the same method on unidirectional NoC you need to keep a first list of masters and a second list of slaves
	//this could be specified by 2 new methods SetAsSlave(ID) SetAsMaster(ID)

	//void ParseCONNECTConfig(const char * TopologyFilePath, const char * RoutingFilePath );

	virtual void display_allstats()=0;

};

};//end namespace vpsim

#endif

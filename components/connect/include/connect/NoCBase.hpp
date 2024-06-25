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

#ifndef NOCBASE_HPP
#define NOCBASE_HPP

#include "systemc.h"
#include <list>
#include <map>
#include <set>
#include "dijkstra.hpp"
#include <semaphore.h> //TODO consider deleting
#include "NoCIF.hpp"



using namespace std;

namespace vpsim
{

//!
//! C_NoCBase is an sc_module that provides all necessary function for NoC topology and routing declaration
//! it does not provide actual implementation of the NoC which is performed by
//! derived classes that specialize the instanciation depending on the necessary accuracy level.
//!
class C_NoCBase: public sc_module, public C_NoCIF
{
	friend class C_NoCCycleAccurate;
	friend class C_NoCNoContention;
	friend class C_NoCTLMBase;

	protected:

	unsigned int  RouterCount; //!< number of routers in the NoC
	unsigned int  LinkCount;//!< number of links between routers in the NoC
	unsigned int  MasterCount;//!< number of initiators on the NoC
	unsigned int  SlaveCount; //!< number of slaves on the NoC

	//----------------------------------//
	// slow structs used at build time
	//----------------------------------//

	set<T_RouterID> SlowRouterIDs; //!< keeps all data from AddRouteur
	std::map<std::pair<T_RouterID,T_PortID>,std::pair<T_RouterID,T_PortID> > Links; //!< keeps all data from AddLink
	std::map<std::pair<T_RouterID,T_RouterID>,T_PortID > Routers2Port; //!< keeps all data from AddLink
	std::map<T_RouterID, std::map<T_RouterID, T_PortID> > RoutingTables;

	//the number of input ports for each router, used for implicit port instanciation
	std::map<T_RouterID, unsigned int> RouterInputPortsCount;
	std::map<T_RouterID, unsigned int> RouterOutputPortsCount;

	std::map<T_RouterID, std::list<TLMMasterBindInfo> >  TLMMasterBindInfoList;
	std::map<T_RouterID, std::list<TLMSlaveBindInfo > >  TLMSlaveBindInfoList;
	std::map<T_RouterID, std::list<CABAMasterBindInfo> >  CABAMasterBindInfoList;
	std::map<T_RouterID, std::list<CABASlaveBindInfo > >  CABASlaveBindInfoList;

	T_MemoryMap MemMap; //! the memory map of the NoC i.e. a map between Target ID (Router ID + out port ID) and address ranges
    std::map<T_RouterID, std::list< std::pair<T_PortID,T_PortID> > >TrafficEndpointInfoList;

	//NoC building status

	bool RoutingDone; //! status flag representing whether or not routing was performed on the NoC
	bool BeforeElaborationDone; //! status flag representing whether or not the before_end_of_elaboration() step occured
	bool ElaborationDone; //! status flag representing whether end_of_elaboration() step occured


	#ifdef STORE_NOC_STATS
	std::map<T_RouterID, std::map<T_SlavePortID, unsigned int > > StatsSlaveAccessCounters; //! statistic of access count to every TLM slave (or Target) in the NoC
	std::map<T_RouterID, std::map<T_SlavePortID, timespec > > total_time_wait_for_lock; //! statistic of parallel access contention for every TLM slave (or Target) in the NoC
	#endif

	//NoC speed parameters
	float FrequencyScaling; //! the ratio between NoC speed and compute speed.
	unsigned int LinkSizeInBytes; //! number of data Bytes that can transit in one flit on a NoC link

	//timing flag
	bool NoTiming; //! status flag representing whether or not NoC accesses shall be timed
	bool TraceActivation; //! status flag representing whether or not statistics shall be displayed
	timespec res; //! seems to be used for contention statistics, to be confirmed

	ofstream CONNECTTopo;

	private:

	Graph NoCGraph; //a graph representation of the NoC structure used by Auto routing

	public:

	std::list<T_TargetID> ValidTargets;

	//!
	//! Constructor that builds an empty sc_module NoC with instance name @param [in] name
	//!
	C_NoCBase(sc_module_name name);

	//!
	//! default destructor that produces statistics output and deallocation of C_NoCBase alloc'd structs
	//!
	~C_NoCBase();

	//!
	//! Creates a new router as part of the C_NoCBase
	//!
	void AddRouter(unsigned RouterID);

	//!
	//! Creates a new link between two Routers, and create Routers if they do not exist yet. Ports arautomatically added for the link
	//! @param [in] RouterSrcID : a unique ID identifying the source Router
	//! @param [in] RouterDestID : a unique ID identifying the target Router
	//! @param [in] debug : optional parameter set to false by default, enables printing of debug messages
	//!
	void AddLink(unsigned int RouterSrcID, unsigned int RouterDestID, bool debug=false);

	// void AddTargetLink(unsigned int RouterSrcID, unsigned int RouterSrcPortID, unsigned int TargetID);

	//!
	//! Creates a new entry in the routing table of a Router for routing packets to a target Router on the NoC (not necessarily directly linked to it)
	//! If a routing entry already exists for this slave, routing insertion is ignored and false is returned.
	//! @param [in] RouterSrcID : a unique ID identifying the source Router
	//! @param [in] TargetID : a unique ID identifying the final target Router
	//! @param [in] OutPortID : the Router port to which packets shall be transferred to in order to reach the slave
	//! @param [in] debug : optional parameter set to false by default, enables printing of debug messages
	//! @return true if successful, false if the Routing table already has an entry for TargetID (the previous entry is kept)
	//!
	bool AddRouting(unsigned int RouterSrcID, unsigned int TargetID, unsigned int OutPortID, bool debug=false);


	//!
	//! Performs an automated routing of the NoC using Dijkstra shortest path method
	//! @note this is not an ideal routing method as it can create congestions and deadlock, but can be useful
	//!       in first steps of developments
	//! @param [in] debug : optional parameter set to false by default, enables printing of debug messages
	//!
	void BuildDefaultRoutingBidirectional(bool debug=false);
	void DebugRoutingTables();
	void CMUDumpRouting();

	// TODO implement a unidirectional default routing
	// Note: To implement the same method on unidirectional NoC you need to keep a first list of masters and a second list of slaves
	// this could be specified by 2 new methods SetAsSlave(ID) SetAsMaster(ID)

	//void SetRouterAsInitiator(T_RouterID InitiatorID); // connect an initiator to router InitiatorID
	//void SetRouterAsTarget(T_RouterID TargetID); //connect a target to router RouterID

	private:

	//!
	//! Associates an T_MemoryRegion to a TargetID, i.e all data transfers targetting an address in this region shall
	//! be transferred to this given TargetID
	//! @note although no memory region overlapping is tested by this function
	//!       calls to CheckMemoryMap allow to verify this
	//! @param [in] TargetID : a unique ID identifying a final target Router
	//! @param [in] MemRegion : A memory region that must addressable through the TargetID
	//!
	void AddMemoryMapping( T_TargetID TargetID, T_MemoryRegion MemRegion);

	public:

	//!
	//! Asserts that no T_MemoryRegion connected to the NoC do overlap
	//! If overlapping is detected the program reaches early termination
	//!
	void CheckMemoryMap();

	//!
	//! Searches for the unique ID of the Router associated with a given address
	//! @return unique ID of the Router associated with a given address
	//!
	T_TargetID GetTargetIDFromAddress(T_MemoryAddress);

	//!
	//! Searches for the base address of memory associated to a given TargetID
	//! @return Address of the beginning of the memory space associated with a Router
	//!
	T_MemoryAddress GetBaseAddressFromTargetID(T_TargetID TargetID);

	//!
	//! connects a master to a router (actual binding is postponed until end of elaboration)
	//! @param [in] MasterPort : the tlm master port
	//! @param [in] RouterID : a unique ID identifying the Router
	//!
	void bind( sc_port<ac_tlm_transport_if>* MasterPort, T_RouterID RouterID);

	//!
	//! Connects a slave to a router with its address map (this creates a new targetID) (actual binding is postponed until end of elaboration)
	//! @param [in] SlavePort : the tlm slave port
	//! @param [in] RouterID : a unique ID identifying the target router
	//! @param [in] MemRegion : a memory region to be associated to a given SlavePort
	//!
	void bind( sc_export<ac_tlm_transport_if>* SlavePort, T_RouterID RouterID, T_MemoryRegion MemRegion);

	T_TargetID bind( sc_fifo_out<NoCFlit>* Master, T_RouterID RouterID);

	T_TargetID BindBiDir(sc_fifo_in<NoCFlit>* Slave, sc_fifo_out<NoCFlit>* Master, T_RouterID RouterID);

	//!
	//! Connects a slave to a router with its address map (this creates a new targetID) (actual binding is postponed until end of elaboration)
	//! @param [in] _FrequencyScaling : the ratio between NoC speed and compute speed. (e.g if NoC runs @1GHz but platform runs at 500MHz, _FrequencyScaling equals 2)
	//!
	void SetFrequencyScaling(float _FrequencyScaling);

	//!
	//! Defines the datawidth of NoC links. This impacts the way data transfer latency is computed in the NoC
	//! @param [in] _LinkSizeInBytes : number of data Bytes that can transit in one flit on a NoC link
	//!
	void SetNoCLinkSize(unsigned int _LinkSizeInBytes);

	//!
	//! Defines whether the TLM accesses on the NoC shall be timed or not
	//! @param [in] _TimingActivation : true if NoC shall compute timings, false otherwise
	//!
	void SetTimingActivation(bool _TimingActivation = false);

	//!
	//! Defines whether the NoC shall display debug traces or not
	//! @param [in] _TraceActivation : true if NoC shall display traces, false otherwise
	//!
	void SetTraceActivation(bool _TraceActivation = false);

	//!
	//! Creates a full NoC topology from CMU's CONNECT NoC configuration files
	//! @warning unimplemented features
	//! @param [in] TopologyFilePath : file path for CONNECT topology configuration file
	//! @param [in] TopologyFilePath : file path for CONNECT RoutingFilePathconfiguration file
	//!
	void ParseCONNECTConfig(const char * TopologyFilePath, const char * RoutingFilePath );

	//!
	//! Stores NoC statistics to a standard output file STAT_PATH/w_noc/stats_NOC_NAME
	//! @note : dump only occurs if TraceActivation is set to True
	//!
	void display_allstats();
};

};//end namespace vpsim


#endif

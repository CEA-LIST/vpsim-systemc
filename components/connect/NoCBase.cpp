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

#include "NoCBase.hpp"

#define CONNECT_SEP "\t"

using namespace vpsim;

C_NoCBase::C_NoCBase(sc_module_name name):sc_module(name)
{
	RouterCount=0;
	LinkCount=0;
	MasterCount=0;
	SlaveCount=0;

	RoutingDone=false;
	BeforeElaborationDone=false;
	ElaborationDone=false;

	FrequencyScaling=1.0;
	LinkSizeInBytes=1; //one byte per cycle => very slow by default

	NoTiming=false;
	TraceActivation=false;

	CONNECTTopo.open("Topology.txt", std::ostream::out);
	CONNECTTopo<<"##########################################################################################################"<<endl;
	CONNECTTopo<<"# This topology description file was automatically generated "<<endl;
	CONNECTTopo<<"# This description is compliant with CONNECT Networks (Carnegie Mellon University) syntax "<<endl;
	CONNECTTopo<<"# NoC debug GUI and HDL generation is available here http://users.ece.cmu.edu/~mpapamic/connect/ "<<endl;
	CONNECTTopo<<"# Network Name : "<<this->name()<<endl;
	CONNECTTopo<<"##########################################################################################################"<<endl;

}

//display_allstats (never call)
void
C_NoCBase::display_allstats()
{
	if (TraceActivation)
	{
		#ifdef COSIMULATION
		string tmp2 = VPSIM_STAT_PATH_VSIM;
		#else
			#ifdef VPSIM_STAT_PATH
				string tmp2 = VPSIM_STAT_PATH;
			#else
				string tmp2 ="/tmp/vpsim_stats/";
			#endif
		#endif
		string tmp = this->name();
		string name_trace_file =  tmp2 + "w_noc/stats_" + tmp;

		//Open file
		ofstream stat;
		stat.open (name_trace_file.c_str(),ios::out);

		//Write in file
		stat<<"------------------------------------------"<<endl;

		#ifdef STORE_NOC_STATS
		//Get total number of accesses (for all slaves)
		unsigned int total_access = 0;
		std::map<T_RouterID, std::map<T_SlavePortID, unsigned int > >::iterator IT6;
		for(IT6=StatsSlaveAccessCounters.begin();IT6!=StatsSlaveAccessCounters.end();IT6++) {
			std::map<T_SlavePortID, unsigned int >::iterator IT7;
			for(IT7=IT6->second.begin();IT7!=IT6->second.end();IT7++)
				total_access += IT7->second;
		}
		stat<<"total_nb_access="<<total_access<<endl<<endl;

		//Get total number of waiting time for locks (s)
		stat<<"total time waiting for locks for each slave (s)"<<endl;
		std::map<T_RouterID, std::map<T_SlavePortID, timespec > >::iterator IT8;
		int i = 0;
		struct timespec total_wait_for_lock;
		total_wait_for_lock.tv_sec = 0;
		total_wait_for_lock.tv_nsec = 0;
		for(IT8=total_time_wait_for_lock.begin();IT8!=total_time_wait_for_lock.end();IT8++) {
			std::map<T_SlavePortID, timespec >::iterator IT9;
			for(IT9=IT8->second.begin();IT9!=IT8->second.end();IT9++) {
				stat<<"total_time_waiting_lock_slave_"<< i << " (s)=\t"<< IT9->second.tv_sec + (float)((IT9->second.tv_nsec*1.0)/1E9) << endl;
				i++;
				total_wait_for_lock = vpsim_add_time (total_wait_for_lock, IT9->second, res);
			}
		}
		stat << "total_wait_for_lock (s)=\t" << total_wait_for_lock.tv_sec + (float)((total_wait_for_lock.tv_nsec*1.0)/1E9) << endl;
		stat<<endl;
		#endif

		//Close file
		stat.close();
	}
}

C_NoCBase::~C_NoCBase()
{
	CMUDumpRouting();

	#ifdef STORE_NOC_STATS
	ofstream NoCStatStream;
	NoCStatStream.open("/tmp/NoCStat.txt",ios::out);
	std::map<T_RouterID, std::map<T_SlavePortID, unsigned int > >::iterator IT6;
	for(IT6=StatsSlaveAccessCounters.begin();IT6!=StatsSlaveAccessCounters.end();IT6++)
	{
		NoCStatStream<<"Router "<<IT6->first<<":"<<endl;
		std::map<T_SlavePortID, unsigned int >::iterator IT7;
		for(IT7=IT6->second.begin();IT7!=IT6->second.end();IT7++)
			NoCStatStream<<"Slave"<< IT7->first <<"access count "<< IT7->second<<endl;
	}
	#endif

	SlowRouterIDs.clear();
	Links.clear();
	Routers2Port.clear();
	RoutingTables.clear();

	TLMMasterBindInfoList.clear();
	TLMSlaveBindInfoList.clear();

	CABAMasterBindInfoList.clear();
	CABASlaveBindInfoList.clear();


	MemMap.clear();

	CONNECTTopo.close();

}


void C_NoCBase::AddRouter(unsigned RouterID)
{
	if(SlowRouterIDs.count(RouterID)==0)
	{
		//cout<<"add router "<<RouterID<<endl;
		SlowRouterIDs.insert(RouterID);
		RouterInputPortsCount[RouterID]=0;
		RouterOutputPortsCount[RouterID]=0;
	}
}


//explicit Port naming : used for CONNECT import only
//void C_NoCBase::AddLink(unsigned int RouterSrcID, unsigned int RouterSrcPortID, unsigned int RouterDestID, unsigned int RouterDestPortID /*unused*/, bool debug)
//{
//}
//implicit ports
void C_NoCBase::AddLink(unsigned int RouterSrcID, unsigned int RouterDestID, bool debug)
{
	//SYSTEMC_INFO(" R"<<RouterSrcID<<":"<<RouterSrcPortID<<"->"<<RouterDestID);
	if (SlowRouterIDs.find(RouterSrcID)==SlowRouterIDs.end()) //the source Router does not exist yet, instanciate it
	{
		AddRouter(RouterSrcID);
	}
	if (SlowRouterIDs.find(RouterDestID)==SlowRouterIDs.end()) //the destination Router does not exist yet, instanciate it
	{
		AddRouter(RouterDestID);
	}

	T_PortID RouterSrcOutPortID = RouterOutputPortsCount[RouterSrcID]++;
	T_PortID RouterDestInPortID = RouterInputPortsCount[RouterDestID]++;

	if(debug)
	{
		//cout<<"New Link R"<<RouterSrcID<<"(port"<<RouterSrcOutPortID<<")->R"<<RouterDestID<<"(port"<<RouterDestInPortID<<")"<<endl;
		//CMU compatible print
		CONNECTTopo<<"RouterLink"<<CONNECT_SEP<<"R"<<RouterSrcID<<":"<<RouterSrcOutPortID<<CONNECT_SEP<<"->"<<CONNECT_SEP<<"R"<<RouterDestID<<":"<<RouterDestInPortID<<endl;
	}

	Links[ std::pair<T_RouterID,T_PortID>(RouterSrcID,RouterSrcOutPortID)]=std::pair<T_RouterID,T_LinkID>(RouterDestID,RouterDestInPortID);
	Routers2Port[ std::pair<T_RouterID,T_RouterID>(RouterSrcID,RouterDestID) ]= RouterSrcOutPortID;

	//for auto routing build a graph compatible with djkstra:
	//each element of a graph is a list of arcs to a destination and with a constant cost of 1
	if (RouterSrcID >= NoCGraph.size())
	{
		NoCGraph.resize(RouterSrcID+1);
	}
	NoCGraph[RouterSrcID].push_back(make_pair(RouterDestID,1));
}


//keeps the binding info for elaboration (dependant on level of description)
void C_NoCBase::bind( sc_port<ac_tlm_transport_if>* MasterPort, T_RouterID RouterID){

	//find new ports to connect master endpoint to router
	T_PortID OutPortID = RouterOutputPortsCount[RouterID]++;
	T_PortID InPortID = RouterInputPortsCount[RouterID]++;

	T_TargetID TargetID = T_TargetID(RouterID,OutPortID,/*IsNew=*/true);

	unsigned CMUEndpoinIDTarget = TargetID.GetCMUEndPointIDTarget();
	CONNECTTopo<<"RecvPort "<<CONNECT_SEP<<CMUEndpoinIDTarget<<CONNECT_SEP<<"->"<<CONNECT_SEP<<"R"<<RouterID<<":"<<OutPortID<<endl;
	CONNECTTopo<<"SendPort "<<CONNECT_SEP<<CMUEndpoinIDTarget<<CONNECT_SEP<<"->"<<CONNECT_SEP<<"R"<<RouterID<<":"<<InPortID<<endl;

	ValidTargets.push_back(TargetID);

	//cout<<"Master port bound to Router "<<RouterID<<" using the router input port "<<InPortID<<" and output port "<<OutPortID<<endl;

	//store binding info and ports in topology struct for end of elaboration binding
	TLMMasterBindInfo info =  TLMMasterBindInfo();
		info.MasterPort=MasterPort;
		info.RouterID=RouterID;
		info.RouterFWPort=InPortID;
		info.RouterBWPort=OutPortID ;
	TLMMasterBindInfoList[RouterID].push_back(info);
}

//implicit slave endpoint binding to RouterID
//assign an output port to the slave and use it to identify target (RouterID + output port)
//assign an input port to the slave (used for debug and CONNECT compat)
void C_NoCBase::bind( sc_export<ac_tlm_transport_if>* SlavePort, T_RouterID RouterID, T_MemoryRegion MemRegion)
{


	//find new ports to connect slave endpoint to router
	T_PortID OutPortID = RouterOutputPortsCount[RouterID]++;
	T_PortID InPortID = RouterInputPortsCount[RouterID]++;

	//store binding info and ports in topology struct for end of elaboration binding
	TLMSlaveBindInfo info =  TLMSlaveBindInfo();
			info.SlavePort=SlavePort;
			info.RouterID=RouterID;
			info.RouterFWPort=OutPortID;
			info.RouterBWPort=InPortID;

	TLMSlaveBindInfoList[RouterID].push_back(info);

	//this is a TargetID
	T_TargetID TargetID = T_TargetID(RouterID,OutPortID,/*IsNew=*/true);

	//CMU dump
	unsigned CMUEndpoinIDTarget = TargetID.GetCMUEndPointIDTarget();
	CONNECTTopo<<"RecvPort "<<CONNECT_SEP<<CMUEndpoinIDTarget<<CONNECT_SEP<<"->"<<CONNECT_SEP<<"R"<<RouterID<<":"<<OutPortID<<endl;
	CONNECTTopo<<"SendPort "<<CONNECT_SEP<<CMUEndpoinIDTarget<<CONNECT_SEP<<"->"<<CONNECT_SEP<<"R"<<RouterID<<":"<<InPortID<<endl;


	AddMemoryMapping(TargetID,MemRegion);
	ValidTargets.push_back(TargetID);

	#ifdef STORE_NOC_STATS
	//init the counters per slaves (identified by output port ID)
	StatsSlaveAccessCounters[RouterID][OutPortID]=0;
	total_time_wait_for_lock[RouterID][OutPortID].tv_sec = 0;
	total_time_wait_for_lock[RouterID][OutPortID].tv_nsec = 0;
	#endif

}


T_TargetID C_NoCBase::BindBiDir(sc_fifo_in<NoCFlit>* Slave, sc_fifo_out<NoCFlit>* Master, T_RouterID RouterID)
{

	//find new ports to connect endpoint to router
	T_PortID InPortID = RouterInputPortsCount[RouterID]++;
	T_PortID OutPortID = RouterOutputPortsCount[RouterID]++;
	T_TargetID TargetID = T_TargetID(RouterID,OutPortID,/*IsNew=*/true);

	//CMU targets
	unsigned CMUEndpoinIDTarget = TargetID.GetCMUEndPointIDTarget();
	CONNECTTopo<<"RecvPort "<<CONNECT_SEP<<CMUEndpoinIDTarget<<CONNECT_SEP<<"->"<<CONNECT_SEP<<"R"<<RouterID<<":"<<OutPortID<<endl;
	CONNECTTopo<<"SendPort "<<CONNECT_SEP<<CMUEndpoinIDTarget<<CONNECT_SEP<<"->"<<CONNECT_SEP<<"R"<<RouterID<<":"<<InPortID<<endl;

	ValidTargets.push_back(TargetID);

	CABASlaveBindInfo SlvInfo  =  CABASlaveBindInfo();
	SlvInfo.Slave=Slave;
	SlvInfo.RouterID=RouterID;
	SlvInfo.OutPortID=OutPortID;

	CABASlaveBindInfoList[RouterID].push_back(SlvInfo);


	CABAMasterBindInfo MstInfo  =  CABAMasterBindInfo();
	MstInfo.Master=Master;
	MstInfo.RouterID=RouterID;
	MstInfo.InPortID=InPortID;

	CABAMasterBindInfoList[RouterID].push_back(MstInfo);

	return TargetID;

}



void C_NoCBase::AddMemoryMapping( T_TargetID TargetID, T_MemoryRegion MemRegion)
{
	MemMap[TargetID] = MemRegion;
}

void C_NoCBase::CheckMemoryMap()
{
	//TODO
	T_MemoryMap::iterator IT;
	T_MemoryMap::iterator IT2;

	//display the MemMap
	/*for(IT=MemMap.begin();IT!=MemMap.end();IT++)
	{
		cout<<"TargetID: (RouterID "<<IT->first.first <<",SlavePortID "<<IT->first.second<<") -> ";
		cout<<"MemoryAddress: (AddressBase 0x"<<std::hex<<IT->second.first<<" AddressEnd 0x"<<IT->second.second<<")"<<std::dec<<endl;
	}*/

	//assert that MemMap is not overlapping
	for(IT=MemMap.begin();IT!=MemMap.end();IT++)
	{
		IT2=IT;
		for(IT2++;IT2!=MemMap.end();IT2++)
		{
			//2 cases:
			//case 1:
			//	[  ]
			// [     ]
			//or
			//	  [       ]
			// [     ]
			//or
			// [     ]
			//	  [       ]
			//		=> one of IT2 MemoryAddress is in IT
			T_MemoryAddress MemoryAddress1 = IT2->second.first;
			T_MemoryAddress MemoryAddress2 = IT2->second.first;
			if( (IT->second.first <=MemoryAddress1 && IT->second.second >=MemoryAddress1) ||
				(IT->second.first <=MemoryAddress2 && IT->second.second >=MemoryAddress2)		)
			{
				cerr<<"TargetID: (RouterID "<<IT->first.first <<",SlavePortID "<<IT->first.second<<") -> ";
				cerr<<"MemoryAddress: (AddressBase "<<IT->second.first<<"AddressEnd "<<IT->second.second<<")"<<endl;
				cerr<<"TargetID: (RouterID "<<IT2->first.first <<",SlavePortID "<<IT2->first.second<<") -> ";
				cerr<<"MemoryAddress: (AddressBase "<<IT2->second.first<<"AddressEnd "<<IT2->second.second<<")"<<endl;
				SYSTEMC_ERROR("The memory map contains overlapping address ");
			}
			//case 2:
			// [     ]
			//	[  ]
			//		=> first of IT is in IT2 (second too but always both => test only first)
			else if( MemoryAddress1 <=IT->second.first && MemoryAddress2 >=IT->second.first)
			{
				cerr<<"TargetID: (RouterID "<<IT->first.first <<",SlavePortID "<<IT->first.second<<") -> ";
				cerr<<"MemoryAddress: (AddressBase "<<IT->second.first<<"AddressEnd "<<IT->second.second<<")"<<endl;
				cerr<<"TargetID: (RouterID "<<IT2->first.first <<",SlavePortID "<<IT2->first.second<<") -> ";
				cerr<<"MemoryAddress: (AddressBase "<<IT2->second.first<<"AddressEnd "<<IT2->second.second<<")"<<endl;
				SYSTEMC_ERROR("The memory map contains overlapping address ");

			}
		}

	}
}


T_TargetID C_NoCBase::GetTargetIDFromAddress(T_MemoryAddress MemoryAddress)
{
	T_MemoryMap::iterator IT;
	for(IT=MemMap.begin();IT!=MemMap.end();IT++)
	{
		if(IT->second.first <=MemoryAddress && IT->second.second >=MemoryAddress)
		{
			//we found a memory region containing the Address
			return IT->first;
		}
	}
	SYSTEMC_ERROR("No router found with this MemoryAddress "<<MemoryAddress);
}

//mainly used for traffic generators tests
T_MemoryAddress C_NoCBase::GetBaseAddressFromTargetID(T_TargetID TargetID)
{
	return MemMap[TargetID].first;
}


bool C_NoCBase::AddRouting(unsigned int RouterSrcID, unsigned int RouterTargetID, unsigned int OutPortID, bool debug)
{
//	if(sc_start_of_simulation_invoked())
//			SYSTEMC_ERROR("Simulation already invoked");

	if(RoutingTables.find(RouterSrcID) != RoutingTables.end() && RoutingTables[RouterSrcID].find(RouterTargetID)!= RoutingTables[RouterSrcID].end())
	{
		//the mapping already exist for this target and router => keep the old one
		if(debug)
		{
			cout<<"OutPortID "<<OutPortID<<" ignored"<<endl;
		}
		return false;
	}
	if(debug)
	{
		cout<<"OutPortID "<<OutPortID<<" added"<<endl;
	}

	RoutingTables[RouterSrcID][RouterTargetID]=OutPortID;
	return true;
}

void C_NoCBase::CMUDumpRouting()
{
	ofstream CONNECTRouting;
	CONNECTRouting.open("Routing.txt", std::ostream::out);
	CONNECTRouting<<"##########################################################################################################"<<endl;
	CONNECTRouting<<"# This topology description file was automatically generated "<<endl;
	CONNECTRouting<<"# This description is compliant with CONNECT Networks (Carnegie Mellon University) syntax "<<endl;
	CONNECTRouting<<"# NoC debug GUI and HDL generation is available here http://users.ece.cmu.edu/~mpapamic/connect/ "<<endl;
	CONNECTRouting<<"# Network Name : "<<this->name()<<endl;
	CONNECTRouting<<"##########################################################################################################"<<endl;

	//for al routers
	//do for loop on router IDs
	std::map<T_RouterID, std::map<T_RouterID, T_PortID> >::iterator IT;
	for(IT = RoutingTables.begin();  IT != RoutingTables.end(); IT++)
	{
		T_RouterID RouterID = IT->first;
		std::map<T_RouterID, T_PortID> RouterRouting = IT->second;

		CONNECTRouting<<"# Routing for Router "<<RouterID<<endl;

		//for all targets
		std::list<T_TargetID>::iterator ITTargets;
		for(ITTargets = ValidTargets.begin() ; ITTargets!= ValidTargets.end(); ITTargets++)
		{
			T_TargetID TargetID= *ITTargets;

			T_RouterID TargetRouterID= TargetID.first;
			T_PortID TargetPortID=TargetID.second;
			uint CMUEndpoinIDTarget=TargetID.GetCMUEndPointIDTarget();

			//local endpoints
			if (TargetRouterID==RouterID)
			{
				//local target use the port field in the targetID
				CONNECTRouting<<"R"<<RouterID<<CONNECT_SEP<<":"<<CONNECT_SEP<<CMUEndpoinIDTarget<<CONNECT_SEP<<"->"<<CONNECT_SEP<<TargetPortID<<endl;
			}
			else
			{
				//we use the per RouterID Routing table info to send the requests to the adapted port
				CONNECTRouting<<"R"<<RouterID<<CONNECT_SEP<<":"<<CONNECT_SEP<<CMUEndpoinIDTarget<<CONNECT_SEP<<"->"<<CONNECT_SEP<<RouterRouting[TargetRouterID]<<endl;
			}

		}
		CONNECTRouting<<endl<<endl;
	}

	CONNECTRouting.close();

}

void C_NoCBase::BuildDefaultRoutingBidirectional(bool debug)
{
	//for all init router
	set<T_RouterID>::iterator IT1, IT2;
	for(IT1=SlowRouterIDs.begin(); IT1!=SlowRouterIDs.end(); IT1++)
	{
		//for all target Router
		for(IT2=SlowRouterIDs.begin(); IT2!=SlowRouterIDs.end(); IT2++)
		{

			T_RouterID MasterID = *IT1;
			T_RouterID SlaveID  = *IT2;

			//target == slave no routing needed
			if(MasterID==SlaveID)
				continue;

			//if the target is already routed for router in IT1, ignore it
			if(RoutingTables.find(MasterID) != RoutingTables.end() && RoutingTables[MasterID].find(SlaveID)!= RoutingTables[MasterID].end())
				continue;

			//search for shortest path between IT1 (src) and IT2 (target)
			vector<int> path;
			dijkstra(NoCGraph,*IT1,*IT2,path);

			if(debug)
			{
				cout<<"path from R"<<*IT1<<" to R"<<*IT2<<":"<<endl;
				for(int i=path.size()-1;i>=0;i--)
		 			cout<<path[i]<<"->";
		 		cout<<endl;
		 	}

			//build the routing table according to this path
			for(int i=path.size()-1;i>0;i--){
				T_RouterID CurRouter= path[i];
				T_RouterID NextRouter= path[i-1];
				T_PortID CurPort = Routers2Port[ std::pair<T_RouterID,T_RouterID>(CurRouter,NextRouter) ];

				if( !AddRouting(CurRouter,SlaveID,CurPort))//stop when reaching a router with an already mapped target IT2
					break;
					/*
				RoutingTables[path[i]][*IT2]=
				C_Router* RouterOnPath= Routers.at(path[i]);
				if( !RouterOnPath->AddRouting(IT2->first,Routers.at(path[i-1]))) //stop when reaching a router with an already mapped target IT2
					break;
					*/
			}
			//update the network costs to take into account increased usage on links used by path
			//shall automatically implement the histogram method targetted by Nico
		}
	}

	//we do not need this graph anymore
	NoCGraph.clear();

	RoutingDone=true;
}

void C_NoCBase::DebugRoutingTables()
{
	//for all init router
		set<T_RouterID>::iterator IT1, IT2;
		for(IT1=SlowRouterIDs.begin(); IT1!=SlowRouterIDs.end(); IT1++)
		{
			//for all target Router
			for(IT2=SlowRouterIDs.begin(); IT2!=SlowRouterIDs.end(); IT2++)
			{

				T_RouterID MasterID = *IT1;
				T_RouterID SlaveID  = *IT2;

				cout<<"Master "<<MasterID<<" to SlaveID "<< SlaveID <<" use port :"<<RoutingTables[MasterID][SlaveID]<<endl;

			}
		}
}

//NoC speed parameters
void C_NoCBase::SetFrequencyScaling(float _FrequencyScaling)
{
	sc_time tmp ((double)(1.0/_FrequencyScaling), SC_NS);
	if(tmp<sc_get_time_resolution()) {
		SYSTEMC_WARN("1/\"Frequency scaling\" must be higher than the resolution time");
	}

	FrequencyScaling=_FrequencyScaling;
}

void C_NoCBase::SetNoCLinkSize(unsigned int _LinkSizeInBytes)
{
	LinkSizeInBytes=_LinkSizeInBytes;
}

void C_NoCBase::SetTimingActivation(bool _TimingActivation)
{
	NoTiming=!(_TimingActivation);
}

void C_NoCBase::SetTraceActivation(bool _TraceActivation)
{
	TraceActivation=_TraceActivation;
}

void C_NoCBase::ParseCONNECTConfig(const char * TopologyFilePath, const char * RoutingFilePath )
{
//	if(sc_start_of_simulation_invoked())
//			SYSTEMC_ERROR("Simulation already invoked");

	SYSTEMC_ERROR("unimplemented feature");
}

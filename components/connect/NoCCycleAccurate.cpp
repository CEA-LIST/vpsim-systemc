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

#include "NoCCycleAccurate.hpp"

using namespace vpsim;

C_NoCCycleAccurate::C_NoCCycleAccurate(sc_module_name name_, C_NoCBase* Topo_) : sc_module(name_)
{
	Topo = Topo_;

	FifoSize=1024;//2
	NoCCycleAccurateBeforeElaborationCalled=false;
};

C_NoCCycleAccurate::~C_NoCCycleAccurate()
{
	if(NoCCycleAccurateBeforeElaborationCalled)
	{
		set<T_RouterID>::iterator IT;

		//stats forward
		for(IT= Topo->SlowRouterIDs.begin(); IT!=Topo->SlowRouterIDs.end(); IT++)
		{
			T_RouterID RouterID=*IT;
			SYSTEMC_ROUTER_ACCESS_STATS("Router "<<RouterID<<"Access stats fw "<<Routers[RouterID]->RoutedFlitsFW);
		}

		//stats backward
		for(IT= Topo->SlowRouterIDs.begin(); IT!=Topo->SlowRouterIDs.end(); IT++)
		{
			T_RouterID RouterID=*IT;
			SYSTEMC_ROUTER_ACCESS_STATS("Router "<<RouterID<<"Access stats BW "<<Routers[RouterID]->RoutedFlitsBW);
		}


		for(IT= Topo->SlowRouterIDs.begin(); IT!=Topo->SlowRouterIDs.end(); IT++)
		{
			T_RouterID RouterID=*IT;
			delete Routers[RouterID];
		}
		Routers.clear();

		//connect the router altogether
		std::map<std::pair<T_RouterID,T_PortID>,std::pair<T_RouterID,T_LinkID> >::iterator ITLink;
		for(ITLink=Topo->Links.begin(); ITLink!=Topo->Links.end(); ITLink++)
		{
			T_RouterID SrcRouterID = ITLink->first.first ;
			T_PortID SrcRouterPortID = ITLink->first.second;
			T_RouterID DestRouterID = ITLink->second.first;

			delete Fifos[ std::pair<T_RouterID,T_RouterID>(SrcRouterID,DestRouterID)];
		}
		Fifos.clear();

		list< C_WrapperMasterNoCToFifo * >::iterator ITMaster; //to store master wrappers
		for(ITMaster=WrapperMasterNoCToFifos.begin();ITMaster!=WrapperMasterNoCToFifos.end(); ITMaster++ )
		{
			delete *ITMaster;
		}

		list< C_WrapperSlaveFifoToNoC * >::iterator ITSlave; //to store master wrappers
		for(ITSlave=WrapperSlaveFifoToNoCs.begin();ITSlave!=WrapperSlaveFifoToNoCs.end(); ITSlave++ )
		{
			delete *ITSlave;
		}

		list< sc_fifo<NoCFlit>* >::iterator ITLocalFifo;
		for(ITLocalFifo=LocalLinksFifo.begin();ITLocalFifo!=LocalLinksFifo.end(); ITLocalFifo++ )
		{
			delete *ITLocalFifo;
		}

	}
}

void C_NoCCycleAccurate::before_end_of_elaboration()
{
	if(Topo->NoTiming)
		SYSTEMC_ERROR("Cannot remove timings from CycleAccurate models");

	//build all routers with their output routing table
	set<T_RouterID>::iterator IT;
	for(IT= Topo->SlowRouterIDs.begin(); IT!=Topo->SlowRouterIDs.end(); IT++)
	{
		T_RouterID RouterID=*IT;

		//create router
		stringstream ss;
		ss<<"R"<<RouterID;
		Routers[RouterID]=new C_Router(ss.str().c_str(),RouterID, Topo->LinkSizeInBytes,Topo->FrequencyScaling,Topo->NoTiming);

		list<TLMMasterBindInfo>::iterator ITMasters;
		for(ITMasters= Topo->TLMMasterBindInfoList[RouterID].begin(); ITMasters!=Topo->TLMMasterBindInfoList[RouterID].end(); ITMasters++)
		{
			Routers[RouterID]->AddInPort(ITMasters->RouterFWPort);
			Routers[RouterID]->AddOutPort(ITMasters->RouterBWPort);

			//build the wrapper for this master
			stringstream ss;
			ss<<"WrapperMasterNoCToFifo_"<<RouterID<<"_"<<ITMasters->RouterBWPort;
			C_WrapperMasterNoCToFifo * RouterWrap = new C_WrapperMasterNoCToFifo(ss.str().c_str(),RouterID,Topo->LinkSizeInBytes,Topo->FrequencyScaling,Topo->NoTiming);
			RouterWrap->clk(*clk);
			WrapperMasterNoCToFifos.push_back(RouterWrap); //for future dealloc;
			RouterWrap->SetMemoryMap(&Topo->MemMap);

			//Bind the master to the wrapper
			ITMasters->MasterPort->bind(RouterWrap->MasterIn);

			//we create a fifo in/out to connect locally to the router for inbound/outbound messages
			sc_fifo<NoCFlit> * CurFifoFW =new sc_fifo<NoCFlit>(FifoSize);
			sc_fifo<NoCFlit> * CurFifoBW =new sc_fifo<NoCFlit>(FifoSize);

			//fw message binding
				RouterWrap->FifoOut.bind(*CurFifoFW);
				//get the routers in
				sc_fifo_in<NoCFlit>* InPort =Routers[RouterID]->InputPorts[ITMasters->RouterFWPort];
				InPort->bind(*CurFifoFW);

			//bw message binding
				RouterWrap->FifoIn.bind(*CurFifoBW);
				//get the routers out
				sc_fifo_out<NoCFlit>* OutPort = Routers[RouterID]->OutputPorts[ITMasters->RouterBWPort];
				OutPort->bind(*CurFifoBW);

		}

		list<TLMSlaveBindInfo>::iterator ITSlaves;
		for(ITSlaves= Topo->TLMSlaveBindInfoList[RouterID].begin(); ITSlaves!=Topo->TLMSlaveBindInfoList[RouterID].end(); ITSlaves++)
		{
			Routers[RouterID]->AddInPort(ITSlaves->RouterBWPort);
			Routers[RouterID]->AddOutPort(ITSlaves->RouterFWPort);

			//build the wrapper for this slave
			stringstream ss;
			ss<<"WrapperSlaveFifoToNoC"<<RouterID<<"_"<<ITSlaves->RouterBWPort;
			C_WrapperSlaveFifoToNoC * RouterWrap = new C_WrapperSlaveFifoToNoC(ss.str().c_str(),RouterID,Topo->LinkSizeInBytes,Topo->FrequencyScaling,Topo->NoTiming);
			RouterWrap->clk(*clk);
			WrapperSlaveFifoToNoCs.push_back(RouterWrap); //for future dealloc;

			//Bind the slave to the wrapper
			//RouterWrap->SlaveOuts[0]->bind(*ITSlaves->SlavePort);
			RouterWrap->SlaveOut.bind(*ITSlaves->SlavePort);

			//we create a fifo in/out to connect locally to the router for inbound/outbound messages
			sc_fifo<NoCFlit> * CurFifoFW =new sc_fifo<NoCFlit>(FifoSize);
			sc_fifo<NoCFlit> * CurFifoBW =new sc_fifo<NoCFlit>(FifoSize);

			//fw message binding
				RouterWrap->FifoOut.bind(*CurFifoBW);
				//get the routers in
				sc_fifo_in<NoCFlit>* InPort =Routers[RouterID]->InputPorts[ITSlaves->RouterBWPort];
				InPort->bind(*CurFifoBW);

			//bw message binding
				RouterWrap->FifoIn.bind(*CurFifoFW);
				//get the routers out
				sc_fifo_out<NoCFlit>* OutPort = Routers[RouterID]->OutputPorts[ITSlaves->RouterFWPort];
				OutPort->bind(*CurFifoFW);
		}

		list<CABASlaveBindInfo>::iterator ITCABASlaves;
		for(ITCABASlaves= Topo->CABASlaveBindInfoList[RouterID].begin(); ITCABASlaves!=Topo->CABASlaveBindInfoList[RouterID].end(); ITCABASlaves++)
		{
			//create the port
			Routers[RouterID]->AddOutPort(ITCABASlaves->OutPortID);

			//create the fifo
			sc_fifo<NoCFlit> * CurFifoFW =new sc_fifo<NoCFlit>(FifoSize);

			//bind
			//cout<<"binding slave "<<ITCABASlaves->Slave<<endl;
			ITCABASlaves->Slave->bind(*CurFifoFW);
			Routers[RouterID]->OutputPorts[ITCABASlaves->OutPortID]->bind(*CurFifoFW);
		}

		list<CABAMasterBindInfo>::iterator ITCABAMasters;
		for(ITCABAMasters= Topo->CABAMasterBindInfoList[RouterID].begin(); ITCABAMasters!=Topo->CABAMasterBindInfoList[RouterID].end(); ITCABAMasters++)
		{
			//create the port
			Routers[RouterID]->AddInPort(ITCABAMasters->InPortID);

			//create the fifo
			sc_fifo<NoCFlit> * CurFifoFW =new sc_fifo<NoCFlit>(FifoSize);

			//bind
			//cout<<"binding master"<<ITCABAMasters->Master<<endl;
			ITCABAMasters->Master->bind(*CurFifoFW);
			Routers[RouterID]->InputPorts[ITCABAMasters->InPortID]->bind(*CurFifoFW);
		}



		//set the routing table for this router for all endpoints
		std::list<T_TargetID>::iterator ITTargets;
		for(ITTargets=Topo->ValidTargets.begin();ITTargets!=Topo->ValidTargets.end();ITTargets++)
		{
			T_TargetID TargetID= *ITTargets;
			T_RouterID TargetRouterID= TargetID.first;
			T_PortID TargetPortID=TargetID.second;

			//local endpoints
			if (TargetRouterID==RouterID)
			{
				//we associate the target with the local output port (i.e. the Target OutportID field)
				Routers[RouterID]->AddOutMapping(TargetID, TargetPortID);
			}
			else
			{
				//we use the per RouterID Routing table info to send the requests to the adapted port
				Routers[RouterID]->AddOutMapping(TargetID, Topo->RoutingTables[RouterID][TargetRouterID]);
			}
		}
	}


	//connect the routers altogether
	std::map<std::pair<T_RouterID,T_PortID>,std::pair<T_RouterID,T_LinkID> >::iterator ITLink;

	for(ITLink=Topo->Links.begin(); ITLink!=Topo->Links.end(); ITLink++)
	{
		T_RouterID SrcRouterID = ITLink->first.first ;
		T_PortID SrcRouterPortID = ITLink->first.second;
		T_RouterID DestRouterID = ITLink->second.first;
		T_RouterID DestRouterPortID = ITLink->second.second;

		//create the routers in
		Routers[DestRouterID]->AddInPort(DestRouterPortID);
		sc_fifo_in<NoCFlit>* InPort =Routers[DestRouterID]->InputPorts[DestRouterPortID];

		sc_fifo<NoCFlit> * CurFifo =new sc_fifo<NoCFlit>(FifoSize);

		Fifos[ std::pair<T_RouterID,T_RouterID>(SrcRouterID,DestRouterID)] = CurFifo;

		InPort->bind(*CurFifo);
		//TODO
		Routers[SrcRouterID]->OutputPorts[SrcRouterPortID]->bind(*CurFifo);
	}

	BeforeElaborationDone=true;
	NoCCycleAccurateBeforeElaborationCalled=true;
};

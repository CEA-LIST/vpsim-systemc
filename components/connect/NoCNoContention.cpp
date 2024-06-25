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

#include "NoCNoContention.hpp"

using namespace vpsim;

	C_NoCNoContention::C_NoCNoContention(sc_module_name name_, C_NoCBase* Topo_): C_NoCTLMBase(name_,Topo_)
	{
		Topo=Topo_;

		//SYSTEMC_INFO("Constructor Called");
		NoCNoContentionBeforeElaborationCalled=false;

	}

	C_NoCNoContention::~C_NoCNoContention()
	{
		//SYSTEMC_INFO("Destructor Called");

		if (NoCNoContentionBeforeElaborationCalled)
		{
			DoPortDeallocation();

			//clean HopCount
			for(unsigned int RouterID=0; RouterID<Topo->RouterCount;RouterID++)
			{
				delete[] HopCount[RouterID];
			}
			delete [] HopCount;
		}

	}

	void C_NoCNoContention::before_end_of_elaboration(){

		//SYSTEMC_INFO("before_end_of_elaboration Called");

		DoPortInstanciationAndBinding();

		//we need to populate the fast structures used for Routing
		Topo->RouterCount=Topo->SlowRouterIDs.size();
		//cout<<"RouterCount"<<RouterCount<<endl;

		Topo->SlaveCount=Topo->RouterCount; //all routers are slaves too so far
		Topo->MasterCount=Topo->RouterCount;
		Topo->LinkCount=Topo->Links.size();


		Next = new std::pair<T_RouterID,T_LinkID> * [Topo->RouterCount];
		for(unsigned int RouterID=0; RouterID<Topo->RouterCount;RouterID++)
		{
			Next[RouterID]=new std::pair<T_RouterID,T_LinkID> [Topo->SlaveCount];
			for(unsigned int SlaveID=0; SlaveID<Topo->SlaveCount;SlaveID++)
			{
				T_PortID outport=Topo->RoutingTables[RouterID][SlaveID];
				std::pair<T_RouterID,T_LinkID> NextRouterAndLink = Topo->Links[std::pair<T_RouterID,T_PortID>(RouterID,outport)];

				Next[RouterID][SlaveID]=NextRouterAndLink;
			}
		}

		HopCount = new CycleCount* [Topo->RouterCount];
		for(unsigned int RouterID=0; RouterID<Topo->RouterCount;RouterID++)
		{
			HopCount[RouterID]=new CycleCount [Topo->SlaveCount];
			for(unsigned int SlaveID=0; SlaveID<Topo->SlaveCount;SlaveID++)
			{
				CycleCount HopCounter=0;
				T_RouterID CurRouter=RouterID;
				//search for the routing
				std::pair<T_RouterID,T_LinkID> nexts;
				while(CurRouter != SlaveID)
				{
					nexts =  Next[CurRouter][SlaveID];
					HopCounter++;
					CurRouter=nexts.first;
				}
				//cout<<"HopCount["<<RouterID<<"]["<<SlaveID<<"]="<<HopCounter<<endl;
				HopCount[RouterID][SlaveID]=HopCounter;
			}
		}

		//clean Next
		for(unsigned int RouterID=0; RouterID<Topo->RouterCount;RouterID++)
		{
			delete[] Next[RouterID];
		}
		delete [] Next;

		NoCNoContentionBeforeElaborationCalled=true;
		Topo->BeforeElaborationDone=true;

		//SYSTEMC_INFO("before_end_of_elaboration Ended");

	}

	//ac_tlm_rsp C_NoCNoContention::transport(ac_tlm_req const &  req){
	void C_NoCNoContention::b_transport( tlm::tlm_generic_payload& trans, sc_time& delay ){

		//cout<<"C_NoCNoContention::transport called"<<endl;

		T_TargetID TargetID= Topo->GetTargetIDFromAddress(trans.get_address()); //TODO check if costy and seek optimisation
		T_RouterID DestID= TargetID.first;
		T_SlavePortID SlavePortID = TargetID.second;
		//T_RouterID SrcID = req.dev_id;
		T_RouterID SrcID = 0;
		//FIXME : consider trans.get_extension() to hold master ID
		//CycleCount LocalTime= (CycleCount) sc_time_stamp().to_double();

		//forward travel
		CycleCount LatencyForward = HopCount[SrcID][DestID];

		//additional cycles to take into account the travel from master to its router and from the destination
		//router to its slave for forward travel
		LatencyForward+=2;
		ac_tlm_rsp rsp;
		rsp= (*OutPorts[DestID][SlavePortID])->transport(req);

		#ifdef STORE_NOC_STATS
		Topo->StatsSlaveAccessCounters[DestID][SlavePortID]++;
		#endif

		//rsp.time = 0;
		CycleCount LatencySlave = rsp.time;

		//backward travel
		CycleCount LatencyBackward = LatencyForward;

		CycleCount LatencyBurst=0;
		if(req.type==READ_BURST || req.type==WRITE_BURST)
			LatencyBurst=ceil((float)req.size_burst/Topo->LinkSizeInBytes)-1; //-1 to take into account only overhead of additional flits
		//cumulate latencies

		float TotalLatenciesNoC = LatencyForward + LatencyBackward + LatencyBurst;

		cout<<"LatencyForward "<<LatencyForward<<endl;
		cout<<"LatencyBackward "<<LatencyBackward<<endl;
		cout<<"TotalLatenciesNoC "<<LatencyBurst<<endl;
		cout<<"req.size_burst "<<req.size_burst<<endl;
		cout<<"LinkSizeInBytes "<<Topo->LinkSizeInBytes<<endl;
		cout<<"TotalLatenciesNoC "<<TotalLatenciesNoC<<endl;

		if(Topo->NoTiming)
		{
			//forward the time of the slave directly
		}
		else{
			//always round to ceiling
			rsp.time = ceil(TotalLatenciesNoC/Topo->FrequencyScaling)+ LatencySlave; //this method is the correct one for ISS latency but not for accuracy charac
			//rsp.time = TotalLatenciesNoC/Topo->FrequencyScaling + LatencySlave; //this method is correct for CMU accuracy charac
			//cout<<"rsp.time"<<rsp.time;

		}

		return rsp;
	}


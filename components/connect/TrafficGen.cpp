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

#include "TrafficGen.hpp"

//---------------------------------------------------------------------------------------//
//		CABA Generator member functions
//---------------------------------------------------------------------------------------//

C_CABATrafficGen::C_CABATrafficGen(sc_module_name name, CycleCount InterReqLatency_):sc_module(name)
{
	InterReqLatency=InterReqLatency_;
	SC_THREAD(Gen);
}//end constructor

void C_CABATrafficGen::SetSourceID(T_TargetID SourceID_)
{
	SourceID=SourceID_;
}

void C_CABATrafficGen::SetValidTargets(std::list<T_TargetID> * ValidTargets_)
{
	ValidTargets=ValidTargets_;
}
T_TargetID C_CABATrafficGen::GetRandomTargetID()
{
	unsigned NBTargets = ValidTargets->size();
	unsigned TargetOffset = rand()%NBTargets;

	std::list<T_TargetID>::iterator it=	ValidTargets->begin();
	for(unsigned int i= 0; i<TargetOffset;i++)
		it++;
	return *it;
}

void C_CABATrafficGen::Gen()
{
	double RandomVal_0_1 = ((double) rand())/RAND_MAX;
	wait(RandomVal_0_1*InterReqLatency,SC_NS);

	NoCFlit flit;

	flit.SrcId=SourceID;
//	flit.req=NULL;
//	flit.rsp=NULL;
	flit.Last=true;
	flit.IsFW=true;
	while(1){
		wait(clk.posedge_event());

		flit.EmissionTimeStamp=sc_time_stamp();
		flit.TargetId=GetRandomTargetID(); //need to know allowed target IDs
		flit.CMUDump();
		FifoOut.write(flit);

		wait(InterReqLatency,SC_NS);

	}

}


//---------------------------------------------------------------------------------------//
//		CABA Consumer member functions
//---------------------------------------------------------------------------------------//


unsigned int C_CABATrafficCons::FlitsCountAll=0;
CycleCount C_CABATrafficCons::TotalLatencyAll=0;

C_CABATrafficCons::C_CABATrafficCons(sc_module_name name):sc_module(name)
{
	TotalLatency=0;
	FlitsCount=0;

	SC_THREAD(Cons);
}//end constructor


void C_CABATrafficCons::Cons()
{
	NoCFlit flit;
	while(1)
	{
		wait(clk.posedge_event());

		FifoIn.read(flit); //keep on reading input messages
		//cout<<"Msg rcv on endpoint "<<flit.TargetId.first<<","<<flit.TargetId.second<<" ("<<sc_time_stamp()<<")"<<endl;
		//flit.CMUDump(); //only dump emission of messages
		CycleCount Lat= (CycleCount) (sc_time_stamp()-flit.EmissionTimeStamp).to_double()/10;
		if (Lat < 1)
			cerr<<"latency "<<Lat<<endl;
		FlitsCount++;
		FlitsCountAll++;
		TotalLatency+= Lat;
		TotalLatencyAll+=Lat;

	}

}


void C_CABATrafficCons::DisplayLoadDelayCurveAll()
{
	float FlitsPerCycle=((double)10*FlitsCountAll)/sc_time_stamp().to_double();

	cout<<FLOAT_FORMAT<<FlitsPerCycle<<CSV_SEP<<FLOAT_FORMAT<<((float)TotalLatencyAll)/FlitsCountAll<<CSV_SEP<<"(TotalFlitsReceived "<<FlitsCountAll<<")"<<endl;

}

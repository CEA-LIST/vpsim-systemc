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

#include "WrapperNoC.hpp"
#include <algorithm>

using namespace vpsim;

//----------------------------------------------------------------------------
// C_BasicWrapperMasterNoC
//----------------------------------------------------------------------------


C_BasicWrapperMasterNoC::C_BasicWrapperMasterNoC(sc_module_name name, unsigned int _ID):sc_module(name)
{
	MasterIn.bind(*this);
	ID=_ID;
};

//ac_tlm_rsp C_BasicWrapperMasterNoC::transport(ac_tlm_req const &  req){
void C_BasicWrapperMasterNoC::b_transport( tlm::tlm_generic_payload& trans, sc_time& delay ){

//	ac_tlm_req LocalReq;
////			LocalReq.LocalTime=req.LocalTime;
//	LocalReq.addr=req.addr;
//	LocalReq.data=req.data;
//	LocalReq.size_burst=req.size_burst;
//	LocalReq.stat=req.stat;
//	LocalReq.type=req.type;
//	LocalReq.dev_id=ID;

	//FIXME add ID to the trans payload

	 MasterOut->b_transport(trans);
	 return trans;
}


//----------------------------------------------------------------------------
// C_WrapperSlaveFifoToNoC
//----------------------------------------------------------------------------

T_TargetID C_WrapperMasterNoCToFifo::GetTargetIDFromAddress(T_MemoryAddress MemoryAddress)
{
	T_MemoryMap::iterator IT;
	for(IT=MemMap->begin();IT!=MemMap->end();IT++)
	{
		if(IT->second.first <=MemoryAddress && IT->second.second >=MemoryAddress)
		{
			//we found a memory region containing the Address
			return IT->first;
		}
	}

	SYSTEMC_ERROR("No router found with this MemoryAddress");
}


//constructor
C_WrapperMasterNoCToFifo::C_WrapperMasterNoCToFifo(sc_module_name name, unsigned int _ID, unsigned int _LinkSizeInBytes, float _FrequencyScaling, bool NoTiming=false):
		sc_module(name)
{
	ParallelAccessCount=0;

	FrequencyScaling=_FrequencyScaling;
	LinkSizeInBytes=_LinkSizeInBytes;

	MasterIn.bind(*this);
	//ID=_ID;
	ID=0;

//	if(NoTiming)
//		NoCCycle=SC_ZERO_TIME;
//	else
//		NoCCycle=sc_time(1.0/_FrequencyScaling,SC_NS);

	//SC_THREAD(RouteBW);
	SC_METHOD(RouteBW);
#ifndef SYSTEMC_NG_ONLY
//	sensitive << FifoIn.data_written();
	sensitive << clk.pos();
#endif
	dont_initialize();
	MemMap=NULL;
}


void C_WrapperMasterNoCToFifo::SetMemoryMap(T_MemoryMap* _MemMap)
{
	MemMap=_MemMap;
}

//ac_tlm_rsp C_WrapperMasterNoCToFifo::transport(ac_tlm_req const &  req){
void C_WrapperMasterNoCToFifo::b_transport( tlm::tlm_generic_payload& trans, sc_time& delay ){

	ac_tlm_rsp NullRsp;

	//wait for the posedge_event to sync the evaluation with router method evaluation
	wait(clk.posedge_event());

	ParallelAccessCount++;
	if (ParallelAccessCount>1)
	{
		if (DEBUG_CurRequests.find(&trans)!=DEBUG_CurRequests.end()){ //the request is already used, this should never happen
			SYSTEMC_ERROR("Parallel access with request address already in use !!")
		}
		//else{
			//cout<<"Access to the address 0x"<<hex<<req.addr<<dec<<" (ID is "<<GetTargetIDFromAddress(req.addr).first<<")"<<endl;
			//SYSTEMC_ERROR("ParallelAccessCount>1");
		//}
	}
	DEBUG_CurRequests[&req]=true;

	NoCFlit flit;
	flit.TargetId = GetTargetIDFromAddress(trans.get_address());
//	flit.SrcId =T_TargetID(ID,-1);
//	flit.SrcId =T_TargetID(ID,req.dev_id);
//FIXME handle dev id in standard payload requests
	flit.SrcId =T_TargetID(ID,-1);
	//flit.PrevRouterId
	flit.CurrentInputPortID = 0;
	flit.Last=false;
	flit.req=& trans;
	//flit.rsp=NULL;
	flit.IsFW=true;

	//reset response status for this request
	ResponseReceived[&trans]=std::pair<bool,ac_tlm_rsp>(false, NullRsp);

	//manage burst length
	unsigned int FlitsToSend=0;

	//removed to be compliant with connect
	FlitsToSend++; //header flit
	FlitsToSend+=ceil(((float)sizeof(trans.get_address()))/LinkSizeInBytes);
	//end removed connect

	if(req.type==WRITE_BURST)
		FlitsToSend+=ceil(((float) trans.get_data_length())/LinkSizeInBytes);
	if(req.type==WRITE)
		FlitsToSend+=ceil(((float) trans.get_data_length())/LinkSizeInBytes);


	FlitsToSend=std::max<unsigned>(FlitsToSend,1);

//	cout<<sc_time_stamp()<<"flits to send FW "<<FlitsToSend<<endl;

	//send flits one by one
	for(unsigned int i =0; i<FlitsToSend; i++)
	{
		if(i==FlitsToSend-1)
			flit.Last=true;

		SYSTEMC_WRAPPER_CA("try forward flit sent");
		if(!FifoOut.nb_write( flit )){
			i--;
		}
		else{
			flit.CMUDump();
		}


//		wait(NoCCycle);
		wait(clk.posedge_event());
	}
	SYSTEMC_WRAPPER_CA("last forward flit sent (prev cycle)");

	//wait for bw message
	while(ResponseReceived[&trans]==false)
//		wait(NoCCycle);
		wait(clk.posedge_event());

//	//assert that the rsp is not corrupt (req data == rsp data)
//	if(ResponseReceived[&trans].second.data!=req.data)
//		SYSTEMC_WARN("wrong rsp data ");

	//send the response from the slave
	// no wait shall be performed on master side all waits have been taken into account during routing
	// so we do not update delay

	SYSTEMC_WRAPPER_CA("send back tlm response");

	ParallelAccessCount--;
	DEBUG_CurRequests.erase(&req);
	return;
}

void C_WrapperMasterNoCToFifo::RouteBW()
{
	NoCFlit Flitbw;
//	while(true)
	{

//		while(!FifoIn.nb_read(Flitbw))
		if(!FifoIn.nb_read(Flitbw))
		{
//			wait(NoCCycle);
			return;
		}
		//bw flit received
		if(Flitbw.Last)
		{
			//if(Flitbw.rsp.data!=Flitbw.req->data)
			//	SYSTEMC_ERROR("wrong rsp data ");

			ResponseReceived[Flitbw.req]=true;
			SYSTEMC_WRAPPER_CA("last backward flit received");
//			wait(NoCCycle);
		}
	}
}


//----------------------------------------------------------------------------
// C_WrapperSlaveFifoToNoC
//----------------------------------------------------------------------------

//constructor
C_WrapperSlaveFifoToNoC::C_WrapperSlaveFifoToNoC(sc_module_name name, unsigned int _ID, unsigned int _LinkSizeInBytes, float _FrequencyScaling, bool NoTiming=false):
		sc_module(name)
{
	FrequencyScaling=_FrequencyScaling;
	LinkSizeInBytes=_LinkSizeInBytes;
	ID=_ID;

//	if(NoTiming)
//		NoCCycle=SC_ZERO_TIME;
//	else
//		NoCCycle=sc_time(1.0/_FrequencyScaling,SC_NS);

	SC_THREAD(RouteFW);
};


//only one slave per output port

////binding function that instantiates as many slave ports as required
//void C_WrapperSlaveFifoToNoC::bind(sc_export<ac_tlm_transport_if> & SlavePort)
//{
//	//instanciate a master port and connect it to SlavePort
//	sc_port<ac_tlm_transport_if> * port = new sc_port<ac_tlm_transport_if>;
//	SlaveOuts[SlaveOuts.size()]=port;
//	port->bind(SlavePort);
//}


//The slave wrapper thread that deals with input NoCFlit to send a TLM request to slave
//and re-emmit Backward NoCFlit response
void C_WrapperSlaveFifoToNoC::RouteFW()
{
	NoCFlit Flitfw;
	NoCFlit Flitbw;
	while(true)
	{

//		wait(NoCCycle);
		wait(clk.posedge_event());

//		while(!FifoIn.nb_read(Flitfw))
//		{
//			wait(NoCCycle);
//		}

		if(FifoIn.nb_read(Flitfw))
		{
			//fw flit received
			if(Flitfw.Last==true) //last flit received => forward to corresponding slave
			{
				SYSTEMC_WRAPPER_CA("last forward flit received");

				//wait(1,SC_NS);

				//ac_tlm_rsp rsp= (*SlaveOuts[Flitfw.TargetId.second])->transport(*Flitfw.req);
				ac_tlm_rsp rsp= SlaveOut->transport(*Flitfw.req);
	//					if(rsp.time>Flitfw.req->size_burst)
	//						rsp.time=rsp.time-Flitfw.req->size_burst;

				if(rsp.data!=Flitfw.req->data)
					{
					SYSTEMC_WARN("wrong rsp data on slave"<<Flitfw.TargetId.second );
					exit(-1);
					}

				if (rsp.time!=0){
					wait(rsp.time,SC_NS);
					rsp.time=0;
					wait(clk.posedge_event()); // to resync on update and not on time events (this shall not modify current time)
				}
				SYSTEMC_WRAPPER_CA("slave transport response received");

				//create response flits
				Flitbw=Flitfw;
				Flitbw.TargetId=Flitfw.SrcId;
				Flitbw.SrcId=Flitfw.TargetId;
				Flitbw.rsp=rsp;
				Flitbw.Last=false;
				Flitbw.IsFW=false;

				unsigned int FlitsToSend=0; //no header for bw messages

				if(Flitbw.req->type==READ_BURST){
					FlitsToSend=ceil(((float) Flitbw.req->size_burst)/LinkSizeInBytes); //multiple bytes
				}
				else if(Flitbw.req->type==READ){
					FlitsToSend=ceil(((float) sizeof(Flitbw.req->data))/LinkSizeInBytes); //single data
				}
				else{
					FlitsToSend=1; //simple ack (for write accesses)
				}

//				cout<<sc_time_stamp()<<"flits to send BW "<<FlitsToSend<<endl;
				for(unsigned int i =0; i<FlitsToSend; i++)
				{
					if(i==FlitsToSend-1)
						Flitbw.Last=true;

					if(!FifoOut.nb_write( Flitbw )){
						i--;
					}
					else{
						Flitbw.CMUDump();
					}




//					wait(NoCCycle);
					wait(clk.posedge_event());
				}

				SYSTEMC_WRAPPER_CA("last backward flit sent (prev cycle)");
			}
			else{ //do nothing, this is not the last flit message
				}
			//else{wait(NoCCycle);}
		}//no input flit
	}//end while true
}

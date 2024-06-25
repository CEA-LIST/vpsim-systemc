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

#include "Router.hpp"
#include <list>

using namespace vpsim;

void C_Router::AddOutMapping(T_TargetID TargetID, T_PortID OutPortID)
{
	if (OutputPorts.count(OutPortID)==0)
	{
		OutputPorts[OutPortID]= new sc_fifo_out<NoCFlit>; //new sc_port < tlm_noc_if >;
		NbOut++;
		RoundRobinStatesPerPort[OutPortID]=0;
	}
	TargetToOutPort[TargetID]=OutPortID;

}


void C_Router::AddOutPort(T_PortID OutPortID)
{
	assert(OutputPorts.count(OutPortID)==0);
	OutputPorts[OutPortID]= new sc_fifo_out<NoCFlit>;
	NbOut++;
	return ;
}

void C_Router::AddInPort(T_PortID InPortID)
{
	assert(InputPorts.count(InPortID)==0);
	InputPorts[InPortID]= new sc_fifo_in<NoCFlit>;
	NbIn++;
	return ;
}


//sc_fifo_out<NoCFlit>* C_Router::AddOutPort()
//{
//	sc_fifo_out<NoCFlit>* OutpuFifoPort=new sc_fifo_out<NoCFlit>;
//
//	assert(OutputPorts.count(NbOut)==0);
//
//	OutputPorts[NbOut]= OutpuFifoPort;
//	NbOut++;
//	return OutpuFifoPort;
//}
//
//sc_fifo_in<NoCFlit>* C_Router::AddInPort()
//{
//	sc_fifo_in<NoCFlit>* InputFifoPort=new sc_fifo_in<NoCFlit>;
//
//	assert(InputPorts.count(NbIn)==0);
//
//	InputPorts[NbIn]= InputFifoPort;
//	NbIn++;
//	return InputFifoPort;
//}


//obsolete?
/*
void C_Router::AddInMapping(unsigned int SrcID)
{
		//SYSTEMC_INFO("AddInMapping "<<SrcID);

		InputPorts[NbIn]= new sc_fifo_in<NoCFlit>; //sc_export < tlm_noc_if >;

//		InputPorts[NbIn]->bind(*this);
//		InputFifos[NbIn]= new std::queue<NoCFlit>();

		//useless
		SrcRouterIDToInputPortNum[SrcID]=NbIn;
		InputPortNumToSrcRouterID[NbIn]=SrcID;

		NbIn++;
}
*/
//search for the request with the highest priority in a map of port to request
//the request is writtent in input NF while the result of the search is returned as a bool
bool C_Router::RoundRobinSearch (map<T_PortID, NoCFlit> const & RequestPerInputPort, unsigned int RoundRobinStateCounter, unsigned int NbInputPort, NoCFlit & NF)
{
	//apply RoundRobin priority => i.e search for the input port with the highest prio
	for(unsigned int i = 0; i<NbInputPort; i++)
	{
		unsigned int InputPortIDWithMaxPrio=(i + RoundRobinStateCounter)%NbInputPort;

		if(RequestPerInputPort.count(InputPortIDWithMaxPrio)>0) //we found a request to send which has the highest priority
		{
			NF=RequestPerInputPort.at(InputPortIDWithMaxPrio); //[] discards qualifiers because off auto insertion
			return true;
		}
	}
	return false;
}

void C_Router::DoRoute()
{
//	map<T_RouterID, NoCFlit> RequestsBeeingRouted; //request accessed by source id (represents the router internal buffer)
//
//	//request structs per output port (to implement an efficient round robin on the requests)
//	map<T_RouterID, NoCFlit> RequestsBeeingRoutedLocalSlave;
//	map<T_RouterID, NoCFlit> RequestsBeeingRoutedLocalMaster;
//	map<T_PortID,map<T_RouterID, NoCFlit> >  RequestsBeeingRoutedPerOutputPort;

	//list<NoCFlit> RequestOrderedByPriority;

//	//there is no imput port numbering so we add them one by one
//	//we can safely add the local and slave master InputPort in the
//	//InpuPorts
//	if(MasterInputPort!=NULL)
//	{
//		InputPorts[NbIn]=MasterInputPort;
//		NbIn++;
//	}
//	if(SlaveInputPort!=NULL){
//		InputPorts[NbIn]=SlaveInputPort;
//		NbIn++;
//	}


	//warning output ports can be non sequential...
	//OutputPorts[OutputPorts.size()]=MasterOutputPort;
	//OutputPorts[OutputPorts.size()]=SlaveOutputPort;

	//while(true)
	{
		SYSTEMC_DEBUG_ROUTER("begin routing step");

		//1- read input ports once for every input port if there is no
		// request from that port in the internal buffer
		 map<unsigned int ,sc_fifo_in<NoCFlit> *>::iterator IT1;
		 for(IT1=InputPorts.begin(); IT1!=InputPorts.end(); IT1++)
		 {
			unsigned int InputPortId = IT1->first;
		 	if(RequestsBeeingRouted.count(InputPortId)==0) // no request from that port in the buffer
		 	{
		 		//read the input fifo
		 		NoCFlit TmpNF;
		 		if(IT1->second!=NULL && IT1->second->nb_read(TmpNF)) //assert that port exists (could occur if no local master or slave => TODO check if still necessary)
		 		{	//there was an element in the fifo
		 			TmpNF.CurrentInputPortID=InputPortId;//Update the current input port id for future sorting
		 			RequestsBeeingRouted[InputPortId]= TmpNF;
		 			SYSTEMC_DEBUG_ROUTER("add request "<<TmpNF<<" to internal buffer for input port"<<InputPortId);

		 			//stats only
		 			if(TmpNF.IsFW)
		 				RoutedFlitsFW++;
		 			else{
		 				RoutedFlitsBW++;
// 		 				if(TmpNF.rsp.data!=TmpNF.req->data)
// 		 					SYSTEMC_ERROR("wrong rsp data ");
		 			}

					T_RouterID ReqOutPort =TargetToOutPort[TmpNF.TargetId]; //find the port to use for this TargetID
					RequestsBeeingRoutedPerOutputPort[ReqOutPort][InputPortId] = TmpNF;
		 		}
		 	}
		 }

		 //2- for every output port send the request with highest priority

		 //2-a for standard ports
		 map<T_PortID,map<T_PortID, NoCFlit> >::iterator IT2;
		 for(IT2=RequestsBeeingRoutedPerOutputPort.begin(); IT2!=RequestsBeeingRoutedPerOutputPort.end(); IT2++) //advantage of stl if some ports are unused => no loss of performance
		 {
			T_PortID OutputPortID = IT2->first;
			NoCFlit TmpNF;

			if(OutputPorts[OutputPortID]->num_free()>0) //avoid useless search if output fifo is full (performance only)
			{
				if(RoundRobinSearch(IT2->second,RoundRobinStatesPerPort[OutputPortID],NbIn,TmpNF))
				{
					if( OutputPorts[OutputPortID]->nb_write(TmpNF)){ //the request was correctly sent
						// the request should no longer be kept in any internal buffer
						RequestsBeeingRouted.erase(TmpNF.CurrentInputPortID);
						IT2->second.erase(TmpNF.CurrentInputPortID);

						SYSTEMC_DEBUG_ROUTER("request "<<TmpNF<<"from InputPortID "<<TmpNF.CurrentInputPortID<<" was sent to OutputPortID "<<OutputPortID);

						//update the RoundRobin pointer for future cycles
						RoundRobinStatesPerPort[OutputPortID]=TmpNF.CurrentInputPortID+1;
					}
					else
					{
						SYSTEMC_ERROR("impossible to write on fifo which has free slots");
					}
				}
				else{
						//cout<<this->name()<<" no flit found to route to "<<OutputPortID<<", nothing sent";
						//cout<<" whereas there are "<<IT2->second.size()<<" request targetting this port"<<endl;
				}
			}
			else
			{
				//cout<<this->name()<<" output fifo on port "<<OutputPortID<<" is full"<<endl;
			}
		 }
		//clean up the temporary structs


		SYSTEMC_DEBUG_ROUTER("end routing step"<<endl);

		//wait();
		//wait(NoCCycle);
	}

};




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

#ifndef ROUTER_HPP
#define ROUTER_HPP

#include <systemc>
#include "NoCBasicTypes.hpp"
#include <map>
#include <queue>

namespace vpsim
{

using namespace std;

class C_Router: public sc_module /*, public tlm_noc_if*/
{
	unsigned int Id;
	unsigned int NbOut;
	unsigned int NbIn;
	unsigned int InputFifoSize;

	map<T_TargetID, T_PortID > TargetToOutPort;


	//unsigned int RoundRobinState;

//	//we keep a round robin routing state per output port to avoid starvation
	map<T_PortID, unsigned int> RoundRobinStatesPerPort ;
	unsigned int RoundRobinStateLocalSlave;
	unsigned int RoundRobinStateLocalMaster;

public:
	sc_in_clk clk;
	//stats
	unsigned int RoutedFlitsFW;
	unsigned int RoutedFlitsBW;

	map<T_PortID,sc_fifo_in<NoCFlit>*> InputPorts; //input ports are numbered by src router id
	map<T_PortID ,sc_fifo_out<NoCFlit>*> OutputPorts;

	
 	//map<T_PortID, std::queue<NoCFlit> *> InputFifos;
	
	//NoC speed parameters
	float FrequencyScaling;
	sc_time NoCCycle;
	unsigned int LinkSizeInBytes;

 	//TODO clean up
	//map<T_PortID, NoCFlit> RequestsBeeingRouted; //request accessed by source id (represents the router internal buffer)
	
	map<T_RouterID, NoCFlit> RequestsBeeingRouted; //request accessed by source id (represents the router internal buffer)

	//request structs per output port (to implement an efficient round robin on the requests)
	map<T_PortID,map<T_RouterID, NoCFlit> >  RequestsBeeingRoutedPerOutputPort;

	public: 
		C_Router(sc_module_name name, unsigned int _Id, unsigned int _LinkSizeInBytes, float _FrequencyScaling , bool _NoTiming=false, unsigned int _InputFifoSize=1):
			sc_module(name),clk("clk")
		{
			FrequencyScaling=_FrequencyScaling;
			LinkSizeInBytes=_LinkSizeInBytes;
			Id=_Id;
			NbOut=0;
			NbIn=0;
			InputFifoSize=_InputFifoSize;
			RoutedFlitsFW=0;
			RoutedFlitsBW=0;
			RoundRobinStateLocalSlave=0;
			RoundRobinStateLocalMaster=0;

			if(_NoTiming)
				NoCCycle=SC_ZERO_TIME;
			else
				NoCCycle=sc_time(1.0/_FrequencyScaling,SC_NS);

			//RoundRobinState=0;

			//SC_THREAD(DoRoute);
			SC_METHOD(DoRoute);
			sensitive<<clk.pos();

			//SYSTEMC_INFO("construction done");
		}
		SC_HAS_PROCESS(C_Router);

		~C_Router()
		{
			map<unsigned int ,sc_fifo_in<NoCFlit>*>::iterator IT1;
			for(IT1=InputPorts.begin(); IT1!=InputPorts.end(); IT1++)
				delete IT1->second;
			InputPorts.clear();

			map<unsigned int ,sc_fifo_out<NoCFlit>*>::iterator IT2;
			for(IT2=OutputPorts.begin(); IT2!=OutputPorts.end(); IT2++)
				delete IT2->second;
			OutputPorts.clear();


		}

		//routing info
		void AddOutMapping(T_RouterID TargetID, T_PortID OutPortID);

		//Needed as there is only one transport for all in ports 
		//(need to access to exact link to manage contention)
		//void AddInMapping(unsigned int SrcID, unsigned int InPortID);

		//the SrcID is used as input identification (no need for port here: one origin == one link == one port)
		void AddInPort(T_PortID OutPortID);
		void AddOutPort(T_PortID OutPortID);

		bool RoundRobinSearch (map<T_PortID, NoCFlit> const & RequestPerInputPort, unsigned int RoundRobinStateCounter, unsigned int NbInputPort, NoCFlit & NF);

		//void before_end_of_elaboration(); //TODO delete
		
		void DoRoute();
};


//extern unsigned int GlobalRoundRobinState;
//extern unsigned int GlobalNbIn;

bool RoundRobinPriority ( const NoCFlit& nf1, const NoCFlit& nf2);

};//namespace vpsim


#endif //ROUTER_HPP

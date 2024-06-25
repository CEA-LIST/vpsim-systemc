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

#ifndef TRAFFICGEN_HPP
#define TRAFFICGEN_HPP

#include "NoCBasicTypes.hpp"
#include <list>

#include <iostream> // to use std::fixed
#include <iomanip> //to manage float formatting

#ifndef CSV_SEP
#define CSV_SEP "\t"
#endif

#define FLOAT_FORMAT std::fixed<<std::setw(8)<<std::setprecision(2)<<std::setfill(' ')

class C_CABATrafficGen:public sc_module
{
		private:
			CycleCount InterReqLatency;
			T_TargetID SourceID;
			std::list<T_TargetID> * ValidTargets;

			//threads
			void Gen();

		public:
			sc_in_clk clk;
			sc_fifo_out<NoCFlit> FifoOut; //to send response flits on the NoC

			C_CABATrafficGen(sc_module_name name, CycleCount InterReqLatency);
			SC_HAS_PROCESS(C_CABATrafficGen);

			void SetValidTargets(std::list<T_TargetID> * ValidTargets_);
			void SetSourceID(T_TargetID SourceID_);
			T_TargetID GetRandomTargetID();


};

class C_CABATrafficCons:public sc_module
{
		private:

			CycleCount TotalLatency;
			unsigned int FlitsCount;


			static CycleCount TotalLatencyAll;
			static unsigned int FlitsCountAll;

			//threads
			void Cons();

		public:
			sc_in_clk clk;
			sc_fifo_in<NoCFlit> FifoIn; //to receive request flits on the NoC

			C_CABATrafficCons(sc_module_name name);
			SC_HAS_PROCESS(C_CABATrafficCons);

			void DisplayLoadDelayCurveAll();

			
};

class C_CABABiDirTraffic: public sc_module
{
	C_CABATrafficGen* Gen;
	C_CABATrafficCons* Cons;

	public:
	sc_in_clk clk;
	sc_fifo_out<NoCFlit> FifoOut;
	sc_fifo_in<NoCFlit> FifoIn;
	C_CABABiDirTraffic(sc_module_name name_, CycleCount InterReqLatency):sc_module(name_)
	{
		Gen = new C_CABATrafficGen("Gen",InterReqLatency);
		Cons = new C_CABATrafficCons("Cons");

		//bindings
		// clk(Gen->clk);
		// clk(Cons->clk);

		Gen->clk(clk);
		Cons->clk(clk);

		
		//cout<<"FifoOut "<<& FifoOut<<endl;
		//cout<<"FifoOut "<<& FifoIn<<endl;
		Gen->FifoOut(FifoOut);
		Cons->FifoIn(FifoIn);
	};

	~C_CABABiDirTraffic()
	{
		delete Gen;
		delete Cons;
	}

	void SetValidTargets(std::list<T_TargetID> * ValidTargets_)
	{
		Gen->SetValidTargets(ValidTargets_);
	};
			
	void SetSourceID(T_TargetID SourceID_)
	{
		Gen->SetSourceID(SourceID_);
	};

	void DisplayLoadDelayCurveAll()
	{
		Cons->DisplayLoadDelayCurveAll();
	};

};

#endif //TRAFFICGEN_HPP

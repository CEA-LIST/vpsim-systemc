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

#ifndef NOCCYCLEACCURATE
#define NOCCYCLEACCURATE

#include "NoCBase.hpp"
#include "Router.hpp"
#include <map>
#include "WrapperNoC.hpp"
#include "SyncFifo.hpp"

namespace vpsim
{

class C_NoCCycleAccurate: public sc_module
{
	C_NoCBase* Topo; //!< the topology of the network

	map<T_RouterID,C_Router*> Routers;
	map< std::pair<T_RouterID,T_RouterID>,sc_fifo<NoCFlit>* > Fifos;
	unsigned int FifoSize;
	bool NoCCycleAccurateBeforeElaborationCalled;
	list< C_WrapperMasterNoCToFifo * > WrapperMasterNoCToFifos; //to store master wrappers
	list< C_WrapperSlaveFifoToNoC * > WrapperSlaveFifoToNoCs; //to store master wrappers
	list< sc_fifo<NoCFlit>* > LocalLinksFifo;

	public:
	C_NoCCycleAccurate(sc_module_name name, C_NoCBase* Topo_);
	~C_NoCCycleAccurate();

	void before_end_of_elaboration();

};
};// namespace vpsim

#endif  //NOCCYCLEACCURATE

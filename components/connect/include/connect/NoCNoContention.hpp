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

#ifndef NOCNOCONTENTION_HPP
#define NOCNOCONTENTION_HPP

#include <systemc>
#include <tlm>
#include "NoCTLMBase.hpp"

namespace vpsim
{

class C_NoCNoContention: public C_NoCTLMBase
{
	C_NoCBase* Topo;
	CycleCount ** HopCount; //HopCount[SrcRouterID][DestRouterID]
	std::pair<T_RouterID,T_LinkID> ** Next; // pair [RouterCount][SlaveCount] //temporary

	bool NoCNoContentionBeforeElaborationCalled;

	public:

	C_NoCNoContention(sc_module_name name_,C_NoCBase* Topo_);

	~C_NoCNoContention();

	void before_end_of_elaboration();

	protected:
	//ac_tlm_rsp transport(ac_tlm_req const &  req);
	void b_transport( tlm::tlm_generic_payload& trans, sc_time& delay );

};

};// namespace vpsim

#endif //NOCNOCONTENTION_HPP

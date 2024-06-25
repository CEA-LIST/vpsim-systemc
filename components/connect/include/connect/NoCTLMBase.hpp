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

#ifndef NOCTLMBASE_HPP
#define NOCTLMBASE_HPP

#include <map>
#include "NoCBase.hpp"
#include "WrapperNoC.hpp"
//#include "ac_tlm_protocol.H"

namespace vpsim
{

class C_NoCTLMBase: public sc_module, public ac_tlm_transport_if
{
	public:
	//Construct
	C_NoCTLMBase (sc_module_name name_, C_NoCBase* Topo_ ): sc_module(name_) {
		Topo=Topo_;
		SYSTEMC_INFO("Constructor called");
	}
	virtual C_NoCTLMBase ( ) =0;

	//TLM wrapping features
	protected:
	std::map<T_RouterID, std::map<T_SlavePortID,sc_port<ac_tlm_transport_if>* > > OutPorts; //todo replace by vectors

	list< C_BasicWrapperMasterNoC * > BasicWrapperMasterNoCs;
	void DoPortInstanciationAndBinding();
	void DoPortDeallocation();

};

};//namespace vpsim



#endif //NOCTLMBASE_HPP

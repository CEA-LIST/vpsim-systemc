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

#include "NoCTLMBase.hpp"

using namespace vpsim;

void C_NoCTLMBase::DoPortInstanciationAndBinding()
	{

	}

	void C_NoCTLMBase::DoPortDeallocation()
	{
		//SYSTEMC_INFO("DoPortDeallocation Called");
		//delete master wrappers
		list< C_BasicWrapperMasterNoC * >::iterator IT;
		for(IT=BasicWrapperMasterNoCs.begin();IT!=BasicWrapperMasterNoCs.end(); IT++)
		{
			delete (*IT);
		}
		BasicWrapperMasterNoCs.clear();

		//delete slave ports
		std::map<T_RouterID, std::map<T_SlavePortID,sc_port<ac_tlm_transport_if>* > >::iterator IT2;
		for(IT2=OutPorts.begin(); IT2!=OutPorts.end(); IT2++)
		{
			std::map<T_SlavePortID,sc_port<ac_tlm_transport_if>* >::iterator IT3;
			for(IT3=IT2->second.begin(); IT3!=IT2->second.end(); IT3++)
			{
				delete IT3->second;
			}
			IT2->second.clear();
		}
		OutPorts.clear();

	}

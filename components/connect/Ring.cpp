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

#include "Ring.hpp"

using namespace vpsim;

C_Ring::C_Ring(sc_module_name name, unsigned int Size_,bool Bidirectional_, bool debug):C_NoC(name)
{

	Size= Size_;
	Bidirectional=Bidirectional_;


	// unidirectional n = Size
	// R0 -> R1 -> .... -> Rn-1 -> R0

	//bidirectional n = Size
	// R0 <-> R1 <-> .... <-> Rn-1 <-> R0

	if(Size==1)
	{
		AddRouter(0); //necessary only for unconnected router
		return;
	}

	for(unsigned int RouterId=0;RouterId<Size;RouterId++)
	{
		unsigned NextRouterID=(RouterId+1)%Size;
		unsigned PrevRouterID=(RouterId==0)?Size-1:RouterId-1;

		//build links
		//out ports are numbered clockwise starting from north
		//							local masters
		//								^
		//								|
		//	   Router -1  (port1)<--- Router ----> Router +1 (port0)
		//								|
		//								v
		//							local slaves


		//AddLink(RouterId,0,NextRouterID);
		AddLink(RouterId,NextRouterID,debug);
		if (Bidirectional){
			//AddLink(RouterId,1,PrevRouterID);
			AddLink(RouterId,PrevRouterID,debug);
		}

	}
}//end constructor

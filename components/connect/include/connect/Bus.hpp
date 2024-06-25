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

#ifndef BUS_HPP
#define BUS_HPP

#include "NoC.hpp"

namespace vpsim
{

//!
//! C_Bus provides functions to implement a bus based Topology of
//!
class C_Bus:public C_NoC
{
public:
	//!
	//! Constructor of C_Mesh
	//! @param [in] name : unique sc_module name of the C_Mesh instance (used for debugging and statistics)
	//!
	C_Bus(sc_module_name name);


	//!
	//!Populates the routing tables for all routers
	//!
	void BuildRouting();
};

};//namespace vpsim

#endif

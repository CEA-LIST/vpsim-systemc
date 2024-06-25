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

#ifndef MESH_HPP
#define MESH_HPP

#include "NoC.hpp"

namespace vpsim
{

//!
//! C_Mesh provides an extension to C_NoC to implement a Noc topology
//! composed of a regular mesh of Routers in a rectangular X*Y grid
//!
class C_Mesh:public C_NoC
{
		private:
			unsigned int SizeX; //!< Number of router in the X axis
			unsigned int SizeY; //!< Number of router in the Y axis

			//!
			//! builds a routing that sends request in the X axis then the Y axis
			//!
			void BuildRoutingXY();

		public:

			//!
			//! Constructor of C_Mesh
			//! @param [in] name : unique sc_module name of the C_Mesh instance (used for debugging and statistics)
			//! @param [in] _SizeX : the number of router in the X axis
			//! @param [in] _SizeX : the number of router in the Y axis
			//!
			C_Mesh(sc_module_name name, unsigned int _SizeX, unsigned int _SizeY);

			//!
			//! Enumberation of available routing modes for C_Mesh
			//!
			enum E_RoutingMode {
				Generic, //!< Stands for the base C_NoC routing
				XY //!< Stands for the XY routing defined by C_Mesh
			};

			//!
			//! Populates the routing tables for all routers
			//! @param [in] RMode : the routing method to use
			//!
			void BuildRouting(E_RoutingMode RMode);
};

};//namespace vpsim

#endif

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

#ifndef RING_HPP
#define RING_HPP

#include "NoC.hpp"

namespace vpsim{
class C_Ring:public C_NoC
{
		private:
			unsigned int Size;
			bool  Bidirectional;

			//builds a routing that sends request in the X axis then the Y axis
			void BuildRoutingXY();

		public:
			C_Ring(sc_module_name name, unsigned int Size,bool Bidirectional=true, bool debug=true);

};
}; //end namespace vpsim
#endif

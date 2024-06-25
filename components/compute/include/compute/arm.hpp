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

#ifndef ARM_HPP_
#define ARM_HPP_

#include "issWrapper.hpp"
#include "global.hpp"
#include "issFinder.hpp"

namespace vpsim{


	class arm : public IssWrapper
	{
	public:
		//---------------------------------------------------
		//Constructor
		arm (sc_module_name name, std::string model, const IssFinder& iss, uint32_t id, uint32_t quantum, bool is_gdb, bool simflag, uint64_t init_pc):
			IssWrapper (name, id, iss.getIssLibPath("arm-softmmu"),
					model.c_str() /*"arm1136"*/ /*arm926*/ /*"cortex-r5f"*/, quantum, is_gdb, B64, simflag, init_pc)

		{}
	};

} //end namespace vpsim

#endif

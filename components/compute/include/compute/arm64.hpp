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

#ifndef ARM64_HPP_
#define ARM64_HPP_

#include "issWrapper.hpp"
#include "global.hpp"
#include "issFinder.hpp"

namespace vpsim{


	class arm64 : public IssWrapper
	{
	public:
		//---------------------------------------------------
		//Constructor
		arm64 (sc_module_name name, std::string model, const IssFinder& iss, uint32_t id, uint32_t quantum, bool is_gdb, bool simflag, uint64_t init_pc, bool use_log, const char* logfile):
			IssWrapper (name, id, iss.getIssLibPath("aarch64-softmmu"),
					model.c_str(), quantum, is_gdb, B64, simflag, init_pc, use_log, logfile)

		{}
	};

} //end namespace vpsim


#endif

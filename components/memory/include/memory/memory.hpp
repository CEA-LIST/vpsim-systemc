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

#ifndef MEMORY_HPP_
#define MEMORY_HPP_

#include "global.hpp"
#include "TargetIf.hpp"
#include "elfloader.hpp"

namespace vpsim
{
	class memory : public sc_module, public TargetIf < unsigned char >, public elfloader {

		typedef memory this_type;

	private:

		//Local variables
		uint32_t mWordLengthInByte;

	public:

		sc_time ReadLatency;
		sc_time WriteLatency;
		memory(sc_module_name Name, uint64_t Size);
		memory(sc_module_name Name, uint64_t Size, bool ByteEnable, bool DmiEnable);
		void Init ( );

		SC_HAS_PROCESS(memory);

		~memory();

		//Main functions
		tlm::tlm_response_status read(payload_t & payload, sc_time & delay);
		tlm::tlm_response_status write(payload_t & payload, sc_time & delay);

		//!Debug function
		//! @param[in] StartAddress address in the memory address space from which dump starts
		//! @param[in] EndAddress address in the memory address space at which dump ends
		void Dump(uint64_t StartAddress, uint64_t EndAddress);

		void
		loadElfFile ( const string name, bool debug=false){
			load_elf_file(name,getBaseAddress(),getSize(),debug);
		}

		void loadBlob(const string filename, const uint64_t off);

		void setChannelWidth(uint32_t bytes) { mWordLengthInByte=bytes; }
	};

}

#endif /* MEMORY_HPP_ */

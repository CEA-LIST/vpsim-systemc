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

#ifndef ELFLOADER_HPP_
#define ELFLOADER_HPP_

#include "global.hpp"
#include <elfio/elfio.hpp>
#include <elfio/elfio_dump.hpp>

using namespace ELFIO;

namespace vpsim
{
	class elfloader
	{
	private:

		elfio elf_struct;
		char * elf_memory_ptr;
		uint64_t elf_memory_size;

	public:

		void
		elfloader_init ( char * ptr_mem, uint64_t size_ );

		void
		dump_elf_file ();

		void
		load_elf_file ( const string name, uint64_t base_addr, uint64_t size, bool debug = false );

		void
		print_elf_segments_info ( );

		void
		print_elf_sections_info ( );

		void
		print_elf_properties ( );
	};

}

#endif /* ELFLOADER_HPP_ */

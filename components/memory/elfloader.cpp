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

#include "elfloader.hpp"
using namespace ELFIO;

namespace vpsim
{

void
elfloader::elfloader_init ( char * ptr_mem, uint64_t size_ )
{
	elf_memory_ptr = ptr_mem;
	elf_memory_size = size_;
}

void
elfloader::dump_elf_file ()
{
	ELFIO::dump::header         ( std::cout, elf_struct );
    ELFIO::dump::section_headers( std::cout, elf_struct );
    ELFIO::dump::segment_headers( std::cout, elf_struct );
    ELFIO::dump::symbol_tables  ( std::cout, elf_struct );
    ELFIO::dump::notes          ( std::cout, elf_struct );
    ELFIO::dump::dynamic_tags   ( std::cout, elf_struct );
    ELFIO::dump::section_datas  ( std::cout, elf_struct );
    ELFIO::dump::segment_datas  ( std::cout, elf_struct );
}

void
elfloader::load_elf_file ( const string name, uint64_t base_addr , uint64_t size, bool debug)
{
	//------------------------------------------------------------------------------
	//Read the ELF file
    std::ifstream stream;
    stream.open( name.c_str(), std::ios::in | std::ios::binary );
    if ( !stream ) {
    	std::cerr << "Can't find the given ELF file " << name << std::endl;
    	throw(0);
    }

    if ( !elf_struct.load(stream) ) {
    	std::cerr << "Can't process the given ELF file " << name << std::endl;
    	throw(0);
    }

    //------------------------------------------------------------------------------
    //Check that the ELF file content can fit into the memory
    stream.seekg ( 0, ios::end );
    uint64_t uiLength = stream.tellg ();

    /*if ( uiLength > elf_memory_size ) {
    	std::cerr << "Size of ELF file is higher than the allocated memory space (size is " << uiLength << " bytes)."<< std::endl;
    	throw(0);
    }*/

//    if ( (base_addr+uiLength) > elf_memory_size ) {
//    	std::cerr << "Size of ELF file can be contained in the memory but not starting from address 0x"<<hex<<base_addr<<dec<<std::endl;
//    	throw(0);
//    }

    //------------------------------------------------------------------------------
    //Load ELF file
    uint32_t sec_num = elf_struct.sections.size();
    for ( uint32_t i=0; i<sec_num ; i++ )
    {
    	//Use base address as an offset for the whole elf file, let the position of the section
    	//be determined by elf section information
    	uint64_t pos =  elf_struct.sections[i]->get_address()- base_addr;

    	if (elf_struct.sections[i]->get_address() < base_addr)
    		continue;

    	if (elf_struct.sections[i]->get_address()+elf_struct.sections[i]->get_size()-1 >= base_addr+size)
    		continue;

    	if ( debug ) {
    		cout << endl;
			cout << "Loading" << elf_struct.sections[i]->get_name() << " at ROM position " << std::hex << pos << std::dec << std::endl;
			cout << "\t elf specified address is " << std::hex << elf_struct.sections[i]->get_address() << std::dec << endl;
    	}

    	if ( elf_struct.sections[i]->get_flags() & SHF_ALLOC && elf_struct.sections[i]->get_data() != nullptr) {
    		memcpy ( (char*)(elf_memory_ptr+pos), elf_struct.sections[i]->get_data(), elf_struct.sections[i]->get_size() );
    		if ( debug ) cout << "\t " << std::hex<<elf_struct.sections[i]->get_size() << std::dec << "Data loaded." << std::endl;
    	}
    	else{
    		if( debug ) cout << "\t " << "Section is not loaded." << std::endl;
    	}
    }
}

void
elfloader::print_elf_properties ( )
{
	// Print ELF file properties
	std::cout << "ELF file class    : ";
	if ( elf_struct.get_class() == ELFCLASS32 )
		std::cout << "ELF32" << std::endl;
	else
		std::cout << "ELF64" << std::endl;

	std::cout << "ELF file encoding : ";
	if ( elf_struct.get_encoding() == ELFDATA2LSB )
		std::cout << "Little endian" << std::endl;
	else
		std::cout << "Big endian" << std::endl;

	// Print ELF file sections info
	Elf_Half sec_num = elf_struct.sections.size();
	std::cout << "Number of sections: " << sec_num << std::endl;
	for ( int i = 0; i < sec_num; ++i ) {
		section* psec = elf_struct.sections[i];
		std::cout << "  [" << i << "] "
				  << psec->get_name()
				  << "\t"
				  << psec->get_size()
				  << std::endl;
		// Access to section's data
		// const char* p = elf_struct.sections[i]->get_data()
	}
}

void
elfloader::print_elf_segments_info ( )
{
    Elf_Half seg_num = elf_struct.segments.size();
	std::cout << "Number of segments: " << seg_num << std::endl;

	for ( int i = 0; i < seg_num; ++i ) {
		const segment* pseg = elf_struct.segments[i];
		std::cout << "  [" << i << "] 0x" << std::hex
				  << pseg->get_flags()
				  << "\t0x"
				  << pseg->get_virtual_address()
				  << "\t0x"
				  << pseg->get_file_size()
				  << "\t0x"
				  << pseg->get_memory_size()
				  << std::endl;
		// Access to segments's data
		// const char* p = elf_struct.segments[i]->get_data()
	}
}

void
elfloader::print_elf_sections_info ( )
{
    Elf_Half sec_num = elf_struct.sections.size();
    std::cout << "Number of sections: " << sec_num << std::endl;

    for ( int i = 0; i < sec_num; ++i ) {
		section* psec = elf_struct.sections[i];
		// Check section type
		if ( psec->get_type() == SHT_SYMTAB ) {
			const symbol_section_accessor symbols( elf_struct, psec );
			for ( unsigned int j = 0; j < symbols.get_symbols_num(); ++j ) {
				std::string   name;
				Elf64_Addr    value;
				Elf_Xword     size;
				unsigned char bind;
				unsigned char type;
				Elf_Half      section_index;
				unsigned char other;

				// Read symbol properties
				symbols.get_symbol( j, name, value, size, bind,
									   type, section_index, other );
				std::cout << j << " " << name << " " << value << std::endl;
			}
		}
	}
}

}

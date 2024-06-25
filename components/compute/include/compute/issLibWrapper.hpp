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

#ifndef _ISS_LIB_WRAPPER_H
#define _ISS_LIB_WRAPPER_H

#include <string>
#include <dlfcn.h>
#include <cassert>

#include "iss_plugin.h"
#include "global.hpp"


namespace vpsim{

	extern sem_t VPSIM_LOCK;

	class IssLibWrapper
	{
	private:
		void *m_lib_hdl;
		struct iss_context_t ctx;
		void * wrapper;
		std::string NAME;
		uint32_t CPU_ID;
		bool is_gdb;
		string lib_path_lib;
		uint64_t* mBuffer;

	public:

		IssLibWrapper ( std::string name, std::string lib_path, void * w, uint32_t cpu_id, bool is_gdb );

		~IssLibWrapper ( );

		void init ( int num_cpu, std::string cpu_model, uint32_t instr_quantum, uint64_t init_pc );

		void *get_symbol(const char* sym);

		void run ( );

		void map_dmi ( string name, uint64_t base, uint32_t size, void *data );

		void create_rom ( string name, uint64_t base, uint32_t size, void *data );

		void linux_mem_init ( uint32_t ncores, uint32_t size );

		//for arm
		void load_elf ( uint64_t ram_size, char *kernel_filename, char *kernel_cmdline, char *initrd_filename );

		void update_irq ( uint64_t val, uint32_t irq_idx );

		void tb_cache_flush ( );

		void do_cpu_reset ( );

		bool is_application_done ( );

		bool reset_application_done ( );

		//Independent functions
		static void iss_rwsync ( void * class_inst_ptr, uint64_t addr, int rw, uint64_t ltime, int num_bytes, uint64_t value );

		static void iss_fsync ( void * class_inst_ptr, uint64_t addr, uint64_t cnt, uint32_t instr_quantum, uint32_t );

		static void iss_wait_for_interrupt ( void * class_inst_ptr );
		static void iss_forcesync ( void * class_inst_ptr,uint64_t nosyncinstr );
		static uint64_t iss_get_time ( void * class_inst_ptr,uint64_t nosync );
		static void iss_interrupt_me ( void * class_inst_ptr, uint32_t val, uint32_t line );
		static void iss_request_timeout( void * class_inst_ptr, uint64_t ticks, void(*cb)(void*),void*iss_provider,uint64_t nosync );

		static void iss_sstop ( void * class_inst_ptr );

		static void iss_atomic_set_flag ( void * class_inst_ptr );

		static void iss_atomic_reset_flag ( void * class_inst_ptr );

		static uint64_t * iss_get_dotlm(void *class_inst_ptr, uint64_t base, uint64_t end, bool isFetch);

		uint64_t *getResultBuffer() ;




	};
}//end vpsim namespace



#endif

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

#ifndef ISS_PLUGIN_H_
#define ISS_PLUGIN_H_


#ifdef __cplusplus
extern "C" {
#endif

//---------------------------------------------------------------------
//Includes
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

//Typedef
typedef bool run_func ( void );
typedef void application_done_func ( bool );
typedef bool application_is_done_func ( void );
typedef bool application_reset_done_func ( void );
typedef void clean_func ( void );
typedef void map_dmi_func ( const char *, uint64_t, uint32_t, void * );
typedef void linux_mem_init_func ( uint32_t, uint32_t );
typedef uint64_t* get_result_buffer_func (  );

typedef void load_elf_func ( uint64_t, char *, char *, char * );
typedef void update_irq_func ( uint64_t, uint32_t );
typedef void tb_cache_flush_func ( void );
typedef void do_cpu_reset_func ( void );
typedef uint64_t vpsim_get_time_func ( void *,uint64_t );
typedef vpsim_get_time_func * vpsim_get_time_func_ptr;

typedef void vpsim_interrupt_me_func ( void * , uint32_t, uint32_t);
typedef vpsim_interrupt_me_func* vpsim_interrupt_me_func_ptr;

typedef void vpsim_rwsync_func ( void *, uint64_t, int, uint64_t, int, uint64_t );
typedef void vpsim_fsync_func ( void *, uint64_t, uint64_t, uint32_t, uint32_t );
typedef void vpsim_count_func ( void *, uint64_t );
typedef void vpsim_force_sync_func ( void *,uint64_t );
typedef void vpsim_stop_func ( void * );
typedef void vpsim_atomic_set_flag_func ( void * );
typedef void vpsim_atomic_reset_flag_func ( void * );
typedef void vpsim_wait_for_interrupt_func ( void * );

typedef uint64_t* vpsim_get_dotlm_func ( void *, uint64_t, uint64_t, bool );

typedef run_func * run_func_ptr;
typedef application_done_func * application_done_func_ptr;
typedef application_is_done_func * application_is_done_func_ptr;
typedef application_reset_done_func * application_reset_done_func_ptr;
typedef clean_func * clean_func_ptr;
typedef map_dmi_func * map_dmi_func_ptr;
typedef linux_mem_init_func * linux_mem_init_func_ptr;
typedef get_result_buffer_func* get_result_buffer_func_ptr;

typedef load_elf_func * load_elf_func_ptr;
typedef update_irq_func * update_irq_func_ptr;
typedef tb_cache_flush_func * tb_cache_flush_func_ptr;
typedef do_cpu_reset_func * do_cpu_reset_func_ptr;

typedef vpsim_rwsync_func * vpsim_rwsync_func_ptr;
typedef vpsim_fsync_func * vpsim_fsync_func_ptr;
typedef vpsim_count_func * vpsim_count_func_ptr;
typedef vpsim_force_sync_func * vpsim_force_sync_func_ptr;
typedef vpsim_stop_func * vpsim_stop_func_ptr;
typedef vpsim_atomic_set_flag_func * vpsim_atomic_set_flag_func_ptr;
typedef vpsim_atomic_reset_flag_func * vpsim_atomic_reset_flag_func_ptr;
typedef vpsim_wait_for_interrupt_func * vpsim_wait_for_interrupt_func_ptr;

typedef vpsim_get_dotlm_func * vpsim_get_dotlm_func_ptr;
typedef void vpsim_request_timeout_func ( void *, uint64_t,void(*)(void*),void*,uint64_t  );
typedef vpsim_request_timeout_func* vpsim_request_timeout_func_ptr;

//Shared structures
struct iss_plugin_t
{
	run_func_ptr run;
	application_done_func_ptr done;
	application_is_done_func_ptr is_done;
	application_reset_done_func_ptr reset_done;
	clean_func_ptr clean;
	map_dmi_func_ptr map_dmi;
	map_dmi_func_ptr create_rom;
	linux_mem_init_func_ptr linux_mem_init;
	get_result_buffer_func_ptr get_result_buffer;

	load_elf_func_ptr load_elf;
	update_irq_func_ptr update_irq;
	tb_cache_flush_func_ptr tb_cache_flush;
	do_cpu_reset_func_ptr do_cpu_reset;

};

struct vpsim_plugin_t
{
	vpsim_rwsync_func_ptr rwsync;
	vpsim_fsync_func_ptr fsync;
	vpsim_force_sync_func_ptr force_sync;
	vpsim_get_time_func_ptr get_time;
	vpsim_stop_func_ptr stop;
	vpsim_atomic_set_flag_func_ptr atomic_set_flag;
	vpsim_atomic_reset_flag_func_ptr atomic_reset_flag;
	vpsim_get_dotlm_func_ptr get_dotlm;
	vpsim_interrupt_me_func_ptr interrupt_me;
	vpsim_request_timeout_func_ptr request_timeout;
	vpsim_wait_for_interrupt_func_ptr wait_for_interrupt;
};

struct iss_context_t
{
	char* cpu_model;
	uint32_t cpu_id;
	void * iss;
	void * vpsim_sim;
	struct vpsim_plugin_t vpsim_plugin;
	struct iss_plugin_t iss_plugin;

	uint32_t instr_quantum;

//#ifdef CONFIG_VPSIM_WITH_ARM
	uint32_t nb_cores;
//#endif //CONFIG_VPSIM_WITH_ARM

};


//---------------------------------------------------------------------
//ISS functions
bool run(void);

bool is_application_done(void);

void set_application_done(bool val);

void reset_application_done(void);

void iss_plugin_init(struct iss_context_t *ctx,const char* cpu_model, bool is_gdb, uint64_t start_pc);

void iss_map_dmi(const char * name, uint64_t base_address, uint32_t size, void * data);

void iss_create_rom(const char * name, uint64_t base_address, uint32_t size, void * data);

void iss_load_elf(uint64_t ram_size, char *kernel_filename, char *kernel_cmdline, char *initrd_filename);
void iss_update_irq(uint64_t val, uint32_t irq_idx);
void iss_do_cpu_reset (void);

uint32_t iss_get_cpuid(void);

//---------------------------------------------------------------------
#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* ISS_PLUGIN_H_ */ 

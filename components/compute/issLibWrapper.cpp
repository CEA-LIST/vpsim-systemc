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

#include <sstream>

#include "issLibWrapper.hpp"
#include "issWrapper.hpp"
#include "log.hpp"

#define DIAGNOSTIC_LEVEL DBG_L0

namespace vpsim {

//TODO rename m_lib_hdl
IssLibWrapper::IssLibWrapper ( string name, std::string lib_path, void * w, uint32_t cpu_id, bool is_gdb ):
	wrapper(w), NAME (name), CPU_ID(cpu_id), is_gdb(is_gdb), mBuffer(nullptr)
{
	int status;

	//We cannot initialize a processor and open a DLL in parallel.
	sem_wait ( &VPSIM_LOCK );

	//We cannot open twice the same library. We must make copies first.
	//Create new path
	lib_path_lib = lib_path + string(".") + name + "." + std::to_string(getpid());

	if(!ifstream(lib_path)){
		cerr<<"failed to find library "<<lib_path<<endl;
		throw(0);
	}

	//Generate stream for the copy command
    string copycommand = string("cp ") + lib_path + string(" ") + lib_path_lib;
    status = system ( copycommand.c_str() );
    if(status !=0){
    	LOG_GLOBAL_ERROR <<"system called returned "<<status<<", failed to execute command "<<copycommand<<std::endl;
    	std::cerr<<"system called returned "<<status<<", failed to execute command "<<copycommand<<std::endl;
    }

	//Open shared library
	m_lib_hdl = dlopen(lib_path_lib.c_str(), RTLD_LOCAL | RTLD_LAZY);

	//Test if opening is fine
	if (!m_lib_hdl)
	{
		LOG_GLOBAL_ERROR <<NAME<< ": Cannot load library " << dlerror() << endl;
		cerr <<NAME<< ": Cannot load library " << dlerror() << endl;
		exit ( EXIT_FAILURE );
	}

	printf ("iss_lib_wrapper::load DLL %s\n", lib_path_lib.c_str());

	//Release the global lock
	sem_post ( &VPSIM_LOCK );
}


IssLibWrapper::~IssLibWrapper ( ) {
	int status;

	//Free memory in DLL
	ctx.iss_plugin.clean ();

	//Close the dynamic library
	if (m_lib_hdl!=NULL) {
		//dlclose ( m_lib_hdl ); //deactivated for efficient iss valgrind
	}

	//Clean created library
	string rmcommand = string("rm -f ") + lib_path_lib;
	status = system ( rmcommand.c_str() );
    if(status !=0){
    	LOG_GLOBAL_ERROR <<"system called returned "<<status<<", failed to execute command "<<rmcommand<<std::endl;
    	std::cerr<<"system called returned "<<status<<", failed to execute command "<<rmcommand<<std::endl;
    }

	//printf("in ~iss_lib_wrapper delete created library files!\n");
}


void IssLibWrapper::iss_sstop ( void * class_inst_ptr ) {
	LOG_GLOBAL_DEBUG(dbg2) <<"iss_lib_wrapper::iss_sstop called"<<std::endl;

	IssLibWrapper *libw = (IssLibWrapper *)(class_inst_ptr);
	IssWrapper * wrap = (IssWrapper *) (libw->wrapper);
	wrap->iss_stop ( );
}


void IssLibWrapper::iss_forcesync ( void * class_inst_ptr,uint64_t nosyncinstr ) {
	LOG_GLOBAL_DEBUG(dbg2)<<"iss_lib_wrapper::iss_forcesync called"<<std::endl;

	IssLibWrapper *libw = (IssLibWrapper *)(class_inst_ptr);
	IssWrapper * wrap = (IssWrapper *) (libw->wrapper);
	wrap->iss_force_sync (nosyncinstr);
}

void IssLibWrapper::iss_wait_for_interrupt ( void * class_inst_ptr ) {
	LOG_GLOBAL_DEBUG(dbg2)<<"iss_lib_wrapper::iss_wait_for_interrupt called"<<std::endl;

	IssLibWrapper *libw = (IssLibWrapper *)(class_inst_ptr);
	IssWrapper * wrap = (IssWrapper *) (libw->wrapper);
	wrap->waitForEvent ();
}

void IssLibWrapper::iss_rwsync ( void * class_inst_ptr, uint64_t addr, int rw, uint64_t ltime, int num_bytes, uint64_t value ) {

	LOG_GLOBAL_DEBUG(dbg2)<<"iss_lib_wrapper::iss_rwsync rw="<<rw<<" with time "<< ltime<<endl;

	IssLibWrapper *libw = (IssLibWrapper *)(class_inst_ptr);
	IssWrapper * wrap = (IssWrapper *) (libw->wrapper);

	wrap->iss_rw_sync ( addr, (ACCESS_TYPE) rw, ltime, num_bytes, value );
}


void IssLibWrapper::iss_fsync ( void * class_inst_ptr, uint64_t addr, uint64_t cnt, uint32_t instr_quantum, uint32_t call ) {
	//Call function in wrapper
	LOG_GLOBAL_DEBUG(dbg2)<<"iss_lib_wrapper::iss_fsync with time "<<cnt<<endl;

	IssLibWrapper *libw = (IssLibWrapper *)(class_inst_ptr);
	IssWrapper * wrap = (IssWrapper *) (libw->wrapper);
	wrap->iss_fetch_sync ( addr, cnt, instr_quantum, call );
}

uint64_t IssLibWrapper::iss_get_time ( void * class_inst_ptr,uint64_t nosync ) {

	IssLibWrapper *libw = (IssLibWrapper *)(class_inst_ptr);
	IssWrapper * wrap = (IssWrapper *) (libw->wrapper);

	return wrap->iss_get_time ( nosync );
}

void IssLibWrapper::iss_atomic_set_flag( void * class_inst_ptr ) {
	IssLibWrapper *libw = (IssLibWrapper *)(class_inst_ptr);
	IssWrapper * wrap = (IssWrapper *) (libw->wrapper);
	wrap->iss_update_atomic_flag ( true );
}

void IssLibWrapper::iss_atomic_reset_flag( void * class_inst_ptr ) {
	IssLibWrapper *libw = (IssLibWrapper *)(class_inst_ptr);
	IssWrapper * wrap = (IssWrapper *) (libw->wrapper);
	wrap->iss_update_atomic_flag ( false );
}

void IssLibWrapper::iss_interrupt_me( void * class_inst_ptr, uint32_t val, uint32_t idx ) {
	IssLibWrapper *libw = (IssLibWrapper *)(class_inst_ptr);
	IssWrapper * wrap = (IssWrapper *) (libw->wrapper);
	wrap->iss_interrupt_me ( val,idx );
}

void IssLibWrapper::iss_request_timeout( void * class_inst_ptr, uint64_t ticks, void(*cb)(void*),void*iss_provider,uint64_t nosync ) {
	IssLibWrapper *libw = (IssLibWrapper *)(class_inst_ptr);
	IssWrapper * wrap = (IssWrapper *) (libw->wrapper);
	wrap->iss_request_timeout ( ticks,cb,iss_provider,nosync );
}

uint64_t * IssLibWrapper::iss_get_dotlm(void *class_inst_ptr, uint64_t base, uint64_t end, bool isFetch) {
	IssLibWrapper *libw = (IssLibWrapper *)(class_inst_ptr);
	IssWrapper * wrap = (IssWrapper *) (libw->wrapper);
	return wrap->iss_get_dotlm ( base, end, isFetch );
}

void IssLibWrapper::init ( int cpu_id, std::string cpu_model, uint32_t instr_quantum, uint64_t init_pc) {
	LOG_GLOBAL_DEBUG(dbg2) << NAME << ": iss_lib_wrapper::init." << std::endl;

	//---------------------------------------------------------------------------------
	//Initialize plugin structure
	ctx.vpsim_plugin.rwsync 			= iss_rwsync;
	ctx.vpsim_plugin.fsync 				= iss_fsync;
	ctx.vpsim_plugin.force_sync 		= iss_forcesync;
	ctx.vpsim_plugin.stop 				= iss_sstop;
	ctx.vpsim_plugin.atomic_set_flag 	= iss_atomic_set_flag;
	ctx.vpsim_plugin.atomic_reset_flag 	= iss_atomic_reset_flag;
	ctx.vpsim_plugin.get_dotlm		 	= iss_get_dotlm;
	ctx.vpsim_plugin.get_time = iss_get_time;
	ctx.vpsim_plugin.interrupt_me = iss_interrupt_me;
	ctx.vpsim_plugin.request_timeout = iss_request_timeout;
	ctx.vpsim_plugin.wait_for_interrupt = iss_wait_for_interrupt;

	ctx.vpsim_sim 	= this;
	ctx.cpu_id 		= cpu_id;
	ctx.cpu_model 	= NULL; //initialized and freed in ISS side
	ctx.instr_quantum 	= instr_quantum;


	//---------------------------------------------------------------------------------
	//Get the iss_init function
	typedef void (*iss_plugin_init_ptr)( struct iss_context_t *, const char*, bool, uint64_t );
	iss_plugin_init_ptr iss_plugin_init;
	iss_plugin_init = (iss_plugin_init_ptr) get_symbol ( /*m_lib_hdl,*/ "iss_plugin_init" );
	if (!iss_plugin_init) {
		LOG_GLOBAL_ERROR << "Cannot load iss_plugin_init symbol." << endl;
		cerr << "Cannot load iss_plugin_init symbol." << endl;
		dlerror ( ) ;
		throw ( 0 );
	}

	//---------------------------------------------------------------------------------
	//Call iss_init (from iss_plugin)
	iss_plugin_init ( &ctx, cpu_model.c_str(), is_gdb, init_pc );
}

void* IssLibWrapper::get_symbol(const char* sym) {
	dlerror();
	void* ptr = dlsym(m_lib_hdl, sym);
	if (!ptr) {
		dlerror();
		throw runtime_error(string("ISS Wrapper: unable to load symbol: ") + sym);
	}
	return ptr;
}

bool IssLibWrapper::is_application_done ( ) {
	return ctx.iss_plugin.is_done ( );
}

bool IssLibWrapper::reset_application_done ( ) {
	return ctx.iss_plugin.reset_done ( );
}

void IssLibWrapper::run ( ) {
	ctx.iss_plugin.run ( );
}

uint64_t* IssLibWrapper::getResultBuffer() {
	if (!mBuffer)
		mBuffer = ctx.iss_plugin.get_result_buffer();

	return mBuffer;
}


void IssLibWrapper::map_dmi ( string name, uint64_t base, uint32_t size, void *data) {
	ctx.iss_plugin.map_dmi ( name.c_str(), base, size, data );
}

void IssLibWrapper::create_rom ( string name, uint64_t base, uint32_t size, void *data) {
	ctx.iss_plugin.create_rom ( name.c_str(), base, size, data );
}

void IssLibWrapper::linux_mem_init ( uint32_t ncores, uint32_t size ) {
	ctx.iss_plugin.linux_mem_init ( ncores, size );
}

void IssLibWrapper::load_elf ( uint64_t ram_size, char *kernel_filename, char *kernel_cmdline, char *initrd_filename ) {
	ctx.iss_plugin.load_elf( ram_size, kernel_filename, kernel_cmdline, initrd_filename );
}

void IssLibWrapper::update_irq ( uint64_t val, uint32_t irq_idx ) {
	ctx.iss_plugin.update_irq( val, irq_idx );
}

void IssLibWrapper::tb_cache_flush( ) {
	ctx.iss_plugin.tb_cache_flush( );
}

void IssLibWrapper::do_cpu_reset ( ) {
	ctx.iss_plugin.do_cpu_reset( );
}




}

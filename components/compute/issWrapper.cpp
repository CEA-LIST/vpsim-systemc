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

#include "issWrapper.hpp"
#include "EndianHelper.hpp"
#include "log.hpp"


extern uint64_t HOST_TIME_START;
namespace vpsim {

IssWrapper::IssWrapper ( sc_module_name Name, int id_cpu, string lib, const char *cpu_model, unsigned int quantum,
							bool is_gdb, ARCHI_TYPE type, bool simflag, uint64_t init_pc, bool use_log, const char* logfile )
:	sc_module ( Name ),
	InitiatorIf (string(Name), quantum, true, 2),
	mQuantumKeeper ( quantum ),
	cpu_id ( id_cpu ),
	TYPE(type),
	mQuantumEnable ( true ),
	atomic_flag ( false ),
	sim_flag (simflag),
	mICount ( 0 ),
	mDCount ( 0 ),
	mLib ( string (Name), lib, (void *) this, id_cpu, is_gdb ),
	mGic(nullptr)
{
	setForceLt(false);
	setDiagnosticLevel(DBG_L0);

	uint32_t instr_quantum = quantum * 1; //1 nano second per instruction


	//Get bus size
	switch ( TYPE ) {
		case B16: BUS_SIZE = 2; break;
		case B32: BUS_SIZE = 4; break;
		case B64: BUS_SIZE = 8; break;
		default: cout <<getName()<<": ERROR wrong architecture type"<<endl; throw(0);
	}

	mWaitForInterruptStart=false;

    //Declare main SystemC thread
	SC_THREAD ( core_function );

	SC_METHOD(onTimeout_0);
	sensitive<<m_timeout_event[0];

	SC_METHOD(onTimeout_1);
	sensitive<<m_timeout_event[1];

	SC_METHOD(onTimeout_2);
	sensitive<<m_timeout_event[2];

	SC_METHOD(onTimeout_3);
	sensitive<<m_timeout_event[3];

	//SC_METHOD(iss_io_step);
	//sensitive<<mWaitIo;

	m_timeout_cb_0=m_timeout_cb_1=m_timeout_cb_2=m_timeout_cb_3=nullptr;

	mIoStep=nullptr;
	mInstrQuantum=instr_quantum;
	mCpuModel=cpu_model;
	mInitPc=init_pc;
	mWasInterrupted=false;

	if (use_log)
		setLog(use_log, logfile);

	//Initialize ISS library
	mLib.init ( cpu_id, mCpuModel.c_str(), mInstrQuantum, mInitPc );
}


IssWrapper::~IssWrapper() {
}

void IssWrapper::iss_fetch_sync ( uint64_t addr, uint64_t cnt, uint32_t instr_nosync, uint32_t call ) {
	//Debug
	LOG_GLOBAL_DEBUG(dbg2) << getName() <<": Fetch at the address 0x"<<hex<<addr<<dec<<endl;


	//Local variable
	sc_time delay = sc_time(cnt + instr_nosync, SC_NS);

	//Send inactive TLM communication
    tlm::tlm_response_status status;
    uint8_t * val = NULL;
    status = tlm::TLM_OK_RESPONSE; //InitiatorIf::target_mem_access ( 0, addr, cnt*BUS_SIZE, (uint8_t*) val, READ, delay );
    InitiatorIf::tlm_error_checking ( status );

	//Debug
	LOG_GLOBAL_DEBUG(dbg2) << getName() <<": Returned delay is "<<delay<<endl;

	//HW monitoring
	mICount += cnt + instr_nosync;

	//if (get_cpu_id()==1) cout<<"still alive !"<<endl;

	if ( getQuantumEnable() ) {

		//update local time
		mQuantumKeeper += delay;

		if(!atomic_flag){
			//Check quantum
			mQuantumKeeper.sync( );
		}
	}
}

void IssWrapper::iss_rw_sync ( uint64_t addr, ACCESS_TYPE rw_type, uint64_t cnt, int num_bytes, uint64_t value ) {

	ACCESS_TYPE rw;


	rw = rw_type;
	/*if (atomic_flag == true) {
		if (rw_type == READ)
			rw = READ_ATOMIC;
		else
			rw = WRITE_ATOMIC;
	}
*/
	bool nonAtomic{(rw == READ) || (rw == WRITE)};

	//Debug
	if(nonAtomic){
		LOG_GLOBAL_DEBUG(dbg2) << getName()
			<< (rw==READ ? ": Read" : rw==WRITE ? ": Write" : "")
			<< "at the address 0x"<<hex<<addr<<dec<<endl;
	}

	//HW monitoring
	mDCount += cnt;

	//Local variable
	sc_time delay = sc_time(cnt, SC_NS);

    tlm::tlm_response_status status;
//		uint8_t * val = NULL;
//		switch(num_bytes){
//			case 1 : { val = new uint8_t;  *(uint8_t*)val = (uint8_t)  (value & 0x00000000000000FF); break;}
//			case 2 : { val = new uint16_t; *(uint16_t*)val = (uint16_t) (value & 0x000000000000FFFF);break;}
//			case 4 : { val = new uint32_t; *(uint32_t*)val = (uint32_t) (value & 0x00000000FFFFFFFF);break;}
//			case 8 : { val = new uint64_t; *(uint64_t*)val = (uint64_t) (value & 0xFFFFFFFFFFFFFFFF);break;}
//			default : cerr<<"invalid access size"; break;
//		}

    LOG_GLOBAL_DEBUG(dbg2) << getName() << ": num_bytes is " << num_bytes << endl;
    if (rw == WRITE)
        LOG_GLOBAL_DEBUG(dbg2) << getName() << ": value to write is 0x" << hex << value << dec << endl;
    //uint64_t val = EndianHelper::IssValToByteArray<false>(value, num_bytes);

    //uint8_t * val = &value;
    if (rw == READ) {
        //memset(mLib.getResultBuffer(), 0,sizeof(uint64_t));
        status = InitiatorIf::target_mem_access(1, addr, num_bytes/*cnt*BUS_SIZE*/, (uint8_t *) mLib.getResultBuffer(),
                                                READ, delay, get_cpu_id());
        InitiatorIf::tlm_error_checking(status);

    } else if (rw == WRITE) {
        status = InitiatorIf::target_mem_access(1, addr, num_bytes/*cnt*BUS_SIZE*/, (uint8_t *) &value, WRITE, delay, get_cpu_id());
        InitiatorIf::tlm_error_checking(status);



        // DEBUG memcpy(mLib.getResultBuffer(), &value, sizeof(uint64_t));

    } else {
        //atomic read/write
        throw runtime_error("Not supposed to leave ISS (atomic).");
    }
/*
    if(rw==READ) {
        LOG_GLOBAL_DEBUG(dbg1) << getName() <<": Raw read data ";
        for(int i=0; i<num_bytes; i++){
            LOG_GLOBAL_DEBUG(dbg1) <<hex<<" 0x"<<(int)(((uint8_t*)&val)[i]);
        }
        globalLogger.logDebug(dbg1) <<dec<<endl;
        LOG_GLOBAL_DEBUG(dbg1) << getName() <<": read value is 0x"<<hex<<EndianHelper::ByteArrayToIssVal<false>((uint8_t*)&val,num_bytes)<<dec<<endl;
    }*/


	//Debug
	LOG_GLOBAL_DEBUG(dbg2) << getName() <<": Returned delay is "<<delay<<endl;

	if (  getQuantumEnable() ) {

		//update local time
		mQuantumKeeper+= delay;

		if (!atomic_flag){ //no atomic loads or stores
			mQuantumKeeper.sync();
		}
	}

	for (auto range: mMonitoredRanges) {
		if (addr >= range.first && addr < range.first+range.second)
		{
			std::cout<<"[\e[32mCPU with ID "<<get_cpu_id()<<" ("<<getName()<<")\e[39m] ";
			std::cout<<(rw==READ?"read ":"write ")<<"0x"<<hex<<addr<<" value: "<<(rw==WRITE?value:*(uint64_t *)mLib.getResultBuffer());
			std::cout<<" size: "<<num_bytes<<endl;
		}
	}

}


void IssWrapper::iss_stop ( ) {
	if ( mLib.is_application_done() ) {
		LOG_GLOBAL_INFO << getName() <<": Application done." << endl;
		LOG_GLOBAL_INFO << getName() <<": stops transaction at time (seconds) = "<<sc_time_stamp().to_seconds()<< ", local time (seconds) = "<< mQuantumKeeper.get_local_time().to_seconds() << endl;
	}

	//Wait remaining time
	mQuantumKeeper.forceSync();


	//Stop
	//sc_stop ( );

	//add a wait so that sc_stop happens during update
	//wait();
}

void IssWrapper::iss_io_step() {
	/*while (true) {
		wait(1000,SC_NS);
		if (!mIoStep) {
			mIoStep = (provider_io_step)get_symbol("io_step_tlm");
			//mWaitIo.notify(sc_time(1000,SC_NS));
			//return;
		}

	}*/
	if (mIoStep)
		mIoStep();
	else {
		mIoStep = (provider_io_step)get_symbol("io_step_tlm");
	}
	next_trigger(tlm::tlm_global_quantum::instance().get());
}

void IssWrapper::iss_force_sync ( uint64_t instrs) {
	//sc_core::wait(100,SC_NS);
	//cout<<"going to sleep : "<<get_cpu_id()<<endl;
	// mQuantumKeeper += sc_time(std::max((int)instrs,1000),SC_NS); //tlm::tlm_global_quantum::instance().get()/2;
	// mQuantumKeeper.sync();
	// check io
	for (sc_event* mWaitIoEvent: mWaitIoEvents) {
		mWaitIoEvent->notify(SC_ZERO_TIME);
	}
	mQuantumKeeper += sc_time(instrs,SC_NS);
	if (mQuantumKeeper.get_local_time()!=SC_ZERO_TIME){
		mQuantumKeeper.forceSync();
	} else {
		wait(tlm::tlm_global_quantum::instance().get(),
				mWaitForInterrupt);
	}
	//wait(mWaitForInterrupt);
	//cout<<get_cpu_id()<<" cpu waken up."<<endl;
	//mQuantumKeeper.forceSync();
/*
	if (mQuantumKeeper.get_local_time()==SC_ZERO_TIME){
		// nothing to wait for, just wait for any interrupt
		cout<<"waiting event."<<endl;
		wait(mWaitForInterrupt);
	} else {
		mQuantumKeeper.forceSync();
	}*/
}

uint64_t IssWrapper::iss_get_time (uint64_t nosync) { // in ns
	return (uint64_t) ((mQuantumKeeper.get_current_time()
				+sc_time(nosync,SC_NS)).to_seconds()*1000000000)
				+HOST_TIME_START;
}


void IssWrapper::print_statistics ( ) {
	//Debug
	LOG_GLOBAL_STATS << "("<<getName()<<") total number of executed instructions = "<<mICount<<endl;
	LOG_GLOBAL_STATS << "("<<getName()<<") total number of data accesses = "<<mDCount<<endl;
	LOG_GLOBAL_STATS << "("<<getName()<<") total number of TLM transactions = "<<mDCount+mICount<<endl << endl;
}

void IssWrapper::setWaitForInterrupt(bool wfi) {
	mWaitForInterruptStart=wfi;
}

void IssWrapper::core_function ( ) {

	LOG_GLOBAL_INFO <<"ISS main loop started"<<std::endl;
	std::cout<<"ISS main loop started"<<std::endl;

	//Reset the local time
	mQuantumKeeper.reset();



	using set_io_only_t =  void (*) (int);
	auto set_io_only = (set_io_only_t) get_symbol("set_io_only");
	set_io_only(io_only);

	wait(delay_before_boot);


	//Start main ISS loop
	if (mWaitForInterruptStart) {
		wait(mWaitForInterruptStart);
		cout<<"cpu "<<get_cpu_id()<<" booted after delay. "<<sc_time_stamp()<<endl;
	}


	mLib.run ( );

	//Exit from main ISS loop
	LOG_GLOBAL_INFO <<"ISS main loop exited"<<std::endl;
	std::cout<<"ISS main loop exited: cpu_id = "<<cpu_id<<std::endl<<flush;


	//mLib.run();
	//std::cout<<"ISS main loop exited again"<<std::endl<<flush;


	if (sim_flag && cpu_id == 0) { //Stop simulation when core 0 finishes
		sc_stop();
	}


	return;
}


//Set functions
void IssWrapper::setQuantumEnable( bool QuantumEnable ) {
	mQuantumEnable = QuantumEnable;
}

uint32_t IssWrapper::get_cpu_id ( ) {
	return cpu_id;
}

bool IssWrapper::getQuantumEnable() {
	return mQuantumEnable;
}

//Get functions
uint64_t IssWrapper::get_nbinstr ( ) {
	return mICount;
}

void IssWrapper::iss_create_rom ( string name, uint64_t base_address, uint32_t size, void *data ) {
	mLib.create_rom ( name, base_address, size, data );
}

void IssWrapper::add_map_dmi(string name, uint64_t base_address, uint32_t size, void *data) {
	mLib.map_dmi ( name, base_address, size, data );
}

void IssWrapper::iss_linux_mem_init ( uint32_t ncores, uint32_t size ) {
	mLib.linux_mem_init ( ncores, size );
}

void IssWrapper::iss_load_elf(uint64_t ram_size, char *kernel_filename, char *kernel_cmdline, char *initrd_filename) {
	mLib.load_elf(ram_size, kernel_filename, kernel_cmdline, initrd_filename);
}

void  IssWrapper::iss_update_irq(uint64_t val, uint32_t irq_idx) {
	mLib.update_irq(val, irq_idx&0xffff);
	if (val) {
		mWaitForInterrupt.notify(SC_ZERO_TIME);
		mWasInterrupted=true;
	}
}

void IssWrapper::iss_interrupt_me(uint32_t value, uint32_t line){
	InterruptIf* gic = mGic;
	if (gic != nullptr) {
		//if(get_cpu_id()==1) cout<<hex<<get_cpu_id()<<" interrupting self: "<<value<<" line "<<line<<dec<<endl;
		gic->update_irq(value, line);
	}
}

void  IssWrapper::iss_tb_cache_flush() {
	mLib.tb_cache_flush( );
}

void IssWrapper::iss_update_atomic_flag(bool val) {
	atomic_flag = val;
}




//############################################################

}

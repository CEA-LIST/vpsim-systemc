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

#ifndef ISS_WRAPPER_HPP_
#define ISS_WRAPPER_HPP_

#include "global.hpp"
#include "issLibWrapper.hpp"
#include "InitiatorIf.hpp"
#include "quantum.hpp"
#include "InterruptIf.hpp"
#include "HybridList.hpp"


#include <functional>
#include <deque>
#include <unordered_map>

#define DEFAULT_TIMER_SCALE 16 // ns/tick

namespace vpsim{
typedef
void (*provider_io_step)();

	class IssWrapper: public sc_module, public InitiatorIf, public InterruptIf
	{

	protected:
		ParallelQuantumKeeper mQuantumKeeper;

	private:
		//Members
		uint32_t cpu_id;
		ARCHI_TYPE TYPE;
		uint32_t BUS_SIZE;
		bool mQuantumEnable;

		bool atomic_flag; //atomic = 1, non-atomic = 0

		//1 = core 0 calls sc_stop on exiting to end simulation.
		//0 = all cores must exit before simulation ends.
		bool sim_flag;

		//HW counters
		uint64_t mICount;
		uint64_t mDCount;

		uint64_t mInitPc;
		string mCpuModel;
		uint64_t mInstrQuantum;

		vector<sc_event*> mWaitIoEvents;

		//ISS library
		IssLibWrapper mLib;

		sc_event mWaitForInterrupt;
		sc_event mWaitIo;
		bool mWaitForInterruptStart;

		InterruptIf* mGic;

		provider_io_step mIoStep;
		bool mWasInterrupted;

		void internal_cpu_timeout(uint64_t ticks, void(*timeout_cb)(void*), void* internal_cpu, uint32_t epoch, uint64_t nosync) {
			while (ticks*DEFAULT_TIMER_SCALE > iss_get_time(0)) {
				sc_core::wait(sc_time(ticks*DEFAULT_TIMER_SCALE-iss_get_time(0),SC_NS)  );
				if (epoch!=timer_epoch) {
					return;
				}

				//mGic->update_irq(1,30 | ((1<<get_cpu_id())<<16));
				//break;
			}
			timeout_cb(internal_cpu);
		}
		uint32_t timer_epoch;

	private:
		void updateIssParameters();
		void(*m_timeout_cb_0)(void*);
		void(*m_timeout_cb_1)(void*);
		void(*m_timeout_cb_2)(void*);
		void(*m_timeout_cb_3)(void*);
		void* m_iss_provider;

		sc_event m_timeout_event[4];


	public:

		void waitForEvent() {
			while (!mWasInterrupted) {
				if (mQuantumKeeper.get_local_time()!=SC_ZERO_TIME)
					mQuantumKeeper += mQuantumKeeper.getNextSyncPoint()-mQuantumKeeper.get_current_time();
				else
					mQuantumKeeper += tlm::tlm_global_quantum::instance().get();
				mQuantumKeeper.sync();
			}
			mWasInterrupted=false;
		}

		sc_event* getIoEvent() { return &mWaitIo; }
		void addIoEvent(sc_event* ev) { mWaitIoEvents.push_back(ev); }
		bool io_only;
		void setIoOnly(bool io_only) { this->io_only = io_only; }

		sc_time delay_before_boot;
		void setDelayBeforeBoot(sc_time delay) { delay_before_boot=delay; }

		string log_file;
		bool log;
		void setLog(bool log, string logfile) {
			this->log=log;
			this->log_file=logfile;
			using set_log_t = void(*)(int, const char*);
			auto set_log=(set_log_t)get_symbol("set_log");
			set_log(log,logfile.c_str());
		}

		void iss_io_step();
		// we need to drive the internal cpu timer (if any) from the SystemC kernel.
		// below is what we need to do just that.
		void iss_request_timeout(uint64_t ticks, void(*timeout_cb)(void*),void* iss_provider,uint64_t nosync) {
			uint32_t idx=nosync>>32;
			nosync=nosync & 0xffffffff;
			switch(idx) {
			case 0:m_timeout_cb_0=timeout_cb;break;
			case 1:m_timeout_cb_1=timeout_cb;break;
			case 2:m_timeout_cb_2=timeout_cb;break;
			case 3:m_timeout_cb_3=timeout_cb;break;
			}
			m_iss_provider=iss_provider;
			uint64_t current_time = iss_get_time(0);
			timer_epoch++;
			m_timeout_event[idx].notify(ticks*DEFAULT_TIMER_SCALE - current_time, SC_NS);
			//sc_spawn(sc_bind(&IssWrapper::internal_cpu_timeout,this,ticks,timeout_cb,iss_provider,timer_epoch,nosync));
		}

		void onTimeout_0() {
			if (m_timeout_cb_0) {
				//cout<<"timeout 0."<<endl;
				m_timeout_cb_0(m_iss_provider);
			}
		}
		void onTimeout_1() {
			if (m_timeout_cb_1) {
				//cout<<"timeout 1."<<endl;
				m_timeout_cb_1(m_iss_provider);
			}
		}
		void onTimeout_2() {
			if (m_timeout_cb_2) {
				//cout<<"timeout 2."<<endl;
				m_timeout_cb_2(m_iss_provider);
			}
		}
		void onTimeout_3() {
			if (m_timeout_cb_3) {
				//cout<<"timeout 3."<<endl;
				m_timeout_cb_3(m_iss_provider);
			}
		}
		void setGic(InterruptIf* gic_ptr) { mGic = gic_ptr; }
		void setWaitForInterrupt(bool wfi);

		void add_map_dmi ( string name, uint64_t base_address, uint32_t size, void *data );

		void iss_create_rom ( string name, uint64_t base_address, uint32_t size, void *data );

		void iss_linux_mem_init ( uint32_t, uint32_t size );

		void iss_load_elf( uint64_t ram_size, char *kernel_filename, char *kernel_cmdline, char *initrd_filename );

		void iss_update_irq ( uint64_t val, uint32_t irq_idx );
		inline void update_irq ( uint64_t val, uint32_t irq_idx ){iss_update_irq(val, irq_idx);}

		void iss_tb_cache_flush ( );

		void iss_rw_sync ( uint64_t addr, ACCESS_TYPE rw_type, uint64_t cnt, int num_bytes, uint64_t value );

		virtual void iss_fetch_sync ( uint64_t addr, uint64_t cnt, uint32_t instr_quantum, uint32_t call );

		void iss_force_sync ( uint64_t instrs );
		uint64_t iss_get_time ( uint64_t nosync );

		void iss_interrupt_me(uint32_t value, uint32_t line);

		void iss_stop ( );

		void iss_update_atomic_flag(bool val);

        using issGetDoTLM_ft = std::function<uint64_t*(uint64_t, uint64_t, bool)>;
        issGetDoTLM_ft iss_get_dotlm;
        void registerIssGetDoTLM(issGetDoTLM_ft&& f){iss_get_dotlm = f;};


        uint64_t getInstructionCount() { return mICount; }
        uint64_t getDataAccessCount() { return mDCount; }

        void* get_symbol(const char* sym) { return mLib.get_symbol(sym); }

        std::vector<std::pair<uint64_t,uint64_t>> mMonitoredRanges;
        void monitorRange(uint64_t start, uint64_t size) {
        	mMonitoredRanges.push_back(make_pair(start,size));
        }
        void removeMonitor(uint64_t start, uint64_t size) {
        	std::vector<std::pair<uint64_t,uint64_t>> newM;
        	for (auto rng: mMonitoredRanges) {
        		if (rng.first >= start && rng.first + rng.second - 1 <= start + size - 1)
        			continue; // remove
        		newM.push_back(make_pair(start,size));
        	}
        	mMonitoredRanges=newM;
        }
        void showMonitor() {
        	cout.clear();
        	for (auto rng: mMonitoredRanges) {
        		cout<<"Range: "<<hex<<rng.first<<" size: "<<rng.second<<dec<<endl;
        	}
        }

	public:
		//Constructor
		IssWrapper( sc_module_name Name, int id_cpu, string lib,
				const char *cpu_model, unsigned int QuantumKeeper, bool is_gdb, ARCHI_TYPE type, bool simflag, uint64_t init_pc=0,
				bool use_log=false, const char* logfile=nullptr);
		SC_HAS_PROCESS (IssWrapper);

		//Destructor
		~IssWrapper();

		//Main functions
		void core_function ( );

		//Set functions
		void setQuantumEnable ( bool QuantumEnable );

		//Get functions
		bool getQuantumEnable ( );

		uint32_t get_cpu_id ( );

		uint64_t get_nbinstr ( );

		void print_statistics();
	};
}//end namespace vpsim

#endif

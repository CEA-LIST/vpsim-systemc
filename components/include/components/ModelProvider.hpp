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

#ifndef _MODELPROVIDER_HPP_
#define _MODELPROVIDER_HPP_

#include "global.hpp"
#include "InitiatorIf.hpp"
#include "TargetIf.hpp"

#include "quantum.hpp"
#include "InterruptIf.hpp"
#include "VpsimIp.hpp"
#include "MainMemCosim.hpp"
#include "IOAccessCosim.hpp"

#include <dlfcn.h>

#include <functional>
#include <deque>
#include <unordered_map>

#include "DynamicComponents.hpp"
#include "CacheBase.hpp"

using namespace tlm;

namespace vpsim {

	/* Instruction cache model based on CacheBase */
	class StandaloneInstructionCache: public CacheBase<uint64_t,uint64_t> {
		public:	StandaloneInstructionCache(sc_module_name name,
				int cpu_id,
				uint64_t CacheSize,
				uint64_t CacheLineSize,
				uint64_t Associativity,
				CacheReplacementPolicy ReplPolicy=LRU)
		: CacheBase<uint64_t,uint64_t>(name,CacheSize,CacheLineSize,Associativity,ReplPolicy){
			// anything ?
			SetEvictionNotifier(StandaloneInstructionCache::OnLineEvicted);
			mCpuId=cpu_id;
		}

		virtual tlm::tlm_response_status ForwardRead(uint64_t Addr, size_t size, sc_time& delay) override {
			// register the fetch miss
			MainMemCosim::NotifyFetchMiss(mCpuId, (void*)Addr, size);
			return TLM_OK_RESPONSE;
		}

		static void OnLineEvicted(void* handle) {
			// Check if still valid
			int** flag = (int**)handle;

			if(*flag != &mZero && *flag != &mOne) {
				// throw runtime_error("Weird segfault was about to happen... :/");
			} else {
				*flag=&mZero; // stop assuming hits.
			}
		}

		int mCpuId;
		static int mZero;
		static int mOne;

		// victim list
		static void AppendVictim(void* victim) {
			Victims[victim]=true;
		}

		static map<void*,bool> Victims;

	};




    typedef void (*ThreadFunctionType)(void* cpu, uint64_t quantum);



    typedef uint64_t(*ReadCb)(void *opaque,
            uint64_t addr,
            unsigned size);

    typedef void (*WriteCb)(void *opaque,
            uint64_t addr,
            uint64_t data,
            unsigned size);
    typedef void (*SyncCb)(void* opaque, uint64_t executed, int wfi);

    typedef uint64_t (*ICacheMissCb)(void* opaque, uint64_t addr, unsigned size, int* tb_hit);

    typedef void (*AddVictimCb)(void* victim);
    typedef void (*MainMemCb)(
                          void*
                        , uint64_t exec
                        , uint8_t is_write
                        , void* phys
                        , uint64_t virt
                        , unsigned int size
                        );

    typedef uint64_t (*OuterStatGetter)(uint32_t index, enum OuterStat type);
    typedef void(*FillBiasCb)(uint64_t* ts, int n);

    typedef void (*IOAccessCb)(uint32_t device, uint64_t exec, uint8_t is_write
                        	, void* phys
                        	, uint64_t virt
                        	, unsigned int size
				, uint64_t tag
                              );
    typedef uint64_t (*IOAccessStatGetter)(uint32_t device, enum IOAccessStat type);
    typedef uint8_t (*IOAccessGetDelayCb)(uint32_t device, uint64_t* time_stamp, uint64_t* delay, uint64_t* tag);
    typedef int (*modelprovider_configure_t)(int argc, char **argv, char **envp);
    typedef void (*modelprovider_set_default_read_callback_t)(ReadCb cb);
    typedef void (*modelprovider_set_default_write_callback_t)(WriteCb cb);
    typedef void(* modelprovider_set_sync_callback_t)(SyncCb cb);
    typedef void (*modelprovider_declare_external_dev_t)(char* name, uint64_t base, uint64_t size);
    typedef void (*modelprovider_declare_external_ram_t)(char* name, uint64_t base, uint64_t size, void* data);
    typedef void* (*modelprovider_create_internal_cpu_t)(void *proxy, char* type, int index, uint64_t start_pc, int secure, int start_off);
    typedef void (*modelprovider_run_cpu_t)(void* cpu, uint64_t quantum);
    typedef void* (*modelprovider_create_internal_dev_default_t)(char* name,
            uint64_t base,
            int irq,
            ReadCb* rd,
            WriteCb* wr);
    typedef void (*modelprovider_poll_io_t)(void);
    typedef void (*modelprovider_finalize_config_t)(void);


    typedef void (*modelprovider_register_unlock_t)(void(*)(void*), void*);
    typedef void (*modelprovider_register_wait_unlock_t)(void(*)(void*), void*);

    typedef void (*modelprovider_interrupt_t)(int index, int value);

    typedef void (*modelprovider_cpu_get_stats_t)(int index, uint32_t*, void**);
    typedef void (*modelprovider_show_cpu_t)(void* handle);
    typedef void (*modelprovider_register_main_mem_callback_t)(MainMemCb, uint64_t);
    typedef void (*modelprovider_unregister_main_mem_callback_t)(void);
    typedef void (*modelprovider_register_outer_stat_cb_t)(OuterStatGetter);
    typedef void (*modelprovider_register_fill_bias_cb_t)(FillBiasCb);

    typedef void (*modelprovider_register_icache_miss_cb_t)(ICacheMissCb);
    typedef void (*modelprovider_register_add_victim_cb_t)(AddVictimCb);

    typedef void (*modelprovider_register_ioaccess_callback_t)(IOAccessCb);
    typedef void (*modelprovider_register_ioaccess_get_delay_cb_t)(IOAccessGetDelayCb);
    typedef void (*modelprovider_register_ioaccess_stat_cb_t)(IOAccessStatGetter);

#define LDFCT(typ,nm) nm=(typ##_t)loadSymbol(#typ)

    struct ModelProvider : public sc_module, public InterruptIf {

        ModelProvider(sc_module_name name, string path, uint64_t poll_period, uint64_t quantum = 1000) : sc_module(name), configured(false), poll_period(poll_period), quantum(quantum) {
            lib = dlopen(path.c_str(), RTLD_LOCAL | RTLD_LAZY);

            if (!lib) {
                throw runtime_error(path + ": unable to load library " + dlerror());
            }
            LDFCT(modelprovider_configure, configure);
            LDFCT(modelprovider_set_default_read_callback, set_default_read_callback);
            LDFCT(modelprovider_set_default_write_callback, set_default_write_callback);
            LDFCT(modelprovider_set_sync_callback, set_sync_callback);
            LDFCT(modelprovider_run_cpu, run_cpu);
            LDFCT(modelprovider_poll_io, poll_io);
            LDFCT(modelprovider_declare_external_dev, declare_external_dev);
            LDFCT(modelprovider_declare_external_ram, declare_external_ram);
            LDFCT(modelprovider_create_internal_cpu, create_internal_cpu);
            LDFCT(modelprovider_create_internal_dev_default, create_internal_dev_default);
            LDFCT(modelprovider_finalize_config, finalize_config);
            LDFCT(modelprovider_register_unlock, modelprovider_unlock);
            LDFCT(modelprovider_register_wait_unlock, modelprovider_wait_unlock);
            LDFCT(modelprovider_interrupt, interrupt);
            LDFCT(modelprovider_cpu_get_stats, get_stats);
            LDFCT(modelprovider_show_cpu, show_cpu);
            LDFCT(modelprovider_register_main_mem_callback, modelprovider_register_main_mem_callback);
            LDFCT(modelprovider_unregister_main_mem_callback, modelprovider_unregister_main_mem_callback);
            LDFCT(modelprovider_register_outer_stat_cb, modelprovider_register_outer_stat_cb);
            LDFCT(modelprovider_register_fill_bias_cb, modelprovider_register_fill_bias_cb);
            LDFCT(modelprovider_register_icache_miss_cb, modelprovider_register_icache_miss_cb);
            LDFCT(modelprovider_register_add_victim_cb, modelprovider_register_add_victim_cb);
            LDFCT(modelprovider_register_ioaccess_callback, modelprovider_register_ioaccess_callback);
            LDFCT(modelprovider_register_ioaccess_get_delay_cb, modelprovider_register_ioaccess_get_delay_cb);
            LDFCT(modelprovider_register_ioaccess_stat_cb, modelprovider_register_ioaccess_stat_cb);

            addParam1("ModelProvider");
            internal_executed = 0;
            wait_to_consume = SC_ZERO_TIME;
            max_time = SC_ZERO_TIME;

            SC_THREAD(io_thread);
            SC_THREAD(cpu_thread);
        }

        SC_HAS_PROCESS(ModelProvider);

        void *loadSymbol(string sym) {
            if (!lib)
                throw runtime_error("getting symbol from null library !");

            dlerror();
            void* ptr = dlsym(lib, sym.c_str());
            if (!ptr) {
                dlerror();
                throw runtime_error(string("ISS Wrapper: unable to load symbol: ") + sym);
            }
            return ptr;
        }

        void addParam1(string arg) {
            argv.push_back(arg);
        }

        void addParam2(string param, string value) {
            argv.push_back(param);
            argv.push_back(value);
        }

        void config() {
            if (configured)
                return;
            char** argv_c = (char**) malloc(sizeof (char*)*argv.size());
            int i = 0;
            for (string& arg : argv) {
                argv_c[i++] = strdup(arg.c_str());
            }
            configure(i, argv_c, nullptr);
            configured = true;
        }

        void io_thread() {
            while (true) {
                poll_io();
            }
        }

        void cpu_thread() {
            run_cpu(NULL,
                    tlm::tlm_global_quantum::instance().get().to_seconds()
                    * 1000000000);
        }

        static void get_cpu_biases(uint64_t* times, int n) {
            MainMemCosim::FillBiases(times, n);
        }

        void wait_unlock() {
            wait(big_mutex);
        }

        void unlock() {
            big_mutex.notify(SC_ZERO_TIME);
            wait(1, SC_NS);
        }

        void update_irq(uint64_t val, uint32_t irq_idx) override {
            if (val) {
                sysc_event.notify(SC_ZERO_TIME);
            }
            interrupt(irq_idx, val);
        }

        void sync(uint64_t executed, bool wait_for_event) {
            internal_executed += executed;

            if (internal_executed >= poll_period || (internal_executed && wait_for_event)) {
                wait(internal_executed, SC_NS);
                internal_executed = 0;
            } else if (wait_for_event) {
                wait(sc_time(poll_period, SC_NS), sysc_event);
            }
        }

        void *lib;
        vector<string> argv;

        bool configured;
        sc_event check_io_event;
        uint64_t poll_period;
        uint64_t quantum;

        sc_event big_mutex;
        sc_event sysc_event;

        sc_time wait_to_consume;
        sc_time max_time;

        uint64_t internal_executed;

        // functions

        modelprovider_configure_t configure;
        modelprovider_set_default_read_callback_t set_default_read_callback;
        modelprovider_set_default_write_callback_t set_default_write_callback;
        modelprovider_set_sync_callback_t set_sync_callback;
        modelprovider_run_cpu_t run_cpu;
        modelprovider_poll_io_t poll_io;

        modelprovider_declare_external_dev_t declare_external_dev;
        modelprovider_declare_external_ram_t declare_external_ram;
        modelprovider_create_internal_cpu_t create_internal_cpu;

        modelprovider_create_internal_dev_default_t create_internal_dev_default;

        modelprovider_finalize_config_t finalize_config;
        modelprovider_register_unlock_t modelprovider_unlock;
        modelprovider_register_wait_unlock_t modelprovider_wait_unlock;

        modelprovider_interrupt_t interrupt;
        modelprovider_cpu_get_stats_t get_stats;
        modelprovider_show_cpu_t show_cpu;

        modelprovider_register_main_mem_callback_t modelprovider_register_main_mem_callback;
        modelprovider_unregister_main_mem_callback_t modelprovider_unregister_main_mem_callback;
        modelprovider_register_outer_stat_cb_t modelprovider_register_outer_stat_cb;
        modelprovider_register_fill_bias_cb_t modelprovider_register_fill_bias_cb;
        modelprovider_register_icache_miss_cb_t modelprovider_register_icache_miss_cb;
        modelprovider_register_add_victim_cb_t modelprovider_register_add_victim_cb;

        modelprovider_register_ioaccess_callback_t modelprovider_register_ioaccess_callback;
        modelprovider_register_ioaccess_get_delay_cb_t modelprovider_register_ioaccess_get_delay_cb;
        modelprovider_register_ioaccess_stat_cb_t modelprovider_register_ioaccess_stat_cb;
    };

    void model_provider_unlock_cb(void* opaque) {
        ModelProvider* mp = (ModelProvider*) opaque;
        mp->unlock();
    }

    void model_provider_wait_unlock_cb(void* opaque) {
        ModelProvider* mp = (ModelProvider*) opaque;
        mp->wait_unlock();
    }


    struct ModelProviderDev : public sc_module {

        ModelProviderDev(sc_module_name name, string model, uint64_t addr, uint32_t size, int irq) :
        sc_module(name),
        //TargetIf(string(name), size),
        model(model),
        read_callback(nullptr),
        write_callback(nullptr),
        internal_dev(nullptr),
        irq(irq) {
            //TargetIf <REG_T>::RegisterReadAccess(REGISTER(ModelProviderDev,read));
            //TargetIf <REG_T>::RegisterWriteAccess(REGISTER(ModelProviderDev,write));

            //setBaseAddress(addr);

            base_address = addr;
        }

        tlm::tlm_response_status read(payload_t & payload, sc_time & delay) {
            if (!read_callback || !write_callback || !internal_dev)
                throw runtime_error("ModelProviderDev: not properly initialized !");

            uint64_t res = read_callback(internal_dev, payload.addr, payload.len);
            memcpy(payload.ptr, &res, payload.len);

            return TLM_OK_RESPONSE;
        }

        tlm::tlm_response_status write(payload_t & payload, sc_time & delay) {
            if (!read_callback || !write_callback || !internal_dev)
                throw runtime_error("ModelProviderDev: not properly initialized !");

            uint64_t wr = 0;
            memcpy(&wr, payload.ptr, payload.len);
            write_callback(internal_dev, payload.addr, wr, payload.len);

            return TLM_OK_RESPONSE;
        }

        uint64_t getBaseAddress() {
            return base_address;
        }

        void setProvider(ModelProvider* prov) {
            //prov->config();
            get_stats = prov->get_stats;
            internal_dev = prov->create_internal_dev_default((char*) model.c_str(), getBaseAddress(), irq, &read_callback, &write_callback);
        }

        string model;

        ReadCb read_callback;
        WriteCb write_callback;
        void *internal_dev;
        uint64_t base_address;
        int irq;

        modelprovider_cpu_get_stats_t get_stats;
    };

    struct ModelProviderCpu : public sc_module, public InitiatorIf, public InterruptIf {

        ModelProviderCpu(sc_module_name name, string model, uint32_t index, uint64_t start_pc, uint64_t quantum, int secure, int start_off,
        		uint64_t iCacheSize,
				uint64_t iCacheLineSize,
				uint64_t iCacheAssociativity,
				CacheReplacementPolicy iCacheReplPolicy) :

			sc_module(name),
			InitiatorIf(string(name), quantum, true, 1),
			model(model),
			index(index),
			start_pc(start_pc),
			quantum(quantum),
			thread_function(nullptr),
			internal_cpu(nullptr),
			quantum_keeper(quantum),
			secure(secure),
			start_off(start_off),

			icache((string(name)+"_icache").c_str(),index,iCacheSize,iCacheLineSize,iCacheAssociativity,iCacheReplPolicy)
        {
            //SC_THREAD(exec_thread_function);
        }

        SC_HAS_PROCESS(ModelProviderCpu);

        void exec_thread_function() {
            if (!thread_function || !internal_cpu || !provider) {
                throw runtime_error("ModelProviderCpu: not properly intialized.");
            }
            provider->config();
            thread_function(internal_cpu, quantum);
        }

        uint64_t do_read(uint64_t addr,
                unsigned size) {
            uint64_t res = 0;
            InitiatorIf::tlm_error_checking(
                    InitiatorIf::target_mem_access(0, addr, size, (uint8_t *) & res,
                    READ, local_bias, index)
                    );
            return res;
        }

        void do_write(uint64_t addr,
                uint64_t data,
                unsigned size) {
            InitiatorIf::tlm_error_checking(
                    InitiatorIf::target_mem_access(0, addr, size, (uint8_t *) & data,
                    WRITE, local_bias, index)
                    );
        }

        void sync(uint64_t executed) {
            quantum_keeper += sc_time(executed, SC_NS);
            quantum_keeper.sync();
            throw (0);
        }

        void update_irq(uint64_t val, uint32_t irq_idx) {
            //cerr<<"warning: ModelProviderCpu: interrupt not yet implemented."<<endl;
        }

        void setProvider(ModelProvider* prov) {
            //prov->config();
            thread_function = prov->run_cpu;
            internal_cpu = prov->create_internal_cpu((void*) this, (char*) model.c_str(), index, start_pc, secure, start_off);
            provider = prov;
            get_stats = prov->get_stats;
        }

        void show_cpu() {
            provider->show_cpu(internal_cpu);
        }

        string model;
        uint32_t index;
        uint64_t start_pc;
        uint64_t quantum;

        ThreadFunctionType thread_function;
        void* internal_cpu;

        ParallelQuantumKeeper quantum_keeper;

        int secure, start_off;

        ModelProvider* provider;
        modelprovider_cpu_get_stats_t get_stats;

        sc_time local_bias;

        // instruction cache here ;)
        StandaloneInstructionCache icache;


        // should be a base class (AddressConverter)
    	vector< tuple<void*,uint64_t,uint64_t> > mMaps;
    	bool convertAddr(void* host, uint64_t* p) {
			uint64_t H=(uint64_t)host;
			for (auto& t: mMaps) {
				uint64_t Hp=(uint64_t)get<0>(t);
				//cout<<"Checking range: "<<hex<<Hp<<endl;
				if (H>=Hp && H < Hp+get<2>(t)){
					*p = get<1>(t) + (H-Hp);
					return true;
				}
			}
			return false;
		}
    };

    uint64_t model_provider_read_cb(void *opaque,
            uint64_t addr,
            unsigned size) {
        ModelProviderCpu* cpu = (ModelProviderCpu*) opaque;
        return cpu->do_read(addr, size);
    }

    void model_provider_write_cb(void *opaque,
            uint64_t addr,
            uint64_t data,
            unsigned size) {
        ModelProviderCpu* cpu = (ModelProviderCpu*) opaque;
        cpu->do_write(addr, data, size);
    }

    uint64_t model_provider_fetch_miss_cb( void* opaque
                                        , uint64_t addr
                                        , unsigned size
                                        , int* tb_hit
                                        ) {
        ModelProviderCpu* cpu = (ModelProviderCpu*) opaque;
        sc_time null;
        uint64_t phaddr=0;
        cpu->convertAddr((void*)addr,&phaddr);
        cpu->icache.ReadData(nullptr, phaddr, size, cpu->index, cpu->index, null, null, (void*)tb_hit);
        *(int**)tb_hit = &cpu->icache.mOne;
        return cpu->icache.MissCount;
    }


    void model_provider_sync(void* opaque, uint64_t executed, int wfi) {
        ModelProviderCpu* cpu = (ModelProviderCpu*) opaque;
        cpu->provider->sync(executed, wfi);
    }

   /* void model_provider_main_mem_cb(void* opaque,
    		int write, void* phys, uint64_t virt, uint64_t size) {
    	ModelProviderCpu* cpu = (ModelProviderCpu*) opaque;
    	MainMemCosim::Notify(cpu->index,write,phys,size);
    }*/
    void model_provider_main_mem_cb(void* opaque,uint64_t exec,
               uint8_t write, void* phys, uint64_t virt, unsigned int size) {
        ModelProviderCpu* cpu = (ModelProviderCpu*) opaque;
        MainMemCosim::Notify(cpu->index,exec,write,phys,size);
    }

    uint64_t model_provider_outer_stat_cb(uint32_t index, enum OuterStat stat) {
        return MainMemCosim::GetStat(index, stat);
    }

    void model_provider_ioaccess_cb(uint32_t device, uint64_t exec,
            uint8_t write, void* phys, uint64_t virt, unsigned int size, uint64_t tag) {
        MainMemCosim::NotifyIO(device,exec,write,phys,virt,size,tag);
    }

    uint8_t model_provider_ioaccess_get_delay_cb(uint32_t device, uint64_t* time_stamp, uint64_t* delay, uint64_t* tag) {
        return IOAccessCosim::GetDelay(device, time_stamp, delay, tag);
    }
    uint64_t model_provider_ioaccess_stat_cb(uint32_t device, enum IOAccessStat stat) {
        return IOAccessCosim::GetStat(device, stat);
    }

    typedef tlm::tlm_target_socket<> InPortType;
    typedef tlm::tlm_initiator_socket<> OutPortType;

    struct DynamicModelProviderCpu
    : public VpsimIp<InPortType, OutPortType > {
    public:

        DynamicModelProviderCpu(std::string name) :
        VpsimIp(name),
        mModulePtr(nullptr) {

            registerRequiredAttribute("model");
            registerRequiredAttribute("reset_pc");
            registerRequiredAttribute("provider");
            registerRequiredAttribute("id");
            registerRequiredAttribute("quantum");
            registerRequiredAttribute("secure");
            registerRequiredAttribute("start_powered_off");

            registerRequiredAttribute("icache_size");
            registerRequiredAttribute("icache_associativity");
            registerRequiredAttribute("icache_line_size");
            // registerRequiredAttribute("intc");
        }


        NEEDS_DMI_OVERRIDE;
        PROCESSOR_OVERRIDE;

        N_IN_PORTS_OVERRIDE(0);
        N_OUT_PORTS_OVERRIDE(1);

        virtual InPortType* getNextInPort() override {
            throw runtime_error("No input ports for CPU.");
        }

        virtual OutPortType* getNextOutPort() override {
            if (!mModulePtr) {
                throw runtime_error("Please call make() before handling ports.");
            }
            return mModulePtr->mInitiatorSocket[mOutPortCounter];
        }

        void pushStats() override {
            if (mModulePtr) {

                struct ent {
                    char name[512];
                    uint64_t val;
                };
                ent* statlist;
                uint32_t count;
                mModulePtr->get_stats(mModulePtr->index, &count, (void**) &statlist);


                //printf("Initial stat push in CPU\n");
                if (mSegmentedStats.empty()) {
                    mSegmentedStats.push_back({
						{string(statlist[0].name), "0"},
						{string(statlist[1].name), "0"},
						{string(statlist[2].name), "0"},
						{string(statlist[3].name), "0"},
						{string(statlist[4].name), "0"},
						{string(statlist[5].name), "0"},
						{string(statlist[6].name), "0"},
						{string(statlist[7].name), "0"}
                    });
                }

                const auto& back = mSegmentedStats.back();

//                for (uint32_t i = 0; i < count; i++) {
//                    instructions[i] = statlist[i].val - stoull(back.at("executed_instructions"));
//                    mSegmentedStats.push_back({
//                        {"executed_instructions", to_string(instructions[i])}
//                    });
//                }

                mSegmentedStats.push_back({
                    {string(statlist[0].name), to_string(statlist[0].val - stoull(back.at(string(statlist[0].name))))},
                    {string(statlist[1].name), to_string(statlist[1].val - stoull(back.at(string(statlist[1].name))))},
                    {string(statlist[2].name), to_string(statlist[2].val - stoull(back.at(string(statlist[2].name))))},
                    {string(statlist[3].name), to_string(statlist[3].val - stoull(back.at(string(statlist[3].name))))},
                    {string(statlist[4].name), to_string(statlist[4].val - stoull(back.at(string(statlist[4].name))))},
                    {string(statlist[5].name), to_string(statlist[5].val - stoull(back.at(string(statlist[5].name))))},
                    {string(statlist[6].name), to_string(statlist[6].val - stoull(back.at(string(statlist[6].name))))},
                    {string(statlist[7].name), to_string(statlist[7].val - stoull(back.at(string(statlist[7].name))))}
                });
            }
        }

        virtual void make() override {
            if (mModulePtr != nullptr) {
                throw runtime_error("make() already called for DynamicArm");
            }
            checkAttributes();
            mModulePtr = new ModelProviderCpu(
            		getName().c_str(),
                    getAttr("model"),
                    getAttrAsUInt64("id"),
					getAttrAsUInt64("reset_pc"),
					getAttrAsUInt64("quantum"),
					getAttrAsUInt64("secure"),
					getAttrAsUInt64("start_powered_off"),

					// icache data
					/*size*/ getAttrAsUInt64("icache_size"),
					/* line size */ getAttrAsUInt64("icache_line_size"),
					/*assoc*/ getAttrAsUInt64("icache_associativity"),
					/*repl*/ LRU
            );
        }

        virtual void addDmiAddress(std::string targetIpName, uint64_t baseAddr, uint64_t size, unsigned char* pointer, bool cached, bool has_dmi) override {
        	mModulePtr->mMaps.push_back(make_tuple(pointer,baseAddr,size));
        }

        virtual void addMonitor(uint64_t base, uint64_t size) override {
            //mModulePtr->monitorRange(base,size);
        }

        virtual void removeMonitor(uint64_t base, uint64_t size) override {
            //mModulePtr->removeMonitor(base,size);
        }

        virtual void showMonitor() override {
            //mModulePtr->showMonitor();
        }

        virtual void finalize() override {
            /*VpsimIp *par=VpsimIp::Find(this->getAttr("provider"));
            if (par==nullptr)
                    throw runtime_error(getName() + ": Unfound provider :" + getAttr("provider"));
            DynamicModelProvider* mp = dynamic_cast<DynamicModelProvider*>(par);
            mModulePtr->setProvider(mp->mModulePtr);*/
            /*VpsimIp *par=VpsimIp::Find(this->getAttr("intc"));
            if (par==nullptr)
                    throw runtime_error(getName() + ": Unfound interrupt controller :" + getAttr("intc"));
            dynamic_cast<DynamicGIC*>(par)->connectCpu(getIrqIf(),getAttrAsUInt64("id"));*/
        }

        /*virtual VpsimModule* asModule() {
                return mModulePtr;
        }*/

        virtual void setStatsAndDie() override {
            if (mModulePtr) {

                struct ent {
                    char name[512];
                    uint64_t val;
                };

                ent* statlist;
                uint32_t count;

                mModulePtr->get_stats(mModulePtr->index, &count, (void**) &statlist);
                for (unsigned i = 0; i < count; i++) {
                    mStats[string(statlist[i].name)] = to_string(statlist[i].val);
                }

                delete mModulePtr;
            }
        }

        void show() override {
            mModulePtr->show_cpu();
        }

        InterruptIf *getIrqIf() override {
            return mModulePtr;
        }


        ModelProviderCpu* mModulePtr;
    };

    struct DynamicModelProviderDev
    : public VpsimIp<InPortType, OutPortType > {
    public:

        DynamicModelProviderDev(std::string name) :
        VpsimIp(name),
        mModulePtr(nullptr) {

            registerRequiredAttribute("model");
            registerRequiredAttribute("base_address");
            registerRequiredAttribute("size");
            registerRequiredAttribute("irq");
            registerRequiredAttribute("provider");
        }


        // MEMORY_MAPPED;

        N_IN_PORTS_OVERRIDE(0);
        N_OUT_PORTS_OVERRIDE(0);

        virtual InPortType* getNextInPort() override {
            throw runtime_error("model provider device has no output ports.");
            // return &mModulePtr->mTargetSocket;
        }

        virtual OutPortType* getNextOutPort() override {
            throw runtime_error("model provider device has no output ports.");
        }

        virtual void make() override {
            if (mModulePtr != nullptr) {
                throw runtime_error("make() already called for DynamicArm");
            }
            checkAttributes();
            mModulePtr = new ModelProviderDev(getName().c_str(),
                    getAttr("model"), getAttrAsUInt64("base_address"), getAttrAsUInt64("size"), getAttrAsUInt64("irq"));
        }

                /*
                virtual uint64_t getBaseAddress() override {
                        return getAttrAsUInt64("base_address");
                }

                virtual uint64_t getSize() override {
                        return getAttrAsUInt64("size");
                }*/

        virtual void addDmiAddress(std::string targetIpName, uint64_t baseAddr, uint64_t size, unsigned char* pointer, bool cached, bool has_dmi) override {
        }

        virtual void addMonitor(uint64_t base, uint64_t size) override {
        }

        virtual void removeMonitor(uint64_t base, uint64_t size) override {
        }

        virtual void showMonitor() override {
        }

        virtual void finalize() override {

        }

        virtual void setStatsAndDie() override {
            if (mModulePtr) {
                delete mModulePtr;
            }
        }



        ModelProviderDev* mModulePtr;
    };

    struct DynamicModelProviderParam1 : public VpsimIp<InPortType, OutPortType> {

        DynamicModelProviderParam1(std::string name) :
        VpsimIp(name) {
            registerRequiredAttribute("option");
            registerRequiredAttribute("provider");
        }

        N_IN_PORTS_OVERRIDE(0);
        N_OUT_PORTS_OVERRIDE(0);

        virtual InPortType* getNextInPort() override {
            throw runtime_error("No input ports for model provider param.");
        }

        virtual OutPortType* getNextOutPort() override {
            throw runtime_error("no output ports for model provider param.");

        }

        virtual void make() override {

            checkAttributes();
        }

        virtual void addDmiAddress(std::string targetIpName, uint64_t baseAddr, uint64_t size, unsigned char* pointer, bool cached, bool has_dmi) override {
        }

        virtual void addMonitor(uint64_t base, uint64_t size) override {
        }

        virtual void removeMonitor(uint64_t base, uint64_t size) override {
        }

        virtual void showMonitor() override {
        }

        virtual void finalize() override {
            /*VpsimIp *par=VpsimIp::Find(this->getAttr("provider"));
            if (par==nullptr)
                    throw runtime_error(getName() + ": Unfound provider :" + getAttr("provider"));
            DynamicModelProvider* mp = dynamic_cast<DynamicModelProvider*>(par);
            mp->mModulePtr->addParam1(getAttr("option"));*/
        }

        virtual void setStatsAndDie() override {
        }
    };

    struct DynamicModelProviderParam2 : public VpsimIp<InPortType, OutPortType> {

        DynamicModelProviderParam2(std::string name) :
        VpsimIp(name) {
            registerRequiredAttribute("option");
            registerRequiredAttribute("value");
            registerRequiredAttribute("provider");
        }

        N_IN_PORTS_OVERRIDE(0);
        N_OUT_PORTS_OVERRIDE(0);

        virtual InPortType* getNextInPort() override {
            throw runtime_error("No input ports for model provider param.");
        }

        virtual OutPortType* getNextOutPort() override {
            throw runtime_error("no output ports for model provider param.");

        }

        virtual void make() override {

            checkAttributes();
        }

        virtual void addDmiAddress(std::string targetIpName, uint64_t baseAddr, uint64_t size, unsigned char* pointer, bool cached, bool has_dmi) override {
        }

        virtual void addMonitor(uint64_t base, uint64_t size) override {
        }

        virtual void removeMonitor(uint64_t base, uint64_t size) override {
        }

        virtual void showMonitor() override {
        }

        virtual void finalize() override {
            /*VpsimIp *par=VpsimIp::Find(this->getAttr("provider"));
            if (par==nullptr)
                    throw runtime_error(getName() + ": Unfound provider :" + getAttr("provider"));
            DynamicModelProvider* mp = dynamic_cast<DynamicModelProvider*>(par);
            mp->mModulePtr->addParam2(getAttr("option"),getAttr("value"));*/
        }

        virtual void setStatsAndDie() override {
        }
    };

    struct DynamicModelProvider : public VpsimIp<InPortType, OutPortType> {

        DynamicModelProvider(std::string name) :
        VpsimIp(name),
        mModulePtr(nullptr) {

            registerRequiredAttribute("path");
            registerRequiredAttribute("io_poll_period");
            registerOptionalAttribute("quantum","1000");
            registerRequiredAttribute("notify_main_memory_access");
            registerOptionalAttribute("roi_only","1");


            registerRequiredAttribute("simulate_icache");

            registerRequiredAttribute("notify_ioaccess");
        }

        NEEDS_DMI_OVERRIDE;

        N_IN_PORTS_OVERRIDE(0);
        N_OUT_PORTS_OVERRIDE(0);

        virtual InPortType* getNextInPort() override {
            throw runtime_error("No input ports for model provider.");
        }

        virtual OutPortType* getNextOutPort() override {
            throw runtime_error("no output ports for model provider.");

        }

        virtual void make() override {
            if (mModulePtr != nullptr) {
                throw runtime_error("make() already called for DynamicArm");
            }
            checkAttributes();
            mModulePtr = new ModelProvider(getName().c_str(), getAttr("path"), getAttrAsUInt64("io_poll_period"), getAttrAsUInt64("quantum"));


            mModulePtr->set_default_read_callback(model_provider_read_cb);
            mModulePtr->set_default_write_callback(model_provider_write_cb);
            mModulePtr->set_sync_callback(model_provider_sync);
            mModulePtr->modelprovider_unlock(model_provider_unlock_cb, (void*) mModulePtr);
            mModulePtr->modelprovider_wait_unlock(model_provider_wait_unlock_cb, (void*) mModulePtr);
            mModulePtr->modelprovider_register_fill_bias_cb(&ModelProvider::get_cpu_biases);

            if (getAttrAsUInt64("notify_main_memory_access")) {
                if (!getAttrAsUInt64("roi_only")) mModulePtr->modelprovider_register_main_mem_callback(model_provider_main_mem_cb, mModulePtr->quantum);
                else MainMemCosim::addRegisterMainMemCb(mModulePtr->modelprovider_register_main_mem_callback, model_provider_main_mem_cb, mModulePtr->quantum, mModulePtr->modelprovider_unregister_main_mem_callback);
                mModulePtr->modelprovider_register_outer_stat_cb(model_provider_outer_stat_cb);
            }

            if (getAttrAsUInt64("simulate_icache")) {
                mModulePtr->modelprovider_register_icache_miss_cb(model_provider_fetch_miss_cb);
                mModulePtr->modelprovider_register_add_victim_cb(&StandaloneInstructionCache::AppendVictim);
            }

            if (getAttrAsUInt64("notify_ioaccess")) {
                mModulePtr->modelprovider_register_ioaccess_callback(model_provider_ioaccess_cb);
                mModulePtr->modelprovider_register_ioaccess_get_delay_cb(model_provider_ioaccess_get_delay_cb);
                mModulePtr->modelprovider_register_ioaccess_stat_cb(model_provider_ioaccess_stat_cb);
            }
        }

        virtual void addDmiAddress(std::string targetIpName, uint64_t baseAddr, uint64_t size, unsigned char* pointer, bool cached, bool has_dmi) override {
            if (mModulePtr == nullptr) {
                throw runtime_error(getName() + " : calling addDmiAddress() before make() !!!");
            }
            if (!mModulePtr->configured) {
                VpsimIp::MapTypeIf("ModelProviderParam1",
                        [this](VpsimIp * target) {
                            return target->getAttr("provider") == this->getName();
                        },
                [this](VpsimIp * target) {
                    this->mModulePtr->addParam1(target->getAttr("option"));
                });
                VpsimIp::MapTypeIf("ModelProviderParam2",
                        [this](VpsimIp * target) {
                            return target->getAttr("provider") == this->getName();
                        },
                [this](VpsimIp * target) {
                    this->mModulePtr->addParam2(target->getAttr("option"), target->getAttr("value"));
                });

                mModulePtr->config();
            }
            if (has_dmi) {
                mModulePtr->declare_external_ram((char*) targetIpName.c_str(), baseAddr, size, pointer);
            } else {
                mModulePtr->declare_external_dev((char*) targetIpName.c_str(), baseAddr, size);
            }
        }

        virtual void addMonitor(uint64_t base, uint64_t size) override {
            //mModulePtr->monitorRange(base,size);
        }

        virtual void removeMonitor(uint64_t base, uint64_t size) override {
            //mModulePtr->removeMonitor(base,size);
        }

        virtual void showMonitor() override {
            //mModulePtr->showMonitor();
        }

        virtual void finalize() override {
            // gather all params !
            if (!mModulePtr->configured) {
                VpsimIp::MapTypeIf("ModelProviderParam1",
                        [this](VpsimIp * target) {
                            return target->getAttr("provider") == this->getName();
                        },
                [this](VpsimIp * target) {
                    this->mModulePtr->addParam1(target->getAttr("option"));
                });
                VpsimIp::MapTypeIf("ModelProviderParam2",
                        [this](VpsimIp * target) {
                            return target->getAttr("provider") == this->getName();
                        },
                [this](VpsimIp * target) {
                    this->mModulePtr->addParam2(target->getAttr("option"), target->getAttr("value"));
                });

                mModulePtr->config();
            }
            VpsimIp::MapTypeIf("ModelProviderDev",
                    [this](VpsimIp * target) {
                        return target->getAttr("provider") == this->getName();
                    },
            [this](VpsimIp * target) {
                dynamic_cast<DynamicModelProviderDev*> (target)->mModulePtr->setProvider(this->mModulePtr);
            });


            VpsimIp::MapTypeIf("ModelProviderCpu",
                    [this](VpsimIp * target) {
                        return target->getAttr("provider") == this->getName();
                    },
            [this](VpsimIp * target) {
                dynamic_cast<DynamicModelProviderCpu*> (target)->mModulePtr->setProvider(this->mModulePtr);
            });

            mModulePtr->finalize_config();

        }

        virtual void setStatsAndDie() override {
            if (mModulePtr) {
                delete mModulePtr;
            }
        }

        InterruptIf *getIrqIf() override {
            return mModulePtr;
        }

        ModelProvider* mModulePtr;
    };

}

#endif /* _MODELPROVIDER_HPP_ */

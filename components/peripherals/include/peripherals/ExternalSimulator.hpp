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

#ifndef _EXTERNALSIMULATOR_HPP_
#define _EXTERNALSIMULATOR_HPP_

#include <core/TargetIf.hpp>
#include <core/TlmCallbackPrivate.hpp>
#include "InterruptSource.hpp"
#include <dlfcn.h>

namespace vpsim {

enum RemoteTransactionType{
	REMOTE_WRITE,
	REMOTE_READ
};

enum RemoteResponseType {
	REMOTE_WRITE_OK,
	REMOTE_READ_OK,
	REMOTE_WRITE_ERR,
	REMOTE_READ_ERR
};

	typedef void (*InterruptCb)(void * opaque, uint32_t line, uint32_t value);
	typedef void (*SynchroCb)(void * opaque, uint64_t executed, bool wait_for_event);

	typedef uint32_t (*remote_access_t)(uint8_t, uint64_t, uint64_t, uint8_t*);
	typedef void (*set_external_simulator_t)(void * proxy);
	typedef void (*config_external_simulator_t)(int argc, char** param);
	typedef void (*register_sync_callback_t)(SynchroCb);
	typedef void (*register_irq_callback_t)(InterruptCb);
	typedef void (*run_simulator_t)(void);
	typedef uint32_t (*write_simulator_t) (uint64_t address, uint64_t size, uint8_t* data);
	typedef uint32_t (*read_simulator_t) (uint64_t address, uint64_t size, uint8_t* data);

#define LDFCT(typ,nm) nm=(typ##_t)loadSymbol(#typ)

class ExternalSimulator : public sc_module, public TargetIf<uint8_t>, public InterruptSource {
public:
	ExternalSimulator(sc_module_name name, size_t size, string path);
	virtual ~ExternalSimulator();

	tlm::tlm_response_status read (payload_t & payload, sc_time & delay);
	tlm::tlm_response_status write (payload_t & payload, sc_time & delay);

	virtual void interrupt(uint32_t line, uint32_t value) {
		if (line != InterruptSource::mInterruptLine) {
			throw runtime_error("RemoteTarget: received interrupt with mismatching line number !");
		}
		if (value)
			raiseInterrupt();
		else
			lowerInterrupt();
	}

    void *loadSymbol(string sym) {
        if (!lib)
            throw runtime_error("getting symbol from null library !");

        dlerror();
        void* ptr = dlsym(lib, sym.c_str());
        if (!ptr) {
            dlerror();
            throw runtime_error(string("External Simulator: unable to load symbol: ") + sym);
        }
        return ptr;
    }

    void SimThread(){
    	run();
    }

	void sync(uint64_t executed, bool wait_for_event){
		wait(executed, SC_NS);
	}

	void addParam(string arg) {
		argv.push_back(arg);
	}
	void config() {
		if (configured)
			return;
		char** argv_c = (char**) malloc(sizeof (char*)*argv.size());
		int i = 0;
		for (string& arg : argv) {
			argv_c[i++] = strdup(arg.c_str());
		}
		config_external_simulator(i, argv_c);
		configured = true;
	}

	SC_HAS_PROCESS(ExternalSimulator);

	void* lib;
	bool configured;
	vector<string> argv;

	set_external_simulator_t set_external_simulator;
	config_external_simulator_t config_external_simulator;
	run_simulator_t run;
	remote_access_t remote_access;
	register_irq_callback_t register_irq_cb;
	register_sync_callback_t register_sync_cb;
	write_simulator_t write_ext_access;
	read_simulator_t read_ext_access;
	};

	void external_simulator_interrupt_cb(void * opaque, uint32_t line, uint32_t value);
	void external_simulator_sync_cb(void *opaque, uint64_t executed, bool wait_for_event);

} /* namespace vpsim */


#endif /* COMPONENTS_PERIPHERALS_INCLUDE_PERIPHERALS_EXTERNALSIMULATOR_HPP_ */

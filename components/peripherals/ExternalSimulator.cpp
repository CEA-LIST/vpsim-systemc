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

#include <ExternalSimulator.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

namespace vpsim {

ExternalSimulator::ExternalSimulator(sc_module_name name, size_t size, string path):
		sc_module(name),
		TargetIf(string(name),size),
		lib(nullptr),
		configured(false)
{
	cout<<"Opening library: "<< path <<endl;
	lib = dlopen(path.c_str(), RTLD_LOCAL | RTLD_LAZY);
	if (!lib) {
		throw runtime_error(string("Could not load External Simulator : ") + dlerror());
	}

	LDFCT(set_external_simulator, set_external_simulator);
	LDFCT(config_external_simulator, config_external_simulator);
	LDFCT(run_simulator, run);
	LDFCT(register_sync_callback, register_sync_cb);
	LDFCT(register_irq_callback, register_irq_cb);
	LDFCT(write_simulator, write_ext_access);
	LDFCT(read_simulator, read_ext_access);

	TargetIf <REG_T>::RegisterReadAccess(REGISTER(ExternalSimulator,read));
	TargetIf <REG_T>::RegisterWriteAccess(REGISTER(ExternalSimulator,write));

	SC_THREAD(SimThread);
}

ExternalSimulator::~ExternalSimulator() {
}

tlm::tlm_response_status ExternalSimulator::read (payload_t & payload, sc_time & delay) {
	if (!payload.ptr) {
		throw runtime_error("External Simulator does not support null payloads !");
	}

	// This line should be replaced
	if (read_ext_access(payload.addr, payload.len, payload.ptr)==REMOTE_READ_OK) {
		return tlm::TLM_OK_RESPONSE;
	} else {
		return tlm::TLM_ADDRESS_ERROR_RESPONSE;
	}
}

tlm::tlm_response_status ExternalSimulator::write (payload_t & payload, sc_time & delay) {
	if (!payload.ptr) {
		throw runtime_error("External Simulator does not support null payloads !");
	}

	// This line should be replaced
	if (write_ext_access(payload.addr, payload.len, payload.ptr)==REMOTE_WRITE_OK) {
		return tlm::TLM_OK_RESPONSE;
	} else {
		return tlm::TLM_ADDRESS_ERROR_RESPONSE;
	}
}

void external_simulator_interrupt_cb(void * opaque, uint32_t line, uint32_t value){
	ExternalSimulator* sim = (ExternalSimulator*) opaque;
	sim->interrupt(line, value);
}

void external_simulator_sync_cb(void *opaque, uint64_t executed, bool wait_for_event){
	ExternalSimulator* sim = (ExternalSimulator*) opaque;
	sim->sync(executed, wait_for_event);
}

} /* namespace vpsim */

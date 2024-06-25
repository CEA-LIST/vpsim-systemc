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

#ifndef _REMOTEINITIATOR_HPP_
#define _REMOTEINITIATOR_HPP_

#include "RemoteTransaction.hpp"
#include "InitiatorIf.hpp"
#include "InterruptIf.hpp"

namespace vpsim {

class RemoteInitiator : public sc_module,public InitiatorIf,public GenericRemoteInitiator, public InterruptIf {
public:
	RemoteInitiator(sc_module_name name): sc_module(name),InitiatorIf(string(name),0,true,1) {
		SC_THREAD(riPoll);
	}
	virtual ~RemoteInitiator() {}


	void riPoll() {
		while (true) {
			wait(mPollPeriod,SC_NS);
			poll();
		}
	}

	virtual uint32_t localRead(uint64_t addr, uint64_t size, uint8_t* data) override {
		sc_time t;
		auto status = InitiatorIf::target_mem_access(0, addr, size, data, READ, t);
		if (status != tlm::TLM_OK_RESPONSE) {
			completeRead(REMOTE_READ_ERR, data);
			return REMOTE_READ_ERR;
		}
		else {
			completeRead(REMOTE_READ_OK, data);
			return REMOTE_READ_OK;
		}
	}

	virtual uint32_t localWrite(uint64_t addr, uint64_t size, uint8_t* data) override {
		sc_time t;
		auto status = InitiatorIf::target_mem_access(0, addr, size, data, WRITE, t);
		if (status != tlm::TLM_OK_RESPONSE) {
			completeWrite(REMOTE_WRITE_ERR);
			return REMOTE_WRITE_ERR;
		}
		else {
			completeWrite(REMOTE_WRITE_OK);
			return REMOTE_WRITE_OK;
		}
	}

	virtual void update_irq(uint64_t value, uint32_t line) override {
		interrupt(line, value);
	}

	SC_HAS_PROCESS(RemoteInitiator);
};

} /* namespace vpsim */

#endif /* _REMOTEINITIATOR_HPP_ */

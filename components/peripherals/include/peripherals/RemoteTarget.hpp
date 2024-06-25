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

#ifndef _REMOTETARGET_HPP_
#define _REMOTETARGET_HPP_

#include <core/TargetIf.hpp>
#include <core/TlmCallbackPrivate.hpp>
#include "ChannelManager.hpp"
#include "RemoteTransaction.hpp"
#include "InterruptSource.hpp"

namespace vpsim {

class RemoteTarget : public sc_module, public TargetIf<uint8_t>, public GenericRemoteTarget, public InterruptSource {
public:
	RemoteTarget(sc_module_name name, size_t size);
	virtual ~RemoteTarget();

	tlm::tlm_response_status read (payload_t & payload, sc_time & delay);
	tlm::tlm_response_status write (payload_t & payload, sc_time & delay);

	virtual void interrupt(uint32_t line, uint32_t value) override {
		// cout.clear(); cout<<"remote target interrupting local cpu."<<endl;
		if (line != InterruptSource::mInterruptLine) {
			throw runtime_error("RemoteTarget: received interrupt with mismatching line number !");
		}
		if (value)
			raiseInterrupt();
		else
			lowerInterrupt();
	}


	void rtPoll() {
		while (true) {
			wait(mPollPeriod,SC_NS);
			poll();
		}
	}

	SC_HAS_PROCESS(RemoteTarget);
};

} /* namespace vpsim */

#endif /* _REMOTETARGET_HPP_ */

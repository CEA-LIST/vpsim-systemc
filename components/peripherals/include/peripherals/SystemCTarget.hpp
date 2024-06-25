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

#ifndef _SYSTEMCTARGET_HPP_
#define _SYSTEMCTARGET_HPP_
#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>
#include <core/TlmCallbackPrivate.hpp>
#include <functional>
#include <core/TargetIf.hpp>
#include "paramManager.hpp"
#include "InterruptIf.hpp"
#include "InterruptSource.hpp"

namespace vpsim{
class SystemCTarget: public sc_module, public TargetIf<uint8_t>, public InterruptSource  {
public:

	SystemCTarget(sc_module_name name, uint64_t size): sc_module(name), TargetIf(string(name),size) {
		TargetIf <uint8_t>::RegisterReadAccess(REGISTER(SystemCTarget,read));
		TargetIf <uint8_t>::RegisterWriteAccess(REGISTER(SystemCTarget,write));

		_int = [this] (int line, int value) -> void {
			setInterruptLine(line);
			if (value)
				raiseInterrupt();
			else
				lowerInterrupt();
		};
	}

	tlm::tlm_response_status read (payload_t & payload, sc_time & delay){
		_out->b_transport(*payload.original_payload, delay);
		return payload.original_payload->get_response_status();
	}
	tlm::tlm_response_status write (payload_t & payload, sc_time & delay){
		_out->b_transport(*payload.original_payload, delay);
		return payload.original_payload->get_response_status();
	}

	tlm_utils::simple_initiator_socket<SystemCTarget> _out;
	std::function<void(int,int)> _int;
};
}


#endif /* _SYSTEMCTARGET_HPP_ */

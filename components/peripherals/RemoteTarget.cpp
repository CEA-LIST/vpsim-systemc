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

#include <RemoteTarget.hpp>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

namespace vpsim {

RemoteTarget::RemoteTarget(sc_module_name name, size_t size): sc_module(name),TargetIf(string(name),size) {
	TargetIf <REG_T>::RegisterReadAccess(REGISTER(RemoteTarget,read));
	TargetIf <REG_T>::RegisterWriteAccess(REGISTER(RemoteTarget,write));
	SC_THREAD(rtPoll);
}

RemoteTarget::~RemoteTarget() {
}

tlm::tlm_response_status RemoteTarget::read (payload_t & payload, sc_time & delay) {
	if (!payload.ptr) {
		throw runtime_error("Remote Target does not support null payloads !");
	}

	if (remoteRead(payload.addr, payload.len, payload.ptr)==REMOTE_READ_OK) {
		return tlm::TLM_OK_RESPONSE;
	} else {
		return tlm::TLM_ADDRESS_ERROR_RESPONSE;
	}

}

tlm::tlm_response_status RemoteTarget::write (payload_t & payload, sc_time & delay) {
	if (!payload.ptr) {
		throw runtime_error("Remote Target does not support null payloads !");
	}

	if (remoteWrite(payload.addr, payload.len, payload.ptr)==REMOTE_WRITE_OK) {
		return tlm::TLM_OK_RESPONSE;
	} else {
		return tlm::TLM_ADDRESS_ERROR_RESPONSE;
	}
}

} /* namespace vpsim */

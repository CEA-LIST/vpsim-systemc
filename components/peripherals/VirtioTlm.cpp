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

#include <VirtioTlm.hpp>
#include <core/TlmCallbackPrivate.hpp>

namespace vpsim {

VirtioTlm::VirtioTlm(sc_module_name name) :
		sc_module(name), TargetIf(string(name), 0x10000), mRdFct(nullptr), mWrFct(nullptr), mProxyPtr(nullptr) {
	TargetIf <REG_T>::RegisterReadAccess(REGISTER(VirtioTlm,read));
	TargetIf <REG_T>::RegisterWriteAccess(REGISTER(VirtioTlm,write));

	// SC_THREAD(main);

}

tlm::tlm_response_status VirtioTlm::read (payload_t & payload, sc_time & delay) {
	if (!mProxyPtr) {
		throw runtime_error("VIRTIO: Provider Proxy Pointer was not initialized.");
	}
	if (!mRdFct) {
		throw runtime_error("VIRTIO: Read function was not initialized.");
	}

	uint64_t data = mRdFct(mProxyPtr, payload.addr-getBaseAddress(), payload.len);
	memcpy(payload.ptr, &data, payload.len);

	//cout<<"Virtio-read: "<<hex<<payload.addr-getBaseAddress()<<" val: "<<(uint64_t)data<<dec<<endl;

	return tlm::TLM_OK_RESPONSE;
}

tlm::tlm_response_status VirtioTlm::write (payload_t & payload, sc_time & delay) {
	if (!mProxyPtr) {
		throw runtime_error("VIRTIO: Provider Proxy Pointer was not initialized.");
	}
	if (!mWrFct) {
		throw runtime_error("VIRTIO: Write function was not initialized.");
	}
	uint64_t tmp=0;
	memcpy(&tmp, payload.ptr, payload.len);
	mWrFct(mProxyPtr, payload.addr-getBaseAddress(), tmp, payload.len);

	if(mIoStep)
		mIoStep();

	//cout<<"Virtio-write: "<<hex<<payload.addr-getBaseAddress()<<" val: "<<(uint64_t)tmp<<dec<<endl;

	return tlm::TLM_OK_RESPONSE;
}

void VirtioTlm::main() {
	if (!mIoStep)
		throw runtime_error("IOStep function not set (VirtioTlm).");

	while (true) {
		wait(tlm::tlm_global_quantum::instance().get());
		mIoStep();
	}
}

} /* namespace vpsim */

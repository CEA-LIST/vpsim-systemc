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

#ifndef _VIRTIOTLM_HPP_
#define _VIRTIOTLM_HPP_

#include <cstdint>
#include <iostream>
#include <algorithm>
#include <functional>

#include <core/TargetIf.hpp>
#include <queue>
#include "paramManager.hpp"

typedef
uint64_t (*virtio_mmio_read_type)(void *opaque, uint64_t offset, unsigned size);

typedef
void (*provider_io_step)();

typedef
void (*virtio_mmio_write_type)(void *opaque, uint64_t offset, uint64_t value,
		                              unsigned size);

namespace vpsim {

class VirtioTlm: public sc_module, public TargetIf<uint8_t>  {
public:
	VirtioTlm(sc_module_name name);
	tlm::tlm_response_status read (payload_t & payload, sc_time & delay);
	tlm::tlm_response_status write (payload_t & payload, sc_time & delay);

	SC_HAS_PROCESS(VirtioTlm);

	void main();

	virtio_mmio_read_type mRdFct;
	virtio_mmio_write_type mWrFct;
	provider_io_step mIoStep;
	void *mProxyPtr;
};

} /* namespace vpsim */

#endif /* _VIRTIOTLM_HPP_ */

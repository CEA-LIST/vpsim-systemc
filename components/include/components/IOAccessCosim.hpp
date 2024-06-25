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

#ifndef _IOACCESSCOSIM_HPP_
#define _IOACCESSCOSIM_HPP_

#include <systemc>
#include <vector>
#include <tlm>
#include "tlm_utils/simple_initiator_socket.h"
#include "CosimExtensions.hpp"
#include "readerwriterqueue.h"
#include <map>
#include <tuple>
#include <cstdio>
#include <cinttypes>

using namespace std;
using namespace moodycamel;

using namespace tlm;

namespace vpsim {

enum IOAccessStat {
	IOACCESS_READ=0,
	IOACCESS_WRITE
};


class IOAccessCosim {

public:
	IOAccessCosim(){}

	virtual ~IOAccessCosim() {}
	static uint8_t GetDelay(uint32_t device, uint64_t* time_stamp, uint64_t* delay, uint64_t* tag) {
		std::tuple<uint64_t,uint64_t,uint64_t> k;
		if(_Q_Accomplished[device]->try_dequeue(k)){
			*time_stamp = get<0>(k);
			*delay = get<1>(k);
			*tag = get<2>(k);
			return 1;
		}
		return 0;
	}

	static uint64_t GetStat(uint32_t device, IOAccessStat st) {
		return 0;
	}

	virtual void insert(uint32_t device, uint8_t write, void* phys, unsigned int size, uint64_t time_stamp, uint64_t tag)=0;
  protected:
	static vector<ReaderWriterQueue<std::tuple<uint64_t,uint64_t,uint64_t>,512>*> _Q_Accomplished;
};

class IOAccessCosimulator: public sc_module, public IOAccessCosim {
public:
	IOAccessCosimulator(sc_module_name name, uint32_t outPorts): sc_module(name) {
		mOutPorts.resize(outPorts);
		for (uint32_t i = 0; i < outPorts; i++) {
			mOutPorts[i] = new tlm_utils::simple_initiator_socket<IOAccessCosimulator>((string("dma_port_")+to_string(i)).c_str());//+string(name)
		}

		_Q_Accomplished.resize(32);
		for (uint32_t i = 0; i < outPorts; i++) {
			_Q_Accomplished[i] = new ReaderWriterQueue<std::tuple<uint64_t,uint64_t,uint64_t>,512>(4096*4096);
		}
	}

	virtual ~IOAccessCosimulator() {
		for (uint32_t i = 0; i < _Q_Accomplished.size(); i++) {
			delete _Q_Accomplished[i];
		}
	}

	bool convertAddr(void* host, uint64_t* p) {
		uint64_t H=(uint64_t)host;
		for (auto& t: mMaps) {
			if(get<0>(t)==(void*)0)
			{
				uint64_t Hp=(uint64_t)get<1>(t);
				if (H>=Hp && H < Hp+get<2>(t)){
					*p = H;
					return true;
				}
			}
			else{
				uint64_t Hp=(uint64_t)get<0>(t);
				if (H>=Hp && H < Hp+get<2>(t)){
					*p = get<1>(t) + (H-Hp);
					return true;
				}
			}
		}
		return false;
	}


	using portType=tlm_utils::simple_initiator_socket<IOAccessCosimulator>;

	virtual void insert(uint32_t device, uint8_t write, void* phys, unsigned int size, uint64_t time_stamp, uint64_t tag) override {
		uint64_t addr=0;
		convertAddr(phys,&addr);
		pld.set_data_ptr(NULL);
		pld.set_address(addr);
		pld.set_data_length(size);
		pld.set_command((write?tlm::TLM_WRITE_COMMAND:tlm::TLM_READ_COMMAND));
		src.type=1;//The source is a device (other than a cpu)
		src.device_id=device;
		src.time_stamp=sc_time((double)time_stamp,SC_NS);
		pld.set_extension<SourceDeviceExtension>(&src);
		delay=SC_ZERO_TIME;
		portType& p=*mOutPorts[device];
		p->b_transport(pld, delay);
		pld.clear_extension(&src);

		// enqueue request
		while(!_Q_Accomplished[device]->try_enqueue(make_tuple(src.time_stamp.value(),delay.value(), tag)))
			;
	}

	vector<portType*> mOutPorts;
	tlm::tlm_generic_payload pld;
	SourceDeviceExtension src;
	vector< tuple<void*,uint64_t,uint64_t> > mMaps;
	sc_time delay;
};

}

#endif /* _IOACCESSCOSIM_HPP_ */

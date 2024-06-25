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

#ifndef _XUARTPS_HPP_
#define _XUARTPS_HPP_

#include <cstdint>
#include <iostream>
#include <algorithm>
#include <functional>

#include <core/TargetIf.hpp>
#include <queue>
#include "paramManager.hpp"
#include "CommonUartInterface.hpp"

#include "reg"

#define FIFO_SIZE 64

namespace vpsim {

class xuartps: public CommonUartInterface,
				public TargetIf<uint8_t> {
public:
	xuartps(sc_module_name name);

	tlm::tlm_response_status read (payload_t & payload, sc_time & delay);
	tlm::tlm_response_status write (payload_t & payload, sc_time & delay);

	bool mRxEnable, mTxEnable;

	virtual bool inputReady() override {
		while(CommonUartInterface::inputReady()) {
			char b = CommonUartInterface::readByte();
			mInFifo.push(b);
		}

		return (mInFifo.size() /*&& mRxEnable*/);
	}

	virtual char readByte() override {
		if (!inputReady())
			throw runtime_error("CDNS: Trying to read empty FIFO !");
		char b = mInFifo.front();
		mInFifo.pop();
		return b;
	}


	bool isFifoFull() {
		return inputReady() && mInFifo.size() >= FIFO_SIZE;
	}

	bool isFifoEmpty() {
		return !inputReady();
	}

	bool isFifoOver() {
		auto trigger = *(uint32_t*)&getLocalMem()[0x20];
		return inputReady() && mInFifo.size() >= trigger;
	}

private:
	std::queue<char> mInFifo;
};

} /* namespace vpsim */

#endif /* _XUARTPS_HPP_ */

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

#include <PL011Uart.hpp>
#include <core/TlmCallbackPrivate.hpp>
#include <poll.h>
#include <unistd.h>
#include "reg"

extern ssize_t (*SysRead)(int fd, void *buf, size_t count);

namespace vpsim {

using namespace std;
using namespace tlm;


PL011Uart::PL011Uart(sc_module_name name):
		CommonUartInterface(name),
		TargetIf<uint32_t>(string(name),0x1000)
{
	TargetIf <REG_T>::RegisterReadAccess(REGISTER(PL011Uart,read));
	TargetIf <REG_T>::RegisterWriteAccess(REGISTER(PL011Uart,write));

	mRxReady=0;
}

tlm::tlm_response_status PL011Uart::read (payload_t & payload, sc_time & delay) {
	//delay += sc_time(50,SC_NS);

	cout.clear(); cout<<"ACCESS READ .\n"<<hex<<payload.addr<<endl;

	struct pollfd f;
	int t;
	switch (payload.addr - getBaseAddress()) {
	case 0:
		*(uint32_t*)payload.ptr=0;
		*(char*)payload.ptr=readByte();
		//delay += sc_time(70,SC_US);

		break;
	case 0x4:
		*(uint32_t*)payload.ptr = 0;
		break;
	case 0x18:
		 // transmit and receive always ready !

		t=inputReady();
		if (t==0) {
			mRxReady=(0<<6)|(1<<4);
		} else {
			mRxReady=(1<<6)|(0<<4);
		}

		*(uint32_t*)payload.ptr = 0x80 | mRxReady;
		break;

	case 0x03C: // raw int
		if(inputReady()) {
			getLocalMem()[0x03C/4] |= (1<<4);
		}
		break;

	case 0x040: // masked int
		if(inputReady()) {
			getLocalMem()[0x03C/4] |= (1<<4);
		}
		*(uint32_t*)payload.ptr = getLocalMem()[0x03C/4] & getLocalMem()[0x038/4];
		break;
	default:
		*(uint32_t*)payload.ptr = 0;
		//throw runtime_error(to_string(payload.addr)+" -> Reading from invalid UART offset");
	}



	return TLM_OK_RESPONSE;
}

tlm::tlm_response_status PL011Uart::write (payload_t & payload, sc_time & delay) {
	//delay += sc_time(50,SC_NS);
	cout.clear(); cout<<"ACCESS WRITE .\n"<<hex<<payload.addr<<endl;
	IMPL_REG;

	switch (payload.addr - getBaseAddress()) {
	case 0:
		//std::cout<<*(char*)payload.ptr<<flush;
		writeByte(*(char*)payload.ptr);
		getLocalMem()[0x03C/4] |= (1<<5);
		if (mOutIntEnable) {
			mOutInt=true;
			cout.clear(); cout<<"interrupting after write.\n"<<endl;
		}
		//delay += sc_time(70,SC_US);
		break;

	case 0x038: //mask
		getLocalMem()[0x38/4]=*(uint32_t*) payload.ptr;
		if (getLocalMem()[0x38/4] & (1<<5)) {
			mOutIntEnable=true;
			mOutInt=true;
			cout.clear(); cout<<"TX interrupt enabled !\n"<<endl;
		}
		else {
			mOutIntEnable=false;
		}
		if (getLocalMem()[0x38/4] & (1<<4)) {
			mIntEnable=true;
			cout.clear(); cout<<"RX interrupt enabled !\n"<<endl;
		}
		else {
			mIntEnable=false;
		}
		break;
	case 0x044: // int clr
		getLocalMem()[0x03C/4] &= ~ *(uint32_t*)payload.ptr;
		if (!(getLocalMem()[0x3C/4] & (1<<5))) {
			mOutInt=false;
		}
		break;
	default:
		break;
		//throw runtime_error(to_string(payload.addr) + " -> Writing to invalid UART offset");
	}

	END_IMPL_REG;

	return TLM_OK_RESPONSE;
}

} /* namespace vpsim */

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

#include "xuartps.hpp"
#include <core/TlmCallbackPrivate.hpp>
#include <poll.h>
#include <unistd.h>

#include "log.hpp"

namespace vpsim {

using namespace std;
using namespace tlm;

xuartps::xuartps(sc_module_name name):
		CommonUartInterface(name),
		TargetIf(string(name),0x1000)
{
	TargetIf <REG_T>::RegisterReadAccess(REGISTER(xuartps,read));
	TargetIf <REG_T>::RegisterWriteAccess(REGISTER(xuartps,write));

	*(uint32_t*)&getLocalMem()[0x18]=0x0000028B;
	*(uint32_t*)&getLocalMem()[0x34]=0x0000000F;
}


tlm::tlm_response_status xuartps::read (payload_t & payload, sc_time & delay) {
	IMPL_REG;

	LOG_DEBUG(dbg2)<<getName()<<hex<<"read: "<<" addr: "<<payload.addr<<dec<<" len: "<<payload.len<<endl;

	REG("CR", 0x00, 4) {
		SET_BIT(0x00, 0, 0);
		SET_BIT(0x00, 1, 0);
		REG_READ();
	}

	REG("MR", 0x04, 4) {
		REG_READ();
	}

	REG("IMR", 0x10, 4) {
		*(uint32_t*)&getLocalMem()[0x10] =
				*(uint32_t*)&getLocalMem()[0x8] & ~ *(uint32_t*)&getLocalMem()[0xC];
		REG_READ();
		//cout<<"mask read: "<<hex<<*(uint32_t*)payload.ptr<<dec<<endl;
	}
	REG("ISR", 0x14, 4) {
		SET_BIT(0x14, 3, 1);
		SET_BIT(0x14, 2, isFifoFull());
		SET_BIT(0x14, 0, !isFifoEmpty());
		SET_BIT(0x14, 8, (mHasTimeout && mTimeoutCounter==0));
		REG_READ();
		mOutInt=false;
	}
	REG("FIFO", 0x30, 4) {
		*(char*)payload.ptr=readByte();
	}
	REG("SR", 0x2C, 4) {
		SET_BIT(0x2C, 3, 1);
		SET_BIT(0x2C, 2, isFifoFull());
		SET_BIT(0x2C, 1, isFifoEmpty());
		SET_BIT(0x2C, 0, isFifoOver());
		REG_READ();
	}

	REG("BAUDGEN", 0x18, 4) {
		REG_READ();
	}

	REG("BAUDDIV", 0x34, 4) {
		REG_READ();
	}

	END_IMPL_REG;
	return TLM_OK_RESPONSE;
}

tlm::tlm_response_status xuartps::write (payload_t & payload, sc_time & delay) {
	IMPL_REG;

	LOG_DEBUG(dbg2)<<getName()<<hex<<"write: "<<*(uint32_t*)payload.ptr<<" addr: "<<payload.addr<<dec<<" len: "<<payload.len<<endl;

	REG("CR", 0x00, 4) {
		REG_WRITE();
		if (GET_BIT(0x00, 4)) {
			mTxEnable=true;
		}
		if (GET_BIT(0x00, 5)) {
			mTxEnable=false;
		}
		if (GET_BIT(0x00, 2)) {
			mRxEnable=true;
			cout<<"rx enable"<<endl;
		}
		if (GET_BIT(0x00, 3)) {
			mRxEnable=false;
			cout<<"rx disable"<<endl;
		}
	}

	REG("MR", 0x04, 4) {
		REG_WRITE();
	}

	REG("RXTOUT", 0x20, 4) {
		REG_WRITE();
		mTimeoutCounter=*(uint8_t*)payload.ptr;
		if (mTimeoutCounter==0)
			mHasTimeout=false;
		else
			mHasTimeout=true;
	}

	REG("IER", 0x08, 4) {
		REG_WRITE();
		LOG_DEBUG(dbg1)<<hex<<"write ier: "<<*(uint32_t*)payload.ptr<<" addr: "<<payload.addr<<dec<<" len: "<<payload.len<<endl;

		if (GET_BIT(0x08, 3)) {
			mOutIntEnable=true;
			//cout<<"xuart: out int enabled."<<endl;
		}

		if (GET_BIT(0x08, 0)) {
			mIntEnable=true;
			LOG_DEBUG(dbg1)<<"xuart: input int trigger enabled."<<endl;
		}

		if (GET_BIT(0x08, 2)) {
			mIntEnable=true;
			LOG_DEBUG(dbg1)<<"xuart: input int full enabled."<<endl;
		}

		if (GET_BIT(0x08, 8)) {
			mToIntEnable=true;
			mHasTimeout=true;
			//cout<<"enabled timeout. "<<mTimeoutCounter<<endl;
		}
	}


	REG("IDR", 0x0C, 4) {
		REG_WRITE();
		//cout<<hex<<"write idr: "<<*(uint32_t*)payload.ptr<<dec<<endl;

		if (GET_BIT(0x0C, 3)) {
			mOutIntEnable=false;
			//cout<<"xuart: out int disabled."<<endl;
		}

		if (GET_BIT(0x0C, 0)) {
			mIntEnable=false;
		}

		if (GET_BIT(0x0C, 2)) {
			mIntEnable=false;
		}

		if (GET_BIT(0x08, 8)) {
			mToIntEnable=false;
			mHasTimeout=false;
			//cout<<"disabled timeout."<<endl;
		}
	}
	REG("RXWM", 0x20, 4) {
		REG_WRITE();
		LOG_DEBUG(dbg1)<<hex<<"in trigger: "<<*(uint32_t*)payload.ptr<<dec<<endl;
	}
	REG("FIFO", 0x30, 4) {
		writeByte(*(char*)payload.ptr);
		mOutInt=true;
	}
	REG("BAUDGEN", 0x18, 4) {
		REG_WRITE();
	}
	REG("BAUDDIV", 0x34, 4) {
		REG_WRITE();
	}

	END_IMPL_REG;

	return TLM_OK_RESPONSE;
}
} /* namespace vpsim */

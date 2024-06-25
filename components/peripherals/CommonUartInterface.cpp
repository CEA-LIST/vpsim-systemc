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

#include "CommonUartInterface.hpp"
#include <core/TlmCallbackPrivate.hpp>
#include <poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "ChannelManager.hpp"


namespace vpsim {

using namespace std;
using namespace tlm;


CommonUartInterface::CommonUartInterface(sc_module_name name): sc_module(name) {
	SC_THREAD(interruptLoop);

	mIntEnable=false;
	mOutIntEnable=false;
	mInterrupting=false;
	mOutInt=true;
	mHasTimeout=false;
	mToIntEnable=false;
	mTimeoutCounter=0;

	mBaudRate=115200;
}

CommonUartInterface::~CommonUartInterface() {

}
void CommonUartInterface::selectChannel(string channel) {
	mChannel = ChannelManager::get().allocChannel(channel);
}

void CommonUartInterface::writeByte(char c) {
	if (write(mChannel.second,&c,1) == -1){
		std::cerr << "CommonUartInterface: Error on write"<< std::endl;
		return;
	}
}
char CommonUartInterface::readByte() {
	unsigned char c;
	if (read(mChannel.first,&c,1) < 0){
		std::cerr << "CommonUartInterface: Error on read"<< std::endl;
		return -1;
	}
	return c;
}

void CommonUartInterface::setPollPeriod(sc_time period) {
	mPollPeriod=period;
}

void CommonUartInterface::interruptLoop() {
	while(true) {
		sc_core::wait(mPollPeriod);
		//wait(tlm::tlm_global_quantum::instance().get()/2);
		//wait(10.0 / mBaudRate,SC_SEC);
		if (!mInterruptParent)
			return ; //throw runtime_error("UART must have interrupt parent !");

		//cout.clear(); cout<<"int loop."<<endl;

		if ((mOutIntEnable && mOutInt) || (mIntEnable && inputReady())) {
			//if (!mInterrupting)
				raiseInterrupt();
			mInterrupting=true;
			//cout.clear() ; cout<<"uart interrupt"<<endl;
		} else if (mHasTimeout && !inputReady())
		{
			if (mTimeoutCounter) --mTimeoutCounter;
			if (!mTimeoutCounter && mToIntEnable)
			    raiseInterrupt();
		}else /*if (mInterrupting)*/{
			//if (mInterrupting) {
				lowerInterrupt();
			//}
			mInterrupting=false;
			//cout.clear() ; cout<<"uart low interrupt"<<endl;
		}


	}
}

bool CommonUartInterface::inputReady() {
	return ChannelManager::fdCheckReady(mChannel.first);
}





} /* namespace vpsim */

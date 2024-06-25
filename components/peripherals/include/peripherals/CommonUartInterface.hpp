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

#ifndef _COMMONUARTINTERFACE_HPP_
#define _COMMONUARTINTERFACE_HPP_
#include <cstdint>
#include <iostream>
#include <map>
#include <core/TargetIf.hpp>
#include "paramManager.hpp"
#include "InterruptSource.hpp"


namespace vpsim {

class CommonUartInterface: public sc_module,
							public InterruptSource{
public:
	CommonUartInterface(sc_module_name name);
	virtual ~CommonUartInterface();

	virtual bool inputReady();

	virtual void interruptLoop();
	virtual void setPollPeriod(sc_time time);

	virtual void selectChannel(string channel);

	virtual void writeByte(char c);
	virtual char readByte();

	virtual void setBaudRate(uint32_t rate) { mBaudRate=rate; }

	SC_HAS_PROCESS(CommonUartInterface);

protected:
	bool mIntEnable;
	bool mOutIntEnable;
	bool mInterrupting;
	bool mOutInt;
	sc_time mPollPeriod;
	std::pair<int,int> mChannel;
	char mLastWritten;
	uint32_t mBaudRate;
	bool mHasTimeout;
	uint32_t mTimeoutCounter;
	bool mToIntEnable;


};


} /* namespace vpsim */

#endif /* _COMMONUARTINTERFACE_HPP_ */

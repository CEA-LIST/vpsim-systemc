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

#include "rtc.hpp"

using namespace std;
using namespace sc_core;

namespace vpsim{

template <typename reg_T>
Rtc<reg_T>::Rtc(sc_module_name name, uint64_t frequency): sc_module(name),
	mFrequency(frequency)
{
	SC_THREAD(watchThread);
	sensitive << mNewWatchdogEvent;
}

template <typename reg_T>
Rtc<reg_T>::~Rtc(){}

template <typename reg_T>
void Rtc<reg_T>::watchThread()
{
	while(true){
		while(!mWatchdogs.empty()){
            IrqWatchdog nextWatchdog(getNextWatchdog());
            while(!nextWatchdog.isPassed()){
                sc_core::wait(nextWatchdog.deadline - sc_time_stamp(), mNewWatchdogEvent);
                nextWatchdog = getNextWatchdog();
            }
            cancelWatchdog(nextWatchdog.watchdogIdx);
            nextWatchdog.irqIf->update_irq(nextWatchdog.value, nextWatchdog.irqIdx);
		}
		sc_core::wait(mNewWatchdogEvent);
	}
}

template <typename reg_T>
IrqWatchdog Rtc<reg_T>::getNextWatchdog()
{
	//It is assumed that this function has been called with a non empty watchdogs map
	IrqWatchdog nextWatchdog(mWatchdogs.begin()->second);

	for(auto p: mWatchdogs)
	{
		if(nextWatchdog.deadline > p.second.deadline)
			nextWatchdog = p.second;
	}

	return nextWatchdog;
}

template <typename reg_T>
reg_T Rtc<reg_T>::getCounter() const
{
	double sec = sc_time_stamp().to_seconds();
	reg_T counter = sec * mFrequency;

	return counter;
}

template <typename reg_T>
sc_core::sc_time Rtc<reg_T>::counterToTime(reg_T val) const
{
	double sec = static_cast<double>(val) / mFrequency;
	return sc_time(sec, SC_SEC);
}

template <typename reg_T>
void Rtc<reg_T>::setWatchdog(IrqWatchdog watchdog)
{
	cancelWatchdog(watchdog.watchdogIdx);
	mWatchdogs.emplace(watchdog.watchdogIdx, watchdog);
	if(sc_start_of_simulation_invoked ()){
		mNewWatchdogEvent.notify(SC_ZERO_TIME);
	}
}


template <typename reg_T>
void Rtc<reg_T>::setWatchdog(uint32_t watchdogIdx,
		 uint64_t deadline,
		 InterruptIf* irq,
		 uint64_t value,
		 uint32_t irqIdx)
{
	setWatchdog(IrqWatchdog{watchdogIdx, counterToTime(deadline + 300000), irq, value, irqIdx});
}

template <typename reg_T>
bool Rtc<reg_T>::cancelWatchdog(uint32_t idx)
{
	if(mWatchdogs.erase(idx)){
		mNewWatchdogEvent.notify(SC_ZERO_TIME);
		return true;
	}else{
		return false;
	}
}

template <typename reg_T>
uint64_t Rtc<reg_T>::getFrequency() const
{
	return mFrequency;
}

template <typename reg_T>
ostream& Rtc<reg_T>::toOstream(ostream& os) const
{
	os << "Counter register = " << hex << getCounter() << dec << endl;

	os << "Active Watchdogs :" << endl;
	for(auto p: mWatchdogs){
		os << "\t" << p.first << "\t=> "
		   << "deadline: " << p.second.deadline
		   << "irq line: " << p.second.irqIdx
		   << ", value: " << p.second.value << endl;
	}

	return os;
}

template <typename reg_T>
ostream& operator<<(std::ostream& os, const Rtc<reg_T>& rtc)
{
	return rtc.toOstream(os);
}

bool IrqWatchdog::isPassed() const
{
	return deadline <= sc_time_stamp();
}


template class Rtc<uint16_t>;
template class Rtc<uint32_t>;
template class Rtc<uint64_t>;
template ostream& operator<<(std::ostream& os, const Rtc<uint16_t>& rtc);
template ostream& operator<<(std::ostream& os, const Rtc<uint32_t>& rtc);
template ostream& operator<<(std::ostream& os, const Rtc<uint64_t>& rtc);
} //namespace vpsim

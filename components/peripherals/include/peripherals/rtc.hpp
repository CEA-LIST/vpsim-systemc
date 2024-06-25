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

#ifndef _RTC_HPP_
#define _RTC_HPP_

#include <vector>
#include <systemc>
#include <ostream>
#include <map>

#include "InterruptIf.hpp"

//! Default RTC frequency in Hz
#define RTC_DEFAULT_FREQUENCY 100 * 1000 * 1000

namespace vpsim{

struct IrqWatchdog
{
	uint32_t watchdogIdx;
	sc_core::sc_time deadline;
	InterruptIf* irqIf;
	uint64_t value;
	uint32_t irqIdx;

	bool isPassed() const;
};

//! @brief Generic class to implement basic RTC features
template <typename reg_T>
class Rtc: public sc_core::sc_module
{
private:

	//! @brief Stores the pending watchdogs [watchdogId -> watchdog]
	std::map<uint32_t, IrqWatchdog> mWatchdogs;

	//! @brief Frequency of the RTC in Hz
	const uint64_t mFrequency;

	//! @brief Event to notify when an Appointment is added to mSchedule
	sc_core::sc_event mNewWatchdogEvent;

private:
	IrqWatchdog getNextWatchdog();

public:
	Rtc() = delete;

	//! @brief Constructor
	//! @param[in] name name of the sc_module
	//! @param[in] frequency frequency of the RTC
	Rtc(sc_core::sc_module_name name, uint64_t frequency = RTC_DEFAULT_FREQUENCY);
	//! @cond
	// Ignore this systemc specificity for the documentation
	SC_HAS_PROCESS(Rtc);
	//! @endcond

	//! @brief Destructor
	~Rtc() override;

	//! @brief sc_thread in charge of waiting for the next watchdog and raising the corresponding interruption
	void watchThread();

	//! @brief Add a watchdog to the list of pending watchdogs. If any, replace the watchdog with the same ID.
	//! @param[in] watchdog watchdog to be added to the pending watchdogs list
	void setWatchdog(IrqWatchdog watchdog);

	//! @brief Helper to add a watchdog to the pending watchdogs list for conveniency
	//! @param[in] watchdogIdx	Index of the watchog to be added
	//! @param[in] deadline 	Date when the watchdog expires expressed in RTC counter value
	//! @param[in] irq 			Pointer to an interrupt interface in charge of raising the interrupt
	//! @param[in] value		Value to set when the watchdog expires
	//! @param[in] irqIdx		Index of the interrupt line
	void setWatchdog(uint32_t watchdogIdx,
					 uint64_t deadline,
					 InterruptIf* irq,
					 uint64_t value,
					 uint32_t irqIdx);

	//! @param[in] watchdogIdx	index of the watchdog to be canceled
	//! @return True if a watchdog has been canceled, false otherwise
	bool cancelWatchdog(uint32_t watchdogIdx);

	//! @brief Get the counter value calculated with the double precision approximation of the simulation time
	//! @return The current value of the counter
	reg_T getCounter() const;

	//! @brief converts a counter value into the corresponding sc_time value
	//! @param[in] val Counter value to convert
	//! @return sc_time obtained after conversion
	sc_core::sc_time counterToTime(reg_T val) const;

	//! @brief accessor to the RTC frequency
	//! @return frequency of the timer in Hz
	uint64_t getFrequency() const;

public:
	//! @brief classic printer helper for operator << implementation
	std::ostream& toOstream(std::ostream& os) const;
};

//! @brief implementation of operator << for the RTC
template <typename reg_T>
std::ostream& operator<<(std::ostream& os, const Rtc<reg_T>& rtc);

}




#endif /* S_RTC_HPP_ */

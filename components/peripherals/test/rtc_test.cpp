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

#include <gtest/gtest.h>
#include "rtc.hpp"

using namespace vpsim;
using namespace sc_core;
using namespace std;


class MyIrc : public InterruptIf
{
	virtual void update_irq( uint64_t val, uint32_t irq_idx )
	{
		cout << "update_irq called with val = " << val
			 << " and irq_idx = " << irq_idx << endl;
	}
};

TEST(Rtc, construction){
	Rtc<uint64_t> rtc1("rtc64NoFreq");
	Rtc<uint32_t> rtc2("rtc32NoFreq");
	Rtc<uint16_t> rtc3("rtc16NoFreq");
	Rtc<uint64_t> rtc4("rtc64Freq", 1e9);
	Rtc<uint32_t> rtc5("rtc32Freq", 1e6);
	Rtc<uint16_t> rtc6("rtc16Freq", 1e7);

	SUCCEED();
}

TEST(Rtc, getFrequency){
	Rtc<uint64_t> rtc1("rtcDefaultFreq");
	Rtc<uint64_t> rtc2("rtcCustomFreq", 1e9);

	EXPECT_EQ(rtc1.getFrequency(), RTC_DEFAULT_FREQUENCY);
	EXPECT_EQ(rtc2.getFrequency(), 1e9);
}

//More tests can be written but it is quite hard because of systemc

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

#include "LatencyIf.hpp"

namespace vpsim {

LatencyIf::LatencyIf() :
	mEnableLatency(false),
	mCyclesPerRead(0),
	mCyclesPerWrite(0),
	mInitialCyclesPerAccess(0),
	mCycleDuration(sc_time(1, SC_NS))
{
}

LatencyIf::~LatencyIf () { }


void LatencyIf::setCyclesPerRead(int CyclesPerRead) { mCyclesPerRead = CyclesPerRead; }

void LatencyIf::setCyclesPerWrite(int CyclesPerWrite) { mCyclesPerWrite = CyclesPerWrite; }

void LatencyIf::setInitialCyclesPerAccess(int InititalCyclesPerAccess) { mInitialCyclesPerAccess = InititalCyclesPerAccess; }

void LatencyIf::setCycleDuration(sc_time CycleDuration) { mCycleDuration = CycleDuration; }

void LatencyIf::setEnableLatency(bool EnableLatency) { mEnableLatency = EnableLatency; }



int LatencyIf::getCyclesPerRead() { return (mCyclesPerRead); }

int	LatencyIf::getCyclesPerWrite() { return (mCyclesPerWrite); }

int LatencyIf::getInitialCyclesPerAccess() { return (mInitialCyclesPerAccess); }

sc_time LatencyIf::getReadWordLatency() {
    return (((mInitialCyclesPerAccess + mCyclesPerRead) * (1 /*DefaultLen/WordBytes*/)) * mCycleDuration);
}

sc_time LatencyIf::getWriteWordLatency() {
    return (((mInitialCyclesPerAccess + mCyclesPerWrite) * (1 /*DefaultLen/WordBytes*/)) * mCycleDuration);
}

sc_time LatencyIf::getCycleDuration() { return (mCycleDuration); }

bool LatencyIf::getEnableLatency() { return (mEnableLatency); }

}

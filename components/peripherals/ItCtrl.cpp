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

#include "ItCtrl.hpp"
#include "EndianHelper.hpp"
#include "GlobalPrivate.hpp"
#include "log.hpp"

namespace vpsim {

ItCtrl::ItCtrl(sc_module_name Name, uint32_t LineCount, uint32_t LineSize):
		sc_module(Name),
		TargetIf(string(Name), LineCount * LineSize),
		mLineCount(LineCount),
		mLineSize(LineSize),
		mWordLengthInByte(4)
{
	//Set timings
	TargetIf <unsigned char >::setDmiEnable ( false );

	mModules = new InterruptIf* [mLineCount];
	mLines = new int [mLineCount];
	for(size_t i = 0; i < mLineCount; i++) {
		mModules[i]	= NULL;
		mLines[i]	= 0;
	}

	TargetIf <unsigned char>::RegisterReadAccess(REGISTER(this_type,read));
	TargetIf <unsigned char>::RegisterWriteAccess(REGISTER(this_type,write));
}


ItCtrl::~ItCtrl() {

	//printf("\nItCtrl.cpp: destructor \n");

	delete[] mModules;
	delete[] mLines;
}

void ItCtrl::Map(uint32_t LineIdx, InterruptIf* Module, uint32_t LineNumber) {

	if (LineIdx >= mLineCount) {
		cerr<<"not enough interrupt lines to connect line: " << LineNumber << " to line_idx: " << LineIdx <<endl;
	}

	if (mModules[LineIdx] != NULL){
		cerr<<"Overriding Interrupt line Module mapping may lead to undefined behaviour" << endl;
	}

	if (mLines[LineIdx] != 0){
		cerr<<"Overriding Interrupt line index mapping may lead to undefined behaviour" << endl;
	}

	mLines[LineIdx]		= LineNumber;
	mModules[LineIdx]	= Module;

}


tlm::tlm_response_status ItCtrl::read ( payload_t & payload, sc_time & delay ) {
	LOG_DEBUG(dbg1)<<"access to ItCtrl in read mode @"<<std::hex<<payload.addr<<" len is "<<payload.len<<std::dec<<endl;

	//Compute delay time (can be more accurate)
	if ( getEnableLatency() ) {
		delay += ((getInitialCyclesPerAccess() + getCyclesPerRead()) * (payload.len / mWordLengthInByte)) * getCycleDuration();
	}
	throw string("Not supposed to read ITCTRL\n\n");
	return ( tlm::TLM_OK_RESPONSE );
}

tlm::tlm_response_status ItCtrl::write ( payload_t & payload, sc_time & delay ) {

	//Compute delay time (can be more accurate)
	if ( getEnableLatency() ) {
		delay += ((getInitialCyclesPerAccess() + getCyclesPerWrite()) * (payload.len / mWordLengthInByte)) * getCycleDuration();
	}

	LOG_DEBUG(dbg1)<<"access to ItCtrl in write mode @"<<std::hex<<payload.addr<<" len is "<<payload.len<<std::dec<<endl;

	uint32_t TargetLine = (payload.addr-getBaseAddress())/mLineSize;
	uint32_t Value = EndianHelper::GuestToHost<unsigned int, true,true>(payload.ptr, payload.len);

	LOG_DEBUG(dbg1)<<"ItCtrl.cpp: TargetLine = "<<TargetLine<<" Value = "<<Value<<endl;

	mModules[TargetLine]->update_irq(Value, mLines[TargetLine]);

	//End
	return ( tlm::TLM_OK_RESPONSE );
}




} /* namespace vpsim */

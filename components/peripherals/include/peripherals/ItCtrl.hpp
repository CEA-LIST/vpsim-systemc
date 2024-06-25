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

#ifndef _ITCTRL_HPP_
#define _ITCTRL_HPP_

#include <systemc>
#include "TargetIf.hpp"
#include "InterruptIf.hpp"

namespace vpsim {

class ItCtrl : public sc_module, public TargetIf<unsigned char> {

	typedef ItCtrl this_type;

private:
	// number of interrupt lines
	uint32_t mLineCount;
	//Address range for an interrupt line
	uint32_t mLineSize;

	int* mLines;				//contains indices of interrupt lines in their associated CPUs
	InterruptIf** mModules;	//contains modules associated with each interrupt line in Lines

	uint32_t mWordLengthInByte;


public:
	ItCtrl(sc_module_name Name, uint32_t LineCount, uint32_t LineSize );
	virtual ~ItCtrl();

	//! link an output interrupt line @param LineIdx to a module implementing InterruptIf @param Module and its nth @param LineNumber
	void Map(uint32_t LineIdx, InterruptIf* Module, uint32_t LineNumber);

	//TLM access functions
	//! Read access have no side effect whatsoever
	tlm::tlm_response_status read ( payload_t & payload, sc_time & delay );

	//! TLM write access trigger a call to the update_irq function of the InterruptIf linked with this
	//! address range.
	tlm::tlm_response_status write ( payload_t & payload, sc_time & delay );

};

} /* namespace vpsim */

#endif /* _ITCTRL_HPP_ */

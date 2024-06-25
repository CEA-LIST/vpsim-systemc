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

#ifndef _INTERRUPTSOURCE_HPP_
#define _INTERRUPTSOURCE_HPP_
#include <cstdint>
#include <iostream>
#include <core/TargetIf.hpp>
#include "paramManager.hpp"
#include "InterruptIf.hpp"

namespace vpsim {

class InterruptSource {
public:
	InterruptSource();
	virtual ~InterruptSource();

	void setInterruptParent(InterruptIf* parent);
	void setInterruptLine(uint32_t index);
	void raiseInterrupt();
	void lowerInterrupt();

protected:
	InterruptIf* mInterruptParent;
	uint32_t mInterruptLine;
};

} /* namespace vpsim */

#endif /* _INTERRUPTSOURCE_HPP_ */

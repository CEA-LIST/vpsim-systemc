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

#include "InterruptSource.hpp"

namespace vpsim {

InterruptSource::InterruptSource() {
	// TODO Auto-generated constructor stub
	mInterruptParent=nullptr;
	mInterruptLine=0;
}

InterruptSource::~InterruptSource() {
	// TODO Auto-generated destructor stub
}

void InterruptSource::setInterruptParent(InterruptIf* parent) { mInterruptParent=parent; }
void InterruptSource::setInterruptLine(uint32_t index) { mInterruptLine=index; }
void InterruptSource::raiseInterrupt() {if (!mInterruptParent) return ; /*throw runtime_error("Raising IRQ with no interrupt parent.");*/ mInterruptParent->update_irq(1,mInterruptLine);}
void InterruptSource::lowerInterrupt() {if (!mInterruptParent) return ; /*throw runtime_error("Lowering IRQ with no interrupt parent.");*/ mInterruptParent->update_irq(0,mInterruptLine);}
} /* namespace vpsim */

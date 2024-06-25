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

#ifndef _INTERRUPTIF_HPP_
#define _INTERRUPTIF_HPP_

#include <stdint.h>
namespace vpsim {

class InterruptIf {
public:
	InterruptIf();
	virtual ~InterruptIf();

	//! update_irq must be implemented by modules that support interruptions
	//! the interface is used to assert that objects pointers
	//! passed to an interrupt controller support said function
	virtual void update_irq( uint64_t val, uint32_t irq_idx )=0;
};

} /* namespace vpsim */

#endif /* _INTERRUPTIF_HPP_ */

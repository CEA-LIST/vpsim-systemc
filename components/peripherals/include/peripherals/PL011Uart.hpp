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

#ifndef _PL011UART_HPP_
#define _PL011UART_HPP_
#include <cstdint>
#include <iostream>
#include <core/TargetIf.hpp>
#include "paramManager.hpp"
#include "CommonUartInterface.hpp"

namespace vpsim {

class PL011Uart: public CommonUartInterface,
					public TargetIf<uint32_t> {
public:
	PL011Uart(sc_module_name name);

	tlm::tlm_response_status read (payload_t & payload, sc_time & delay);
	tlm::tlm_response_status write (payload_t & payload, sc_time & delay);

private:
	uint32_t mRxReady;
};

} /* namespace vpsim */

#endif /* _PL011UART_HPP_ */

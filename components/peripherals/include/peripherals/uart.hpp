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

#ifndef UART_HPP_
#define UART_HPP_

#include "global.hpp"
#include "TargetIf.hpp"

namespace vpsim
{
	class uart : public sc_module, public TargetIf  <uint8_t>
	{
		typedef uart this_type;

	private:
		//Local variables
		uint32_t mWordLengthInByte;

	public:
		//---------------------------------------------------
		//Constructor
		uart ( sc_module_name Name );
		uart ( sc_module_name Name, bool ByteEnable, bool DmiEnable );
		void init ();
		SC_HAS_PROCESS ( uart );

		~uart ();

		//Main functions
		tlm::tlm_response_status read ( payload_t & payload, sc_time & delay );
		tlm::tlm_response_status write ( payload_t & payload, sc_time & delay );
	};

}

#endif /* UART_HPP_ */

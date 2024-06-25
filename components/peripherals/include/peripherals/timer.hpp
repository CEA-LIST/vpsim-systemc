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

#ifndef TIMER_HPP_
#define TIMER_HPP_

#include "global.hpp"
#include "TargetIf.hpp"


//Class declaration
namespace vpsim
{
	class timer : public sc_core::sc_module,
		public TargetIf <uint32_t>
	{
		typedef timer this_type;

	private:
		sc_time mCurrentTime;
		uint64_t * mWatchdogs;
		uint32_t mNbWatchdogs;
		bool mSeparateIntLines;
		uint32_t mTimerSize;


	public:
		timer( sc_module_name  Name, uint32_t Quantum );
		timer( sc_module_name  Name, uint32_t NbWatchdogs, uint32_t Quantum );
		SC_HAS_PROCESS ( timer );
		~timer();

		//Communication interface
		sc_out <bool> mIntr;

		//Methods
		void setTimerSize ( uint32_t TimerSize );
		void setSeparateIntLines ( bool SeparateIntLines );

		uint32_t getTimerSize ( );
		bool getSeparateIntLines ( );

		void Reset ( );

	protected:

		//Threads and  methods
		void CoreFunc ( );

		//Other methods
		tlm::tlm_response_status
		read ( payload_t & payload, sc_time & delay );

		tlm::tlm_response_status
		write ( payload_t & payload, sc_time & delay );
	};
}


#endif /* UART_HPP_ */

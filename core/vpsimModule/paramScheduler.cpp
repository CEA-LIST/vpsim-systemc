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

#include <systemc>
#include <iomanip>
#include "paramScheduler.hpp"

using namespace std;

namespace vpsim{

//! @brief SystemC standard constructor
//! @param[in] name Name of the ParamScheduler as a systemC module
ParamScheduler::ParamScheduler(sc_core::sc_module_name name):
		sc_module(name)
{
	SC_THREAD(schedule);
	sensitive << mNewAppointmentEvent;
}

void ParamScheduler::schedule()
{
	while(true){
		while(!mSchedule.empty()){
			ParamAppointment const & nextAppointment{*mSchedule.begin()};

			//If the schedule is not empty, wait for the next appointment or
			//for a new one to be added
			sc_core::wait(nextAppointment.timeTo(), mNewAppointmentEvent);

			//This point can be reached without any reached appointment so
			//the test is mandatory
			if(nextAppointment.isNow()){
				nextAppointment.apply();
				mSchedule.erase(mSchedule.begin());
			}
		}
		//If the schedule is empty, then wait for a new Appointment to be added
		sc_core::wait(mNewAppointmentEvent);
	}
}

void ParamScheduler::addAppointment(const ParamAppointment& appointment)
{
	if(appointment.isPassed()){
		std::cout << "[WARNING] Tried to add a passed appointment. Ignored." << std::endl;
		return;
	}

	mSchedule.insert(appointment);
	mNewAppointmentEvent.notify(sc_core::SC_ZERO_TIME);
}




}

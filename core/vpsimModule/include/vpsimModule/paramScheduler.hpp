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

#ifndef _PARAMSCHEDULER_HPP_
#define _PARAMSCHEDULER_HPP_

#include <systemc>
#include <set>
#include "paramAppointment.hpp"

namespace vpsim{

//! @brief SystemC module responsible for changing the logging parameter of the Logger instances during the simulation
SC_MODULE(ParamScheduler){

private:
  //! @brief Ordered list of the Appointments to come
  std::multiset<ParamAppointment> mSchedule;

  //! @brief Event to notify when an Appointment is added to mSchedule
  sc_core::sc_event mNewAppointmentEvent;

public:
  //! @brief SystemC standard constructor
  //! @param[in] name Name of the ParamScheduler as a systemC module
  ParamScheduler(sc_core::sc_module_name name);
  //! @cond
  // Ignore this systemc specificity for the documentation
  SC_HAS_PROCESS(ParamScheduler);
  //! @endcond

  //! @brief SC_THREAD which wakes up when an Appointment expires or is added to the schedule
  void schedule();

  //! @brief Adds an Appointment to the schedule
  //! @param[in] appointment Appointment to be added to the Schedule
  void addAppointment(const ParamAppointment& appointment);

};

}

#endif /* end of include guard: _PARAMSCHEDULER_HPP_ */

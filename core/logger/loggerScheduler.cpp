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
#include "loggerScheduler.hpp"
#include "appointment.hpp"

namespace vpsim{

LoggerScheduler::LoggerScheduler(sc_core::sc_module_name name):
  sc_module(name)
{
  SC_THREAD(schedule);
  sensitive << mNewAppointmentEvent;
}

void LoggerScheduler::schedule()
{
  while(true){
    while(!mSchedule.empty()){
      Appointment nextAppointment = mSchedule.front();

      //If the schedule is not empty, wait for the next appointment or
      //for a new one to be added
      sc_core::wait(nextAppointment.timeTo(), mNewAppointmentEvent);

      //This point can be reached without any reached appointment so
      //the test is mandatory
      if(nextAppointment.isNow()){
        nextAppointment.apply();
        mSchedule.pop_front();
      }
    }
    //If the schedule is empty, then wait for a new Appointment to be added
    sc_core::wait(mNewAppointmentEvent);
  }
}

void LoggerScheduler::addAppointment(Appointment appointment)
{
  if(appointment.isPassed()){
    std::cout << "[WARNING] Tried to add a passed appointment" << std::endl;
    return;
  }

  if(mSchedule.empty()){
    mSchedule.push_front(appointment);
  } else {
    //Look for the right position for the new appointment (i.e. preserve ordering)
    auto it = mSchedule.begin();
    while(it != mSchedule.end() &&
          it->timeTo() < appointment.timeTo()){
      ++it;
    }
    mSchedule.insert(it, appointment);
  }
  mNewAppointmentEvent.notify(sc_core::SC_ZERO_TIME);
}


//! @brief classic insertion operator overloading for class LoggerScheduler
std::ostream& operator<<(std::ostream& ostr,
                         const LoggerScheduler& loggerScheduler)
{
  //Write the header of the array whose dimension are given
  //by the macros in the header
  ostr << std::setw(LOGGER_NAME_WIDTH)  << "LOGGER NAME |"
       << std::setw(DATE_WIDTH)         << "APPOINTMENT DATE |"
       << std::setw(DEBUG_LVL_WIDTH)    << "DEBUG LEVEL"
       << std::endl;

  //A lign of "---------"
  ostr << std::string(LOGGER_NAME_WIDTH + DATE_WIDTH + DEBUG_LVL_WIDTH, '-')
       << std::endl;

  //Write down each appointment
  for(auto app: loggerScheduler.mSchedule){
    ostr << app;
  }

  return ostr;
}

}

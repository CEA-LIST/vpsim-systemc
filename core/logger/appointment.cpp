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
#include "appointment.hpp"
#include "loggerCore.hpp"
#include "logger.hpp"
#include "logResources.hpp"

namespace vpsim{

Appointment::Appointment(Logger& logger,
                         sc_core::sc_time date,
                         DebugLvl debugLvl):
  mLogger(logger), mDate(date), mDebugLvl(debugLvl)
{}


bool Appointment::isPassed() const
{
  return mDate < sc_core::sc_time_stamp();
}

bool Appointment::isNow() const
{
  return mDate == sc_core::sc_time_stamp();
}


sc_core::sc_time Appointment::timeTo() const
{
  return mDate - sc_core::sc_time_stamp();
}


void Appointment::apply()
{
  if(LoggerCore::get().isRegistered(mLogger)){
    LoggerCore::get().setDebugLvl(mLogger.name(), mDebugLvl);
  }
}

//! @brief classic insertion operator overloading for class Appointment
std::ostream& operator<<(std::ostream& ostr, const Appointment& appointment)
{
  //Allign the data in an array whose dimension are given by the macros in the header
  if(LoggerCore::get().isRegistered(appointment.mLogger)){
    ostr << std::setw(LOGGER_NAME_WIDTH)  << appointment.mLogger.name() << " |"
         << std::setw(DATE_WIDTH)         << appointment.mDate          << " |"
         << std::setw(DEBUG_LVL_WIDTH)    << appointment.mDebugLvl
         << std::endl;
  }
  return ostr;
}

}

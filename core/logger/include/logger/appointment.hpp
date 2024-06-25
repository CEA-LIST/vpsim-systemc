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

#ifndef _APPOINTMENT_HPP_
#define _APPOINTMENT_HPP_

#include <systemc>
#include "logger.hpp"
#include "logResources.hpp"

//!
//!defines the width of the column of the names of the loggers when printing the schedule
//!
#define LOGGER_NAME_WIDTH 30
//!
//!defines the width of the column of the dates when printing the schedule
//!
#define DATE_WIDTH        20
//!
//!defines the width of the column of the debug level when printing the schedule
//!
#define DEBUG_LVL_WIDTH   15

namespace vpsim{

//! @brief Describes an appointment for a logging level change and provides information about it
class Appointment{
private:
  //! @brief Logger whose logging level will change at the appointment
  Logger& mLogger;

  //! @brief Date of the appointment
  const sc_core::sc_time mDate;

  //! @brief Debug level to set at the appointment
  const DebugLvl mDebugLvl;

public:
  //! @brief Only public constructor
  //! @param[in] logger   The logger to be modified when the appointment is reached
  //! @param[in] date     The date of the appointment
  //! @param[in] debugLvl The debug level of the logger after the appointment
  Appointment(Logger& logger,
              sc_core::sc_time date,
              DebugLvl debugLvl);

  //! @brief Tels if the appointment is passed
  //! @return True if the appointment is passed, false otherwise
  bool isPassed() const;

  //! @brief Tels if the appointment is now
  //! @return True if the appointment is now, false otherwise
  bool isNow() const;

  //! @brief Tells how much time remains before the appointemnt
  //! @return Time before the appointment is passed
  sc_core::sc_time timeTo() const;

  //! @brief Set the debug level of the logger in the appointement to the planned debug level
  //!
  //! Works wether the appointment is passed or not.
  void apply();

  friend std::ostream& operator<<(std::ostream& ostr,
                                  const Appointment& appointment);

  Appointment() = delete;
};

}

#endif /* end of include guard: _APPOINTMENT_HPP_ */

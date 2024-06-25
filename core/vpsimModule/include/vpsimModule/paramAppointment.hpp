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

#ifndef _PARAMAPPOINTMENT_HPP_
#define _PARAMAPPOINTMENT_HPP_

#include <systemc>
#include <memory>
#include "moduleParameters.hpp"
#include "AddrSpace.hpp"

namespace vpsim {

//! @brief Describes an appointment for a logging level change and provides information about it
class ParamAppointment{
private:
  //! @brief Module whose logging level will change at the appointment
  std::string mModule;

  //! @brief Address where the parameter will be set
  AddrSpace mAddrSpace;

  //! @brief Parameter to set at the appointment
  const std::shared_ptr<ModuleParameter> mParam;

  //! @brief Date of the appointment
  const sc_core::sc_time mDate;

  //! @brief tell if the default adress space of the VpsimModule should be used
  bool mUseDefaultAs;

public:
  //! @brief Only public constructor
  //! @param[in] module   The logger to be modified when the appointment is reached
  //! @param[in] as   	  Address space where the parameter will be applied
  //! @param[in] date     The date of the appointment
  //! @param[in] param	  The parameter to set at the appointment
  ParamAppointment(string module,
                   AddrSpace as,
                   sc_core::sc_time date,
                   const ModuleParameter &param);

  //! @brief Only public constructor
  //! @param[in] module   The logger to be modified when the appointment is reached
  //! @param[in] date     The date of the appointment
  //! @param[in] param	  The parameter to set at the appointment
  ParamAppointment(string module,
                   sc_core::sc_time date,
                   const ModuleParameter &param);

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
  void apply() const;

  //! @brief Comparison operator based on the sc_time comparison operator
  //! @param[in] that Other ParamAppointment to compare with this
  bool operator<(const ParamAppointment& that) const;

  ParamAppointment() = delete;
};

}

#endif /* end of include guard: _PARAMAPPOINTMENT_HPP_ */

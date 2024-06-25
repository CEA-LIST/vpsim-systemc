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

#ifndef _LOGGER_CORE_HPP_
#define _LOGGER_CORE_HPP_

#include <memory>
#include <string>
#include <unordered_map>
#include <systemc>
#include "loggerScheduler.hpp"
#include "logResources.hpp"
#include "logger.hpp"

namespace vpsim{

//! @brief Singleton class responsible for managing the logging system
class LoggerCore{
private:

  //! @brief Map associating a name to the correponding registered Logger object
  std::unordered_map<std::string, Logger&> mLoggers;

  //! @brief SystemC module responsible for dynamically changing the debug lvl during the simulation
  LoggerScheduler mLoggerScheduler;

  //! @brief Tells wether or not the logging is globaly enabled
  bool mLoggingEnabled;

  //! @brief Tells wether or not the logging has been made impossible (e.g. by an error)
  bool mLoggingImpossible;

  //! @brief Global lvl of debug
  DebugLvl mGlobalDebugLvl;

public:

  //! @brief Accessor to the unique instance of the class LoggerCore
  //! @return Reference to the unique instance of the class LoggerCore
  static LoggerCore& get();

  //! @brief Set the level of debug allowed to a given Logger
  //! @param[in] logger   Name of the Logger
  //! @param[in] debugLvl New level of debug allowed
  void setDebugLvl(std::string logger, DebugLvl debugLvl);

  //! @brief Set the level of debug allowed to a given Logger
  //! @param[in, out] logger  Reference to the Logger
  //! @param[in] debugLvl     New level of debug allowed
  void setDebugLvl(Logger& logger, DebugLvl debugLvl);

  //! @brief Set the level of debug allowed to all Loggers
  //! @param[in] debugLvl New level of debug
  void setDebugLvl(DebugLvl debugLvl);

  //! @brief Schedule an Appointment to change the debug level of a Logger during the simulation
  //! @param[in] logger   Name of the Logger
  //! @param[in] date     Date of the Appointment
  //! @param[in] debugLvl New level of debug
  void addAppointment(std::string logger,
                      sc_core::sc_time date,
                      DebugLvl debugLvl);

  //! @brief Schedule an Appointment to change the debug level of a Logger during the simulation
  //! @param[in] logger   Reference to the Logger
  //! @param[in] date     Date of the Appointment
  //! @param[in] debugLvl New level of debug
  void addAppointment(const Logger& logger,
                      sc_core::sc_time date,
                      DebugLvl debugLvl);

  //! @brief Globaly enable or disable the logging
  //! @param[in] enable Set to true to enable the logging, false to disable it
  void enableLogging(const bool enable);

  //! @brief Tells if the logging is globaly enabled
  //! @return True if the logging is globaly enabled, false otherwise
  bool loggingEnabled() const;

  //! @brief Print the current schedule of the LoggerScheduler module
  void printSchedule() const;

  //! @brief Register a Logger in the LoggerCore (use with care)
  //! @param[in, out] logger Logger to be registered
  void registerLogger(Logger& logger);

  //! @brief Unregister a registered Logger
  //! @param[in] logger Logger to be unregistered
  void unregisterLogger(const Logger& logger);

  //! @brief Tells wether or not a given Logger is registered in the LoggerCore
  //! @param[in] logger Logger to be tested
  //! @return True if the Logger passed in the argument is registered, false otherwise
  bool isRegistered(const Logger& logger) const;


private:
  //! @brief Default constructor made private to prevent from additional instanciations
  LoggerCore();

  //!@brief Copy constructor deleted to prevent from singleton copy
  LoggerCore(const LoggerCore&) = delete;
};

}



#endif /* end of include guard: _LOGGER_CORE_HPP_ */

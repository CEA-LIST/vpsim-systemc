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

#ifndef _LOGGER_HPP_
#define _LOGGER_HPP_

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include "logResources.hpp"

namespace vpsim{

//! @brief Class to inherit from to use the logging macros in log.hpp.
class Logger{
  //! @brief Allows the LoggerCore to change the debug level of the Logger object
  friend class LoggerCore;

private:
  //! @brief Name of the logger
  std::string mName;

  //! @brief Name of the log output file of the logger
  std::string mLogName;

  //! @brief Current level of debug of the logger
  DebugLvl mDebugLvl;

  //! @brief Output stream to the mLogName file
  std::ostream& mOfstream;
  std::ofstream mStatStream;

  //! @brief Tells if the output is enabled for this logger
  bool mEnabled;

public:
  //! @brief Only public Constructor
  //! @param[in] name Name to give to the logger
  Logger(const std::string name, std::ostream& stream=std::cout);

  //! @brief Destructor
  ~Logger();

  //! @brief Access the name of the logger
  //! @return The name of the logger
  std::string name() const;

  //! @brief Access the name of the output file of the logger
  //! @return The name of the output file of the logger
  std::string logName() const;

  //! @brief Tells if the logger can log info messages
  //! @return True if it can log info messages, false otherwise
  //! @see LOG_INFO
  //! @see logInfo()
  bool canLogInfo() const ;

  //! @brief Tells if the logger can log warning messages
  //! @return True if it can log warning messages, false otherwise
  //! @see LOG_WARNING
  //! @see logWarning()
  bool canLogWarning() const ;

  //! @brief Tells if the logger can log Error messages
  //! @return True if it can log Error messages, false otherwise
  //! @see LOG_ERROR
  //! @see logError()
  bool canLogError() const ;

  //! @brief Tells if the logger can log stats messages
  //! @return True if it can log stats messages, false otherwise
  //! @see LOG_STATS
  //! @see logStats()
  bool canLogStats() const ;

  //! @brief Tells if the logger can log debug messages
  //! @param[in] lvl Level of the debug requiered
  //! @return True if it can log debug messages of the given level, false otherwise
  //! @see LOG_INFO
  //! @see logDebug(DebugLvl)
  bool canLogDebug(DebugLvl lvl) const ;

  //! @brief Access the output stream to log info messages
  //! @return The stream to log info
  std::ostream& logInfo();

  //! @brief Access the output stream to log warning messages
  //! @return The stream to log warnings
  std::ostream& logWarning();

  //! @brief Access the output stream to log error messages
  //! @return The stream to log errors
  std::ostream& logError();

  //! @brief Access the output stream to log stats messages
  //! @return The stream to log stats
  std::ofstream& logStats();

  //! @brief Access the output stream to log debug messages
  //! @param[in] lvl level of the debug messages to be loggeg
  //! @return The stream to log debugs
  std::ostream& logDebug(DebugLvl lvl);

private:
  Logger();
};

}

#endif /* end of include guard: _LOGGER_HPP_ */

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

#ifndef _LOG_HPP_
#define _LOG_HPP_

#include "logResources.hpp"
#include "logger.hpp"
#include "loggerCore.hpp"

//! @brief provides a stream to the logging file for a line of INFO
#define LOG_INFO        if(Logger::canLogInfo())       Logger::logInfo()       << "[Info] "

//! @brief provides a stream to the logging file for a line of WARNING
#define LOG_WARNING     if(Logger::canLogWarning())    Logger::logWarning()    << "[Warning] "

//! @brief provides a stream to the logging file for a line of STATS
#define LOG_STATS       if(Logger::canLogStats())      Logger::logStats()      << "[Stats] "

//! @brief provides a stream to the logging file for a line of ERROR
#define LOG_ERROR       if(Logger::canLogError())      Logger::logError()      << "[Error] "

//! @brief provides a stream to the logging file for a line of DEBUG
//! @param[in] lvl Level of debug of the message
#define LOG_DEBUG(lvl)  if(Logger::canLogDebug((lvl))) Logger::logDebug((lvl)) << "[Debug" << (lvl) << "] "

namespace vpsim{
  extern Logger globalLogger;
}

//! @brief provides a stream to the logging file for a line of INFO in the global log file
#define LOG_GLOBAL_INFO        if(globalLogger.canLogInfo())       globalLogger.logInfo()       << "[Info] "

//! @brief provides a stream to the logging file for a line of WARNING in the global log file
#define LOG_GLOBAL_WARNING     if(globalLogger.canLogWarning())    globalLogger.logWarning()    << "[Warning] "

//! @brief provides a stream to the logging file for a line of ERROR in the global log file
#define LOG_GLOBAL_STATS       if(globalLogger.canLogStats())      globalLogger.logStats()      << "[Stats] "

//! @brief provides a stream to the logging file for a line of ERROR in the global log file
#define LOG_GLOBAL_ERROR       if(globalLogger.canLogError())      globalLogger.logError()      << "[Error] "

//! @brief provides a stream to the logging file for a line of DEBUG in the global log file
//! @param[in] lvl Level of debug of the message
#define LOG_GLOBAL_DEBUG(lvl)  if(globalLogger.canLogDebug((lvl))) globalLogger.logDebug((lvl)) << "[Debug" << (lvl) << "] "

#endif /* end of include guard: _LOG_HPP_ */

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

#include <memory>
#include "loggerCore.hpp"
#include "loggerScheduler.hpp"
#include "appointment.hpp"
#include "logger.hpp"
#include "logResources.hpp"

namespace vpsim{

LoggerCore& LoggerCore::get()
{
  //Instance of the LoggerCore
  static std::unique_ptr<LoggerCore> instance(new LoggerCore());

  return *instance;
}

LoggerCore::LoggerCore():
  mLoggerScheduler("loggerScheduler"),
  mLoggingEnabled(false),
  mLoggingImpossible(false),
  mGlobalDebugLvl(dbg0)
{}

void LoggerCore::registerLogger(Logger& logger)
{
  std::string name(logger.name());
  //fails if this logger has the same name than another one already registered
  bool fail = !mLoggers.insert({name, logger}).second;

  if(fail){
    std::cerr << "[ERROR] Two loggers have the same name: " << name << std::endl
              << "[ERROR] disabling logging." << std::endl;

    //The following policy might be a bit harsh and is not reversible (on purpose)
    //This causes additional undesirable side effects which could break the tests

    //mLoggingImpossible = true;

    enableLogging(false);

  } else {
    //Don't forget to set the "enabled" flag of this new logger
    //(which is false by default)
    logger.mEnabled = loggingEnabled();
    //And the level of debug to the global setting
    logger.mDebugLvl = mGlobalDebugLvl;
  }
}

void LoggerCore::unregisterLogger(const Logger& logger)
{
  if(isRegistered(logger)){
    mLoggers.erase(logger.name());
  }
}

bool LoggerCore::isRegistered(const Logger& logger) const
{
  //Understand "for each pair in the map, take the Logger ref and compare
  //the address of the pointed element with the address of the argument"
  for(auto& pair: mLoggers){
    if(&pair.second == &logger){
      return true;
    }
  }
  return false;
}

void LoggerCore::enableLogging(const bool enable)
{
  if(enable && mLoggingImpossible){
    std::cerr << "[WARNING] It turns out that logging is impossible for this run." << std::endl
              << "Logging is disabled" << std::endl
              << "Look for the reason in the previous error outputs." << std::endl;
    mLoggingEnabled = false;
  } else {
    mLoggingEnabled = enable;
  }

  for(auto& p : mLoggers){
    p.second.mEnabled = loggingEnabled();
  }
}

bool LoggerCore::loggingEnabled() const
{
  return mLoggingEnabled && !mLoggingImpossible;
}

void LoggerCore::addAppointment(std::string logger,
                                sc_core::sc_time date,
                                DebugLvl debugLvl)
{
  bool exists = mLoggers.count(logger) == 1;
  if(!exists){
    std::cerr << "[WARNING] The logger " << logger << " does not exist." << std::endl;
  } else {
    mLoggerScheduler.addAppointment(Appointment(mLoggers.at(logger), date, debugLvl));
  }
}

void LoggerCore::addAppointment(const Logger& logger,
                                sc_core::sc_time date,
                                DebugLvl debugLvl)
{
  if(isRegistered(logger)){
    addAppointment(logger.name(), date, debugLvl);
  }
}

void LoggerCore::setDebugLvl(std::string logger, DebugLvl debugLvl)
{
  bool exists = mLoggers.count(logger) == 1;
  if(!exists){
    std::cerr << "[WARNING] The logger " << logger << " does not exist." << std::endl;
  } else {
    mLoggers.at(logger).mDebugLvl = debugLvl;
  }
}

void LoggerCore::setDebugLvl(Logger& logger, DebugLvl debugLvl)
{
  if(isRegistered(logger)){
    setDebugLvl(logger.name(), debugLvl);
  }
}

void vpsim::LoggerCore::setDebugLvl(DebugLvl debugLvl) {
	mGlobalDebugLvl = debugLvl;

	for(auto logger: mLoggers){
		setDebugLvl(logger.second, debugLvl);
	}
}

void LoggerCore::printSchedule() const
{
    std::cout << mLoggerScheduler;
}
}

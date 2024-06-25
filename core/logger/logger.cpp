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

#include "logger.hpp"
#include "loggerCore.hpp"

namespace vpsim{

Logger globalLogger("globalLog");

Logger::Logger(std::string name, std::ostream& stream):
  mName(name), mLogName(name.append(".log")), mDebugLvl(dbg0), mOfstream(stream), mEnabled(false)
{
  //The logger must be registered to be accessible by the LoggerCore
  LoggerCore::get().registerLogger(*this);
}

Logger::~Logger()
{
  //Unregistering the logger prevents from segmentation fault if
  //the LoggerCore tries to access it after deletion
  LoggerCore::get().unregisterLogger(*this);
}


std::string Logger::name() const{
  return mName;
}


std::string Logger::logName() const{
  return mLogName;
}


bool Logger::canLogInfo() const{
  return mEnabled;
}


bool Logger::canLogWarning() const{
  return mEnabled;
}


bool Logger::canLogError() const{
  return mEnabled;
}


bool Logger::canLogStats() const{
  return mEnabled;
}


bool Logger::canLogDebug(DebugLvl lvl) const {
  return mEnabled && (lvl <= mDebugLvl);
}


std::ostream& Logger::logInfo(){
  if(canLogInfo()){
    mOfstream.clear();
  } else {
    mOfstream.clear(std::ios::failbit);
  }
  return mOfstream;
}


std::ostream& Logger::logWarning(){
  if(canLogWarning()){
    mOfstream.clear();
  } else {
    mOfstream.clear(std::ios::failbit);
  }
  return mOfstream;
}


std::ostream& Logger::logError(){
  if(canLogError()){
    mOfstream.clear();
  } else {
    mOfstream.clear(std::ios::failbit);
  }
  return mOfstream;
}


std::ofstream& Logger::logStats(){
  if(canLogStats()){
    if (!mStatStream.is_open()){
      mStatStream.open(mLogName.c_str(), std::ofstream::out);
    }
    mStatStream.clear();
  } else {
	  mStatStream.clear(std::ios::failbit);
  }
  return mStatStream;
}


std::ostream& Logger::logDebug(DebugLvl lvl){
  if(canLogDebug(lvl)){
    mOfstream.clear();
  } else {
    mOfstream.clear(std::ios::failbit);
  }
  return mOfstream;
}

}

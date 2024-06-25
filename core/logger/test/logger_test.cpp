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


#include <gtest/gtest.h>
#include "log.hpp"

using namespace vpsim;

int sc_main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(Logger, construction){
  __attribute__((unused))
  Logger logger("testLoggerConstruction");
  SUCCEED();//Constructor did not crash: fine
}

TEST(Logger, name){
  Logger logger("testLoggerName");

  EXPECT_EQ("testLoggerName", logger.name());
}

TEST(Logger, logName){
  Logger logger("testLoggerName");

  EXPECT_EQ("testLoggerName.log", logger.logName());
}

TEST(Logger, canLog){
  Logger logger("testLoggerCanLog");

  LoggerCore::get().enableLogging(false);
  EXPECT_FALSE(logger.canLogInfo());
  EXPECT_FALSE(logger.canLogWarning());
  EXPECT_FALSE(logger.canLogError());
  EXPECT_FALSE(logger.canLogStats());
  EXPECT_FALSE(logger.canLogDebug(dbg0));

  LoggerCore::get().enableLogging(true);
  EXPECT_TRUE(logger.canLogInfo());
  EXPECT_TRUE(logger.canLogWarning());
  EXPECT_TRUE(logger.canLogError());
  EXPECT_TRUE(logger.canLogStats());
  EXPECT_FALSE(logger.canLogDebug(dbg1));

  LoggerCore::get().setDebugLvl("testLoggerCanLog", dbg1);
  EXPECT_TRUE(logger.canLogDebug(dbg1));
}

TEST(Logger, getOftreams){
  Logger logger("testLoggerGetOfstreams");

  LoggerCore::get().enableLogging(false);
  EXPECT_FALSE(logger.logInfo().good());
  EXPECT_FALSE(logger.logWarning().good());
  EXPECT_FALSE(logger.logError().good());
  EXPECT_FALSE(logger.logStats().good());
  EXPECT_FALSE(logger.logDebug(dbg0).good());

  LoggerCore::get().enableLogging(true);
  EXPECT_TRUE(logger.logInfo().good());
  EXPECT_TRUE(logger.logWarning().good());
  EXPECT_TRUE(logger.logError().good());
  EXPECT_TRUE(logger.logStats().good());
  EXPECT_TRUE(logger.logDebug(dbg0).good());
  EXPECT_FALSE(logger.logDebug(dbg1).good());

  LoggerCore::get().setDebugLvl("testLoggerGetOfstreams", dbg1);
  EXPECT_TRUE(logger.logDebug(dbg1).good());
}

TEST(Logger, writeLog){
  Logger logger("testLoggerWriteLog");
  std::ifstream ifstream(logger.logName());
  const uint size = 100;
  char lineIn[size], lineOut[size];
  LoggerCore::get().enableLogging(true);

  sprintf(lineIn, "test log info");
  logger.logInfo() << lineIn << std::endl;
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

  sprintf(lineIn, "test log Warning");
  logger.logWarning() << lineIn << std::endl;
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

  sprintf(lineIn, "test log error");
  logger.logError() << lineIn << std::endl;
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

  sprintf(lineIn, "test log stats");
  logger.logStats() << lineIn << std::endl;
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

  sprintf(lineIn, "test log debug");
  logger.logDebug(dbg0) << lineIn << std::endl;
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);
}

TEST(Logger, writeLogMacro){
  class MyIp: public Logger{
  public:
    MyIp(std::string name): Logger(name){}
    void writeAnInfo()            {LOG_INFO << "An info" << std::endl;}
    void writeAWarning()          {LOG_WARNING << "A warning" << std::endl;}
    void writeAStat()             {LOG_STATS << "A stat" << std::endl;}
    void writeAnError()           {LOG_ERROR << "An error" << std::endl;}
    void writeADebug(DebugLvl lvl){LOG_DEBUG(lvl) << "A debug" << std::endl;}
  };

  MyIp myIp("testLoggerWriteLogMacro");
  LoggerCore::get().enableLogging(true);
  LoggerCore::get().setDebugLvl(myIp, dbg6);
  std::ifstream ifstream(myIp.logName());

  const uint size = 100;
  char lineIn[size], lineOut[size];

  myIp.writeAnInfo();
  sprintf(lineIn, "[Info] An info");
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

  myIp.writeAWarning();
  sprintf(lineIn, "[Warning] A warning");
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

  myIp.writeAStat();
  sprintf(lineIn, "[Stats] A stat");
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

  myIp.writeAnError();
  sprintf(lineIn, "[Error] An error");
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

  myIp.writeADebug(dbg1);
  sprintf(lineIn, "[Debug1] A debug");
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

  myIp.writeADebug(dbg6);
  sprintf(lineIn, "[Debug6] A debug");
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

}


TEST(Logger, writeGlobalLogMacro){
  LoggerCore::get().enableLogging(true);
  LoggerCore::get().setDebugLvl("globalLog", dbg6);
  std::ifstream ifstream(globalLogger.logName());

  const uint size = 100;
  char lineIn[size], lineOut[size];

  LOG_GLOBAL_INFO << "An info" << std::endl;
  sprintf(lineIn, "[Info] An info");
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

  LOG_GLOBAL_WARNING << "A warning" << std::endl;
  sprintf(lineIn, "[Warning] A warning");
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

  LOG_GLOBAL_STATS << "A stat" << std::endl;
  sprintf(lineIn, "[Stats] A stat");
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

  LOG_GLOBAL_ERROR << "An error" << std::endl;
  sprintf(lineIn, "[Error] An error");
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

  LOG_GLOBAL_DEBUG(dbg1) << "A debug" << std::endl;
  sprintf(lineIn, "[Debug1] A debug");
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);

  LOG_GLOBAL_DEBUG(dbg6) << "A debug" << std::endl;
  sprintf(lineIn, "[Debug6] A debug");
  ifstream.getline(lineOut, size);
  EXPECT_STREQ(lineIn, lineOut);
}

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
#include <sstream>

using namespace vpsim;
using namespace sc_core;

int sc_main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST(LoggerCore, get){
  __attribute__((unused))
  LoggerCore& loggerCore(LoggerCore::get());
  SUCCEED();
}

TEST(LoggerCore, registerLogger){
  Logger logger1("testLoggerCoreRegisterLogger1");
  LoggerCore::get().unregisterLogger(logger1);
  Logger logger2("testLoggerCoreRegisterLogger2");
  Logger logger3("testLoggerCoreRegisterLogger3");
  LoggerCore::get().unregisterLogger(logger2);
  LoggerCore::get().unregisterLogger(logger3);

  EXPECT_FALSE(LoggerCore::get().isRegistered(logger1));
  EXPECT_FALSE(LoggerCore::get().isRegistered(logger2));
  EXPECT_FALSE(LoggerCore::get().isRegistered(logger3));

  LoggerCore::get().registerLogger(logger1);
  LoggerCore::get().registerLogger(logger2);

  EXPECT_TRUE(LoggerCore::get().isRegistered(logger1));
  EXPECT_TRUE(LoggerCore::get().isRegistered(logger2));
  EXPECT_FALSE(LoggerCore::get().isRegistered(logger3));

  LoggerCore::get().setDebugLvl("testLoggerCoreRegisterLogger1", dbg0);
  LoggerCore::get().setDebugLvl("testLoggerCoreRegisterLogger2", dbg2);
  LoggerCore::get().setDebugLvl("testLoggerCoreRegisterLogger3", dbg4);

  SUCCEED();
}


TEST(LoggerCore, enableLogging){
  Logger logger1("testeLoggerCoreEnableLogging1");
  LoggerCore::get().enableLogging(false);
  EXPECT_FALSE(LoggerCore::get().loggingEnabled());

  Logger logger2("testeLoggerCoreEnableLogging2");
  EXPECT_FALSE(logger1.canLogInfo());
  EXPECT_FALSE(logger2.canLogInfo());


  LoggerCore::get().enableLogging(true);
  EXPECT_TRUE(LoggerCore::get().loggingEnabled());

  Logger logger3("testeLoggerCoreEnableLogging3");
  EXPECT_TRUE(logger1.canLogInfo());
  EXPECT_TRUE(logger2.canLogInfo());
  EXPECT_TRUE(logger3.canLogInfo());

}

TEST(LoggerCore, setDebugLvl){
  Logger logger("testLoggerCoreSetDebugLvl");
  LoggerCore::get().enableLogging(true);

  LoggerCore::get().setDebugLvl("testLoggerCoreSetDebugLvl", dbg0);
  EXPECT_TRUE(logger.canLogDebug(dbg0));
  EXPECT_FALSE(logger.canLogDebug(dbg1));

  LoggerCore::get().setDebugLvl(logger, dbg6);
  EXPECT_TRUE(logger.canLogDebug(dbg0));
  EXPECT_TRUE(logger.canLogDebug(dbg6));

  LoggerCore::get().setDebugLvl("testLoggerCoreSetDebugLvl", dbg5);
  EXPECT_TRUE(logger.canLogDebug(dbg0));
  EXPECT_FALSE(logger.canLogDebug(dbg6));
}

TEST(LoggerCore, addAppointment){
  //These two loggers will have Appointments referring to them.
  //It must be assumed that some other tests might step the simulation forward
  //and make it reaches these appointments. In that case, if these loggers no
  //longer exist, a segmentation fault will occur.
  //Because of that, they are allocated on the heap and never deleted by hand.
  //It is perfectly fine for this particular application (testing), where the
  //OS do the cleaning itself at the end.
  Logger* logger = new Logger("testLoggerCoreAddAppointment1");
  new Logger("testLoggerCoreAddAppointment2");

  LoggerCore::get().addAppointment(*logger,
                                   sc_time(1, SC_MS),
                                   dbg3);


  LoggerCore::get().addAppointment("testLoggerCoreAddAppointment2",
                                  sc_time(100, SC_NS),
                                  dbg0);


  LoggerCore::get().addAppointment("testLoggerCoreAddAppointment3",
                                   sc_time(2, SC_MS),
                                   dbg6);

  SUCCEED(); //No crashes occur, that's fine for this test
}

TEST(LoggerCore, printScheduler){
  new Logger("testLoggerCorePrintSchedule");
  LoggerCore::get().addAppointment("testLoggerCorePrintSchedule",
                                   sc_time(1, SC_MS),
                                   dbg3);

  LoggerCore::get().printSchedule();

  SUCCEED(); //No crashes occur, that's fine for this test
  //Feel free to check with your eyes that everything looks ok.
}

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
using namespace sc_core;
using namespace std;

/*
 * General comments
 * This test set is a bit tricky because of systemC side effects.
 * GTest guaranties that, by default, the tests run in the same order than they
 * are written in the files. (The uncertainty is on the order of the files)
 * This will help to manage the simulation progress and module instanciation.
 * Simple rules:
 *    - Every module must be instanciated before the simulation starts,
 *      i.e. in the first tests.
 *    - Every module must be instanciated on the heap as systemC core will
 *      access them during the simulation and thus, they shall not be deleted.
 *    - Once the simulation started, it must not be stopped and no assumption
 *      must be done on the time at the beginning of a test.
 */

int sc_main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST(LoggerScheduler, construction){
  new LoggerScheduler("test");
  SUCCEED(); //Constructor did not crash: fine
}


TEST(LoggerScheduler, addAppointment){
  LoggerScheduler* loggerScheduler =
    new LoggerScheduler("testLoggerSchedulerAddAppointment");
  Logger* loggerA = new Logger("testLoggerSchedulerAddAppointmentA");
  Logger* loggerB = new Logger("testLoggerSchedulerAddAppointmentB");

  sc_time currentTime = sc_time_stamp();

  loggerScheduler->addAppointment(
    Appointment(*loggerA, currentTime + sc_time(1, SC_MS), dbg2));
  loggerScheduler->addAppointment(
    Appointment(*loggerB, currentTime + sc_time(2, SC_MS), dbg5));
  loggerScheduler->addAppointment(
    Appointment(*loggerA, currentTime + sc_time(1.5, SC_MS), dbg0));

  SUCCEED(); //AddAppointment did not crash: fine
}

TEST(LoggerScheduler, operatorInsert){
  LoggerScheduler* loggerScheduler =
  new LoggerScheduler("testLoggerSchedulerOperatorInsert");

  stringstream tested, ref;
  ref << setw(LOGGER_NAME_WIDTH)  << "LOGGER NAME |"
      << setw(DATE_WIDTH)         << "APPOINTMENT DATE |"
      << setw(DEBUG_LVL_WIDTH)    << "DEBUG LEVEL"
      << endl;

  ref  << string(LOGGER_NAME_WIDTH + DATE_WIDTH + DEBUG_LVL_WIDTH, '-') << endl;


  tested << *loggerScheduler;

  EXPECT_EQ(ref.str(), tested.str());
}


TEST(LoggerScheduler, schedule){
  LoggerScheduler* loggerScheduler =
    new LoggerScheduler("testLoggerSchedulerSchedule");
  Logger* loggerA = new Logger("testLoggerSchedulerScheduleA");
  Logger* loggerB = new Logger("testLoggerSchedulerScheduleB");

  //Test the appointments scheduled before the simulation starts
  sc_time currentTime = sc_time_stamp();

  loggerScheduler->addAppointment(
    Appointment(*loggerA, currentTime + sc_time(1, SC_MS), dbg2));
  loggerScheduler->addAppointment(
    Appointment(*loggerB, currentTime + sc_time(2, SC_MS), dbg5));
  loggerScheduler->addAppointment(
    Appointment(*loggerA, currentTime + sc_time(1.5, SC_MS), dbg0));

  LoggerCore::get().enableLogging(true);

  EXPECT_FALSE(loggerA->canLogDebug(dbg1));
  EXPECT_FALSE(loggerB->canLogDebug(dbg1));

  //Make sure the appointments will be passed for the tests
  sc_start(1, SC_NS);

  sc_start(1, SC_MS);
  EXPECT_TRUE(loggerA->canLogDebug(dbg2));
  EXPECT_FALSE(loggerB->canLogDebug(dbg1));

  sc_start(0.5, SC_MS);
  EXPECT_FALSE(loggerA->canLogDebug(dbg1));
  EXPECT_FALSE(loggerB->canLogDebug(dbg5));

  sc_start(0.5, SC_MS);
  EXPECT_FALSE(loggerA->canLogDebug(dbg5));
  EXPECT_TRUE(loggerB->canLogDebug(dbg5));

  //Test the appointments scheduled once the simulation has started
  currentTime = sc_time_stamp();

  loggerScheduler->addAppointment(
    Appointment(*loggerB, currentTime + sc_time(1, SC_MS), dbg0));

  sc_start(1, SC_NS);

  EXPECT_TRUE(loggerB->canLogDebug(dbg5));
  sc_start(1, SC_MS);
  EXPECT_FALSE(loggerB->canLogDebug(dbg1));
}

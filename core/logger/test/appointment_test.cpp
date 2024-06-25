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
#include "appointment.hpp"
#include <sstream>
#include <iomanip>

using namespace vpsim;
using namespace sc_core;
using namespace std;

/*
 * General comments
 * systemc is used in this simulation to test time related features.
 * As a consequence, every test must assume that it can run at any date in the
 * simulation, before or after it started. Also, every test must not stop the
 * simulation to allow the other test to run properly.
 */

int sc_main(int argc, char* argv[])
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

TEST(Appointment, construction){
  Logger logger("testAppointmentConstruction");

  __attribute__((unused))
  Appointment test (logger, SC_ZERO_TIME, dbg0);

  SUCCEED();//Constructor did not crash: fine
}


TEST(Appointment, timeTo){
  Logger logger("testAppointmentTimeTo");
  Appointment test (logger, sc_time_stamp() + sc_time(10, SC_NS), dbg0);

  EXPECT_EQ(sc_time(10, SC_NS), test.timeTo());
  sc_start(5, SC_NS);
  EXPECT_EQ(sc_time(5, SC_NS), test.timeTo());
}


TEST(Appointment, isPassed){
  Logger logger("testAppointmentIsPassed");
  Appointment test (logger, sc_time_stamp() + sc_time(10, SC_NS), dbg0);

  //Before simulation run
  EXPECT_FALSE(test.isPassed());

  //Before the date is reached
  sc_start(5, SC_NS);
  EXPECT_FALSE(test.isPassed());

  //Precisely at the date
  sc_start(5, SC_NS);
  EXPECT_FALSE(test.isPassed());

  //Once the date is passed
  sc_start(5, SC_NS);
  EXPECT_TRUE(test.isPassed());
}


TEST(Appointment, apply){
    LoggerCore::get().enableLogging(true);
    Logger logger("testAppointmentApply");
    Appointment test (logger, sc_time_stamp(), dbg2);

    EXPECT_FALSE(logger.canLogDebug(dbg2));
    test.apply();
    EXPECT_TRUE(logger.canLogDebug(dbg2));
}

TEST(Appointment, operatorInstert){
  Logger logger("testAppointmentOperator<<");
  Appointment test1 (logger, sc_time_stamp(), dbg2);
  Appointment test2 (logger, sc_time_stamp() + sc_time(1, SC_MS), dbg6);

  stringstream tested, ref;
  ref << setw(LOGGER_NAME_WIDTH)  << "testAppointmentOperator<<"  << " |"
      << setw(DATE_WIDTH)         << sc_time_stamp()              << " |"
      << setw(DEBUG_LVL_WIDTH)    << dbg2
      << endl;

  ref << setw(LOGGER_NAME_WIDTH)  << "testAppointmentOperator<<"        << " |"
      << setw(DATE_WIDTH)         << sc_time_stamp() + sc_time(1, SC_MS)<< " |"
      << setw(DEBUG_LVL_WIDTH)    << dbg6
      << endl;

  tested << test1;
  tested << test2;

  EXPECT_EQ(ref.str(), tested.str());
}

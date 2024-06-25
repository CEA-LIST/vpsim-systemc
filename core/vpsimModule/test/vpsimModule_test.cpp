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
#include "vpsimModule.hpp"
#include "paramManager.hpp"

using namespace vpsim;

int sc_main(int argc, char* argv[])
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

class VpsimModuleTest: public ::testing::Test
{
protected:
	AddrSpace modmm1As;
	AddrSpace modmm2As;

	VpsimModule modmm1;
	VpsimModule modmm2;
	VpsimModule modi1;
	VpsimModule modi2;
	VpsimModule modd1;
	VpsimModule modd2;

	virtual void SetUp(){

	}

	VpsimModuleTest():
		modmm1As(0x0, 0x2000),
		modmm2As(0x2001, 0x4000),
		modmm1("modmm1", moduleType::memoryMapped, modmm1As, 0),
		modmm2("modmm2", moduleType::memoryMapped, modmm2As, 0),
		modi1("modi1", moduleType::intermediate, 1),
		modi2("modi2", moduleType::intermediate, 1),
		modd1("modd1", moduleType::dummy, 2),
		modd2("modd2", moduleType::dummy, 2)
	{}


};

TEST_F(VpsimModuleTest, constructor){
	SUCCEED();
}

TEST_F(VpsimModuleTest, twoModules){
	modi1.addSuccessor(modmm1, 0);

	ParamManager::get().setParameter("modmm1", AddrSpace(0x100, 0x200), BlockingTLMEnabledParameter::bt_enabled);
	ParamManager::get().setParameter("modmm1", AddrSpace(0x1000, 0x2000), BlockingTLMEnabledParameter::bt_disabled);

	ParamManager::get().setParameter("modmm1", AddrSpace(0x300, 0x400), ApproximateDelayParameter(sc_time(10, SC_NS)));
	ParamManager::get().setParameter("modmm1", AddrSpace(0x1000, 0x2000), ApproximateDelayParameter(sc_time(100, SC_NS)));

	//Add two translators which compensate eachother
	modmm1.addParameterSetModifier(ParameterSetAddressTranslator(0x50, false));
	modmm1.addParameterSetModifier(ParameterSetAddressTranslator(0x50, true));

	BlockingTLMEnabledParameter::setDefault(BlockingTLMEnabledParameter::BT_ENABLED);

	//Here the modmm1 parameters effect is visible on modi1
	auto p1 = modi1.getBlockingTLMEnabled(0, 0x10);
	EXPECT_TRUE(p1);
	auto p2 = modi1.getBlockingTLMEnabled(0, 0x100);
	EXPECT_TRUE(p2);
	auto p3 = modi1.getBlockingTLMEnabled(0, 0x200);
	EXPECT_TRUE(p3);
	auto p4 = modi1.getBlockingTLMEnabled(0, 0xfff);
	EXPECT_TRUE(p4);
	auto p5 = modi1.getBlockingTLMEnabled(0, 0x1000);
	EXPECT_FALSE(p5);
	auto p6 = modi1.getBlockingTLMEnabled(0, -0x100);
	EXPECT_TRUE(p6);

	//Here the modmm1 parameters effect is visible on modi1
	auto d1 = modi1.getApproximateDelay(0, 0x10);
	EXPECT_EQ(SC_ZERO_TIME, d1);
	auto d2 = modi1.getApproximateDelay(0, 0x300);
	EXPECT_EQ(sc_time(10, SC_NS), d2);
	auto d3 = modi1.getApproximateDelay(0, 0x350);
	EXPECT_EQ(sc_time(10, SC_NS), d3);
	auto d4 = modi1.getApproximateDelay(0, 0xfff);
	EXPECT_EQ(SC_ZERO_TIME, d4);
	auto d5 = modi1.getApproximateDelay(0, 0x2000);
	EXPECT_EQ(sc_time(100, SC_NS), d5);
	auto d6 = modi1.getApproximateDelay(0, -0x100);
	EXPECT_EQ(SC_ZERO_TIME, d6);

	//This parameter won't have effect on modi1
	ParamManager::get().setParameter("modi1", AddrSpace::maxRange, BlockingTLMEnabledParameter::bt_enabled);

	auto p7 = modi1.getBlockingTLMEnabled(0, 0x10);
	EXPECT_TRUE(p7);
	auto p8 = modi1.getBlockingTLMEnabled(0, 0x100);
	EXPECT_TRUE(p8);
	auto p9 = modi1.getBlockingTLMEnabled(0, 0x1fff);
	EXPECT_FALSE(p9);
	auto p10 = modi1.getBlockingTLMEnabled(0, -0x10);
	EXPECT_TRUE(p10);

}

TEST_F(VpsimModuleTest, loopModules){
	// modd1 and modd2 form a loop in the graph
	/* modi1
	     |
	   modi2
	     |
	   modd1<--->modd2
	     |         |
	   modmm1   modmm2 */
	modd1.addSuccessor(modmm1, 0);
	modd2.addSuccessor(modmm2, 0);
	modi1.addSuccessor(modi2, 0);
	modi2.addSuccessor(modd1, 0);
	modd1.addSuccessor(modd2, 1);
	modd2.addSuccessor(modd1, 1);
	//MemoryMapped modules 1 and 2 blocking TLM is enabled and disabled respectively
	ParamManager::get().setParameter("modmm1", AddrSpace(0x0, 0x100), BlockingTLMEnabledParameter::bt_enabled);
	ParamManager::get().setParameter("modmm2", AddrSpace(0x2001, 0x3000), BlockingTLMEnabledParameter::bt_disabled);
	//Dummy modules 1 and 2 blocking TLM is disabled and enabled respectively on the whole range
	ParamManager::get().setParameter("modd1", BlockingTLMEnabledParameter::bt_disabled);
	ParamManager::get().setParameter("modd2", BlockingTLMEnabledParameter::bt_enabled);
	//modi2 operates a negative translation of value 0x1000
	//For instance, the range [0x200, 0x300] for modmm2 is seen as [0x1200, 0x1300] by modi1.
	modi2.addParameterSetModifier(ParameterSetAddressTranslator(0x1000, false));
	//Intermediate modules 1 and 2 are both in default mode

	ParamManager::get().setParameter("modmm2", AddrSpace(0x2500, 0x3500), ApproximateDelayParameter(sc_time(10, SC_NS)));

	//Lets use BT_DISABLED as default value to emphasize the result
	BlockingTLMEnabledParameter::setDefault(BlockingTLMEnabledParameter::BT_DISABLED);

	//modi1 blocking TLM is enabled everywhere a parameter has been defined by its successors,
	//with the translation
	auto p1 = modi1.getBlockingTLMEnabled(0, 0x10);
	EXPECT_FALSE(p1);
	auto p2 = modi1.getBlockingTLMEnabled(0, 0xfff);
	EXPECT_FALSE(p2);
	auto p3 = modi1.getBlockingTLMEnabled(0, 0x1000);
	EXPECT_TRUE(p3);
	auto p4 = modi1.getBlockingTLMEnabled(0, 0x1250);
	EXPECT_TRUE(p4);
	auto p5 = modi1.getBlockingTLMEnabled(0, -0x1);
	EXPECT_FALSE(p5);

	//modi2 has the same parameters but without the translation
	auto p6 = modi2.getBlockingTLMEnabled(0, 0x0);
	EXPECT_TRUE(p6);
	auto p7 = modi2.getBlockingTLMEnabled(0, 0x250);
	EXPECT_TRUE(p7);
	auto p8 = modi2.getBlockingTLMEnabled(0, 0xfff);
	EXPECT_TRUE(p8);
	auto p9 = modi2.getBlockingTLMEnabled(0, 0x5500);
	EXPECT_FALSE(p9);

	auto p10 = modd1.getBlockingTLMEnabled(0, 0x0);
	EXPECT_TRUE(p10);
	auto p11 = modd1.getBlockingTLMEnabled(1, 0x150);
	EXPECT_TRUE(p11);
	auto p12 = modd1.getBlockingTLMEnabled(1, 0x30000);
	EXPECT_FALSE(p12);
	auto p13 = modd1.getBlockingTLMEnabled(1, 0x1000);
	EXPECT_TRUE(p13);
	auto p14 = modd2.getBlockingTLMEnabled(1, 0x0);
	EXPECT_TRUE(p14);
	auto p15 = modd2.getBlockingTLMEnabled(1, 0x150);
	EXPECT_TRUE(p15);
	auto p16 = modd2.getBlockingTLMEnabled(0, 0x30000);
	EXPECT_FALSE(p16);
	auto p17 = modd2.getBlockingTLMEnabled(1, 0x1000);
	EXPECT_TRUE(p17);
	BlockingTLMEnabledParameter::setDefault(BlockingTLMEnabledParameter::BT_ENABLED);

	auto d0 = modi1.getApproximateDelay(0, 0x4000);
	EXPECT_EQ(sc_time(10, SC_NS), d0);
	auto d1 = modi2.getApproximateDelay(0, 0x6000);
	EXPECT_EQ(SC_ZERO_TIME, d1);
	auto d2 = modd1.getApproximateDelay(0, 0x0);
	EXPECT_EQ(SC_ZERO_TIME, d2);
	auto d3 = modd2.getApproximateDelay(1, 0x3000);
	EXPECT_EQ(sc_time(10, SC_NS), d3);
}

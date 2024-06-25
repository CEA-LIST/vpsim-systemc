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
#include "parameterSetModifier.hpp"

using namespace vpsim;

int sc_main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

class ParameterSetAddressTranslatorTest: public ::testing::Test
{
protected:
	ParameterSetAddressTranslator tr1;
	ParameterSetAddressTranslator tr2;
	ParameterSetAddressTranslator tr3;
	ParameterSetAddressTranslator tr4;

	ParameterSet ps1;
	ParameterSet ps2;

	virtual void SetUp(){
		//tr1 default construction
		tr2 = ParameterSetAddressTranslator(0, false);
		tr3 = ParameterSetAddressTranslator(10, false);
		tr4 = ParameterSetAddressTranslator(20, true);
		tr4.attach(tr3);

		// . = default ; + = enabled ; - = disabled
		// 1 symbol ~ range of size 10
		//ps1: ..........++++++++++--------------------......
		//ps2: ++++++++++++++++++++++++++++++++++++++++++++++

		ps1.setParameter(
				AddrSpace(100, 200),
				BlockingTLMEnabledParameter(BlockingTLMEnabledParameter::BT_ENABLED));
		ps1.setParameter(
						AddrSpace(201, 400),
						BlockingTLMEnabledParameter(BlockingTLMEnabledParameter::BT_DISABLED));

		ps2.setParameter(
				AddrSpace::maxRange,
				BlockingTLMEnabledParameter(BlockingTLMEnabledParameter::BT_ENABLED));

	}
};

TEST_F(ParameterSetAddressTranslatorTest, construction){
	SUCCEED();//Constructor did not crash: fine
}

TEST_F(ParameterSetAddressTranslatorTest, apply){
	EXPECT_EQ(ps1, tr1.apply(ps1));
	EXPECT_EQ(ps1, tr2(ps1));

	EXPECT_EQ(ps2, tr1.apply(ps2));
	EXPECT_EQ(ps2, tr2(ps2));

	//ps1tr4: .........++++++++++--------------------.......
	//ps1tr3: error
	auto ps1cp1 = ps1;
	auto ps1cp2 = ps1;
	auto ps1tr4 = tr4(ps1cp1);
	auto ps1tr3 = tr3(ps1cp2);

	auto p1 = ps1tr4.getBlockingTLMEnabledParameter(0);
	EXPECT_TRUE(p1.get());
	auto p2 = ps1tr4.getBlockingTLMEnabledParameter(90);
	EXPECT_TRUE(p2.get());
	auto p3 = ps1tr4.getBlockingTLMEnabledParameter(191);
	EXPECT_FALSE(p3.get());
	auto p4 = ps1tr4.getBlockingTLMEnabledParameter(391);
	EXPECT_TRUE(p4.get());

	//Lets change the default value to see the difference
	BlockingTLMEnabledParameter::setDefault(BlockingTLMEnabledParameter::BT_DISABLED);
	auto p5 = ps1tr3.getBlockingTLMEnabledParameter(0);
	EXPECT_FALSE(p5.get());
	auto p6 = ps1tr3.getBlockingTLMEnabledParameter(210);
	EXPECT_TRUE(p6.get());
	auto p7 = ps1tr3.getBlockingTLMEnabledParameter(211);
	EXPECT_FALSE(p7.get());
	auto p8 = ps1tr3.getBlockingTLMEnabledParameter(-1);
	EXPECT_FALSE(p8.get());
	BlockingTLMEnabledParameter::setDefault(BlockingTLMEnabledParameter::BT_ENABLED);

	EXPECT_THROW(tr3(ps2), overflow_error);

}

TEST_F(ParameterSetAddressTranslatorTest, clone){
	auto ps1cp1 = ps1;
	auto ps1cp2 = ps1;
	auto tr2clone = tr2.clone();
	auto tr4clone = tr4.clone();

	EXPECT_EQ(tr2(ps1cp1), tr2clone->apply(ps1cp2));
	EXPECT_EQ(tr4(ps1cp1), tr4clone->apply(ps1cp2));
}

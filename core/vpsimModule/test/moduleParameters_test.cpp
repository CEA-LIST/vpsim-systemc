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
#include <systemc>
#include "moduleParameters.hpp"

using namespace vpsim;
using namespace sc_core;

int sc_main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

TEST(ModuleParameterBlockingTLMTest, construction){
	BlockingTLMEnabledParameter defaultValue;
	BlockingTLMEnabledParameter enabled(BlockingTLMEnabledParameter::BT_ENABLED);
	BlockingTLMEnabledParameter disabled(false);

	EXPECT_TRUE(defaultValue);
	EXPECT_TRUE(enabled);
	EXPECT_FALSE(disabled);
}


TEST(ModuleParameterBlockingTLMTest, setDefault){
	BlockingTLMEnabledParameter initialDefaultValue;
	EXPECT_TRUE(initialDefaultValue);

	BlockingTLMEnabledParameter::setDefault(BlockingTLMEnabledParameter::BT_DISABLED);
	BlockingTLMEnabledParameter newDefaultValue;
	EXPECT_FALSE(newDefaultValue);

	BlockingTLMEnabledParameter::setDefault(BlockingTLMEnabledParameter::BT_ENABLED);
	BlockingTLMEnabledParameter newDefaultValue2;
	EXPECT_TRUE(newDefaultValue2);
}


TEST(ModuleParameterBlockingTLMTest, ordering){
	BlockingTLMEnabledParameter enabled1(BlockingTLMEnabledParameter::BT_ENABLED);
	BlockingTLMEnabledParameter enabled2(enabled1);
	BlockingTLMEnabledParameter disabled1(BlockingTLMEnabledParameter::BT_DISABLED);
	BlockingTLMEnabledParameter disabled2(disabled1);

	EXPECT_LT(disabled1, enabled1);
	EXPECT_TRUE(disabled1 < enabled1);

	EXPECT_TRUE(enabled1 == enabled2);
	EXPECT_EQ(disabled1, disabled2);

	EXPECT_FALSE(enabled1 < disabled1);
	EXPECT_FALSE(enabled1 == disabled1);
}


TEST(ModuleParameterBlockingTLMTest, clone){
	BlockingTLMEnabledParameter param(BlockingTLMEnabledParameter::BT_DISABLED);

	auto paramClone = param.clone();
	EXPECT_EQ(param, *paramClone);

	auto* cloneCast = dynamic_cast<BlockingTLMEnabledParameter*>(paramClone.get());

	EXPECT_FALSE(*cloneCast);
}


TEST(ModuleParameterApproximateDelayTest, construction){
	ApproximateDelayParameter delayDefault;
	ApproximateDelayParameter delay1(sc_time(10, SC_MS));

	EXPECT_EQ(delayDefault.get(), SC_ZERO_TIME);
	EXPECT_NE(delayDefault, delay1);
	EXPECT_EQ(sc_time(10, SC_MS), delay1);

	SUCCEED();
}


TEST(ModuleParameterApproximateDelayTest, setDefault){
	ApproximateDelayParameter::setDefault(sc_time(42, SC_FS));
	EXPECT_EQ(ApproximateDelayParameter().get(), sc_time(42, SC_FS));

	ApproximateDelayParameter delayDefault;
	EXPECT_EQ(sc_time(42, SC_FS), ApproximateDelayParameter());

	ApproximateDelayParameter::setDefault(SC_ZERO_TIME);
	EXPECT_EQ(ApproximateDelayParameter().get(), SC_ZERO_TIME);
}


TEST(ModuleParameterApproximateDelayTest, oerdering){
	ApproximateDelayParameter delay1(sc_time(10, SC_NS));
	ApproximateDelayParameter delay11(delay1);
	ApproximateDelayParameter delay2(sc_time(20, SC_NS));
	ApproximateDelayParameter delay22(delay2);

	//The order relation in inverted compared to the delay value order
	EXPECT_LT(delay2, delay1);
	EXPECT_TRUE(delay22 < delay11);
	EXPECT_EQ(delay1, delay11);
	EXPECT_NE(delay1, delay2);
	EXPECT_TRUE(delay11 != delay22);
}


TEST(ModuleParameterApproximateDelayTest, adding){
	ApproximateDelayParameter delay1(sc_time(10, SC_NS));
	ApproximateDelayParameter delay2(sc_time(20, SC_NS));
	auto delay3 = *static_cast<ApproximateDelayParameter*>((delay1+delay2).get());
	auto delay4 =  *static_cast<ApproximateDelayParameter*>(
			(delay3 + ApproximateDelayParameter(sc_time(10, SC_NS))).get());

	EXPECT_EQ(sc_time(30, SC_NS), delay3);
	EXPECT_EQ(sc_time(40, SC_NS), delay4);
}

TEST(ModuleParameterApproximateDelayTest, clone){
	ApproximateDelayParameter param(ApproximateDelayParameter(sc_time(42, SC_NS)));

	auto paramClone = param.clone();
	EXPECT_EQ(param, *paramClone);

	auto* cloneCast = dynamic_cast<ApproximateDelayParameter*>(paramClone.get());

	EXPECT_EQ(sc_time(42, SC_NS), *cloneCast);
}

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
#include "parameterSet.hpp"

using namespace vpsim;

int sc_main(int argc, char* argv[])
{
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

class ParameterSetTest: public ::testing::Test
{
protected:
	ParameterSet dps;
	ParameterSet ps1;
	ParameterSet ps2;
	ParameterSet ps22;
	ParameterSet ps3;
	ParameterSet ps4;

	virtual void SetUp()
	{
		// . = default ; + = enabled ; - = disabled
		// 1 symbol ~ range of size 0x10
		//dps: ..............................................
		//ps1: ++++++++++..........++++++++++................
		//ps2: ......+++++++++----------.....................
		//ps3: ++++++++++++++++++++++++++++++++++++++++++++++
		//ps4: ----------------------------------------------

		ps1.setParameter(
				AddrSpace(0x0, 0x100),
				BlockingTLMEnabledParameter(BlockingTLMEnabledParameter::BT_ENABLED));
		ps1.setParameter(
				AddrSpace(0x200, 0x300),
				BlockingTLMEnabledParameter(BlockingTLMEnabledParameter::BT_ENABLED));

		ps2.setParameter(
				AddrSpace(0x50, 0x170),
				BlockingTLMEnabledParameter(BlockingTLMEnabledParameter::BT_ENABLED));
		ps2.setParameter(
				AddrSpace(0x150, 0x250),
				BlockingTLMEnabledParameter(BlockingTLMEnabledParameter::BT_DISABLED));

		ps3.setParameter(
				AddrSpace::maxRange,
				BlockingTLMEnabledParameter(BlockingTLMEnabledParameter::BT_ENABLED));

		ps4.setParameter(
				AddrSpace::maxRange,
				BlockingTLMEnabledParameter(BlockingTLMEnabledParameter::BT_DISABLED));


		//Less testing on approximate delays as they rely on the same algorithms
		//ps1: .......(10)(10)(10)...............(100)(100)(100)(100).............
		//ps2: ...(20)(20)(20)...........................(200)(200)(200)(200).....
		ps1.setParameter(AddrSpace(0x100, 0x200), ApproximateDelayParameter(sc_time(10, SC_NS)));
		ps1.setParameter(AddrSpace(0x1000, 0x2000), ApproximateDelayParameter(sc_time(100, SC_NS)));
		ps2.setParameter(AddrSpace(0x50, 0x150), ApproximateDelayParameter(sc_time(20, SC_NS)));
		ps2.setParameter(AddrSpace(0x1500, 0x2500), ApproximateDelayParameter(sc_time(200, SC_NS)));

		ps22 = ps2;
	}

};

TEST_F(ParameterSetTest, construction){
	//Building the fixture can be considered as a success
	SUCCEED();
}

TEST_F(ParameterSetTest, equality){
	EXPECT_EQ(ps22, ps2);
	EXPECT_FALSE(ps22 != ps2);
	EXPECT_FALSE(dps == ps3);
	EXPECT_TRUE(ps2 != ps1);
}

TEST_F(ParameterSetTest, set_get){
	//Pick random values and check if they are properly set
	auto p0 = dps.getBlockingTLMEnabledParameter(0x0);
	EXPECT_TRUE(p0.get() == BlockingTLMEnabledParameter());
	auto p1 = dps.getBlockingTLMEnabledParameter(-0x2);
	EXPECT_TRUE(p1.get() == BlockingTLMEnabledParameter());
	auto p2 = dps.getBlockingTLMEnabledParameter(1ul << 63);
	EXPECT_TRUE(p2.get() == BlockingTLMEnabledParameter());

	auto p3 = ps1.getBlockingTLMEnabledParameter(0x0);
	EXPECT_TRUE(p3.get());
	auto p4 = ps1.getBlockingTLMEnabledParameter(0x150);
	EXPECT_TRUE(p4.get());
	auto p5 = ps1.getBlockingTLMEnabledParameter(0x230);
	EXPECT_TRUE(p5.get());
	auto p6 = ps1.getBlockingTLMEnabledParameter(0x5000);
	EXPECT_TRUE(p6.get());

	auto p7 = ps2.getBlockingTLMEnabledParameter(0x0);
	EXPECT_TRUE(p7.get());
	auto p8 = ps2.getBlockingTLMEnabledParameter(0x100);
	EXPECT_TRUE(p8.get());
	auto p9 = ps2.getBlockingTLMEnabledParameter(0x140);
	EXPECT_TRUE(p9.get());
	auto p10 = ps2.getBlockingTLMEnabledParameter(0x141);
	EXPECT_TRUE(p10.get());
	auto p11 = ps2.getBlockingTLMEnabledParameter(0x200);
	EXPECT_FALSE(p11.get());
	auto p12 = ps2.getBlockingTLMEnabledParameter(0x1000);
	EXPECT_TRUE(p12.get());

	auto p13 = ps3.getBlockingTLMEnabledParameter(0x0);
	EXPECT_TRUE(p13.get());
	auto p14 = ps3.getBlockingTLMEnabledParameter(-0x2);
	EXPECT_TRUE(p14.get());
	auto p15 = ps3.getBlockingTLMEnabledParameter(1ul << 62);
	EXPECT_TRUE(p15.get());

	auto p16 = ps4.getBlockingTLMEnabledParameter(0x10);
	EXPECT_FALSE(p16.get());
	auto p17 = ps4.getBlockingTLMEnabledParameter(0x123456789);
	EXPECT_FALSE(p17.get());
	auto p18 = ps4.getBlockingTLMEnabledParameter(1ul << 61);
	EXPECT_FALSE(p18.get());

	auto d0 = dps.getApproximateDelayParameter(0x0);
	EXPECT_EQ(SC_ZERO_TIME, d0);
	auto d1 = ps1.getApproximateDelayParameter(0x150);
	EXPECT_EQ(sc_time(10, SC_NS), d1);
	auto d2 = ps2.getApproximateDelayParameter(0x2500);
	EXPECT_EQ(sc_time(200, SC_NS), d2);

}

TEST_F(ParameterSetTest, merge){
	//ps1: ++++++++++..........++++++++++................
	//ps4: ----------------------------------------------
	//sum: ++++++++++----------++++++++++----------------
	auto psMerge1 = ps1;
	psMerge1.mergeImportedParam(ps4);
	auto p0 = psMerge1.getBlockingTLMEnabledParameter(0x0);
	EXPECT_TRUE(p0.get());
	auto p1 = psMerge1.getBlockingTLMEnabledParameter(0x150);
	EXPECT_FALSE(p1.get());
	auto p2 = psMerge1.getBlockingTLMEnabledParameter(0x1ff);
	EXPECT_FALSE(p2.get());
	auto p3 = psMerge1.getBlockingTLMEnabledParameter(0x200);
	EXPECT_TRUE(p3.get());
	auto p4 = psMerge1.getBlockingTLMEnabledParameter(0x1000);
	EXPECT_FALSE(p4.get());

	//ps1: ++++++++++..........++++++++++................
	//ps2: ......+++++++++----------.....................
	//sum: +++++++++++++++-----++++++++++----------------
	auto psMerge2 = ps1;
	psMerge2.mergeImportedParam(ps2);
	auto p5 = psMerge2.getBlockingTLMEnabledParameter(0x0);
	EXPECT_TRUE(p5.get());
	auto p6 = psMerge2.getBlockingTLMEnabledParameter(0x130);
	EXPECT_TRUE(p6.get());
	auto p7 = psMerge2.getBlockingTLMEnabledParameter(0x150);
	EXPECT_FALSE(p7.get());
	auto p8 = psMerge2.getBlockingTLMEnabledParameter(0x250);
	EXPECT_TRUE(p8.get());
	auto p9 = psMerge2.getBlockingTLMEnabledParameter(0x300);
	EXPECT_TRUE(p9.get());
	auto p10 = psMerge2.getBlockingTLMEnabledParameter(1ul << 63);
	EXPECT_TRUE(p10.get());

	//           100         200           1000                2000
	//ps1: .......(10)(10)(10)...............(100)(100)(100)(100).............
	//ps2: ...(20)(20)(20)...........................(200)(200)(200)(200).....
	auto d0 = psMerge2.getApproximateDelayParameter(0x60);
	EXPECT_EQ(sc_time(20, SC_NS), d0);
	auto d1 = psMerge2.getApproximateDelayParameter(0x100);
	EXPECT_EQ(sc_time(10, SC_NS), d1);
	auto d2 = psMerge2.getApproximateDelayParameter(0x250);
	EXPECT_EQ(SC_ZERO_TIME, d2);
	auto d3 = psMerge2.getApproximateDelayParameter(0x1200);
	EXPECT_EQ(sc_time(100, SC_NS), d3);


	auto ps2Copy = ps2;
	auto ps4Copy = ps4;
	EXPECT_TRUE(ps2Copy.mergeImportedParam(ps4) == ps4Copy.mergeImportedParam(ps2));
}

TEST_F(ParameterSetTest, add){
	//ps1: ++++++++++..........++++++++++................
	//ps4: ----------------------------------------------
	//and: ++++++++++----------++++++++++----------------
	auto psAdd1 = ps1;
	psAdd1.addExportedParam(ps4);

	auto p0 = psAdd1.getBlockingTLMEnabledParameter(0x0);
	EXPECT_TRUE(p0.get());
	auto p1 = psAdd1.getBlockingTLMEnabledParameter(0x150);
	EXPECT_FALSE(p1.get());
	auto p2 = psAdd1.getBlockingTLMEnabledParameter(0x1ff);
	EXPECT_FALSE(p2.get());
	auto p3 = psAdd1.getBlockingTLMEnabledParameter(0x200);
	EXPECT_TRUE(p3.get());
	auto p4 = psAdd1.getBlockingTLMEnabledParameter(0x1000);
	EXPECT_FALSE(p4.get());

	//ps2: ......+++++++++----------.....................
	//ps1: +++++++++++++++-----++++++++++................
	//and: +++++++++++++++-----++++++++++................
	auto psAdd2 = ps2;
	psAdd2.addExportedParam(ps1);
	//Lets change the default value to see the difference
	BlockingTLMEnabledParameter::setDefault(BlockingTLMEnabledParameter::BT_DISABLED);
	auto p5 = psAdd2.getBlockingTLMEnabledParameter(0x0);
	EXPECT_TRUE(p5.get());
	auto p6 = psAdd2.getBlockingTLMEnabledParameter(0x130);
	EXPECT_TRUE(p6.get());
	auto p7 = psAdd2.getBlockingTLMEnabledParameter(0x150);
	EXPECT_FALSE(p7.get());
	auto p8 = psAdd2.getBlockingTLMEnabledParameter(0x250);
	EXPECT_TRUE(p8.get());
	auto p9 = psAdd2.getBlockingTLMEnabledParameter(0x300);
	EXPECT_TRUE(p9.get());
	auto p10 = psAdd2.getBlockingTLMEnabledParameter(1ul << 63);
	EXPECT_FALSE(p10.get());
	BlockingTLMEnabledParameter::setDefault(BlockingTLMEnabledParameter::BT_ENABLED);

	//           100         200           1000                2000
	//ps1: .......(10)(10)(10)...............(100)(100)(100)(100).............
	//ps2: ...(20)(20)(20)...........................(200)(200)(200)(200).....
	auto d0 = psAdd2.getApproximateDelayParameter(0x60);
	EXPECT_EQ(sc_time(20, SC_NS), d0);
	auto d1 = psAdd2.getApproximateDelayParameter(0x100);
	EXPECT_EQ(sc_time(30, SC_NS), d1);
	auto d2 = psAdd2.getApproximateDelayParameter(0x250);
	EXPECT_EQ(SC_ZERO_TIME, d2);
	auto d3 = psAdd2.getApproximateDelayParameter(0x1200);
	EXPECT_EQ(sc_time(100, SC_NS), d3);

	auto ps1Copy = ps1;
	auto ps2Copy = ps2;
	EXPECT_TRUE((ps1Copy.addExportedParam(ps2)) ==
				(ps2Copy.addExportedParam(ps1)));
}




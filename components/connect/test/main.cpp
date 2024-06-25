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

#define TEST_MESH

#ifdef TEST_MESH

#include "systemc.h"
#include "ac_tlm_protocol.H"
#include "Mesh.hpp"
#include <iostream> // to use std::fixed
#include <iomanip> //to manage float formatting
#include <algorithm>
using namespace std;



//---------------------------------------------------------------------------------//
//------------------------------ BEGIN TEST ---------------------------------------//
//---------------------------------------------------------------------------------//


C_Mesh* TestMesh;

class C_TrafficSlave: public sc_module, public ac_tlm_transport_if
{

	public:
	unsigned int AccessCount;
	unsigned int ParallelAccessCount;
	C_TrafficSlave(sc_module_name name):sc_module(name){
		InPort.bind(*this);
		AccessCount=0;
		ParallelAccessCount=0;
	}

	sc_export<ac_tlm_transport_if> InPort;

	//ac_tlm_rsp transport(ac_tlm_req const &  req){
	void b_transport( tlm::tlm_generic_payload& trans, sc_time& delay ){
	
		ParallelAccessCount++;
		if (ParallelAccessCount>1)
			SYSTEMC_WARN("ParallelAccessCount>1");

		trans.set_response_status(SUCCESS);
		rsp.status=SUCCESS;
  		rsp.req_type=req.type;
  		rsp.data=0xfeedbabe;
  		trans//No delay for response
  		//rsp.id=req.dev_id;
  		delay+=2;
		
		AccessCount++;
		ParallelAccessCount--;

		//to check that verification performed by NoC are effective
		//wait(1,SC_NS);

		return rsp;
	}

};

class C_TrafficGenerator:public sc_module
{
	private:
	unsigned int SrcID,MaxDestID;
	CycleCount AvgInterReqLatency;
	CycleCount MinInterReqLatency;

	CycleCount LocalTime;
	CycleCount LookAhead;

	enum GenerationMode {UniformTarget, UniqueTarget, RandomTarget};
	GenerationMode GenMode;
	unsigned int UniqueDestID;

	//stats:
	CycleCount TotalLatency;
	unsigned int ReqCount;

	//stats per DestID
	CycleCount * TotalLatencyPerDestID;
	unsigned int * ReqCountPerDestID;

	bool CycleAccurateModel;
	//bool TimeDecoupledModel;

	public:
	
	//to test for connection similar to PE
	sc_port<ac_tlm_transport_if> OutPortI;
	sc_port<ac_tlm_transport_if> OutPortD;

	C_TrafficGenerator(sc_module_name name, unsigned int _SrcID, unsigned int _MaxDestID,
	 CycleCount _LookAhead=1000, float ICacheMissRate=0.02, float DCacheMissRate=0.05, float DataAccessRate=0.2 ):sc_module(name)
	{
		SrcID=_SrcID;
		MaxDestID=_MaxDestID;
		LocalTime=0;
		LookAhead=_LookAhead;

		AvgInterReqLatency=1/(ICacheMissRate + DCacheMissRate * DataAccessRate);
		MinInterReqLatency=1;

		CycleAccurateModel=false; //default is decoupled TLM
		//TimeDecoupledModel=false;

		//stats
		ReqCount=0;
		TotalLatency=0;

		TotalLatencyPerDestID = new CycleCount[_MaxDestID];
		ReqCountPerDestID = new unsigned int [_MaxDestID];
		for(unsigned int i=0;i<MaxDestID;i++)
		{
			TotalLatencyPerDestID[i]=0;
			ReqCountPerDestID[i]=0;
		}


		SC_THREAD(GenReqs);

	}

	~C_TrafficGenerator()
	{
		//cout<<this->name()<<" TotalLatency: "<<TotalLatency<<", ReqCount : "<<ReqCount<<", AvgLatency : "<<TotalLatency/ReqCount<<endl;

		delete[] TotalLatencyPerDestID;
		delete[] ReqCountPerDestID;
	}

	C_TrafficGenerator* WithUniformTargetGen()
	{
		GenMode= UniformTarget;
		return this;
	}

	C_TrafficGenerator* WithRandomTargetGen()
	{
		GenMode= RandomTarget;
		return this;
	}

	C_TrafficGenerator* WithUniqueTargetGen(unsigned int _UniqueDestID)
	{
		GenMode= UniqueTarget;
		UniqueDestID=_UniqueDestID;
		return this;
	}

	C_TrafficGenerator* WithCycleAccurate()
	{
		CycleAccurateModel=true;
		return this;
	}

	float GetAvgLatency()
	{
		return ((float)TotalLatency)/ReqCount;
	}

	float GetTotalLatency()
	{
		return TotalLatency;
	}

	unsigned GetRequestCount()
	{
		return ReqCount;
	}

	void DisplayAccessStat(unsigned int SIZEY=16 /*this parameter controls the end of line in stats display*/)
	{
		cout<<"access stats for traffic generator "<<SrcID<<":"<<endl;
		cout<<"Total Latency Per Destination ID";
		for(unsigned int i=0;i<MaxDestID;i++)
		{
			//add endl every n vals for readibility
			if (i%SIZEY==0)
				cout<<endl;

			cout<<std::fixed<<std::setw(8)<<std::setprecision(2)<<std::setfill(' ')<<TotalLatencyPerDestID[i]<<" ";
		}
		cout<<endl;

		cout<<"Request Count Per Destination ID";
		for(unsigned int i=0;i<MaxDestID;i++)
		{
			//add endl every n vals for readibility
			if (i%SIZEY==0)
				cout<<endl;

			cout<<std::fixed<<std::setw(8)<<std::setprecision(2)<<std::setfill(' ')<<ReqCountPerDestID[i]<<" ";
		}
		cout<<endl;


		cout<<"AvgLatencyPerDestID";
		for(unsigned int i=0;i<MaxDestID;i++)
		{
			//add endl every n vals for readibility
			if (i%SIZEY==0)
				cout<<endl;

			cout<<std::fixed<<std::setw(8)<<std::setprecision(2)<<std::setfill(' ')<<((float)TotalLatencyPerDestID[i])/ReqCountPerDestID[i]<<" ";
		}
		cout<<endl;

	}

	SC_HAS_PROCESS(C_TrafficGenerator);
	
	void GenReqs()
	{
		unsigned int DestID=-1;
		while(true)
		{
			CycleCount InterReqLatency;
			//InterReqLatency=1 + rand() % (2*AvgInterReqLatency -1);
			InterReqLatency = max(MinInterReqLatency,AvgInterReqLatency);


			//InterReqLatency=SrcID*1000; //decouple request
			//InterReqLatency=1;

			//various model
			if(CycleAccurateModel)
				wait(InterReqLatency,SC_NS);
			else
				LocalTime+=InterReqLatency;

			switch (GenMode)
			{
				case UniformTarget : DestID=(DestID+1)%MaxDestID; break;
				case UniqueTarget : DestID=UniqueDestID; break;
				case RandomTarget : DestID=rand()%MaxDestID; break;
				default: SYSTEMC_ERROR("undefined traffic generation mode"); break;
			}

			sc_time InitTime =sc_time_stamp();

			ac_tlm_req req;

			req.type=READ_BURST;
			req.dev_id=SrcID;
			req.addr=TestMesh->GetBaseAddressFromTargetID(T_TargetID(DestID,0) ); //only reach first slave
			req.data=0xfeedbabe;
			req.size_burst=4;
			req.stat=NULL;
//			req.LocalTime=LocalTime;

			int InstructionAccess = floor(rand()%2);
			ac_tlm_rsp rsp; rsp.time=0;
			if(InstructionAccess==0){
				SYSTEMC_TRAFFIC_GEN("call transport I port");
				rsp = OutPortI->transport(req);
				SYSTEMC_TRAFFIC_GEN("return from transport I port");
			}
			else /*if(InstructionAccess==1)*/{
				SYSTEMC_TRAFFIC_GEN("call transport D port");
				rsp = OutPortD->transport(req);
				SYSTEMC_TRAFFIC_GEN("return from transport D port");
			}


			CycleCount Latency;

			if(CycleAccurateModel)
			{
				//if cycle accurate the wait have been performed in the noc  rsp.time==0
				//update time for traffic generator calculation
				sc_time EndTime =sc_time_stamp();
				Latency=ceil((EndTime-InitTime).to_double()/10); // /10 due to resolution...
			}
			else
			{
				Latency = rsp.time;
				LocalTime+=Latency;
			}

			//stats
			TotalLatency+=Latency;
			ReqCount++;
			TotalLatencyPerDestID[DestID] += Latency;
			ReqCountPerDestID[DestID] ++;

			//to test 1 request:
			//return;

			if(!CycleAccurateModel)
			{
				while (LocalTime>LookAhead)
				{
					LocalTime-=LookAhead;
					wait(LookAhead,SC_NS);
				}
			}
		}
	}
};


//can be used for speed because it instantiates only 1 master thread
void OneMaster2AllSlaves(C_NoC::E_ModellingLevel AccuracyLevel, int Duration, unsigned int SIZE_MESH_X, unsigned int SIZE_MESH_Y, unsigned int MasterX, unsigned int MasterY) //center to all
{
	cout<<"Begin "<<__func__<<endl;

	TestMesh=new C_Mesh("TestMesh",SIZE_MESH_X,SIZE_MESH_Y);
	TestMesh->BuildRouting(C_Mesh::Generic);
	TestMesh->SetAccuracyLevel(AccuracyLevel);
	TestMesh->SetFrequencyScaling(2);
	TestMesh->SetNoCLinkSize(2);

	list<C_TrafficSlave*> Slaves;

	//all routers are master and slave
	for(unsigned int i=0;i<SIZE_MESH_X;i++)
		for(unsigned int j=0;j<SIZE_MESH_Y;j++)
		{
			unsigned int RouterID=i*SIZE_MESH_Y+j;
			
			T_MemoryRegion MemRegion1;
			T_MemoryRegion MemRegion2;
			MemRegion1.first = RouterID*0x1000;
			MemRegion1.second = RouterID*0x1000 + 0x7FF;
			MemRegion2.first = RouterID*0x1000 + 0x800;
			MemRegion2.second = RouterID*0x1000 + 0xFFF;

			//build traffic slave and connect it to mesh
			C_TrafficSlave * Slave;

			stringstream ss;
			ss.str(""); //empty the string stream
			ss<<"slave1_"<<RouterID;
			Slave = new C_TrafficSlave(ss.str().c_str());
			Slaves.push_back(Slave);
			TestMesh->bind(&(Slave->InPort),RouterID,MemRegion1);

			ss.str(""); //empty the string stream
			ss<<"slave2_"<<RouterID;
			Slave = new C_TrafficSlave(ss.str().c_str());
			Slaves.push_back(Slave);
			TestMesh->bind(&(Slave->InPort),RouterID,MemRegion2);
		}
	
	//build a unique traffic generator
	unsigned int GeneratorRouterID= MasterX*SIZE_MESH_Y + MasterY;
	C_TrafficGenerator* Generator = new C_TrafficGenerator("Generator",GeneratorRouterID ,SIZE_MESH_X*SIZE_MESH_Y );
	Generator->WithUniformTargetGen();
	if(AccuracyLevel == C_NoC::CycleAccurate)
		Generator->WithCycleAccurate();

	TestMesh->bind(&( Generator->OutPortD),GeneratorRouterID);
	TestMesh->bind(&( Generator->OutPortI),GeneratorRouterID);

	sc_start(Duration,SC_US);
	cout<<"End "<<__func__<<endl;

	Generator->DisplayAccessStat(SIZE_MESH_Y);


	delete Generator;

	list<C_TrafficSlave*>::iterator IT2;
	for(IT2=Slaves.begin(); IT2!=Slaves.end() ; IT2++)
		delete (*IT2);

	delete TestMesh;
}

//All to all
void AllMasters2AllSlaves(C_NoC::E_ModellingLevel AccuracyLevel, int Duration, unsigned int SIZE_MESH_X, unsigned int SIZE_MESH_Y)
{
	cout<<"Begin "<<__func__<<endl;

	TestMesh=new C_Mesh("TestMesh",SIZE_MESH_X,SIZE_MESH_Y);
	TestMesh->BuildRouting(C_Mesh::Generic); //use the generic routing instead of the manually specified one
	TestMesh->SetAccuracyLevel(AccuracyLevel);
	TestMesh->SetFrequencyScaling(2);
	TestMesh->SetNoCLinkSize(4);

	list<C_TrafficGenerator*> Generators;
	list<C_TrafficSlave*> Slaves;


	//Build and connect all masters and slave (all routers are master and slave in this test)
	for(unsigned int i=0;i<SIZE_MESH_X;i++)
		for(unsigned int j=0;j<SIZE_MESH_Y;j++)
		{
			unsigned int RouterID=i*SIZE_MESH_Y+j;

			//build traffic generator and connect it to mesh
			stringstream ss;
			ss<<"Generator"<<RouterID;
			C_TrafficGenerator* Generator = new C_TrafficGenerator(ss.str().c_str(),RouterID,SIZE_MESH_X*SIZE_MESH_Y );
			Generator->WithUniformTargetGen();
			if(AccuracyLevel == C_NoC::CycleAccurate)
				Generator->WithCycleAccurate();
			Generators.push_back(Generator);
			TestMesh->bind(&( Generator->OutPortD),RouterID);//connect first master port as master to RouterID
			TestMesh->bind(&( Generator->OutPortI),RouterID);//connect first master port as master to RouterID

			//build traffic slaves and connect them to mesh

			//specify slave address memory range for router/address matching
			T_MemoryRegion MemRegion1;
			T_MemoryRegion MemRegion2;
			MemRegion1.first = RouterID*0x1000;
			MemRegion1.second = RouterID*0x1000 + 0x7FF;
			MemRegion2.first = RouterID*0x1000 + 0x800;
			MemRegion2.second = RouterID*0x1000 + 0xFFF;

			C_TrafficSlave * Slave;

			ss.str(""); //empty the string stream
			ss<<"slave1_"<<RouterID;
			Slave = new C_TrafficSlave(ss.str().c_str());
			Slaves.push_back(Slave);
			TestMesh->bind(&(Slave->InPort),RouterID,MemRegion1); //connect as slave to RouterID

			ss.str(""); //empty the string stream
			ss<<"slave2_"<<RouterID;
			Slave = new C_TrafficSlave(ss.str().c_str());
			Slaves.push_back(Slave);
			TestMesh->bind(&(Slave->InPort),RouterID,MemRegion2); //connect as slave to RouterID

		}

	TestMesh->CheckMemoryMap();

	sc_start(Duration,SC_US);
	cout<<"End "<<__func__<<endl;

	//stats
	cout<<"latencies:"<<endl;
	list<C_TrafficGenerator*>::iterator IT;
	IT=Generators.begin();
	for(unsigned int i=0;i<SIZE_MESH_X;i++)
	{
		for(unsigned int j=0;j<SIZE_MESH_Y;j++)
		{
			cout<<std::fixed<<std::setw(8)<<std::setprecision(2)<<std::setfill(' ')<<(*IT)->GetAvgLatency()<<" ";
			IT++;
		}
		cout<<endl;
	}

	cout<<endl;
	cout<<"slave access count:"<<endl;
	list<C_TrafficSlave*>::iterator IT2= Slaves.begin();
	for(unsigned int i=0;i<SIZE_MESH_X;i++)
	{
		for(unsigned int j=0;j<SIZE_MESH_Y;j++)
		{
			cout<<std::fixed<<std::setw(8)<<std::setprecision(2)<<std::setfill(' ')<<(*IT2)->AccessCount<<" ";
			IT2++;
			cout<<std::fixed<<std::setw(8)<<std::setprecision(2)<<std::setfill(' ')<<(*IT2)->AccessCount<<" ";
			IT2++;
		}
		cout<<endl;
	}

	//destructors

	for(IT=Generators.begin(); IT!=Generators.end() ; IT++)
		delete (*IT);


	for(IT2=Slaves.begin(); IT2!=Slaves.end() ; IT2++)
		delete (*IT2);

	delete TestMesh;

}

void AllMasters2OneSlave(C_NoC::E_ModellingLevel AccuracyLevel, int Duration, unsigned int SIZE_MESH_X, unsigned int SIZE_MESH_Y, unsigned int SlaveX, unsigned int SlaveY)
{
	cout<<"Begin "<<__func__<<endl;

	TestMesh=new C_Mesh("TestMesh",SIZE_MESH_X,SIZE_MESH_Y);
	TestMesh->BuildRouting(C_Mesh::Generic); //use the generic routing instead of the manually specified one
	TestMesh->SetAccuracyLevel(AccuracyLevel);
	TestMesh->SetFrequencyScaling(2);
	TestMesh->SetNoCLinkSize(4);
	//TestMesh->SetNoTiming();


	list<C_TrafficGenerator*> Generators; //
	list<C_TrafficSlave*> Slaves;


	unsigned int TargetID= SlaveX*SIZE_MESH_Y+SlaveY;
	//Build and connect all masters and slave (all routers are master and slave in this test)
	for(unsigned int i=0;i<SIZE_MESH_X;i++)
		for(unsigned int j=0;j<SIZE_MESH_Y;j++)
		{
			unsigned int RouterID=i*SIZE_MESH_Y+j;

			//build traffic generator and connect it to mesh
			stringstream ss;
			ss<<"Generator"<<RouterID;
			C_TrafficGenerator* Generator = new C_TrafficGenerator(ss.str().c_str(),RouterID,SIZE_MESH_X*SIZE_MESH_Y );
			Generator->WithUniqueTargetGen(TargetID);
			if(AccuracyLevel == C_NoC::CycleAccurate)
				Generator->WithCycleAccurate();
			Generators.push_back(Generator);

			TestMesh->bind(&( Generator->OutPortD),RouterID);//connect first master port as master to RouterID
			TestMesh->bind(&( Generator->OutPortI),RouterID);//connect first master port as master to RouterID


			//build traffic slaves and connect them to mesh

			//specify slave address memory range for router/address matching
			T_MemoryRegion MemRegion1;
			T_MemoryRegion MemRegion2;
			MemRegion1.first = RouterID*0x1000;
			MemRegion1.second = RouterID*0x1000 + 0x7FF;
			MemRegion2.first = RouterID*0x1000 + 0x800;
			MemRegion2.second = RouterID*0x1000 + 0xFFF;

			C_TrafficSlave * Slave;

			ss.str(""); //empty the string stream
			ss<<"slave1_"<<RouterID;
			Slave = new C_TrafficSlave(ss.str().c_str());
			Slaves.push_back(Slave);
			TestMesh->bind(&(Slave->InPort),RouterID,MemRegion1); //connect as slave to RouterID

			ss.str(""); //empty the string stream
			ss<<"slave2_"<<RouterID;
			Slave = new C_TrafficSlave(ss.str().c_str());
			Slaves.push_back(Slave);
			TestMesh->bind(&(Slave->InPort),RouterID,MemRegion2); //connect as slave to RouterID

		}

	TestMesh->CheckMemoryMap();

	sc_start(Duration,SC_US);
	cout<<"End "<<__func__<<endl;

	//stats
	cout<<"Average latencies:"<<endl;
	list<C_TrafficGenerator*>::iterator IT;
	IT=Generators.begin();
	for(unsigned int i=0;i<SIZE_MESH_X;i++)
	{
		for(unsigned int j=0;j<SIZE_MESH_Y;j++)
		{
			cout<<std::fixed<<std::setw(8)<<std::setprecision(2)<<std::setfill(' ')<<(*IT)->GetAvgLatency()<<" ";
			IT++;
		}
		cout<<endl;
	}
	cout<<endl;

	cout<<"Total latencies:"<<endl;
	IT=Generators.begin();
	for(unsigned int i=0;i<SIZE_MESH_X;i++)
	{
		for(unsigned int j=0;j<SIZE_MESH_Y;j++)
		{
			cout<<std::fixed<<std::setw(8)<<std::setprecision(2)<<std::setfill(' ')<<(*IT)->GetTotalLatency()<<" ";
			IT++;
		}
		cout<<endl;
	}
	cout<<endl;

	cout<<"request count:"<<endl;
	IT=Generators.begin();
	for(unsigned int i=0;i<SIZE_MESH_X;i++)
	{
		for(unsigned int j=0;j<SIZE_MESH_Y;j++)
		{
			cout<<std::fixed<<std::setw(8)<<std::setprecision(2)<<std::setfill(' ')<<(*IT)->GetRequestCount()<<" ";
			IT++;
		}
		cout<<endl;
	}
	cout<<endl;


	cout<<"slave access count:"<<endl;
	list<C_TrafficSlave*>::iterator IT2= Slaves.begin();
	for(unsigned int i=0;i<SIZE_MESH_X;i++)
	{
		for(unsigned int j=0;j<SIZE_MESH_Y;j++)
		{
			cout<<std::fixed<<std::setw(8)<<std::setprecision(2)<<std::setfill(' ')<<(*IT2)->AccessCount<<" ";
			IT2++;
			cout<<std::fixed<<std::setw(8)<<std::setprecision(2)<<std::setfill(' ')<<(*IT2)->AccessCount<<" ";
			IT2++;
		}
		cout<<endl;
	}
	cout<<endl;


	cout<<"Detailed traffic generator stats:"<<endl;
	IT=Generators.begin();
	for(unsigned int i=0;i<SIZE_MESH_X;i++)
	{
		for(unsigned int j=0;j<SIZE_MESH_Y;j++)
		{
			(*IT)->DisplayAccessStat(SIZE_MESH_Y);
			cout<<endl;
			IT++;
		}
	}
	cout<<endl;

	//destructors

	for(IT=Generators.begin(); IT!=Generators.end() ; IT++)
		delete (*IT);


	for(IT2=Slaves.begin(); IT2!=Slaves.end() ; IT2++)
		delete (*IT2);

	delete TestMesh;

}

class C_ThreadDeallocTester:public sc_module
{
	private:
	unsigned int ID;

	public:
	C_ThreadDeallocTester(sc_module_name name, unsigned int _ID):sc_module(name)
	{
		ID=_ID;
		SC_THREAD(MyThread);
	}
	SC_HAS_PROCESS(C_ThreadDeallocTester);

	~C_ThreadDeallocTester()
	{
	}
	
	void MyThread()
	{
		while(true)
		{
			unsigned int RandomTime= rand()%100;
			wait(RandomTime,SC_NS);
		}
	}
};


void TestThreadDealloc(unsigned int LoopCount)
{
	std::list<C_ThreadDeallocTester*> ModuleList;
	for(unsigned int i=0;i<LoopCount;i++)
	{
		//build traffic generator and connect it to mesh
		stringstream ss;
		ss<<"ThreadDeallocTester_"<<i;
		C_ThreadDeallocTester* ptr = new C_ThreadDeallocTester(ss.str().c_str(),i);
		ModuleList.push_back(ptr);
	}

	sc_start(10,SC_US);

	list<C_ThreadDeallocTester*>::iterator IT;
	for(IT=ModuleList.begin(); IT!=ModuleList.end() ; IT++)
		delete (*IT);
}


int sc_main(int argc, char* argv[])
{
	if(argc<6)
	{
		cerr<<"No test specified"<<endl;
		cerr<<"use >bin #test #AbstractLevel #duration(us) #MeshX #MeshY (#Slave/TargetX #Slave/TargetY)"<<endl;
		return -1;
	}

	unsigned int Test = atoi(argv[1]);
	C_NoC::E_ModellingLevel LevelOfDescription = (C_NoC::E_ModellingLevel) atoi(argv[2]);
	if(LevelOfDescription!=C_NoC::CycleAccurate && LevelOfDescription!=C_NoC::NoContentions )
	{
		cerr<<"available levels of description:"<<endl;
		cerr<<"CycleAccurate "<<C_NoC::CycleAccurate<<endl;
		cerr<<"NoContentions "<<C_NoC::NoContentions<<endl;
		return -1;
	}
	unsigned int Duration = atoi(argv[3]);
	unsigned int MeshX = atoi(argv[4]);
	unsigned int MeshY = atoi(argv[5]);

	unsigned int TargetX = 0;
	unsigned int TargetY = 0;
	if(Test==0 || Test == 1)
	{
		if(argc<8)
		{
			cerr<<"for test"<<Test<<"Target parameters are needed"<<endl;
			cerr<<"use >bin #test #AbstractLevel #duration #MeshX #MeshY #TargetX #TargetY"<<endl;
		}
		TargetX = atoi(argv[6]);
		TargetY = atoi(argv[7]);
	}

	//debug/trace
	cout<<"TEST PARAMETERS"<<endl;
	cout<<"Test ";
	switch(Test)
	{
		case 0 : cout<<"OneMaster2AllSlaves"; break;
		case 1 : cout<<"AllMasters2OneSlave"; break;
		case 2 : cout<<"AllMasters2AllSlaves"; break;
		default : cout<<"undef"; break;
	}
	cout<<endl;
	cout<<"Duration "<<Duration <<"SC_US"<<endl;
	cout<<"MeshX "<<MeshX<<endl;
	cout<<"MeshY "<<MeshY<<endl;
	cout<<"TargetX "<<TargetX<<endl;
	cout<<"TargetY "<<TargetY<<endl;
	cout<<"LOD : ";

	//TODO when more tests are needed
	//add parameter on the routing algo
	//add parameter on the request generation method (realistic/constant) (constant==0 => ASAP, constant==1000 => limits contentions)

	switch(LevelOfDescription)
	{
		case C_NoC::CycleAccurate : cout<<"C_NoC::CycleAccurate"; break;
		case C_NoC::NoContentions : cout<<"C_NoC::NoContentions"; break;
		default : cout<<"undef"; break;
	}
	cout<<endl;
	//end debug

	switch(atoi(argv[1]))
	{
		case 0 : OneMaster2AllSlaves(LevelOfDescription, Duration,MeshX,MeshY,TargetX,TargetY); break;
		case 1 : AllMasters2OneSlave(LevelOfDescription, Duration,MeshX,MeshY,TargetX,TargetY); break;
		case 2 : AllMasters2AllSlaves(LevelOfDescription, Duration,MeshX,MeshY);break;
		case 3 : TestThreadDealloc(100); break;
		default : cerr<<"Not such test"<<endl; break;
	}

	cout<<"simulation ended"<<endl;
	return 0;
}
#endif

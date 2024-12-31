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

#ifndef MAIN_EXECUTABLE
#define MAIN_EXECUTABLE
#endif

#include "DynamicComponents.hpp"
#include "platform_builder/xmlConfigParser.hpp"

#include <sstream>

#include <signal.h>
#include <netinet/ip.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <netdb.h>

#include "ModelProvider.hpp"

namespace vpsim {
	//Locks
	sem_t VPSIM_LOCK;
}


using namespace vpsim;
using namespace std;

vector<MainMemCosim*> MainMemCosim::_Simulators;
bool MainMemCosim::_Inited=false;
pthread_t MainMemCosim::_T;
MainMemCosim::Req MainMemCosim::_Buffer;
bool MainMemCosim::_Stopped=false;
tbb::concurrent_priority_queue<MainMemCosim::Req, MainMemCosim::compare_Req> MainMemCosim::_PQ;
map<uint32_t, uint64_t*> MainMemCosim::_Stats[MAX_CPUS];
mutex MainMemCosim::_Mut[EPOCHS];
uint64_t MainMemCosim::_CurQuantum=0;
uint64_t MainMemCosim::_CpuEpoch=0;
uint64_t MainMemCosim::_MemEpoch=0;
uint64_t MainMemCosim::_epoch_sc_time=0;
uint64_t MainMemCosim::_current_time_stamp=0;
MainMemCosim::notifyFunctionType MainMemCosim::Notify=MainMemCosim::haltNotify;
MainMemCosim::notifyFetchMissFunctionType MainMemCosim::NotifyFetchMiss=MainMemCosim::haltNotifyFetchMiss;
MainMemCosim::notifyIOFunctionType MainMemCosim::NotifyIO=MainMemCosim::haltNotifyIO;
bool MainMemCosim::_focusOnROI=false;
map<void*,bool> StandaloneInstructionCache::Victims;
int StandaloneInstructionCache::mZero=0;
int StandaloneInstructionCache::mOne=1;

vector<ReaderWriterQueue<std::tuple<uint64_t,uint64_t,uint64_t>,512>*> IOAccessCosim::_Q_Accomplished;

//#############################################################
// Helper functions
//#############################################################
static void
copyright ( )
{
	printf("\n\tCopyright (C) 2024 Commissariat à l'énergie atomique et aux énergies alternatives (CEA).\n");

	printf("\tLicensed under the Apache License, Version 2.0 (the 'License');\n");
	printf("\tyou may not use this file except in compliance with the License.\n");
	printf("\tYou may obtain a copy of the License at\n");

	printf("\thttp://www.apache.org/licenses/LICENSE-2.0\n");

	printf("\tUnless required by applicable law or agreed to in writing, software\n");
	printf("\tdistributed under the License is distributed on an 'AS IS' BASIS,\n");
	printf("\tWITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.\n");
	printf("\tSee the License for the specific language governing permissions and\n");
	printf("\tlimitations under the License.\n\n");

}

int64_t getClk()
{
	struct timeval tv;

	gettimeofday(&tv, NULL);
	return tv.tv_sec * 1000000000LL + (tv.tv_usec * 1000);
}

void onInterrupt(int sig) {
	cout<<"User interrupt received. Stopping SystemC simulation."<<endl;
	sc_stop();
}

uint64_t HOST_TIME_START;
bool DynamicSesamController::InstanceExists=false;
bool PyDevice::PythonInit=false;

//#############################################################
// Main function
//#############################################################
int sc_main ( int argc, char * argv[] )
{

	copyright ();

	//-----------------------------------------------------------------------------------------
	//Initialize global mutexes
	sem_init  ( &VPSIM_LOCK, 0, 1 );
	//-----------------------------------------------------------------------------------------
	//Set logging enabled
	//LoggerCore::get().enableLogging(true);
	//LoggerCore::get().setDebugLvl(dbg0);

	// LoggerCore::get().setDebugLvl(globalLogger, dbg1);

	/***** Register all known types *****/
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicMemory> ("Memory");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicItCtrl> ("ItCtrl");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicUart> ("Uart");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicTLMCallbackRegister<uint32_t>> ("CallbackRegister32");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicTLMCallbackRegister<uint64_t>> ("CallbackRegister64");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicInterconnect> ("Interconnect");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicArm> ("Arm");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicArm64> ("Arm64");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicExternalCPU> ("ExternalCPU");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicExternalSimulator> ("ExternalSimulator");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicPL011Uart> ("PL011Uart");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicGIC> ("GIC");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicVirtioProxy> ("VirtioProxy");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicXuartPs> ("XuartPs");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicBlobLoader> ("BlobLoader");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicElfLoader> ("ElfLoader");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicSesamController> ("Monitor");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicAddressTranslator> ("AddressTranslator");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicRemoteInitiator> ("RemoteInitiator");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicRemoteTarget> ("RemoteTarget");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicSystemCTarget> ("SystemCTarget");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicModelProvider> ("ModelProvider");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicModelProviderCpu> ("ModelProviderCpu");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicModelProviderDev> ("ModelProviderDev");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicModelProviderParam1> ("ModelProviderParam1");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicModelProviderParam2> ("ModelProviderParam2");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicPythonDevice> ("PythonDevice");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicSystemCCosimulator> ("SystemCCosim");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicIOAccessCosimulator> ("IOAccessCosim");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicCache> ("Cache");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicCoherenceInterconnect> ("CoherentInterconnect");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicNoCMemoryController> ("NoCMemoryController");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicNoCSource> ("NoCSource");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicNoCHomeNode> ("NoCHomeNode");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicNoCDeviceController> ("NoCDeviceController");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicCacheController> ("CacheController");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicCacheIdController> ("CacheIdController");
	VpsimIp<InPortType, OutPortType>::RegisterClass<DynamicCpuController> ("CpuController");
	VpsimIp<InPortType, OutPortType>::RegisterClass<Container<InPortType,OutPortType> > ("Container");


	if (argc<2) {
		cerr<<"Call with --dump-components or --run <platform_name>.xml"<<endl;
		return 1;
	} else {
		if (string("--dump-components") == argv[1]) {
			PlatformBuilder::dumpComponents(cout);
			return 0;
		} else if (string("--run") == argv[1]) {
			if (argc < 3) {
				cerr<<"Please provide platform description file (XML)"<<endl;
				cerr<<"Call with --dump-components or --run <platform_name>.xml"<<endl;
				return 1;
			}

		}

		XmlConfigParser parser(argv[2]);
		if (!parser.read()) {
			throw(0);
		}

		//Real-time computation
		struct timeval tp;
		double sec, usec, start, end;
		// Time stamp before the computations
		gettimeofday( &tp, NULL );
		sec   = static_cast<double>( tp.tv_sec  ) * 1e6;
		usec  = static_cast<double>( tp.tv_usec );
		start = sec + usec;

		// HOST_TIME_START = getClk();
		HOST_TIME_START=0;

		signal(SIGINT, onInterrupt);
		signal(SIGTERM, onInterrupt);

		//-----------------------------------------------------------------------------------------
		//Start simulation
		sc_start ();

		//-----------------------------------------------------------------------------------------
		// Time stamp after the computations
		gettimeofday( &tp, NULL );
		sec  = static_cast<double>( tp.tv_sec  ) * 1e6;
		usec = static_cast<double>( tp.tv_usec );
		end  = sec + usec;
		// Time calculation (in ms)
		double PhysicalTimeSec = (end - start) / 1000;

		//Print time
		//setvbuf(stdout, NULL, _IOLBF, 0);//line buffered

		setvbuf(stdout, NULL, _IONBF, 0);//char buffered

		VpsimIp<InPortType,OutPortType>::WriteStat("global", "real_execution_time", tostr(PhysicalTimeSec), "ms");
		VpsimIp<InPortType,OutPortType>::WriteStat("global", "sc_simulation_time", tostr(sc_time_stamp()), "");

		return 0;

	}

}

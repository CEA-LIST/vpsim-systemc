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

#include "vpsim_subsystem.hpp"


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
#include <systemc>
#include <tlm>

#include "ModelProvider.hpp"

namespace vpsim {
	//Locks
	sem_t VPSIM_LOCK;
}


using namespace vpsim;
using namespace std;

#define CURRENT_VERSION "1.0"



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
vector<tuple<MainMemCosim::registerMainMemCb, MainMemCosim::model_provider_main_mem_cb, uint64_t, MainMemCosim::unRegisterMainMemCb>> MainMemCosim::_MainMemCb;
bool MainMemCosim::_focusOnROI=false;
map<void*,bool> StandaloneInstructionCache::Victims;
int StandaloneInstructionCache::mZero=0;
int StandaloneInstructionCache::mOne=1;

vector<ReaderWriterQueue<std::tuple<uint64_t,uint64_t,uint64_t>,512>*> IOAccessCosim::_Q_Accomplished;

string stringify(uint64_t adr) {
	char ret[256];
	sprintf(ret, "%lu", adr);
	return string(ret);
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

#define LINUX

#define N_MSG 1
#define INTERNAL 1

bool VPSim_Lib_Init=false;

void init_() {
	sem_init  ( &VPSIM_LOCK, 0, 1 );

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
	// Do not forget to declare Container !
	VpsimIp<InPortType, OutPortType>::RegisterClass<Container<InPortType,OutPortType> > ("Container");

	VPSim_Lib_Init=true;
}
extern "C" {
namespace vpsim {

Subsystem::Subsystem(sc_module_name name, const char* xml_path): sc_module(name) {
	if(!VPSim_Lib_Init)
		init_();

	XmlConfigParser* parser = new XmlConfigParser(xml_path);
	if (!parser->read()) {
		throw runtime_error("VPSim: Error parsing XML description.");
	}

	_handle = (void*) parser;
}

Subsystem::~Subsystem() {
	XmlConfigParser* parser = (XmlConfigParser*) _handle;
	delete parser;
}

/*tlm::tlm_target_socket<>& Subsystem::in(std::string port_name){
	auto ip = VpsimIp<InPortType, OutPortType>::FindWithType(port_name);
	if (ip.first != "SystemCInitiator")
		throw runtime_error(string("No SystemCInitiator object with name ") + port_name);

	return * get<0>((ip.second)->getInPort("from_systemc"));
}*/

void Subsystem::_apply(void* f, int a, int b) {
    std::function<void(int,int)> * _f=(std::function<void(int,int)> *)f;
    (*_f)(a,b);
}

std::pair<tlm::tlm_initiator_socket<>*, void* > Subsystem::_out(const char* port_name){
	auto ip = VpsimIp<InPortType, OutPortType>::FindWithType(port_name);
	if (ip.first != "SystemCTarget"){
		std::cerr<<string("No SystemCTarget object with name ") + port_name<<endl;
		throw runtime_error("No SystemCTarget object with this name.");
	}

	return make_pair(get<0>((ip.second)->getOutPort("to_systemc")),
				(void*)(&(dynamic_cast<DynamicSystemCTarget*>(ip.second)->mModulePtr->_int)));
}

void Subsystem::declare_dmi_ptr(const char* name, uint64_t mem_base, uint64_t size, void* pointer) {
	VpsimIp<InPortType, OutPortType>::MapIf(
			[](VpsimIp<InPortType, OutPortType>* ip) { return ip->needsDmiAccess(); },
			[name,mem_base,size,pointer](VpsimIp<InPortType, OutPortType>* ip) {
				ip->addDmiAddress(name, mem_base, size, (unsigned char*)pointer, true, true);
			}
	);
}

}/* namespace vpsim */

}

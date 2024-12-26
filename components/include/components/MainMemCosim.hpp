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

#ifndef _MAINMEMCOSIM_HPP_
#define _MAINMEMCOSIM_HPP_
#include <string>
#include <systemc>
#include <pthread.h>
#include <vector>
#include <tlm>
#include "tlm_utils/simple_initiator_socket.h"
#include "atomicops.h"
#include "CosimExtensions.hpp"
#include "concurrent_priority_queue.h"
#include "IOAccessCosim.hpp"
#include "SesamController.hpp"
#include <atomic>
#include <map>
#include <mutex>
#include <tuple>
using namespace std;
using namespace moodycamel;

using namespace tlm;

namespace vpsim {

enum OuterStat {
	L1_MISS=0,
	L2_MISS,
	L1_WB,
	L2_WB,
	L1_LD,
	L1_ST,
	L2_LD,
	L2_ST
};

enum NotifyType {
	CPU=0,
	DEVICE,
	SESAMCOMMAND
};

#define MAX_CPUS 256
#define DECOUPLED_QUANTUMS 100000
#define EPOCHS 2 // for the priority_queue to be used correctly, EPOCHS must be higher than 1


class MainMemCosim {
public:	static struct Req {
		NotifyType type;
		void* phys;
		unsigned int size;// This is how it is defined in SystemC
		uint32_t id;
		uint64_t time_stamp;
		uint8_t write;
		uint64_t tag;
		uint64_t epoch;
		uint8_t fetch;
	} _Buffer;
	MainMemCosim(){
		Add(this);
	}

	virtual ~MainMemCosim() {}

    static void Notify(uint32_t cpu, uint64_t exec, uint8_t write, void* phys, unsigned int size) {
        _current_time_stamp=exec+_epoch_sc_time;
		_Buffer.type=CPU;
		_Buffer.id=cpu;
		_Buffer.write=write;
		_Buffer.phys=phys;
		_Buffer.size=size;
		_Buffer.fetch=0;
		_Buffer.epoch=_CpuEpoch;
        _Buffer.time_stamp=_current_time_stamp;
		_PQ.push(_Buffer);
	}

	static void NotifyFetchMiss(uint32_t cpu, void* phys, unsigned int size) {
		_Buffer.type=CPU;
		_Buffer.id=cpu;
		_Buffer.write=0;
		_Buffer.fetch=1;
		_Buffer.phys=phys;
		_Buffer.size=size;
		_Buffer.epoch=_CpuEpoch;
        _Buffer.time_stamp=_current_time_stamp;
		_PQ.push(_Buffer);
	}

	static void NotifyIO(uint32_t device, uint64_t exec, uint8_t write, void* phys, uint64_t virt, unsigned int size, uint64_t tag) {
		_Buffer.type=DEVICE;
		_Buffer.id=device;
		_Buffer.write=write;
		_Buffer.phys=phys;
		_Buffer.size=size;
		_Buffer.epoch=_CpuEpoch;
        _Buffer.time_stamp= exec + _epoch_sc_time;
		_Buffer.tag = tag;
		_PQ.push(_Buffer);
	}

	static void NotifySesamCommand(uint64_t counter, bool start) {
		_Buffer.type=SESAMCOMMAND;
		_Buffer.tag=counter;
		_Buffer.write=start; // Reuse of write to indicate a start or finish command 
 		_Buffer.epoch=_CpuEpoch;
		_Buffer.time_stamp=_current_time_stamp;
 		_PQ.push(_Buffer);
 	}

	static void FillBiases(uint64_t* ts, uint32_t n) {
		for (uint32_t i = 0; i < n; i++)
			ts[i]=0;
		_CpuEpoch++;
		if(_Simulators.size()){
			while(_MemEpoch + EPOCHS <= _CpuEpoch) usleep(1); // stops the iss from running more than EPOCH epochs ahead
			_Mut[_CpuEpoch%EPOCHS].lock();
			for (MainMemCosim* cosim: _Simulators) {
				cosim->fillBiases(ts, n, _CpuEpoch);
			}
			_Mut[_CpuEpoch%EPOCHS].unlock();
		}
		_epoch_sc_time=(sc_time_stamp().to_seconds())*1000000000;
	}

	static uint64_t GetStat(uint32_t cpu, OuterStat st) {
		if (_Stats[cpu].find(st) != _Stats[cpu].end()) {
			return * _Stats[cpu][st];
		}
		return 0;
	}

	static void RegStat(uint32_t cpu, OuterStat st, uint64_t* ptr) {
		_Stats[cpu][st]=ptr;
	}

	static void Stop() {
		if (!_Stopped) {
			_Stopped=true;
			void* dt;
			pthread_join(_T,&dt);
		}
	}
	void setIOAccessPtr(IOAccessCosim* ptr){
		_IOAccessPtr = ptr;
	}

	void setMonitorPtr(SesamController* ptr){
		_Monitor = ptr;
 	}

	sc_time getCurrentTime(){
		return sc_time((double)_current_time_stamp,SC_NS);
 	}

	virtual void insert(uint32_t cpu, uint8_t write, uint8_t fetch, void* phys, unsigned int size, uint64_t epoch, uint64_t time_stamp)=0;
	virtual void fillBiases(uint64_t* ts, uint32_t n, uint64_t epoch)=0;

private:

	static void Add(MainMemCosim* simulator) {
		if(!_Inited) {
			Init();
		}
		_Simulators.push_back(simulator);
		cout.clear(); cout<<"Simulator added !"<<endl;
	}

	static void Init() {
		if (_Inited)
			throw runtime_error("MainMemCosim initialized twice !");
		_Inited=true;
		pthread_create(&_T,NULL,&MainMemCosim::Run,NULL);
	}

	static void* Run(void* unused) {
		Req k;
		vector<string> strParam;
		bool exitLoop=false;
		uint64_t tmpMemEpoch;
		while(1){
			if(_Stopped) break;
			tmpMemEpoch = _MemEpoch;
			while(tmpMemEpoch >= _CpuEpoch){ // Requests ordering needs the iss to run at least one epoch ahead
				usleep(1);
				if(_Stopped) break;
			}
			if (_PQ.try_pop(k)) {
				if(tmpMemEpoch != k.epoch){
					_PQ.push(k);// put the element back
					_MemEpoch=k.epoch;
					continue;   // need to check if _MemEpoch < _CpuEpoch
				}
				_Mut[tmpMemEpoch%EPOCHS].lock();
				do {
					if(tmpMemEpoch != k.epoch){
						_PQ.push(k);
						_MemEpoch=k.epoch;
						exitLoop=true;
						break;
					}
					if(k.type==DEVICE){
						for (MainMemCosim* cosim: _Simulators) {
							cosim->_IOAccessPtr->insert(k.id,k.write,k.phys,k.size,k.time_stamp,k.tag);
						}
					}else if(k.type==CPU){
						for (MainMemCosim* cosim: _Simulators) {
							cosim->insert(k.id,k.write,k.fetch,k.phys,k.size,_MemEpoch,k.time_stamp);
						}
					}
					else if(k.type==SESAMCOMMAND){
						for (MainMemCosim* cosim: _Simulators) {
							strParam.clear();
							if(k.write)	strParam.push_back("StartCapture"); 
							else		strParam.push_back("EndCapture"); 
							cosim->_Monitor->sesamCommand(strParam, k.tag);
						}
						exitLoop=true;
						break;
					}
				} while(_PQ.try_pop(k));
				_Mut[tmpMemEpoch%EPOCHS].unlock();
				if(exitLoop) exitLoop=false;
				else ++_MemEpoch;
			}
			else ++_MemEpoch; // an empty epoch!
		}
		return NULL;
	}

	static vector<MainMemCosim*> _Simulators;
	static bool _Inited;
	static pthread_t _T;

	static bool _Stopped;

	class compare_Req {
	public:
    	bool operator()(const Req& u, const Req& v) const {
			return (u.epoch > v.epoch || (u.epoch == v.epoch && u.time_stamp > v.time_stamp));
		}
	};
	static tbb::concurrent_priority_queue<MainMemCosim::Req, MainMemCosim::compare_Req> _PQ;

	static map<uint32_t, uint64_t*> _Stats[MAX_CPUS];

	static mutex _Mut[EPOCHS];

	static uint64_t _CurQuantum;

	static uint64_t _CpuEpoch;
	static uint64_t _MemEpoch;
	static uint64_t _epoch_sc_time;
	static uint64_t _current_time_stamp;

	IOAccessCosim* _IOAccessPtr;
	SesamController* _Monitor;
};

class SystemCCosimulator: public sc_module, public MainMemCosim {
public:
	SystemCCosimulator(sc_module_name name, uint32_t outPorts): sc_module(name) {
		mOutPorts.resize(outPorts);
		for (uint32_t i=0; i < outPorts; i++) {
			mOutPorts[i].first = new tlm_utils::simple_initiator_socket<SystemCCosimulator>((string("cosim_out_fetch_")+string(name)+to_string(i)).c_str());
			mOutPorts[i].second = new tlm_utils::simple_initiator_socket<SystemCCosimulator>((string("cosim_out_data_")+string(name)+to_string(i)).c_str());
			for (uint32_t e = 0;e<EPOCHS;e++)mTimes[i][e]=SC_ZERO_TIME;
		}
	}

	virtual ~SystemCCosimulator() {
		MainMemCosim::Stop();
	}

	bool convertAddr(void* host, uint64_t* p) {
		uint64_t H=(uint64_t)host;
		for (auto& t: mMaps) {
			uint64_t Hp=(uint64_t)get<0>(t);
			//cout<<"Checking range: "<<hex<<Hp<<endl;
			if (H>=Hp && H < Hp+get<2>(t)){
				*p = get<1>(t) + (H-Hp);
				return true;
			}
		}
		return false;
	}


	using portType=tlm_utils::simple_initiator_socket<SystemCCosimulator>;

	virtual void insert(uint32_t cpu, uint8_t write, uint8_t fetch, void* phys, unsigned int size, uint64_t epoch, uint64_t time_stamp) override {
		uint64_t addr=0;
		if (! fetch && !convertAddr(phys,&addr)){
			// std::cerr<<"Warning: Could not convert address "<<hex<<phys<<endl;
			//throw 0;
		} else if (fetch) {
			addr=(uint64_t)phys;
		}
		pld.set_data_ptr(NULL);
		pld.set_address(addr);
		pld.set_data_length(size);
		pld.set_command((write?tlm::TLM_WRITE_COMMAND:tlm::TLM_READ_COMMAND));
		src.type=0;//the source is a cpu
		src.cpu_id=cpu;
		//printf("MainMemCosim::insert::time_stamp=%ld\n",time_stamp);
		src.time_stamp=sc_time((double)time_stamp,SC_NS);
		//src.time_stamp=sc_time(time_stamp,SC_PS);
		pld.set_extension<SourceCpuExtension>(&src);
		portType& p=(fetch? *mOutPorts[cpu].first: *mOutPorts[cpu].second);
		p->b_transport(pld, mTimes[cpu][epoch%EPOCHS]);
		pld.clear_extension(&src);
	}

	virtual void fillBiases(uint64_t* ts, uint32_t n, uint64_t epoch) override {
		for (uint32_t i = 0; i < n; i++) {
			ts[i]+=(mTimes[i][epoch%EPOCHS].to_seconds()*1000000000.0);
			mTimes[i][epoch%EPOCHS]=SC_ZERO_TIME;
		}
	}

	vector<std::pair<portType*,portType*> > mOutPorts;
	tlm::tlm_generic_payload pld;
	SourceCpuExtension src;
	vector< tuple<void*,uint64_t,uint64_t> > mMaps;
	sc_time mTimes[MAX_CPUS][EPOCHS];

};

}

#endif /* _MAINMEMCOSIM_HPP_ */

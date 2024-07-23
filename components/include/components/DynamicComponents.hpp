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

#ifndef DYNAMICCOMPONENTS_HPP_
#define DYNAMICCOMPONENTS_HPP_

#include "PythonDevice.hpp"
#include <sstream>
#include <signal.h>
#include <atomic>
#include "VpsimIp.hpp"
#include "TargetIf.hpp"
#include "InitiatorIf.hpp"
#include "components/SmartUart.hpp"
#include "PL011Uart.hpp"
#include "gic.hpp"
#include "VirtioTlm.hpp"
#include "xuartps.hpp"
#include "AddressTranslator.hpp"
#include "SesamController.hpp"

#include "components/CallbackRegister.hpp"
#include <vpsimModule/ForwardSimpleSocket.hpp>
#include "peripherals/ItCtrl.hpp"
#include "peripherals/uart.hpp"
#include "memory/memory.hpp"
#include "connect/interconnect.hpp"
#include "memory/Cache.hpp"
#include "compute/arm.hpp"
#include "compute/arm64.hpp"

#include "RemoteInitiator.hpp"
#include "RemoteTarget.hpp"

#include "ExternalSimulator.hpp"

#include "SystemCTarget.hpp"
#include "MainMemCosim.hpp"
#include "IOAccessCosim.hpp"
#include "CoherenceInterconnect.hpp"

#define tostr(x) dynamic_cast<std::stringstream&&>(std::stringstream{}<<(x)).str()

namespace vpsim {


typedef  tlm::tlm_target_socket<> InPortType  ;
typedef  tlm::tlm_initiator_socket<> OutPortType;


struct DynamicExternalSimulator : public VpsimIp<InPortType, OutPortType> {
	DynamicExternalSimulator(string name) : VpsimIp(name), mModulePtr(nullptr) {
		registerRequiredAttribute("base_address");
		registerRequiredAttribute("size");
		registerRequiredAttribute("lib_path");
		registerRequiredAttribute("irq_n");
		registerRequiredAttribute("to_configure");
		registerRequiredAttribute("param");
		registerRequiredAttribute("interrupt_parent");
	}

	N_IN_PORTS_OVERRIDE(1);
	N_OUT_PORTS_OVERRIDE(0);
	MEMORY_MAPPED_OVERRIDE;

	virtual InPortType* getNextInPort() override {
		return &mModulePtr->mTargetSocket;
	}

	virtual OutPortType* getNextOutPort() override {
		throw runtime_error(VpsimIp::getName() + " : Memory has no out sockets.");
	}

	virtual void make() override {
		checkAttributes();
		mModulePtr=new ExternalSimulator(getName().c_str(),getSize(), getAttr("lib_path"));
		mModulePtr->setBaseAddress(getBaseAddress());
		mModulePtr->setInterruptLine(getAttrAsUInt64("irq_n"));
		mModulePtr->set_external_simulator(this->mModulePtr);
		mModulePtr->register_sync_cb(external_simulator_sync_cb);
		mModulePtr->register_irq_cb(external_simulator_interrupt_cb);
		//Gather all configuration parameters for the external simulator
		if(!mModulePtr->configured && getAttrAsUInt64("to_configure"))
		{
			string params = getAttr("param");
			stringstream ss(params);

			while (ss.good()) {
				string substr;
				getline(ss, substr, ',');
				mModulePtr->addParam(substr);
			}

			mModulePtr->config();
		}
	}

	virtual uint64_t getBaseAddress() override {
		return getAttrAsUInt64("base_address");
	}

	virtual uint64_t getSize() override {
		return getAttrAsUInt64("size");
	}

	virtual unsigned char* getActualAddress() override {
		return ( unsigned char*)-1;
	}

	virtual void finalize() override {
		VpsimIp* intp=VpsimIp::Find(getAttr("interrupt_parent"));
		if (intp==nullptr)
			throw runtime_error(getAttr("interrupt_parent") + " is not a valid interrupt parent for "+getName());
		mModulePtr->setInterruptParent(intp->getIrqIf());
	}

	ExternalSimulator* mModulePtr;

};

struct DynamicSystemCTarget : public VpsimIp<InPortType, OutPortType > {

	DynamicSystemCTarget(string name): VpsimIp(name), mModulePtr(nullptr){
		registerRequiredAttribute("base_address");
		registerRequiredAttribute("size");
		registerRequiredAttribute("interrupt_parent");
	}
	N_IN_PORTS_OVERRIDE(1);
	N_OUT_PORTS_OVERRIDE(1);
	MEMORY_MAPPED_OVERRIDE;

	virtual InPortType* getNextInPort() override {
		return &mModulePtr->mTargetSocket;
	}

	virtual OutPortType* getNextOutPort() override {
		return &mModulePtr->_out;
	}

	virtual void make() override {
		checkAttributes();
		mModulePtr=new SystemCTarget(getName().c_str(),getSize());
		mModulePtr->setBaseAddress(getBaseAddress());
	}


	virtual uint64_t getBaseAddress() override {
		return getAttrAsUInt64("base_address");
	}

	virtual uint64_t getSize() override {
		return getAttrAsUInt64("size");
	}

	virtual unsigned char* getActualAddress() override {
		return ( unsigned char*)-1;
	}

	virtual void finalize() override {
		VpsimIp* intp=VpsimIp::Find(getAttr("interrupt_parent"));
		if (intp==nullptr && getAttr("interrupt_parent") != "none")
			throw runtime_error(getAttr("interrupt_parent") + " is not a valid interrupt parent for "+getName());
		else if (intp)
			mModulePtr->setInterruptParent(intp->getIrqIf());
	}

	SystemCTarget* mModulePtr;
};

struct DynamicGIC :
        public gic,
        public VpsimIp<InPortType, OutPortType> {

public:
    explicit DynamicGIC(std::string name) :
			gic(name.c_str()),
            VpsimIp(name) {

        registerRequiredAttribute("base_address");

        registerRequiredAttribute("cpu_if_base");
        registerRequiredAttribute("cpu_if_size");
        registerRequiredAttribute("distributor_base");
        registerRequiredAttribute("distributor_size");

        registerRequiredAttribute("vdistributor_base");
        registerRequiredAttribute("vdistributor_size");
        registerRequiredAttribute("vcpu_if_base");
        registerRequiredAttribute("vcpu_if_size");
        registerRequiredAttribute("filter");
        registerRequiredAttribute("maintenance_irq");
    }

    ~DynamicGIC() override {
    }

    MEMORY_MAPPED_OVERRIDE;
    INTERRUPT_CONTROLLER_OVERRIDE;

    unsigned getMaxInPortCount() override {
        return 1;
    }

    unsigned getMaxOutPortCount() override {
        return 0;
    }

    InPortType *getNextInPort() override {
        return &mTargetSocket;
    }

    OutPortType *getNextOutPort() override {
        throw runtime_error(VpsimIp::getName() + " : GIC has no out sockets.");
    }

    void make() override {
        checkAttributes();
        setBaseAddress(getAttrAsUInt64("base_address"));
        setDistBase(getAttrAsUInt64("distributor_base"));
        setCPUBase(getAttrAsUInt64("cpu_if_base"));
        setDistSize(getAttrAsUInt64("distributor_size"));
        setCPUSize(getAttrAsUInt64("cpu_if_size"));

        setVDistBase(getAttrAsUInt64("vdistributor_base"));
		setVCPUBase(getAttrAsUInt64("vcpu_if_base"));
		setVDistSize(getAttrAsUInt64("vdistributor_size"));
		setVCPUSize(getAttrAsUInt64("vcpu_if_size"));

		setMaintenanceInterrupt(getAttrAsUInt64("maintenance_irq"));
    }

    uint64_t getBaseAddress() override {
        return getAttrAsUInt64("base_address");
    }

    uint64_t getSize() override {
        return 0x100000;
    }

    unsigned char *getActualAddress() override {
        return (unsigned char*)getLocalMem();
    }


    virtual void finalize() override {

		if (AllInstances.find(getAttr("filter")) != AllInstances.end()) {

			VpsimIp<InPortType,OutPortType>::MapTypeIf(getAttr("filter"),
					[](VpsimIp<InPortType,OutPortType>* ip) { return ip->isProcessor(); },
					[this](VpsimIp<InPortType,OutPortType>* ip) {
						this->connectCpu(ip->getIrqIf(), ip->getAttrAsUInt64("cpu_id"));
					}
				);
		}

	}

    InterruptIf *getIrqIf() override { return this; }

    virtual void setStatsAndDie() override {
    }
};

struct DynamicRemoteInitiator
	: public VpsimIp<InPortType, OutPortType >
{

public:
	DynamicRemoteInitiator(std::string name):
		VpsimIp(name),
	    mModulePtr(nullptr) {

		registerRequiredAttribute("remote_ip");
		//registerRequiredAttribute("port");
		//registerRequiredAttribute("irq_ip");
		//registerRequiredAttribute("irq_port");
		registerOptionalAttribute("poll_period", "1000");
	}




	N_IN_PORTS_OVERRIDE(0);
	N_OUT_PORTS_OVERRIDE(1);

	virtual InPortType* getNextInPort() override {
		throw runtime_error("No input ports for CPU.");
	}

	virtual OutPortType* getNextOutPort() override {
		if (!mModulePtr) {
			throw runtime_error("Please call make() before handling ports.");
		}
        return mModulePtr->mInitiatorSocket[mOutPortCounter];
	}

	virtual void make() override {
		if (mModulePtr != nullptr) {
			throw runtime_error("make() already called for DynamicRemoteInitiator");
		}
		checkAttributes();
		mModulePtr = new RemoteInitiator(getName().c_str());

		uint16_t port,irq_port;
		cout<<"Port for remote transactions: ";
		cin>>port;

		mModulePtr->setIp(getAttr("remote_ip"));
		mModulePtr->setPort(port);
		mModulePtr->setChannel(getAttr("remote_ip")+":"+to_string(port));

		cout<<"Port for remote interrupts: ";
		cin>>irq_port;

		mModulePtr->setIrqIp(getAttr("remote_ip"));
		mModulePtr->setIrqPort(irq_port);
		mModulePtr->setIrqChannel(getAttr("remote_ip")+":"+to_string(irq_port));

		mModulePtr->setPollPeriod(getAttrAsUInt64("poll_period"));
    }

	virtual void addDmiAddress(std::string targetIpName, uint64_t baseAddr, uint64_t size, unsigned char* pointer, bool cached, bool has_dmi) override {
	}

	virtual void finalize() override {
	}

    void pushStats() override {
    }

	virtual void setStatsAndDie() override {
		if (mModulePtr) {
			delete mModulePtr;
		}
	}

	InterruptIf* getIrqIf() override {
		return mModulePtr;
	}

private:
	RemoteInitiator* mModulePtr;
};

struct DynamicRemoteTarget : public VpsimIp<InPortType, OutPortType > {

	DynamicRemoteTarget(string name): VpsimIp(name), mModulePtr(nullptr){
		registerRequiredAttribute("base_address");
		registerRequiredAttribute("size");
		registerRequiredAttribute("channel");
		registerRequiredAttribute("irq_channel");
		registerRequiredAttribute("irq_n");
		registerRequiredAttribute("interrupt_parent");
		registerOptionalAttribute("poll_period", "1000");
	}
	N_IN_PORTS_OVERRIDE(1);
	N_OUT_PORTS_OVERRIDE(0);
	MEMORY_MAPPED_OVERRIDE;

	virtual InPortType* getNextInPort() override {
		return &mModulePtr->mTargetSocket;
	}

	virtual OutPortType* getNextOutPort() override {
		throw runtime_error(VpsimIp::getName() + " : Memory has no out sockets.");
	}

	virtual void make() override {
		checkAttributes();
		mModulePtr=new RemoteTarget(getName().c_str(),getSize());
		mModulePtr->setBaseAddress(getBaseAddress());
		mModulePtr->setChannel(getAttr("channel"));
		mModulePtr->setIrqChannel(getAttr("irq_channel"));
		mModulePtr->setInterruptLine(getAttrAsUInt64("irq_n"));
		mModulePtr->setPollPeriod(getAttrAsUInt64("poll_period"));
	}


	virtual uint64_t getBaseAddress() override {
		return getAttrAsUInt64("base_address");
	}

	virtual uint64_t getSize() override {
		return getAttrAsUInt64("size");
	}

	virtual unsigned char* getActualAddress() override {
		return ( unsigned char*)-1;
	}

	virtual void finalize() override {
		VpsimIp* intp=VpsimIp::Find(getAttr("interrupt_parent"));
		if (intp==nullptr)
			throw runtime_error(getAttr("interrupt_parent") + " is not a valid interrupt parent for "+getName());
		mModulePtr->setInterruptParent(intp->getIrqIf());
	}

	RemoteTarget* mModulePtr;
};

struct DynamicSystemCCosimulator
	: public VpsimIp<InPortType, OutPortType >
{

public:
	DynamicSystemCCosimulator(std::string name):
		VpsimIp(name),
	    mModulePtr(nullptr) {
		registerRequiredAttribute("n_out_ports");
	}


	NEEDS_DMI_OVERRIDE;

	N_IN_PORTS_OVERRIDE(0);
	N_OUT_PORTS_OVERRIDE(getAttrAsUInt64("n_out_ports")*2);

	virtual InPortType* getNextInPort() override {
		throw runtime_error("No input ports for CPU.");
	}

	virtual OutPortType* getNextOutPort() override {
		if (!mModulePtr) {
			throw runtime_error("Please call make() before handling ports.");
		}
		int cpu = mOutPortCounter / 2;
		int pt = mOutPortCounter % 2;
        return (pt == 0 ? mModulePtr->mOutPorts[cpu].first: mModulePtr->mOutPorts[cpu].second);
	}

	virtual void make() override {
		if (mModulePtr != nullptr) {
			throw runtime_error("make() already called for DynamicSystemCCosimulator");
		}
		checkAttributes();
		mModulePtr = new SystemCCosimulator (sc_module_name(getName().c_str()),
				getAttrAsUInt64("n_out_ports"));

		unsigned cpu=0;
		for (cpu=0; cpu < getAttrAsUInt64("n_out_ports"); cpu++) {
			char ptName[512];
			sprintf(ptName, "fetch_port_%u", cpu);
			addOutPort(string(ptName));
			sprintf(ptName, "data_port_%u", cpu);
			addOutPort(string(ptName));
		}
    }

	virtual void addDmiAddress(std::string targetIpName, uint64_t baseAddr, uint64_t size, unsigned char* pointer, bool cached, bool has_dmi) override {
		if (has_dmi) {
			mModulePtr->mMaps.push_back(make_tuple((void*)pointer,baseAddr,size));
		}
	}

	virtual void finalize() override {

	}

	virtual void setStatsAndDie() override {
		if (mModulePtr) {
			delete mModulePtr;
		}
	}

private:
	SystemCCosimulator* mModulePtr;
	friend struct DynamicIOAccessCosimulator;
};

struct DynamicIOAccessCosimulator
	: public VpsimIp<InPortType, OutPortType >
{

public:
	DynamicIOAccessCosimulator(std::string name):
		VpsimIp(name),
	    mModulePtr(nullptr) {
		registerRequiredAttribute("n_out_ports");
	}


	NEEDS_DMI_OVERRIDE;

	N_IN_PORTS_OVERRIDE(0);
	N_OUT_PORTS_OVERRIDE(getAttrAsUInt64("n_out_ports"));

	virtual InPortType* getNextInPort() override {
		throw runtime_error("No input ports for DynamicIOAccessCosimulator.");
	}

	virtual OutPortType* getNextOutPort() override {
		if (!mModulePtr) {
			throw runtime_error("Please call make() before handling ports.");
		}
        return mModulePtr->mOutPorts[mOutPortCounter];
	}

	virtual void make() override {
		if (mModulePtr != nullptr) {
			throw runtime_error("make() already called for DynamicIOAccessCosimulator");
		}
		checkAttributes();
		mModulePtr = new IOAccessCosimulator (sc_module_name(getName().c_str()),
				getAttrAsUInt64("n_out_ports"));

		unsigned iodevice=0;
		for (iodevice=0; iodevice < getAttrAsUInt64("n_out_ports"); iodevice++) {
			char ptName[512];
			sprintf(ptName, "dma_port_%d", iodevice);
			addOutPort(string(ptName));
		}
    }

	virtual void addDmiAddress(std::string targetIpName, uint64_t baseAddr, uint64_t size, unsigned char* pointer, bool cached, bool has_dmi) override {
		if (has_dmi) {
			mModulePtr->mMaps.push_back(make_tuple((void*)pointer,baseAddr,size));//Cosim address space. necessary for tests but should be removed
			mModulePtr->mMaps.push_back(make_tuple((void*)0,baseAddr,size));//Memory address space for IO notification comes from guest linux. Thus, it is different from that of Cosim
		}
	}

	virtual void finalize() override {
		VpsimIp* ip = VpsimIp::Find("SystemCCosim0");
		DynamicSystemCCosimulator* cosim=dynamic_cast<DynamicSystemCCosimulator*>(ip);
		cosim->mModulePtr->setIOAccessPtr(mModulePtr);

	}

	virtual void setStatsAndDie() override {
		if (mModulePtr) {
			delete mModulePtr;
		}
	}

private:
	IOAccessCosimulator* mModulePtr;
};

struct DynamicArm
	: public VpsimIp<InPortType, OutPortType >
{

public:
	DynamicArm(std::string name):
		VpsimIp(name),
	    mModulePtr(nullptr) {

		registerRequiredAttribute("model");
		registerRequiredAttribute("iss");
		registerRequiredAttribute("cpu_id");
		registerRequiredAttribute("quantum");
		registerRequiredAttribute("gdb_enable");
		registerRequiredAttribute("stop_on_first_core_done");
		registerRequiredAttribute("ram_size");
		registerRequiredAttribute("kernel");
		registerRequiredAttribute("reset_pc");

		registerOptionalAttribute("force_lt", "0");
		registerOptionalAttribute("quantum_enable", "1");

		registerOptionalAttribute("wait_for_interrupt", "0");

		//registerOptionalAttribute("cmdline", "");
		//registerOptionalAttribute("initrd", "");
	}


	NEEDS_DMI_OVERRIDE;
	PROCESSOR_OVERRIDE;

	N_IN_PORTS_OVERRIDE(0);
	N_OUT_PORTS_OVERRIDE(2);

	virtual InPortType* getNextInPort() override {
		throw runtime_error("No input ports for CPU.");
	}

	virtual OutPortType* getNextOutPort() override {
        if (!mModulePtr) {
            throw runtime_error("Please call make() before handling ports.");
        }
        return mModulePtr->mInitiatorSocket[mOutPortCounter];
	}

	virtual void make() override {
		if (mModulePtr != nullptr) {
			throw runtime_error("make() already called for DynamicArm");
		}
		checkAttributes();
		mModulePtr = new arm ( getName().c_str(),
				getAttr("model"),
				IssFinder(getAttr("iss")),
				getAttrAsUInt64("cpu_id"),
				getAttrAsUInt64("quantum")/1000,
				(bool)getAttrAsUInt64("gdb_enable"),
				(bool)getAttrAsUInt64("stop_on_first_core_done"),
				getAttrAsUInt64("reset_pc"));

		if (getAttrAsUInt64("quantum_enable")) {
			mModulePtr->setQuantumEnable(true);
		} else {
			mModulePtr->setQuantumEnable(false);
		}

		if (getAttrAsUInt64("force_lt")) {
			mModulePtr->setForceLt(true);
		} else {
			mModulePtr->setForceLt(false);
		}

		mModulePtr->setWaitForInterrupt(getAttrAsUInt64("wait_for_interrupt"));

		addOutPort("to_icache");
		addOutPort("to_dcache");

        auto getDoTLM = [this](uint64_t base, uint64_t end, bool isFetch) {
            //icache is on port 0, dcache is on port 1
            AddrSpace addr(base, end);

            const auto& paramF =  this->mVpsimModule->getBlockingTLMEnabled(0, addr);
            const auto& paramRW =  this->mVpsimModule->getBlockingTLMEnabled(1, addr);

            this->mIssTLMParam.emplace_back(make_pair(addr, uint64_t(paramF) | (uint64_t(paramRW) << 1)));
            return &this->mIssTLMParam.rbegin()->second;
        };
        mModulePtr->registerIssGetDoTLM(move(getDoTLM));

        auto updateIssDoTLM = [this]{
            //icache is on port 0, dcache is on port 1
            for(auto& param: this->mIssTLMParam){
                const auto& addr = param.first;
                param.second = static_cast<uint64_t>(mVpsimModule->getBlockingTLMEnabled(0, addr)) |
                               (static_cast<uint64_t>(mVpsimModule->getBlockingTLMEnabled(1, addr)) << 1);
            }
        };

        ParamManager::get().registerUpdateHook(getName(), move(updateIssDoTLM));
	}

	virtual void addDmiAddress(std::string targetIpName, uint64_t baseAddr, uint64_t size, unsigned char* pointer, bool cached, bool has_dmi) override {
		if (mModulePtr == nullptr) {
			throw runtime_error(getName() + " : calling addDmiAddress() before make() !!!");
		}
		mModulePtr->add_map_dmi ( targetIpName, baseAddr, size, pointer );
		if (has_dmi) {
			// mModulePtr->setDmiRange(0, baseAddr, size, pointer);
		}
	}

	virtual void finalize() override {
		auto nCores=VpsimIp<InPortType,OutPortType>::AllInstances.find("Arm")->second.size();
		std::cout<<"Number of cores: "<<nCores<<endl;
		if (getAttr("kernel")!="")
			mModulePtr->iss_load_elf(getAttrAsUInt64("ram_size"), (char*)getAttr("kernel").c_str(),
				nullptr,nullptr);
	}

    void pushStats() override {
        if(mSegmentedStats.empty()){
            mSegmentedStats.push_back({
                                              {"instructions",  "0"},
                                              {"data_access", "0"}
                                      });
        }

        const auto& back = mSegmentedStats.back();
        auto instructions = to_string(mModulePtr->getInstructionCount() - stoull(back.at("instructions")));
        auto dataAccesses = to_string(mModulePtr->getDataAccessCount() - stoull(back.at("data_access")));

        mSegmentedStats.push_back({
                                          {"instructions",  instructions},
                                          {"data_access", dataAccesses}
                                  });
    }

	virtual void setStatsAndDie() override {
		if (mModulePtr) {
			mStats["instructions"] = tostr(mModulePtr->getInstructionCount());
			mStats["data_access"] = tostr(mModulePtr->getDataAccessCount());

			delete mModulePtr;
		}
	}

	InterruptIf *getIrqIf() override { return mModulePtr; }

	IssWrapper* getIssHandle() { return mModulePtr; }

private:
	arm* mModulePtr;
    std::deque<std::pair<AddrSpace, uint64_t>> mIssTLMParam;
};

struct DynamicArm64
	: public VpsimIp<InPortType, OutPortType >
{

public:
	DynamicArm64(std::string name):
		VpsimIp(name),
	    mModulePtr(nullptr) {

		registerRequiredAttribute("model");
		registerRequiredAttribute("iss");
		registerRequiredAttribute("cpu_id");
		registerRequiredAttribute("quantum");
		registerRequiredAttribute("gdb_enable");
		registerRequiredAttribute("stop_on_first_core_done");
		registerRequiredAttribute("ram_size");
		registerRequiredAttribute("kernel");
		registerRequiredAttribute("reset_pc");
		registerRequiredAttribute("io_only");
		registerRequiredAttribute("delay_before_boot");
		registerRequiredAttribute("log");
		registerRequiredAttribute("log_file");

		registerOptionalAttribute("force_lt", "0");
		registerOptionalAttribute("quantum_enable", "1");

		registerOptionalAttribute("wait_for_interrupt", "0");
		registerOptionalAttribute("gic", "none");

		//registerOptionalAttribute("cmdline", "");
		//registerOptionalAttribute("initrd", "");
	}


	NEEDS_DMI_OVERRIDE;
	PROCESSOR_OVERRIDE;

	N_IN_PORTS_OVERRIDE(0);
	N_OUT_PORTS_OVERRIDE(2);

	virtual InPortType* getNextInPort() override {
		throw runtime_error("No input ports for CPU.");
	}

	virtual OutPortType* getNextOutPort() override {
		if (!mModulePtr) {
			throw runtime_error("Please call make() before handling ports.");
		}
		return  mModulePtr->mInitiatorSocket[mOutPortCounter];
	}

	void pushStats() override {
		if(mSegmentedStats.empty()){
			mSegmentedStats.push_back({
											  {"instructions",  "0"},
											  {"data_access", "0"}
									  });
		}

		const auto& back = mSegmentedStats.back();
		auto instructions = to_string(mModulePtr->getInstructionCount() - stoull(back.at("instructions")));
		auto dataAccesses = to_string(mModulePtr->getDataAccessCount() - stoull(back.at("data_access")));

		mSegmentedStats.push_back({
										  {"instructions",  instructions},
										  {"data_access", dataAccesses}
								  });
	}

	virtual void make() override {
		if (mModulePtr != nullptr) {
			throw runtime_error("make() already called for DynamicArm");
		}
		checkAttributes();
		mModulePtr = new arm64 ( getName().c_str(),
				getAttr("model"),
				IssFinder(getAttr("iss")),
				getAttrAsUInt64("cpu_id"),
				getAttrAsUInt64("quantum")/1000,
				(bool)getAttrAsUInt64("gdb_enable"),
				(bool)getAttrAsUInt64("stop_on_first_core_done"),
				getAttrAsUInt64("reset_pc"),
				getAttrAsUInt64("log"), getAttr("log_file").c_str());

		if (getAttrAsUInt64("quantum_enable")) {
			mModulePtr->setQuantumEnable(true);
		} else {
			mModulePtr->setQuantumEnable(false);
		}

		mModulePtr->setIoOnly(getAttrAsUInt64("io_only"));
		//mModulePtr->setDelayBeforeBoot(sc_time(getAttrAsUInt64("delay_before_boot"),SC_PS));
		mModulePtr->setDelayBeforeBoot(sc_time(getAttrAsUInt64("delay_before_boot"),SC_NS));
		//mModulePtr->setLog(getAttrAsUInt64("log"), getAttr("log_file").c_str());

		if (getAttrAsUInt64("force_lt")) {
			mModulePtr->setForceLt(true);
		} else {
			mModulePtr->setForceLt(false);
		}

		mModulePtr->setWaitForInterrupt(getAttrAsUInt64("wait_for_interrupt"));


		addOutPort("to_icache");
		addOutPort("to_dcache");

		auto getDoTLM = [this](uint64_t base, uint64_t end, bool isFetch) {
			//icache is on port 0, dcache is on port 1
			AddrSpace addr(base, end);

			const auto& paramF =  this->mVpsimModule->getBlockingTLMEnabled(0, addr);
			const auto& paramRW =  this->mVpsimModule->getBlockingTLMEnabled(1, addr);

			this->mIssTLMParam.emplace_back(make_pair(addr, uint64_t(paramF) | (uint64_t(paramRW) << 1)));
			return &this->mIssTLMParam.rbegin()->second;
		};
		mModulePtr->registerIssGetDoTLM(move(getDoTLM));

		auto updateIssDoTLM = [this]{
			//icache is on port 0, dcache is on port 1
			for(auto& param: this->mIssTLMParam){
				const auto& addr = param.first;
				param.second = static_cast<uint64_t>(mVpsimModule->getBlockingTLMEnabled(0, addr)) |
							   (static_cast<uint64_t>(mVpsimModule->getBlockingTLMEnabled(1, addr)) << 1);
			}
		};

		ParamManager::get().registerUpdateHook(getName(), move(updateIssDoTLM));
	}

	virtual void addDmiAddress(std::string targetIpName, uint64_t baseAddr, uint64_t size, unsigned char* pointer, bool cached, bool has_dmi) override {
		if (mModulePtr == nullptr) {
			throw runtime_error(getName() + " : calling addDmiAddress() before make() !!!");
		}
		mModulePtr->add_map_dmi ( targetIpName, baseAddr, size, pointer );
		if (has_dmi) {
			// mModulePtr->setDmiRange(0, baseAddr, size, pointer);
		}
	}

	virtual void addMonitor(uint64_t base, uint64_t size) override {
		mModulePtr->monitorRange(base,size);
	}

	virtual void removeMonitor(uint64_t base, uint64_t size) override {
		mModulePtr->removeMonitor(base,size);
	}

	virtual void showMonitor() override {
		mModulePtr->showMonitor();
	}

	virtual void finalize() override {
		auto nCores=VpsimIp<InPortType,OutPortType>::AllInstances.find("Arm64")->second.size();
		std::cout<<"Number of cores: "<<nCores<<endl;
		if (getAttr("kernel")!="")
			mModulePtr->iss_load_elf(getAttrAsUInt64("ram_size"), (char*)getAttr("kernel").c_str(),
				nullptr,nullptr);
		VpsimIp *par=VpsimIp::Find(this->getAttr("gic"));
		if (par==nullptr)
			throw runtime_error("AARCH64: Please specify the gic attribute to point to an actual GIC.");
		this->mModulePtr->setGic(par->getIrqIf());
		if (!getAttrAsUInt64("io_only"))
			dynamic_cast<DynamicGIC*>(par)->connectCpu(mModulePtr,mModulePtr->get_cpu_id());
		else
			VpsimIp::MapTypeIf("Arm64",
					[](VpsimIp* ip)  { return true; },
					[this](VpsimIp* ip){
						dynamic_cast<DynamicArm64*>(ip)->mModulePtr->addIoEvent(this->mModulePtr->getIoEvent());
					}
		    );
	}

	/*virtual VpsimModule* asModule() override {
		return mModulePtr;
	}*/

	virtual void setStatsAndDie() override {
		if (mModulePtr) {
			mStats["instructions"] = tostr(mModulePtr->getInstructionCount());
			mStats["data_access"] = tostr(mModulePtr->getDataAccessCount());

			delete mModulePtr;
		}
	}

	InterruptIf *getIrqIf() override { return mModulePtr; }

	IssWrapper* getIssHandle() { return mModulePtr; }

private:
	arm64* mModulePtr;
    std::deque<std::pair<AddrSpace, uint64_t>> mIssTLMParam;
};



struct DynamicVirtioProxy : public VpsimIp<InPortType, OutPortType >, public VirtioTlm {

	DynamicVirtioProxy(string name): VpsimIp(name), VirtioTlm(name.c_str()) {
		registerRequiredAttribute("provider_instance");
		registerRequiredAttribute("base_address");
		registerRequiredAttribute("irq");
		registerRequiredAttribute("device_type");
		registerRequiredAttribute("backend_config");
		registerOptionalAttribute("mac", "52:55:00:d1:55:01");
	}
	N_IN_PORTS_OVERRIDE(1);
	N_OUT_PORTS_OVERRIDE(0);
	MEMORY_MAPPED_OVERRIDE;

	virtual InPortType* getNextInPort() override {
		return &mTargetSocket;
	}

	virtual OutPortType* getNextOutPort() override {
		throw runtime_error(VpsimIp::getName() + " : Memory has no out sockets.");
	}

	virtual void make() override {
		checkAttributes();
		setBaseAddress(getBaseAddress());
	}


	virtual uint64_t getBaseAddress() override {
		return getAttrAsUInt64("base_address");
	}

	virtual uint64_t getSize() override {
		return 0x1000;
	}

	virtual unsigned char* getActualAddress() override {
		return ( unsigned char*)-1;
	}

	virtual void finalize() override {
		cout<<"VIRTIO: Initializing callbacks..."<<endl;
		pair<string,VpsimIp*> issProvider = VpsimIp::FindWithType(getAttr("provide_instance"));
		IssWrapper* wrapper = nullptr;
		if (issProvider.first == "Arm64") {
			wrapper = dynamic_cast<DynamicArm64*>(issProvider.second)->getIssHandle();
		} else if (issProvider.first == "Arm") {
			wrapper = dynamic_cast<DynamicArm*>(issProvider.second)->getIssHandle();
		} else {
			throw runtime_error(getAttr("provider_instance") + " : Does not provide targets. Please provide valid ISS instance name.");
		}

		if (!wrapper) {
			throw runtime_error(getAttr("provider_instance") + " : Unable to get ISS instance for VIRTIO proxy.");
		}


		typedef void* (*sysbus_create_simple_t)(const char*, uint64_t, void*);
		auto create_f = (sysbus_create_simple_t)wrapper->get_symbol("vpsim_bus_create");
		create_f("virtio-mmio", getAttrAsUInt64("base_address"), (void*) getAttrAsUInt64("irq"));

		typedef void (*virtio_mmio_get_read_cb_t)(virtio_mmio_read_type* cb);
		typedef void (*virtio_mmio_get_write_cb_t)(virtio_mmio_write_type* cb);
		typedef void (*virtio_mmio_get_proxy_t)(void** proxy);

		auto virtio_mmio_get_proxy = (virtio_mmio_get_proxy_t)wrapper->get_symbol("virtio_mmio_get_proxy");
		auto virtio_mmio_get_read_cb =(virtio_mmio_get_read_cb_t) wrapper->get_symbol("virtio_mmio_get_read_cb");
		auto virtio_mmio_get_write_cb = (virtio_mmio_get_write_cb_t)wrapper->get_symbol("virtio_mmio_get_write_cb");

		virtio_mmio_get_read_cb(&mRdFct);
		virtio_mmio_get_write_cb(&mWrFct);
		virtio_mmio_get_proxy(&mProxyPtr);

		typedef void(*io_step_t)();
		mIoStep = (io_step_t) wrapper->get_symbol("io_step_tlm");

		typedef void (*create_dev_t)(const char* name, const char* args, const char* extra);
		create_dev_t create_dev;
		string extra;
		// Now create device and backend
		if (getAttr("device_type") == "blk") {
			create_dev = (create_dev_t) wrapper->get_symbol("vpsim_create_blk");
		} else if (getAttr("device_type") == "net") {
			create_dev = (create_dev_t) wrapper->get_symbol("vpsim_create_net");
			extra = getAttr("mac");
		} else {
			throw runtime_error(getAttr("device_type") + " is not a known virtio device type, known types are: blk, net.");
		}

		create_dev(VpsimIp::getName().c_str(), getAttr("backend_config").c_str(), extra.c_str());
	}
};

struct DynamicExternalCPU
	: public VpsimIp<InPortType, OutPortType >,
	  public VpsimModule,
	  public InterruptIf
{

public:
	DynamicExternalCPU(std::string name):
		VpsimIp(name),
		VpsimModule(name, moduleType::intermediate, 1),
		lib(nullptr){

		registerRequiredAttribute("lib_path");
		registerRequiredAttribute("quantum");
		registerRequiredAttribute("kernel");
		registerRequiredAttribute("model_name");
		registerRequiredAttribute("extra_arg");
		registerRequiredAttribute("gic");
		registerRequiredAttribute("id");
		registerRequiredAttribute("finalize");
		registerRequiredAttribute("n_smp_cpus");

		destry=nullptr;
	}

	~DynamicExternalCPU() {
		if (destry)
			destry();
	}

	//NEEDS_DMI_OVERRIDE;
	// PROCESSOR_OVERRIDE;  --> this only works for our ISSWrapper

	N_IN_PORTS_OVERRIDE(0);
	N_OUT_PORTS_OVERRIDE(1);

	using destroyFctType = void (*)();
	using extIrqFctType = void (*)(uint64_t , uint32_t );
	destroyFctType destry;

	virtual InPortType* getNextInPort() override {
		throw runtime_error("No input ports for CPU.");
	}

	virtual OutPortType* getNextOutPort() override {
		if (!lib) {
			throw runtime_error("Please call make() before handling ports.");
		}
		return nullptr;
	}

	virtual void connect(std::string outPortAlias, VpsimIp<InPortType, OutPortType>* otherIp, std::string inPortAlias) override {
		//std::cout<<"Connecting "<<getName()<<" to "<<otherIp->getName()<<std::endl;

		WrappedInSock thatSock = otherIp->getInPort(inPortAlias);

		if (thatSock.second != nullptr) {
			this->addSuccessor(*thatSock.second, 0);
		}
		using extFctType = void (*)(const char*, uint64_t, const char*, const char*, InPortType*);
		extFctType createAndConnectCPU = (extFctType)dlsym(lib, (getAttr("model_name") + "_createAndConnectCPU") . c_str());
		if (!createAndConnectCPU) {
			throw runtime_error(string("Could not load initializer function !") + dlerror());
		}

		destry = (destroyFctType)dlsym(lib, (getAttr("model_name") + "_destroy") . c_str());

		if (!destry) {
			cout<<"warning: "<< (string("Could not load destructor function !") + dlerror()) <<endl;
		}



		createAndConnectCPU(getName().c_str(),
				getAttrAsUInt64("quantum")/1000,
				getAttr("kernel").c_str(),
				getAttr("extra_arg").c_str(),
				thatSock.first);
	}

	virtual void make() override {
		if (lib != nullptr) {
			throw runtime_error("make() already called for DynamicExternalCPU");
		}
		checkAttributes();
		// load shared library
		cout<<"Opening library: "<<getAttr("lib_path")<<endl;
		lib = dlopen(getAttr("lib_path").c_str(), RTLD_LOCAL | RTLD_LAZY);
		if (!lib) {
			throw runtime_error(string("Could not load External CPU : ") + dlerror());
		}


		ext_update_irq = (extIrqFctType)dlsym(lib, (getAttr("model_name") + "_update_irq") . c_str());
		if (!ext_update_irq) {
			throw runtime_error(string("Could not load update_irq function !") + dlerror());
		}
	}

	virtual void finalize() override {
		if (!getAttrAsUInt64("finalize"))
			return;
		VpsimIp *par=VpsimIp::Find(this->getAttr("gic"));
		if (par==nullptr)
			throw runtime_error("External CPU: Please specify the gic attribute to point to an actual GIC.");
		for (uint32_t i=0; i < getAttrAsUInt64("n_smp_cpus"); i++)
			dynamic_cast<DynamicGIC*>(par)->connectCpu(this,getAttrAsUInt64("id")+i);

		typedef void (*update_irq_cb_t)(void* instance, uint32_t id, int value, int line);
		typedef void (*register_irq_cb_t)(update_irq_cb_t cb, void* gic, uint32_t id);
		auto reg_cb = (register_irq_cb_t) dlsym(lib, "register_external_irq_callback");
		if (!reg_cb) {
			throw runtime_error(string("Could not load register_irq_cb function !") + dlerror());
		}
		reg_cb(update_irq_cb, (void*)dynamic_cast<DynamicGIC*>(par), getAttrAsUInt64("id"));

		typedef void(*set_id_t)(uint64_t id);
		auto set_id = (set_id_t)dlsym(lib, "vpsim_set_id");
		if (!set_id) {
			throw runtime_error("set_id function not found.\n");
		}
		set_id(getAttrAsUInt64("id"));
	}

	static void update_irq_cb(void* instance, uint32_t id, int value, int line) {
		((DynamicGIC*)instance)->update_irq(value, line | ((1<<id)<<16));
	}

	virtual void setStatsAndDie() override {
		if (lib) {
			dlclose(lib);
		}
	}

	void update_irq(uint64_t val, uint32_t irq_idx) override {
		ext_update_irq(val, irq_idx);
	}

	InterruptIf *getIrqIf() override { return this; }

private:
	void* lib;
	extIrqFctType ext_update_irq;
};

struct DynamicCache : public VpsimIp<InPortType, OutPortType> {

    DynamicCache(std::string name):
      VpsimIp(name),
      mModulePtr(nullptr) {

      registerRequiredAttribute("latency");
      registerRequiredAttribute("size");
      registerRequiredAttribute("line_size");
      registerRequiredAttribute("associativity");
      registerRequiredAttribute("repl_policy");
      registerRequiredAttribute("writing_policy");
      registerRequiredAttribute("allocation_policy");
      registerRequiredAttribute("cpu");
      registerRequiredAttribute("local");
      registerRequiredAttribute("id");
      registerRequiredAttribute("level");
      registerRequiredAttribute("levels_number");
      registerRequiredAttribute("is_home");
      registerOptionalAttribute("inclusion_higher", "NINE");
      registerOptionalAttribute("inclusion_lower", "NINE");
      registerOptionalAttribute("is_coherent", "0");
      registerOptionalAttribute("home_base_address", "0");
      registerOptionalAttribute("home_size", "0");
      registerOptionalAttribute("l1i_simulate", "0");
    }
    inline unsigned int getnIn () {
      unsigned int nin = 1; // port for data from Level-1
      if (getAttrAsUInt64("level")<getAttrAsUInt64("levels_number")) nin++; // add port for invalidation if not LLC
      if (getAttrAsUInt64("level")==2 && getAttrAsUInt64("l1i_simulate")) nin++; // add port for instruction cache if L2
      return nin;
    }
  /*inline unsigned int getnOut () { //Assuming 3 levels, TODO enhance
      if (getAttrAsUInt64("level") == 1) return 1;
      else if (getAttrAsUInt64("level") == 3) return 1; // one output for the NoC
      else if (getAttrAsUInt64("level") == 2) return 2;
      else return 1;
    }*/
    inline unsigned int getnOut () { //Assuming 3 levels, TODO enhance
      unsigned int nout = 1; // port for data
      if (getAttrAsUInt64("level")>1 && !getAttrAsUInt64("is_home")) nout++; // add port for invalidation if not connected to NoC
      // if home, unique output for the NoC
      return nout;
    }
    NEEDS_DMI_OVERRIDE;
    ID_MAPPED_OVERRIDE;
    //MEMORY_MAPPED_OVERRIDE;
    //ISHOME (getAttrAsUInt64("level")==3); //Assuming 3 levels, TODO enhance
    N_IN_PORTS_OVERRIDE(getnIn());
    N_OUT_PORTS_OVERRIDE(getnOut());
    virtual InPortType* getNextInPort() override {
      if (!mModulePtr) throw runtime_error(getName() + "Please call make() before handling ports.");
      return &mModulePtr->socket_in[mInPortCounter];
    }
    virtual OutPortType* getNextOutPort() override {
      if (!mModulePtr) throw runtime_error(getName() + " Please call make() before handling ports.");
      return &mModulePtr->socket_out[mOutPortCounter];
    }
    virtual void make() override {
      if (mModulePtr != nullptr) throw runtime_error("make() already called for DynamicCache");
      checkAttributes();
      CacheReplacementPolicy repl;
      if (getAttr("repl_policy") == "LRU") repl = LRU;
      else if (getAttr("repl_policy") == "FIFO") repl = FIFO;
      else if (getAttr("repl_policy") == "MRU") repl = MRU;
      else throw runtime_error(getAttr("repl_policy") + " Unknown replacement policy");
      CacheWritePolicy writePol;
      if (getAttr("writing_policy") == "WBack") writePol = WBack;
      else if (getAttr("writing_policy") == "WThrough") writePol = WThrough;
      else throw runtime_error(getAttr("write_policy") + " Unknown writing policy");
      CacheAllocPolicy allocPol;
      if (getAttr("allocation_policy") == "WAllocate") allocPol = WAllocate;
      else if (getAttr("allocation_policy") == "WAround") allocPol = WAround;
      else throw runtime_error(getAttr("alloc_policy") + " Unknown allocation policy");
      CacheInclusionPolicy incl_higher, incl_lower;
      if (getAttrAsUInt64("level") == 1) incl_higher = NINE;
      else {
        if (getAttr("inclusion_higher") == "Inclusive") incl_higher = Inclusive;
        if (getAttr("inclusion_higher") == "Exclusive") incl_higher = Exclusive;
        if (getAttr("inclusion_higher") == "NINE"     ) incl_higher = NINE;
      }
      if (getAttrAsUInt64("level") == 3) incl_lower = NINE;
      else {
        if (getAttr("inclusion_lower") == "Inclusive") incl_lower = Inclusive;
        if (getAttr("inclusion_lower") == "Exclusive") incl_lower = Exclusive;
        if (getAttr("inclusion_lower") == "NINE"     ) incl_lower = NINE;
      }
      mModulePtr = new Cache<uint64_t,uint64_t>
                            (sc_module_name(getName().c_str()),
                             //sc_time(getAttrAsUInt64("latency"), SC_PS),
                             sc_time(getAttrAsUInt64("latency"), SC_NS),
                             getAttrAsUInt64("size"),
                             getAttrAsUInt64("line_size"),
                             getAttrAsUInt64("associativity"),
                             repl,
                             writePol,
                             allocPol,
                             false,
                             getAttrAsUInt64("id"),
                             getAttrAsUInt64("level"),
                             getnIn(),
                             getnOut(),
                             incl_higher,
                             incl_lower,
                             getAttrAsUInt64("is_home"),
                             getAttrAsUInt64("is_coherent"));
      mModulePtr->setCpuId(getAttrAsUInt64("cpu"));
      setId(getAttrAsUInt64("id"));
      if (getAttrAsUInt64("local")) {
        mModulePtr->setIsPriv(true);
		//IOAccessCosim::RegStat(getAttrAsUInt64("cpu"), IOACCESS_READ,   &mModulePtr->WriteBacks);
		//IOAccessCosim::RegStat(getAttrAsUInt64("cpu"), IOACCESS_WRITE,  &mModulePtr->WriteBacks);
        if (getAttrAsUInt64("level")==1) {
          MainMemCosim::RegStat(getAttrAsUInt64("cpu"), L1_WB,   &mModulePtr->WriteBacks);
          MainMemCosim::RegStat(getAttrAsUInt64("cpu"), L1_MISS, &mModulePtr->MissCount);
          MainMemCosim::RegStat(getAttrAsUInt64("cpu"), L1_LD,   &mModulePtr->NReads);
          MainMemCosim::RegStat(getAttrAsUInt64("cpu"), L1_ST,   &mModulePtr->NWrites);
        } else if (getAttrAsUInt64("level")==2) {
          MainMemCosim::RegStat(getAttrAsUInt64("cpu"), L2_WB,   &mModulePtr->WriteBacks);
          MainMemCosim::RegStat(getAttrAsUInt64("cpu"), L2_MISS, &mModulePtr->MissCount);
          MainMemCosim::RegStat(getAttrAsUInt64("cpu"), L2_LD,   &mModulePtr->NReads);
          MainMemCosim::RegStat(getAttrAsUInt64("cpu"), L2_ST,   &mModulePtr->NWrites);
        }
      } else mModulePtr->setIsPriv(false);

      addInPort("in_data");
      if (getAttrAsUInt64("level")==2 && getAttrAsUInt64("l1i_simulate")) addInPort("in_instruction");
      if (getAttrAsUInt64("level")<getAttrAsUInt64("levels_number")) addInPort("in_invalidate");
      addOutPort("out_data");
      if (getAttrAsUInt64("level")>1 && !getAttrAsUInt64("is_home")) addOutPort("out_invalidate");
    }
    virtual uint64_t getBaseAddress() override {
      return getAttrAsUInt64("home_base_address");
    }
    virtual uint64_t getSize() override {
      return getAttrAsUInt64("home_size");
    }
    virtual bool isMemoryMapped() override {
      return getAttrAsUInt64("is_home");
    }
    virtual void addDmiAddress(std::string targetIpName, uint64_t baseAddr, uint64_t size, unsigned char* pointer, bool cached, bool has_dmi) override {
      if (mModulePtr == nullptr) throw runtime_error(getName() + " calling addDmiAddress() before make() !!!");
      if (!cached) mModulePtr->add_uncached_region ( baseAddr, size );
      if (has_dmi) mModulePtr->setDmiRange(0, baseAddr, size, pointer);
    }

    void pushStats() override {
      if(mSegmentedStats.empty())
        mSegmentedStats.push_back({
            {"misses"             , "0"},
            {"hits"               , "0"},
            //{"uncached_forwards"  , "0"},
            {"reads"              , "0"},
            {"writes"             , "0"},
            {"write_backs"        , "0"},
            {"real_invalidations" , "0"},
            {"total_invalidations", "0"},
            {"back_invalidations" , "0"},
            {"evictions"          , "0"},
            {"evict_backs"        , "0"},
            {"PutS"               , "0"},
            {"PutM"               , "0"},
            {"PutI"               , "0"},
            {"GetS"               , "0"},
            {"GetM"               , "0"},
            {"FwdGetS"            , "0"},
            {"FwdGetM"            , "0"}
          });
      const auto& back = mSegmentedStats.back();
      auto misses           = to_string(mModulePtr->getMisses() - stoull(back.at("misses")));
      auto hits             = to_string(mModulePtr->getHits() - stoull(back.at("hits")));
      //auto uncachedForwards = to_string(mModulePtr->getForwards() - stoull(back.at("uncached_forwards")));
      auto reads            = to_string(mModulePtr->getReads() - stoull(back.at("reads")));
      auto writes           = to_string(mModulePtr->getWrites() - stoull(back.at("writes")));
      auto writeBacks       = to_string(mModulePtr->getWriteBacks() - stoull(back.at("write_backs")));
      auto invals           = to_string(mModulePtr->getInvals() - stoull(back.at("real_invalidations")));
      auto totalInvals      = to_string(mModulePtr->getTotalInvals() - stoull(back.at("total_invalidations")));
      auto backInvals       = to_string(mModulePtr->getBackInvals() - stoull(back.at("back_invalidations")));
      auto evictions        = to_string(mModulePtr->getEvictions() - stoull(back.at("evictions")));
      auto evictBacks       = to_string(mModulePtr->getEvictBacks() - stoull(back.at("evict_backs")));
      auto PutS    = to_string(mModulePtr->getPutS() - stoull(back.at("PutS")));
      auto PutM    = to_string(mModulePtr->getPutM() - stoull(back.at("PutM")));
      auto PutI    = to_string(mModulePtr->getPutI() - stoull(back.at("PutI")));
      auto GetS    = to_string(mModulePtr->getGetS() - stoull(back.at("GetS")));
      auto GetM    = to_string(mModulePtr->getGetM() - stoull(back.at("GetM")));
      auto FwdGetS = to_string(mModulePtr->getFwdGetS() - stoull(back.at("FwdGetS")));
      auto FwdGetM = to_string(mModulePtr->getFwdGetM() - stoull(back.at("FwdGetM")));

      mSegmentedStats.push_back({
          {"misses", misses},
          {"hits", hits},
          //{"uncached_forwards"  , uncachedForwards},
          {"reads"              , reads},
          {"writes"             , writes},
          {"write_backs"        , writeBacks},
          {"real_invalidations" , invals},
          {"total_invalidations", totalInvals},
          {"back_invalidations" , backInvals},
          {"evictions"          , evictions},
          {"evict_backs"        , evictBacks},
          {"PutS"               , PutS},
          {"PutM"               , PutM},
          {"PutI"               , PutI},
          {"GetS"               , GetS},
          {"GetM"               , GetM},
          {"FwdGetS"            , FwdGetS},
          {"FwdGetM"            , FwdGetM}
        });
    }

    virtual void setStatsAndDie() override {
      if (mModulePtr) {
        mStats["misses"]              = tostr(mModulePtr->getMisses());
        mStats["hits"]                = tostr(mModulePtr->getHits());
        //mStats["uncached_forwards"]   = tostr(mModulePtr->getForwards());
        mStats["reads"]               = tostr(mModulePtr->getReads());
        mStats["writes"]              = tostr(mModulePtr->getWrites());
        mStats["write_backs"]         = tostr(mModulePtr->getWriteBacks());
        //if (mModulePtr->InclusionOfLower==Inclusive) {
        mStats["real_invalidations"]  = tostr(mModulePtr->getInvals());
        mStats["total_invalidations"] = tostr(mModulePtr->getTotalInvals());
        //}
        //if (mModulePtr->InclusionOfHigher==Inclusive)
        mStats["back_invalidations"]  = tostr(mModulePtr->getBackInvals());
        //if (mModulePtr->InclusionOfLower==Exclusive)
        mStats["evictions"]   = tostr(mModulePtr->getEvictions());
        mStats["evict_backs"] = tostr(mModulePtr->getEvictBacks());
        mStats["PutS"]    = tostr(mModulePtr->getPutS());
        mStats["PutM"]    = tostr(mModulePtr->getPutM());
        mStats["PutI"]    = tostr(mModulePtr->getPutI());
        mStats["GetS"]    = tostr(mModulePtr->getGetS());
        mStats["GetM"]    = tostr(mModulePtr->getGetM());
        mStats["FwdGetS"] = tostr(mModulePtr->getFwdGetS());
        mStats["FwdGetM"] = tostr(mModulePtr->getFwdGetM());
        delete mModulePtr;
      }
    }

   virtual void configure() override {
   	 mModulePtr->configure();
    }

  private:
    friend struct DynamicCacheIdController;
    Cache<uint64_t,uint64_t>* mModulePtr;
  };

 struct DynamicCoherenceInterconnect : public VpsimIp<InPortType, OutPortType> {
   public:
     DynamicCoherenceInterconnect(std::string name):
       VpsimIp(name),
       mConnectionCounter_cache(0),
       mConnectionCounter_home(0),
       mConnectionCounter_mmapped(0),
       mModulePtr(nullptr)
     {
       registerRequiredAttribute("latency");
       registerRequiredAttribute("n_cache_in");
       registerRequiredAttribute("n_cache_out");
       registerRequiredAttribute("n_home_in");
       registerRequiredAttribute("n_home_out");
       registerRequiredAttribute("n_mmapped");
	   registerRequiredAttribute("n_device");
	   registerRequiredAttribute("flitSize");
	   registerRequiredAttribute("memory_word_length"),
       registerRequiredAttribute("is_coherent");
	   registerRequiredAttribute("interleave_length");
       registerOptionalAttribute("latency_enable", "1");
       registerRequiredAttribute("is_mesh");
       registerRequiredAttribute("mesh_x");
       registerRequiredAttribute("mesh_y");
       registerRequiredAttribute("with_contention");
       registerRequiredAttribute("contention_interval");
       registerRequiredAttribute("buffer_size");
       registerRequiredAttribute("virtual_channels");
       registerRequiredAttribute("router_latency");
       registerRequiredAttribute("link_latency");
       registerRequiredAttribute("noc_stats_per_initiator_on");
     }
     void pushStats() override {
       	   static uint64_t ns_per_sec=1000000000;

	   string distanceKey = string("Total_Distance");
	   string latencyKey  = string("Total_Latency");
	   string packetsKey  = string("Packets");
           string totallatencyKey  = string("Total_Latency");
	   string averagelatencyKey = string("Average_Latency");
	   //string readsKey, writesKey;
	   /*for (size_t i = 0; i<mModulePtr->getMMappedCount(); ++i) {
		 readsKey   = string("memory_reads[")+tostr(i)+string("]");
		 writesKey  = string("memory_writes[")+tostr(i)+string("]");
	   }*/
	   // Initialize mSegmentedStats
	   if(mSegmentedStats.empty()) {
		 mSegmentedStats.push_back({});
		 auto &back = mSegmentedStats.back();
		 /*for (size_t i = 0; i<mModulePtr->getMMappedCount(); ++i) {
                   //printf("MMappedCount=%d\n", mModulePtr->getMMappedCount());
                   readsKey   = string("memory_reads[")+tostr(i)+string("]");
        	   writesKey  = string("memory_writes[")+tostr(i)+string("]");

               back[readsKey]  = "0";
		   back[writesKey] = "0";
           }*/

		 back[distanceKey] = "0";
		 back[latencyKey]  = "0";
		 back[packetsKey] = "0";
		 if(getAttrAsUInt64("with_contention")){
                   back[totallatencyKey]  = "0";
			 for (size_t j = 0; j < getAttrAsUInt64("mesh_y"); j++)
			{
				for (size_t i = 0; i < getAttrAsUInt64("mesh_x"); i++)
				{
					back[string("Router(")+tostr(i)+string(",")+tostr(j)+string(")_")+string("Packets")] = "0";
					back[string("Router(")+tostr(i)+string(",")+tostr(j)+string(")_")+string("Contention")] = "0";
				}
			}
		 }


                //NoC stats per initiator
         if (getAttrAsUInt64("noc_stats_per_initiator_on")){
                 for (const auto& itr:mModulePtr->initTotalStats) {
                   back[string("Initiator_")+ tostr(get<0>(itr))+string("(Packets_Sent)")] = "0";
                   back[string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Distance)")] = "0";
                   back[string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Network_Latency)")] = "0";
                 }
         }
		for (size_t i = 0; i < mModulePtr->getMMappedSize(); ++i)
		{
			std::pair<size_t,size_t> pos = mModulePtr->getMMappedPos(i);
			back[string("Memory(")+tostr(pos.first)+string(",")+tostr(pos.second)+string(")_")+string("Reads")] = "0";
			back[string("Memory(")+tostr(pos.first)+string(",")+tostr(pos.second)+string(")_")+string("Writes")] = "0";
		}
	   }

	   // Update mSegmentedStats
	   //const auto& back = mSegmentedStats.back();
	   auto& back = mSegmentedStats.back();
	   decltype(mSegmentedStats)::value_type newMap;
	   /*for (size_t i = 0; i<mModulePtr->getMMappedCount(); ++i) {
		 auto reads     = to_string(mModulePtr->getReadMemoryCount(i) - stoull(back.at(readsKey)));
		 auto writes    = to_string(mModulePtr->getWriteMemoryCount(i) - stoull(back.at(writesKey)));
		 auto reads     = to_string(mModulePtr->getReadMemoryCount(i) - stoull(back.at(string("memory_reads[")+tostr(i)+string("]"))));
		 auto writes    = to_string(mModulePtr->getWriteMemoryCount(i) - stoull(back.at(string("memory_writes[")+tostr(i)+string("]"))));
		 //newMap[readsKey]= reads;
		 //newMap[writesKey]= writes;
		 newMap[string("memory_reads[")+tostr(i)+string("]")]= reads;
		 newMap[string("memory_writes[")+tostr(i)+string("]")]= writes;
         }*/


	   auto totalDistance = to_string(mModulePtr->getTotalDistance() - stoull(back.at(distanceKey)));
	   auto packetsCount  = to_string(mModulePtr->getPacketsCount() - stoull(back.at(packetsKey)));
	   newMap[distanceKey] = totalDistance;
	   newMap[packetsKey]  = packetsCount;
           if(!getAttrAsUInt64("with_contention")){
	     auto totalLatency  = to_string((mModulePtr->getTotalLatency()).to_seconds()*ns_per_sec - stoull(back.at(latencyKey)));
	     newMap[latencyKey]  = totalLatency;
           }
	   if(getAttrAsUInt64("with_contention")){
             auto TotalLatencyContention  = to_string((mModulePtr->getTotalLatencyWithContention()).to_seconds()*ns_per_sec - stoull(back.at(totallatencyKey)));
	     auto AverageLatency = to_string((double)(stoull(TotalLatencyContention)/(double)(stoull(packetsCount))));
             //auto TotalLatencyContention  = to_string((mModulePtr->getTotalLatencyWithContention()).to_seconds()*ns_per_sec);
	     //printf("TotalLatencyContention=%d\n", (mModulePtr->getTotalLatencyWithContention()).to_seconds()*ns_per_sec -stoull(back.at(totallatencyKey)));
             newMap[totallatencyKey]  = TotalLatencyContention;
             newMap[averagelatencyKey]  = AverageLatency;
			 //NoC stats per Router
			for (size_t j = 0; j < getAttrAsUInt64("mesh_y"); j++)
			{
				for (size_t i = 0; i < getAttrAsUInt64("mesh_x"); i++)
				{
					newMap[string("Router(")+tostr(i)+string(",")+tostr(j)+string(")_")+string("Packets")] = tostr(mModulePtr->getRouterPacketsCount(i,j) - stoull(back.at(string("Router(")+tostr(i)+string(",")+tostr(j)+string(")_")+string("Packets"))));
					newMap[string("Router(")+tostr(i)+string(",")+tostr(j)+string(")_")+string("Contention")] = tostr((mModulePtr->getRouterTotalLatency(i,j)).to_seconds()*ns_per_sec - stod(back.at(string("Router(")+tostr(i)+string(",")+tostr(j)+string(")_")+string("Contention"))));
				}
			}
	   }
		for (size_t i = 0; i < mModulePtr->getMMappedSize(); ++i)
		{
			std::pair<size_t,size_t> pos = mModulePtr->getMMappedPos(i);
			newMap[string("Memory(")+tostr(pos.first)+string(",")+tostr(pos.second)+string(")_")+string("Reads")] = tostr(mModulePtr->getReadCount(i) - stoull(back.at(string("Memory(")+tostr(pos.first)+string(",")+tostr(pos.second)+string(")_")+string("Reads"))));
			newMap[string("Memory(")+tostr(pos.first)+string(",")+tostr(pos.second)+string(")_")+string("Writes")] = tostr(mModulePtr->getWriteCount(i) - stoull(back.at(string("Memory(")+tostr(pos.first)+string(",")+tostr(pos.second)+string(")_")+string("Writes"))));
		}
           //NoC stats per initiator
       if (getAttrAsUInt64("noc_stats_per_initiator_on")){
           double avg_latency=0.0;
           for (const auto& itr:mModulePtr->initTotalStats) {
             //printf("initiator_id %d\n", get<0>(itr));
             newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Mesh_Position)")] = get<0>(get<1>(itr));
             //newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Mesh_Position)")] = tostr(get<0>(get<1>(itr)));
             //newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Packets_Sent)")] = to_string(get<1>(get<1>(itr))-stoull(back.at(string("Initiator_")+ tostr(get<0>(itr))+string("(Packets_Sent)"))));
             if(back[(string("Initiator_")+ tostr(get<0>(itr))+string("(Packets_Sent)"))]=="")
               newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Packets_Sent)")] = to_string(get<1>(get<1>(itr)));
             else
               newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Packets_Sent)")] = to_string(get<1>(get<1>(itr))-stoull(back[(string("Initiator_")+ tostr(get<0>(itr))+string("(Packets_Sent)"))]));
             //printf("nbr pckts sent is %d\n", stoull(newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Packets_Sent)")]));
             //newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Distance)")] = to_string(get<2>(get<1>(itr))-stoull(back.at(string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Distance)"))));
             if(back[(string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Distance)"))]=="")
               newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Distance)")] = to_string(get<2>(get<1>(itr)));
             else
               newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Distance)")] = to_string(get<2>(get<1>(itr))-stoull(back[(string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Distance)"))]));
             //newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Network_Latency)")] = to_string((get<3>(get<1>(itr)).to_seconds())*ns_per_sec -stoull(back.at(string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Network_Latency)"))));
             if(back[(string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Network_Latency)"))]=="")
               newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Network_Latency)")] = to_string((get<3>(get<1>(itr)).to_seconds())*ns_per_sec);
             else
               newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Network_Latency)")] = to_string((get<3>(get<1>(itr)).to_seconds())*ns_per_sec -stoull(back[(string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Network_Latency)"))]));
             //printf("total latency is %d\n", stoull(newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Network_Latency)")]));
             if(stoull(newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Packets_Sent)")])!=0){
               avg_latency=(double)stoull(newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Network_Latency)")])/(double)stoull(newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Packets_Sent)")]);
               newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Avg_Packet_Latency)")] =to_string(avg_latency);
             }
             else
               newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Avg_Packet_Latency)")] ="0";

           //printf("average latency is %f\n", stoull(newMap[string("Initiator_")+ tostr(get<0>(itr))+string("(Avg_Packet_Latency)")]));
           //printf("average latency is %f\n", avg_latency);

           }
       }

	   mSegmentedStats.push_back(move(newMap));

	 }
     virtual void setStatsAndDie() override {
       uint64_t ns_per_sec=1000000000;
	   if (mModulePtr) {
             if (getAttrAsUInt64("is_mesh")) {

		 /*for (unsigned i = 0; i<mModulePtr->getMMappedCount(); i++) {
		   mStats[string("read_bytes[") + tostr(i) + "]"]    = tostr(mModulePtr->getReadMemoryCount(i));
		   mStats[string("written_bytes[") + tostr(i) + "]"] = tostr(mModulePtr->getWriteMemoryCount(i));
           }*/
		 mStats[string("Total_Distance")] = tostr(mModulePtr->getTotalDistance());
		 mStats[string("Packets")]       = tostr(mModulePtr->getPacketsCount());
                 if(getAttrAsUInt64("with_contention")){
		   double totlat=(mModulePtr->getTotalLatencyWithContention()).to_seconds()*ns_per_sec;
                   mStats[string("Total_Latency")]  = tostr(totlat) + " ns";
                   double avgLat= totlat/(mModulePtr->getPacketsCount());
                   //mStats[string("Total_Latency")]  = tostr((mModulePtr->getTotalLatencyWithContention()).to_seconds()*ns_per_sec) + " ns";
                   //double avgLat= ((mModulePtr->getTotalLatencyWithContention().to_seconds())*ns_per_sec)/(mModulePtr->getPacketsCount());
                   mStats[string("Average_Latency")]  = tostr(avgLat) + " ns";
					//NoC stats per Router
					for (size_t j = 0; j < getAttrAsUInt64("mesh_y"); j++)
					{
						for (size_t i = 0; i < getAttrAsUInt64("mesh_x"); i++)
						{
							mStats[string("Router(")+tostr(i)+string(",")+tostr(j)+string(")_")+string("Packets")] = tostr(mModulePtr->getRouterPacketsCount(i,j));
							mStats[string("Router(")+tostr(i)+string(",")+tostr(j)+string(")_")+string("Contention")] = tostr((mModulePtr->getRouterTotalLatency(i,j)).to_seconds()*ns_per_sec) + " ns";
						}
					}
                 }
	         else
		   mStats[string("Total_Latency")]  = tostr((mModulePtr->getTotalLatency()).to_seconds()*ns_per_sec) + " ns";
			for (size_t i = 0; i < mModulePtr->getMMappedSize(); ++i)
			{
				std::pair<size_t,size_t> pos = mModulePtr->getMMappedPos(i);
				mStats[string("Memory(")+tostr(pos.first)+string(",")+tostr(pos.second)+string(")_")+string("Reads")] = tostr(mModulePtr->getReadCount(i));
				mStats[string("Memory(")+tostr(pos.first)+string(",")+tostr(pos.second)+string(")_")+string("Writes")] = tostr(mModulePtr->getWriteCount(i));
			}
           //NoC stats per initiator
         if (getAttrAsUInt64("noc_stats_per_initiator_on")){
           for (const auto& itr:mModulePtr->initTotalStats) {
             //mStats[string("Initiator_")+ tostr(get<0>(itr))+string("(Mesh_Position)")] = get<0>(get<1>(itr));
             //mStats[string("Initiator_")+ tostr(get<0>(itr))+string("(Mesh_Position)")] = tostr(get<0>(get<1>(itr)));
             mStats[string("Initiator_")+ tostr(get<0>(itr))+string("(Packets_Sent)")] = tostr(get<1>(get<1>(itr)));
             mStats[string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Distance)")] = tostr(get<2>(get<1>(itr))) + " hops";
             mStats[string("Initiator_")+ tostr(get<0>(itr))+string("(Total_Network_Latency)")] = tostr((get<3>(get<1>(itr)).to_seconds())*ns_per_sec ) + " ns";
             sc_time avg_lat=SC_ZERO_TIME;
             if((get<1>(get<1>(itr)))!=0)
               avg_lat = ((get<3>(get<1>(itr))))/((get<1>(get<1>(itr))));
             mStats[string("Initiator_")+ tostr(get<0>(itr))+string("(Avg_Packet_Latency)")] = tostr(avg_lat.to_seconds()*ns_per_sec) + " ns";
           }

                 (mModulePtr->initTotalStats).clear();
         }

	}
		 delete mModulePtr;

	   }
	 }
     virtual unsigned getMaxInPortCount() override {
       return getAttrAsUInt64("n_cache_in")+getAttrAsUInt64("n_home_in")+getAttrAsUInt64("n_device");
     }
     virtual unsigned getMaxOutPortCount() override {
       return getAttrAsUInt64("n_cache_out")+getAttrAsUInt64("n_home_out")+getAttrAsUInt64("n_mmapped");
     }
     virtual InPortType *getNextInPort() override {
       throw runtime_error(VpsimIp::getName() + " : Does not support dynamic port allocation.");
     }
     virtual OutPortType *getNextOutPort() override {
       throw runtime_error(VpsimIp::getName() + " : Does not support dynamic port allocation.");
     }

     virtual sc_module* getScModule() override { return mModulePtr; }
     virtual void make() override {
       if (mModulePtr != nullptr) throw runtime_error(getName() + " make() already called !");
       checkAttributes();
       mModulePtr = new CoherenceInterconnect
                    (sc_module_name(getName().c_str()),
                     getAttrAsUInt64("n_cache_in"),
                     getAttrAsUInt64("n_cache_out"),
                     getAttrAsUInt64("n_home_in"),
                     getAttrAsUInt64("n_home_out"),
                     getAttrAsUInt64("n_mmapped"),
					 getAttrAsUInt64("n_device"),
					 getAttrAsUInt64("flitSize"),
					 getAttrAsUInt64("memory_word_length"),
                     getAttrAsUInt64("is_coherent"),
					 getAttrAsUInt64("interleave_length"));
       if (!getAttrAsUInt64("is_mesh")) {
         mModulePtr->set_is_mesh(false);
         //mModulePtr->set_latency(sc_time(getAttrAsUInt64("latency"), SC_PS));
         mModulePtr->set_latency(sc_time(getAttrAsUInt64("latency"), SC_NS));
         mModulePtr->set_enable_latency(true);
       } else {
         mModulePtr->set_is_mesh(true);
         if (!getAttrAsUInt64("noc_stats_per_initiator_on"))
           mModulePtr->set_noc_stats_per_initiator(false);
         else
           mModulePtr->set_noc_stats_per_initiator(true);
         mModulePtr->set_mesh_coord(getAttrAsUInt64("mesh_x"),getAttrAsUInt64("mesh_y"));
         //mModulePtr->set_router_latency(getAttrAsUInt64("router_latency"));
         mModulePtr->set_router_latency(stod(getAttr("router_latency")));
         mModulePtr->set_enable_latency(false);
         if(getAttrAsUInt64("with_contention")){
           mModulePtr->set_contention(true);
           //mModulePtr->set_contention_interval(getAttrAsUInt64("contention_interval"));
           mModulePtr->set_contention_interval(stod(getAttr("contention_interval")));
           mModulePtr->set_virtual_channels(getAttrAsUInt64("virtual_channels"));
           mModulePtr->set_buffer_size(getAttrAsUInt64("buffer_size"));
           //mModulePtr->set_link_latency(getAttrAsUInt64("link_latency"));
           mModulePtr->set_link_latency(stod(getAttr("link_latency")));
         } else {
           mModulePtr->set_contention(false);
         }

       }
       /* mModulePtr-> set_latency(sc_time(getAttrAsUInt64("latency"), SC_PS));
        if (getAttrAsUInt64("latency_enable")) mModulePtr->set_enable_latency(true);
        else mModulePtr-> set_enable_latency(false); */
        for (uint32_t i = 0; i < getAttrAsUInt64("n_cache_in"); i++) {
          addInPort(string("cache_in_")+to_string(i), mModulePtr->mCacheSocketsIn[i]);
        }
        for (uint32_t i = 0; i < getAttrAsUInt64("n_cache_out"); i++) {
          addOutPort(string("cache_out_")+to_string(i), mModulePtr->mCacheSocketsOut[i]);
          mModulePtr-> set_cache_pos (string("cache_out_")+to_string(i), i);
        }
        for (uint32_t i = 0; i < getAttrAsUInt64("n_home_in"); i++) {
          addInPort(string("home_in_") +to_string(i), mModulePtr->mHomeSocketsIn [i]);
        }
         for (uint32_t i = 0; i < getAttrAsUInt64("n_home_out"); i++) {
          addOutPort(string("home_out_")+to_string(i), mModulePtr->mHomeSocketsOut[i]);
        }
        for (uint32_t i = 0; i < getAttrAsUInt64("n_mmapped"); i++){
          addOutPort(string("mmapped_out_")+to_string(i), mModulePtr->mMMappedSocketsOut[i]);
        }
		for (uint32_t i = 0; i < getAttrAsUInt64("n_device"); i++){
          addInPort(string("device_")+to_string(i), mModulePtr->mDeviceSocketsIn[i]);
        }
     }

     virtual void connect(std::string outPortAlias, VpsimIp<InPortType,OutPortType>* otherIp, std::string inPortAlias) override {
       if (otherIp->isMemoryMapped()&&otherIp->isIdMapped())
         mModulePtr->
           set_home_output (mConnectionCounter_home++, otherIp->getId(), otherIp->getBaseAddress(), otherIp->getSize());
       else if (otherIp->isIdMapped())
         mModulePtr-> set_cache_id (mConnectionCounter_cache++, otherIp->getId(), outPortAlias);
       else if (otherIp->isMemoryMapped())
         mModulePtr-> set_mmapped_output (mConnectionCounter_mmapped++, otherIp->getBaseAddress(), otherIp->getSize());
       else
         throw runtime_error ("Component is not id-mapped nor home nor memory-mmaped\n");
       VpsimIp<InPortType,OutPortType>::connect(outPortAlias, otherIp, inPortAlias);
       //cout << "connect " << outPortAlias << " to " << inPortAlias << endl;
      }

   private:
     uint32_t mConnectionCounter_cache;
     uint32_t mConnectionCounter_home;
     uint32_t mConnectionCounter_mmapped;

     friend struct DynamicNoCMemoryController;
     friend struct DynamicCacheController;
     friend struct DynamicCacheIdController;
     friend struct DynamicCpuController;
	 friend struct DynamicNoCSource;
	 friend struct DynamicNoCHomeNode;
	 //friend struct DynamicMemory;
	 friend struct DynamicNoCDeviceController;
     CoherenceInterconnect* mModulePtr;
   };

struct DynamicNoCDeviceController
    : public  VpsimIp<InPortType, OutPortType>
{
	public:
    DynamicNoCDeviceController(std::string name):
       VpsimIp(name) {
       registerRequiredAttribute("id_dev");
       registerRequiredAttribute("x_id");
       registerRequiredAttribute("y_id");
	   registerRequiredAttribute("noc");
    }
    N_IN_PORTS_OVERRIDE(0);
    N_OUT_PORTS_OVERRIDE(0);
	virtual InPortType* getNextInPort() override {
    	throw runtime_error(getName() + " : IOAccess Device has no in sockets.");
    }
    virtual OutPortType* getNextOutPort() override {
    	throw runtime_error(getName() + " : IOAccess Device has no out sockets.");
    }
    virtual void make() override {
    	checkAttributes();
    }
    virtual void finalize() override {
		VpsimIp* ip = VpsimIp::Find(getAttr("noc"));
       	DynamicCoherenceInterconnect* noc=dynamic_cast<DynamicCoherenceInterconnect*>(ip);
       	noc->mModulePtr->register_device_ctrl(	getAttrAsUInt64("id_dev"),
			   									getAttrAsUInt64("x_id"),
                                         		getAttrAsUInt64("y_id"));
    }
};

 struct DynamicNoCMemoryController
     : public  VpsimIp<InPortType, OutPortType>
   {

   public:
     DynamicNoCMemoryController(std::string name):
       VpsimIp(name) {
       registerRequiredAttribute("size");
       registerRequiredAttribute("base_address");
       //registerRequiredAttribute("cpu_affinity");
       registerRequiredAttribute("noc");
       registerRequiredAttribute("x_id");
       registerRequiredAttribute("y_id");
     }
     N_IN_PORTS_OVERRIDE(0);
     N_OUT_PORTS_OVERRIDE(0);
     virtual InPortType* getNextInPort() override {
       throw runtime_error(getName() + " : MemoryView has no in sockets.");
     }
     virtual OutPortType* getNextOutPort() override {
       throw runtime_error(getName() + " : Memory has no out sockets.");
     }
     virtual void make() override {
       checkAttributes();
     }
     virtual void finalize() override {
       VpsimIp* ip = VpsimIp::Find(getAttr("noc"));
       //DynamicInterconnect* noc=dynamic_cast<DynamicInterconnect*>(ip);
       DynamicCoherenceInterconnect* noc=dynamic_cast<DynamicCoherenceInterconnect*>(ip);
	   noc->mModulePtr->set_first_memory_controller();//call first and then register in order to capture the right index in the vector
       noc->mModulePtr->register_mem_ctrl(getAttrAsUInt64("base_address"),
                                          getAttrAsUInt64("size"),
                                          getAttrAsUInt64("x_id"),
                                          getAttrAsUInt64("y_id"));
                                          //getAttrAsUInt64("cpu_affinity"));
       //cout << "memory base address= " << getAttrAsUInt64("base_address") << " memory mesh coordinates: x= " << getAttrAsUInt64("x_id") << " y= " << getAttrAsUInt64("y_id") << endl;
   		if(noc->mModulePtr->get_ram_base_addr() > getAttrAsUInt64("base_address")) noc->mModulePtr->set_ram_base_addr(getAttrAsUInt64("base_address"));
		if(noc->mModulePtr->get_ram_last_addr() < getAttrAsUInt64("base_address") + getAttrAsUInt64("size")) noc->mModulePtr->set_ram_last_addr(getAttrAsUInt64("base_address") + getAttrAsUInt64("size"));
     }

   };

   struct DynamicCacheController
     : public  VpsimIp<InPortType, OutPortType>
   {

   public:
     DynamicCacheController(std::string name):
       VpsimIp(name) {
       registerRequiredAttribute("size");
       registerRequiredAttribute("base_address");
       //registerRequiredAttribute("cpu_affinity");
       registerRequiredAttribute("noc");
       registerRequiredAttribute("x_id");
       registerRequiredAttribute("y_id");
     }
     N_IN_PORTS_OVERRIDE(0);
     N_OUT_PORTS_OVERRIDE(0);
     virtual InPortType* getNextInPort() override {
       throw runtime_error(getName() + " : Cache Controller has no in sockets.");
     }
     virtual OutPortType* getNextOutPort() override {
       throw runtime_error(getName() + " : Cache Controller has no out sockets.");
     }
     virtual void make() override {
       checkAttributes();
     }
     virtual void finalize() override {
       VpsimIp* ip = VpsimIp::Find(getAttr("noc"));
       DynamicCoherenceInterconnect* noc=dynamic_cast<DynamicCoherenceInterconnect*>(ip);
       noc->mModulePtr->register_home_ctrl(getAttrAsUInt64("base_address"),
                                          getAttrAsUInt64("size"),
                                          getAttrAsUInt64("x_id"),
                                          getAttrAsUInt64("y_id"));
       //cout << "cache base address= " << getAttrAsUInt64("base_address") << " cache mesh coordinates: x= " << getAttrAsUInt64("x_id") << " y= " << getAttrAsUInt64("y_id") << endl;
     }
   };

   struct DynamicCacheIdController
     : public  VpsimIp<InPortType, OutPortType>
   {
   public:
     DynamicCacheIdController(std::string name):
       VpsimIp(name) {
       registerRequiredAttribute("noc");
       registerRequiredAttribute("cache");
       registerRequiredAttribute("x_id");
       registerRequiredAttribute("y_id");
     }
     N_IN_PORTS_OVERRIDE(0);
     N_OUT_PORTS_OVERRIDE(0);
     virtual InPortType* getNextInPort() override {
       throw runtime_error(getName() + " : Cache Controller has no in sockets.");
     }
     virtual OutPortType* getNextOutPort() override {
       throw runtime_error(getName() + " : Cache Controller has no out sockets.");
     }
     virtual void make() override {
       checkAttributes();
     }
     virtual void finalize() override {
       VpsimIp* ip = VpsimIp::Find(getAttr("noc"));
       DynamicCoherenceInterconnect* noc=dynamic_cast<DynamicCoherenceInterconnect*>(ip);
       VpsimIp* ip1 = VpsimIp::Find(getAttr("cache"));
       DynamicCache* cache=dynamic_cast<DynamicCache*>(ip1);
       noc->mModulePtr->register_cpu_ctrl(cache->getId(),
                                          getAttrAsUInt64("x_id"),
                                          getAttrAsUInt64("y_id"));
       //cout << "cacheId= " << cache->getId() << " cache mesh coordinates: x= " << getAttrAsUInt64("x_id") << " y= " << getAttrAsUInt64("y_id") << endl;
     }
   };

   struct DynamicCpuController
       : public  VpsimIp<InPortType, OutPortType>
     {
     public:
       DynamicCpuController(std::string name):
         VpsimIp(name) {
         registerRequiredAttribute("id");
         registerRequiredAttribute("noc");
         registerRequiredAttribute("x_id");
         registerRequiredAttribute("y_id");
       }
       N_IN_PORTS_OVERRIDE(0);
       N_OUT_PORTS_OVERRIDE(0);
       virtual InPortType* getNextInPort() override {
         throw runtime_error(getName() + " : Cpu Controller has no in sockets.");
       }
       virtual OutPortType* getNextOutPort() override {
         throw runtime_error(getName() + " : Cpu Controller has no out sockets.");
       }
       virtual void make() override {
         checkAttributes();
       }
       virtual void finalize() override {
         VpsimIp* ip = VpsimIp::Find(getAttr("noc"));
         DynamicCoherenceInterconnect* noc=dynamic_cast<DynamicCoherenceInterconnect*>(ip);
         noc->mModulePtr->register_cpu_ctrl(getAttrAsUInt64("id"),
                                            getAttrAsUInt64("x_id"),
                                            getAttrAsUInt64("y_id"));
         //cout << "cpuId= " << getAttrAsUInt64("id") << " cpu mesh coordinates: x= " << getAttrAsUInt64("x_id") << " y= " << getAttrAsUInt64("y_id") << endl;
       }
     };


 struct DynamicInterconnect
 	: public VpsimIp<InPortType, OutPortType>
 {

 public:
     DynamicInterconnect(std::string name):
    	VpsimIp(name),
 	    mConnectionCounter(0),
 	    mModulePtr(nullptr) {

 		registerRequiredAttribute("latency");
		registerRequiredAttribute("n_in_ports");
		registerRequiredAttribute("n_out_ports");

 		registerRequiredAttribute("is_mesh");
 		registerRequiredAttribute("mesh_x");
 		registerRequiredAttribute("mesh_y");
 		registerRequiredAttribute("router_latency");
 	}

     void pushStats() override {
         if(mSegmentedStats.empty()) {
             mSegmentedStats.push_back({});
             auto &back = mSegmentedStats.back();

             for (size_t i = 0; i < getMaxOutPortCount(); ++i) {
                 auto readsKey = string("reads") + tostr(i);
                 auto writesKey = string("writes") + tostr(i);
                 back[readsKey] = "0";
                 back[writesKey] = "0";
             }
         }

         const auto& back = mSegmentedStats.back();
         decltype(mSegmentedStats)::value_type newMap;

         for (size_t i = 0; i < getMaxOutPortCount(); ++i) {
             auto readsKey = string("reads") + tostr(i);
             auto writesKey = string("writes") + tostr(i);

             auto reads = to_string(mModulePtr->getReadCount(i) - stoull(back.at(readsKey)));
             auto writes = to_string(mModulePtr->getWriteCount(i) - stoull(back.at(writesKey)));

             newMap[readsKey]= reads;
             newMap[writesKey]= writes;
         }

         mSegmentedStats.push_back(move(newMap));
     }

    virtual void setStatsAndDie() override {
		if (mModulePtr) {
			for (unsigned i = 0; i < getMaxOutPortCount(); i++) {
				mStats[string("written_bytes[") + tostr(i) + "]"] = tostr(mModulePtr->getWriteCount(i));
				mStats[string("read_bytes[") + tostr(i) + "]"] = tostr(mModulePtr->getReadCount(i));
			}
			delete mModulePtr;
		}
	}

    N_IN_PORTS_OVERRIDE(getAttrAsUInt64("n_in_ports"));
    N_OUT_PORTS_OVERRIDE(getAttrAsUInt64("n_out_ports"));

 	virtual InPortType* getNextInPort() override {
 		if (!mModulePtr) {
			throw runtime_error(getName() + "Please call make() before handling ports.");
		}
		return &mModulePtr->socket_in[mInPortCounter];
 	}

 	virtual OutPortType* getNextOutPort() override {
 		if (!mModulePtr) {
 			throw runtime_error(getName() + " Please call make() before handling ports.");
 		}

        return &mModulePtr->socket_out[mOutPortCounter];
    }

 	virtual void make() override {
 		if (mModulePtr != nullptr) {
 			throw runtime_error(getName() + " make() already called !");
 		}
 		checkAttributes();

        const auto nInPorts = getAttrAsUInt64("n_in_ports");
        const auto nOutPorts = getAttrAsUInt64("n_out_ports");
        mModulePtr = new interconnect(sc_module_name(getName().c_str()), nInPorts, nOutPorts);
 		if (!getAttrAsUInt64("is_mesh")) {
 			mModulePtr->set_is_mesh(false);
			//mModulePtr->set_latency(sc_time(getAttrAsUInt64("latency"), SC_PS));
			mModulePtr->set_latency(sc_time(getAttrAsUInt64("latency"), SC_NS));
			mModulePtr->set_enable_latency(true);
 		} else {
 			mModulePtr->set_is_mesh(true);
 			mModulePtr->set_mesh_coord(getAttrAsUInt64("mesh_x"),getAttrAsUInt64("mesh_x"));
 			mModulePtr->set_router_latency(getAttrAsUInt64("router_latency"));
			mModulePtr->set_enable_latency(false);
 		}
 	}

 	virtual void connect(std::string outPortAlias, VpsimIp<InPortType,OutPortType>* otherIp, std::string inPortAlias) override {
 		// set address before connecting (used for forwarding)
 		if (otherIp->isMemoryMapped()) {
 			cout<<"MAP : "<<otherIp->getBaseAddress()<<" - "<<otherIp->getSize()<<endl;
 			mModulePtr->set_socket_out_addr(mConnectionCounter++, otherIp->getBaseAddress(), otherIp->getSize());
 		} else {
 			mModulePtr->setDefaultRoute(mConnectionCounter++);
 		}
		VpsimIp<InPortType,OutPortType>::connect(outPortAlias, otherIp, inPortAlias);
	}

 private:
 	uint32_t mConnectionCounter;
 	interconnect* mModulePtr;

 	friend struct DynamicNoCMemoryController;
 	friend struct DynamicNoCSource;
 	friend struct DynamicNoCHomeNode;
 };

 struct DynamicNoCHomeNode
  	: public  VpsimIp<InPortType, OutPortType>
   {

   public:
	 DynamicNoCHomeNode(std::string name):
  			VpsimIp(name) {

  		registerRequiredAttribute("size");
  		registerRequiredAttribute("base_address");
  		registerRequiredAttribute("noc_id");
  		registerRequiredAttribute("noc");
  	}


  	N_IN_PORTS_OVERRIDE(0);
  	N_OUT_PORTS_OVERRIDE(0);

  	virtual InPortType* getNextInPort() override {
  		throw runtime_error(getName() + " : MemoryView has no in sockets.");
  	}

  	virtual OutPortType* getNextOutPort() override {
  		throw runtime_error(getName() + " : Memory has no out sockets.");
  	}

  	virtual void make() override {
  		checkAttributes();
  	}

  	virtual void finalize() override {
  		VpsimIp* ip = VpsimIp::Find(getAttr("noc"));
  		DynamicInterconnect* noc=dynamic_cast<DynamicInterconnect*>(ip);
  		noc->mModulePtr->register_hn_input(
  				getAttrAsUInt64("base_address"),
  				getAttrAsUInt64("size"),
 				getAttrAsUInt64("noc_id"));
  	}

   };

 struct DynamicNoCSource
  	: public  VpsimIp<InPortType, OutPortType>
   {

   public:
	 DynamicNoCSource(std::string name):
  			VpsimIp(name) {

  		registerRequiredAttribute("src_id");
  		registerRequiredAttribute("noc_id");
  		registerRequiredAttribute("noc");
  	}


  	N_IN_PORTS_OVERRIDE(0);
  	N_OUT_PORTS_OVERRIDE(0);

  	virtual InPortType* getNextInPort() override {
  		throw runtime_error(getName() + " : MemoryView has no in sockets.");
  	}

  	virtual OutPortType* getNextOutPort() override {
  		throw runtime_error(getName() + " : Memory has no out sockets.");
  	}

  	virtual void make() override {
  		checkAttributes();
  	}

  	virtual void finalize() override {
  		VpsimIp* ip = VpsimIp::Find(getAttr("noc"));
  		DynamicInterconnect* noc=dynamic_cast<DynamicInterconnect*>(ip);
  		noc->mModulePtr->register_source(
  				getAttrAsUInt64("src_id"),
 				getAttrAsUInt64("noc_id"));
  	}

   };

struct DynamicMemory
	: public  VpsimIp<InPortType, OutPortType>
 {

 public:
	DynamicMemory(std::string name):
			VpsimIp(name),
			mModulePtr(nullptr) {

		registerRequiredAttribute("size");
		registerRequiredAttribute("base_address");

		registerRequiredAttribute("cycle_duration");
		registerRequiredAttribute("write_cycles");
		registerRequiredAttribute("read_cycles");

		registerRequiredAttribute("channel_width");

		registerRequiredAttribute("load_elf");
		registerRequiredAttribute("elf_file");

		//registerOptionalAttribute("load_blob", "0");
		//registerOptionalAttribute("blob_file", "0");
		//registerOptionalAttribute("blob_offset", "0");

		registerOptionalAttribute("latency_enable", "1");
		registerRequiredAttribute("dmi_enable");

		//registerRequiredAttribute("noc");

	}

    void pushStats() override {
        if(mSegmentedStats.empty()){
            mSegmentedStats.push_back({
                                              {"reads",  "0"},
                                              {"writes", "0"}
                                      });
        }

        const auto& back = mSegmentedStats.back();
        auto reads = to_string(mModulePtr->getReadCount() - stoull(back.at("reads")));
        auto writes = to_string(mModulePtr->getWriteCount() - stoull(back.at("writes")));

        mSegmentedStats.push_back({
                                          {"reads",  reads},
                                          {"writes", writes}
                                  });
    }

	virtual void setStatsAndDie() override {
		if (mModulePtr) {
			mStats["reads"] = tostr(mModulePtr->getReadCount());
			mStats["writes"] = tostr(mModulePtr->getWriteCount());
			delete mModulePtr;
		}
	}

	MEMORY_MAPPED_OVERRIDE;
	CACHED_OVERRIDE;
	virtual bool hasDmi() override { return getAttrAsUInt64("dmi_enable"); }

	N_IN_PORTS_OVERRIDE(1);
	N_OUT_PORTS_OVERRIDE(0);

	virtual InPortType* getNextInPort() override {
		return &mModulePtr->mTargetSocket;
	}

	virtual OutPortType* getNextOutPort() override {
		throw runtime_error(getName() + " : Memory has no out sockets.");
	}

	virtual void make() override {
		if (mModulePtr != nullptr) {
			throw runtime_error(getName() + " make() already called !!");
		}
		checkAttributes();
		mModulePtr = new memory ( getName().c_str(),
				getAttrAsUInt64("size") );

		mModulePtr->setBaseAddress(getAttrAsUInt64("base_address"));
		//mModulePtr->setCycleDuration(sc_time(getAttrAsUInt64("cycle_duration"), SC_PS));
		mModulePtr->setCycleDuration(sc_time(getAttrAsUInt64("cycle_duration"), SC_NS));
		mModulePtr->setCyclesPerRead(getAttrAsUInt64("read_cycles"));
		mModulePtr->setCyclesPerWrite(getAttrAsUInt64("write_cycles"));
		mModulePtr->setChannelWidth(getAttrAsUInt64("channel_width"));
		mModulePtr->ReadLatency=sc_time(getAttrAsUInt64("read_cycles"), SC_NS);
		mModulePtr->WriteLatency=sc_time(getAttrAsUInt64("write_cycles"), SC_NS);

		if (getAttrAsUInt64("latency_enable")) {
			mModulePtr->setEnableLatency(true);
		} else {
			mModulePtr->setEnableLatency(false);
		}

        if(getAttrAsUInt64("dmi_enable")) {
            mModulePtr->setDmiEnable(true);
        } else {
            mModulePtr->setDmiEnable(false);
        }
	}


	virtual uint64_t getBaseAddress() override {
		return getAttrAsUInt64("base_address");
	}

	virtual uint64_t getSize() override {
		return getAttrAsUInt64("size");
	}

	virtual unsigned char* getActualAddress() override {
		return mModulePtr->getLocalMem();
	}

	virtual void finalize() override {
		if (getAttrAsUInt64("load_elf")) {
			cout<<"Loading ELF: "<<getAttr("elf_file")<<endl;
			mModulePtr->loadElfFile(getAttr("elf_file").c_str());
		}
		//VpsimIp* ip = VpsimIp::Find(getAttr("noc"));
       	//DynamicCoherenceInterconnect* noc=dynamic_cast<DynamicCoherenceInterconnect*>(ip);
       	//noc->mModulePtr->set_memory_word_length (getAttrAsUInt64("channel_width"));
	}

 private:
	memory* mModulePtr;

	friend struct DynamicBlobLoader;
 };

struct DynamicBlobLoader: public VpsimIp<InPortType, OutPortType>
{
	N_IN_PORTS_OVERRIDE(0);
	N_OUT_PORTS_OVERRIDE(0);

	DynamicBlobLoader(string name): VpsimIp(name) {
		registerRequiredAttribute("target_memory");
		registerRequiredAttribute("file");
		registerRequiredAttribute("offset");
	}

	virtual InPortType* getNextInPort() override {
		throw runtime_error(getName() + " : BlobLoader has no sockets.");
	}

	virtual OutPortType* getNextOutPort() override {
		throw runtime_error(getName() + " : BlobLoader has no sockets.");
	}

	virtual void make() override {
		checkAttributes();
	}

	virtual void finalize() override {
		VpsimIp* mem = VpsimIp::Find(getAttr("target_memory"));
		if (!mem) {
			throw runtime_error(getName() + ": Could not find target memory " + getAttr("target_memory"));
		}
		dynamic_cast<DynamicMemory*>(mem)->mModulePtr->loadBlob(
				getAttr("file").c_str(),
				getAttrAsUInt64("offset"));
		cout<<getName()<<" successfully loaded file "<<getAttr("file")<<" into memory "<<mem->getName()<<endl;
	}

	virtual void setStatsAndDie() override {
	}
};

struct DynamicElfLoader: public VpsimIp<InPortType, OutPortType>
{
	N_IN_PORTS_OVERRIDE(0);
	N_OUT_PORTS_OVERRIDE(0);

	NEEDS_DMI_OVERRIDE;

	DynamicElfLoader(string name): VpsimIp(name) {
		registerRequiredAttribute("path");
	}

	virtual InPortType* getNextInPort() override {
		throw runtime_error(getName() + " : BlobLoader has no sockets.");
	}

	virtual OutPortType* getNextOutPort() override {
		throw runtime_error(getName() + " : BlobLoader has no sockets.");
	}

	virtual void make() override {
		checkAttributes();
	}

	virtual void addDmiAddress(std::string targetIpName, uint64_t baseAddr, uint64_t size, unsigned char* pointer, bool cached, bool has_dmi) override {

		if (has_dmi) {
			elfloader loader;
			loader.elfloader_init((char*)pointer, size);
			loader.load_elf_file(getAttr("path"),baseAddr,size,false);
		}

	}

	virtual void finalize() override {
	}

	virtual void setStatsAndDie() override {
	}
};

struct DynamicMonitor: public VpsimIp<InPortType, OutPortType>
{
	N_IN_PORTS_OVERRIDE(0);
	N_OUT_PORTS_OVERRIDE(0);

	DynamicMonitor(string name): VpsimIp(name) {
		registerRequiredAttribute("start_address");
		registerRequiredAttribute("size");
		registerRequiredAttribute("cpu");
	}

	virtual InPortType* getNextInPort() override {
		throw runtime_error(getName() + " : Monitor has no sockets.");
	}

	virtual OutPortType* getNextOutPort() override {
		throw runtime_error(getName() + " : Monitor has no sockets.");
	}

	virtual void make() override {
		checkAttributes();
	}

	virtual void finalize() override {
		VpsimIp* cpu = VpsimIp::Find(getAttr("cpu"));
		if (!cpu) {
			throw runtime_error(getName() + ": Could not find target cpu to monitor " + getAttr("cpu"));
		}
		IssWrapper* issProvider=dynamic_cast<DynamicArm64*>(cpu)->getIssHandle();
		issProvider->monitorRange(getAttrAsUInt64("start_address"),getAttrAsUInt64("size"));
	}

	virtual void setStatsAndDie() override {
	}
};


struct DynamicUart
: public VpsimIp<InPortType, OutPortType>
{

public:
	DynamicUart(std::string name):
		VpsimIp(name),
		mModulePtr(nullptr) {

		registerRequiredAttribute("size");
		registerRequiredAttribute("base_address");
		registerRequiredAttribute("cycle_duration");
		registerRequiredAttribute("write_cycles");
		// registerRequiredAttribute("read_cycles");

		registerOptionalAttribute("latency_enable", "1");


	}

    void pushStats() override {
        if(mSegmentedStats.empty()){
            mSegmentedStats.push_back({
                                              {"reads",  "0"},
                                              {"writes", "0"}
                                      });
        }

        const auto& back = mSegmentedStats.back();
        auto reads = to_string(mModulePtr->getReadCount() - stoull(back.at("reads")));
        auto writes = to_string(mModulePtr->getWriteCount() - stoull(back.at("writes")));

        mSegmentedStats.push_back({
            {"reads",  reads},
            {"writes", writes}
        });
    }

	virtual void setStatsAndDie() override {
		if (mModulePtr) {
			mStats["reads"] = tostr(mModulePtr->getReadCount());
			mStats["writes"] = tostr(mModulePtr->getWriteCount());
			delete mModulePtr;
		}
	}

	MEMORY_MAPPED_OVERRIDE;

	N_IN_PORTS_OVERRIDE(1);
	N_OUT_PORTS_OVERRIDE(0);

	virtual InPortType* getNextInPort() override {
		return &(mModulePtr->mTargetSocket);
	}

	virtual OutPortType* getNextOutPort() override {
		throw runtime_error(getName() + " : uart has no out sockets.");
	}

	virtual void make() override {
		if (mModulePtr != nullptr) {
			throw runtime_error(getName() + " make() already called.");
		}
		checkAttributes();
		mModulePtr = new uart ( getName().c_str() );
		mModulePtr->setBaseAddress(getAttrAsUInt64("base_address"));
		//mModulePtr->setCycleDuration(sc_time(getAttrAsUInt64("cycle_duration"), SC_PS));
		mModulePtr->setCycleDuration(sc_time(getAttrAsUInt64("cycle_duration"), SC_NS));
		mModulePtr->setCyclesPerWrite(getAttrAsUInt64("write_cycles"));

		if (getAttrAsUInt64("latency_enable")) {
			mModulePtr->setEnableLatency(true);
		} else {
			mModulePtr->setEnableLatency(false);
		}
	}

	virtual uint64_t getBaseAddress() override {
		return getAttrAsUInt64("base_address");
	}

	virtual uint64_t getSize() override {
		return getAttrAsUInt64("size");
	}

	virtual unsigned char* getActualAddress() override {
		return (unsigned char*)-1;
	}

private:
	uart* mModulePtr;
};

struct DynamicItCtrl
: public VpsimIp<InPortType, OutPortType>
{

public:
	DynamicItCtrl(std::string name):
		VpsimIp(name),
		mModulePtr(nullptr) {


		registerRequiredAttribute("size");
		registerRequiredAttribute("base_address");

		registerRequiredAttribute("line_size");
		registerRequiredAttribute("size_per_cpu");
		//registerRequiredAttribute("ipi_irq_idx");
		//registerRequiredAttribute("ipi_line_offset");
		//registerRequiredAttribute("irc_size_per_cpu");

	}

    void pushStats() override {
        if(mSegmentedStats.empty()){
            mSegmentedStats.push_back({
                                              {"reads",  "0"},
                                              {"writes", "0"}
                                      });
        }

        const auto& back = mSegmentedStats.back();
        auto reads = to_string(mModulePtr->getReadCount() - stoull(back.at("reads")));
        auto writes = to_string(mModulePtr->getWriteCount() - stoull(back.at("writes")));

        mSegmentedStats.push_back({
                                          {"reads",  reads},
                                          {"writes", writes}
                                  });
    }

	virtual void setStatsAndDie() override {
		if (mModulePtr) {
			mStats["reads"] = tostr(mModulePtr->getReadCount());
			mStats["writes"] = tostr(mModulePtr->getWriteCount());
			delete mModulePtr;
		}
	}

	MEMORY_MAPPED_OVERRIDE;
	// HAS_DMI_OVERRIDE;

	N_IN_PORTS_OVERRIDE(1);
	N_OUT_PORTS_OVERRIDE(0);

	virtual InPortType* getNextInPort() override {
		return &(mModulePtr->mTargetSocket);
	}

	virtual OutPortType* getNextOutPort() override {
		throw runtime_error(getName() + " : itctrl has no out sockets.");
	}

	virtual void make() override {
		if (mModulePtr != nullptr) {
			throw runtime_error(getName() +  " : make() already called.");
		}
		checkAttributes();
		mModulePtr = new ItCtrl ( getName().c_str(),
				getAttrAsUInt64("size") / getAttrAsUInt64("line_size"),
				getAttrAsUInt64("line_size"));
		mModulePtr->setBaseAddress(getAttrAsUInt64("base_address"));
	}

	virtual uint64_t getBaseAddress() override {
		return getAttrAsUInt64("base_address");
	}

	virtual uint64_t getSize() override {
		return getAttrAsUInt64("size");
	}

	virtual unsigned char* getActualAddress() override {
		return (unsigned char*)-1;// reinterpret_cast<unsigned char*>(mModulePtr->getLocalMem()); // We don't have to provide address as we don't support DMI
	}

	virtual void finalize() override {

		if (AllInstances.find("Arm") != AllInstances.end()) {
			std::cout<<"FIXME: Auto-mapping arm interrupt lines"<<endl;
			VpsimIp<InPortType,OutPortType>::MapTypeIf("Arm",
					[](VpsimIp<InPortType,OutPortType>* ip) { return ip->isProcessor(); },
					[this](VpsimIp<InPortType,OutPortType>* ip) {
						unsigned nLines = this->getAttrAsUInt64("size")/this->getAttrAsUInt64("line_size");
						for (unsigned i = 0; i < nLines; i++) {
							this->mModulePtr->Map(ip->getAttrAsUInt64("cpu_id") * this->getAttrAsUInt64("size_per_cpu") /
								this->getAttrAsUInt64("line_size") + i,
								ip->getIrqIf(),
								i
								);
						}
					}
				);
		}
	}

private:
	ItCtrl* mModulePtr;
};

struct DynamicPL011Uart :
        public PL011Uart,
        public VpsimIp<InPortType, OutPortType> {

public:
    explicit DynamicPL011Uart(std::string name) :
			PL011Uart(name.c_str()),
            VpsimIp(name) {

        registerRequiredAttribute("base_address");
        registerOptionalAttribute("size", "4095");
        registerOptionalAttribute("cycle_duration", "100e3");
        registerOptionalAttribute("write_cycles", "1");
        registerOptionalAttribute("read_cycles", "1");

        registerRequiredAttribute("interrupt_parent");
        registerRequiredAttribute("irq_n");
        registerRequiredAttribute("poll_period");

        registerRequiredAttribute("channel");
    }

    ~DynamicPL011Uart() override {
    }

    MEMORY_MAPPED_OVERRIDE;

    unsigned getMaxInPortCount() override {
        return 1;
    }

    unsigned getMaxOutPortCount() override {
        return 0;
    }

    InPortType *getNextInPort() override {
        return &mTargetSocket;
    }

    OutPortType *getNextOutPort() override {
        throw runtime_error(VpsimIp::getName() + " : PL011Uart has no out sockets.");
    }

    void make() override {
        checkAttributes();
        setBaseAddress(getAttrAsUInt64("base_address"));
        //setCycleDuration(sc_time(getAttrAsUInt64("cycle_duration"), SC_PS));
        setCycleDuration(sc_time(getAttrAsUInt64("cycle_duration"), SC_NS));
        setCyclesPerWrite(static_cast<int>(getAttrAsUInt64("write_cycles")));
        setInterruptLine(getAttrAsUInt64("irq_n"));
        //setPollPeriod(sc_time(getAttrAsUInt64("poll_period"), SC_PS));
        setPollPeriod(sc_time(getAttrAsUInt64("poll_period"), SC_NS));
        selectChannel(getAttr("channel"));
    }

    uint64_t getBaseAddress() override {
        return getAttrAsUInt64("base_address");
    }

    uint64_t getSize() override {
        return getAttrAsUInt64("size");
    }

    unsigned char *getActualAddress() override {
        return (unsigned char*)getLocalMem();
    }

    virtual void finalize() override {

		//if (AllInstances.find("Arm") != AllInstances.end()) {

			VpsimIp<InPortType,OutPortType>::MapIf(
					[this](VpsimIp<InPortType,OutPortType>* ip) { return ip->getName() == this->getAttr("interrupt_parent"); },
					[this](VpsimIp<InPortType,OutPortType>* ip) {
						this->setInterruptParent(ip->getIrqIf());
						cout<<"Set interrupt parent of "<<this->VpsimIp::getName()<< " to "<<ip->getName()<<endl;
					}
				);
		//}

	}


    virtual void setStatsAndDie() override {
    }
};

struct DynamicXuartPs :
        public xuartps,
        public VpsimIp<InPortType, OutPortType> {

public:
    explicit DynamicXuartPs(std::string name) :
		xuartps(name.c_str()),
            VpsimIp(name) {

        registerRequiredAttribute("base_address");
        registerOptionalAttribute("cycle_duration", "100e3");
        registerOptionalAttribute("write_cycles", "1");
        registerOptionalAttribute("read_cycles", "1");

        registerRequiredAttribute("interrupt_parent");
        registerRequiredAttribute("irq_n");

        registerRequiredAttribute("poll_period");

        registerRequiredAttribute("channel");
    }

    ~DynamicXuartPs() override {
    }

    MEMORY_MAPPED_OVERRIDE;

    unsigned getMaxInPortCount() override {
        return 1;
    }

    unsigned getMaxOutPortCount() override {
        return 0;
    }

    InPortType *getNextInPort() override {
        return &mTargetSocket;
    }

    OutPortType *getNextOutPort() override {
        throw runtime_error(VpsimIp::getName() + " : XuartPs has no out sockets.");
    }

    void make() override {
        checkAttributes();
        setBaseAddress(getAttrAsUInt64("base_address"));
        //setCycleDuration(sc_time(getAttrAsUInt64("cycle_duration"), SC_PS));
        setCycleDuration(sc_time(getAttrAsUInt64("cycle_duration"), SC_NS));
        setCyclesPerWrite(static_cast<int>(getAttrAsUInt64("write_cycles")));
        setInterruptLine(getAttrAsUInt64("irq_n"));
        selectChannel(getAttr("channel"));
        //setPollPeriod(sc_time(getAttrAsUInt64("poll_period"),SC_PS));
        setPollPeriod(sc_time(getAttrAsUInt64("poll_period"),SC_NS));
    }

    uint64_t getBaseAddress() override {
        return getAttrAsUInt64("base_address");
    }

    uint64_t getSize() override {
        return 0x1000;
    }

    unsigned char *getActualAddress() override {
        return (unsigned char*)getLocalMem();
    }

    virtual void finalize() override {

		//if (AllInstances.find("Arm") != AllInstances.end()) {

			VpsimIp<InPortType,OutPortType>::MapIf(
					[this](VpsimIp<InPortType,OutPortType>* ip) { return ip->getName() == this->getAttr("interrupt_parent"); },
					[this](VpsimIp<InPortType,OutPortType>* ip) {
						this->setInterruptParent(ip->getIrqIf());
						cout<<"Set interrupt parent of "<<this->VpsimIp::getName()<< " to "<<ip->getName()<<endl;
					}
				);
		//}

	}


    virtual void setStatsAndDie() override {
    }
};

struct DynamicAddressTranslator :
        public VpsimIp<InPortType, OutPortType>,
		public AddressTranslator {

public:
    explicit DynamicAddressTranslator(std::string name) :
            VpsimIp(name),AddressTranslator(name.c_str()) {

        registerRequiredAttribute("base_address");
        registerRequiredAttribute("size");
        registerRequiredAttribute("output_base_address");
    }


    MEMORY_MAPPED_OVERRIDE;

    unsigned getMaxInPortCount() override {
        return 1;
    }

    unsigned getMaxOutPortCount() override {
        return 1;
    }

    InPortType *getNextInPort() override {
        return &mSockIn;
    }

    OutPortType *getNextOutPort() override {
    	return &mSockOut;
    }

    void make() override {
        checkAttributes();

        setShift(getAttrAsUInt64("output_base_address") - getAttrAsUInt64("base_address"));
    }

    uint64_t getBaseAddress() override {
        return getAttrAsUInt64("base_address");
    }

    uint64_t getSize() override {
        return getAttrAsUInt64("size");
    }

    unsigned char *getActualAddress() override {
        return (unsigned char*)nullptr;
    }
/*
    VpsimModule *asModule() override {
        return nullptr;
    }*/

    virtual void connect(std::string outPortAlias, VpsimIp<InPortType,OutPortType>* otherIp, std::string inPortAlias) override {

		VpsimIp<InPortType,OutPortType>::connect(outPortAlias, otherIp, inPortAlias);
	}

    virtual void finalize() override {

	}


    virtual void setStatsAndDie() override {
    }

};


template<class T>
class DynamicTLMCallbackRegister :
        public TLMCallbackRegister<T>,
        public VpsimIp<InPortType, OutPortType>
{

public:
    explicit DynamicTLMCallbackRegister(std::string name) :
            TLMCallbackRegister<T>(name.c_str()),
            VpsimIp(name)
    {
        registerRequiredAttribute("base_address");
        registerOptionalAttribute("cycle_duration", "1e3");
        registerOptionalAttribute("write_cycles", "1");
        registerOptionalAttribute("read_cycles", "1");
    }

    ~DynamicTLMCallbackRegister() override {
        this->PrintStatistics();
    }

    MEMORY_MAPPED_OVERRIDE;

    unsigned getMaxInPortCount() override {
        return 1;
    }

    unsigned getMaxOutPortCount() override {
        return 0;
    }

    InPortType *getNextInPort() override {
        return &this->mTargetSocket;
    }

    OutPortType *getNextOutPort() override {
        throw runtime_error(VpsimIp::getName() + " : CallbackRegister has no out sockets.");
    }

    void make() override {
        checkAttributes();
        this->setBaseAddress(getAttrAsUInt64("base_address"));
        //this->setCycleDuration(sc_time(getAttrAsUInt64("cycle_duration"), SC_PS));
        this->setCycleDuration(sc_time(getAttrAsUInt64("cycle_duration"), SC_NS));
        this->setCyclesPerWrite(static_cast<int>(getAttrAsUInt64("write_cycles")));
    }

    uint64_t getBaseAddress() override {
        return getAttrAsUInt64("base_address");
    }

    uint64_t getSize() override {
        return sizeof(T);
    }

    unsigned char *getActualAddress() override {
        return reinterpret_cast<unsigned char*>(this->getLocalMem());
    }

    void pushStats() override {
        if(mSegmentedStats.empty()){
            mSegmentedStats.push_back({
                                              {"reads",  "0"},
                                              {"writes", "0"}
                                      });
        }

        const auto& back = mSegmentedStats.back();
        auto reads = to_string(this->getReadCount() - stoull(back.at("reads")));
        auto writes = to_string(this->getWriteCount() - stoull(back.at("writes")));

        mSegmentedStats.push_back({
                                          {"reads",  reads},
                                          {"writes", writes}
                                  });
    }

    void setStatsAndDie() override{
        mStats["nb_reads"] = tostr(this->getNbReads());
        mStats["nb_writes"] = tostr(this->getNbWrites());
    }

    void registerCallback(uint64_t val, const string& callback) override{
        TLMCallbackRegister<T>::registerCallback(val, callback);
    }

};

struct DynamicSesamController :
        public VpsimIp<InPortType, OutPortType>,
        public SesamController
{
private:
	monitorState mState;
	uint64_t mBytesPerLine;
	uint32_t mCurrentDomain;
	static bool InstanceExists;
	bool mValid;

	// benchmarking data
	string appName;
	bool mInBenchmark;
	uint32_t mBenchDomain;
	sc_time mBenchStartTime;

	// checkpoint related data
	map<string, int> mCheckpoints;
public:
    explicit DynamicSesamController(std::string name) :
            VpsimIp(name),
            SesamController(name.c_str())
    {
        registerRequiredAttribute("base_address");
        registerOptionalAttribute("size", "4");


        //SC_THREAD(monitorSimulation);
    	mState=RUN;
    	mBytesPerLine=8;
    	if (!InstanceExists) {
    		InstanceExists=true;
    		mValid=true;
    	} else
    		mValid=false;
    	mInBenchmark=false;
    	//dont_initialize();
    }

    SC_HAS_PROCESS(DynamicSesamController);

    ~DynamicSesamController() override {
    }

    N_IN_PORTS_OVERRIDE(1);
    N_OUT_PORTS_OVERRIDE(0);
    MEMORY_MAPPED_OVERRIDE;

    InPortType *getNextInPort() override {
        return &mTargetSocket;
    }

    OutPortType *getNextOutPort() override {
        throw runtime_error(VpsimIp::getName() + " : SesamController has no out sockets.");
    }

    void make() override {
        checkAttributes();
        setBaseAddress(getAttrAsUInt64("base_address"));
        setPtrState(&mState);
    	mCurrentDomain = this->getAttrAsUInt64("domain");
    }

    virtual uint64_t getBaseAddress() override {
        return getAttrAsUInt64("base_address");
    }

    virtual uint64_t getSize() override {
	return getAttrAsUInt64("size");
    }

    void monitorSimulation() {
    	if (!mValid)
    		return;
        wait (100000, SC_NS);
    	system("clear");
    	printf("Press Enter to start VPSim monitor.\n");
    	while (true) {
    		if (ready()) {
    			process();
    		}
            wait(100000, SC_NS);
    	}
    }

    bool ready() {
    	return ChannelManager::fdCheckReady(0);
    }

    string getLine() {
    	char line[1024];
    	fgets(line, 1024, stdin);
    	return string(line);
    }

    string prompt() {
    	stringstream ss;
    	ss<<"\n"<<"@"<<sc_time_stamp();
    	printf("%s sesam # ", ss.str().c_str());
    	return getLine();
    }

    vector<string> getArgv(istringstream &ss) {
    	vector<string> argv;
    	ss >> std::skipws;
    	string curr;
    	while (ss >> curr) {
    		argv.push_back(curr);
    	}
    	return argv;
    }


    void sesamCommand(vector<string> &args) override {
        switch(mState) {
            case RUN: {
                if (mInBenchmark) {
                    mInBenchmark=false;
                    // get and display stats
                    //printf("Checking all IPs of domain %ld\n", this->mBenchDomain);
                    fflush(stdout);
                    mCommandOutputBuffer=string();
                    VpsimIp::MapIf(
                                    [this](VpsimIp* ip) {
                    					//printf("ip %s is of domain %ld\n", ip->getName().c_str(),ip->getAttrAsUInt64("domain"));
                                    	return ip->getAttrAsUInt64("domain")==this->mBenchDomain;
                    				},
                                    [this](VpsimIp* ip) {
                                            // Go back to fast mode !
                                            /*if (ip->isMemoryMapped() && ip->hasDmi()) {
                                                    AddrSpace as(ip->getBaseAddress(),ip->getBaseAddress()+ip->getSize()-1);
                                                    ParamManager::get().setParameter(ip->getName(), as, BlockingTLMEnabledParameter::bt_disabled);
                                            }*/
                                            // now segment stats
                                            ip->pushStats();

                                            auto &stats = ip->getSegStats().back();

                                            //printf("Fetching stats for IP %s, has %ld stats\n", ip->getName().c_str(), stats.size());
											//fflush(stdout);



                                            if (stats.size()) {
                                            		mCommandOutputBuffer += "-----------------------------------\n";
                                            		mCommandOutputBuffer += "\nStatistics from ";
                                            		mCommandOutputBuffer += ip->getName() + "\n";
                                                    for (auto& stat: stats) {
                                                    	mCommandOutputBuffer += "\t";
                                                    	mCommandOutputBuffer += stat.first + " = ";
                                                    	mCommandOutputBuffer += stat.second + "\n";
                                                    }
                                                    ip->clearSegStats();
                                            }
                            }
                    );
                    sc_time diff = sc_time_stamp() - mBenchStartTime;
                    stringstream ss;
                    ss<<diff;
                    mCommandOutputBuffer += "Simulated time: ";
                    mCommandOutputBuffer += ss.str() + "\n";
					std::FILE* LogFile = fopen((std::string("sesamBench_")+ appName +std::string("_")+std::to_string(nbCommandCounter++) +".log").c_str(), "w");
					fprintf(LogFile,"%s",mCommandOutputBuffer.c_str());
					fclose(LogFile);
                }
            }
            break;
            case TAKE_CMD: {
                string cmd = args.at(0);
                if (cmd == "quit") {
                    sc_stop();
                    return;
                } else if (cmd == "show") {
                    if (args.size() - 1 < 1) {
                        printf("Usage: show component1_name component2_name ...\n");
                        return;
                    }
                    for (unsigned i = 1; i < args.size(); i++) {
                        string component = args.at(i);
                        VpsimIp* ip = VpsimIp::Find(component);
                        if (!ip) {
                            printf("Error: Component %s not known to VPSim.\n", component.c_str());
                        } else {
                            ip->show();
                        }
                    }
                } else if (cmd == "showmem") {
                    if (args.size()-1 != 2) {
                            printf("Usage: showmem start_addr size\n");
                            return;
                    }
                    bool covered=false;
                    istringstream st (args.at(1));
                    istringstream sz (args.at(2));
                    uint64_t start, size;
                    st>>hex>>start;
                    sz>>hex>>size;

                    while (!covered) {
                            uint64_t end = start + size - 1;
                            bool found = false;
                            VpsimIp::MapIf(	[this,&start]
											(VpsimIp* ip){
												return ip->getAttrAsUInt64("domain")==this->mCurrentDomain
												&& ip->isMemoryMapped()
												&& ip->getActualAddress()!=nullptr
												&& ip->getActualAddress()!=(unsigned char*)-1
												&& ip->getBaseAddress() <= start
												&& start < ip->getBaseAddress()+ip->getSize();
											}
											,
                                            [this,&start,&found,&end ](VpsimIp* ip) {
                                                    uint64_t actualEnd = std::min(end, ip->getBaseAddress()+ip->getSize()-1);
                                                    uint64_t actualSize = actualEnd-start+1;
                                                    uint64_t lines = actualSize / mBytesPerLine;
                                                    uint64_t left = actualSize;
                                                    found=true;
                                                    for (uint64_t i = 0; i < lines; i++) {
                                                            printf("\n%016" PRIu64 "\t",start+i*mBytesPerLine);
                                                            uint64_t Left=std::min(mBytesPerLine,left);
                                                            for (uint64_t j = 0; j < Left; j++) {
                                                                    printf("%02X\t", ip->getActualAddress()[i*mBytesPerLine+j]);
                                                                    left--;
                                                            }
                                                    }
                                                    start = actualEnd+1;

                                            }
                                        );
                            if (!found) {
                                    printf("\nWarning: address space %" PRIu64 " to %" PRIu64 " not covered.\n",start,end);
                                    break;
                            }
                            if (start > end) {
                                    covered=true;
                            }
                    }
                    printf("\n");
                } else if (cmd == "list") {
                	if (args.size() - 1 != 0) {
                		printf("Usage: list\n");
                		return;
                	}
                	mCommandOutputBuffer=string();

                	VpsimIp::MapIf(
                			[this](VpsimIp* ip) { return ip->getAttrAsUInt64("domain")==this->mCurrentDomain; }, // all IPs
							[this](VpsimIp* ip) {
                				mCommandOutputBuffer += ip->getName() + "\n";
                			}
                	);
                } else if (cmd == "configure") {
                	if (args.size() - 1 < 3) {
                		printf("Usage: configure component_family parameter value\n");
                		return;
                	}

                	string component = args.at(1);
                	VpsimIp* ip = VpsimIp::Find(component);
                	if (!ip) {
                		printf("Error: Component %s not known to VPSim.\n", component.c_str());
                	} else {
                		// change component configuration
                		ip->configure();
                	}
                } else if (cmd == "debug") {
                    if (args.size() - 1 < 2) {
                            printf("Usage: debug lvl component1 component2 component3 ... \n");
                            return;
                    }
                    istringstream lv(args[1]);
                    uint64_t lvl;
                    lv >> lvl;
                    for (unsigned i = 2; i < args.size(); i++) {
                            LoggerCore::get().enableLogging(true);
                            LoggerCore::get().setDebugLvl(args[i],(DebugLvl)lvl);

                            // Also make sure accesses reach the component.

                    }
                } else if (cmd == "watch") {
                    if (args.size() - 1 < 2) {
                            printf("Usage: watch base size\n");
                            return;
                    }
                    istringstream st (args.at(1));
                    istringstream sz (args.at(2));
                    uint64_t start, size;
                    st>>hex>>start;
                    sz>>hex>>size;
                    printf("Now monitoring following ranges: \n");


                    // Ask all IPs to watch out for this address space !
                    VpsimIp::MapIf(
                                    [this](VpsimIp* ip) { return ip->getAttrAsUInt64("domain")==this->mCurrentDomain; },
                                    [&start,&size](VpsimIp* ip) {
                                            AddrSpace as(start,start+size-1);
                                            // First make sure accesses reach their targets (i.e. No DMI !)
                                            try{
                                                    ParamManager::get().setParameter(ip->getName(), as, BlockingTLMEnabledParameter::bt_enabled);
                                            } catch(exception &ex){

                                            }

                                            ip->addMonitor(start,size);
                                            ip->showMonitor();
                                    }
                    );
                } else if (cmd == "unwatch") {
                    if (args.size() - 1 < 2) {
                            printf("Usage: unwatch base size\n");
                            return;
                    }
                    istringstream st (args.at(1));
                    istringstream sz (args.at(2));
                    uint64_t start, size;
                    st>>hex>>start;
                    sz>>hex>>size;

                    printf("Now monitoring following ranges: \n");

                    // Ask all IPs to watch out for this address space !
                    VpsimIp::MapIf(
                                    [this](VpsimIp* ip) { return ip->getAttrAsUInt64("domain")==this->mCurrentDomain; }, // all IPs
                                    [&start,&size](VpsimIp* ip) {
                                            AddrSpace as(start,start+size-1);
                                            // First make sure accesses reach their targets (i.e. No DMI !)

                                            // broken, should use default param instead.
                                            try{
                                                    if (ip->hasDmi())
                                                            ParamManager::get().setParameter(ip->getName(), as, BlockingTLMEnabledParameter::bt_disabled);
                                            } catch(exception &ex){

                                            }
                                            ip->removeMonitor(start,size);
                                            ip->showMonitor();
                            }
                    );
                } else if (cmd == "benchmark") {
                    if (args.size() - 2 != 0) {
                            printf("Usage: benchmark app\n");
                            return;
                    }
                    // First, create a new stats segment
                    VpsimIp::MapIf(
                                    [this](VpsimIp* ip) { return ip->getAttrAsUInt64("domain")==this->mCurrentDomain; }, // all IPs
                                    [](VpsimIp* ip) {
                                            // Enable access simulation to all IPs
                                            /*if (ip->isMemoryMapped()) {
                                                    AddrSpace as(ip->getBaseAddress(),ip->getBaseAddress()+ip->getSize()-1);
                                                    ParamManager::get().setParameter(ip->getName(), as, BlockingTLMEnabledParameter::bt_enabled);
                                            }*/
                                            // now segment stats
                                            //printf("Pushing stats for IP %s\n", ip->getName().c_str());
                                            ip->pushStats();
                                            //printf("Push ok.\n");
                            }
                    );
                    appName = args.at(1);
                    mInBenchmark=true;
                    mBenchDomain=mCurrentDomain;
                    mBenchStartTime=sc_time_stamp();
                    fprintf(stderr, "Benchmark mode started.\n");
                }
            }
            break;
            default:
    		throw runtime_error("SesamController in unknown state.");
        }
    }




    void process() {
    	switch (mState) {
    	case RUN: {
			getLine();
			if (mInBenchmark) {
				mInBenchmark=false;
				// get and display stats
				VpsimIp::MapIf(
						[this](VpsimIp* ip) { return ip->getAttrAsUInt64("domain")==this->mBenchDomain; },
						[](VpsimIp* ip) {
							// Go back to fast mode !
							if (ip->isMemoryMapped() && ip->hasDmi()) {
								AddrSpace as(ip->getBaseAddress(),ip->getBaseAddress()+ip->getSize()-1);
								ParamManager::get().setParameter(ip->getName(), as, BlockingTLMEnabledParameter::bt_disabled);
							}
							// now segment stats
							ip->pushStats();
							auto &stats = ip->getSegStats().back();
							if (stats.size()) {
								printf("-----------------------------------\n");
								printf("\nStatistics from %s:\n",ip->getName().c_str());
								for (auto& stat: stats) {
									printf("\t%s = %s\n", stat.first.c_str(), stat.second.c_str());
								}
							}
						}
				);
				sc_time diff = sc_time_stamp() - mBenchStartTime;
				stringstream ss;
				ss<<diff;
				printf("Simulated time: %s\n",ss.str().c_str());

			}
    		mState=TAKE_CMD;
    		process();
    	}
    	break;
    	case TAKE_CMD: {
			do {
				istringstream userInput ( prompt() );
				vector<string> argv = getArgv(userInput);
				if (argv.size() == 0)
					continue;
				string cmd = argv.at(0);
				if (cmd == "quit") {
					sc_stop();
					return;
				} else if (cmd == "go") {
					mState = RUN;
				} else if (cmd == "show") {
					if (argv.size()-1 < 1) {
						printf("Usage: show component1_name component2_name ...\n");
						continue;
					}
					for (unsigned i = 1; i < argv.size(); i++) {
						string component = argv.at(i);
						VpsimIp* ip = VpsimIp::Find(component);
						if (!ip) {
							printf("Error: Component %s not known to VPSim.\n", component.c_str());
						} else {
							ip->show();
						}
					}
				} else if (cmd == "showmem") {
					if (argv.size()-1 != 2) {
						printf("Usage: showmem start_addr size\n");
						continue;
					}
					bool covered=false;
					istringstream st (argv.at(1));
					istringstream sz (argv.at(2));
					uint64_t start, size;
					st>>hex>>start;
					sz>>hex>>size;

					while (!covered) {
						uint64_t end = start + size - 1;
						bool found = false;
						VpsimIp::MapIf(
							[this,&start](VpsimIp* ip){
								return ip->getAttrAsUInt64("domain")==this->mCurrentDomain
								&& ip->isMemoryMapped()
								&& ip->getActualAddress()!=nullptr
								&& ip->getActualAddress()!=(unsigned char*)-1
								&& ip->getBaseAddress() <= start
								&& start < ip->getBaseAddress()+ip->getSize();
							}
							,
							[this,&start,&found, &end](VpsimIp* ip) {
									uint64_t actualEnd = std::min(end, ip->getBaseAddress()+ip->getSize()-1);
									uint64_t actualSize = actualEnd-start+1;
									uint64_t lines = actualSize / mBytesPerLine;
									uint64_t left = actualSize;
									found=true;
									for (uint64_t i = 0; i < lines; i++) {
										printf("\n%016" PRIu64 "\t",start+i*mBytesPerLine);
										uint64_t Left=std::min(mBytesPerLine,left);
										for (uint64_t j = 0; j < Left; j++) {
											printf("%02X\t", ip->getActualAddress()[i*mBytesPerLine+j]);
											left--;
										}
									}
									start = actualEnd+1;
							}
						);
						if (!found) {
							printf("\nWarning: address space %016" PRIu64 " to %016" PRIu64 " not covered.\n",start,end);
							break;
						}
						if (start > end) {
							covered=true;
						}
					}
					printf("\n");
				} else if (cmd == "help") {
					printf("Available commands are:\n");
					printf("show component1 component2 component3 ... : Dump some components' current status\n");
					printf("showmem base size : Display 'size' bytes starting from address 'base'\n");
					printf("debug lvl component1 component2 component3 ... : Set debug level for some components to 'lvl'\n");
					printf("watch base size : Log all accesses between base and base+size-1\n");
					printf("unwatch base size : Stop logging all accesses between base and base+size-1\n");
					printf("benchmark : Enter precise simulation mode to benchmark an application\n");
					printf("domainof c: Switch address domain to that of component 'c'\n");
					printf("\n");
				} else if (cmd == "debug") {
					if (argv.size() - 1 < 2) {
						printf("Usage: debug lvl component1 component2 component3 ... \n");
						continue;
					}
					istringstream lv(argv[1]);
					uint64_t lvl;
					lv >> lvl;
					for (unsigned i = 2; i < argv.size(); i++) {
						LoggerCore::get().enableLogging(true);
						LoggerCore::get().setDebugLvl(argv[i],(DebugLvl)lvl);

						// Also make sure accesses reach the component.

					}

				} else if (cmd == "watch") {
					if (argv.size() - 1 < 2) {
						printf("Usage: watch base size\n");
						continue;
					}
					istringstream st (argv.at(1));
					istringstream sz (argv.at(2));
					uint64_t start, size;
					st>>hex>>start;
					sz>>hex>>size;
					printf("Now monitoring following ranges: \n");


					// Ask all IPs to watch out for this address space !
					VpsimIp::MapIf(
							[this](VpsimIp* ip) { return ip->getAttrAsUInt64("domain")==this->mCurrentDomain; },
							[&start,&size](VpsimIp* ip) {
								AddrSpace as(start,start+size-1);
								// First make sure accesses reach their targets (i.e. No DMI !)
								try{
									ParamManager::get().setParameter(ip->getName(), as, BlockingTLMEnabledParameter::bt_enabled);
								} catch(exception &ex){

								}

								ip->addMonitor(start,size);
								ip->showMonitor();
						}
					);


				} else if (cmd == "unwatch") {
					if (argv.size() - 1 < 2) {
						printf("Usage: unwatch base size\n");
						continue;
					}
					istringstream st (argv.at(1));
					istringstream sz (argv.at(2));
					uint64_t start, size;
					st>>hex>>start;
					sz>>hex>>size;

					printf("Now monitoring following ranges: \n");

					// Ask all IPs to watch out for this address space !
					VpsimIp::MapIf(
							[this](VpsimIp* ip) { return ip->getAttrAsUInt64("domain")==this->mCurrentDomain; }, // all IPs
							[&start,&size](VpsimIp* ip) {
								AddrSpace as(start,start+size-1);
								// First make sure accesses reach their targets (i.e. No DMI !)

								// broken, should use default param instead.
								try{
									if (ip->hasDmi())
										ParamManager::get().setParameter(ip->getName(), as, BlockingTLMEnabledParameter::bt_disabled);
								} catch(exception &ex){

								}
								ip->removeMonitor(start,size);
								ip->showMonitor();
						}
					);




				} else if (cmd == "domainof") {
					if (argv.size() - 1 != 1) {
						printf("Usage: domainof component_name\n");
						continue;
					}
					string cmp (argv.at(1));
					VpsimIp* ip = VpsimIp::Find(cmp);
					if (ip == nullptr) {
						printf("No component named %s\n",cmp.c_str());
					} else {
						mCurrentDomain=ip->getAttrAsUInt64("domain");
					}
				} else if (cmd == "benchmark") {
					if (argv.size() - 1 != 0) {
						printf("Usage: benchmark\n");
						continue;
					}
					// First, create a new stats segment
					VpsimIp::MapIf(
							[this](VpsimIp* ip) { return ip->getAttrAsUInt64("domain")==this->mCurrentDomain; }, // all IPs
							[](VpsimIp* ip) {
								// Enable access simulation to all IPs
								//fprintf(stderr, "now with %s\n",ip->getName().c_str());
								if (ip->isMemoryMapped()) {
									//fprintf(stderr, "enabling TLM of %s\n",ip->getName().c_str());

									AddrSpace as(ip->getBaseAddress(),ip->getBaseAddress()+ip->getSize()-1);
									ParamManager::get().setParameter(ip->getName(), as, BlockingTLMEnabledParameter::bt_enabled);
									//fprintf(stderr, "enabled TLM of %s\n",ip->getName().c_str());

								}
								// now segment stats
								//fprintf(stderr, "pushing stats of %s\n",ip->getName().c_str());
								ip->pushStats();
								//fprintf(stderr, "pushed stats of %s\n",ip->getName().c_str());
							}
					);

					mInBenchmark=true;
					mBenchDomain=mCurrentDomain;
					mBenchStartTime=sc_time_stamp();
					fprintf(stderr, "Benchmark mode started.\n");
				} else if (cmd == "mips") {
					if (argv.size() - 1 != 0) {
						printf("Usage: mips\n");
						continue;
					}
				} else if (cmd == "checkpoint") {
					if (argv.size() - 1 != 1) {
						printf("Usage: checkpoint id\n");
						continue;
					}

					mCheckpoints[argv[1]] = getpid();

					if (fork()) {
						// parent, wait for signal
						sigset_t s;
						sigemptyset(&s);
						sigaddset(&s, SIGUSR1);
						int received_sig;
						sigwait(&s, &received_sig);

						// here after rollback
						for (auto it=mCheckpoints.begin(); it!=mCheckpoints.end(); it++) {
							if (it->second==getpid()) {
								mCheckpoints.erase(it);
								break;
							}
						}
					}

				} else if (cmd == "rollback") {
					if (argv.size() - 1 != 1) {
						printf("Usage: rollback id\n");
						continue;
					}
					if (mCheckpoints.find(argv[1])!=mCheckpoints.end()) {
						int pid = mCheckpoints[argv[1]];
						kill(pid, SIGUSR1);
						sc_stop();
						return;
					} else {
						printf("Unknown checkpoint: %s\n",argv[1].c_str());
					}
				}
			} while (mState == TAKE_CMD && sc_core::sc_is_running());
    	}
    	break;
    	default:
    		throw runtime_error("SesamController in unknown state.");
    	}
    }
};

struct DynamicPythonDevice :
    public VpsimIp<InPortType, OutPortType> {

	explicit DynamicPythonDevice(std::string name) :
				VpsimIp(name),
				mModulePtr(nullptr) {
		registerRequiredAttribute("base_address");
		registerRequiredAttribute("size");
		registerRequiredAttribute("interrupt_parent");
		registerRequiredAttribute("py_module_name");
		registerRequiredAttribute("param_string");
	}

	MEMORY_MAPPED_OVERRIDE;

	virtual unsigned getMaxInPortCount() override {
		return 1;
	}

	virtual unsigned getMaxOutPortCount() override {
		return 0;
	}

	virtual InPortType *getNextInPort() override {
		return &mModulePtr->mTargetSocket;;
	}

	virtual OutPortType *getNextOutPort() override {
		throw runtime_error("Python Device currently only has one input socket.");
	}

	virtual void make() override {
		checkAttributes();
		string params = getAttr("param_string");
		char* x = strdup(params.c_str());
		char* token=strtok(x,",");
		vector<string> tmp;
		while(token){
			tmp.push_back(token);
			token=strtok(NULL,",");
		}
		vector<string> tmpParam;
		map<string,string> args;
		for (auto t: tmp){
			char* y=strdup(t.c_str());
			token=strtok(y,"=");
			while(token){
				tmpParam.push_back(token);
				token=strtok(NULL,"=");
			}
			args[tmpParam.at(0)]=tmpParam.at(1);
			tmpParam.clear();
		}
		mModulePtr=new PyDevice(getName().c_str(), getAttr("py_module_name"), args ,getAttrAsUInt64("size"));
		mModulePtr->setBaseAddress(getAttrAsUInt64("base_address"));
	}

	virtual uint64_t getBaseAddress() override {
		return mModulePtr->getBaseAddress();
	}

	virtual uint64_t getSize() override {
		return mModulePtr->getSize();
	}

	virtual unsigned char *getActualAddress() override {
		return (unsigned char*)mModulePtr->getLocalMem();
	}

	virtual void setStatsAndDie() override {
		if (mModulePtr) {
			delete mModulePtr;
		}
	}

	 virtual void finalize() override {
		VpsimIp<InPortType,OutPortType>::MapIf(
			[this](VpsimIp<InPortType,OutPortType>* ip) { return ip->getName() == this->getAttr("interrupt_parent"); },
			[this](VpsimIp<InPortType,OutPortType>* ip) {
				this->mModulePtr->setInterruptParent(ip->getIrqIf());
				cout<<"Set interrupt parent of "<<this->VpsimIp::getName()<< " to "<<ip->getName()<<endl;
			}
		);


	}

	virtual sc_module* getScModule() override { return mModulePtr; }



private:
	PyDevice* mModulePtr;

};

} // namespace vpsim

#endif /* DYNAMICCOMPONENTS_HPP_ */

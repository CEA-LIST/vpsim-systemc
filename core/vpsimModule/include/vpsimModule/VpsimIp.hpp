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

#ifndef _VPSIMIP_HPP_
#define _VPSIMIP_HPP_

#include <iostream>
#include <unordered_map>
#include <functional>
#include <vector>
#include <chrono>
#include <sstream>

#include "vpsimModule/vpsimModule.hpp"
#include "vpsimModule/ExtraIpFeatures_if.hpp"
#include "logger/log.hpp"
#include "ForwardSimpleSocket.hpp"


using namespace std;

namespace vpsim {

class InterruptIf;

enum IpPortDirection {
	IpPortDirectionInput,
	IpPortDirectionOutput
};

template<typename InPortType, typename OutPortType>
class VpsimIp : public ExtraIpFeatures_if {
protected:
	typedef std::pair<InPortType*, shared_ptr<VpsimModule>> WrappedInSock;
	typedef std::tuple<OutPortType*, shared_ptr<VpsimModule>, unsigned> WrappedOutSock;

public:
	VpsimIp(std::string name)
	 : mName(name),
	   mInPortCounter(0),
	   mOutPortCounter(0)
	{
		registerRequiredAttribute("domain");
	}

	virtual ~VpsimIp() {

	}

	virtual std::string getName() { return mName; }

	virtual void setAttribute(std::string key, std::string value) {
		mAttributes[key] = value;
	}

	virtual void registerRequiredAttribute(std::string attrName) {
		mRequiredAttrs.push_back(attrName);
	}

	virtual void registerOptionalAttribute(std::string attrName, std::string defaultValue) {
		mOptionalAttrs[attrName] = defaultValue;
	}

	virtual void checkAttributes() {
		for (auto reqIter = mRequiredAttrs.begin(); reqIter != mRequiredAttrs.end(); reqIter++) {
			if (mAttributes.find(*reqIter) == mAttributes.end()) {
				throw runtime_error(*reqIter + " : Required attribute not provided !");
			}
		}
		for (auto optIter = mOptionalAttrs.begin(); optIter!=mOptionalAttrs.end(); optIter++) {
			if (mAttributes.find(optIter->first) == mAttributes.end()) {
				mAttributes[optIter->first] = optIter->second;
			}
		}

	}

	virtual uint64_t getAttrAsUInt64(std::string attrName) {
		std::string str = getAttr(attrName);
		return stoull(str);
	}


	virtual std::string getAttr(std::string attrName) {
		if (mAttributes.find(attrName) == mAttributes.end()) {
			throw runtime_error(attrName + " Getting non existing attribute.");
		}

		return mAttributes[attrName];
	}

	virtual bool isContainer() { return false; }


	virtual VpsimIp<InPortType, OutPortType>* getChild(std::string name) {
		throw runtime_error("Calling getChild() from non-container.");
	}
	virtual void addChild(VpsimIp* child) {
		throw runtime_error("Calling addChild() from non-container.");
	}
	virtual unsigned getMaxInPortCount() = 0;
	virtual unsigned getMaxOutPortCount() = 0;

	virtual InPortType* getNextInPort() = 0;
	virtual OutPortType* getNextOutPort() = 0;

	virtual InterruptIf* getIrqIf() { return nullptr; }


	// This must be called when all params have been set
	// and before any connections are made !!!
	// It will instantiate the underlying vpsim module.
	virtual void make() =0;

	virtual bool isProcessor() { return false; }
	virtual bool isInterruptController() { return false; }
	virtual bool isMemoryMapped() { return false; }
	virtual bool needsDmiAccess() { return false; }

	virtual uint64_t getBaseAddress() {
		if (!isMemoryMapped()) {
			throw runtime_error(getName() + "getBaseAddress() on non-memory-mapped object.");
		}
		return 0;
	}

	virtual uint64_t getSize() {
		if (!isMemoryMapped()) {
			throw runtime_error(getName() + "getSize() on non-memory-mapped object.");
		}
		return 0;
	}

	virtual unsigned char* getActualAddress() {
		if (!isMemoryMapped()) {
			throw runtime_error(getName() + " : Getting address of non-memory-mapped thing.");
		}
		return nullptr;
	}

	virtual bool isCached() { return false; }
	virtual bool hasDmi() { return false; }

	virtual void addDmiAddress(std::string targetIpName, uint64_t baseAddr, uint64_t size, unsigned char* pointer, bool cached, bool hasDmi) {
		if (!needsDmiAccess()) {
			throw runtime_error(getName() + " : Providing DMI address to wrong IP.");
		}
	}



	template <IpPortDirection dir, typename PortType>
	std::string addPort(std::string portAlias, std::map<std::string, PortType>& ports) {
		if (isContainer()) {
			throw runtime_error(getName() + " : Container should not be calling addPort(). Please use forwardChild(In/Out)Port().");
		}
		auto maxi = (dir==IpPortDirectionInput?getMaxInPortCount():getMaxOutPortCount());

		if (ports.size() >= maxi) {
			throw runtime_error(
					portAlias + " : Cannot add port because maximum interface size reached.");
		}

		if (portAlias == std::string("")) {
			// empty string, autogenerate alias
			char name[128];
			sprintf(name,"%s_%d", getName().c_str(), (dir==IpPortDirectionInput?mInPortCounter:mOutPortCounter));
			portAlias += name;
		}

		if (ports.find(portAlias) != ports.end()) {
			// name exists, too bad
			throw runtime_error(portAlias + " : Port alias already exists.");
		}

		return portAlias;
	}

	std::string addInPort(std::string portAlias) {
		std::string newName = addPort<IpPortDirectionInput, WrappedInSock> (portAlias, mInPorts);

		mInPorts[newName] = make_pair(getNextInPort(), getVpsimModule());
		mInPortCounter++;

		return newName;
	}

	std::string addOutPort(std::string portAlias) {
		std::string newName = addPort<IpPortDirectionOutput, WrappedOutSock>(portAlias, mOutPorts);

        if(useDmiSettings) {
            mForwarders.emplace_back(
                    (mName + "_f_" + to_string(mOutPortCounter)).c_str(),
                    getVpsimModule(),
                    mOutPortCounter
            );
            auto &forwarder = mForwarders.back();

            getNextOutPort()->bind(forwarder.socketIn());

            mOutPorts[newName] = make_tuple(&forwarder.socketOut(), getVpsimModule(), mOutPortCounter);
        } else {
            mOutPorts[newName] = make_tuple(getNextOutPort(), getVpsimModule(), mOutPortCounter);
        }
            mOutPortCounter++;

		return newName;
	}

	std::string addInPort(std::string portAlias, InPortType* inP) {
		std::string newName = addPort<IpPortDirectionInput, WrappedInSock> (portAlias, mInPorts);

		mInPorts[newName] = make_pair(inP, getVpsimModule());
		mInPortCounter++;

		return newName;
	}

	std::string addOutPort(std::string portAlias, OutPortType* outP) {
		std::string newName = addPort<IpPortDirectionOutput, WrappedOutSock>(portAlias, mOutPorts);

        if(useDmiSettings){
            mForwarders.emplace_back(
                    (mName + "_f_" + to_string(mOutPortCounter)).c_str(),
                    getVpsimModule(getName()),
                    mOutPortCounter
            );
            auto& forwarder = mForwarders.back();

            outP->bind(forwarder.socketIn());

            mOutPorts[newName] = make_tuple(&forwarder.socketOut(), getVpsimModule(), mOutPortCounter);
        } else {
            mOutPorts[newName] = make_tuple(outP, getVpsimModule(), mOutPortCounter);
        }
		mOutPortCounter++;

		return newName;
	}

	virtual WrappedInSock getInPort(std::string portAlias) {
		if (mInPorts.find(portAlias) == mInPorts.end()) {
			portAlias = addInPort(portAlias);
		}
		return mInPorts[portAlias];
	}

	virtual WrappedOutSock getOutPort(std::string portAlias) {
		if (mOutPorts.find(portAlias) == mOutPorts.end()) {
			portAlias = addOutPort(portAlias);
		}

		return mOutPorts[portAlias];
	}

	virtual sc_module* getScModule() {
		throw runtime_error(getName() + " No proper getScModule() implementation.");
	}

	virtual void forwardChildInPort(std::string childName, std::string childPortAlias, std::string myPortAlias) {
		if (!isContainer()) {
			throw runtime_error(getName() + " : Forwarding in child port from non-container.");
		}
		if (mInPorts.find(myPortAlias) != mInPorts.end()) {
			throw runtime_error(getName() + " : Alias for child port already exists.");
		}
		VpsimIp<InPortType, OutPortType>* child = getChild(childName);
		mInPorts[myPortAlias] = child->getInPort(childPortAlias);
	}

	virtual void forwardChildOutPort(std::string childName, std::string childPortAlias, std::string myPortAlias) {
		if (!isContainer()) {
			throw runtime_error(getName() + " : Forwarding out child port from non-container.");
		}
		if (mOutPorts.find(myPortAlias) != mOutPorts.end()) {
			throw runtime_error(getName() + " : Alias for child port already exists.");
		}
		VpsimIp<InPortType, OutPortType>* child = getChild(childName);
		mOutPorts[myPortAlias] = child->getOutPort(childPortAlias);
	}

	virtual int getId() { return mId; }
	virtual void setId(int id) {
		if (isIdMapped()) mId = id;
	}
	virtual bool isIdMapped() { return false; }

	virtual void connect(std::string outPortAlias, VpsimIp<InPortType, OutPortType>* otherIp, std::string inPortAlias) {
		std::cout<<"Connecting "<<getName()<<" to "<<otherIp->getName()<<std::endl;

		WrappedOutSock thisSock = getOutPort(outPortAlias);
		WrappedInSock thatSock = otherIp->getInPort(inPortAlias);

		if (std::get<1>(thisSock) != nullptr && thatSock.second != nullptr) {
			std::get<1>(thisSock)->addSuccessor(*thatSock.second, std::get<2>(thisSock));
		}
		std::get<0>(thisSock)->bind(*thatSock.first);
	}

	virtual void finalize() {
		// If you need to do anything after all initializations have been done.
	}

	virtual void addMonitor(uint64_t, uint64_t) {}
	virtual void removeMonitor(uint64_t, uint64_t) {}
	virtual void showMonitor() {}
	virtual void show() {cout<<"Your component does not implement show()."<<endl;}
	virtual void configure() {printf("This component does not implement configure()\n");}

    virtual void pushStats() {
        if(mSegmentedStats.empty()){
            mSegmentedStats.push_back({});
        }
        mSegmentedStats.push_back({});
    }

	virtual void setStatsAndDie() {
	}

	static std::map<std::string, std::function<VpsimIp<InPortType, OutPortType> *(std::string)> > RegisteredClasses;

	template<class Class>
	static void RegisterClass(std::string className) {
		RegisteredClasses.insert(
			make_pair(className,
				[](std::string name) { return dynamic_cast<VpsimIp<InPortType, OutPortType>*>(new Class(name)); } ));
	}

	static VpsimIp<InPortType, OutPortType>* NewByName(std::string className, std::string instanceName) {
		if (IsKnown(className)) {
			if (IsNameUsed(instanceName)) {
				throw runtime_error(instanceName + " : Name already used by another IP.");
			}
			auto newInstance = RegisteredClasses[className](instanceName);
			AllInstances[className].insert(make_pair(instanceName, newInstance));
			return newInstance;
		} else  {
			throw runtime_error(className + " : Class is not registered.");
		}
	}

	static bool IsKnown(std::string className) {
		return (RegisteredClasses.find(className) != RegisteredClasses.end());
	}

	static bool IsNameUsed(std::string instanceName) {
		for (auto objs = AllInstances.begin(); objs != AllInstances.end(); objs++) {
			if (objs->second.find(instanceName) != objs->second.end()) {
				return true;
			}
		}
		return false;
	}

	static VpsimIp* Find(std::string instanceName) {
		for (auto objs = AllInstances.begin(); objs != AllInstances.end(); objs++) {
			for (auto obj: objs->second) {
				if (obj.first==instanceName)
					return obj.second;
			}
		}
		return nullptr;
	}

	static std::pair<string, VpsimIp*> FindWithType(std::string instanceName) {
		for (auto objs = AllInstances.begin(); objs != AllInstances.end(); objs++) {
			for (auto obj: objs->second) {
				if (obj.first==instanceName)
					return make_pair(objs->first,obj.second);
			}
		}
		return make_pair("", nullptr);
	}

	static void MapIf(function<bool(VpsimIp<InPortType, OutPortType>*)> filterCond, function<void(VpsimIp<InPortType, OutPortType>*)> callback) {
		for (auto typeIter = AllInstances.begin(); typeIter != AllInstances.end(); typeIter++) {
			for (auto objIter = typeIter->second.begin(); objIter != typeIter->second.end(); objIter++) {
				if (!objIter->second->isContainer() && filterCond(objIter->second)) {
					callback(objIter->second);
				}
			}
		}
	}

	static void MapTypeIf(std::string type,
			function<bool(VpsimIp<InPortType, OutPortType>*)> filterCond,
			function<void(VpsimIp<InPortType, OutPortType>*)> callback) {

			auto typeIter = AllInstances.find(type);
			if (typeIter == AllInstances.end()) {
				return;
			}
			for (auto objIter = typeIter->second.begin(); objIter != typeIter->second.end(); objIter++) {
				if (filterCond(objIter->second)) {
					callback(objIter->second);
				}
			}

	}

	static void NotifyDmiAddresses() {
		MapIf([](VpsimIp<InPortType, OutPortType>* ip) { return ip->isMemoryMapped(); },
			[](VpsimIp<InPortType, OutPortType>* ip) {
				VpsimIp<InPortType, OutPortType>::MapIf([ip](VpsimIp<InPortType, OutPortType>* otherIp){return otherIp->needsDmiAccess() && otherIp->getAttr("domain")==ip->getAttr("domain");},
					[ip](VpsimIp<InPortType, OutPortType>* otherIp) {
						otherIp->addDmiAddress(ip->getName(), ip->getBaseAddress(), ip->getSize(), ip->getActualAddress(), ip->isCached(), ip->hasDmi());
					});
			});
	}

	static void Finalize() {
		for (auto typeIter = AllInstances.begin(); typeIter != AllInstances.end(); typeIter++) {
			for (auto objIter = typeIter->second.begin(); objIter != typeIter->second.end(); objIter++) {
				objIter->second->finalize();
			}
		}

        mStartTime = std::chrono::system_clock::now();
	}

	static void NotifyDmiAddresses(std::vector<std::string>& ipList) {
		MapIf([ipList](VpsimIp<InPortType, OutPortType>* ip) { return ip->isMemoryMapped() && std::find(ipList.begin(),ipList.end(),ip->getName())!=ipList.end(); },
			[](VpsimIp<InPortType, OutPortType>* ip) {
				VpsimIp<InPortType, OutPortType>::MapIf([ip](VpsimIp<InPortType, OutPortType>* otherIp){return otherIp->needsDmiAccess() && otherIp->getAttr("domain")==ip->getAttr("domain");},
					[ip](VpsimIp<InPortType, OutPortType>* otherIp) {
						otherIp->addDmiAddress(ip->getName(), ip->getBaseAddress(), ip->getSize(), ip->getActualAddress(), ip->isCached(), ip->hasDmi());
					});
			});
	}

	static void Finalize(std::vector<std::string>& ipList) {
		for (auto typeIter = AllInstances.begin(); typeIter != AllInstances.end(); typeIter++) {
			for (auto objIter = typeIter->second.begin(); objIter != typeIter->second.end(); objIter++) {
				if (std::find(ipList.begin(),ipList.end(),objIter->second->getName()) != ipList.end())
					objIter->second->finalize();
			}
		}

		mStartTime = std::chrono::system_clock::now();
	}

    static void pushStatistics() {
        for(auto& type: AllInstances){
            for(auto& ip: type.second){
                ip.second->pushStats();
            }
        }

        if(mGlobalStats.empty()){
            mGlobalStats.push_back({
                                           {"simTimeNs", "0"},
                                           {"execTimeMs", "0"}
                                   });
        }

        const auto& back = mGlobalStats.back();

        stringstream ss1, ss2;
        ss1 << sc_time_stamp();
        const auto simTimeNs = stoll(ss1.str().substr(0, ss1.str().size()-3)) - stoll(back.at("simTimeNs"));

        const auto execTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now() - mStartTime).count() - stoll(back.at("execTimeMs"));

        mGlobalStats.push_back({
                                       {"simTimeNs", to_string(simTimeNs)},
                                       {"execTimeMs", to_string(execTimeMs)}
                               });
    }

	static void WriteStat(std::string sourceName, std::string statName, std::string statValue, std::string statUnit="") {
		// for now write to global log.
		LOG_GLOBAL_STATS<<"("<<sourceName<<") "<<statName<<" "<<statValue<<" "<<statUnit<<""<<endl;
	}

	static void GatherStats() {
        /*pushStatistics();

        // Write the segmented statistics in an XML file
        QFile output("segmented_stats.xml");
        QXmlStreamWriter xml(&output);
        output.open(QIODevice::WriteOnly);
        xml.setAutoFormatting(true);
        xml.writeStartDocument();
        xml.writeStartElement("segments");

        // Get number of segments
        const auto nSegments = AllInstances.begin()->second.begin()->second->mSegmentedStats.size() - 1;
        for(size_t segIdx{1}; segIdx <= nSegments ; ++segIdx) {
            xml.writeStartElement("segment");

            // Global stats
            xml.writeStartElement("global");
            for (auto &stat: mGlobalStats[segIdx]) {
                xml.writeTextElement(stat.first.c_str(), stat.second.c_str());
            }
            xml.writeEndElement();

            for (auto &type: AllInstances) {
                for (auto &ip: type.second) {
                    if(ip.first.empty()) continue;
                    xml.writeStartElement(ip.first.c_str());
                    for (auto &stat: ip.second->mSegmentedStats[segIdx]) {
                        xml.writeTextElement(stat.first.c_str(), stat.second.c_str());
                    }
                    xml.writeEndElement();
                }
            }
            xml.writeEndElement();
        }
        xml.writeEndElement();
        xml.writeEndDocument();*/

        for (auto typeIter = AllInstances.begin(); typeIter != AllInstances.end(); typeIter++) {
            for (auto objIter = typeIter->second.begin(); objIter != typeIter->second.end(); objIter++) {
                //cout.clear(); cout<<"Collecting stats for : "<<objIter->second->getName()<<endl;
                objIter->second->setStatsAndDie();
                for (auto stat=objIter->second->mStats.begin(); stat!=objIter->second->mStats.end(); stat++) {
                    WriteStat(objIter->second->getName(), stat->first, stat->second);
                }
                //cout.clear(); cout<<"Done : "<<objIter->second->getName()<<endl;
            }
        }

        AllInstances.clear();
    }


	friend struct PlatformBuilder;


	// For quickly finding an IP
	static std::map<std::string,
		std::map<std::string, VpsimIp<InPortType,OutPortType>*> > AllInstances;

	std::vector<std::map<std::string, std::string>>&
		getSegStats() { return mSegmentedStats; }
        void clearSegStats() { mSegmentedStats.clear(); }

protected:
	std::string mName;
	unsigned mInPortCounter;
	unsigned mOutPortCounter;
	std::map<std::string, WrappedInSock > mInPorts;
	std::map<std::string, WrappedOutSock > mOutPorts;
	std::map<std::string, std::string> mAttributes;
	std::map<std::string, std::string> mOptionalAttrs;
    std::map<std::string, std::string> mStats;
    std::vector<std::map<std::string, std::string>> mSegmentedStats;
	std::vector<std::string> mRequiredAttrs;

protected:
	std::shared_ptr<VpsimModule>& getVpsimModule(){ return getVpsimModule(getName()); }
    std::shared_ptr<VpsimModule>& getVpsimModule(string name){
        if(!mVpsimModule){
            if(isMemoryMapped()){
                mVpsimModule = make_shared<VpsimModule>(
                        name,
                        moduleType::memoryMapped,
                        AddrSpace(getBaseAddress(), getBaseAddress()+getSize()-1),
                        getMaxOutPortCount());
            } else {
                mVpsimModule = make_shared<VpsimModule>(
                        name,
                        moduleType::intermediate,
                        getMaxOutPortCount());
            }
        }

        return mVpsimModule;
    }

    std::shared_ptr<VpsimModule> mVpsimModule;
    std::deque<ForwardSimpleSocket> mForwarders;

private:
    static std::vector<std::map<std::string, std::string>> mGlobalStats;
    static decltype(std::chrono::system_clock::now()) mStartTime;

    static constexpr bool useDmiSettings = false;
	int mId = -1;
};

template<typename InPortType, typename OutPortType>
class Container: public VpsimIp<InPortType, OutPortType> {
	typedef std::pair<InPortType*, shared_ptr<VpsimModule>> WrappedInSock;
	typedef std::tuple<OutPortType*, shared_ptr<VpsimModule>, unsigned> WrappedOutSock;

public:

	Container(std::string name)
	 : VpsimIp<InPortType,OutPortType>(name) {

	}

	 ~Container() {
		for (auto childIter = mChildIps.begin();
				childIter != mChildIps.end(); childIter++) {
			delete childIter->second;
		}
	}

	virtual bool isContainer() { return true; }

	virtual VpsimIp<InPortType, OutPortType>* getChild(std::string name) {
		if (mChildIps.find(name) == mChildIps.end()) {
			throw runtime_error(name + " : Child not found.");
		}
		return mChildIps[name];
	}

	virtual unsigned getMaxInPortCount() {
		return 0;
	}

	virtual unsigned getMaxOutPortCount() {
		return 0;
	}

	virtual InPortType* getNextInPort() {
		throw runtime_error(this->getName() + " : Automatically adding ports is not supported for containers.");
	}
	virtual OutPortType* getNextOutPort() {
		throw runtime_error(this->getName() + " : Automatically adding ports is not supported for containers.");
	}

	virtual WrappedInSock getInPort(std::string portAlias) {
		if (this->mInPorts.find(portAlias) == this->mInPorts.end()) {
			throw runtime_error(this->getName() + " Port not found. Have you forwarded child ports ?");
		}
		return this->mInPorts[portAlias];
	}

	virtual WrappedOutSock getOutPort(std::string portAlias) {
		if (this->mOutPorts.find(portAlias) == this->mOutPorts.end()) {
			throw runtime_error(this->getName() + " Port not found. Have you forwarded child ports ?");
		}
		return this->mOutPorts[portAlias];
	}

	std::string addInPort(std::string portAlias) {
		throw runtime_error(this->getName() + " : Automatically adding ports is not supported for containers.");
	}

	std::string addOutPort(std::string portAlias) {
		throw runtime_error(this->getName() + " : Automatically adding ports is not supported for containers.");
	}

	virtual void addChild(VpsimIp<InPortType, OutPortType>* child) {
		if (mChildIps.find(child->getName()) != mChildIps.end()) {
			throw runtime_error(child->getName() + " : Child already exists.");
		}
		mChildIps[child->getName()] = child;
	}



	virtual void make() { // do nothing
	}


private:
	std::map<std::string, VpsimIp<InPortType, OutPortType>*> mChildIps;
};


template<typename InPortType, typename OutPortType>
std::map<std::string, std::function<VpsimIp<InPortType, OutPortType> *(std::string)> > VpsimIp<InPortType,OutPortType> :: RegisteredClasses;

template<typename InPortType, typename OutPortType>
 std::map<std::string,
		std::map<std::string, VpsimIp<InPortType,OutPortType>*> > VpsimIp<InPortType, OutPortType>::AllInstances;

template<typename InPortType, typename OutPortType>
std::vector<std::map<std::string, std::string>> VpsimIp<InPortType, OutPortType>::mGlobalStats;

template<typename InPortType, typename OutPortType>
decltype(std::chrono::system_clock::now()) VpsimIp<InPortType, OutPortType>::mStartTime;

/* Helper Macros */
#define MEMORY_MAPPED virtual bool isMemoryMapped() { return true; }
#define MEMORY_MAPPED_OVERRIDE virtual bool isMemoryMapped() override { return true; }

#define NEEDS_DMI virtual bool needsDmiAccess() { return true; }
#define NEEDS_DMI_OVERRIDE virtual bool needsDmiAccess() override { return true; }

#define CACHED virtual bool isCached() { return true; }
#define CACHED_OVERRIDE virtual bool isCached() override { return true; }

#define PROCESSOR virtual bool isProcessor() { return true; }
#define PROCESSOR_OVERRIDE virtual bool isProcessor() override { return true; }

#define N_IN_PORTS(n) virtual unsigned getMaxInPortCount() { return n; }
#define N_IN_PORTS_OVERRIDE(n) virtual unsigned getMaxInPortCount() override { return n; }
#define N_OUT_PORTS(n) virtual unsigned getMaxOutPortCount() { return n; }
#define N_OUT_PORTS_OVERRIDE(n) virtual unsigned getMaxOutPortCount() override { return n; }

#define HAS_DMI virtual bool hasDmi() { return true; }
#define HAS_DMI_OVERRIDE virtual bool hasDmi() override { return true; }

#define INTERRUPT_CONTROLLER virtual bool isInterruptController() { return true;}
#define INTERRUPT_CONTROLLER_OVERRIDE virtual bool isInterruptController() override { return true;}

#define ID_MAPPED virtual bool isIdMapped() { return true; }
#define ID_MAPPED_OVERRIDE virtual bool isIdMapped() override { return true; }

}

#endif /* _VPSIMIP_H_ */

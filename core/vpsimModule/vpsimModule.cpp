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

#include <functional>
#include <utility>
#include <algorithm>
#include "vpsimModule.hpp"
#include "paramManager.hpp"

using namespace vpsim;

VpsimModule::VpsimModule(string name, moduleType type, const AddrSpace& as, size_t nbPort):
	mSuccessors(nbPort),
	mEffectiveParameters(nbPort),
	mModifier(new ParameterSetModifier()), //Default modifier does nothing
	mOwnAddrSpace(as),
	mModuleType(type),
	mNbOutPorts(nbPort),
	mName(name)
{
	ParamManager::get().registerModule(*this);
}

VpsimModule::VpsimModule(string name, moduleType type, size_t nbPort):
	VpsimModule(name, type, AddrSpace::maxRange, nbPort)
{}

VpsimModule::~VpsimModule()
{
	ParamManager::get().unregisterModule(mName);
}

//Argument is not const because it is meant to be pushed in a vector of non const references
//If it was const, it would have to be const_cast'ed to match the constructor reference_wrapper(VpsimModule&)
void VpsimModule::addPredecessor(VpsimModule& p)
{
	//If this predecessor has already been registered, return
	for(auto pred: mPredecessors){
		if(pred == &p) return;
	}
	mPredecessors.push_back(&p);
}

void VpsimModule::addSuccessor(VpsimModule& s, size_t port)
{
	if(port >= mNbOutPorts){
		throw(range_error("Trying to add a successor to an non existing port"));
	}
	//Impossible to bind two modules on the same port
	if(mSuccessors[port]){
		throw invalid_argument(string("Trying to bind two modules on the same port"));
	}

	mSuccessors[port] = &s;
	s.addPredecessor(*this);

	refreshParameters();
}

set<AddrSpace> VpsimModule::getAllowedSpaces(constVpsimModuleList &&called) const
{
	called.push_back(this);

    set<AddrSpace> allowedSpaces;
	for(auto s: mSuccessors){
        if(s && find(begin(called), end(called), s) == end(called)){
            const auto& sas = s->getAllowedSpaces(forward<decltype(called)>(called));
            allowedSpaces.insert(sas.begin(), sas.end());
        }
	}

	//Only memory mapped modules can extend the allowed parameter space. The other modules strictly inherit it
	if(mModuleType == moduleType::memoryMapped){
        allowedSpaces.insert(mOwnAddrSpace);
	}

	//Update the exported allowed spaces
    return (*mModifier)(allowedSpaces);
}


void VpsimModule::addParameterSetModifier(const ParameterSetModifier& modifier)
{
	mModifier->attach(*modifier.clone());
    refreshParameters();
}


void VpsimModule::refreshPredecessors()
{
	for(auto pred: mPredecessors){
        pred->refreshParameters(false);
	}
}

void VpsimModule::clearPredecessors(VpsimModule::constVpsimModuleList &&called) {
    called.push_back(this);
    for(auto pred: mPredecessors){
        if(pred && find(begin(called), end(called), pred) == end(called)){
            pred->mExportedParameters = ParameterSet();
            pred->clearPredecessors(forward<decltype(called)>(called));
        }
    }
}


void VpsimModule::refreshParameters(bool clear)
{
    ParameterSet newExportedParam;

    //Clear the exported parameters and update the predecessors a first time
    //It is mandatory on the initial call in case of a loop
    if(clear){
        clearPredecessors();
    }

    for(size_t i{0} ; i < mNbOutPorts ; ++i){
        if(mSuccessors[i]){
            mEffectiveParameters[i] = mSuccessors[i]->mExportedParameters;
        }
    }

    for(auto& f: mUpdateHooks){
        f();
    }

    //The exported Parameter set is the agreggation of the successors' parameter set with the intrinsic one
	for(auto& ep: mEffectiveParameters){
        newExportedParam.mergeImportedParam(ep);
	}

	//If the type of the module is memoryMapped, then use the ParameterSet addition
	//else, if it is intermediate or dummy, use the redefine bitwise and
	mIntrinsecParameters.trim(getAllowedSpaces());
    newExportedParam.addExportedParam(mIntrinsecParameters);

	//Apply the modifier
	//expParam is modified in place
	(*mModifier)(newExportedParam);

	//If the new exported Parameter set is different from the old one, refresh the predecessors of the module
	if(newExportedParam != mExportedParameters){
        mExportedParameters = move(newExportedParam);
		refreshPredecessors();
	}
}


void VpsimModule::setParameter(AddrSpace as,  ModuleParameter const & param)
{
	mIntrinsecParameters.setParameter(as, param);
    refreshParameters();
}


void VpsimModule::setParameter(ModuleParameter const & param)
{
	setParameter(AddrSpace::maxRange, param);
}


BlockingTLMEnabledParameter VpsimModule::getBlockingTLMEnabled(uint64_t addr) const
{
	return getBlockingTLMEnabled(0, addr);
}

BlockingTLMEnabledParameter VpsimModule::getBlockingTLMEnabled(size_t port, uint64_t addr) const
{
	return mEffectiveParameters[port].getBlockingTLMEnabledParameter(addr);
}

BlockingTLMEnabledParameter VpsimModule::getBlockingTLMEnabled(size_t port, AddrSpace addr) const
{
	return mEffectiveParameters[port].getBlockingTLMEnabledParameter(addr);
}

ApproximateDelayParameter VpsimModule::getApproximateDelay(uint64_t addr) const
{
	return getApproximateDelay(0, addr);
}

ApproximateDelayParameter VpsimModule::getApproximateDelay(size_t port, uint64_t addr) const
{
	return mEffectiveParameters[port].getApproximateDelayParameter(addr);
}

void VpsimModule::registerUpdateHook(function<void()> &&hook) {
    mUpdateHooks.emplace_back(move(hook));
}


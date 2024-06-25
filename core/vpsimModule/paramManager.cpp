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

#include <memory>
#include <systemc>
#include "paramManager.hpp"
#include "vpsimModule.hpp"


namespace vpsim{

ParamManager::ParamManager():
	mParamScheduler("paramScheduler")
{}

ParamManager& ParamManager::get()
{
	//Instance of the ParamManager
	static std::unique_ptr<ParamManager> instance(new ParamManager());

	return *instance;
}

void ParamManager::setParameter(string module, AddrSpace as, const ModuleParameter& param)
{
	mVpsimModules.at(module)->setParameter(as, param);
	callParamUpdateHandlers();
}


void ParamManager::setParameter(string module, const ModuleParameter& param)
{
	mVpsimModules.at(module)->setParameter(param);
	callParamUpdateHandlers();
}


void ParamManager::addAppointment(string module,
								  AddrSpace as,
								  sc_core::sc_time date,
								  const ModuleParameter& param)
{
	mParamScheduler.addAppointment(ParamAppointment(module, as, date, param));
}


void ParamManager::addAppointment(string module,
                    			  sc_core::sc_time date,
								  const ModuleParameter& param)
{
	mParamScheduler.addAppointment(ParamAppointment(module, date, param));
}

void ParamManager::registerModule(VpsimModule& module)
{
	auto alreadyReg = !mVpsimModules.insert({module.mName, &module}).second;
	if(alreadyReg){
		throw invalid_argument("Trying to register a second module with the same name.");
	}
}

void ParamManager::unregisterModule(string name)
{
	mVpsimModules.erase(name);
	mUpdateHandlers.erase(name);
}

void ParamManager::registerUpdateHook(std::string name, function<void()> &&handler) {
    mUpdateHandlers.erase(name);
    mUpdateHandlers.emplace(name, move(handler));
}

void ParamManager::callParamUpdateHandlers() {
    for(auto& handler: mUpdateHandlers){
        handler.second();
    }
}

}

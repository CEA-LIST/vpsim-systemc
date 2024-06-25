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

#include <systemc>
#include "paramManager.hpp"

using namespace std;

namespace vpsim{

ParamAppointment::ParamAppointment(string module,
								   AddrSpace as,
								   sc_core::sc_time date,
								   const ModuleParameter &param):
	mModule(module),
	mAddrSpace(as),
	mParam(param.clone()),
	mDate(date),
	mUseDefaultAs(false)
{}

ParamAppointment::ParamAppointment(string module,
								   sc_core::sc_time date,
								   const ModuleParameter &param):
	mModule(module),
	mParam(param.clone()),
	mDate(date),
	mUseDefaultAs(true)
{}


bool ParamAppointment::isPassed() const
{
	return mDate < sc_core::sc_time_stamp();
}


bool ParamAppointment::isNow() const {
	return mDate == sc_core::sc_time_stamp();
}


sc_core::sc_time ParamAppointment::timeTo() const
{
	return mDate - sc_core::sc_time_stamp();
}

void ParamAppointment::apply() const
{
	if(mUseDefaultAs){
		ParamManager::get().setParameter(mModule, *mParam);
	} else {
		ParamManager::get().setParameter(mModule, mAddrSpace, *mParam);
	}
}

bool ParamAppointment::operator<(const ParamAppointment& that) const
{
	return mDate < that.mDate;
}

}

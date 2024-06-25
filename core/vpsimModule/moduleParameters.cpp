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

#include <utility>
#include <algorithm>
#include <paramManager.hpp>
#include "GlobalPrivate.hpp"
#include "moduleParameters.hpp"

using namespace vpsim;
using namespace std;
using namespace sc_core;

//Static initialization
BlockingTLMEnabledParameter::value BlockingTLMEnabledParameter::mDefaultValue =
	BlockingTLMEnabledParameter::BT_ENABLED;
const BlockingTLMEnabledParameter BlockingTLMEnabledParameter::bt_enabled =
	BlockingTLMEnabledParameter(BlockingTLMEnabledParameter::BT_ENABLED);
const BlockingTLMEnabledParameter BlockingTLMEnabledParameter::bt_disabled =
	BlockingTLMEnabledParameter(BlockingTLMEnabledParameter::BT_DISABLED);


sc_time ApproximateDelayParameter::mDefaultDelay = SC_ZERO_TIME;


ModuleParameter::~ModuleParameter()
{}


/////////////////////////////////////////////////////////////////////////////////////////
//BlockingTLMEnabledParameter

BlockingTLMEnabledParameter::BlockingTLMEnabledParameter(value val):
	mValue(val)
{}

BlockingTLMEnabledParameter::BlockingTLMEnabledParameter(bool b):
	mValue(b ? BT_ENABLED : BT_DISABLED)
{}


unique_ptr<ModuleParameter> BlockingTLMEnabledParameter::clone() const
{
	return unique_ptr<ModuleParameter>(new BlockingTLMEnabledParameter(*this));
}

BlockingTLMEnabledParameter::value BlockingTLMEnabledParameter::get() const
{
	return mValue;
}

BlockingTLMEnabledParameter::operator bool() const
{
	return static_cast<bool>(get());
}


void BlockingTLMEnabledParameter::setDefault(value v)
{
	mDefaultValue = v;
}


bool BlockingTLMEnabledParameter::operator<(const ModuleParameter& that) const
{
	//dynamic_cast used instead of static_cast for safety. Can be replaced by a static_cast for speed.
	//A dynamic_cast to a reference throws in case of failure. No need for epxlicit if checking.
	auto t = dynamic_cast<const BlockingTLMEnabledParameter&>(that);
	return (this->mValue < t.mValue);
}


bool BlockingTLMEnabledParameter::operator==(const ModuleParameter& that) const
{
	auto t = dynamic_cast<const BlockingTLMEnabledParameter&>(that);
	return (this->mValue == t.mValue);
}

ModuleParameter& BlockingTLMEnabledParameter::operator+=(const ModuleParameter& that)
{
	auto t = dynamic_cast<const BlockingTLMEnabledParameter&>(that);
	this->mValue = max(*this, t).mValue;
	return *this;
}


unique_ptr<ModuleParameter> BlockingTLMEnabledParameter::operator+(const ModuleParameter& that) const
{
	auto t = dynamic_cast<const BlockingTLMEnabledParameter&>(that);
	return max(*this, t).clone();
}


/////////////////////////////////////////////////////////////////////////////////////////
//ApproximateDelayParameter


ApproximateDelayParameter::ApproximateDelayParameter(sc_time delay):
	mDelay(delay)
{}


unique_ptr<ModuleParameter> ApproximateDelayParameter::clone() const
{
	return unique_ptr<ModuleParameter>(new ApproximateDelayParameter(*this));
}

sc_time ApproximateDelayParameter::get() const
{
	return mDelay;
}

ApproximateDelayParameter::operator sc_core::sc_time() const
{
	return get();
}


void ApproximateDelayParameter::setDefault(sc_time delay)
{
	mDefaultDelay = delay;
	ParamManager::get().callParamUpdateHandlers();
}


bool ApproximateDelayParameter::operator<(const ModuleParameter& that) const
{
	//dynamic_cast used instead of static_cast for safety. Can be replaced by a static_cast for speed.
	//A dynamic_cast to a reference throws in case of failure. No need for epxlicit if checking.
	auto t = dynamic_cast<const ApproximateDelayParameter&>(that);
	return (this->mDelay > t.mDelay);
}


bool ApproximateDelayParameter::operator==(const ModuleParameter& that) const
{
	auto t = dynamic_cast<const ApproximateDelayParameter&>(that);
	return (this->mDelay == t.mDelay);
}

ModuleParameter& ApproximateDelayParameter::operator+=(const ModuleParameter& that)
{
    auto t = dynamic_cast<const ApproximateDelayParameter&>(that);
    this->mDelay += t.mDelay;
    return *this;
}

unique_ptr<ModuleParameter> ApproximateDelayParameter::operator+(const ModuleParameter& that) const
{
	auto t = dynamic_cast<const ApproximateDelayParameter&>(that);
	return unique_ptr<ModuleParameter>(new ApproximateDelayParameter(mDelay + t.mDelay));
}


//////////////////////////////////////////////////////////////////////////////////
//ApproximateTraversalRateParameter

bool ApproximateTraversalRateParameter::operator<(const ModuleParameter& that) const
{
	auto t = dynamic_cast<const ApproximateTraversalRateParameter&>(that);
	return mRate < t.mRate;
}


bool ApproximateTraversalRateParameter::operator==(const ModuleParameter& that) const
{
	auto t = dynamic_cast<const ApproximateTraversalRateParameter&>(that);
	return mRate == t.mRate;
}

ModuleParameter& ApproximateTraversalRateParameter::operator+=(const ModuleParameter& that)
{
    throw(string("operator += should not be used on ApproximateTraversalRateParameter as it does not propagate"));
}

std::unique_ptr<ModuleParameter> ApproximateTraversalRateParameter::operator+(const ModuleParameter&) const
{
	throw(string("operator + should not be used on ApproximateTraversalRateParameter as it does not propagate"));
}


double ApproximateTraversalRateParameter::get() const
{
	return mRate;
}

ApproximateTraversalRateParameter::operator double() const
{
	return get();
}


std::unique_ptr<ModuleParameter> ApproximateTraversalRateParameter::clone() const
{
	return unique_ptr<ModuleParameter>(new ApproximateTraversalRateParameter(*this));
}


ApproximateTraversalRateParameter::ApproximateTraversalRateParameter(double rate):
	mRate(rate)
{}


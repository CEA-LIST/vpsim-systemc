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

#include "parameterSetModifier.hpp"
#include "AddrSpace.hpp"
#include "parameterSet.hpp"


using namespace vpsim;
using namespace std;

ParameterSetModifier::~ParameterSetModifier()
{}

ParameterSet& ParameterSetModifier::applyNonRecursive(ParameterSet& ps) const
{
	return ps;
}

set<AddrSpace> ParameterSetModifier::applyNonRecursive(const set<AddrSpace>& asSet) const
{
	return set<AddrSpace>(asSet);
}

ParameterSet& ParameterSetModifier::apply(ParameterSet& ps) const
{
	//Calling the modifiers in reverse order is coherent with the order in which they are added
	auto prev = mPreviousModifier.get();
	if(prev){
		prev->apply(ps);
	}

	return applyNonRecursive(ps);
}

set<AddrSpace> ParameterSetModifier::apply(const set<AddrSpace>& asSet) const
{
	auto prev = mPreviousModifier.get();
	if(prev){
		return prev->apply(applyNonRecursive(asSet));
	} else {
		return applyNonRecursive(asSet);
	}
}

ParameterSetModifier& ParameterSetModifier::attach(const ParameterSetModifier& ps)
{
	if(mPreviousModifier.get()){
		mPreviousModifier->attach(ps);
	} else {
		mPreviousModifier = ps.clone();
	}
	return *this;
}

std::unique_ptr<ParameterSetModifier> ParameterSetModifier::clone() const
{
	return unique_ptr<ParameterSetModifier>(new ParameterSetModifier(*this));
}

ParameterSetAddressTranslator::ParameterSetAddressTranslator
	(uint64_t offset, bool positive, const ParameterSetModifier& previous):
		mOffset(offset),
		mPositiveOffset(positive)
{
	attach(previous);
}

ParameterSetAddressTranslator::ParameterSetAddressTranslator
	(uint64_t offset, bool positive):
		mOffset(offset),
		mPositiveOffset(positive)
{}

ParameterSet& ParameterSetAddressTranslator::applyNonRecursive(ParameterSet& ps) const
{

	auto translate = [this](paramContainer& ps){
		//Create a new map with the same comparison function as pm without copying its content
		paramContainer newContainer;

		//Fill the new map
		for(auto p = ps.begin(); p != ps.end() ; ++p)
		{
			uint64_t newBase{mPositiveOffset ? p->first.getBaseAddress() - mOffset :
											   p->first.getBaseAddress() + mOffset};
			uint64_t newEnd{mPositiveOffset ? p->first.getEndAddress() - mOffset :
					   	    				  p->first.getEndAddress() + mOffset};
			//Unsigned capacity overflow is a well defined behaviour.
			//This is safe to detect invalid translation this way.
			if((mPositiveOffset && newBase > p->first.getBaseAddress())  ||
			   (!mPositiveOffset && newBase < p->first.getBaseAddress()) ||
			   (mPositiveOffset && newEnd > p->first.getEndAddress()) ||
			   (!mPositiveOffset && newEnd < p->first.getEndAddress())){
				//TODO throw
				throw overflow_error("ERROR: Translation caused an overflow.");
			}
			newContainer.emplace_back(AddrSpace(newBase, newEnd), p->second->clone());
		}

		ps = move(newContainer);
	};

	//Apply the translation on every map in the parameterSet
	translate(ps.mBlockingTLMEnabledParameter);
	translate(ps.mApproximateDelayParameter);
	translate(ps.mApproximateTraversalRateParameter);

	return ps;
}


set<AddrSpace> ParameterSetAddressTranslator::applyNonRecursive(const set<AddrSpace>& asSet) const
{
	set<AddrSpace> newAsSet;

	//Fill the new map
	for(auto p: asSet)
	{
		uint64_t newBase{mPositiveOffset ? p.getBaseAddress() - mOffset :
										   p.getBaseAddress() + mOffset};
		uint64_t newEnd{mPositiveOffset ? p.getEndAddress() - mOffset :
										  p.getEndAddress() + mOffset};
		//Unsigned capacity overflow is a well defined behaviour.
		//This is safe to detect invalid translation this way.
		if((mPositiveOffset && newBase > p.getBaseAddress())  ||
		   (!mPositiveOffset && newBase < p.getBaseAddress()) ||
		   (mPositiveOffset && newEnd > p.getEndAddress()) ||
		   (!mPositiveOffset && newEnd < p.getEndAddress())){
			//TODO throw
			throw overflow_error("ERROR: Translation caused an overflow.");
		}
		newAsSet.emplace(newBase, newEnd);
	}

	return newAsSet;
}


unique_ptr<ParameterSetModifier> ParameterSetAddressTranslator::clone() const
{
	return unique_ptr<ParameterSetModifier>(new ParameterSetAddressTranslator(*this));
}

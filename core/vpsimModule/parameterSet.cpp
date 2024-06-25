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
#include <algorithm>
#include "parameterSet.hpp"

using namespace vpsim;
using namespace std;

template<class T>
T ParameterSet::getParameterValue(const paramContainer& pc, uint64_t addr) const
{
	static_assert(!is_same<T,ApproximateTraversalRateParameter>::value,
		"ApproximateTraversalRateParameter is for internal use only");

	for(auto& p: pc)
	{
		if((p.first.getEndAddress() >= addr) &&
		   (p.first.getBaseAddress() <= addr)){
			//1)get the raw pointer to the parameter
			//2)cast it to its real underlying type (replace with dynamic_cast if not sure) respecting the const qualification
			//3)get the value for the return
			return T(static_cast<const T&>(*p.second));
		}
	}

	return T();
}

BlockingTLMEnabledParameter ParameterSet::getParameterValForAddrSpace(const paramContainer& pc, AddrSpace addr) const
{
//	static size_t i{0};
//	if(i++ % 1000 == 0)
//		cout << "ParameterSet::getParameterValForAddrSpace " << i << "\n" ;
//
//    BlockingTLMEnabledParameter accumulator(false) ;
//    for(const auto& p: pc){
//        if(addr.intersect(p.first)){
//            accumulator += *p.second;
//            if(!accumulator){
//                for(const auto& as: addr.relativeComplement(p.first)){
//                    accumulator += getParameterValForAddrSpace(pc, as);
//                }
//            }
//            //Create a copy before the memory manager by the unique_ptr is deleted
//            return accumulator;
//        }
//        if(addr.getEndAddress() < p.first.getBaseAddress()){
//            break;
//        }
//    }
//
//    return BlockingTLMEnabledParameter();

///////////////////////////////////////////////

	static const function<BlockingTLMEnabledParameter(const paramContainer&, paramContainer::const_iterator, AddrSpace)>
			helper = [](const paramContainer& pc, paramContainer::const_iterator begin,
						 AddrSpace as) -> BlockingTLMEnabledParameter
		{
            BlockingTLMEnabledParameter accumulator(false) ;
			for(auto p = begin ; p != pc.cend() ; ++p){
				if(as.intersect(p->first)){
                    accumulator += *p->second;
					if(!accumulator){
						for(const auto& otherAs: as.relativeComplement(p->first)){
							accumulator += helper(pc, ++p, otherAs);
						}
					}
					return accumulator;
				}
				if(as.getEndAddress() < p->first.getBaseAddress()){
					break;
				}
			}
			return BlockingTLMEnabledParameter();
		};

	return helper(pc, pc.cbegin(), addr);
}

void ParameterSet::setParameter(AddrSpace as, const ModuleParameter& param)
{
	auto pick = [](const ModuleParameter&, const ModuleParameter& newMp)
				->unique_ptr<ModuleParameter>{
					return newMp.clone();
	};

	//Dynamically detect the underlying type of the parameter
	//This is safe to deduce the last type as long as your stay exhaustive
	paramContainer& pc =
			typeid(param) == typeid(BlockingTLMEnabledParameter) ? mBlockingTLMEnabledParameter :
			typeid(param) == typeid(ApproximateDelayParameter) ? mApproximateDelayParameter :
			mApproximateTraversalRateParameter;

	setParameterHelper(pc, as, param, pick);
}

BlockingTLMEnabledParameter ParameterSet::getBlockingTLMEnabledParameter(uint64_t addr) const
{
	return getParameterValue<BlockingTLMEnabledParameter>(mBlockingTLMEnabledParameter, addr);
}

BlockingTLMEnabledParameter ParameterSet::getBlockingTLMEnabledParameter(AddrSpace addr) const
{
	return getParameterValForAddrSpace(mBlockingTLMEnabledParameter, addr);
}


ApproximateDelayParameter ParameterSet::getApproximateDelayParameter(uint64_t addr) const
{
	return getParameterValue<ApproximateDelayParameter>(mApproximateDelayParameter, addr);
}




void ParameterSet::setParameterHelper(paramContainer& pc,
									  AddrSpace as,
									  const ModuleParameter& param,
									  const function<unique_ptr<ModuleParameter>(const ModuleParameter&,
											  	  	  	  	  	  	  	  	  	 const ModuleParameter&)>& pick)
{
	bool conflict{false};

	//Look for conflicts first
	for(auto p = pc.begin() ; p != pc.end() ; ++p)
	{
		const AddrSpace curAs = p->first;
        const auto& curParam = p->second->clone();

        if(curAs.intersect(as)){
            conflict = true;

            //Compute the interesting address spaces:
            //The conflicting range if any (intersection)
            //The ranges with only the already existing parameter (oldSpaces)
            //The ranges with only the new parameter (newSpaces)
            //
            //Treat them depending on the emplacement policy

            AddrSpace intersection = curAs.intersection(as);
            auto oldSpaces = curAs.relativeComplement(as);
            auto newSpaces = as.relativeComplement(curAs);

            //Remove the conflicting space
            pc.erase(p);

            //Put the new spaces
            for(auto& nas: newSpaces){
                setParameterHelper(pc, nas, param, pick);
            }

            setParameterHelper(pc, intersection, *pick(*curParam, param), pick);

            //Always put back the old non conflicting spaces
            for(const auto& oas: oldSpaces){
                //Not conflicting so always set the new value
                setParameterHelper(pc, oas, *curParam, pick);
			}

			break;
		}
	}

	if(!conflict){
		pc.emplace_back(as, param.clone());
		sort(pc.begin(), pc.end());
	}
}


ParameterSet::ParameterSet()
{}

ParameterSet::ParameterSet(const ParameterSet& that)
{
    *this = that;
}

ParameterSet& ParameterSet::operator=(const ParameterSet& that)
{
    auto copy = [](const paramContainer& from, paramContainer& to){
        to.clear();
        for(const auto& f: from){
            to.emplace_back(f.first, f.second->clone());
        }
    };

    copy(that.mBlockingTLMEnabledParameter, mBlockingTLMEnabledParameter);
    copy(that.mApproximateDelayParameter, mApproximateDelayParameter);
    copy(that.mApproximateTraversalRateParameter, mApproximateTraversalRateParameter);

    return *this;
}


ParameterSet& ParameterSet::mergeImportedParam(const ParameterSet& importedParam)
{
	auto pick = [](const ModuleParameter& oldMp, const ModuleParameter& newMp)
				->unique_ptr<ModuleParameter> {
		return max(oldMp, newMp).clone();
	};

	auto helper = [&](const paramContainer& from, paramContainer& to){
		for(auto& p: from){
			setParameterHelper(to, p.first, *p.second, pick);
		}
		defrag(to);
	};

	helper(importedParam.mBlockingTLMEnabledParameter, mBlockingTLMEnabledParameter);
	helper(importedParam.mApproximateDelayParameter, mApproximateDelayParameter);
	//The traversal rate does not propagate

	return *this;
}


ParameterSet& ParameterSet::addExportedParam(const ParameterSet& exportedParam)
{
	auto pick = [](const ModuleParameter& oldMp, const ModuleParameter& newMp)
				->unique_ptr<ModuleParameter> {
		return oldMp + newMp;
	};

	auto helper = [&](const paramContainer& from, paramContainer& to){
		for(auto& p: from){
			setParameterHelper(to, p.first, *p.second, pick);
		}
		defrag(to);
	};

	helper(exportedParam.mBlockingTLMEnabledParameter, mBlockingTLMEnabledParameter);

	/////////////////////////////////////////////////////////////////////////////////////
	//Traversal rates gestion
	//Set the new delay taking the traversal rates into account
	paramContainer newDelays;

	function<void(AddrSpace, const ModuleParameter&)>
		newDelaysHelper = [&exportedParam, &newDelays, &newDelaysHelper]
						  (AddrSpace as, const ModuleParameter& delay){

		auto& d = dynamic_cast<const ApproximateDelayParameter&>(delay);
		auto& trRates = exportedParam.mApproximateTraversalRateParameter;

		for(auto& trRate: trRates){
			const AddrSpace& trAs = trRate.first;
			auto& tr = dynamic_cast<const ApproximateTraversalRateParameter&>(*trRate.second);

			if(as.intersect(trAs)){
				AddrSpace intersection = as.intersection(trAs);
				auto otherAs = as.relativeComplement(trAs);

				//Set the intersection
				newDelays.emplace_back(intersection, ApproximateDelayParameter(d * tr).clone());
				//Set the other AddrSpaces reccursively
				for(auto oas: otherAs){
					newDelaysHelper(oas, delay);
				}
				return;
			}
		}
		//No intersection so simply put the delay
		newDelays.emplace_back(as, delay.clone());
	};

	for(auto& d: mApproximateDelayParameter){
		newDelaysHelper(d.first, *d.second);
	}
	sort(newDelays.begin(), newDelays.end());
	defrag(newDelays);
	mApproximateDelayParameter = move(newDelays);
	/////////////////////////////////////////////////////////////////////////////////////////

	helper(exportedParam.mApproximateDelayParameter, mApproximateDelayParameter);

	return *this;
}


void ParameterSet::trim(const set<AddrSpace>& restriction)
{
	auto helper = [&](paramContainer& pc){
		paramContainer newPc;
		for(auto& p: pc){
			for(auto& as: restriction){
				if(p.first.intersect(as)){
					newPc.emplace_back(p.first.intersection(as), p.second->clone());
				} else if(as.getBaseAddress() > p.first.getEndAddress()){
					break;
				}
			}
		}
		sort(newPc.begin(), newPc.end());
		//Should not be necessary, but it is cheap;
		defrag(newPc);
		pc = move(newPc);
	};

	helper(mBlockingTLMEnabledParameter);
	helper(mApproximateDelayParameter);
	helper(mApproximateTraversalRateParameter);
}

void ParameterSet::defrag(paramContainer& pc)
{
	//The maps containing the parameters migh be frangmented,
	//i.e {[a,b] => val} might be stored as {[a,c] => val ; [c+1, b] => val}
	//Defragmenting the maps brings back the unicity of the representation of a parameter set

	//As we are using an ordered map with the guarantee that, by construction, no key are conflicting,
	//it is easy to merge the elements that can be as they are contiguously stored

	if(pc.size() > 1){
		for(auto it1 = pc.begin(), it2 = ++pc.begin();
			it2 !=  pc.end();
			++it1, ++it2){

			const AddrSpace &as1 = it1->first,
							&as2 = it2->first;
			const ModuleParameter &p1 = *it1->second,
								  &p2 = *it2->second;

			if((as1.getEndAddress() + 1 == as2.getBaseAddress()) && p1 == p2){
				AddrSpace newAs(as1.getBaseAddress(), as2.getEndAddress());

				//Save the parameter before erasing it
				auto p1s = p1.clone();

				pc.erase(it2);
				pc.erase(it1);

				pc.emplace(it1, newAs, move(p1s));

				//Possible optimization:
				//body of the loop in do while
				//while condition is "2 elements have been merged" (i.e. reaching this line)
				//reassign it1 and it2 to the new element and the next one respectively

				//Recursive call is convenient but it adds some extra work
				//it shouldn't be noticeable though...
				defrag(pc);
				return;
			}
		}
	}
}


bool ParameterSet::operator==(const ParameterSet& that) const
{
	//Two parameter sets are considered equivalent if for any address, the parameters returned by both are the same
	//In a defragmented map, it is equivalent to test strict equality between the maps content (keys and values).

	auto equalHelper = [](const paramContainer& a, const paramContainer& b) -> bool {
		if(a.size() != b.size()){
			return false;
		}

		for(auto it1 = a.begin(), it2 = b.begin();
			it1 != a.end(); //sufficient because same size
			++it1, ++it2){

			const AddrSpace &as1 = it1->first,
							&as2 = it2->first;
			const ModuleParameter &p1 = *it1->second,
								  &p2 = *it2->second;

			if(as1 != as2 || p1 != p2){
				return false;
			}
		}

		return true;
	};

	return equalHelper(mBlockingTLMEnabledParameter, that.mBlockingTLMEnabledParameter) &&
		   equalHelper(mApproximateDelayParameter, that.mApproximateDelayParameter) &&
		   equalHelper(mApproximateTraversalRateParameter, that.mApproximateTraversalRateParameter);
}

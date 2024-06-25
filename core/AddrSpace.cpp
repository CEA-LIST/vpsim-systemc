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

#include <algorithm>
#include "AddrSpace.hpp"

#include <iostream>

using namespace std;

namespace vpsim {

const AddrSpace AddrSpace::maxRange = AddrSpace(0x0, -0x1);

AddrSpace::AddrSpace(uint64_t Size):
	mBaseAddress(0x0),
	mEndAddress(Size)
{}

AddrSpace::AddrSpace(uint64_t base, uint64_t end):
	mBaseAddress(base),
	mEndAddress(end)
{}

AddrSpace::~AddrSpace() {}



void AddrSpace::setSize(uint64_t Size) {
	mEndAddress = mBaseAddress + Size - 1;

	if(mBaseAddress + Size < mEndAddress){
		throw overflow_error("setSize caused an overflow");
	}
}

void AddrSpace::setBaseAddress (uint64_t BaseAddress) {
	//Works with unsigned
	uint64_t shift{BaseAddress - mBaseAddress};
	mBaseAddress = BaseAddress;
	mEndAddress = mEndAddress + shift;
}

void AddrSpace::setEndAddress(uint64_t endAddress)
{
	mEndAddress = endAddress;
}



uint64_t AddrSpace::getSize() const {
	if(mBaseAddress == 0x0 && mEndAddress == 0xffffffffffffffff){
		throw overflow_error("Size can not be stored in a uint64_t");
	}
	return(mEndAddress - mBaseAddress + 1);
}

uint64_t AddrSpace::getBaseAddress() const { return(mBaseAddress); }

uint64_t AddrSpace::getEndAddress() const { return(mEndAddress); }

bool AddrSpace::operator==(const AddrSpace& that) const
{
	return (this->mBaseAddress == that.mBaseAddress &&
			this->mEndAddress == that.mEndAddress);
}

bool AddrSpace::operator<(const AddrSpace& that) const
{
	return (mBaseAddress < that.mBaseAddress) ||
		   (mBaseAddress == that.mBaseAddress && mEndAddress < that.mEndAddress);
}

bool AddrSpace::contains(const AddrSpace& that) const
{
	return (this->mBaseAddress <= that.mBaseAddress &&
			this->mEndAddress >= that.mEndAddress);
}

bool AddrSpace::intersect(const AddrSpace& that) const
{
	//Necessary and sufficient intersection condition
	return (this->mBaseAddress <= that.mEndAddress &&
			this->mEndAddress >= that.mBaseAddress);
}

AddrSpace AddrSpace::intersection(const AddrSpace& that) const
{
	if(!this->intersect(that)){
		return AddrSpace();
	}

	uint64_t base{max(this->mBaseAddress, that.mBaseAddress)},
			 end{min(this->mEndAddress, that.mEndAddress)};

	return AddrSpace(base, end);
}

vector<AddrSpace> AddrSpace::relativeComplement(const AddrSpace& that) const
{
	vector<AddrSpace> result;

	if(this->mBaseAddress < that.mBaseAddress){
		result.push_back(AddrSpace(this->mBaseAddress, that.mBaseAddress - 1));
	}

	if(this->mEndAddress > that.mEndAddress){
		AddrSpace temp(this->mEndAddress - that.mEndAddress);
		temp.setBaseAddress(that.mEndAddress + 1);
		result.push_back(AddrSpace(that.mEndAddress + 1, this->mEndAddress));
	}

	return result;
}

}

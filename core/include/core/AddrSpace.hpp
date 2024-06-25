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

#ifndef ADDRSPACE_HPP_
#define ADDRSPACE_HPP_

#include "global.hpp"
#include <vector>

namespace vpsim
{
	class AddrSpace {
	protected:
		uint64_t mBaseAddress; //!< the base address of the TargetIf, i.e any access between BASE_ADDRESS and BASE_ADDRESS+SIZE shall be mapped to internal local_mem
		uint64_t mEndAddress; //!< end of address mapped to local mem



	public:

		AddrSpace(uint64_t Size = 0);
		AddrSpace(uint64_t base, uint64_t end);
		~AddrSpace();

		static AddrSpace const maxRange;

		//!
		//! sets the value of mBaseAddress to that of BaseAddress
		//! @param [in] BaseAddress
		//!
		void setBaseAddress(uint64_t BaseAddress);

		//!
		//! sets the value of mEndAddress to that of endAddress
		//! @param [in] BaseAddress
		//!
		void setEndAddress(uint64_t endAddress);

		//!
		//! sets the value of mSize to that of Size
		//! @param [in] Size
		//!
		void setSize(uint64_t Size);

		//!
		//! @return the value of mBaseAddress
		//!
		uint64_t getBaseAddress() const;

		//!
		//! @return the value of mSize
		//!
		uint64_t getSize() const;

		//!
		//! @return the value of mEndAddress
		//!
		uint64_t getEndAddress() const;

		//!
		//! @param that AddrSpace used for the test
		//! @return true if that equals *this
		//!
		bool operator==(const AddrSpace& that) const;


		//!
		//! @param that AddrSpace used for the test
		//! @return true if that is different from *this
		//!
		bool operator!=(const AddrSpace& that) const {return !(*this == that);};

		//!
		//! @param that AddrSpace used for the test
		//! @return true if that is greater than *this
		//!
		bool operator<(const AddrSpace& that) const;

		//!
		//! @param that AddrSpace used for the test
		//! @return true if that is contained by *this
		//!
		bool contains(const AddrSpace& that) const;

		//!
		//! @param that AddrSpace used for the test
		//! @return true if that intersect with *this
		//!
		bool intersect(const AddrSpace& that) const;

		//!
		//! @param[in] that AddrSpace to intersect with *this
		//! @return the intersaction of *this and that
		AddrSpace intersection(const AddrSpace& that) const;

		//!
		//! @param that AddrSpace used to compute the relative complement
		//! @return the relative complement of that in *this (*this \ that)
		//!
		vector<AddrSpace> relativeComplement(const AddrSpace& that) const;
	};

}

#endif /* ADDRSPACE_HPP_ */

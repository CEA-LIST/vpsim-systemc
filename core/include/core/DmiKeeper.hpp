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

#ifndef _DMIKEEPER_HPP_
#define _DMIKEEPER_HPP_

#include <inttypes.h>
#include <stdint.h>
#include <vector>
#include <deque>

using namespace std;

namespace vpsim {

struct DmiKeeper {

	DmiKeeper(uint32_t nports) {
		mRanges.resize(nports);
	}

	unsigned char* getDmi(uint32_t port, uint64_t addr) {
		for (auto & entry: mRanges[port]) {
			if (addr >= get<0>(entry) && addr < get<0>(entry)+get<1>(entry)) {
				return get<2>(entry) + addr-get<0>(entry);
			}
		}
		return nullptr;
	}

	void setDmiRange(uint32_t port, uint64_t base, uint64_t size, unsigned char* ptr) {
		mRanges[port].push_back(make_tuple(base, size, ptr));
	}

private:
	std::vector<
	    std::deque<
		    std::tuple<uint64_t, uint64_t, unsigned char*> > > mRanges;
};

}

#endif /* _DMIKEEPER_HPP_ */

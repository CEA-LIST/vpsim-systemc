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

#ifndef DDS_SUBSCRIPTION_POINT_HPP
#define DDS_SUBSCRIPTION_POINT_HPP

#include "libddsadvanced.h"
#include "memory.hpp"

#include <sys/time.h> //for timeval

namespace vpsim
{

class DdsSubscriptionPoint : public sc_module, public vpsim::TargetIf<unsigned char >, public dds::SubscriptionPointAdv {
	private:
		//Internal shared memory space
		//sDdsSubscriptionPoint * LocalMemAsSub;
		//sDdsSubscriptionPoint * BaseAddressAsSub;

//		stringstream ss;
//		ss << sizeof(TheType)*8;
//		return ss.str();

		stringstream HostName;
		stringstream SubName;

		timeval LastReadDDSTime;

	public:

		DdsSubscriptionPoint( sc_module_name Name, uint64_t Size);
		virtual ~DdsSubscriptionPoint();

		tlm::tlm_response_status read ( payload_t & payload, sc_time & delay );
		tlm::tlm_response_status write ( payload_t & payload, sc_time & delay );

};

}//end vpsim namespace

#endif /* DDS_SUBSCRIPTION_POINT_HPP */

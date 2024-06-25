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

#ifndef _ADDRESSTRANSLATOR_HPP_
#define _ADDRESSTRANSLATOR_HPP_


#include <cstdint>
#include <iostream>
#include <tuple>
#include <vector>

#include <core/TargetIf.hpp>
#include "paramManager.hpp"

namespace vpsim {

class AddressTranslator  : public sc_module {
public:
	AddressTranslator(sc_module_name name);
	void setShift(uint64_t translate) { mTranslate=translate; }

	tlm_utils::simple_target_socket<AddressTranslator> mSockIn;
	tlm_utils::simple_initiator_socket<AddressTranslator> mSockOut;
	void b_transport ( tlm::tlm_generic_payload& trans, sc_time& delay );

private:
	uint64_t mTranslate;
};

} /* namespace vpsim */

#endif /* _ADDRESSTRANSLATOR_HPP_ */

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

#include <AddressTranslator.hpp>
#include "log.hpp"

namespace vpsim {

AddressTranslator::AddressTranslator(sc_module_name name): sc_module(name),mTranslate(0) {
	mSockIn.register_b_transport ( this, &AddressTranslator::b_transport );
}

void
AddressTranslator::b_transport ( tlm::tlm_generic_payload& trans, sc_time& delay )
{
	trans.set_address(trans.get_address() + mTranslate);
	mSockOut->b_transport(trans, delay);
}

} /* namespace vpsim */

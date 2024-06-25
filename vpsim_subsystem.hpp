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

#ifndef _VPSIM_SUBSYSTEM_HPP_
#define _VPSIM_SUBSYSTEM_HPP_

#include <string>
#include <functional>
#include <iostream>
#include <systemc>
#include <tlm>

namespace vpsim {

class Subsystem: public sc_core::sc_module {
public:
	Subsystem(sc_core::sc_module_name name, const char* platform_xml_path);
	virtual ~Subsystem();

	/* Call this function to obtain :
	 * - A reference to the output socket, which you can bind to the rest of the system.
	 * - A callback to call in order to generate an interrupt in the subsystem.
	 */
	std::pair<tlm::tlm_initiator_socket<>*, std::function<void(int,int)> > out(const char* port_name) {
	    std::pair<tlm::tlm_initiator_socket<>*,void*> o = _out(port_name);
	    return std::make_pair(o.first,
	        [this, o](int l, int v) -> void {
	            _apply(o.second,l,v);
	        });
	}

	/* Call this to declare the internal pointers of RAM spaces */
	void declare_dmi_ptr(const char* space_name, uint64_t mem_base, uint64_t size, void* pointer);

private:
	void* _handle;
	std::pair<tlm::tlm_initiator_socket<>*,void*> _out(const char* port_name);
	void _apply(void* f, int a, int b);
};

} /* namespace vpsim */

#endif /* _VPSIM_SUBSYSTEM_HPP_ */

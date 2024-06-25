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

#include "Payload.hpp"

namespace vpsim {

//---------------------------------------------------------------------------//
// Payload structure functions                                                         //
//---------------------------------------------------------------------------//
	tlm::tlm_command payload_t::get_command () { return (cmd); }
	void payload_t::set_command (const tlm::tlm_command command) {cmd = command;}

	uint64_t payload_t::get_address () { return (addr); }
	void payload_t::set_address(const sc_dt::uint64 address) {addr = address;}

	unsigned char* payload_t::get_data_ptr() const {return ptr;}
	void payload_t::set_data_ptr(unsigned char* data) {ptr = data;}

	unsigned int payload_t::get_data_length() const {return len;}
	void payload_t::set_data_length(const unsigned int length) {len = length;}

	unsigned char* payload_t::get_byte_enable_ptr() const {return byte_enable_ptr;}
	void payload_t::set_byte_enable_ptr(unsigned char* byte_enable_ptr_) {byte_enable_ptr = byte_enable_ptr_;}

	unsigned int payload_t::get_byte_enable_length() const {return byte_enable_len;}
	void payload_t::set_byte_enable_length(const unsigned int byte_enable_len_) {byte_enable_len = byte_enable_len_;}

	bool payload_t::get_is_active() { return (is_active); }
	void payload_t::set_is_active(const bool val) {is_active = val;}

} /* namespace vpsim */

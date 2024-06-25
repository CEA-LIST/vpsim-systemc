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

#ifndef _PAYLOAD_HPP_
#define _PAYLOAD_HPP_

#include <systemc>
#include <tlm>

namespace vpsim {

struct payload_t {

	tlm::tlm_command cmd ;
	uint64_t addr ;
	unsigned char* ptr ;
	unsigned int len ;
	unsigned char * byte_enable_ptr ;
	unsigned int byte_enable_len ;
	bool is_active;
	bool dmi;
	tlm::tlm_generic_payload* original_payload;

	tlm::tlm_command get_command () ;
	void set_command (const tlm::tlm_command command) ;

	uint64_t get_address () ;
	void set_address(const sc_dt::uint64 address) ;

	unsigned char* get_data_ptr() const ;
	void set_data_ptr(unsigned char* data) ;

	unsigned int get_data_length() const;
	void set_data_length(const unsigned int length);

	unsigned char* get_byte_enable_ptr() const ;
	void set_byte_enable_ptr(unsigned char* byte_enable_ptr_) ;

	unsigned int get_byte_enable_length() const ;
	void set_byte_enable_length(const unsigned int byte_enable_len_) ;

	bool get_is_active() ;
	void set_is_active(const bool val) ;
};

} /* namespace vpsim */

#endif /* _PAYLOAD_HPP_ */

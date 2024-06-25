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

#ifndef INTERCONNECT_HPP_
#define INTERCONNECT_HPP_

#include "global.hpp"
#include <list>
#include "log.hpp"

namespace vpsim
{
	struct addr_space_type {
		uint64_t base_addr;
		uint64_t end_addr;
		uint64_t offset;
		uint32_t port;
	};


	class interconnect : public sc_module, public Logger,
						 public tlm::tlm_bw_transport_if<tlm::tlm_base_protocol_types>,
						 public tlm::tlm_fw_transport_if<tlm::tlm_base_protocol_types>
	{
	private:
		string NAME;
		DIAG_LEVEL DIAGNOSTIC_LEVEL;
		std::vector < addr_space_type > output_ports_t;
		sc_time ACCESS_LATENCY;
		bool ENABLE_LATENCY;

		//Statistics
		std::vector<uint64_t> write_count_out;
		std::vector<uint64_t> read_count_out;

	public:

		uint64_t getWriteCount(int port) { return write_count_out[port]; }
		uint64_t getReadCount(int port) { return read_count_out[port]; }

		void setDefaultRoute(int32_t port) { mDefaultRoute = port; }
		int32_t mDefaultRoute;

		const uint32_t NUM_PORT_IN;
		const uint32_t NUM_PORT_OUT;

		bool mIsMesh;
		int mX;
		int mY;
		sc_time mRouterLatency;
		void set_router_latency(uint64_t nanoseconds){
			mRouterLatency=sc_time(nanoseconds, SC_NS);
		}
		void set_is_mesh(bool is_mesh) { mIsMesh=is_mesh; }
		void set_mesh_coord(int x, int y) {
			mX=x;
			mY=y;
		}
		std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> mAddressIDs;
		std::vector<std::pair<uint64_t,uint64_t>> mIdIDs;
		std::vector<std::tuple<uint64_t, uint64_t, uint64_t>> mHnIDs;
		void register_mem_ctrl(uint64_t base, uint64_t size, uint64_t id) {
			mAddressIDs.push_back(make_tuple(base,size,id));
		}
		void register_source(uint64_t src_id, uint64_t id) {
			mIdIDs.push_back(make_pair(src_id,id));
		}
		void register_hn_input(uint64_t base, uint64_t size, uint64_t id) {
			mHnIDs.push_back(make_tuple(base,size,id));
		}
		uint64_t get_hn_id_by_address(uint64_t addr) {
			for (auto& mapping: mHnIDs) {
				if (addr >= get<0>(mapping) && addr < get<1>(mapping)+get<0>(mapping))
					return get<2>(mapping);
			}
			throw runtime_error("Unknown Home Node ID for address: " + to_string(addr));
		}
		uint64_t get_id_by_address(uint64_t addr) {
			for (auto& mapping: mAddressIDs) {
				if (addr >= get<0>(mapping) && addr < get<1>(mapping)+get<0>(mapping))
					return get<2>(mapping);
			}
			throw runtime_error("Unknown ID for address: " + to_string(addr));
		}
		uint64_t get_id_by_id(uint64_t id) {
			for (auto& mapping: mIdIDs) {
				if (mapping.first == id)
					return mapping.second;
			}
			throw runtime_error("Unknown ID for SOURCE: " + to_string(id));
		}
		//---------------------------------------------------
		//Ports
		std::deque<tlm_utils::simple_target_socket<interconnect>> socket_in;
		std::deque<tlm_utils::simple_initiator_socket<interconnect>> socket_out;


		//---------------------------------------------------
		//Constructor
		interconnect ( sc_module_name name, uint32_t nin, uint32_t nout );
		~interconnect ( ){};
		SC_HAS_PROCESS ( interconnect );


		//---------------------------------------------------
		//Set functions
		void
		set_diagnostic_level ( DIAG_LEVEL val );

		void
		set_socket_out_addr ( uint32_t num_port, uint64_t base_addr, uint64_t offset );

		void
		set_latency ( sc_time val );

		void
		set_enable_latency ( bool val );


		//---------------------------------------------------
		//Get functions
		DIAG_LEVEL
		get_diagnostic_level (  );

		int32_t
		get_port ( uint64_t addr, uint64_t length );

		sc_time
		get_latency ( );

		bool
		get_enable_latency ( );


		//Statistics
		void
		print_statistics ( );


		//---------------------------------------------------
		//TLM 2.0 communication interface
		void
		b_transport( tlm::tlm_generic_payload& trans, sc_time& delay );

		tlm::tlm_sync_enum
		nb_transport_fw(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t);

		bool
		get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data);

		unsigned int
		transport_dbg(tlm::tlm_generic_payload& trans);

		tlm::tlm_sync_enum
		nb_transport_bw ( tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t );

		void
		invalidate_direct_mem_ptr ( sc_dt::uint64 start_range, sc_dt::uint64 end_range );
	};
}

#endif /* INTERCONNECT_HPP_ */

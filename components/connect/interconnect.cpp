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

#include "interconnect.hpp"
#include "log.hpp"
#include <sstream>
#include "MainMemCosim.hpp"

namespace vpsim {

//-----------------------------------------------------------------------------
//Constructor
interconnect::interconnect ( sc_module_name name, uint32_t nin, uint32_t nout ) :
	sc_module(name),
	Logger(string(name)),
	NAME ( string(name) ),
	DIAGNOSTIC_LEVEL ( DBG_L0 ),
	ACCESS_LATENCY ( sc_time(0,SC_NS) ),
	ENABLE_LATENCY ( false ),
	NUM_PORT_IN ( nin ),
	NUM_PORT_OUT ( nout )
{
	//Preliminaries
	char name_socket [100];

	for ( size_t i=0; i<NUM_PORT_OUT; i++ )
	{
		//Instantiate socket_out[i]
		sprintf (name_socket, "socket_out[%zu]",i);
		socket_out.emplace_back(name_socket);

		//Register functions for TLM 2.0 communications
		socket_out[i].register_invalidate_direct_mem_ptr ( this, &interconnect::invalidate_direct_mem_ptr );

		//Statistics
		read_count_out.emplace_back(0);
        write_count_out.emplace_back(0);
	}

	for ( size_t i=0; i<NUM_PORT_IN; i++ )
	{
		//Instantiate socket_in[i]
		sprintf (name_socket, "socket_in[%zu]",i);
		socket_in.emplace_back(name_socket);

		//Register functions for TLM 2.0 communications
		socket_in[i].register_get_direct_mem_ptr ( this, &interconnect::get_direct_mem_ptr );
		socket_in[i].register_b_transport ( this, &interconnect::b_transport );
		socket_in[i].register_transport_dbg ( this, &interconnect::transport_dbg );
	}

	mDefaultRoute=-1;
}


//-----------------------------------------------------------------------------
//Set functions
void
interconnect::set_diagnostic_level ( DIAG_LEVEL val ) { DIAGNOSTIC_LEVEL = val; }


void
interconnect::set_latency ( sc_time val ) { ACCESS_LATENCY = val; }


void
interconnect::set_enable_latency ( bool val ) { ENABLE_LATENCY = val; }


void
interconnect::set_socket_out_addr ( uint32_t num_port, uint64_t base_addr, uint64_t offset )
{
	addr_space_type asp;
	asp.base_addr = base_addr;
	asp.end_addr = base_addr + offset - 1;
	asp.offset = offset;
	asp.port = num_port;

	output_ports_t.push_back ( asp );
}


//Get functions
DIAG_LEVEL
interconnect::get_diagnostic_level (  ) { return ( DIAGNOSTIC_LEVEL ); }


int32_t
interconnect::get_port ( uint64_t addr, uint64_t length )
{
	//Test if there is more than 1 output port
	if (NUM_PORT_OUT>1)
	{
		std::list < addr_space_type >::iterator it;
		int32_t num_port = -1;
		for (auto& as: output_ports_t){
			if ( ( addr >= as.base_addr ) && ( (addr+length-1) <= as.end_addr ) ) {
				num_port = as.port;
				return num_port;
			}
		}

		if (num_port < 0){
			//cout<<"taking default route."<<endl;
			num_port=mDefaultRoute;
		}
		return num_port;
	}

	//Always redirect requests to the only existing port if only one exists
	else return 0;
}


sc_time
interconnect::get_latency ( ) { return ACCESS_LATENCY; }


bool
interconnect::get_enable_latency ( ) { return ENABLE_LATENCY; }


//Statistics
void
interconnect::print_statistics ( ) {
	for (size_t i=0; i<NUM_PORT_OUT; i++) {
		LOG_STATS << "(" << NAME << "): port["<<i<<"]: total read = " <<read_count_out[i]<<", total write = "<<write_count_out[i]<<" (total accesses = "<<read_count_out[i]+write_count_out[i]<<")"<<endl;
	}
}

//---------------------------------------------------
//TLM 2.0 communication interface
void
interconnect::b_transport ( tlm::tlm_generic_payload& trans, sc_time& delay )
{
	//Test the target address and dispatch to the correct output port
    int32_t num_port = get_port ( trans.get_address(), trans.get_data_length() );

	if (num_port==-1) {
		stringstream ss;
    	ss << "Not found - try to access the address 0x"<<hex<<trans.get_address()<<" (burst="<<trans.get_data_length()<<")\n";
		throw runtime_error(ss.str());
    	std::cout << (trans.get_command() == tlm::TLM_READ_COMMAND?"read":"write") <<endl;
    	if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
    		std::cout << "value "<<hex<<*(uint64_t*)trans.get_data_ptr()<<dec<<endl;
    	}
    	char val;
    	if (trans.get_command() == tlm::TLM_WRITE_COMMAND) {
    		cout<<"Proceed ? (y/n)";
    	}
    	else {
    		cout<<"Provide value or fail... Do you have a value ? (y/n)";
    	}
    	cin >> val;
    	if (val == 'y') {
    		if (trans.get_command() == tlm::TLM_READ_COMMAND) {
    			cout<<"Value : ";
    			cin>>hex>>*(uint64_t*)trans.get_data_ptr();
    		}
    		trans.set_response_status(tlm::tlm_response_status::TLM_OK_RESPONSE);
    		return;
    	}
    	trans.set_response_status(tlm::tlm_response_status::TLM_ADDRESS_ERROR_RESPONSE); // this is the clean way to do things, don't throw.
		sc_stop();
    	return;
    }

	//Debug
	LOG_DEBUG(dbg2) <<NAME<<":---------------------------------------------------------"<<endl;
	LOG_DEBUG(dbg2) <<NAME<<": b_transport call"<<endl;
	LOG_DEBUG(dbg2) <<NAME<<": command = "; if (trans.get_command () == tlm::TLM_WRITE_COMMAND) globalLogger.logDebug(dbg2) << "WRITE"; else globalLogger.logDebug(dbg2) << "READ"; globalLogger.logDebug(dbg2)<<endl;
	LOG_DEBUG(dbg2) <<NAME<<": address = 0x"<<hex<<(uint64_t)trans.get_address()<<dec<<endl;
	LOG_DEBUG(dbg2) <<NAME<<": burst = "<<dec<<(uint32_t)trans.get_data_length ()<<dec<<endl;
	LOG_DEBUG(dbg2) <<NAME<<": data ptr = "<<hex<<(uint64_t*)trans.get_data_ptr ()<<dec<<endl;
	LOG_DEBUG(dbg2) <<NAME<<": byte_enable_ptr = 0x"<<hex<<(uint64_t*)trans.get_byte_enable_ptr ()<<dec<<endl;
	LOG_DEBUG(dbg2) <<NAME<<": byte_enable_len = "<<(uint32_t) trans.get_byte_enable_length ()<<endl;
	LOG_DEBUG(dbg2) <<NAME<<": num output port = "<<num_port<<endl;
	LOG_DEBUG(dbg2) <<NAME<<": delay = "<<delay<<endl;

	//Statistics
	if (trans.get_command () == tlm::TLM_WRITE_COMMAND)
		write_count_out [num_port] += trans.get_data_length () ; //TODO
	else read_count_out [num_port] += trans.get_data_length () ; //TODO

	//Add timing for communication
	if (ENABLE_LATENCY) delay += ACCESS_LATENCY;

	// NoC Model
	if (mIsMesh) {
		// get transaction source ID
		SourceCpuExtension* src=nullptr;
		uint64_t noc_id;
		trans.get_extension<SourceCpuExtension>(src);
		if (!src) {
			noc_id = get_hn_id_by_address(trans.get_address());
		} else {
			noc_id = get_id_by_id(src->cpu_id);
		}

		int src_x = noc_id % mX;
		int src_y = noc_id / mX;

		// dest ID
		int id = get_id_by_address(trans.get_address());
		int dst_x = id % mX;
		int dst_y = id / mX;

		int dist = abs(src_x-dst_x) + abs(src_y-dst_y);
		delay += mRouterLatency * dist;
	}

    socket_out[num_port]->b_transport ( trans, delay );
}

 tlm::tlm_sync_enum
 interconnect::nb_transport_fw ( tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t )
 {
 /*	//Debug
 	if ( DIAGNOSTIC_LEVEL >= DBG_L1 ) {
 		cout<<NAME<<":---------------------------------------------------------"<<endl;
 		cout<<NAME<<": nb_transport_fw call"<<endl;
 		cout<<NAME<<": command = "; if (trans.get_command () == tlm::TLM_WRITE_COMMAND) cout << "WRITE"; else cout << "READ"; cout<<endl;
 		cout<<NAME<<": address = 0x"<<hex<<(uint64_t)trans.get_address()<<dec<<endl;
 		cout<<NAME<<": burst = "<<dec<<(uint32_t)trans.get_data_length ()<<dec<<endl;
 		cout<<NAME<<": data ptr = 0x"<<hex<<(uint64_t*)trans.get_data_ptr ()<<dec<<endl;
 		cout<<NAME<<": byte_enable_ptr = 0x"<<hex<<(uint64_t*)trans.get_byte_enable_ptr ()<<dec<<endl;
 		cout<<NAME<<": byte_enable_len = "<<(uint32_t) trans.get_byte_enable_length ()<<endl;
 	}

 	return socket_out->nb_transport_fw ( trans, phase, t );*/

	throw;
 }

 bool
 interconnect::get_direct_mem_ptr ( tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data )
 {
 /*	//Debug
 	if ( DIAGNOSTIC_LEVEL >= DBG_L1 ) {
 		cout<<NAME<<":---------------------------------------------------------"<<endl;
 		cout<<NAME<<": get_direct_mem_ptr call"<<endl;
 		cout<<NAME<<": command = "; if (trans.get_command () == tlm::TLM_WRITE_COMMAND) cout << "WRITE"; else cout << "READ"; cout<<endl;
 		cout<<NAME<<": address = 0x"<<hex<<(uint64_t)trans.get_address()<<dec<<endl;
 		cout<<NAME<<": burst = "<<dec<<(uint32_t)trans.get_data_length ()<<dec<<endl;
 		cout<<NAME<<": data ptr = 0x"<<hex<<(uint64_t*)trans.get_data_ptr ()<<dec<<endl;
 		cout<<NAME<<": byte_enable_ptr = 0x"<<hex<<(uint64_t*)trans.get_byte_enable_ptr ()<<dec<<endl;
 		cout<<NAME<<": byte_enable_len = "<<(uint32_t) trans.get_byte_enable_length ()<<endl;
 	}

 	return socket_out->get_direct_mem_ptr ( trans, dmi_data );*/

	if (trans.get_data_length() == 0)
		trans.set_data_length(1);

	int32_t num_port = get_port ( trans.get_address(), trans.get_data_length() );


	if (num_port<0) {
		throw runtime_error(string("Nothing found between ")+std::to_string(trans.get_address())+" and "+std::to_string(trans.get_data_length()));
	}
	bool ret = socket_out[num_port]->get_direct_mem_ptr ( trans, dmi_data);

	LOG_GLOBAL_INFO <<"At port : "<<num_port<<" -> ";

	if (ret) {
		LOG_GLOBAL_INFO<<"Delivering address space : "<<dmi_data.get_start_address()<<" -> "<<dmi_data.get_end_address()<<endl;
	}
	else {
		LOG_GLOBAL_INFO<<"Address "<<trans.get_address()<<" does not provide DMI"<<endl;
	}
	return ret;
 }

 unsigned int
 interconnect::transport_dbg(tlm::tlm_generic_payload& trans)
 {
 /*	//Debug
 	if ( DIAGNOSTIC_LEVEL >= DBG_L1 ) {
 		cout<<NAME<<":---------------------------------------------------------"<<endl;
 		cout<<NAME<<": transport_dbg call"<<endl;
 		cout<<NAME<<": command = "; if (trans.get_command () == tlm::TLM_WRITE_COMMAND) cout << "WRITE"; else cout << "READ"; cout<<endl;
 		cout<<NAME<<": address = 0x"<<hex<<(uint64_t)trans.get_address()<<dec<<endl;
 		cout<<NAME<<": burst = "<<dec<<(uint32_t)trans.get_data_length ()<<dec<<endl;
 		cout<<NAME<<": data ptr = 0x"<<hex<<(uint64_t*)trans.get_data_ptr ()<<dec<<endl;
 		cout<<NAME<<": byte_enable_ptr = 0x"<<hex<<(uint64_t*)trans.get_byte_enable_ptr ()<<dec<<endl;
 		cout<<NAME<<": byte_enable_len = "<<(uint32_t) trans.get_byte_enable_length ()<<endl;
 	}

 	return socket_out->transport_dbg ( trans );*/
	//TODO: Provide proper implementation or do not use this method
	trans.set_response_status(tlm::tlm_response_status::TLM_OK_RESPONSE);

	// according to TLM source "The return value of defines the number of bytes successfully transferred"
	// since the debug interface does not perform any transfer, it is safer to return 0
	return 0;
 }

 void
 interconnect::invalidate_direct_mem_ptr ( sc_dt::uint64 start_range, sc_dt::uint64 end_range )
 {
 /*	//Debug
 	if ( DIAGNOSTIC_LEVEL >= DBG_L1 ) {
 		cout<<NAME<<":---------------------------------------------------------"<<endl;
 		cout<<NAME<<": invalidate_direct_mem_ptr call"<<endl;
 		cout<<NAME<<": start_range = 0x"<<hex<<(uint64_t)start_range<<dec<<endl;
 		cout<<NAME<<": end_range = 0x"<<hex<<(uint64_t)start_range<<dec<<endl;
 	}*/

     for(auto& s: socket_in){
         s->invalidate_direct_mem_ptr(start_range, end_range);
     }
 }

 tlm::tlm_sync_enum
 interconnect::nb_transport_bw ( tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t )
 {
 /*	//Debug
 	if ( DIAGNOSTIC_LEVEL >= DBG_L1 ) {
 		cout<<NAME<<":---------------------------------------------------------"<<endl;
 		cout<<NAME<<": nb_transport_bw call"<<endl;
 		cout<<NAME<<": command = "; if (trans.get_command () == tlm::TLM_WRITE_COMMAND) cout << "WRITE"; else cout << "READ"; cout<<endl;
 		cout<<NAME<<": address = 0x"<<hex<<(uint64_t)trans.get_address()<<dec<<endl;
 		cout<<NAME<<": burst = "<<dec<<(uint32_t)trans.get_data_length ()<<dec<<endl;
 		cout<<NAME<<": data ptr = 0x"<<hex<<(uint64_t*)trans.get_data_ptr ()<<dec<<endl;
 		cout<<NAME<<": byte_enable_ptr = 0x"<<hex<<(uint64_t*)trans.get_byte_enable_ptr ()<<dec<<endl;
 		cout<<NAME<<": byte_enable_len = "<<(uint32_t) trans.get_byte_enable_length ()<<endl;
 	}

 	return socket_in->nb_transport_bw ( trans, phase, t );*/

	//TODO: Provide proper implementation or do not use this method
	throw;
 }

}

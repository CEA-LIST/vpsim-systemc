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

#include "InitiatorIf.hpp"
#include "log.hpp"

namespace vpsim {

using namespace tlm;

//-----------------------------------------------------------------------------
//Constructor
InitiatorIf::InitiatorIf ( string Name, uint32_t NbPort ) :
	//quantum actually seems to be useless now.
	//duplicating the constructor to prevent from breaking the use of the other one
	InitiatorIf ( Name, 0, NbPort )
{}


InitiatorIf::InitiatorIf ( string Name, unsigned int Quantum, uint32_t NbPort ) :
	InitiatorIf ( Name, Quantum, true, NbPort )
{}


InitiatorIf::InitiatorIf ( string Name, unsigned int Quantum, bool Active, uint32_t NbPort ) :
//	initiator_socket("InitiatorIf_socket"),
	Logger(Name),
	mDmiEnable ( false ),
	mName ( Name ),
	mDiagnosticLevel ( DBG_L0 ),
	mForceLt ( false ),
	mTlmActive ( Active ),
	mNbPort (NbPort)
{
	//Create table of port
	mInitiatorSocket = new tlm_utils::simple_initiator_socket<InitiatorIf> * [getNbPort()];

	for ( size_t i=0; i < getNbPort(); i++ ) {

		//cout << endl << "InitiatorIf.cpp: InitiatorIf: i = " << i << " name = " << getName() << endl;

		//Create output ports
		char tmp [100]; sprintf (tmp, "initiator_socket[%zu]",i);
		getInitiatorSocket()[i] = new tlm_utils::simple_initiator_socket<InitiatorIf> ( tmp );

		//register the socket
		getInitiatorSocket()[i]->register_invalidate_direct_mem_ptr ( this, &InitiatorIf::invalidate_direct_mem_ptr );
	}
}

//Destructor
InitiatorIf::~InitiatorIf ( )
{
	for ( size_t i=0; i<getNbPort(); i++ ) delete mInitiatorSocket [i];
	delete [] mInitiatorSocket;
}

//-----------------------------------------------------------------------------
//Helper functions
void InitiatorIf::tlm_error_checking ( const tlm::tlm_response_status status )
{
	switch ( status )
	{
		//TLM_OK_RESPONSE
		case tlm::TLM_OK_RESPONSE: {
		} break;

		//TLM_INCOMPLETE_RESPONSE
		case tlm::TLM_INCOMPLETE_RESPONSE: {
			LOG_ERROR <<getName()<<": TLM_INCOMPLETE_RESPONSE."<<endl;
			cerr<<getName()<<": TLM_INCOMPLETE_RESPONSE."<<endl; throw(0);
		} break;

		//TLM_GENERIC_ERROR_RESPONSE
		case tlm::TLM_GENERIC_ERROR_RESPONSE: {
			LOG_ERROR <<getName()<<": TLM_GENERIC_ERROR_RESPONSE."<<endl;
			cerr<<getName()<<": TLM_GENERIC_ERROR_RESPONSE."<<endl; throw(0);
		} break;

		//TLM_ADDRESS_ERROR_RESPONSE
		case tlm::TLM_ADDRESS_ERROR_RESPONSE: {
			LOG_ERROR <<getName()<<": TLM_ADDRESS_ERROR_RESPONSE."<<endl;
			cerr<<getName()<<": TLM_ADDRESS_ERROR_RESPONSE."<<endl; throw(0);
		} break;

		//TLM_COMMAND_ERROR_RESPONSE
		case tlm::TLM_COMMAND_ERROR_RESPONSE: {
			LOG_ERROR <<getName()<<": TLM_COMMAND_ERROR_RESPONSE."<<endl;
			cerr<<getName()<<": TLM_COMMAND_ERROR_RESPONSE."<<endl; throw(0);
		} break;

		//TLM_BURST_ERROR_RESPONSE
		case tlm::TLM_BURST_ERROR_RESPONSE: {
			LOG_ERROR <<getName()<<": TLM_BURST_ERROR_RESPONSE."<<endl;
			cerr<<getName()<<": TLM_BURST_ERROR_RESPONSE."<<endl; throw(0);
		} break;

		//TLM_BYTE_ENABLE_ERROR_RESPONSE
		case tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE: {
			LOG_ERROR <<getName()<<": TLM_BYTE_ENABLE_ERROR_RESPONSE."<<endl;
			cerr<<getName()<<": TLM_BYTE_ENABLE_ERROR_RESPONSE."<<endl; throw(0);
		} break;

		default: {
			LOG_ERROR <<getName()<<": undefined TLM response status."<<endl;
			cerr<<getName()<<": undefined TLM response status."<<endl; throw(0);
		} break;
	}
}

//-----------------------------------------------------------------------------
//Set functions
void InitiatorIf::setDiagnosticLevel ( DIAG_LEVEL DiagnosticLevel ) { mDiagnosticLevel = DiagnosticLevel; }

void InitiatorIf::setForceLt ( bool ForceLt ) { mForceLt = ForceLt; }

void InitiatorIf::setDmiEnable(bool DmiEnable) { mDmiEnable = DmiEnable; }

//-----------------------------------------------------------------------------
//Get functions
DIAG_LEVEL InitiatorIf::getDiagnosticLevel() { return ( mDiagnosticLevel ); }

bool InitiatorIf::getForceLt() { return ( mForceLt ); }

uint32_t InitiatorIf::getNbPort() { return (mNbPort); }

string InitiatorIf::getName() { return (mName); }

tlm_utils::simple_initiator_socket<InitiatorIf> ** InitiatorIf::getInitiatorSocket() {
	return (mInitiatorSocket);
}

bool InitiatorIf::getTlmActive() { return (mTlmActive); }

bool InitiatorIf::getDmiEnable() { return (mDmiEnable); }

//-----------------------------------------------------------------------------
//target_mem_access
tlm::tlm_response_status InitiatorIf::target_mem_access ( uint32_t port, uint64_t addr,
		uint32_t length, unsigned char * data, ACCESS_TYPE rw, sc_time &delay, uint32_t id ) {
	//static prevents for constructing/destructing the payload every time (very costly)

	//Reset is only a partial reset
	//One must take care of explicitely initializing every single used field of the payload
	//Otherwise, previous transactions values could interfere
	trans.reset();

	//dmi_data can be safely reused as all fields are explicitely set bellow;
//	static tlm::tlm_dmi dmi_data;

	tlm::tlm_response_status status(tlm::TLM_INCOMPLETE_RESPONSE);

//	if ( !getForceLt() && getTlmActive() )
//	{
//		//Test if this access is in a region already registered as DMI
//		bool found = false;
//		std::vector < tlm::tlm_dmi >::iterator it;
//		for ( it=mCachedDmiRegions.begin() ; it!=mCachedDmiRegions.end() ; it++ ) {
//			if (( addr >= it->get_start_address() ) && ( addr < it->get_end_address() ))
//			{
//				dmi_data.set_dmi_ptr ( it->get_dmi_ptr() );
//				dmi_data.set_start_address ( it->get_start_address() );
//				dmi_data.set_end_address ( it->get_end_address() );
//				dmi_data.set_granted_access ( it->get_granted_access() );
//				dmi_data.set_read_latency( it->get_read_latency() );
//				dmi_data.set_write_latency ( it->get_write_latency() );
//				found = true;
//			}
//		}
//		if(!found)
//		{
//			//Test if DMI is enabled for future communications at this address
//			trans.set_address ( addr );
//			trans.set_data_length ( length );
//			setDmiEnable( (*getInitiatorSocket() [port])->get_direct_mem_ptr ( trans, dmi_data ));
//			tlm_error_checking ( trans.get_response_status() );
//			mCachedDmiRegions.push_back ( dmi_data );
//		}
//	}


	//------------------------------------------------------------------------------
	// DEBUG
	LOG_DEBUG(dbg2) <<getName()<<":---------------------------------------------------------"<<endl;

	//Read or Write?
	if ( rw == READ) {LOG_DEBUG(dbg2) <<getName()<<": command = READ" <<endl;}
	else {LOG_DEBUG(dbg2) <<getName()<<": command = WRITE";}

	//LT or DMI
//	if ( ( dmi_data.is_read_allowed() && !getForceLt() ) ||
//			 ( dmi_data.is_write_allowed() && !getForceLt() ) ){
//		LOG_DEBUG(dbg2)  << " (DMI)"<<endl; else LOG_DEBUG(dbg2) <<" (LT)"<<endl;
//	}

	//Others
	LOG_DEBUG(dbg2) <<getName()<<": address = 0x"<<hex<<(uint64_t)addr<<dec<<endl;
	LOG_DEBUG(dbg2) <<getName()<<": burst = "<<dec<<(uint32_t)length<<dec<<endl;
	LOG_DEBUG(dbg2) <<getName()<<": data ptr = "<<hex<<(uint64_t*)data<<dec<<endl;

	//Active or not?
	LOG_DEBUG(dbg2) <<getName()<<": is_active = " <<(getTlmActive() ? "true":"false") << endl;


	//------------------------------------------------------------------------------
	//READ access
	if ( rw == READ )
	{
		//Test if DMI access
//		if ( dmi_data.is_read_allowed() && !getForceLt() )
//		{
//			if (getTlmActive()) {
//				//Do DMI transaction
//				unsigned char* dmi_ptr = dmi_data.get_dmi_ptr();
//				memcpy ( data, dmi_ptr + addr, length );
//				delay += dmi_data.get_read_latency();
//				status = tlm::TLM_OK_RESPONSE;
//			}
//			else {
//				//Nothing to do (the DMI communication has already been done)
//			}
//		}

		//Loosely-timed communication
//		else
//		{

			//cout << endl << "InitiatorIf.cpp: target_mem_access: else is_read_allowed1: dmi_data.is_read_allowed = " << dmi_data.is_read_allowed() << " name = " << getName() << endl;

			//Prepare payload
			trans.set_address ( addr );
			trans.set_read ( );
			trans.set_data_length ( length );
			trans.set_data_ptr ( (unsigned char*)(data) );
			trans.set_byte_enable_ptr ( NULL );
			trans.set_byte_enable_length ( 0 );
			if (getTlmActive() || getForceLt()) trans.set_gp_option ( tlm::TLM_FULL_PAYLOAD ); else trans.set_gp_option ( tlm::TLM_MIN_PAYLOAD );
			trans.set_response_status ( tlm::TLM_INCOMPLETE_RESPONSE );

			GicCpuExtension cpu_id_ext;
			cpu_id_ext.cpu_id=id;
			trans.set_extension<GicCpuExtension>(&cpu_id_ext);
			//cout << endl << "InitiatorIf.cpp: target_mem_access: else is_read_allowed2: dmi_data.is_read_allowed = " << dmi_data.is_read_allowed() << " name = " << getName() << endl;

			//Execute blocking transaction
			(*getInitiatorSocket() [port])->b_transport ( trans, delay );
			trans.clear_extension(&cpu_id_ext);
			status = trans.get_response_status ( );
//		}
	}
	//WRITE access
	else
	{
		//Test if DMI access
//		if ( dmi_data.is_write_allowed() && !getForceLt() )
//		{
//			if (getTlmActive()) {
//				//Do DMI transaction
//				unsigned char* dmi_ptr = dmi_data.get_dmi_ptr();
//				memcpy ( dmi_ptr + addr, data, length );
//				delay += dmi_data.get_write_latency();
//				status = tlm::TLM_OK_RESPONSE;
//			}
//			else {
//				//Nothing to do (the DMI communication has already been done)
//			}
//		}
		//Loosely-timed communication
//		else
//		{
			//cout << endl << "InitiatorIf.cpp: target_mem_access: else is_write_allowed: dmi_data.is_write_allowed = " << dmi_data.is_write_allowed() << " name = " << getName() << endl;

			//Prepare payload
			trans.set_address ( addr );
			trans.set_write ( );
			trans.set_data_length( length );
			trans.set_data_ptr ( (unsigned char*)(data) );
			trans.set_byte_enable_ptr ( NULL );
			trans.set_byte_enable_length ( 0 );
			if (getTlmActive() || getForceLt()) trans.set_gp_option ( tlm::TLM_FULL_PAYLOAD ); else trans.set_gp_option ( tlm::TLM_MIN_PAYLOAD );
			trans.set_response_status ( tlm::TLM_INCOMPLETE_RESPONSE );
			GicCpuExtension cpu_id_ext;
			cpu_id_ext.cpu_id=id;
			trans.set_extension<GicCpuExtension>(&cpu_id_ext);

			//Execute blocking transaction
			(*getInitiatorSocket() [port])->b_transport(trans, delay);
			trans.clear_extension(&cpu_id_ext);
			status = trans.get_response_status ();
//		}
	}

	//End
	return status;
}

uint32_t InitiatorIf::target_dbg_access ( uint32_t port,
		uint64_t addr, uint32_t length, unsigned char * data, ACCESS_TYPE rw ) {
	if ( !getTlmActive() ) {
		cout << getName()<<": debug mode is not supported when communications are inactive."<<endl;
		throw(0);
	}

	else if (getDmiEnable())
	{
		//Local variables
		static tlm::tlm_generic_payload trans;
		trans.reset();

		//Prepare payload
		trans.set_address ( addr );
		trans.set_data_length ( length );
		trans.set_data_ptr ( data );
		if ( rw == READ) trans.set_read () ; else trans.set_write () ;
		uint32_t nb_bytes = (*getInitiatorSocket()[port])->transport_dbg( trans );
		tlm_error_checking ( trans.get_response_status () );

		//End
		return nb_bytes;
	}
	else return 0;
}

//----------------------------------------------------------------------------------
//Dummy implementation for the backward non-blocking interface
tlm::tlm_sync_enum InitiatorIf::nb_transport_bw ( tlm::tlm_generic_payload& trans,
		tlm::tlm_phase& phase, sc_core::sc_time& t) {
	return tlm::TLM_COMPLETED;
}

//Function for backward DMI
void InitiatorIf::invalidate_direct_mem_ptr ( sc_dt::uint64 start, sc_dt::uint64 end ) {
	setDmiEnable(false);
}

}

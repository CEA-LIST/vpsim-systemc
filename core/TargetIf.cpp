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

#include "TargetIf.hpp"
#include "log.hpp"
#include "string.h"

namespace vpsim {

//-----------------------------------------------------------------------------
//Constructor
template < typename TYPE >
TargetIf<TYPE>::TargetIf ( string Name, uint64_t Size ) :
	LatencyIf (),
	AddrSpace (Size),
	Logger(Name),
	mName ( Name ),
	mByteEnable ( false ),
	mDmiEnable ( false ),
	mReadCallback(NULL),
	mWriteCallback(NULL),
	mLocalMem ( NULL ),
	mReadCount ( 0 ),
	mWriteCount ( 0 ),
	mTargetSocket ( "mTargetSocket" )
{
	mLocalMem = new TYPE [getSize()];

	//Register functions for TLM 2.0 communications
	mTargetSocket.register_get_direct_mem_ptr ( this, &TargetIf::get_direct_mem_ptr );
	mTargetSocket.register_b_transport ( this, &TargetIf::b_transport );
	mTargetSocket.register_transport_dbg ( this, &TargetIf::transport_dbg );
}

template < typename TYPE >
TargetIf<TYPE>::TargetIf ( string Name, uint64_t Size, bool ByteEnable, bool DmiEnable ) :
	LatencyIf (),
	AddrSpace (Size),
	Logger(Name),
	mName ( Name ),
	mByteEnable ( ByteEnable ),
	mDmiEnable ( DmiEnable ),
	mReadCallback(NULL),
	mWriteCallback(NULL),
	mLocalMem ( NULL ),
	mReadCount ( 0 ),
	mWriteCount ( 0 ),
	mTargetSocket ( "mTargetSocket" )
{
	mLocalMem = new TYPE [getSize()];

	//Register functions for TLM 2.0 communications
	mTargetSocket.register_get_direct_mem_ptr ( this, &TargetIf::get_direct_mem_ptr );
	mTargetSocket.register_b_transport ( this, &TargetIf::b_transport );
	mTargetSocket.register_transport_dbg ( this, &TargetIf::transport_dbg );
}

template < typename TYPE >
void TargetIf<TYPE>::setByteEnable ( bool val ) { mByteEnable = val; }

template < typename TYPE >
void TargetIf<TYPE>::setDmiEnable ( bool val ) { mDmiEnable = val; }

//-----------------------------------------------------------------------------
//Get functions
template < typename TYPE >
bool TargetIf<TYPE>::getByteEnable ( ) { return ( mByteEnable ); }

template < typename TYPE >
bool TargetIf<TYPE>::getDmiEnable ( ) { return ( mDmiEnable ); }

template < typename TYPE >
string TargetIf<TYPE>::getName ( ) { return ( mName ); }

template < typename TYPE >
TYPE * TargetIf<TYPE>::getLocalMem ( ) { return mLocalMem; }

template < typename TYPE >
tlm_utils::simple_target_socket<TargetIf<TYPE>> * TargetIf<TYPE>::getTargetSocket() {
	return (&mTargetSocket);
}

//-----------------------------------------------------------------------------
//Statistics
template < typename TYPE >
void TargetIf<TYPE>::PrintStatistics ( ) {
	LOG_STATS << "(" << getName() << "): total read = " <<mReadCount<<", total write = "<<mWriteCount<<" (total accesses = "<<mReadCount+mWriteCount<<")"<<endl;
}

//-----------------------------------------------------------------------------
//Helper functions
template < typename TYPE >
void TargetIf<TYPE>::RegisterMemorySpace ( TYPE * mem ) {
    if (mLocalMem!=NULL) {
    	LOG_ERROR <<getName()<< ": local memory pointer already registered." << endl;
    	cerr <<getName()<< ": local memory pointer already registered." << endl; throw(0);
    } else {
    	mLocalMem = (TYPE*) mem;
    }
}

template < typename TYPE >
void TargetIf<TYPE>::RegisterReadAccess ( Callback_t * callback ) {
    if (mReadCallback) {
    	LOG_ERROR <<getName()<< ": read access function already registered." << endl;
    	cerr <<getName()<< ": read access function already registered." << endl; throw(0);
    } else {
    	mReadCallback = callback;
    }
}

template < typename TYPE >
void TargetIf<TYPE>::RegisterWriteAccess ( Callback_t * callback ) {
    if (mWriteCallback) {
    	LOG_ERROR <<getName()<< ": write access function already registered." << endl;
    	cerr <<getName()<< ": write access function already registered." << endl; throw(0);
    } else {
    	mWriteCallback = callback;
    }
}

//-----------------------------------------------------------------------------
//Destructor
template < typename TYPE >
TargetIf<TYPE>::~TargetIf () {

	//printf("\nTargetIf.cpp: destrcutor: mReadCallback = %p, mWriteCallback = %p, mLocalMem = %p \n", mReadCallback, mWriteCallback, mLocalMem);

	//handle deletion of function callbacks when the interface is destroyed
	if (mReadCallback) delete mReadCallback;
	if (mWriteCallback) delete mWriteCallback;

	delete[] mLocalMem;
}

//----------------------------------------------------------------------------------
// Function name: Blocking interface
//
// Payload :
//   - Data length is in Bytes
// Notes:
//   - No wait calls can be made in this function
//
template < typename TYPE >
void TargetIf<TYPE>::b_transport( tlm::tlm_generic_payload& trans, sc_time& delay ) {
	//-----------------------------------------------------------------------
	//Get information
	payload_t payload;
	payload.cmd = trans.get_command ();
	payload.addr = trans.get_address ();
	payload.ptr = trans.get_data_ptr ();
	payload.len = trans.get_data_length ();
	payload.byte_enable_ptr = trans.get_byte_enable_ptr ();
	payload.byte_enable_len = trans.get_byte_enable_length ();
	payload.is_active = payload.ptr != nullptr; //( trans.get_gp_option() == tlm::TLM_MIN_PAYLOAD ) ? false : true;
	payload.dmi = false;
	payload.original_payload = &trans;

	//-----------------------------------------------------------------------
	//Test address
	if ( (payload.addr < getBaseAddress() ) || ((payload.addr + payload.len) > (getBaseAddress() + getSize())) ) {

		trans.set_response_status( tlm::TLM_ADDRESS_ERROR_RESPONSE );
		return;
	}

	//Test burst size
	if ( (payload.len > getSize() ) || (payload.len == 0) ) {
		trans.set_response_status( tlm::TLM_BURST_ERROR_RESPONSE );
		return;
	}

	//Test byte mode
	if ( !getByteEnable() && ((payload.byte_enable_ptr!=0) || (payload.byte_enable_len!=0) )) {
		trans.set_response_status( tlm::TLM_BYTE_ENABLE_ERROR_RESPONSE );
		return;
	}

	//Test command
	if (( payload.cmd != tlm::TLM_WRITE_COMMAND ) && ( payload.cmd != tlm::TLM_READ_COMMAND) ) {
		trans.set_response_status( tlm::TLM_COMMAND_ERROR_RESPONSE );
		return;
	}

	//-----------------------------------------------------------------------
	//Function core
	tlm::tlm_response_status rsp = CoreFunction ( payload, delay );
	trans.set_dmi_allowed(payload.dmi);

	//-----------------------------------------------------------------------
	// Successful completion
	trans.set_response_status( rsp );

	//End
	return;
}

//----------------------------------------------------------------------------------
// Function name: Core function
template < typename TYPE >
tlm::tlm_response_status TargetIf<TYPE>::CoreFunction ( payload_t& payload, sc_time& delay ) {
	//Local variable
	tlm::tlm_response_status status ( tlm::TLM_OK_RESPONSE );

	if ( payload.cmd == tlm::TLM_WRITE_COMMAND )
	{
		//Send WRITE request
		status = WriteAccessFunction ( payload, delay );

		//Debug
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<":---------------------------------------------------------"<<endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": command = WRITE"<<endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": address = 0x"<<hex<<(uint64_t)payload.addr<<dec<<endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": burst = "<<dec<<(uint32_t)payload.len<<dec<<endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": data ptr = 0x"<<hex<<(uint64_t*)payload.ptr<<dec<<endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": mByteEnable_ptr = 0x"<<hex<<(uint64_t*)payload.byte_enable_ptr<<dec<<endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": mByteEnable_len = "<<(uint32_t) payload.byte_enable_len<<endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": is_active = " << (payload.is_active ? "true" : "false");
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": delay = "<<delay<<endl;

		//Statistics
		mWriteCount+=payload.len / sizeof(TYPE);	//TODO
	}
	else
	{
		//Send READ request
		status = ReadAccessFunction ( payload, delay );

		//Debug
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<":---------------------------------------------------------"<<endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": command = WRITE"<<endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": address = 0x"<<hex<<(uint64_t)payload.addr<<dec<<endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": burst = "<<dec<<(uint32_t)payload.len<<dec<<endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": data ptr = 0x"<<hex<<(uint64_t*)payload.ptr<<dec<<endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": mByteEnable_ptr = 0x"<<hex<<(uint64_t*)payload.byte_enable_ptr<<dec<<endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": mByteEnable_len = "<<(uint32_t) payload.byte_enable_len<<endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": is_active = " << (payload.is_active ? "true" : "false") << endl;
		LOG_GLOBAL_DEBUG(dbg2) <<getName()<<": delay = "<<delay<<endl;

		//Statistics
		mReadCount+=payload.len / sizeof(TYPE);		//TODO
	}

	//End
	return ( status );
}

template < typename TYPE >
tlm::tlm_response_status TargetIf<TYPE>::ReadAccessFunction ( payload_t &payload, sc_time& delay ) {
	if (!mReadCallback) {
	   	LOG_ERROR <<getName()<< ": no read function registered." << endl;
	   	cerr <<getName()<< ": no read function registered." << endl; throw(0);
	} else {
	 	return (*mReadCallback) ( payload, delay );
	}
}

template < typename TYPE >
tlm::tlm_response_status TargetIf<TYPE>::WriteAccessFunction ( payload_t &payload, sc_time& delay ) {
	if (!mWriteCallback) {
	   	LOG_ERROR <<getName()<< ": no write function registered." << endl;
	   	cerr <<getName()<< ": no write function registered." << endl; throw(0);
	} else {
		return (*mWriteCallback) ( payload, delay );
	}
}


//-----------------------------------------------------------------------------
//Dummy implementation for the forward non-blocking interface
template < typename TYPE >
tlm::tlm_sync_enum TargetIf<TYPE>::nb_transport_fw(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t) {
	return tlm::TLM_COMPLETED;
}


//Implementation for the forward DMI interface
template < typename TYPE >
bool TargetIf<TYPE>::get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data) {

	//cout<<"get_direct_mem_ptr called for"<<this->getName()<<endl;

	//Get information
	uint64_t addr = trans.get_address ();
	uint32_t len = trans.get_data_length ();

	//Test address
	if ( ( addr < getBaseAddress() ) || ((addr+len-1) > getEndAddress()) ) {
		trans.set_response_status( tlm::TLM_ADDRESS_ERROR_RESPONSE );
		return false;
	}

	//Set DMI connection parameters

	// Note : Granting both R/W is allowed in TLM 2.0 (standard section 4.2.5 point (m) and (n))
	//        In addition refusing DMI shall be performed with R/W set but with return value set to false
	if(getDmiEnable()) {
		dmi_data.allow_read_write();
	} else {
		dmi_data.allow_none();
	}
	dmi_data.set_start_address ( getBaseAddress() );
	dmi_data.set_end_address ( getEndAddress() );
	//dmi_data.set_read_latency ( getReadLatency() );
	//dmi_data.set_write_latency ( getWriteLatency() );
	dmi_data.set_read_latency ( getReadWordLatency() );
	dmi_data.set_write_latency ( getWriteWordLatency() );
	assert(mLocalMem != NULL);
	dmi_data.set_dmi_ptr ( (unsigned char *) &mLocalMem[0] );

	//Debug
	LOG_GLOBAL_DEBUG(dbg2)<<getName()<<":---------------------------------------------------------"<<endl;
	LOG_GLOBAL_DEBUG(dbg2)<<getName()<<": DMI permission granted"<<endl;
	LOG_GLOBAL_DEBUG(dbg2)<<getName()<<": start address = 0x"<<hex<<(uint64_t)dmi_data.get_start_address ()<<dec<<endl;
	LOG_GLOBAL_DEBUG(dbg2)<<getName()<<": end address = "<<hex<<(uint64_t)dmi_data.get_end_address ()<<dec<<endl;
	LOG_GLOBAL_DEBUG(dbg2)<<getName()<<": read latency = "<<dmi_data.get_read_latency ()<<dec<<endl;
	LOG_GLOBAL_DEBUG(dbg2)<<getName()<<": write latency = "<<dmi_data.get_write_latency ()<<dec<<endl;
	LOG_GLOBAL_DEBUG(dbg2)<<getName()<<": RW access = "<<dmi_data.get_granted_access()<<endl;
	LOG_GLOBAL_DEBUG(dbg2)<<getName()<<": mByteEnable_ptr = 0x"<<hex<<(uint64_t*)dmi_data.get_dmi_ptr ()<<dec<<endl;

	// Successful completion
	trans.set_response_status( tlm::TLM_OK_RESPONSE );

	//End
	// the boolean returned grants (if true) or not dmi access to the required region
	//see TLM2.0 section 4.2.5.n for further explanation
	if (getDmiEnable()){
		return true;
	}else{
		return false;
	}
}

//Implementation for the debug interface
template < typename TYPE >
unsigned int TargetIf<TYPE>::transport_dbg(tlm::tlm_generic_payload& trans) {
	//Test if active mode is used
	if ( trans.get_gp_option() != tlm::TLM_MIN_PAYLOAD ) {
		cout << getName() <<": debug mode is not supported when communications are inactive."<<endl;
		throw(0);
	}

	//Get information
	tlm::tlm_command cmd = trans.get_command();
	sc_dt::uint64 addr = trans.get_address() / 4;
	unsigned char* ptr = trans.get_data_ptr();
	unsigned int len = trans.get_data_length();

	//Test address
	if ( ( addr >= getSize() ) || ( (addr+len) >= getSize() ) ) {
		trans.set_response_status( tlm::TLM_ADDRESS_ERROR_RESPONSE );
		return 0;
	}

	//TODO : shall rely on read_access_function
	if ( cmd == tlm::TLM_READ_COMMAND ) memcpy( ptr, &mLocalMem [ addr ], len );
	else if ( cmd == tlm::TLM_WRITE_COMMAND ) memcpy( &mLocalMem [ addr ], ptr, len );

	// Successful completion
	trans.set_response_status( tlm::TLM_OK_RESPONSE );

	//End
	return (len);
}

//-----------------------------------------------------------------------------
//Explicit template instantiation
template class TargetIf < uint8_t >;
template class TargetIf < uint16_t >;
template class TargetIf < uint32_t >;
template class TargetIf < uint64_t >;

}

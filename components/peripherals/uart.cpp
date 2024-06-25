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

#include "uart.hpp"
#include "log.hpp"
#include "GlobalPrivate.hpp"

//#include "scriptParam.hpp"
#include "vpsimParam.hpp"
#include <string>

namespace vpsim {

//-----------------------------------------------------------------------------
//Constructor
uart::uart ( sc_module_name Name ) :
	sc_module(Name),
	TargetIf <uint8_t> ( string(Name), 0xfff ),
	mWordLengthInByte ( 1 )
{
	init ();
}

uart::uart ( sc_module_name Name, bool ByteEnable, bool DmiEnable ) :
	sc_module(Name),
	TargetIf <uint8_t> ( string(Name), 0xfff, ByteEnable, DmiEnable ),
	mWordLengthInByte ( 1 )
{
	init ();
}

void uart::init ( ) {
	//Set timings
	TargetIf <uint8_t>::setDmiEnable ( false );

	//Compute number of bytes
	mWordLengthInByte = sizeof ( uint8_t );

	//Instantiate addressable memory
	for(unsigned i=0; i<getSize(); i++) getLocalMem()[i]='\0';

	//Register READ/WRITE methods
	//NEW_CALLBACK (WriteCallBack, this_type, write );
	//NEW_CALLBACK (ReadCallBack, this_type, read );

	TargetIf <uint8_t>::RegisterReadAccess(REGISTER(this_type,read));
	TargetIf <uint8_t>::RegisterWriteAccess(REGISTER(this_type,write));
}

uart::~uart () {

	//printf("\nuart.cpp: destructor \n");

}

// Function name: Core function
tlm::tlm_response_status uart::read ( payload_t & payload, sc_time& delay ) {
	*(int*)(getLocalMem()+4) = getchar();
	return ( tlm::TLM_OK_RESPONSE );
}

tlm::tlm_response_status uart::write ( payload_t & payload, sc_time& delay ) {
	LOG_DEBUG(dbg1) <<getName()<<":---------------------------------------------------------"<<endl;
	LOG_DEBUG(dbg1) <<getName()<<": WRITE access to UART: "<< *payload.ptr << endl;

	//Timing
	//if ( getEnableLatency() ) delay += getWriteLatency();
	if ( getEnableLatency() )
		delay += (getInitialCyclesPerAccess() + getCyclesPerWrite()) * getCycleDuration();	//TODO

	//Print words written in DMI
	char tmp = (char) getLocalMem()[0];
	LOG_DEBUG(dbg1) << tmp << flush;
	cout << tmp << flush;

//	constexpr char raisePattern[] = "!@^~rsp";
//	constexpr char lowerPattern[] = "'?@^lsp";
//
//	static string buffer("1234567");
//	for(size_t i{0} ; i < 6 ; i++){
//		buffer[i] = buffer[i+1];
//	}
//	buffer[6] = tmp;
//
//	if(buffer == raisePattern){
//		RAISE_SIMULATION_PRECISION
//	} else if(buffer == lowerPattern) {
//		LOWER_SIMULATION_PRECISION
//	}

	//End
	return ( tlm::TLM_OK_RESPONSE );
}

}

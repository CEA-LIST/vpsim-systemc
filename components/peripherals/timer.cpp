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

#include "timer.hpp"
#include "GlobalPrivate.hpp"

namespace vpsim {

//-----------------------------------------------------------------------------
//Constructor (synchronous)
timer::timer ( sc_module_name  Name, uint32_t Quantum ) :
		sc_module ( Name ),
		TargetIf <uint32_t> ( string(Name), 0x4 ),
		mCurrentTime ( SC_ZERO_TIME ),
		mNbWatchdogs ( 1 ),
		//mQuantum ( Quantum ),
		mSeparateIntLines ( false ),
		mTimerSize ( 64 )
{
	// Set up threads
	SC_THREAD ( CoreFunc );

	// Register the blocking transport method
	TargetIf <uint32_t>::setDmiEnable ( false );
	TargetIf <uint32_t>::RegisterReadAccess ( REGISTER ( this_type, read ) );
	TargetIf <uint32_t>::RegisterWriteAccess ( REGISTER ( this_type, write ) );

	//Create watchdogs
	mWatchdogs = new uint64_t [ mNbWatchdogs ];
	bzero( (uint32_t *)&mWatchdogs, sizeof( mWatchdogs ));
}


//Constructor (synchronous)
timer::timer ( sc_module_name  Name, uint32_t nb_watchdogs, uint32_t Quantum ) :
		sc_module ( Name ),
		TargetIf <uint32_t> ( string(Name), 0x4 ),
		mCurrentTime ( SC_ZERO_TIME ),
		mNbWatchdogs ( nb_watchdogs ),
		//mQuantum ( Quantum ),
		mSeparateIntLines ( false ),
		mTimerSize ( 64 )
{
	// Set up threads
	SC_THREAD ( CoreFunc );

	// Register the blocking transport method
	TargetIf <uint32_t>::setDmiEnable ( false );
	TargetIf <uint32_t>::RegisterReadAccess ( REGISTER ( this_type, read ) );
	TargetIf <uint32_t>::RegisterWriteAccess ( REGISTER ( this_type, write ) );

	//Test nb watchdogs (limited to 32)
	if ( mNbWatchdogs > 32 ) {
		cerr<<getName()<<": error, watchdogs are limited to 32 in timer."<<endl; throw(0);
	}

	//Create watchdogs
	mWatchdogs = new uint64_t [ mNbWatchdogs ];
	bzero( (uint32_t *)&mWatchdogs, sizeof( mWatchdogs ));
}

timer::~timer ( ) {
	delete [] mWatchdogs;
}


//-----------------------------------------------------------------------------
//Set functions
void
timer::setTimerSize ( uint32_t TimerSize ) {
	switch ( TimerSize ) {
		case 8 : mTimerSize = 8; break;
		case 16 : mTimerSize = 16; break;
		case 24 : mTimerSize = 24; break;
		case 32 : mTimerSize = 32; break;
		case 48 : mTimerSize = 48; break;
		case 64 : mTimerSize = 64; break;
		default : {
			cerr<<getName()<<": error, timer sizes are limited to 8, 16, 24, 32, 48 or 64 bits."<<endl; throw(0);
		}
	}
}

void timer::setSeparateIntLines ( bool SeparateIntLines ) { mSeparateIntLines = SeparateIntLines; }

//Get functions
uint32_t timer::getTimerSize ( ) { return ( mTimerSize ); }

bool timer::getSeparateIntLines ( ) { return ( mSeparateIntLines ); }

//Reset function
void timer::Reset ( ) {
	bzero( (uint32_t *)&mWatchdogs, sizeof( mWatchdogs ));
	mIntr.write ( false );
}


//-----------------------------------------------------------------------------
void timer::CoreFunc () { }


//-----------------------------------------------------------------------------
tlm::tlm_response_status timer::read ( payload_t & payload, sc_time & delay ) {
	return ( tlm::TLM_OK_RESPONSE );
}

tlm::tlm_response_status timer::write ( payload_t & payload, sc_time & delay ) {
	return ( tlm::TLM_OK_RESPONSE );
}

}

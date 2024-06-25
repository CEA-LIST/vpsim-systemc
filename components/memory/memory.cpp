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

#include "memory.hpp"
#include "GlobalPrivate.hpp"
namespace vpsim
{
//---------------------------------------------------
//Constructor
memory::memory ( sc_module_name Name, uint64_t Size ) :
	sc_module(Name),
	TargetIf <unsigned char> ( string(Name), Size ),
	mWordLengthInByte ( 4 )
{
	Init ();
}

memory::memory( sc_module_name Name, uint64_t Size, bool ByteEnable, bool DmiEnable ) :
	sc_module(Name),
	TargetIf <unsigned char> ( string(Name), Size, ByteEnable, DmiEnable ),
	mWordLengthInByte ( 4 )
{
	Init ();
}

void memory::Init ( ) {
	//Set timings
	setDmiEnable(true);

	//Compute number of bytes of a word
	mWordLengthInByte = sizeof(unsigned char);

	//Initialization of ELF loader
	elfloader_init ( (char*) getLocalMem(), getSize() );

	//Register READ/WRITE methods
	RegisterReadAccess ( REGISTER ( this_type, read ) );
	RegisterWriteAccess ( REGISTER ( this_type, write ) );

}

memory::~memory () {
	//printf("\nmemory.cpp: destructor \n");

}

//Debug function
void memory::Dump ( uint64_t StartAddress, uint64_t EndAddress ) {

	//check that address range match the memory block
	if ( StartAddress < getBaseAddress() ||
		 EndAddress > getEndAddress()    ||
		 EndAddress < StartAddress ){
		std::cerr<<getName()<<": The memory dump cannot be performed. The memory size does not correspond."<< std::endl;
    	throw(0);
	}

	for (uint64_t i= StartAddress; i< EndAddress; i+=4 ) {
		cout << "[0x" <<hex<< i <<dec<< "]=0x"<<hex;
		fprintf(stdout,"%02x",(uint32_t) (getLocalMem()[i+3]));
		fprintf(stdout,"%02x",(uint32_t) (getLocalMem()[i+2]));
		fprintf(stdout,"%02x",(uint32_t) (getLocalMem()[i+1]));
		fprintf(stdout,"%02x",(uint32_t) (getLocalMem()[i]));
		cout<<dec<< std::endl;
	}
}

void memory::loadBlob(const string filename, const uint64_t init_off) {
	FILE* inFile = fopen(filename.c_str(), "rb");
	if (!inFile)
		throw runtime_error(string("Unable to open blob file: ") + filename);
	const size_t bufSize = 2048;
	size_t nr=0;
	size_t off=init_off;
	while ((nr=fread(getLocalMem()+off, sizeof(char), bufSize, inFile)) == bufSize) {off+=nr;
	//if(off-nr>=0x01FDFF0) cout<<hex<<"Loaded :"<<nr<< " bytes "<<*(uint32_t*)(getLocalMem()+off-nr)<<" @"<<off-nr<<dec<<endl;
	}
	//cout<<hex<<" initially = "<<*(uint32_t*)&getLocalMem()[0x01FDFF0]<<dec<<endl;
	fclose(inFile);
}


//---------------------------------------------------
//Main functions
tlm::tlm_response_status memory::read ( payload_t & payload, sc_time & delay ) {
	//Compute delay time (can be more accurate)

	if ( getEnableLatency() ) {
		size_t accessed=0;
		do
		{
			accessed += mWordLengthInByte;
			//delay += (getInitialCyclesPerAccess() + getCyclesPerRead()) * getCycleDuration();
			delay += ReadLatency;
		} while (accessed < payload.len);
	}

	payload.dmi = true;

	if (payload.ptr==NULL){
		return tlm::TLM_OK_RESPONSE;
	}


	//Test data allocated buffer
	//if (payload.ptr==NULL) {
	//	cerr<<getName()<<": error, data pointer not initialized in payload."<<endl; throw(0);
	//}

	//Read word
	unsigned char * tmp = new unsigned char [ payload.len ];
	for ( size_t i=0 ; i<payload.len ; i++ ) tmp [i] = getLocalMem()[ payload.addr-getBaseAddress()+i ];
    BYTE * tmp_ptr = (BYTE*) tmp;
	memcpy ( payload.ptr, tmp_ptr, payload.len );
	delete [] tmp;


	//End
	return ( tlm::TLM_OK_RESPONSE );
}

tlm::tlm_response_status memory::write ( payload_t & payload, sc_time & delay ) {
	//Compute delay time (can be more accurate)
	if ( getEnableLatency() ) {
		size_t accessed=0;
		do
		{
			accessed += mWordLengthInByte;
			//delay += (getInitialCyclesPerAccess() + getCyclesPerRead()) * getCycleDuration();
			delay += WriteLatency;
		} while (accessed < payload.len);
	}

	payload.dmi = true;

	if (payload.ptr==NULL){
		return tlm::TLM_OK_RESPONSE;
	}

	//Test data allocated buffer
	//if (payload.ptr==NULL) {
	//	cerr<<getName()<<": error, data pointer not initialized in payload."<<endl; throw(0);
	//}

	//Write word
	BYTE * tmp = new BYTE [ payload.len ];
	memcpy (tmp, payload.ptr, payload.len);
	unsigned char * tmp_ptr = (unsigned char*) tmp;
	for ( size_t i=0 ; i<payload.len ; i++ ) getLocalMem()[ payload.addr - getBaseAddress() + i ] = (unsigned char) tmp_ptr [i];
	delete [] tmp;

	//End
	return ( tlm::TLM_OK_RESPONSE );
}

}

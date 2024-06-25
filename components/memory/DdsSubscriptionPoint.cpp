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

#include "DdsSubscriptionPoint.hpp"
#include "EndianHelper.hpp"
#include <stdlib.h>
#include <malloc.h>
#include "GlobalPrivate.hpp"

namespace vpsim
{


DdsSubscriptionPoint::DdsSubscriptionPoint( sc_module_name Name, uint64_t Size):
				    sc_module(Name),
				    TargetIf(string(Name), Size ),
					SubscriptionPointAdv()
{
	// TODO Auto-generated constructor stub
	vpsim::DebugStream<<"generating a new DdsSubscriptionPoint"<<endl;

	//Register READ/WRITE methods
	RegisterReadAccess ( REGISTER ( DdsSubscriptionPoint, read ) );
	RegisterWriteAccess ( REGISTER ( DdsSubscriptionPoint, write ) );

	gettimeofday(&LastReadDDSTime, NULL);
}

DdsSubscriptionPoint::~DdsSubscriptionPoint() {
	// TODO Auto-generated destructor stub
}



tlm::tlm_response_status DdsSubscriptionPoint::read ( payload_t & payload, sc_time & delay ) {


	//Test if inactive communication first
	if ( !payload.get_is_active() ) {
		return ( tlm::TLM_OK_RESPONSE );
	}

	//Test data allocated buffer
	if (payload.ptr==NULL) {
		cerr<<getName()<<": error, data pointer not initialized in payload."<<endl; throw(0);
	}


//	cout<<"access to DDS Sub in read mode @"<<std::hex<<payload.addr<<" len is "<<payload.len<<std::dec<<endl;
	memcpy ( payload.ptr, &(getLocalMem()[payload.addr - getBaseAddress()]), payload.len );
//	cout<<"\t read data ";
//	for(unsigned i=0; i<payload.len; i++) cout<<hex<<" 0x"<<(int)payload.ptr[i];
//	cout<<dec<<endl;

	//End
	return ( tlm::TLM_OK_RESPONSE );
}

tlm::tlm_response_status DdsSubscriptionPoint::write ( payload_t & payload, sc_time & delay ) {

//	cout<<"access to DDS Sub in write mode @"<<std::hex<<payload.addr<<" len is "<<payload.len<<std::dec<<endl;

	//Test if inactive communication first
	//inactive communication can occur for DMI access with force LT
	if ( !payload.get_is_active() ) {

		cerr<<"DDS Slaves do not support inactive communications (e.g DMI), undefined behavior"<<endl;
		return ( tlm::TLM_OK_RESPONSE );
	}

	//Test data allocated buffer
	if (payload.ptr==NULL) {
		cerr<<getName()<<": error, data pointer not initialized in payload."<<endl; throw(0);
	}


	//Handle control register addressing
	if(payload.addr == this->getBaseAddress() && (payload.len==4)){ //port
		if(payload.len==4){
			//this->SetSubscriptionPort( (*(uint32_t*)payload.ptr));
			std::cout<<"SetSubscriptionPort to "<<EndianHelper::GuestToHost<unsigned int, true,true>(payload.ptr, payload.len)<<std::endl;
			this->SetSubscriptionPort( EndianHelper::GuestToHost<unsigned int, true,true>(payload.ptr, payload.len));
		}
	}
	else if (payload.addr == this->getBaseAddress() + 0x4 ){ //HostName
		assert(payload.len==4);
		char c = (char) (*(uint32_t*)payload.ptr);
		if (c=='\0'){
			cout<<"calling SetSubscriptionHost with host "<<HostName.str()<<endl;
			this->SetSubscriptionHost(HostName.str());
		}else{
			HostName<<c;
		}
	}
	else if (payload.addr == this->getBaseAddress() + 0x8){ //Share name
		assert(payload.len==4);
		char c = (char) (*(uint32_t*)payload.ptr);
		if (c=='\0'){
			std::cout<<"SetNameOfShareMemory to "<<SubName.str()<<std::endl;
			this->SetNameOfShareMemory(SubName.str());
		}else{
			SubName<<c;
		}
	}
	else if (payload.addr == this->getBaseAddress() + 0xC){ //
		assert(payload.len==4);
		bool b = (bool) (*(uint32_t*)payload.ptr);
		usleep(1000);

		this->SetReceiveStatus(b);
	}
	else if (payload.addr == this->getBaseAddress() + 0x10){
		assert(payload.len==4);

		void* BufferZone = &(getLocalMem()[+0x18]);
		void* Buffer = nullptr;


		long int elapsedTime;
		timeval CurrentTime;
		gettimeofday(&CurrentTime, NULL);

		elapsedTime = (CurrentTime.tv_usec - LastReadDDSTime.tv_usec);      //microseconds diff
		elapsedTime += (CurrentTime.tv_sec - LastReadDDSTime.tv_sec)*1000000; //full seconds diff
		LastReadDDSTime =CurrentTime;

		DebugStream<<getName()<<" duration (µs) btw 2 ReadDDS : "<<elapsedTime<<endl;

		//std::cout<<"call ReadDDS "<<std::endl;
		bool status = this->ReadDDS(Buffer);
		if(!status) cout<<"failed to read new DDS buffer, continue nonetheless"<<endl;
//		while(!this->ReadDDS(Buffer)){
//			cout<<"failed to read new DDS buffer, retry"<<endl;
//		}

		//std::cout<<"ReadDDS return status "<<status<<std::endl;

		//assert(this->Ptr()!=0);
		//if(!this->Ptr()) return ( tlm::TLM_OK_RESPONSE );
		int TimeOutCount=0;
		while(!this->Ptr() && TimeOutCount<10000){
			status = this->ReadDDS(Buffer);
			TimeOutCount++;
		}

		if(!this->Ptr()){
			cout<<getName()<<" failed to ReadDDS, end of simulation"<<endl;
			sc_stop();
			wait(10,SC_NS);
			return ( tlm::TLM_OK_RESPONSE );
		}

		//size_t DataSize = this->Ptr()->dataSize();
		//Ptr datatype is not defined so we cannot use above line
		size_t DataSize = malloc_usable_size(Buffer);

		//std::cout<<"ReadDDS DataSize is "<<DataSize<<std::endl;

		assert(Buffer != 0 and DataSize>0 and DataSize < (getSize() - 0x18));
		memcpy ( BufferZone,Buffer,(unsigned) DataSize);
		free(Buffer);
		Buffer= nullptr;

		//debug only force write of value to memory
		//uint64_t * tmp = (uint64_t*) BufferZone;
		//*tmp = 0x0102030405060708;
	}


	//Write word
	//BYTE * tmp = new BYTE [ payload.len ];
	//memcpy (tmp, payload.ptr, payload.len);
	//unsigned char * tmp_ptr = (unsigned char*) tmp;
	//delete [] tmp;

	//for ( int i=0 ; i<payload.len ; i++ ) local_mem [payload.addr - getBaseAddress() +i] = payload.ptr[i];
	memcpy ( &(getLocalMem()[payload.addr - getBaseAddress()]),payload.ptr, payload.len );

	//End
	return ( tlm::TLM_OK_RESPONSE );
}



}//end vpsim namespace

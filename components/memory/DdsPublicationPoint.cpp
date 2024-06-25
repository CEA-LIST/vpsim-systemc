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

#include "DdsPublicationPoint.hpp"
#include "EndianHelper.hpp"
#include "GlobalPrivate.hpp"

namespace vpsim {

DdsPublicationPoint::DdsPublicationPoint( sc_module_name Name, uint64_t Size) :
						    sc_module(Name),
						    TargetIf(string(Name), Size ),
							PublicationPointAdv()
{
	// TODO Auto-generated constructor stub
	vpsim::DebugStream<<"generating a new DdsPublicationPoint"<<endl;

	//Register READ/WRITE methods
	RegisterReadAccess ( REGISTER ( DdsPublicationPoint, read ) );
	RegisterWriteAccess ( REGISTER ( DdsPublicationPoint, write ) );
}

DdsPublicationPoint::~DdsPublicationPoint() {
	// TODO Auto-generated destructor stub
}

tlm::tlm_response_status DdsPublicationPoint::read ( payload_t & payload, sc_time & delay ) {

	//Test if inactive communication first
	if ( !payload.get_is_active() ) {
		return ( tlm::TLM_OK_RESPONSE );
	}

	//Test data allocated buffer
	if (payload.ptr==NULL) {
		cerr<<getName()<<": error, data pointer not initialized in payload."<<endl; throw(0);
	}

	//direct copy from memory
	//for ( int i=0 ; i<payload.len ; i++ ) payload.ptr[i] = local_mem [payload.addr - get_base_address() +i];
	memcpy ( payload.ptr, &(getLocalMem()[payload.addr - getBaseAddress()]), payload.len );

	//End
	return ( tlm::TLM_OK_RESPONSE );
}

tlm::tlm_response_status DdsPublicationPoint::write ( payload_t & payload, sc_time & delay ) {

	//cout<<"access to DDS Pub in write mode @"<<std::hex<<payload.addr<<" len is "<<payload.len<<std::dec<<endl;

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


	//Handle special register addresses for DDS

	if(payload.addr == this->getBaseAddress() && (payload.len==4)){ //SetNameOfShareMemory
		assert(payload.len==4);
		char c = (char) (*(uint32_t*)payload.ptr);
		if (c=='\0'){
			std::cout<<"SetNameOfShareMemory to "<<SubName.str()<<std::endl;
			this->SetNameOfShareMemory(SubName.str());
		}else{
			SubName<<c;
		}
	}
	else if (payload.addr == this->getBaseAddress() + 0x4 ){ //SizeOfShare
		unsigned int DataSize = EndianHelper::GuestToHost<unsigned int, true,true>(payload.ptr, payload.len);
		std::cout<<"PublicationPoint data size set to "<<DataSize<<std::endl;
		this->SetDataSize(DataSize);
	}
	else if (payload.addr == this->getBaseAddress() + 0x8 ){ //SetSendStatus
		bool Status = EndianHelper::GuestToHost<unsigned int, true,true>(payload.ptr, payload.len);
		std::cout<<"PublicationPoint set status to "<<Status<<std::endl;
		this->SetSendStatus(Status);
	}
	else if(payload.addr == this->getBaseAddress() + 0xC ){ //WriteDDS
		assert(payload.len==4);

		void* BufferZone = &(getLocalMem()[+0x10]);

		//std::cout<<"call WriteDDS "<<std::endl;
		bool status=this->WriteDDS(BufferZone);
		if(!status) cout<<"failed to write new DDS buffer, continue nonetheless"<<endl;
		//std::cout<<"WriteDDS return status "<<status<<std::endl;

//		void* Buffer = nullptr;
//		std::cout<<"call ReadDDS "<<std::endl;
//		bool status = this->ReadDDS(Buffer);
//		std::cout<<"ReadDDS return status "<<status<<std::endl;
//
//		assert(this->Ptr()!=0);
//		//size_t DataSize = this->Ptr()->dataSize();
//		//Ptr datatype is not defined so we cannot use above line
//		size_t DataSize = malloc_usable_size(Buffer);
//
//		assert(Buffer != 0 and DataSize>0 and DataSize < (this->SIZE - 0x14));
//		memcpy ( BufferZone,Buffer,(unsigned) DataSize);
	}

	//for ( int i=0 ; i<payload.len ; i++ ) local_mem [payload.addr - get_base_address() +i] = payload.ptr[i];
	memcpy ( &(getLocalMem()[payload.addr - getBaseAddress()]),payload.ptr, payload.len );

	//End
	return ( tlm::TLM_OK_RESPONSE );
}


} /* namespace vpsim */

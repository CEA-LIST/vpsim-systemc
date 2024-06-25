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

#ifndef NOC_BASIC_TYPES_HPP
#define NOC_BASIC_TYPES_HPP

#include <systemc>
#include <tlm>
#include <stdlib.h>
#include <map>
//#include <tlm.h>
//#include "ac_tlm_protocol.H"

//using namespace std;
//using tlm::tlm_transport_if;

namespace vpsim
{

//-----------------------------------------------------------//
// Useful macros
//-----------------------------------------------------------//
#define SYSTEMC_ERROR(CONST_MSG) {cerr<<"SYSTEMC_ERROR In "<<this->name()<<" @t="<<sc_time_stamp()<<": "<<CONST_MSG<<" ("<<__FILE__<<":"<<__LINE__<<")"<<endl;exit(EXIT_FAILURE);}
#define SYSTEMC_WARN(CONST_MSG) {cerr<<"SYSTEMC_WARNING In "<<this->name()<<" @t="<<sc_time_stamp()<<": "<<CONST_MSG<<" ("<<__FILE__<<":"<<__LINE__<<")"<<endl;}
#define SYSTEMC_INFO(CONST_MSG) {cout<<"SYSTEMC_INFO In "<<this->name()<<" @t="<<sc_time_stamp()<<": "<<CONST_MSG<<" ("<<__FILE__<<":"<<__LINE__<<")"<<endl;}

//debug for all routers
//#define SYSTEMC_DEBUG_ROUTER SYSTEMC_INFO
//no debug
#define SYSTEMC_DEBUG_ROUTER(CONST_MSG) {}
//debug for specific routers
//#define SYSTEMC_DEBUG_ROUTER(CONST_MSG) if(this->Id==12){SYSTEMC_INFO(CONST_MSG)}

//for traffic generator message
#define SYSTEMC_TRAFFIC_GEN(CONST_MSG) {}
//#define SYSTEMC_TRAFFIC_GEN SYSTEMC_INFO

//for router access stats (CA model)
#define SYSTEMC_ROUTER_ACCESS_STATS(CONST_MSG) {}

#define SYSTEMC_WRAPPER_CA(CONST_MSG) {}

#define STORE_NOC_STATS

#define BEGIN_MEMORY_PROTECT {}
#define END_MEMORY_PROTECT {}

//-----------------------------------------------------------//
// Types / Class definition
//-----------------------------------------------------------//
typedef unsigned int T_RouterID;
typedef unsigned int T_PortID;
typedef unsigned int T_LinkID;
typedef unsigned int T_SlavePortID;
//typedef std::pair<T_RouterID, T_SlavePortID> T_TargetID;



class C_TargetID:public std::pair<T_RouterID, T_SlavePortID>{

	static std::map<C_TargetID, unsigned> TargetToCMUEndPointID;
	static unsigned NextCMUEndPointID;

public:
	C_TargetID():std::pair<T_RouterID, T_SlavePortID>(){};

	C_TargetID(T_RouterID RID, T_SlavePortID SPID, bool IsNew=false):std::pair<T_RouterID, T_SlavePortID>(RID,SPID){
		if (IsNew)
		{
			TargetToCMUEndPointID[*this]=NextCMUEndPointID++;
		}
	}

	unsigned int GetCMUEndPointIDTarget(){
		return TargetToCMUEndPointID[*this];
	}
};

typedef C_TargetID T_TargetID;

//typedef long CycleCount;
typedef double CycleCount;

typedef unsigned int T_MemoryAddress;
typedef std::pair<T_MemoryAddress, T_MemoryAddress> T_MemoryRegion; // (address begin, address end)
typedef std::map<T_TargetID, T_MemoryRegion > T_MemoryMap;


class NoCFlit {
	public:
	T_TargetID TargetId; //used
	T_TargetID SrcId; //for return path
	T_RouterID PrevRouterId; //the router from which it originates
	T_PortID CurrentInputPortID; //used for sorting in the current router (cycle accurate model)
	bool Last; //is it the last packet from a burst
	const tlm::tlm_generic_payload * req; //to keep the ac_tlm_if request
	tlm::ac_tlm_rsp rsp; //to keep the ac_tlm_if response
	bool IsFW; //TODO delete as it must no longer be used
	sc_time EmissionTimeStamp;

	friend ostream & operator <<(ostream & os, NoCFlit nf)
	{
		os<<"NoCFlit ";
		if(nf.IsFW)
		{
			os<<"FW";
		}
		else
		{
			os<<"BW";
		}
		os<<": src "<<nf.SrcId.first<<" -> "<<nf.TargetId.first<<" (port"<<nf.TargetId.second<<")";
		return os;
	};

	void CMUDump()
	{
		unsigned TargetIDCMU=TargetId.GetCMUEndPointIDTarget(); //(TargetId.first<<4) + TargetId.second;
		//cout<<" flit.TargetId.first "<<TargetId.first<<"flit.TargetId.second "<< TargetId.second<<"TargetIDCMU"<<TargetIDCMU<<endl;
		unsigned SourceIDCMU=SrcId.GetCMUEndPointIDTarget(); //(SrcId.first<<4) + SrcId.second;

		// if (SrcId.second>0xf || SrcId.first>0xf){
		// 	cerr<<"Invalid router id or port id for SourceIDCMU generation : "<<SrcId.first<<","<<SrcId.second<<endl;
		// 	exit(EXIT_FAILURE);
		// }
		// if (TargetId.second>0xf || TargetId.first>0xf){
		// 	cerr<<"Invalid router id or port id for TargetIDCMU generation : "<<TargetId.first<<","<<TargetId.second<<endl;
		// 	exit(EXIT_FAILURE);
		// }

		//printf("TargetIDCMU %02x %x %x \n",TargetIDCMU, TargetId.first ,TargetId.second);
		//printf("SourceIDCMU %02x %x %x \n",SourceIDCMU, SrcId.first ,SrcId.second);

		//cout<<" flit.SrcId.first "<<SrcId.first<<"flit.SrcId.second "<< SrcId.second<<"SourceIDCMU"<<SourceIDCMU<<endl;
		printf("%01x%02x%02x%01x%08x\n",Last, /*src*/ SourceIDCMU, /*dst*/ TargetIDCMU, /*vc*/ 0, /*injection_cycle*/ (int) (sc_time_stamp().to_double()/10) );
		//cout<<"timestamp debug"<<(int) (sc_time_stamp().to_double()/10)<<" sc_time"<<sc_time_stamp()<<endl;

	};

};


//class NoCResponse {
//	public:
//		bool Success;
//};
//
//typedef tlm_transport_if < NoCFlit,NoCResponse> tlm_noc_if;


class TLMMasterBindInfo{
public:
	 sc_port<ac_tlm_transport_if>* MasterPort;
	 T_RouterID RouterID;
	 T_PortID RouterFWPort;
	 T_PortID RouterBWPort;
};

class TLMSlaveBindInfo{
public:
	sc_export<ac_tlm_transport_if>* SlavePort;
	 T_RouterID RouterID;
	 T_PortID RouterFWPort;
	 T_PortID RouterBWPort;
};

class CABAMasterBindInfo{
public:
	sc_fifo_out<NoCFlit>* Master;
	T_RouterID RouterID;
	T_PortID InPortID;
};

class CABASlaveBindInfo{
public:
	sc_fifo_in<NoCFlit>* Slave;
	T_RouterID RouterID;
	T_PortID OutPortID;
};



//TODO remove from code (use NoCFlit structs instead)
// class C_Req
// {
// 	private:
// 		unsigned int SrcID;
// 		unsigned int DestID;
// 		CycleCount InitLocalTime;

// 		//static unsigned int InstancesCounter;
		

// 	public:
// 		C_Req(unsigned int _SrcID, unsigned int _DestID, CycleCount _InitLocalTime)
// 		{
// 			//constants for this request
// 			SrcID=_SrcID;
// 			DestID=_DestID;
// 			InitLocalTime=_InitLocalTime;

// 			//InstancesCounter++;
// 		}

// 		unsigned int  GetSrcID()
// 		{
// 			return SrcID;
// 		}

// 		unsigned int  GetDestID()
// 		{
// 			return DestID;
// 		}

// 		CycleCount GetInitLocalTime()
// 		{
// 			return InitLocalTime;
// 		}
// };

};//end namespace vpsim

#endif //NOC_BASIC_TYPES_HPP

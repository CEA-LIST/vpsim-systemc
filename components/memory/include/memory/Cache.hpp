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

#ifndef CACHE_HPP
#define CACHE_HPP

#include "global.hpp"
#include "CacheBase.hpp"
#include "InitiatorIf.hpp"
#include "TargetIf.hpp"
#include "DmiKeeper.hpp"
#include "MainMemCosim.hpp"
#include "CoherenceExtension.hpp"
#include <functional>
#include "log.hpp"

#define begin_uncached_regions(cache) cache->is_uncached_region=[](uint64_t a){return (false
#define region(base, size) ||(a>=base && a<(base+size))
#define end_uncached_regions );};

namespace vpsim{

template <typename AddressType,
          typename WordType,
          bool WCETMode=false>

class Cache :
    public CacheBase<AddressType, WordType, WCETMode>,
    public tlm::tlm_bw_transport_if<tlm::tlm_base_protocol_types>,
    public tlm::tlm_fw_transport_if<tlm::tlm_base_protocol_types>,
    public DmiKeeper
{

public:

  Cache(sc_module_name name,
        sc_time latency,
        unsigned CacheSize,
        unsigned CacheLineSize,
        unsigned Associativity,
        unsigned NbInterleavedCaches,
        CacheReplacementPolicy ReplPolicy = LRU,
        CacheWritePolicy writePolicy = WBack,
        CacheAllocPolicy allocPolicy = WAllocate,
        bool dataSupport = false,
        idx_t id = NULL_IDX,
        const uint32_t level = 1,
        const uint32_t nin = 1,
        const uint32_t nout = 1,
        CacheInclusionPolicy inclusionOfHigher = NINE,
        CacheInclusionPolicy inclusionOfLower = NINE,
        bool isHome = false,
        bool isCoherent = false):
    CacheBase<AddressType, WordType, WCETMode>
  (name, CacheSize, CacheLineSize, Associativity, NbInterleavedCaches, ReplPolicy, writePolicy, allocPolicy,
   dataSupport, level, inclusionOfHigher, inclusionOfLower, isHome, isCoherent, id)
    , DmiKeeper (1)
    , DataSupport (dataSupport)
    , Level (level)
    , IsHome (isHome)
    , Latency (latency)
    , Fwd (0)
    , NUM_PORT_IN (nin)
    , NUM_PORT_OUT (nout)
  {
    isPriv=false;
    char name_socket [100];
    assert ((NUM_PORT_IN>0) && (NUM_PORT_IN<4));
    assert ((NUM_PORT_OUT>0) && (NUM_PORT_OUT<3));
    for (size_t i=0; i<NUM_PORT_OUT; i++){
      sprintf (name_socket, "socket_out[%zu]", i);
      socket_out.emplace_back(name_socket);
      socket_out[i].register_invalidate_direct_mem_ptr (this, &Cache::invalidate_direct_mem_ptr);
    }
    for (size_t i=0; i<NUM_PORT_IN; i++){
      sprintf (name_socket, "socket_in[%zu]",i);
      socket_in.emplace_back(name_socket);
      socket_in[i].register_get_direct_mem_ptr (this, &Cache::get_direct_mem_ptr);
      socket_in[i].register_transport_dbg (this, &Cache::transport_dbg);
    }
    for (size_t i=0; i<NUM_PORT_IN; i++)
      socket_in[i].register_b_transport (this, &Cache::b_transport);
    //socket_in[0].register_b_transport (this, &Cache::b_transport);
    //if (NUM_PORT_IN==2) socket_in[1].register_b_transport (this, &Cache::b_transport);
    is_uncached_region =[](AddressType t) {return false;};
  }

  inline tlm::tlm_response_status ForwardRead (AddressType addr, size_t size, sc_time& delay, sc_time timestamp) override {
    return send_transaction (NULL, addr, size, NULL_IDX, tlm::TLM_READ_COMMAND, delay, timestamp);
  }
  inline tlm::tlm_response_status ForwardWrite (AddressType addr, size_t size, sc_time& delay, sc_time timestamp) override {
    return send_transaction (NULL, addr, size, NULL_IDX, tlm::TLM_WRITE_COMMAND, delay, timestamp);
  }
  inline tlm::tlm_response_status ForwardReadData (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterId, sc_time& delay, sc_time timestamp) override {
      if (this->DataSupport) return send_transaction (lineDataPtr, addr, size, requesterId, tlm::TLM_READ_COMMAND, delay, timestamp);
      else                   return send_transaction (NULL       , addr, size, requesterId, tlm::TLM_READ_COMMAND, delay, timestamp);
  }
  inline tlm::tlm_response_status BackwardRead (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterId, set<idx_t> targetIds, sc_time& delay, sc_time timestamp) override {
    assert (targetIds.size()>=0);
    if (this->DataSupport) return send_readback_transaction (lineDataPtr, addr, size, requesterId, targetIds, delay, timestamp);
    else                   return send_readback_transaction (NULL       , addr, size, requesterId, targetIds, delay, timestamp);
  }
  inline tlm::tlm_response_status ForwardWriteData (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterId, sc_time& delay, sc_time timestamp) override {
      if (this->DataSupport) return send_transaction (lineDataPtr, addr, size, requesterId, tlm::TLM_WRITE_COMMAND, delay, timestamp);
      else                   return send_transaction (NULL       , addr, size, requesterId, tlm::TLM_WRITE_COMMAND, delay, timestamp);
  }
  /*inline tlm::tlm_response_status BackInvalidate (AddressType addr, sc_time& delay) override {
    return send_invalidate_transaction (addr, delay);
    }*/
  inline tlm::tlm_response_status BackInvalidate (AddressType addr, set<idx_t> targetIds, sc_time& delay, sc_time timestamp) override {
    assert (targetIds.size()>=0);
    return send_invalidate_transaction (addr, targetIds, delay, timestamp);
  }
  inline tlm::tlm_response_status ForwardEvict (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterID, sc_time& delay, sc_time timestamp) override {
    if (this->DataSupport) return send_evict_transaction (lineDataPtr, addr, size, requesterID, delay, timestamp);
    else                   return send_evict_transaction (NULL       , addr, size, requesterID, delay, timestamp);
  }
  inline tlm::tlm_response_status SendGetS (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterID, sc_time& delay, sc_time timestamp) override {
    if (this->DataSupport) return send_coherence_transaction (lineDataPtr, addr, size, requesterID, this->Id, std::set<idx_t>(), GetS, delay, timestamp);
    else                   return send_coherence_transaction (NULL       , addr, size, requesterID, this->Id, std::set<idx_t>(), GetS, delay, timestamp);
  }
  inline tlm::tlm_response_status SendGetM (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterID, sc_time& delay, sc_time timestamp) override {
    if (this->DataSupport) return send_coherence_transaction (lineDataPtr, addr, size, requesterID, this->Id, std::set<idx_t>(), GetM, delay, timestamp);
    else                   return send_coherence_transaction (NULL       , addr, size, requesterID, this->Id, std::set<idx_t>(), GetM, delay, timestamp);
  }
  inline tlm::tlm_response_status SendPutS (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterID, sc_time& delay, sc_time timestamp) override {
    if (this->DataSupport) return send_coherence_transaction (lineDataPtr, addr, size, requesterID, this->Id, std::set<idx_t>(), PutS, delay, timestamp);
    else                   return send_coherence_transaction (NULL       , addr, size, requesterID, this->Id, std::set<idx_t>(), PutS, delay, timestamp);
  }
  inline tlm::tlm_response_status SendPutM (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterID, sc_time& delay, sc_time timestamp) override {
    if (this->DataSupport) return send_coherence_transaction (lineDataPtr, addr, size, requesterID, this->Id, std::set<idx_t>(), PutM, delay, timestamp);
    else                   return send_coherence_transaction (NULL       , addr, size, requesterID, this->Id, std::set<idx_t>(), PutM, delay, timestamp);
  }
  inline tlm::tlm_response_status SendFwdGetS (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterID, set<idx_t> targetIds /*,const int targetId*/, sc_time& delay, sc_time timestamp) override {
    //assert (targetId!=NULL_IDX);
    if (this->DataSupport) return send_coherence_transaction (lineDataPtr, addr, size, requesterID, this->Id, targetIds, FwdGetS, delay, timestamp);
    else                   return send_coherence_transaction (NULL       , addr, size, requesterID, this->Id, targetIds, FwdGetS, delay, timestamp);
  }
  inline tlm::tlm_response_status SendFwdGetM (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterID, set<idx_t> targetIds, sc_time& delay, sc_time timestamp) override {
    //assert (targetIds!=NULL_IDX);
    if (this->DataSupport) return send_coherence_transaction (lineDataPtr, addr, size, requesterID, this->Id, targetIds, FwdGetM, delay, timestamp);
    else                   return send_coherence_transaction (NULL       , addr, size, requesterID, this->Id, targetIds, FwdGetM, delay, timestamp);
  }
  inline tlm::tlm_response_status SendPutI (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterID, set<idx_t> targetIds, sc_time& delay, sc_time timestamp) override {
    assert (targetIds.size()>0);
    if (this->DataSupport) return send_coherence_transaction (lineDataPtr, addr, size, requesterID, this->Id, targetIds, PutI, delay, timestamp);
    else                   return send_coherence_transaction (NULL       , addr, size, requesterID, this->Id, targetIds, PutI, delay, timestamp);
  }
  inline tlm::tlm_response_status SendInvS (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterID, set<idx_t> targetIds, sc_time& delay, sc_time timestamp) override {
    assert (targetIds.size()>0);
    if (this->DataSupport) return send_coherence_transaction (lineDataPtr, addr, size, requesterID, this->Id, targetIds, InvS, delay, timestamp);
    else                   return send_coherence_transaction (NULL       , addr, size, requesterID, this->Id, targetIds, InvS, delay, timestamp);
  }
  inline tlm::tlm_response_status SendInvM (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterID, idx_t targetId, sc_time& delay, sc_time timestamp) override {
    assert (targetId!=NULL_IDX);
    if (this->DataSupport) return send_coherence_transaction (lineDataPtr, addr, size, requesterID, this->Id, {targetId}, InvM, delay, timestamp);
    else                   return send_coherence_transaction (NULL       , addr, size, requesterID, this->Id, {targetId}, InvM, delay, timestamp);
  }

  uint64_t getMisses()      { return this->MissCount;    }
  uint64_t getHits()        { return this->HitCount;     }
  uint64_t getForwards()    { return this->Fwd;          }
  uint64_t getReads()       { return this->NReads;       }
  uint64_t getWrites()      { return this->NWrites;      }
  uint64_t getWriteBacks()  { return this->WriteBacks;   }
  uint64_t getInvals()      { return this->NInvals;      }
  uint64_t getTotalInvals() { return this->NTotalInvals; }
  uint64_t getBackInvals()  { return this->NBackInvals;  }
  uint64_t getEvictions()   { return this->NEvicts;      }
  uint64_t getEvictBacks()  { return this->EvictBacks;   }

  uint64_t getPutS()    { return this->NPutS;    }
  uint64_t getPutM()    { return this->NPutM;    }
  uint64_t getPutI()    { return this->NPutI;    }
  uint64_t getGetS()    { return this->NGetS;    }
  uint64_t getGetM()    { return this->NGetM;    }
  uint64_t getFwdGetS() { return this->NFwdGetS; }
  uint64_t getFwdGetM() { return this->NFwdGetM; }




  bool isPriv;
  void setIsPriv(bool priv) { isPriv=priv; }

private:
  //int countAccess = 0;
  bool DataSupport;
  uint32_t Level;
  bool IsHome;

  tlm::tlm_response_status send_transaction (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterId, const tlm::tlm_command command, sc_time& delay, sc_time timestamp){
    size_t nb_bytes = size;
    tlm::tlm_generic_payload trans;
    trans.set_command (command);
    trans.set_address (addr);
    trans.set_data_length (nb_bytes);
    trans.set_data_ptr (lineDataPtr);
    trans.set_byte_enable_ptr (NULL);
    trans.set_byte_enable_length (0);
    trans.set_gp_option (tlm::TLM_MIN_PAYLOAD); //It is not a real TLM access
    trans.set_response_status (tlm::TLM_INCOMPLETE_RESPONSE);
    assert (command != tlm::TLM_IGNORE_COMMAND);
    CoherencePayloadExtension ext;
    ext.setToHome (!IsHome); // No communication between homes
    ext.setInitiatorId (this->Id); // Use cpu Ids for transactions between cpu private caches and shared LLCs
    ext.setRequesterId (requesterId);
    trans.set_extension<CoherencePayloadExtension>(&ext);
    SourceCpuExtension src;
    src.cpu_id = this->Id;  //TODO change
    src.time_stamp = timestamp;
    trans.set_extension<SourceCpuExtension>(&src);
    socket_out[0] -> b_transport (trans, delay);
    trans.clear_extension(&ext);
    trans.clear_extension(&src);
    return (trans.get_response_status());
  }

  tlm::tlm_response_status send_coherence_transaction (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterId, idx_t initiatorId, set<idx_t> targetIds, const CoherenceCommand command,  sc_time& delay, sc_time timestamp) {
    size_t nb_bytes = size;
    tlm::tlm_generic_payload trans;
    trans.set_command (tlm::TLM_IGNORE_COMMAND);
    trans.set_address (addr);
    trans.set_data_length (nb_bytes);
    trans.set_data_ptr (lineDataPtr);
    trans.set_byte_enable_ptr (NULL);
    trans.set_byte_enable_length (0);
    trans.set_gp_option (tlm::TLM_MIN_PAYLOAD);
    trans.set_response_status (tlm::TLM_INCOMPLETE_RESPONSE);
    CoherencePayloadExtension ext;
    ext.setToHome (!IsHome);
    ext.setCoherenceCommand (command);
    //ext.setInitiatorId (Id); // Use cache ids
    ext.setInitiatorId (initiatorId); // TODO: Use cpu Ids for transactions between cpu private caches and shared LLCs??
    ext.setRequesterId (requesterId);
    if (command==FwdGetS||command==FwdGetM||command==PutI||command==InvS||command==InvM){
      if (IsHome) assert (targetIds.size()>0);
      ext.setTargetIds (targetIds);
    }
    trans.set_extension<CoherencePayloadExtension>(&ext);

    SourceCpuExtension src;
    src.cpu_id = this->Id; //TODO change
    src.time_stamp = timestamp;
    trans.set_extension<SourceCpuExtension>(&src);

    switch (command) {
    case Read: case Write: // downstream, to memory
    case GetS: case GetM: case PutS: case PutM: case Evict: // downstream, to lower cache
      socket_out[0]-> b_transport (trans, delay);
      break;
    case FwdGetS: case FwdGetM: case PutI: case InvS: case InvM: // upstream
      if (IsHome) socket_out[0]-> b_transport (trans, delay); // from home via NOC
      else        socket_out[1]-> b_transport (trans, delay); // from lower cache
      break;
    case Invalidate: case ReadBack: // upstream, from lower cache
      socket_out[1]-> b_transport (trans, delay);
      break;
    }
    trans.clear_extension(&ext);
    trans.clear_extension(&src);
    return (trans.get_response_status());
  }

  tlm::tlm_response_status send_invalidate_transaction (AddressType addr, set<idx_t> targetIds, sc_time& delay, sc_time timestamp) {
    tlm::tlm_generic_payload trans;
    trans.set_command (tlm::TLM_IGNORE_COMMAND);
    trans.set_address (addr);
    trans.set_data_length (0);
    trans.set_data_ptr (NULL);
    trans.set_byte_enable_ptr (NULL);
    trans.set_byte_enable_length (0);
    trans.set_gp_option (tlm::TLM_MIN_PAYLOAD); //It is not a real TLM access
    trans.set_response_status (tlm::TLM_INCOMPLETE_RESPONSE);
    CoherencePayloadExtension ext;
    ext.setInitiatorId (this->Id); // Use cache Ids for transactions between private caches of the same cpu
    //ext.disableDefaultTarget();
    ext.setCoherenceCommand (Invalidate);
    ext.setToHome (!IsHome);
    if (IsHome) assert (targetIds.size()>0);
    ext.setTargetIds (targetIds);
    trans.set_extension<CoherencePayloadExtension>(&ext);
    SourceCpuExtension src;
    src.cpu_id = this->Id; //TODO change
    src.time_stamp = timestamp;
    trans.set_extension<SourceCpuExtension>(&src);
    if (IsHome) socket_out[0] -> b_transport (trans, delay);
    else socket_out[1] -> b_transport (trans, delay);
    trans.clear_extension(&ext);
    trans.clear_extension(&src);
    return (trans.get_response_status());
  }

  tlm::tlm_response_status send_evict_transaction (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterID, sc_time& delay, sc_time timestamp) {
    size_t nb_bytes = size;
    tlm::tlm_generic_payload trans;
    trans.set_command (tlm::TLM_IGNORE_COMMAND);
    trans.set_address (addr);
    trans.set_data_length (nb_bytes);
    trans.set_data_ptr (lineDataPtr);
    trans.set_byte_enable_ptr (NULL);
    trans.set_byte_enable_length (0);
    trans.set_gp_option (tlm::TLM_MIN_PAYLOAD); //It is not a real TLM access
    trans.set_response_status (tlm::TLM_INCOMPLETE_RESPONSE);
    CoherencePayloadExtension ext;
    ext.setInitiatorId (this->Id); // Use cache Ids for transactions between private caches of the same cpu
    ext.setCoherenceCommand (Evict);
    ext.setToHome (!IsHome);
    trans.set_extension<CoherencePayloadExtension>(&ext);
    SourceCpuExtension src;
    src.cpu_id = this->Id; //TODO change
    src.time_stamp = timestamp;
    trans.set_extension<SourceCpuExtension>(&src);
    socket_out[0] -> b_transport (trans, delay);
    trans.clear_extension(&ext);
    trans.clear_extension(&src);
    return (trans.get_response_status());
  }

    tlm::tlm_response_status send_readback_transaction (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t requesterId, set<idx_t> targetIds, sc_time& delay, sc_time timestamp) {
    size_t nb_bytes = size;
    tlm::tlm_generic_payload trans;
    trans.set_command (tlm::TLM_IGNORE_COMMAND);
    trans.set_address (addr);
    trans.set_data_length (nb_bytes);
    trans.set_data_ptr (lineDataPtr);
    trans.set_byte_enable_ptr (NULL);
    trans.set_byte_enable_length (0);
    trans.set_gp_option (tlm::TLM_MIN_PAYLOAD);
    trans.set_response_status (tlm::TLM_INCOMPLETE_RESPONSE);
    CoherencePayloadExtension ext;
    ext.setInitiatorId (this->Id);
    ext.setCoherenceCommand (ReadBack);
    ext.setToHome (!IsHome);
    if (IsHome) assert (targetIds.size()>0);
    ext.setTargetIds (targetIds);
    trans.set_extension<CoherencePayloadExtension>(&ext);
    SourceCpuExtension src;
    src.cpu_id = this->Id; //TODO change 
    src.time_stamp = timestamp;
    trans.set_extension<SourceCpuExtension>(&src);
    if (IsHome) socket_out[0] -> b_transport (trans, delay);
    else socket_out[1] -> b_transport (trans, delay);
    trans.clear_extension(&ext);
    trans.clear_extension(&src);
    return (trans.get_response_status());
  }

protected:
  sc_time Latency;
  int Fwd;

public:

  std::function<bool(AddressType)> is_uncached_region;
  sc_time get_latency() { return Latency; }
  void add_uncached_region(AddressType baddr, uint64_t size) {
    auto f = is_uncached_region;
    is_uncached_region = [baddr,size,f](AddressType a) { return f(a) || (a>=baddr && a<(baddr+size));};
  }
  //Ports
  const uint32_t NUM_PORT_IN;
  const uint32_t NUM_PORT_OUT;
  std::deque<tlm_utils::simple_target_socket<Cache<AddressType, WordType, WCETMode>>>    socket_in;
  std::deque<tlm_utils::simple_initiator_socket<Cache<AddressType, WordType, WCETMode>>> socket_out;

  //---------------------------------------------------
  //TLM 2.0
  void b_transport ( tlm::tlm_generic_payload& trans, sc_time& delay ) override {
    tlm::tlm_response_status rsp = tlm::TLM_OK_RESPONSE;
    AddressType addr        = (AddressType) trans.get_address();
    idx_t srcId;
    idx_t requesterId=NULL_IDX;
    //delay += Latency;
    CoherencePayloadExtension* ext;
    trans.get_extension<CoherencePayloadExtension>(ext); // not mandatory extension

    SourceCpuExtension* cpuExt;
    trans.get_extension<SourceCpuExtension>(cpuExt);

    sc_time timestamp = (cpuExt->time_stamp)+Latency;

    if (Level==1 && trans.get_command() != tlm::TLM_IGNORE_COMMAND) {
      assert (cpuExt);
      srcId = cpuExt->cpu_id;
      trans.clear_extension(cpuExt);
    } else {
      assert (ext); // necessarily coherence transaction
      srcId = ext->getInitiatorId ();
      requesterId = ext->getRequesterId();
    }
    /* if (((Level==1)&&((trans.get_command()==tlm::TLM_READ_COMMAND)||(trans.get_command()==tlm::TLM_WRITE_COMMAND)))
          ||  ((Level!=1)&&(trans.get_command()==tlm::TLM_READ_COMMAND)))
        delay += Latency;
    */
    switch (trans.get_command()) {
    case tlm::TLM_WRITE_COMMAND : // From CPU in coherent mode, From CPU or higher caches in non-coherent mode
      rsp = this-> WriteData (trans.get_data_ptr(), addr, trans.get_data_length(), requesterId, srcId, delay, timestamp, nullptr);
      if (Level==1) delay += Latency;
      break;
    case tlm::TLM_READ_COMMAND : // From CPU in coherent mode, From CPU or higher caches in non-coherent mode
      rsp = this-> ReadData (trans.get_data_ptr(), addr, trans.get_data_length(), requesterId, srcId, delay, timestamp, nullptr);
      delay += Latency;
      break;
    case tlm::TLM_IGNORE_COMMAND : // Coherence transactions from caches in coherent mode
                                   // Only Evict and Invalidate in non-coherent mode
      switch (ext->getCoherenceCommand()) {
      case GetS:
        rsp = this-> AccessGetS (trans.get_data_ptr(), addr, trans.get_data_length(), requesterId, srcId, delay, timestamp);
        delay += Latency;
        break;
      case GetM:
        rsp = this-> AccessGetM (trans.get_data_ptr(), addr, trans.get_data_length(), requesterId, srcId, delay, timestamp);
        delay += Latency;
        break;
      case FwdGetS:
        rsp = this-> AccessFwdGetS (trans.get_data_ptr(), addr, trans.get_data_length(), requesterId, srcId, delay, timestamp);
        delay += Latency;
        break;
      case FwdGetM:
        rsp = this-> AccessFwdGetM (trans.get_data_ptr(), addr, trans.get_data_length(), requesterId, srcId, delay, timestamp);
        delay += Latency;
        break;
      case PutS:
        rsp = this-> AccessPutS (trans.get_data_ptr(), addr, trans.get_data_length(), requesterId, srcId, delay, timestamp);
        break;
      case PutM:
        rsp = this-> AccessPutM (trans.get_data_ptr(), addr, trans.get_data_length(), requesterId, srcId, delay, timestamp);
        break;
      case PutI:
        rsp = this-> AccessPutI (trans.get_data_ptr(), addr, trans.get_data_length(), requesterId, srcId, delay, timestamp);
        break;
      case Evict:
        rsp = this-> EvictLine (trans.get_data_ptr(), addr, trans.get_data_length(), requesterId, srcId, delay, timestamp);
        break;
      case Invalidate:
        rsp = this-> InvalidateLine (addr, delay, timestamp);
        break;
      case InvS:
        rsp = this-> AccessInvS (trans.get_data_ptr(), addr, trans.get_data_length(), requesterId, srcId, delay, timestamp);
        break;
      case InvM:
        rsp = this-> AccessInvM (trans.get_data_ptr(), addr, trans.get_data_length(), requesterId, srcId, delay, timestamp);
        break;
      case ReadBack:
        rsp = this-> AccessReadBack (trans.get_data_ptr(), addr, trans.get_data_length(), requesterId, srcId, delay, timestamp);
        delay += Latency;
        break;
      default: assert(false); break; //throw runtime_error("Not a permitted coherence transaction\n");
      }
      break;
    }
    trans.set_response_status (rsp);

  }
  tlm::tlm_sync_enum nb_transport_fw (tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t) override {
    //return mInitiatorSocket -> nb_transport_fw ( trans, phase, t );
    throw;
  }
  bool get_direct_mem_ptr (tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data) override {
    //return mInitiatorSocket -> get_direct_mem_ptr ( trans, dmi_data );
    if (trans.get_command() == tlm::TLM_IGNORE_COMMAND){
      assert (NUM_PORT_OUT>1);
      return socket_out[1] -> get_direct_mem_ptr (trans, dmi_data);
    } else return socket_out[0] -> get_direct_mem_ptr (trans, dmi_data);
  }
  unsigned int transport_dbg (tlm::tlm_generic_payload& trans) override {
    //return mInitiatorSocket -> transport_dbg ( trans );
    trans.set_response_status (tlm::tlm_response_status::TLM_OK_RESPONSE);
    throw;
  }
  tlm::tlm_sync_enum nb_transport_bw (tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t) override {
    //return mTargetSocket -> nb_transport_bw ( trans, phase, t );
    throw;
  }
  void invalidate_direct_mem_ptr ( sc_dt::uint64 start_range, sc_dt::uint64 end_range ) override {
    //mTargetSocket->invalidate_direct_mem_ptr ( start_range, end_range );
    for(auto& s: socket_in) {
      s->invalidate_direct_mem_ptr(start_range, end_range);
    }
  }

  void configure(){
	  printf("\n");
  }

};

}

#endif //_CACHE_HPP_

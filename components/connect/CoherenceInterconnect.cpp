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

#include "CoherenceInterconnect.hpp"
#include "log.hpp"
#include "MainMemCosim.hpp"
#include "IOAccessCosim.hpp"
namespace vpsim {

  CoherenceInterconnect::CoherenceInterconnect
  (sc_module_name name,
   uint32_t    num_cache_in,
   uint32_t    num_cache_out,
   uint32_t    num_home_in,
   uint32_t    num_home_out,
   uint32_t    num_mmapped,
   uint32_t    num_device,
   uint32_t    flitSize,
   uint32_t    wordLengthInByte,
   bool        isCoherent = false,
   uint32_t    interleaveLength = 0)
    : sc_module       (name)
    , Logger          (string(name))
    , NAME            (string(name))
    , DIAGNOSTIC_LEVEL(DBG_L0)
    , ACCESS_LATENCY  (sc_time(0,SC_NS))
    , ENABLE_LATENCY  (false)
    , InterleaveLength(interleaveLength)
    , MWordLengthInByte (wordLengthInByte)
    , FlitSize        (flitSize)
    , NUM_CACHE_IN    (num_cache_in)
    , NUM_CACHE_OUT   (num_cache_out)
    , NUM_HOME_IN     (num_home_in)
    , NUM_HOME_OUT    (num_home_out)
    , NUM_MMAPPED     (num_mmapped)
    , NUM_DEVICE      (num_device)
    , IsCoherent      (isCoherent)
  {
    for (unsigned i = 0; i<NUM_CACHE_IN; i++) {
      mCacheSocketsIn.push_back(new tlm_utils::simple_target_socket<CoherenceInterconnect>((string("cache_in_")+to_string(i)).c_str()));
      mCacheSocketsIn[i]->register_b_transport(this, &CoherenceInterconnect::b_transport);
    }
    for (unsigned i = 0; i<NUM_CACHE_OUT; i++) {
      mCacheSocketsOut.push_back(new tlm_utils::simple_initiator_socket<CoherenceInterconnect>((string("cache_out_")+to_string(i)).c_str()));
    }
    for (unsigned i = 0; i<NUM_HOME_IN; i++) {
      mHomeSocketsIn.push_back(new tlm_utils::simple_target_socket<CoherenceInterconnect>((string("home_in_")+to_string(i)).c_str()));
      mHomeSocketsIn[i]->register_b_transport(this, &CoherenceInterconnect::b_transport);
    }
    for (unsigned i = 0; i<NUM_HOME_OUT; i++) {
      mHomeSocketsOut.push_back(new tlm_utils::simple_initiator_socket<CoherenceInterconnect>((string("home_out_")+to_string(i)).c_str()));
    }
    for (unsigned i = 0; i<NUM_MMAPPED; i++) {
      mMMappedSocketsOut.push_back(new tlm_utils::simple_initiator_socket<CoherenceInterconnect>((string("mmapped_out_")+to_string(i)).c_str()));
    }
    for (unsigned i = 0; i<NUM_DEVICE; i++) {
      mDeviceSocketsIn.push_back(new tlm_utils::simple_target_socket<CoherenceInterconnect>((string("device_")+to_string(i)).c_str()));
      mDeviceSocketsIn[i]->register_b_transport(this, &CoherenceInterconnect::b_transport_device);
    }
    CacheOutputs.resize   (NUM_CACHE_OUT);
    HomeOutputs.resize    (NUM_HOME_OUT);
    MMappedOutputs.resize (NUM_MMAPPED);

    MMappedReadCountOut.resize (NUM_MMAPPED, 0);
    MMappedWriteCountOut.resize (NUM_MMAPPED, 0);

    RamBaseAddr = UINT64_MAX;
    RamLastAddr = 0x0;
    IndexFirstMemoryController= UINT32_MAX;
  }

  CoherenceInterconnect::~CoherenceInterconnect() {
    for (unsigned i = 0; i<NUM_CACHE_IN;  i++) { delete mCacheSocketsIn [i]; }
    for (unsigned i = 0; i<NUM_CACHE_OUT; i++) { delete mCacheSocketsOut[i]; }
    for (unsigned i = 0; i<NUM_HOME_IN;   i++) { delete mHomeSocketsIn [i];  }
    for (unsigned i = 0; i<NUM_HOME_OUT;  i++) { delete mHomeSocketsOut[i];  }
    for (unsigned i = 0; i<NUM_MMAPPED; i++)   delete mMMappedSocketsOut[i];
    for (unsigned i = 0; i<NUM_DEVICE;  i++)   { delete mDeviceSocketsIn [i]; }
  }

  /**
  * Set functions
  */
  void CoherenceInterconnect::set_latency        (sc_time val) { ACCESS_LATENCY = val; }

  void CoherenceInterconnect::set_enable_latency (bool    val) { ENABLE_LATENCY = val; }

  void CoherenceInterconnect::set_cache_output (uint32_t num_port, idx_t id) {
    assert (CacheCount<NUM_CACHE_OUT);
    assert (CacheCount==num_port);
    assert (id != NULL_IDX);
    id_struct cacheOutput;
    cacheOutput.id   = id;
    cacheOutput.port = num_port;
    CacheOutputs [num_port] = cacheOutput;
    CacheCount++;
  }

  void CoherenceInterconnect::set_mmapped_output (uint32_t num_port, uint64_t base_addr, uint64_t offset) {
    assert (MMappedCount<NUM_MMAPPED);
    addr_struct mmappedOutput;
    mmappedOutput.base_addr = base_addr;
    mmappedOutput.end_addr  = base_addr+offset-1;
    mmappedOutput.offset    = offset;
    mmappedOutput.port      = num_port;
    MMappedOutputs [MMappedCount] = mmappedOutput;
    MMappedCount++;
  }

  void CoherenceInterconnect::set_home_output (uint32_t num_port, idx_t id, uint64_t base_addr, uint64_t offset) {
    assert (HomeCount<NUM_HOME_OUT);
    assert (id != NULL_IDX);
    id_addr_struct homeOutput;
    homeOutput.id        = id;
    homeOutput.base_addr = base_addr;
    homeOutput.end_addr  = base_addr+offset-1;
    homeOutput.offset    = offset;
    homeOutput.port      = num_port;
    HomeOutputs [HomeCount] = homeOutput;
    HomeCount++;
  }

  void CoherenceInterconnect::set_cache_id (idx_t num_port, idx_t id, string name) {
    assert (id != NULL_IDX);
    for (unsigned i = 0; i < CacheOutputs.size(); i++){
      if (CacheOutputs[i].name == name){
        CacheOutputs[i].id = id;
        CacheOutputs[i].port = num_port;
        return;
      }
    }
  }

  void CoherenceInterconnect::set_cache_pos (string name, idx_t pos) {
    id_struct cacheOutput;
    cacheOutput.name = name;
    cacheOutput.position = pos;
    CacheOutputs[pos] = cacheOutput;
  }

  /**
  * Get functions
  */
  sc_time CoherenceInterconnect::get_latency () { return ACCESS_LATENCY; }

  bool CoherenceInterconnect::get_enable_latency () { return ENABLE_LATENCY; }

  inline void CoherenceInterconnect::sendTransactionToHome (tlm::tlm_generic_payload& trans, sc_time& delay){
    if (HomeOutputs.size()==1) {
      (*mHomeSocketsOut[0])->b_transport(trans, delay);
      return;
    } else {
      for (auto it = HomeOutputs.begin(); it != HomeOutputs.end(); ++it)
        if ((trans.get_address() >= it->base_addr) && (trans.get_address()+trans.get_data_length()-1 <= it->end_addr)) {
          (*mHomeSocketsOut[it->port])->b_transport(trans, delay);
          return;
        }
      assert (false); //throw runtime_error ("No home found\n");
    }
  }

  inline void CoherenceInterconnect::sendTransactionToCache (tlm::tlm_generic_payload& trans, set<idx_t> targetIds, sc_time& delay) {
    //if (targetIds.size() == 0 || NUM_CACHE == 0) throw runtime_error("No cache found\n");
    if (targetIds.size()==0)
      // TODO: assert (!IsCoherent);
      for (auto it = CacheOutputs.begin(); it != CacheOutputs.end(); ++it) // broadcast to all upper caches
        (*mCacheSocketsOut[it->position])->b_transport(trans, delay);
    else
      // TODO: assert (IsCoherent);
      for (auto it = CacheOutputs.begin(); it != CacheOutputs.end(); ++it) // broadcast to specific upper caches
        for (auto id : targetIds)
          if (it->id==id) (*mCacheSocketsOut[it->position])->b_transport(trans, delay);
  }

  inline void CoherenceInterconnect::sendTransactionToMMapped (tlm::tlm_generic_payload& trans, sc_time& delay){
    assert(trans.get_command()!=tlm::TLM_IGNORE_COMMAND);
    for (auto it = MMappedOutputs.begin(); it != MMappedOutputs.end(); ++it)
      if (trans.get_address() >= it->base_addr && trans.get_address()+trans.get_data_length()-1 <= it->end_addr){
        (*mMMappedSocketsOut[it->port])-> b_transport (trans, delay);
        //if (trans.get_command()==tlm::TLM_READ_COMMAND) MMappedReadCountOut[it->port] += trans.get_data_length();
        //else MMappedWriteCountOut[it->port] += trans.get_data_length();
        return;
      } assert (false); //throw runtime_error ("No memory mapped component found\n");
  }

  /**
  * Statistics
  */

  /* 
  inline void CoherenceInterconnect::print_noc_statistics () {
    LOG_STATS << "(" << NAME << "):" ;
    LOG_STATS << "total distance = " << TotalDistance;
    LOG_STATS << "total latency = "  << TotalLatency;
    LOG_STATS << "packets number = "  << PacketsCount;
  }

  void CoherenceInterconnect::print_mem_statistics () {
    for (size_t i=0; i<NUM_MMAPPED; i++) {
      LOG_STATS << "(" << NAME << "): port[" << i << "]:";
      LOG_STATS << "total read = "      << MMappedReadCountOut[i];
      LOG_STATS <<", total write = "    << MMappedWriteCountOut[i];
      LOG_STATS <<" (total accesses = " << MMappedReadCountOut[i]+MMappedWriteCountOut[i]<<")" << endl;
    }
  }

  void CoherenceInterconnect::print_coherence_statistics () {
    for (size_t i=0; i<NUM_HOME; i++) {
      LOG_STATS << "(" << NAME << "): id [" << HomeOutputs[i].id << "]:";
      LOG_STATS << "total read = "   << HomeReadCountOut[i];
      LOG_STATS <<", total write = " << HomeWriteCountOut[i];
      LOG_STATS <<", total coherence messages = " << HomeCoherentCountOut[i];
      LOG_STATS <<" (total accesses = " << HomeReadCountOut[i]+HomeWriteCountOut[i]+HomeCoherentCountOut[i]<<")" << endl;
    }
    for (size_t i=0; i<NUM_CACHE; i++) {
      LOG_STATS << "(" << NAME << "): id [" << CacheOutputs[i].id << "]:";
      if (IsCoherent) LOG_STATS <<", total coherence messages = " << HomeCoherentCountOut[i];
      LOG_STATS <<" (total accesses = " << HomeReadCountOut[i]+HomeWriteCountOut[i]+HomeCoherentCountOut[i]<<")" << endl;

    }
  }
  */

  void CoherenceInterconnect::print_statistics () {
    // NoC distance and latency
    //print_noc_statistics ();
    // Memory performance counters
    //print_mem_statistics ();
    // Coherence performance counters
    //print_coherence_statistics ();
    return;
  }

  /**
  * NoC latency functions
  */
  void CoherenceInterconnect::set_router_latency (double nanoseconds){
    mRouterLatency=sc_time(nanoseconds, SC_NS);

  }
  void CoherenceInterconnect::set_link_latency (double nanoseconds){
    mLinkLatency=sc_time(nanoseconds, SC_NS);
  }

  void CoherenceInterconnect::set_noc_stats_per_initiator(bool noc_stats_per_initiator_on){
    mNoc_stats_per_initiator_on=noc_stats_per_initiator_on;
  }

  void CoherenceInterconnect::set_is_mesh (bool is_mesh)    { mIsMesh = is_mesh; }

  void CoherenceInterconnect::set_mesh_coord (idx_t x, idx_t y) {
    if (x<0 || y<0) throw runtime_error("Mesh size cannot be negative.\n");
    mX = x; mY = y;
    RouterTotalLatency.resize(mX * mY,sc_time(0, SC_NS));
    RouterPacketsCount.resize(mX * mY,0);
  }

  uint64_t CoherenceInterconnect::get_ram_base_addr () { return RamBaseAddr;}

  void CoherenceInterconnect::set_ram_base_addr (uint64_t firstAddr) { RamBaseAddr = firstAddr;}

  uint64_t CoherenceInterconnect::get_ram_last_addr () { return RamLastAddr;}

  void CoherenceInterconnect::set_ram_last_addr (uint64_t lastAddr)  { RamLastAddr = lastAddr; }

  void CoherenceInterconnect::set_memory_word_length (uint32_t wordLengthInByte) { MWordLengthInByte = wordLengthInByte;}

  void  CoherenceInterconnect::set_first_memory_controller () { if(IndexFirstMemoryController == UINT32_MAX) IndexFirstMemoryController=mAddressIDs.size();}

  void CoherenceInterconnect::register_mem_ctrl (uint64_t base, uint64_t size, idx_t x_id, idx_t y_id) {
    assert (x_id!= NULL_IDX && y_id!= NULL_IDX);
    if (x_id > mX || y_id > mY) throw runtime_error("Incorrect memory/LLC mesh coordinates.\n");
    else{
      mAddressIDs.push_back (make_tuple(base, size, x_id, y_id));
      mReadCount.push_back(UINT64_C(0));mWriteCount.push_back(UINT64_C(0));
    }
  }

  void CoherenceInterconnect::register_cpu_ctrl (idx_t id, idx_t x_id, idx_t y_id) {
    assert (!IsCoherent || id!= NULL_IDX);
    if (x_id > mX || y_id > mY) throw runtime_error("Incorrect cache mesh coordinates.\n");
    else mCpuIDs.push_back (make_tuple(id, x_id, y_id));
  }

  void CoherenceInterconnect::register_device_ctrl (idx_t id, idx_t x_id, idx_t y_id) {
    assert (!IsCoherent || id!= NULL_IDX);
    if (x_id > mX || y_id > mY) throw runtime_error("Incorrect device mesh coordinates.\n");
    else mDeviceIDs.push_back (make_tuple(id, x_id, y_id));
  }

  void CoherenceInterconnect::register_home_ctrl (uint64_t base, uint64_t size, idx_t x_id, idx_t y_id) {
    assert (x_id!= NULL_IDX && y_id!= NULL_IDX);
    if (x_id > mX || y_id > mY) throw runtime_error("Incorrect memory/LLC mesh coordinates.\n");
    else mHomeIDs.push_back (make_tuple(base, size, x_id, y_id));
  }

  /**
  * @param  index   points the registered memory mapped component
  */
  CoherenceInterconnect::mesh_pos CoherenceInterconnect::get_noc_pos_by_address (uint64_t addr, size_t& index) {
    index = 0;
    for (auto& mapping: mAddressIDs){
      if (addr >= get<0>(mapping) && addr < get<1>(mapping)+get<0>(mapping)) {
        idx_t x_id = get<2>(mapping);
        idx_t y_id = get<3>(mapping);
        return { x_id, y_id };
      }
      ++index;
    }
    throw runtime_error("Unknown Address: " + to_string(addr));
  }

  /**
  * @param index index of the memory controller. It supposes that Memory controllers are
  * registered in a sorted way according to their address spaces.
  * Besides, index must have been initialized with that of the first memory controller
  */
  CoherenceInterconnect::mesh_pos CoherenceInterconnect::get_noc_pos_by_address_with_interleave (uint64_t addr, size_t& index) {
    if(addr >= RamBaseAddr && addr < RamLastAddr){
      index += ((addr-RamBaseAddr) / InterleaveLength) % mAddressIDs.size();
      return {get<2>(mAddressIDs[index]), get<3>(mAddressIDs[index])};
    }
    else
      return get_noc_pos_by_address (addr,index);
  }

  CoherenceInterconnect::mesh_pos CoherenceInterconnect::get_noc_pos_by_id (idx_t id) {
    assert (id!= NULL_IDX);
    for (auto& mapping: mCpuIDs)
      if (id == get<0>(mapping)) {
        idx_t x_id = get<1>(mapping);
        idx_t y_id = get<2>(mapping);
        return { x_id, y_id };
      }
    throw runtime_error("Unknown ID: " + to_string(id));
  }

  CoherenceInterconnect::mesh_pos CoherenceInterconnect::get_device_noc_pos_by_id (idx_t id) {
    assert (id!= NULL_IDX);
    for (auto& mapping: mDeviceIDs)
      if (id == get<0>(mapping)) {
        idx_t x_id = get<1>(mapping);
        idx_t y_id = get<2>(mapping);
        return { x_id, y_id };
      }
    throw runtime_error("Unknown Device ID: " + to_string(id));
  }

  CoherenceInterconnect::mesh_pos CoherenceInterconnect::get_home_pos_by_address (uint64_t addr) {
    for (auto& mapping: mHomeIDs)
      if (addr >= get<0>(mapping) && addr < get<1>(mapping)+get<0>(mapping)) {
        idx_t x_id = get<2>(mapping);
        idx_t y_id = get<3>(mapping);
        return { x_id, y_id };
      }
    throw runtime_error("Unknown Address: " + to_string(addr));
  }

  inline void CoherenceInterconnect::computeNoCPerformance (uint64_t distance, sc_time latency) {
    TotalDistance += distance;
    TotalLatency  += latency;
    PacketsCount++;
  }

  uint64_t CoherenceInterconnect::computeNoCLatency (bool isHome, bool isIdMapped, uint64_t addr, idx_t src_id, set<idx_t> dst_ids) {
    mesh_pos src_pos = get_noc_pos_by_id(src_id);
    mesh_pos dst_pos;
    idx_t src_x = src_pos.x_id;
    idx_t src_y = src_pos.y_id;
    idx_t dst_x, dst_y;
    uint64_t dist = 0;

    if (!isIdMapped) {
      // If target is memory-mapped, i.e. main memory or LLC, its mesh position is computed using its address map
      if(!isHome){
        size_t index = 0;
        dst_pos = get_noc_pos_by_address(addr, index);
        dst_x = dst_pos.x_id;
        dst_y = dst_pos.y_id;
        dist  = abs((int64_t)src_x-dst_x) + abs((int64_t)src_y-dst_y)+1;
      } else {
        dst_pos = get_home_pos_by_address(addr);
        dst_x = dst_pos.x_id;
        dst_y = dst_pos.y_id;
        dist  = abs((int64_t)src_x-dst_x) + abs((int64_t)src_y-dst_y)+1;
      }
    } else {
      // If target is not memory-mapped, i.e. higher-level cache, its mesh position is computed using its id
      // If broadcast to higher-level caches, only the largest distance is used for delay computation
      if (IsCoherent) {
        for (set<idx_t>::iterator it = dst_ids.begin(); it!=dst_ids.end(); it++) {
          dst_pos = get_noc_pos_by_id(*it);
          dst_x = dst_pos.x_id;
          dst_y = dst_pos.y_id;
          uint64_t d = abs((int64_t)src_x-dst_x) + abs((int64_t)src_y-dst_y)+1;
          dist = max (dist, d);
        }
      } else {
        for (auto it = CacheOutputs.begin(); it != CacheOutputs.end(); ++it) {
          dst_pos = get_noc_pos_by_id(it->id);
          dst_x = dst_pos.x_id;
          dst_y = dst_pos.y_id;
          uint64_t d = abs ((int64_t)src_x-dst_x) + abs ((int64_t)src_y-dst_y)+1;
          dist = max (dist, d);
        }
      }
    }
    // If target and destination have the same Mesh coordinates, i.e. they are connected to the same router => nbr_hops=1
    // if (dist==0) dist = 1;
    return dist;
  }

  /**
  * NoC perf stats for each initiator
  */
  void CoherenceInterconnect::FillInitTotalStats(idx_t id,idx_t src_x, idx_t src_y, uint64_t dist, sc_time lat)
  {
    string position=to_string(src_x) + '_' + to_string(src_y);
    get<0>(initTotalStats[id])=position;
    get<1>(initTotalStats[id])+=1;
    get<2>(initTotalStats[id])+=dist;
    get<3>(initTotalStats[id])+=lat;
  }

  /**
  * NoC Contention implementation
  */
  void CoherenceInterconnect::set_contention (bool with_contention){mWithContention = with_contention;}

  void CoherenceInterconnect::set_contention_interval (double contention_interval){mContentionInterval = sc_time(contention_interval, SC_NS);}

  void CoherenceInterconnect::set_virtual_channels (uint32_t virtual_channels){
    if (virtual_channels<=0) throw runtime_error("The number of virtual channels should be >=1 .\n");
    mVirtualChannels=virtual_channels;
  }

  void CoherenceInterconnect::set_buffer_size (uint32_t buffer_size){
    if (buffer_size<=0) throw runtime_error("Buffer depth should be >=1 flit(s) .\n");
    mBufferSize=buffer_size;
  }

  void CoherenceInterconnect::SavePacket(idx_t id, route path, uint32_t nbFlits){
    sc_time t0=sc_time(0, SC_NS);
    for(uint32_t count=0;count<nbFlits;++count) packetBuffer.push_back(make_tuple(id+count,path,t0));
  }

  void CoherenceInterconnect::Create_Noc(idx_t noc_x, idx_t noc_y){
    for(idx_t y=0; y<noc_y;y++){
      for (idx_t x=0; x<noc_x; x++){
        outputBuffer N,S,E,W,L;
        router r{{'E',E},{'L',L},{'N',N},{'S',S},{'W',W}};
        noc.insert(make_pair(make_tuple(x,y),r));
      }
    }
  }

  CoherenceInterconnect::route CoherenceInterconnect::ComputeRouteAndUpdateRouters(idx_t src_x,idx_t src_y, idx_t dst_x, idx_t dst_y,idx_t id, uint32_t nbFlits)
  {
    idx_t i=0;
    idx_t j=0;
    sc_time t0=sc_time(0, SC_NS);
    route path;
    if((dst_x==src_x)&&(dst_y==src_y)){ // the destination router is the source router
        path.push_back(make_tuple(dst_x,dst_y,'L')); // the output port of the traversed router and its position are added to the packet's path
        for(uint32_t count=0;count<nbFlits;++count) ((noc[make_tuple(dst_x,dst_y)])['L']).insert(make_pair(id+count,t0)); // update the router's outputBuffer with the arriving packet's id
        RouterPacketsCount[dst_x+(dst_y*mX)]+=nbFlits;
    }
    else{
        if(dst_x<src_x){
        for(i=src_x;i>dst_x;i--){
            path.push_back(make_tuple(i,src_y,'W'));
            for(uint32_t count=0;count<nbFlits;++count) ((noc[make_tuple(i,src_y)])['W']).insert(make_pair(id+count,t0));
            RouterPacketsCount[i+(src_y*mX)]+=nbFlits;
            }
        }
        else if(dst_x>src_x){
            for(i=src_x;i<dst_x;i++){
                path.push_back(make_tuple(i,src_y,'E'));
                for(uint32_t count=0;count<nbFlits;++count) ((noc[make_tuple(i,src_y)])['E']).insert(make_pair(id+count,t0));
                RouterPacketsCount[i+(src_y*mX)]+=nbFlits;
            }
        }
        if(dst_y<src_y){
            if(dst_x==src_x){
                i=src_x;
            }
            for(j=src_y;j>dst_y;j--){
                path.push_back(make_tuple(i,j,'N'));
                for(uint32_t count=0;count<nbFlits;++count) ((noc[make_tuple(i,j)])['N']).insert(make_pair(id+count,t0));
                RouterPacketsCount[i+(j*mX)]+=nbFlits;
            }
        }
        if(dst_y>src_y){
            if(dst_x==src_x){
            i=src_x;
            }
            for(j=src_y;j<dst_y;j++){
                path.push_back(make_tuple(i,j,'S'));
                for(uint32_t count=0;count<nbFlits;++count) ((noc[make_tuple(i,j)])['S']).insert(make_pair(id+count,t0));
                RouterPacketsCount[i+(j*mX)]+=nbFlits;
            }
        }
        //Don't forget to update the destination router's local outputBuffer
        path.push_back(make_tuple(dst_x,dst_y,'L'));
        for(uint32_t count=0;count<nbFlits;++count) ((noc[make_tuple(dst_x,dst_y)])['L']).insert(make_pair(id+count,t0));
        RouterPacketsCount[dst_x+(dst_y*mX)]+=nbFlits;
        }
    return path;
  }

  sc_time CoherenceInterconnect::QueueWaitingTime(sc_time wait, sc_time router_latency, sc_time link_latency, sc_time time_interval, uint64_t queue_nbr_packets){
    int64_t ns_per_sec=1000000000;
    double wt=(wait.to_seconds())*ns_per_sec+(router_latency.to_seconds())*ns_per_sec+(link_latency.to_seconds())*ns_per_sec-(((time_interval.to_seconds())*ns_per_sec)/(queue_nbr_packets/mVirtualChannels));
    sc_time t0=sc_time(0, SC_NS);
        if(wt>=0){
            sc_time waiting_time =sc_time(wt,SC_NS);
            return waiting_time;
        }
        else{
            return t0;
        }

  }

  /**
   * @brief The latency of a packet
   * 
   * @param total_wait 
   * @param router_latency 
   * @param link_latency (the number of links include the network links and 2 external links that connect src/dst routers to the PEs)
   * @param nbr_hops is the number of routers on the packet's path.
   * @return sc_time 
   */
  sc_time CoherenceInterconnect::PacketLatency(sc_time total_wait, sc_time router_latency, sc_time link_latency, uint64_t nbr_hops){
    return(nbr_hops*router_latency+(nbr_hops+1)*link_latency+total_wait);
  }

  sc_time CoherenceInterconnect::ComputePacketLatency(){
    sc_time total_wait; //Waiting time of a packet at the buffers of all traversed routers
    sc_time buffer_wait; //Waiting time of a packet at a given buffer
    sc_time avg_latency=sc_time(0, SC_NS); //average packet latency in a contention interval
    for ( auto& pkt : packetBuffer){
      packetId_t prev_pkt= NULL_PACKET; //id of previous packet
      sc_time prev_wait=sc_time(0, SC_NS);  //contention delay (i.e. buffer waiting time) of the previous packet
      total_wait=sc_time(0, SC_NS);
      for (auto rt=(get<1>(pkt)).begin();rt!=(get<1>(pkt)).end(); rt++){
        //rt is an iterator on the router list forming the "route" of the considered packet
        //Remember that route is a vector <tuple<idx_t, idx_t, char>>
        //So, get<0>(*rt) is the router id in X axis and get<1>(*rt) is the router id in Y axis (both of idx_t type)
        buffer_wait=sc_time(0, SC_NS);
        if((get<0>(pkt))==(((noc[make_tuple(get<0>(*rt),get<1>(*rt))])[get<2>(*rt)]).begin())->first){ //if packet is the first in the outputBuffer then its waiting time in the buffer is 0
          //First packet in buffer
          buffer_wait=sc_time(0, SC_NS);
          total_wait+=buffer_wait;
          ((noc[make_tuple(get<0>(*rt),get<1>(*rt))])[get<2>(*rt)])[get<0>(pkt)]=buffer_wait;
          RouterTotalLatency[get<0>(*rt)+(get<1>(*rt)*mX)]+=buffer_wait;
        }
        else{ //There are one or more packets in the outputBuffer before the current packet
          outputBuffer::iterator it=((noc[make_tuple(get<0>(*rt),get<1>(*rt))])[get<2>(*rt)]).find(get<0>(pkt)); //find position of the current packet in the outputBuffer
          //noc[make_tuple(get<0>(*rt),get<1>(*rt))] retrieves the router in the NoC
          //then router [get<2>(*rt)] retrieves the considered output buffer a map <uint64_t, sc_time> standing for <packet_id,waiting_time>
          //Finally, we get an iterator pointing to the considered packet in its output buffer
          //WARNING : value returned by find not tested against end(), in addition what is the behavior of prev(it) if it is begin(), that is likely to happen ?
          it=prev(it); //position of the packet preceding the current packet in outputBuffer
          if((it->first)==prev_pkt) //if contention with the previous pkt was accounted for in a previous router then there is no waiting time;i.e. these packets were already serialized
          {
            //Packet serialization
            if((it->second)==prev_wait) //if the previous packet's waiting time has not changed
            {
              buffer_wait=sc_time(0, SC_NS);
              total_wait+=buffer_wait;
              ((noc[make_tuple(get<0>(*rt),get<1>(*rt))])[get<2>(*rt)])[get<0>(pkt)]=buffer_wait;
              RouterTotalLatency[get<0>(*rt)+(get<1>(*rt)*mX)]+=buffer_wait;
            }
            else
            {
              buffer_wait=it->second;
              total_wait+=buffer_wait;
              ((noc[make_tuple(get<0>(*rt),get<1>(*rt))])[get<2>(*rt)])[get<0>(pkt)]=buffer_wait;
              RouterTotalLatency[get<0>(*rt)+(get<1>(*rt)*mX)]+=buffer_wait;
            }
          }
          else  //first convergence point between current packet and the previous one; contention should be accounted for here
          {
            buffer_wait=QueueWaitingTime(it->second, mRouterLatency, mLinkLatency, mContentionInterval, ((noc[make_tuple(get<0>(*rt),get<1>(*rt))])[get<2>(*rt)]).size());
            total_wait+=buffer_wait;
            ((noc[make_tuple(get<0>(*rt),get<1>(*rt))])[get<2>(*rt)])[get<0>(pkt)]=buffer_wait;
            RouterTotalLatency[get<0>(*rt)+(get<1>(*rt)*mX)]+=buffer_wait;
            prev_pkt=it->first;
            prev_wait=it->second;
          }
          //Dealing with Head Of Line (HOL) blocking
          if((get<2>(*rt))!='L')  //If the current router is not the last router in the pckt's path
          {
            route::iterator  rt1=next(rt);
            route::iterator rt2=find((get<1>(packetBuffer[prev_pkt-1])).begin(), (get<1>(packetBuffer[prev_pkt-1])).end(),make_tuple(get<0>(*rt),get<1>(*rt),get<2>(*rt))); // find position of previous pckt that is in the same current router, in packetBuffer
            route::iterator rt3=next(rt2);
            if((get<0>(*rt1)!=get<0>(*rt3))||(get<1>(*rt1)!=get<1>(*rt3))||(get<2>(*rt1)!=get<2>(*rt3)))// check if next router of the previous packet is not the same as next router of current packet. If so, we may have HOL blocking
            {
              unsigned index;
              outputBuffer::iterator ob=((noc[make_tuple(get<0>(*rt3),get<1>(*rt3))])[get<2>(*rt3)]).begin();
              for(index=0;index<((noc[make_tuple(get<0>(*rt3),get<1>(*rt3))])[get<2>(*rt3)]).size();index++)
              {
                if((get<0>(*ob))==(get<0>(*it)))
                  break;
                ob=next(ob);
              }
              if(index>=(mBufferSize*mVirtualChannels)) //If the previous packet's position in its next router is greater than BUFFER_SIZE, then there is HOLB
              {
                outputBuffer::iterator it1=it;
                if(mBufferSize>1){
                  it1=prev(it,(mBufferSize*mVirtualChannels)-1);
                }
                buffer_wait=((noc[make_tuple(get<0>(*rt3),get<1>(*rt3))])[get<2>(*rt3)])[get<0>(*it1)];
                total_wait+=buffer_wait;
                ((noc[make_tuple(get<0>(*rt),get<1>(*rt))])[get<2>(*rt)])[get<0>(pkt)]+=buffer_wait;
                RouterTotalLatency[get<0>(*rt)+(get<1>(*rt)*mX)]+=buffer_wait;
              }
            }
          }
        }
      }
      packet_latency=PacketLatency(total_wait, mRouterLatency, mLinkLatency, (get<1>(pkt)).size());
      get<2>(pkt)=packet_latency;
      avg_latency+=packet_latency;
    }
    return avg_latency;
  }

  void CoherenceInterconnect::PrintPath(route path){
    cout << "***path***:" << endl;
    for ( const auto& i : path ) {
      cout << get<0>(i) << "_" << get<1>(i) << "_" << get<2>(i) << endl;
    }
    cout << "*****" << endl;
  }

  void CoherenceInterconnect::PrintPacketBuffer(){
    cout.flush();
    cout << "***PacketBuffer***:" << endl;
    for (const auto& i: packetBuffer){
      cout << "packet_id: "<< get<0>(i)<< endl;
      for(const auto& j: get<1>(i)){
        cout << "path:  " << get<0>(j) << "_" <<get<1>(j) <<"_" << get<2>(j) << endl;
      }
      cout << "packet_latency: "<< ((get<2>(i)).to_seconds())*1000000000<< " ns" <<endl;
      cout << "-----" << endl;
    }
    cout << "*****" << endl;
  }

  void CoherenceInterconnect::PrintNoc(){
    cout << "***NoC***:" << endl;
    for (auto& j : noc ){
      cout << "router_"<<get<0>(get<0>(j))<<"_"<< get<1>(get<0>(j))<<":"<< endl;
      cout << "port N, number of packets= " << ((get<1>(j))['N']).size() <<endl;
      for (const auto& k : (get<1>(j))['N'] ){
        cout << "port N: pkt_id: "<<get<0>(k) << " ,wait= "<< get<1>(k)  << endl;
      }
      cout << "port S, number of packets= " << ((get<1>(j))['S']).size() <<endl;
      for ( const auto& k : (get<1>(j))['S'] ){
        cout << "port S: pkt_id: "<< get<0>(k) << " ,wait= "<<  get<1>(k) << endl;
      }
      cout << "port E, number of packets= " << ((get<1>(j))['E']).size() <<endl;
      for ( const auto& k : (get<1>(j))['E'] ){
        cout << "port E: pkt_id: "<<  get<0>(k) << " ,wait= "<< get<1>(k) << endl;
      }
      cout << "port W, number of packets= " << ((get<1>(j))['W']).size() <<endl;
      for ( const auto& k : (get<1>(j))['W'] ){
        cout << "port W: pkt_id: "<< get<0>(k) << " ,wait= "<< get<1>(k) << endl;
      }
      cout << "port L, number of packets= " << ((get<1>(j))['L']).size() <<endl;
      for ( const auto& k : (get<1>(j))['L'] ){
        cout << "port L: pkt_id: "<< get<0>(k) << " ,wait= "<< get<1>(k) << endl;
      }
      cout << "-----" << endl;
    }
    cout << "*****" << endl;
  }

  vector <tuple<idx_t,idx_t>> CoherenceInterconnect::GetDestinations(tlm::tlm_generic_payload& trans, bool isHome, bool isIdMapped, set<idx_t> dst_ids) {
    vector <tuple<idx_t,idx_t>> dest;
    mesh_pos dst_pos;
    if (!isIdMapped) {
      if(!isHome){
        size_t index = IndexFirstMemoryController;
        if(!InterleaveLength)   dst_pos = get_noc_pos_by_address(trans.get_address(), index);
        else                    dst_pos = get_noc_pos_by_address_with_interleave(trans.get_address(), index);
        dest.push_back(make_tuple(dst_pos.x_id,dst_pos.y_id));
        //Update Memory Controllers Counters
        if(trans.get_command()==tlm::TLM_READ_COMMAND)
          mReadCount[index]+=trans.get_data_length();
        else if (trans.get_command()==tlm::TLM_WRITE_COMMAND)
          mWriteCount[index]+=trans.get_data_length();
        return dest;
      } else {
        dst_pos = get_home_pos_by_address(trans.get_address());
        dest.push_back(make_tuple(dst_pos.x_id,dst_pos.y_id));
        return dest;
      }
    } else {
      if (IsCoherent) {
        for (set<idx_t>::iterator it = dst_ids.begin(); it!=dst_ids.end(); it++) {
          dst_pos = get_noc_pos_by_id(*it);
          dest.push_back(make_tuple(dst_pos.x_id,dst_pos.y_id));
        }
        return dest;
      } else {
        for (auto it = CacheOutputs.begin(); it != CacheOutputs.end(); ++it) {
          dst_pos = get_noc_pos_by_id(it->id);
          dest.push_back(make_tuple(dst_pos.x_id,dst_pos.y_id));
        }
        return dest;
      }
    }
  }

  void CoherenceInterconnect::NetworkTimingModel(tlm::tlm_generic_payload& trans, sc_time trans_time_stamp, sc_time time_interval, bool isHome, bool isIdMapped, uint32_t nbFlits, idx_t src_x, idx_t src_y, set<idx_t> dst_ids, bool device){
    route path;
    vector <tuple<idx_t,idx_t>> dest;
    if(device && trans.get_command()==tlm::TLM_READ_COMMAND){ //Reverse direction: memory -> device
      dest.push_back(make_tuple(src_x,src_y));
      size_t index = IndexFirstMemoryController; //index of the first memory controller
      mesh_pos src_pos;
      if(!InterleaveLength)   src_pos = get_noc_pos_by_address(trans.get_address(), index);
      else                    src_pos = get_noc_pos_by_address_with_interleave(trans.get_address(), index);
      src_x = src_pos.x_id;
      src_y = src_pos.y_id;
    }
    else{
      dest=GetDestinations(trans, isHome, isIdMapped, dst_ids);
    }
    int64_t ns_per_sec=1000000000;
    sc_time ts=sc_time((trans_time_stamp.to_seconds())*ns_per_sec, SC_NS);
    if(totalFlits==0){  //This packet is the first to arrive to the Network during the current time interval
      interval_start=ts;
      interval_end=interval_start + time_interval;
      for (const auto& dst : dest){
        path=ComputeRouteAndUpdateRouters(src_x,src_y,get<0>(dst), get<1>(dst),totalFlits+1,nbFlits);
        TotalDistance+=nbFlits*path.size();
        SavePacket(totalFlits+1,path,nbFlits);
        totalFlits+=nbFlits;
      }
    }
    else if(totalFlits>0){ //PacketBuffer is not empty
      if((ts>=interval_start)&&(ts<=interval_end)){ //packet arrived during time interval
        for (const auto& dst : dest){
          path=ComputeRouteAndUpdateRouters(src_x,src_y, get<0>(dst), get<1>(dst), totalFlits+1, nbFlits);
          TotalDistance+=nbFlits*path.size();
          SavePacket(totalFlits+1,path,nbFlits);
          totalFlits+=nbFlits;
        }
      }
      else if((ts>interval_end)||(ts <interval_start)){ //A new interval of duration time_interval starts (the second part of the condition is a makeshift solution to deal with the timestamps that go back in time)
        AverageLatency+=ComputePacketLatency(); //Sum of average latencies over contention intervals
        //Clear all data structures filled with packets from the previous time interval
        PacketsCount+=packetBuffer.size();
        packetBuffer.clear();
        noc.clear();
        totalFlits=0;
        interval_start=ts; //Start a new time interval
        interval_end=interval_start + time_interval;
        for (const auto& dst : dest){
          path=ComputeRouteAndUpdateRouters(src_x,src_y,get<0>(dst), get<1>(dst), totalFlits+1, nbFlits);
          TotalDistance+=nbFlits*path.size();
          SavePacket(totalFlits+1,path,nbFlits);
          totalFlits+=nbFlits;
        }
      }
    }
  }

  /**
   * TLM 2.0 communication interface
  */
  void CoherenceInterconnect::b_transport (tlm::tlm_generic_payload& trans, sc_time& delay) {
    CoherencePayloadExtension* ext;
    trans.get_extension<CoherencePayloadExtension>(ext);
    assert (ext); //Transactions initiated by caches have ncecessarily extensions
    uint32_t nbFlits=1; // We need to be more specific and retrieve the right number from datasheet
    bool isIdMapped; //False if initiator is memory-mapped or home cache
    SourceCpuExtension* src;
    trans.get_extension<SourceCpuExtension>(src);
    assert(src);

    //Check if request is downstream
    bool isDownstream = (trans.get_command()==tlm::TLM_READ_COMMAND || ((trans.get_command()==tlm::TLM_IGNORE_COMMAND) &&
                                                                        (ext->getCoherenceCommand()==GetS||
                                                                         ext->getCoherenceCommand()==GetM||
                                                                         ext->getCoherenceCommand()==FwdGetS||
                                                                         ext->getCoherenceCommand()==FwdGetM)));
    //Check if component is id-mapped
    if (IsCoherent)
      isIdMapped = (trans.get_command()==tlm::TLM_IGNORE_COMMAND) && (ext->getCoherenceCommand()==FwdGetS||
                                                                      ext->getCoherenceCommand()==FwdGetM||
                                                                      ext->getCoherenceCommand()==PutI||
                                                                      ext->getCoherenceCommand()==InvS||
                                                                      ext->getCoherenceCommand()==InvM);
    else
      isIdMapped = (trans.get_command()==tlm::TLM_IGNORE_COMMAND) && (ext->getCoherenceCommand()==Invalidate);
    assert (ext || !isIdMapped);

    // Compute latency delay
    if (!mIsMesh) {
      if (isDownstream) delay += ACCESS_LATENCY;
    } else  { // Mesh NoC model for NoC latency computation
      if (trans.get_command()==tlm::TLM_WRITE_COMMAND || ext->getCoherenceCommand()==PutS || ext->getCoherenceCommand()==PutM) //Transactions transporting data
        nbFlits=trans.get_data_length()/FlitSize; //Again, we need to be more specific and retrieve the right number from datasheet
      if(mWithContention==0){ // NoC performance model without contention
        uint64_t dist = computeNoCLatency (ext->getToHome(), isIdMapped, trans.get_address(), ext->getInitiatorId(), ext->getTargetIds());
        sc_time latency = mRouterLatency*dist;
        if (isDownstream) delay += latency;
        computeNoCPerformance (dist, latency);
        if(mNoc_stats_per_initiator_on){
          mesh_pos src_pos = get_noc_pos_by_id(ext->getInitiatorId());
          idx_t src_x = src_pos.x_id;
          idx_t src_y = src_pos.y_id;
          FillInitTotalStats(ext->getInitiatorId(),src_x, src_y, dist, latency);
        }
      }
      else{ // NoC performance model with contention
        sc_time ts = src->time_stamp + delay;
        mesh_pos src_pos = get_noc_pos_by_id(ext->getInitiatorId());
        NetworkTimingModel(trans,ts,mContentionInterval,ext->getToHome(),isIdMapped,nbFlits,src_pos.x_id,src_pos.y_id,ext->getTargetIds());
	      if (isDownstream) delay+=packet_latency;
      }
    }

    // send transaction
    if (IsCoherent) {
      if (trans.get_command() != tlm::TLM_IGNORE_COMMAND) sendTransactionToMMapped(trans, delay);
      else {
        assert (ext->getCoherenceCommand()!=Invalidate||ext->getCoherenceCommand()!=Evict);
        switch (ext->getCoherenceCommand()) {
          case GetS: case GetM: case PutS: case PutM: case Evict: sendTransactionToHome(trans, delay); break;
          case FwdGetS: case FwdGetM: case PutI: case InvS: case InvM: case ReadBack:
            assert(ext->getTargetIds().size()>0);
            sendTransactionToCache(trans, ext->getTargetIds(), delay);
            break;
          default: assert (false); break;
        }
      }
    } else { // Non-coherent mode
      if (trans.get_command() != tlm::TLM_IGNORE_COMMAND) { // RD/RW
        if (ext->getToHome()) sendTransactionToHome(trans, delay); // From cache to home
        else sendTransactionToMMapped(trans, delay); // From home to memory
      } else { // Invalidate/Evict/ReadBack
        assert (ext->getCoherenceCommand()==Invalidate||ext->getCoherenceCommand()==Evict||ext->getCoherenceCommand()==ReadBack);
        switch (ext->getCoherenceCommand()) {
          case Invalidate:
            assert(ext->getTargetIds().size()>0);
            sendTransactionToCache(trans, ext->getTargetIds(), delay);
            break;
          case Evict:
            assert (ext->getToHome());
            sendTransactionToHome(trans, delay); // From cache to home
            break;
          case ReadBack:
            assert(ext->getTargetIds().size()>0);
            sendTransactionToCache(trans, ext->getTargetIds(), delay);
            break;
          default: assert (false); break;
        }
      }
    }
  }

  void CoherenceInterconnect::b_transport_device (tlm::tlm_generic_payload& trans, sc_time& delay) {

    SourceDeviceExtension* src;
    trans.get_extension<SourceDeviceExtension>(src);
    assert(src);

    //Backup values
    unsigned int storeSize = trans.get_data_length();
    uint64_t storeAddr = trans.get_address();

    //Some parameters to make delay computation more accurate
    uint64_t addr = storeAddr;

    sc_time arrivalDelay; //Delay at the NoC door!
    sc_time memDelay;     //Delay at the memory door!
    sc_time maxDelay;     //Delay of the whole transaction once the access to the memory+the NoC traversal are achieved!
    sc_time tmpDelay;     //tmp as in tmp!
    mesh_pos src_pos = get_device_noc_pos_by_id(src->device_id);
    idx_t src_x = src_pos.x_id;
    idx_t src_y = src_pos.y_id;

    if(trans.get_command()==tlm::TLM_READ_COMMAND){
      if(!mIsMesh)
      {
        sendTransactionToMMapped(trans, delay);
        delay+= ACCESS_LATENCY;
      }
      else{// NoC performance model with contention
        size_t nbrWords= storeSize/MWordLengthInByte;
        if(storeSize%MWordLengthInByte != 0) ++nbrWords;
        size_t nbrFlitsPerWord= MWordLengthInByte/FlitSize;
        if(MWordLengthInByte%FlitSize != 0) ++nbrFlitsPerWord;

        memDelay=delay;//Actually, we need in addition the delay of the request packet to get to the memory
        for (size_t j = 0; j < nbrWords; ++j)
        {
          // Send transaction
          trans.set_data_length(MWordLengthInByte); //Memory access (Latency computation) is optimized per Word
          sendTransactionToMMapped(trans, memDelay);
          trans.set_data_length(FlitSize);  //Now, it will be split for the NoC, one flit at a time
          arrivalDelay = memDelay;
          for (size_t i = 0; i < nbrFlitsPerWord; ++i)  //From the CoherenceInterconnect perspective, all transactions have 1 flit size, so we will send transactions one flit at a time.
          {
            //Update Arrival delay
            //arrivalDelay += sc_time((double)(1*(i/mVirtualChannels)),SC_NS); ;//It should only depend on router latency, the number of virtual channels and buffersize.
            //arrivalDelay += mRouterLatency*(i/mVirtualChannels);

            //Compute latency delay
            sc_time ts = src->time_stamp + arrivalDelay;

            NetworkTimingModel(trans,ts,mContentionInterval,false,false,1,src_x,src_y,set<idx_t>(),true);//Reverse direction
            tmpDelay = memDelay + packet_latency; //Packet or flit latency
            if(tmpDelay>maxDelay) maxDelay=tmpDelay;  //Find a better way than to compare every time
          }
          addr+=MWordLengthInByte;  //Update address for next word
          trans.set_address(addr);
        }
        delay = maxDelay;
      }
    }
    else{ //tlm::TLM_WRITE_COMMAND
      if(!mIsMesh)
      {
        delay += ACCESS_LATENCY;
        sendTransactionToMMapped(trans, delay);
      }
      else{ //NoC performance model with contention
        size_t nbrFlits= storeSize/FlitSize;
        if(storeSize%FlitSize != 0) ++nbrFlits;

        trans.set_data_length(FlitSize);//From the CoherenceInterconnect perspective, all transactions have 1 flit size, so we will send transactions one flit at a time.

        arrivalDelay=delay;
        for (size_t i = 0; i < nbrFlits; i++)
        {
          //Update Arrival delay
          //arrivalDelay += sc_time((double)(1*(i/mVirtualChannels)),SC_NS);//It should only depend on router latency, the number of virtual channels and buffersize.
          //arrivalDelay += mRouterLatency*(i/mVirtualChannels);

          //Compute latency delay
          sc_time ts = src->time_stamp + arrivalDelay;
          NetworkTimingModel(trans,ts,mContentionInterval,false,false,1,src_x,src_y,set<idx_t>());//ext->getToHome() -> false
          memDelay = arrivalDelay + packet_latency;//packet or flit latency

          //Send transaction
          sendTransactionToMMapped(trans, memDelay);
          if(memDelay>maxDelay) maxDelay=memDelay;//find a better way than to compare every time

          //Update address for next flit
          addr+=FlitSize;
          trans.set_address(addr);
        }
        delay=maxDelay;
      }
    }
    //Restore values
    trans.set_data_length(storeSize);
    trans.set_address(storeAddr);
  }

  bool CoherenceInterconnect::get_direct_mem_ptr (tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data) {
    // TODO
    return true;
  }

  void CoherenceInterconnect::invalidate_direct_mem_ptr (sc_dt::uint64 start_range, sc_dt::uint64 end_range) {
    /* for (auto& s: SocketsIn) s->invalidate_direct_mem_ptr(start_range, end_range); */
  }

  tlm::tlm_sync_enum
  CoherenceInterconnect::nb_transport_fw (tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t) {
    throw;
  }

  tlm::tlm_sync_enum
  CoherenceInterconnect::nb_transport_bw (tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t) {
    throw;
  }

  unsigned int CoherenceInterconnect::transport_dbg (tlm::tlm_generic_payload& trans) {
    trans.set_response_status(tlm::tlm_response_status::TLM_OK_RESPONSE);
    return 0;
  }
}

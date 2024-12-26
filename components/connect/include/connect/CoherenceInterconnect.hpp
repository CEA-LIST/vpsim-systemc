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

#ifndef COHERENCEINTERCONNECT_HPP_
#define COHERENCEINTERCONNECT_HPP_

#include "global.hpp"
#include <list>
#include "log.hpp"
#include "TargetIf.hpp"
#include "InitiatorIf.hpp"
#include "DmiKeeper.hpp"
#include "CoherenceExtension.hpp"

using namespace std;

namespace vpsim {

  struct id_struct      { string name; idx_t id; uint32_t port; idx_t position; };
  struct addr_struct    {         uint64_t base_addr; uint64_t end_addr; uint64_t offset; uint32_t port; };
  struct id_addr_struct { idx_t id; uint64_t base_addr; uint64_t end_addr; uint64_t offset; uint32_t port; };

  class CoherenceInterconnect: public sc_module,
                               public Logger,
                               public tlm::tlm_bw_transport_if<tlm::tlm_base_protocol_types>,
                               public tlm::tlm_fw_transport_if<tlm::tlm_base_protocol_types>
  {

  //this shall not be namespace visible but kept within the base Interconnect class
  typedef uint64_t packetId_t ;
  packetId_t NULL_PACKET=(packetId_t) 0xffffffffffffffff;
  typedef struct mesh_pos { idx_t x_id; idx_t y_id; } mesh_pos;
  typedef vector <tuple<idx_t, idx_t, char>> route; //route<x,y,outputPort>
  typedef map <packetId_t, sc_time> outputBuffer; //outputBuffer<packet_id,waiting_time>
  typedef map<uint8_t,outputBuffer> router; //router<outputBuffer_Name,outputBuffer>

  private:

    string NAME;
    DIAG_LEVEL DIAGNOSTIC_LEVEL;

    vector<id_struct>      CacheOutputs;
    vector<addr_struct>    MMappedOutputs;
    vector<id_addr_struct> HomeOutputs;

    sc_time ACCESS_LATENCY;
    bool ENABLE_LATENCY;

    /**
     * Performance counterss
    */
    vector<uint64_t> MMappedReadCountOut;
    vector<uint64_t> MMappedWriteCountOut;

    /**
     * Coherence performance counters
    */
    vector<uint64_t> CacheInvalCountOut;
    vector<uint64_t> HomeReadCountOut;
    vector<uint64_t> HomeWriteCountOut;
    vector<uint64_t> HomeCoherentCountOut;
    vector<uint64_t> TotalCoherentCountOut;

    uint32_t         InterleaveLength; //0 to disable interleaving
    uint64_t         RamBaseAddr;    //RAM base address used for interleaving
    uint64_t         RamLastAddr;    //RAM Last address (not included) used for interleaving
    uint32_t         IndexFirstMemoryController;

    uint32_t         MWordLengthInByte;      //Memory Word Length In Byte

    uint32_t         FlitSize;

    /**
     * NoC performance counters
    */
    uint64_t TotalDistance = 0;
    sc_time TotalLatency; // TODO initialize
    uint64_t PacketsCount  = 0;

    /**
     * Ports
    */
    const uint32_t NUM_CACHE_IN;
    const uint32_t NUM_CACHE_OUT;
    const uint32_t NUM_HOME_IN;
    const uint32_t NUM_HOME_OUT;
    const uint32_t NUM_MMAPPED;
    const uint32_t NUM_DEVICE;

    uint32_t CacheCount   = 0;
    uint32_t HomeCount    = 0;
    uint32_t MMappedCount = 0;

    bool IsCoherent;

    using SimpleInSocket  = tlm_utils::simple_target_socket<CoherenceInterconnect>;
    using SimpleOutSocket = tlm_utils::simple_initiator_socket<CoherenceInterconnect>;


    vector<tuple<uint64_t, uint64_t, mesh_pos>> mAddressIDs;
    vector<uint64_t> mReadCount, mWriteCount; //for address mapped components
    vector<tuple<uint64_t, mesh_pos>> mCpuIDs;
    vector<tuple<uint64_t, mesh_pos>> mDeviceIDs;
    vector<tuple<uint64_t, uint64_t, mesh_pos>> mHomeIDs;

    /**
     * a Mesh NoC model for NoC latency computation
    */
    bool mIsMesh;
    bool mNoc_stats_per_initiator_on;
    idx_t mX;
    idx_t mY;
    sc_time mRouterLatency;
    sc_time mLinkLatency;

    /**
     * NoC Contention variables and Data structures
    */
    sc_time mContentionInterval=sc_time(0, SC_NS);
    bool mWithContention;
    uint32_t mBufferSize;
    uint32_t mVirtualChannels;

    sc_time interval_start=sc_time(0, SC_NS);
    sc_time interval_end=sc_time(0, SC_NS);
    sc_time AverageLatency=sc_time(0, SC_NS);

    uint64_t totalFlits=0; // counter of the number of flits in the current Contention_Interval
    sc_time packet_latency; // total latency of a packet = router_crossing_latency + contention_latency
    sc_time AverageContentionDelay=sc_time(0, SC_NS);
    sc_time MaxLatency=sc_time(0, SC_NS);
    sc_time MinLatency=sc_time(1000000, SC_NS);
    sc_time MaxContentionDelay=sc_time(0, SC_NS);

    map <tuple<idx_t, idx_t>,router> noc; // noc<pos_x,pos_y,router>
    vector <tuple<packetId_t,route,sc_time>> packetBuffer; //packetBuffer<packet_id,path,packetlatency>, buffer used to save the packets that arrive during contention interval N

    /**
     * Routers performance counters (need contention mode)
    */
    vector<sc_time>  RouterTotalLatency;//Only contention related latency. Easy to add routing latency knowing the number of packets that traverse the router.
    vector<uint64_t> RouterPacketsCount;

  public:

    /**
     * All input sockets (Upstream and Downstream)
    */
    vector<SimpleInSocket*> mCacheSocketsIn;
    vector<SimpleInSocket*> mHomeSocketsIn;
    vector<SimpleInSocket*> mDeviceSocketsIn;

    /**
     * All output sockets (Upstream and Downstream)
    */
    vector<SimpleOutSocket*> mCacheSocketsOut;
    vector<SimpleOutSocket*> mHomeSocketsOut;
    vector<SimpleOutSocket*> mMMappedSocketsOut;

    map <idx_t,tuple<string,uint64_t,uint64_t,sc_time>> initTotalStats;// initTotalStats<source_id,<mesh_pos,nbr_pckts,dist,lat> used to hold (total; i.e. over all targets) perf information for each initiator

    inline bool isCoherent () { return IsCoherent; }
    inline uint32_t getMMappedCount () { return MMappedCount;  }

    inline uint64_t getReadMemoryCount  (idx_t port) { return MMappedReadCountOut[port];  }
    inline uint64_t getWriteMemoryCount (idx_t port) { return MMappedWriteCountOut[port]; }

    inline uint64_t getTotalDistance () { return TotalDistance; }
    inline sc_time getTotalLatencyWithContention  () {
      //latency of the packets arriving during CONTENTION INTERVAL I is computed at CONTENTION INTERVAL I+1. 
      //So, packets arriving during the last CONTENTION INTERVAL need to be handled seperately*/
      AverageLatency+=ComputePacketLatency();
      PacketsCount+=packetBuffer.size();
      return (AverageLatency);
    }

    inline sc_time getTotalLatency  () { return TotalLatency;  }
    inline uint64_t getPacketsCount () { return PacketsCount;  }

    inline sc_time getRouterTotalLatency (size_t x, size_t y) { return RouterTotalLatency[x+(y*mX)];  }
    inline uint64_t getRouterPacketsCount (size_t x, size_t y) { return RouterPacketsCount[x+(y*mX)];  }

    inline size_t   getMMappedSize() {return mAddressIDs.size();}
    inline std::pair<size_t, size_t> getMMappedPos (size_t index) { return std::pair<size_t,size_t>(get<2>(mAddressIDs[index]).x_id,get<2>(mAddressIDs[index]).y_id); }

    inline uint64_t getReadCount (size_t index) {return mReadCount[index]; }
    inline uint64_t getWriteCount(size_t index) {return mWriteCount[index];}

    CoherenceInterconnect (sc_module_name name, uint32_t nb_cache_in, uint32_t nb_cache_out, uint32_t nb_home_in, uint32_t nb_home_out, uint32_t nb_mmapped, uint32_t num_device, uint32_t flitSize, uint32_t wordLengthInByte, bool isCoherent, uint32_t interleaveLength);
    ~CoherenceInterconnect ();
    SC_HAS_PROCESS (CoherenceInterconnect);

    /**
    * Set functions
    */
    void set_cache_output   (uint32_t num_port, idx_t id);
    void set_mmapped_output (uint32_t num_port,         uint64_t base_addr, uint64_t offset);
    void set_home_output    (uint32_t num_port, idx_t id, uint64_t base_addr, uint64_t offset);
    void set_latency (sc_time val);
    void set_enable_latency (bool val);

    void set_cache_id  (uint32_t num_port, idx_t id, string name);
    void set_cache_pos (string name, uint32_t pos);

    /**
    * Get functions
    */
    int32_t get_port (uint64_t addr, uint64_t length);
    sc_time get_latency ();
    bool get_enable_latency ();

    /**
    * Statistics
    */
    void print_statistics ();

    /**
    * NoC latency functions
    */
    void set_router_latency (double nanoseconds);
    void set_link_latency (double nanoseconds);
    void set_is_mesh (bool is_mesh);
    void set_noc_stats_per_initiator (bool noc_stats_per_initiator_on);
    void set_mesh_coord (idx_t  x, idx_t y);
    void register_mem_ctrl (uint64_t base, uint64_t size, idx_t x_id, idx_t y_id);
    void register_cpu_ctrl (idx_t id, idx_t x_id, idx_t y_id);
    void register_device_ctrl (idx_t id, idx_t x_id, idx_t y_id);
    void register_home_ctrl (uint64_t base, uint64_t size, idx_t x_id, idx_t y_id);
    uint64_t get_ram_base_addr ();
    void set_ram_base_addr (uint64_t firstAddr);
    uint64_t get_ram_last_addr ();
    void set_ram_last_addr (uint64_t lastAddr);
    void set_memory_word_length (uint32_t wordLengthInByte);
    void set_first_memory_controller ();
    mesh_pos get_noc_pos_by_address (uint64_t addr, size_t& index);
    mesh_pos get_noc_pos_by_address_with_interleave (uint64_t addr, size_t& index);
    mesh_pos get_noc_pos_by_id (idx_t id);
    mesh_pos get_device_noc_pos_by_id (idx_t id);
    mesh_pos get_home_pos_by_address (uint64_t addr);
    uint64_t computeNoCLatency (bool isHome, bool isIdMapped, uint64_t addr, idx_t src_id, set<idx_t> dst_ids);
    void computeNoCPerformance (uint64_t distance, sc_time latency);

    void FillInitTotalStats(idx_t id, mesh_pos src_pos, uint64_t dist, sc_time lat);

    /**
    * NoC Contention model
    */
    void set_contention (bool with_contention);
    void set_contention_interval (double contention_interval);
    void set_buffer_size (uint32_t buffer_size);
    void set_virtual_channels (uint32_t virtual_channels);
    void SavePacket(idx_t id, route path, uint32_t nbFlits=1);
    void Create_Noc(idx_t noc_x, idx_t noc_y);
    route ComputeRouteAndUpdateRouters(mesh_pos src_pos, mesh_pos dst_pos,idx_t id, uint32_t nbFlits=1);
    sc_time QueueWaitingTime(sc_time wait, sc_time router_latency, sc_time link_latency, sc_time time_interval, uint64_t queue_nbr_packets);
    sc_time PacketLatency(sc_time total_wait, sc_time router_latency, sc_time link_latency, uint64_t nbr_hops);
    sc_time ComputePacketLatency();
    vector <mesh_pos> GetDestinations(tlm::tlm_generic_payload& trans, bool isHome, bool isIdMapped, set<idx_t> dst_ids);
    void NetworkTimingModel(tlm::tlm_generic_payload& trans, sc_time trans_time_stamp, sc_time time_interval,bool isHome, bool isIdMapped, uint32_t nbFlits, mesh_pos src_pos, set<idx_t> dst_ids, bool device = false);

    void PrintPath(route path);
    void PrintPacketBuffer();
    void PrintNoc();

    /**
     * TLM 2.0 communication interface
    */
    void sendTransactionToHome    (tlm::tlm_generic_payload& trans, sc_time& delay);
    void sendTransactionToCache   (tlm::tlm_generic_payload& trans, set<idx_t> targetIds, sc_time& delay);
    void sendTransactionToMMapped (tlm::tlm_generic_payload& trans, sc_time& delay);

    void b_transport (tlm::tlm_generic_payload& trans, sc_time& delay);
    void b_transport_device (tlm::tlm_generic_payload& trans, sc_time& delay);
    tlm::tlm_sync_enum nb_transport_fw (tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t);
    bool get_direct_mem_ptr (tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data);
    unsigned int transport_dbg (tlm::tlm_generic_payload& trans);
    tlm::tlm_sync_enum nb_transport_bw (tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t);
    void invalidate_direct_mem_ptr (sc_dt::uint64 start_range, sc_dt::uint64 end_range);
  };
}

#endif /* COHERENCEINTERCONNECT_HPP_ */


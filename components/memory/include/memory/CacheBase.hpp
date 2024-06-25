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

#ifndef CACHEBASE_HPP
#define CACHEBASE_HPP

#include <map>
#include <unordered_map>
#include <unordered_set>
#include <list>
#include <deque>
#include <bitset>
#include <iostream>
#include "systemc.h"
#include "CacheLine.hpp"
#include "CacheSet.hpp"
#include "CoherenceExtension.hpp"

using namespace std;

//!
//! Recursive template function Log2 provides the Log2 value of its template parameter N at compilation time
//! @tparam N the integer whose Log2 value is being evaluated
//! @tparam P the temporary count of number of division by 2 propagated throughout recursive calls
//!
template<int N,uint64_t P=0> struct _Log2 { enum { value = _Log2<N/2,P+1>::value }; };


//!
//! Log2<0, p> is a partial template specialization of log2<N,P> used as terminal of the recursion
//! It is the case where N reaches 0 (no longer divisable by 2), hence the number of division by 2 (P) is the Log2 value we return
//!
template <uint64_t P> struct _Log2<0, P> { enum { value = P }; };

//!
//! Log2<1, p> is a partial template specialization of log2<N,P> used as terminal of the recursion
//! It is the case where N reaches 1 (no longer divisable by 2), hence the number of division by 2 (P) is the Log2 value we return
//!
template <uint64_t P> struct _Log2<1, P> { enum { value = P }; };

constexpr uint64_t log2(uint64_t n){ return n < 2 ? 0 : 1 + log2(n / 2); }


namespace vpsim{

  //!
  //! All write policies for caches
  //!
  enum  CacheWritePolicy {
    WThrough,  //!< Write-through policy (always forward write to next level cache)
    WBack      //!< Write-back policy (forward write to next level cache only when line get removed from cache)
  };

  //!
  //! All allocation policies for caches
  //!
  enum  CacheAllocPolicy {
    WAllocate, //!< Write-allocate (do allocate data in cache for write that do miss)
    WAround //!< Write-around (do not allocate data in cache for write that do miss)
  };

  //!
  //! All inclusion policies for caches
  //!
  enum CacheInclusionPolicy {
    NINE,
    Inclusive, //!< Contains a copy of cache lines of the higher-level cache, not valid for L1
    Exclusive //!< Victim cache for the higher-level cache, not valid for L1
  };

  //enum CacheAccessMode { Read, Write, Evict, Invalidate };
  enum CacheLevel {
    LOne, // L1, emits requests only to lower-lvel caches
    Ln,   // Intermediate cache level, emits requests to lower- and higher-level caches
    LLC   // Least-level cache, emits requests only to higher-level caches
  };

  //!
  //! Generic cache structure that supports the following template parameters
  //! @tparam AddressType	the type of the addresses that the cache shall use (e.g uint64_t)
  //! @tparam CacheSize		the total size in Bytes of the cache in terms of storage. CacheSize impacts the number of lines in the cache
  //! @tparam CacheLineSize	the size (in Bytes) of each line of data in the cache
  //! @tparam Associativity	the associativity degree of the cache. An Associativity shall be >0, Associativity of 1 stands for direct mapped cache
  //!  Increasing the Associativity does not increase the size of the cache.
  //! @tparam ReplPolicy		the replacement policy of the cache (defaults to LRU, other values are not implemented yet)
  //! @tparam WritePolicy		the write policy of the cache (defaults to write-back)
  //! @tparam AllocPolicy		the allocation policy of the cache (defaults to write Allocate)
  //!
  //class template

  template <typename AddressType,
            typename WordType,
            bool WCETMode=false>

  class CacheBase : public sc_module {

  protected :

    // address bits
    // the address to a Bytes is of size [AddressBits] that can be decomposed in several fields [TagBits|IndexBits|OffsetBits]
    uint64_t AddressBits;                                     //!< number of bits composing a data address in the cache
    uint64_t OffsetBits;                                      //!< number of bits necessary to address a specific byte within a given CacheLine
    uint64_t IndexBits;                                       //!< number of bits necessary to address a specific cache set
    uint64_t TagBits;                                         //!< number of bits to be stored as tags to uniquely identify a CacheLine from within a set
    //address masks
    uint64_t OffsetMask;                                      //!< mask for the leastx-significant OffsetBits bits in a full AddressType
    uint64_t IndexMask;                                       //!< mask for the least-significant IndexBits bits in a full AddressType
    uint64_t TagMask;                                         //!< mask for the least-significant TagBits bits in a full AddressType
    //address Shifts
    uint64_t IndexShift;                                      //!< offset of the Index bits in the AddressType
    uint64_t TagShift;                                        //!< offset of the tag bits in the AddressType
    uint64_t CacheLineSize;
    uint64_t CacheSize;

    bool IsCoherent;

    bool NotifyEvictions;
    void (*NotifyEviction) (void* hdl);

  private :

    typedef CacheLine<AddressType> CacheLineType;               //!< specialization of CacheLine to the cache data types for easier line manipulation
    typedef CacheSet<CacheLineType, AddressType> CacheSetState; //!< structure representing the state of a CacheSet
    typedef vector<CacheSetState> CacheState;                   //!< structure representing the state of a CacheBase, i.e. the state of all its CacheSet(s)

    CacheState CacheLines;                                      //!< the current state of the CacheBase
    uint64_t NbLines;                                           //!< number of cache lines in the cache
    uint64_t NbSets;                                            //!< the number of sets in the cache
    // indeed associativity = NbLinesPerSet
    // For WCET
    // AbstractCacheState<AddressType,Associativity> acs[NbSets];
    // ACSImage acs;
    // unordered_set<AddressType> touched_sets;

    bool DataSupport;
    uint32_t Level;
    bool IsHome = false;

    uint64_t Associativity;
    CacheReplacementPolicy ReplPolicy;
    CacheWritePolicy WritePolicy;
    CacheAllocPolicy AllocPolicy;

    //uint64_t MaxLineSharers;
    //typedef uint64_t SharerIds [MaxLineSharers];
    typedef  set<idx_t> SharerIds;
    struct DirectoryEntry { CoherenceState State; idx_t Owner; SharerIds Sharers; };
    map<AddressType, DirectoryEntry> Directory; // Directory[3] = {Invalid, NULL_IDX, {0, 0, 0, 0} };
    map<AddressType, SharerIds> Sharers;

  public :

    uint64_t MissCount, HitCount, NReads, NWrites, NInvals, NTotalInvals, NBackInvals, NEvicts, WriteBacks, EvictBacks,
      HitReads, HitWrites, MissReads, MissWrites, NPutS, NPutM, NPutI, NGetS, NGetM, NFwdGetS, NFwdGetM, ReadBacks;
    CacheInclusionPolicy InclusionOfHigher, InclusionOfLower;
    idx_t IdTest;

    //!
    //! constructor of CacheBase
    //! @param name		a unique sc_module_name identifying the CacheBase module
    //!
    CacheBase(sc_module_name         name,
              uint64_t               cacheSize,
              uint64_t               cacheLineSize,
              uint64_t               associativity,
              CacheReplacementPolicy replPolicy = LRU,
              CacheWritePolicy writePolicy = WBack,
              CacheAllocPolicy allocPolicy = WAllocate,
              bool                   dataSupport = false,
              uint32_t               level = 1,
              CacheInclusionPolicy   inclusionOfHigher = NINE,
              CacheInclusionPolicy   inclusionOfLower = NINE,
              bool isHome = false,
              bool isCoherent = false,
              idx_t id = NULL_IDX)
    : sc_module         (name)
    , CacheLineSize     (cacheLineSize)
    , CacheSize         (cacheSize)
    , IsCoherent (isCoherent)
    , NotifyEvictions(false)
    , DataSupport       (dataSupport)
    , Level             (level)
    , IsHome (isHome)
    , Associativity     (associativity)
    , ReplPolicy        (replPolicy)
    , WritePolicy       (writePolicy)
    , AllocPolicy       (allocPolicy)
    , MissCount         (0)
    , HitCount          (0)
    , InclusionOfHigher (inclusionOfHigher)
    , InclusionOfLower  (inclusionOfLower)
    , IdTest (id)
      //, MaxLineSharers    (maxLineSharers)
    {
      NbLines = CacheSize / CacheLineSize;               //!< number of cache lines in the cache
      NbSets  = NbLines / Associativity;                 //!< the number of sets in the cache
      // indeed associativity = NbLinesPerSet
      assert (Associativity <= NbLines);                 // associativity cannot be greater than the number of lines
      //address bits
      // the address to a Bytes is of size [AddressBits] that can be decomposed in several fields [TagBits|IndexBits|OffsetBits]
      AddressBits = sizeof(AddressType) * 8;              //!< number of bits composing a data address in the cache
      OffsetBits  = (log2 (CacheLineSize));               //!< number of bits necessary to address a specific byte within a given CacheLine
      IndexBits   = (log2 (NbSets));                      //!< number of bits necessary to address a specific cache set
      TagBits     = AddressBits - IndexBits - OffsetBits; //!< number of bits that are to be stored as tags to uniquely identify a CacheLine from within a set
      //address masks
      OffsetMask  = (1ULL<<OffsetBits)-1;                 //!< mask for the least-significant OffsetBits bits in a full AddressType
      IndexMask   = (1ULL<<IndexBits)-1;                  //!< mask for the least-significant IndexBits bits in a full AddressType
      TagMask     = (1ULL<<TagBits)-1;                    //!< mask for the least-significant TagBits bits in a full AddressType
      //address Shifts
      IndexShift = OffsetBits;                            //!< offset of the Index bits in the AddressType
      TagShift = IndexBits+OffsetBits;                    //!< offset of the tag bits in the AddressType

      assert (ReplPolicy == LRU || !WCETMode);
      CacheLines.resize(NbSets);

      for (auto& set: CacheLines)
        set = CacheSet <CacheLineType, AddressType> (CacheLineSize, Associativity, ReplPolicy/*, higherCachesNb*/);

      cout << "Cache parameters: " << endl;
      cout << "Address bits: "     << AddressBits   << endl;
      cout << "Offset bits : "     << OffsetBits    << endl;
      cout << "IndexBits   : "     << IndexBits     << endl;
      //cout << "TagBits     : "     << TagBits       << endl;
      cout << "Nb sets     : "     << NbSets        << endl;
      cout << "Cache size  : "     << CacheSize     << endl;
      //cout << "NbLines     : "     << NbLines       << endl;
      cout << "Line size   : "     << CacheLineSize << endl;
      cout << "Is a home   : "     << IsHome << endl;

      NReads = NWrites = NInvals = NTotalInvals = NBackInvals = NEvicts = WriteBacks = EvictBacks = 0;
      NPutS = NPutM = NPutI = NGetS = NGetM = NFwdGetS = NFwdGetM = ReadBacks = 0;
    }

    //!
    //! Default destructor that displays stats for CacheBase upon destruction
    //!
    ~CacheBase() { displayStats(); }

    void SetEvictionNotifier(void(*ev)(void*)) {
      NotifyEvictions=true;
      NotifyEviction=ev;
    }

 template<CoherenceCommand accessMode>
  tlm::tlm_response_status accessNonCoherentCache (unsigned char* src_data_ptr, size_t size, AddressType addr, idx_t id, sc_time& delay, sc_time timestamp, void* handle=nullptr) {

    uint64_t offset = addr & OffsetMask;
    uint64_t index  = (addr>>IndexShift) & IndexMask;
    uint64_t tag    = (addr>>TagShift) & TagMask;
    tlm::tlm_response_status stat = tlm::TLM_OK_RESPONSE;
    CacheSetState& set = CacheLines [index];
    CacheLineType* line;
    bool isHit = set.accessSet (tag, &line);
    bool alignment;
    size_t accessSize;


    assert (!isHit||line->getState()!=Invalid);

    if (((Level==1)&&((accessMode==Read)||(accessMode==Write))) || ((Level!=1)&&(accessMode==Read))) {
      if (isHit) HitCount++; else MissCount++;
    }
    if (!isHit && line->getState() == Shared) {
        //if (line->getState()!=Invalid) {
      if (NotifyEvictions) {
        this->NotifyEviction(line->handle);
      }
    }
    if (accessMode==Invalidate) { // No replacement
      NTotalInvals++;
      if (isHit) {
        if (line->getState()==Modified) stat = ForwardWriteData (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
        line->setState(Invalid);
        NInvals++;
      }
    } else if (accessMode==ReadBack) {
      assert (isHit);
      ReadBacks++;
      //cacheMemcpy (src_data_ptr, line->getDataPtr()+addr-line->getAddress(), accessSize);
    } else if (!isHit && InclusionOfHigher==Exclusive && accessMode==Read) {
      assert(Level!=1); // addr is the line base address
      NReads++;
      if (Sharers.find(addr)!=Sharers.end() && Sharers[addr].size()!=0)
        stat = BackwardRead (src_data_ptr, addr, CacheLineSize, Sharers[addr], delay, timestamp);
      else
        stat = ForwardReadData (src_data_ptr, addr, CacheLineSize, delay, timestamp);
      Sharers[addr].insert(id);
      return stat; //"return" & multiline accesses: "return" can be used safely here since size=CacheLineSize if Level>1

    } else {
      assert (!isHit||(line->getAddress()<=addr && addr-line->getAddress()<CacheLineSize));
      // Write Back
      if (!isHit && line->getState()==Modified && WritePolicy==WBack) {
        WriteBacks++;
        stat = ForwardWriteData (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
        line->setState(Invalid);
      }
      // Eviction, assumes exclusive policy with lower cache
      if (!isHit && line->getState()==Shared && InclusionOfLower==Exclusive) {
        // assumes WBack policy
        stat = ForwardEvict (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
        EvictBacks++;
      }
      // Invalidation, assumes inclusive policy with higher cache
      if (InclusionOfHigher==Inclusive && !isHit && Sharers[line->getAddress()].size()!=0) {
        //stat = BackInvalidate (line->getAddress(), delay);
        stat = BackInvalidate (line->getAddress(), Sharers[line->getAddress()], delay, timestamp);
        Sharers[line->getAddress()].clear();
        NBackInvals++;
      }
      // Prepare new line on miss
      if (!isHit && AllocPolicy==WAllocate) {
        line->setNewLine(addr-offset, tag);
        line->handle = handle;
      }
      // Check and resolve non-aligned requests
      alignment = (addr-line->getAddress()+size) > CacheLineSize; // once a new line is set on miss
      if (alignment) accessSize = CacheLineSize-(addr-line->getAddress()); else accessSize = size;
      assert (accessSize>=0);
      assert (accessSize<=CacheLineSize);
      switch (accessMode) {
      case Read:
        assert(InclusionOfHigher!=Exclusive||isHit);
        Sharers[line->getAddress()].insert(id);
        if (!isHit) {
          stat = ForwardReadData (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp); // get data from memory
          line->setState(Shared);
        } else if (InclusionOfHigher==Exclusive){
          line->setState(Invalid);
        }
        cacheMemcpy (src_data_ptr, line->getDataPtr()+addr-line->getAddress(), accessSize);
        NReads++;
        break;
      case Write:
        Sharers[line->getAddress()].erase(id);
        if (InclusionOfHigher==Exclusive && !isHit) {
          if (Sharers.find(line->getAddress())!=Sharers.end() && Sharers[line->getAddress()].size()!=0) {
            line->setState(Invalid);
          } else {
            stat = ForwardReadData (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
            cacheMemcpy (line->getDataPtr()+addr-line->getAddress(), src_data_ptr, accessSize);
            line->setState(Modified);
          }
        } else {
          if (WritePolicy==WThrough) // Forward to next level
            stat = ForwardWriteData (line->getDataPtr(), addr, accessSize/*sizeof(WordType)*/, delay, timestamp);
          else { // WritePolicy==WBack
           if(!isHit) stat = ForwardReadData (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp); // Get line from memory
            cacheMemcpy (line->getDataPtr()+addr-line->getAddress(), src_data_ptr, accessSize);
            line->setState(Modified);
          }
        }
        // Inclusive, forward updated line to maintain inclusion
        if (InclusionOfLower==Inclusive) stat = ForwardWriteData (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
        /*if (Level != 1 && InclusionOfHigher==Exclusive) {
          //Sharers[line->getAddress()].erase(id);
          if (Sharers[line->getAddress()].size()!=0) {
            stat = ForwardWriteData (line->getDataPtr(), line->getAddress(), CacheLineSize, delay);
            line->setState(Invalid);
          }
          }*/
        NWrites++;
        break;
      case Evict: // From higher cache
        assert (InclusionOfHigher==Exclusive);
        cacheMemcpy (line->getDataPtr()+addr-line->getAddress(), src_data_ptr, accessSize);
        Sharers[line->getAddress()].erase(id);
        assert(!isHit||line->getState()==Modified);
        if (Sharers[line->getAddress()].size()!=0) line->setState(Invalid); else line->setState(Shared);
        NEvicts++;
        break;
      default:
        assert (false); break; //throw runtime_error ("Command prohibited in non-coherent mode\n");
      }
      if (alignment) {
        if (src_data_ptr) stat = this->accessNonCoherentCache<accessMode> (src_data_ptr+accessSize, size-accessSize, addr+accessSize, id, delay, timestamp, handle);
        else stat = this->accessNonCoherentCache<accessMode> (NULL, size-accessSize, addr+accessSize, id, delay, timestamp, handle);
      }
    }
    return stat;
  }


  template<CoherenceCommand accessMode>
  tlm::tlm_response_status accessCpuCache (unsigned char* src_data_ptr, size_t size, AddressType addr, idx_t id, sc_time& delay, sc_time timestamp, void* handle=nullptr) {
    uint64_t offset = addr & OffsetMask;
    uint64_t index  = (addr>>IndexShift) & IndexMask;
    uint64_t tag    = (addr>>TagShift) & TagMask;
    tlm::tlm_response_status stat = tlm::TLM_OK_RESPONSE;
    CacheSetState& set = CacheLines [index];
    CacheLineType* line;
    bool isHit = set.accessSet (tag, &line);
    bool alignment;
    size_t accessSize;

    if (accessMode==Read) {
      if (isHit) HitCount++; else MissCount++;
    } else if (accessMode==Write) {
      if (isHit && line->getState()==Modified) HitCount++; else MissCount++;
    }

    if (!isHit && line->getState() == Shared) {
      if (NotifyEvictions) this->NotifyEviction(line->handle);
    }

    assert (!isHit||(line->getAddress()<=addr && addr-line->getAddress()<CacheLineSize));
    // Proceed Non-allocating requests
    switch (accessMode) {
    case ReadBack:
      assert (false); // replaced by FwdGetS
      break;
    case FwdGetS:
      assert (id != NULL_IDX);
      assert (isHit); // Shared or Modified
      line->setState(Shared);
      NFwdGetS++;
      return stat;
      break;
    case FwdGetM:
      assert (id != NULL_IDX);
      assert (isHit);
      line->setState(Invalid);
      NFwdGetM++;
      return stat;
      break;
    case PutI:
      assert (isHit);
      assert (line->getState()==Shared);
      line->setState(Invalid);
      NPutI++;
      return stat;
      break;
    default:
      break;
    }
    // Writeback dirty line to make room for allocating requests
    if (!isHit && line->getState()!=Invalid && WritePolicy==WBack) {
      WriteBacks++;
      switch (line->getState()) {
      case Modified:
        stat = SendPutM (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
        line->setState(Invalid);
        break;
      case Shared:
        stat = SendPutS (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
        line->setState(Invalid);
        break;
      case Invalid:
        assert (false);
        break;
    default:
      break;
      }
    }
    // Writeback clean line to make room for allocating requests
    if (!isHit && line->getState()==Shared && InclusionOfLower==Exclusive) {
      assert (false); // L2-exclusive is only partially supported
      stat = ForwardEvict (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
      NEvicts++;
    }
    // Prepare new line for allocating requests
    if (!isHit && AllocPolicy==WAllocate) {
      line->setNewLine(addr-offset, tag);
      line->handle = handle;
    }
    assert (addr-offset==line->getAddress());
    // Align multiline requests
    alignment = (addr-line->getAddress()+size) > CacheLineSize; // once a new line is set on miss
    if (alignment) accessSize = CacheLineSize-(addr-line->getAddress()); else accessSize = size;
    assert (accessSize>=0);
    // Proceed allocating requests
    switch (accessMode) {
    case Read:
      if (!isHit) {
        stat = SendGetS (line->getDataPtr(), addr-offset, CacheLineSize, delay, timestamp); // get data from lower cache
        line->setState(Shared);
      }
      NReads++;
      break;
    case Write: // WritePolicy==WBack
      if (line->getState() != Modified) {
        stat = SendGetM (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
        line->setState(Modified);
      }
      NWrites++;
      break;
    default:
      assert(false); break;
    }

    if (alignment) {
      if (src_data_ptr) stat = this->accessCpuCache<accessMode> (src_data_ptr+accessSize, size-accessSize, addr+accessSize, id, delay, timestamp, handle);
      else stat = this->accessCpuCache<accessMode> (NULL, size-accessSize, addr+accessSize, id, delay, timestamp, handle);
    }
    return stat;
  }


  template<CoherenceCommand accessMode>
  tlm::tlm_response_status accessL2Cache (unsigned char* src_data_ptr, size_t size, AddressType addr, idx_t id, sc_time& delay, sc_time timestamp, void* handle=nullptr) {
    uint64_t offset = addr & OffsetMask;
    uint64_t index  = (addr>>IndexShift) & IndexMask;
    uint64_t tag    = (addr>>TagShift) & TagMask;
    tlm::tlm_response_status stat = tlm::TLM_OK_RESPONSE;
    CacheSetState& set = CacheLines [index];
    CacheLineType* line;
    bool isHit = set.accessSet (tag, &line);
    bool alignment;
    size_t accessSize;

    assert (offset==0);
    assert (size==CacheLineSize);
    assert (!isHit||(line->getAddress()<=addr && addr-line->getAddress()<CacheLineSize));

    assert (!isHit||addr==line->getAddress());
    assert(!(isHit && line->getState()==Modified && Directory[addr].State==Modified));

    if (accessMode==GetS) {
      if (isHit) HitCount++; else MissCount++;
    } else if (accessMode==GetM) {
      if (isHit && line->getState()==Modified) HitCount++; else MissCount++;
    }

    if (!isHit && line->getState() == Shared) {
      if (NotifyEvictions) this->NotifyEviction(line->handle);
    }
    // Proceed Non-allocating requests
    switch (accessMode) { // should be addr not line->getAddress here
    case FwdGetS: // In Exclusive L3, Readbacks are FwdGetSs
      assert (id != NULL_IDX);
      assert (Directory.find(addr) != Directory.end());
      assert (isHit||Directory[addr].State!=Invalid); // Shared or Modified
      assert (InclusionOfLower==Exclusive ||  ((isHit&&line->getState()==Modified)||Directory[addr].State==Modified));
      if (isHit && line->getState()==Modified) // on miss, line addr!=addr
        line->setState(Shared);
      if (!isHit&&Directory[addr].State==Shared) // Readback, exclusive policy
        stat = SendFwdGetS (src_data_ptr, addr, CacheLineSize, Directory[addr].Sharers, delay, timestamp);
      if (Directory[addr].State==Modified) {
        stat = SendFwdGetS (src_data_ptr, addr, CacheLineSize, {Directory[addr].Owner}, delay, timestamp);
        Directory[addr] = { Shared, NULL_IDX, {Directory[addr].Owner}};
      }
      assert (Directory[addr].State!=Modified);
      assert (Directory[addr].Owner==NULL_IDX);
      assert (!isHit || line->getState()==Shared);
      NFwdGetS++;
      return stat;
    case FwdGetM:
      assert (id != NULL_IDX);
      assert (Directory.find(addr) != Directory.end());
      assert((isHit&&line->getState()==Modified)||Directory[addr].State==Modified);
      if (isHit)  line->setState(Invalid);
      switch (Directory[addr].State) { // Invalid clean/dirty copy in L1
      case Shared:
        stat = SendFwdGetM (src_data_ptr, addr, CacheLineSize, {Directory[addr].Sharers}, delay, timestamp);
        Directory[addr] = { Invalid, NULL_IDX, {}};
        break;
      case Modified:
        stat = SendFwdGetM (src_data_ptr, addr, CacheLineSize, {Directory[addr].Owner}, delay, timestamp);
        Directory[addr] = { Invalid, NULL_IDX, {}};
        break;
      case Invalid: break;
      }
      NFwdGetM++;
      assert (Directory[addr].State==Invalid);
      assert (Directory[addr].Owner==NULL_IDX);
      assert (Directory[addr].Sharers.size()==0);
      assert (!isHit || line->getState()==Invalid);
      return stat;
    case PutS: // Replacement on higher cache, line being in shared state
      assert (Directory.find(addr) != Directory.end());
      assert (Directory[addr].State==Shared);
      Directory[addr].Sharers.erase(id);
      if (Directory[addr].Sharers.size()==0) {// Last PutS
        Directory[addr] = { Invalid, NULL_IDX, {} };
        if (!isHit) stat = SendPutS (src_data_ptr, addr, CacheLineSize, delay, timestamp); // Line is no longer in caches, update LLC
      }
      assert (Directory[addr].State!=Modified);
      assert (Directory[addr].Owner==NULL_IDX);
      //assert (Directory[addr].Sharers.size()==0); // does not hold if L2 is shared
      NPutS++;
      return stat;
    case PutI:
      assert (Directory.find(addr) != Directory.end());
      assert ((isHit&&line->getState()==Shared)||Directory[addr].State==Shared);
      assert (!isHit||(line->getState()!=Modified&&Directory[addr].State!=Modified));
      if (isHit) line->setState(Invalid);
      if (Directory[addr].State==Shared) {
        //assert (Directory[addr].Sharers.size()!=0); // redundant
        stat = SendPutI (src_data_ptr, addr, CacheLineSize, Directory[addr].Sharers, delay, timestamp);
        Directory[addr] = { Invalid, NULL_IDX, {}};
      }
      NPutI++;
      assert (Directory[addr].State==Invalid);
      assert (Directory[addr].Owner==NULL_IDX);
      assert (Directory[addr].Sharers.size()==0);
      assert (!isHit||line->getState()==Invalid);
      return stat;
    default :
      break;
    }
    // Write-back victim line (clean & dirty)
    if (!isHit && line->getState()!=Invalid && WritePolicy==WBack) { // line=victim line != req line
      assert (Directory.find(line->getAddress()) != Directory.end());
      //assert (line->getState()!=Modified||Directory[line->getAddress()].State!=Modified);
      WriteBacks++;
      switch (Directory[line->getAddress()].State) {
      case Invalid:
        if (line->getState()==Shared)
          stat = SendPutS (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
        else // line state = Modified
          stat = SendPutM (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
        break;
      case Shared:
        if (line->getState()==Modified)
          // TODO: Use another request type. GetS will be used for hit counting
          stat = SendGetS (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
        break;
      case Modified:
        break;
      }
      line->setState(Invalid);
    }
    // Prepare new line on miss

    if (!isHit && AllocPolicy==WAllocate) {
      line->setNewLine(addr, tag);
      line->handle = handle;
      assert (line->getState()==Invalid);
    }

    // Proceed allocating requests
    switch (accessMode) {
    case PutM: // Replacement on higher cache, line being in modified state
        assert (Directory.find(line->getAddress()) != Directory.end());
        switch (Directory[line->getAddress()].State) {
        case Invalid:
          assert(false); break;
        case Shared: // When this case occurs ?
          assert(false);
          Directory[line->getAddress()].Sharers.erase(id);
          if (Directory[line->getAddress()].Sharers.size()==0)  // Last PutM
            Directory[line->getAddress()] ={ Invalid, NULL_IDX, {} };
          break;
        case Modified:
          assert (id == Directory[line->getAddress()].Owner);
          line->setState(Modified);
          Directory[line->getAddress()] = { Invalid, NULL_IDX, {} };
          break;
        }
        NPutM++;
        break;

    case GetS:
      if (Directory.find(line->getAddress()) == Directory.end()) {
        assert (!isHit);
        stat = SendGetS (line->getDataPtr(), addr, CacheLineSize, delay, timestamp);
        line->setState(Shared);
        Directory[line->getAddress()] = { Shared, NULL_IDX, {id} };
      } else {
        switch (Directory[line->getAddress()].State) {
        case Invalid:
          if (!isHit) {
            stat = SendGetS (line->getDataPtr(), addr, CacheLineSize, delay, timestamp);
            line->setState(Shared);
          }
          Directory[line->getAddress()] = { Shared, NULL_IDX, {id} };
          break;
        case Shared:
          assert (find (Directory[line->getAddress()].Sharers.begin(),Directory[line->getAddress()].Sharers.end(),id)==Directory[line->getAddress()].Sharers.end());
          if (!isHit) {
            stat = SendFwdGetS (line->getDataPtr(), line->getAddress(), CacheLineSize, Directory[line->getAddress()].Sharers, delay, timestamp);
            line->setState(Shared);
          }
          Directory[line->getAddress()].Sharers.insert(id);
          break;
        case Modified:
          assert (id != Directory[line->getAddress()].Owner);
          stat = SendFwdGetS (line->getDataPtr(), line->getAddress(), CacheLineSize, {Directory[line->getAddress()].Owner}, delay, timestamp); // get updated data
          Directory[line->getAddress()] = {Shared, NULL_IDX, {id,  Directory[line->getAddress()].Owner}};
          line->setState(Modified);
          break;
        }
      }
      assert (Directory[line->getAddress()].State==Shared);
      assert (Directory[line->getAddress()].Owner==NULL_IDX);
      assert (Directory[line->getAddress()].Sharers.size()!=0);
      NGetS++;
      break;

    case GetM:
      if (Directory.find(line->getAddress()) == Directory.end()) {
        assert (!isHit);
        stat = SendGetM (line->getDataPtr(), addr, CacheLineSize, delay, timestamp);
        Directory[line->getAddress()] = { Modified, id, {} };
      } else {
        switch (Directory[line->getAddress()].State) {
        case Invalid:
          if (line->getState()!=Modified) {
            stat = SendGetM (line->getDataPtr(), addr, CacheLineSize, delay, timestamp);
          }
          Directory[line->getAddress()] = { Modified, id, {} };
          break;
        case Shared:
          if (line->getState()!=Modified) stat = SendGetM (line->getDataPtr(), addr, CacheLineSize, delay, timestamp);
          Directory[line->getAddress()].Sharers.erase(id);
          if (Directory[line->getAddress()].Sharers.size()!=0)
            stat = SendPutI (line->getDataPtr(), addr, CacheLineSize, Directory[line->getAddress()].Sharers, delay, timestamp);
          Directory[line->getAddress()] = { Modified, id, {} };
          break;
        case Modified: // What should be the line state
          assert (id != Directory[line->getAddress()].Owner);
          stat = SendFwdGetM (line->getDataPtr(), line->getAddress(), CacheLineSize, {Directory[line->getAddress()].Owner}, delay, timestamp); // give updated data to requester
          Directory[line->getAddress()].Owner = id;
          break;
        }
      }
      line->setState(Shared);
      assert (Directory[line->getAddress()].State==Modified);
      assert (Directory[line->getAddress()].Owner==id);
      assert (Directory[line->getAddress()].Sharers.size()==0);
      NGetM++;
      break;

    case PutI:
      assert (Directory.find(line->getAddress()) != Directory.end());
      assert (line->getState()==Shared||Directory[line->getAddress()].State==Shared);
      if (Directory[line->getAddress()].State==Shared) {
        assert (Directory[line->getAddress()].Sharers.size()!=0);
        stat = SendPutI (line->getDataPtr(), addr, CacheLineSize, Directory[line->getAddress()].Sharers, delay, timestamp);
        Directory[line->getAddress()] = { Invalid, NULL_IDX, {} };
      }
      assert (Directory[line->getAddress()].State==Invalid);
      assert (Directory[line->getAddress()].Owner==NULL_IDX);
      assert (Directory[line->getAddress()].Sharers.size()==0);
      if (line->getState()!=Invalid) line->setState(Invalid);
      NPutI++;
      break;

    default:
      assert(false); break; //throw runtime_error ("Command prohibited for local coherent caches\n"); break;
    }
    assert (Directory.find(line->getAddress())==Directory.end() ||
            (Directory[addr].State==Invalid    && Directory[addr].Owner==NULL_IDX && Directory[addr].Sharers.size()==0) ||
            (Directory[addr].State==Shared   && Directory[addr].Owner==NULL_IDX && Directory[addr].Sharers.size()!= 0) ||
            (Directory[addr].State==Modified && Directory[addr].Owner!=NULL_IDX  && Directory[addr].Sharers.size()==0));
    return stat;
  }

    template<CoherenceCommand accessMode>
    tlm::tlm_response_status accessCoherentHome (unsigned char* src_data_ptr, size_t size, AddressType addr, idx_t id, sc_time& delay, sc_time timestamp, void* handle=nullptr) {
      uint64_t offset = addr & OffsetMask;
      uint64_t index  = (addr>>IndexShift) & IndexMask;
      uint64_t tag    = (addr>>TagShift) & TagMask;
      tlm::tlm_response_status stat = tlm::TLM_OK_RESPONSE;
      CacheSetState& set = CacheLines [index];
      CacheLineType* line;
      bool isHit = set.accessSet (tag, &line);
      bool alignment;
      //size_t accessSize;

      assert (id!=NULL_IDX);
      assert (offset==0);
      assert (size==CacheLineSize);
      assert (!isHit||(line->getAddress()<=addr && addr-line->getAddress()<CacheLineSize));

      assert (!isHit||addr==line->getAddress());
      assert(!(isHit && line->getState()==Modified && Directory[addr].State==Modified));

      if (accessMode==GetS) {
        if (isHit) HitCount++; else MissCount++;
      } else if (accessMode==GetM) {
        if (isHit && line->getState()==Modified) HitCount++; else MissCount++;
      }

      if (!isHit && line->getState() == Shared) {
        if (NotifyEvictions) this->NotifyEviction(line->handle);
      }

      // If exclusive cache, proceed non-allocating requests on miss
      if ((InclusionOfHigher==Exclusive) && !isHit) {
        // TODO: factorize code GetS/GetM
        // Use addr rather than line (line is possibly not allocated)
        switch (accessMode) {
        case GetS:
          if (Directory.find(addr)==Directory.end()) { // Line has never been requested
            stat = ForwardReadData (src_data_ptr, addr, CacheLineSize, delay, timestamp);
            Directory[addr] = { Shared, NULL_IDX, {id} };
          } else {
            switch (Directory[addr].State) {
            case Invalid: // Line is not in upper cache
              stat = ForwardReadData (src_data_ptr, addr, CacheLineSize, delay, timestamp);
              Directory[addr] = { Shared, NULL_IDX, {id} };
              break;
            case Shared:// Line is in upper cache
              stat = SendFwdGetS (src_data_ptr, addr, CacheLineSize, Directory[addr].Sharers, delay, timestamp);
              Directory[addr].Sharers.insert(id);
              break;
            case Modified:
              // if Owner==id,  LLC should have latest version
              if (Directory[addr].Owner!=id)
                stat = SendFwdGetS (src_data_ptr, addr, CacheLineSize, {Directory[addr].Owner}, delay, timestamp);
              Directory[addr] = { Shared, NULL_IDX, {id,  Directory[addr].Owner}};
              break;
            }
          }
          assert (Directory[addr].State==Shared);
          assert (Directory[addr].Owner==NULL_IDX);
          assert (Directory[addr].Sharers.size()!=0);
          NGetS++;
          return stat;
          //break;
        case GetM:
          if (Directory.find(addr)==Directory.end()) { // Line has never been requested
            stat = ForwardReadData (src_data_ptr, addr, CacheLineSize, delay, timestamp);
            Directory[addr] = { Modified, id, {} };
          } else {
            switch (Directory[addr].State) {
            case Invalid: // Line is not in upper cache
              stat = ForwardReadData (src_data_ptr, addr, CacheLineSize, delay, timestamp);
              Directory[addr] = { Modified, id, {} };
              break;
            case Shared: // Line is in upper cache
              //if (!(Directory[line->getAddress()].Sharers.size()==1&&Directory[line->getAddress()].Sharers.find(id)))
              assert (Directory[addr].Sharers.size()!=0);
              Directory[addr].Sharers.erase(id);
              if (Directory[addr].Sharers.size()!=0)
                stat = SendPutI (src_data_ptr, addr, CacheLineSize, Directory[addr].Sharers, delay, timestamp);
              Directory[addr] = {Modified, id, {}};
              break;
            case Modified:
              assert (Directory[addr].Owner!=NULL_IDX);
              assert (Directory[addr].Owner != id);
              stat = SendFwdGetM (src_data_ptr, addr, CacheLineSize, {Directory[addr].Owner}, delay, timestamp);
              Directory[addr].Owner = id;
              break;
            }
          }
          assert (Directory[addr].State==Modified);
          assert (Directory[addr].Owner==id);
          assert (Directory[addr].Sharers.size()==0);
          NGetM++;
          return stat;
          //break;
        default:
          break;
        }
      }
      // Write-back dirty victim line
      if (!isHit && line->getState()==Modified && WritePolicy==WBack) {//TODO: WritePolicy==WBack in coherent mode
        WriteBacks++;
        stat = ForwardWriteData (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
      }
      // Prepare new line for allocating requests
      if (!isHit && AllocPolicy==WAllocate) {
        line->setNewLine(addr, tag);
        line->handle = handle;
      }
      assert (addr==line->getAddress());
      // Proceed allocating requests
      switch (accessMode) {

      case PutS: //PutS is only allocating in exclusive cache
        assert (Directory.find(line->getAddress()) != Directory.end());
        assert (Directory[line->getAddress()].State==Shared);
        switch (Directory[line->getAddress()].State) {
        case Invalid: case Modified:
          assert (false); // do these cases happen ?
          assert (Directory[line->getAddress()].Sharers.size()==0);
          break;
        case Shared:
            assert (Directory[line->getAddress()].Sharers.size()>0);
            assert (Directory[line->getAddress()].Owner==NULL_IDX);
            assert(InclusionOfHigher!=Exclusive||!isHit);
            Directory[line->getAddress()].Sharers.erase(id);
            if (Directory[line->getAddress()].Sharers.size()==0) {// Last PutS
              Directory[line->getAddress()] = { Invalid, NULL_IDX, {} };
              if (InclusionOfHigher==Exclusive) line->setState(Shared);
            }
            break;
        }
        assert (Directory[line->getAddress()].State!=Modified);
        assert (Directory[line->getAddress()].Owner==NULL_IDX);
        NPutS++;
        break;

      case PutM:
        // cache behaviour
        line->setState(Modified);
        // directory behaviour
        assert (Directory.find(line->getAddress()) != Directory.end());
        switch (Directory[line->getAddress()].State) {
        case Invalid: case Shared: assert(false); break; // Maybe for shared, remove req from sharers
        case Modified:
          assert (Directory[line->getAddress()].Owner!=NULL_IDX);
          assert (Directory[line->getAddress()].Sharers.size()==0);
          assert (id == Directory[line->getAddress()].Owner);
          Directory[line->getAddress()] = { Invalid, NULL_IDX, {} };
          break;
        }
        NPutM++;
        break;

      case GetS: // TODO: on GetS, line should be either in L2 or in L3
        if (Directory.find(line->getAddress())==Directory.end()) { // Line has never been requested
          assert (!isHit); // first request
          assert (InclusionOfHigher!=Exclusive);
          stat = ForwardReadData (src_data_ptr, line->getAddress(), CacheLineSize, delay, timestamp);
          line->setState(Shared);
          Directory[line->getAddress()] = { Shared, NULL_IDX, {id} };
        } else {
          switch (Directory[line->getAddress()].State) {
          case Invalid: // Line is not in upper cache
            assert (Directory[line->getAddress()].Sharers.size()==0);
            assert (Directory[line->getAddress()].Owner==NULL_IDX);
            if (!isHit) {
              assert (InclusionOfHigher!=Exclusive);
              stat = ForwardReadData (src_data_ptr, line->getAddress(), CacheLineSize, delay, timestamp);
              line->setState(Shared);
            } else if (InclusionOfHigher==Exclusive) {
              if (line->getState()==Modified)
                stat = ForwardWriteData (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp); // clean line
              line->setState(Invalid);
            }
            Directory[line->getAddress()] = { Shared, NULL_IDX, {id} };
            break;
          case Shared: // Line is clean in upper cache
            assert (Directory[line->getAddress()].Owner==NULL_IDX);
            assert (Directory[line->getAddress()].Sharers.size()>0);
            if (!isHit) {
              assert (InclusionOfHigher!=Exclusive);
              stat = SendFwdGetS (line->getDataPtr(), line->getAddress(), CacheLineSize, Directory[line->getAddress()].Sharers, delay, timestamp);
              line->setState(Shared);
            } else if (InclusionOfHigher==Exclusive) {
              if (line->getState()==Modified)
                stat = ForwardWriteData (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp); // clean line
              line->setState(Invalid);
            }
            Directory[line->getAddress()].Sharers.insert(id);
            break;
          case Modified: // Line is dirty in upper cache
            assert (Directory[line->getAddress()].Sharers.size()==0);
            assert (Directory[line->getAddress()].Owner!= NULL_IDX);
            //if (Directory[line->getAddress()].Owner!=id) // should neccesssarily be owner!=id if correctly designed
            if (Directory[line->getAddress()].Owner!=id) // possible if eq to PutMGetS
              stat = SendFwdGetS (line->getDataPtr(), line->getAddress(), CacheLineSize, {Directory[line->getAddress()].Owner}, delay, timestamp); // get updated data
            Directory[line->getAddress()] = {Shared, NULL_IDX, {id,  Directory[line->getAddress()].Owner}};
            if (InclusionOfHigher==Exclusive) {
              assert (isHit); // Exclusive is non-allocating
              stat = ForwardWriteData (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
              line->setState(Invalid);
            } else line->setState(Modified);
            break;
          }
        }
        NGetS++;
        assert (Directory[line->getAddress()].State==Shared);
        assert (Directory[line->getAddress()].Owner==NULL_IDX);
        assert (Directory[line->getAddress()].Sharers.size()!=0);
        break;

      case GetM:
        if (Directory.find(line->getAddress())==Directory.end()) { // Line has never been requested
          assert (!isHit); // first request
          assert (InclusionOfHigher!=Exclusive); // No line allocation in exclusive caches
          stat = ForwardReadData (src_data_ptr, line->getAddress(), CacheLineSize, delay, timestamp);
          line->setState(Shared); // Line is not dirty yet
          Directory[line->getAddress()] = { Modified, id, {} };
        } else {
          switch (Directory[line->getAddress()].State) {
          case Invalid: // Line is not in upper cache
            assert (Directory[line->getAddress()].Sharers.size()==0);
            assert (Directory[line->getAddress()].Owner==NULL_IDX);
            if (!isHit) {
              assert (InclusionOfHigher!=Exclusive); // No line allocation in exclusive caches
              stat = ForwardReadData (src_data_ptr, line->getAddress(), CacheLineSize, delay, timestamp);
              line->setState(Shared); // Line is not dirty yet
            } else if (InclusionOfHigher==Exclusive && line->getState()==Modified) { // clean line before invalidation
              stat = ForwardWriteData (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
              line->setState(Invalid);
            }
            Directory[line->getAddress()] = { Modified, id, {} };
            break;
          case Shared: // Line is clean in upper cache
            assert (Directory[line->getAddress()].Owner==NULL_IDX);
            assert (Directory[line->getAddress()].Sharers.size()>0);
            if (!isHit) {
              assert (InclusionOfHigher!=Exclusive); // No line allocation in exclusive caches
              stat = SendFwdGetS (line->getDataPtr(), line->getAddress(), CacheLineSize, Directory[line->getAddress()].Sharers, delay, timestamp);
              line->setState(Shared);
            } else if (InclusionOfHigher==Exclusive && line->getState()==Modified) { // clean line before invalidation
              stat = ForwardWriteData (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
              line->setState(Invalid);
            }
            Directory[line->getAddress()].Sharers.erase(id);
            assert(Directory[line->getAddress()].Sharers.find(id) == Directory[line->getAddress()].Sharers.end());
            if (Directory[line->getAddress()].Sharers.size() != 0) // Invalidate all sharers
              stat = SendPutI (line->getDataPtr(), addr, CacheLineSize, Directory[line->getAddress()].Sharers, delay, timestamp);
            Directory[line->getAddress()] = { Modified, id, {} };
            break;
          case Modified: // Line is dirty in upper cache
            assert (Directory[line->getAddress()].Owner!=NULL_IDX);
            assert (Directory[line->getAddress()].Owner != id);
            assert (Directory[line->getAddress()].Sharers.size()==0);
            stat = SendFwdGetM (line->getDataPtr(), line->getAddress(), CacheLineSize, {Directory[line->getAddress()].Owner}, delay, timestamp);
            Directory[line->getAddress()].Owner = id;
            // Line is home and up-to-date, give dirty line to requester, no need for writeback
            //stat = ForwardWriteData (line->getDataPtr(), line->getAddress(), CacheLineSize, delay, timestamp);
            if (InclusionOfHigher==Exclusive) line->setState(Invalid);
            // else line->setState(Modified);
            break;
          }
        }
        NGetM++;
        assert (Directory[line->getAddress()].State==Modified);
        assert (Directory[line->getAddress()].Owner==id);
        assert (Directory[line->getAddress()].Sharers.size()==0);
        break;

      default:
        assert(false); break; //throw runtime_error ("Command non allowed for home\n");
      }

      assert (Directory.find(line->getAddress())==Directory.end() ||
              (Directory[addr].State==Invalid    && Directory[addr].Owner==NULL_IDX && Directory[addr].Sharers.size()==0) ||
              (Directory[addr].State==Shared   && Directory[addr].Owner==NULL_IDX && Directory[addr].Sharers.size()!= 0)  ||
              ( Directory[addr].State==Modified && Directory[addr].Owner!=NULL_IDX && Directory[addr].Sharers.size()==0));

      return stat;
    }


    //!
    //! function used to access the cache and update its state
    //! @tparam WriteMode, true if the access is actually a write
    //! @param [in] Addr 		the address that needs to be read or written
    //! @param [in] NbBytes 		the number of successive bytes starting from Addr that this access covers
    //! @param [in,out] SrcDestBuffer 	an allocated buffer of NbBytes size from/to which data is read/written depending on WriteMode
    //!
    /*template<CacheAccessMode accessMode>*/
    /*  template<CoherenceCommand accessMode>
  tlm::tlm_response_status accessCache (unsigned char* src_data_ptr, size_t size, AddressType addr, int id, sc_time& delay, sc_time timestamp=sc_time(0,SC_NS), void* handle=nullptr) {
    // id contains the initiator Id for downstream transactions and the target id for upstream transactions
    // TODO add verif on ids
    if (!IsCoherent)
      return this-> accessNonCoherentCache<accessMode>(src_data_ptr, size, addr, id, delay, timestamp, handle);
    else {
      if (!IsHome) return this-> accessCoherentLocalCache<accessMode>(src_data_ptr, size, addr, id, delay, timestamp, handle);
      else         return this-> accessCoherentHome<accessMode>(src_data_ptr, size, addr, id, delay, timestamp, handle);
    }
    }*/
  template<CoherenceCommand accessMode>
  tlm::tlm_response_status accessCache (unsigned char* src_data_ptr, size_t size, AddressType addr, idx_t id, sc_time& delay, sc_time timestamp=sc_time(0,SC_NS), void* handle=nullptr) {
    // id contains the initiator Id for downstream transactions and the target id for upstream transactions
    tlm::tlm_response_status stat = tlm::TLM_OK_RESPONSE;
    if (!IsCoherent)
      stat = this-> accessNonCoherentCache<accessMode>(src_data_ptr, size, addr, id, delay, timestamp, handle);
    else {
      if (IsHome) stat = this-> accessCoherentHome<accessMode>(src_data_ptr, size, addr, id, delay, timestamp, handle);
      else
        switch (Level) {
        case 1:
          stat = this-> accessCpuCache<accessMode>(src_data_ptr, size, addr, id, delay, timestamp, handle);
          break;
        case 2:
          stat = this-> accessL2Cache<accessMode>(src_data_ptr, size, addr, id, delay, timestamp, handle);
          break;
        default:
          assert (false);
          break;
        }
    }
    return stat;
  }

    inline bool isDataSupported (){
      return DataSupport;
    }
    inline void cacheMemcpy (unsigned char* dest_ptr, unsigned char* src_ptr, size_t size){
      if (DataSupport) memcpy (dest_ptr, src_ptr, size);
      else return;
    }

    //!
    //! Displays the access counts and miss rate of the CacheBase since the beginning of the simulation
    //!
    void displayStats () {
      uint64_t AccessCount = MissCount + HitCount + NInvals + NEvicts;
      double   MissRate    = (AccessCount>0) ? ((double)MissCount)/AccessCount : 0;
      cout << this->name() << ": MissCount "      << MissCount   << " , HitCount " << HitCount << endl;
      cout << this->name() << ": total accesses " << AccessCount << " , MissRate " << MissRate;
      cout << " writes: "  << NWrites << " reads: "  << NReads << " WriteBacks: " << WriteBacks;
      if (InclusionOfLower==Inclusive)  cout << " total invalidations: " << NTotalInvals << " real invalidations: " << NInvals;

      if (InclusionOfLower==Exclusive) cout << " evictions: " << NEvicts;
      cout << endl;
    }

    //!
    //! Function called by the cache itself whenever it must forward a read access to a next-level cache
    //! This function may be overriden to implement relevant TLM interfaces for instance
    //! @param [in] Addr				the address that needs to be written
    //! @param [in] NbBytes				the number of successive bytes starting from Addr that this access covers
    //! @param [in,out] TargetBuffer	an allocated buffer of NbBytes size from which access data can be written
    //!
    virtual tlm::tlm_response_status ForwardRead (AddressType addr, size_t size, sc_time& delay) {
      return tlm::TLM_OK_RESPONSE;
    };
    virtual tlm::tlm_response_status ForwardRead (AddressType addr, size_t size, sc_time& delay, sc_time timestamp) {
      return tlm::TLM_OK_RESPONSE;
    };

    virtual tlm::tlm_response_status ForwardReadData (unsigned char* cacheLineData, AddressType Addr, size_t size, sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }
    virtual tlm::tlm_response_status BackwardRead (unsigned char* lineDataPtr, AddressType addr, size_t size, set<idx_t> sharerIds, sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }

    //!
    //! Function called by the cache itself whenever it must forward a write access to a next-level cache
    //! This function may be overriden to implement relevant TLM interfaces for instance
    //! @param [in] Addr	 	the address that needs to be written
    //! @param [in] NbBytes	 	the number of successive bytes starting from Addr that this access covers
    //! @param [in] SrcBuffer	an allocated buffer of NbBytes size from which access data can be read
    //!
    virtual tlm::tlm_response_status ForwardWrite (AddressType addr, size_t size, sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }
    virtual tlm::tlm_response_status ForwardWriteData (unsigned char* cacheLineData, AddressType addr, size_t size, sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }
    virtual tlm::tlm_response_status ForwardEvict (unsigned char* cacheLineData, AddressType addr, size_t size, sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }
    //virtual tlm::tlm_response_status BackInvalidate (AddressType addr, sc_time& delay) { return tlm::TLM_OK_RESPONSE; }
    virtual tlm::tlm_response_status BackInvalidate (AddressType addr, set<idx_t> sharerIds, sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }
    virtual tlm::tlm_response_status SendGetS (unsigned char* lineDataPtr, AddressType addr, size_t size, sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }
    virtual tlm::tlm_response_status SendGetM (unsigned char* lineDataPtr, AddressType addr, size_t size, sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }
    virtual tlm::tlm_response_status SendPutS (unsigned char* lineDataPtr, AddressType addr, size_t size, sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }
    virtual tlm::tlm_response_status SendPutM (unsigned char* lineDataPtr, AddressType addr, size_t size, sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }
    virtual tlm::tlm_response_status SendFwdGetS (unsigned char* lineDataPtr, AddressType addr, size_t size, set<idx_t> sharerIds, /*const int id,*/ sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }
    virtual tlm::tlm_response_status SendFwdGetM (unsigned char* lineDataPtr, AddressType addr, size_t size, set<idx_t> ids, sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }
    virtual tlm::tlm_response_status SendPutI (unsigned char* lineDataPtr, AddressType addr, size_t size, set<idx_t> sharerIds, sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }
    virtual tlm::tlm_response_status SendInvS (unsigned char* lineDataPtr, AddressType addr, size_t size, set<idx_t> sharerIds, sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }
    virtual tlm::tlm_response_status SendInvM (unsigned char* lineDataPtr, AddressType addr, size_t size, idx_t id, sc_time& delay, sc_time timestamp) { return tlm::TLM_OK_RESPONSE; }


    //!
    //! Performs a read access to the cache and returns the valid value read in the cache in TargetBuffer
    //! @param [in] Addr			the address that needs to be written
    //! @param [in] NbBytes			the number of successive bytes starting from Addr that this access covers
    //! @param [out] TargetBuffer	an allocated buffer of NbBytes size from which access data can be read
    //!
    /*virtual tlm::tlm_response_status Read (AddressType addr, sc_time& delay){
      return AccessCache<false>(addr,  delay);
      }*/
    virtual inline tlm::tlm_response_status ReadData (unsigned char* data_ptr, AddressType addr, size_t size,  idx_t initiatorId, sc_time& delay, sc_time timestamp, void* handle=nullptr){
      if (DataSupport) return accessCache<Read>(data_ptr, size, addr, initiatorId, delay, timestamp, handle);
      else             return accessCache<Read>(NULL, size, addr, initiatorId, delay, timestamp, handle);
    }

    //!
    //! Performs a write access to the cache
    //! @param [in] Addr			the address that needs to be written
    //! @param [in] NbBytes			the number of successive bytes starting from Addr that this access covers
    //! @param [out] TargetBuffer	an allocated buffer of NbBytes size from which access data can be read
    //!
    /*    virtual tlm::tlm_response_status Write (AddressType addr, sc_time& delay){
      return AccessCache<true>(addr,  delay);
      } */
    virtual inline tlm::tlm_response_status WriteData (unsigned char* data_ptr, AddressType addr, size_t size, idx_t initiatorId, sc_time& delay, sc_time timestamp, void* handle=nullptr){
      if (DataSupport) return accessCache<Write>(data_ptr, size, addr, initiatorId, delay, timestamp, handle);
      else             return accessCache<Write>(NULL, size, addr, initiatorId, delay, timestamp, handle);
    }

    virtual inline tlm::tlm_response_status InvalidateLine (AddressType addr, sc_time& delay, sc_time timestamp){
      return accessCache<Invalidate>(NULL, CacheLineSize, addr, NULL_IDX, delay, timestamp, nullptr);
    }
    virtual inline tlm::tlm_response_status EvictLine (unsigned char* data_ptr, AddressType addr, size_t size, idx_t initiatorId, sc_time& delay, sc_time timestamp){
      if (DataSupport) return accessCache<Evict>(data_ptr, size, addr, initiatorId, delay, timestamp, nullptr);
      else             return accessCache<Evict>(NULL,     size, addr, initiatorId, delay, timestamp, nullptr);
    }
    virtual inline tlm::tlm_response_status AccessGetM (unsigned char* data_ptr, AddressType addr, size_t size, idx_t initiatorId, sc_time& delay, sc_time timestamp){
      if (DataSupport) return accessCache<GetM>(data_ptr, size, addr, initiatorId, delay, timestamp, nullptr);
      else             return accessCache<GetM>(NULL, size, addr, initiatorId, delay, timestamp, nullptr);
    }
    virtual inline tlm::tlm_response_status AccessGetS (unsigned char* data_ptr, AddressType addr, size_t size, idx_t initiatorId, sc_time& delay, sc_time timestamp){
      if (DataSupport) return accessCache<GetS>(data_ptr, size, addr, initiatorId, delay, timestamp, nullptr);
      else             return accessCache<GetS>(NULL, size, addr, initiatorId, delay, timestamp, nullptr);
    }
    virtual inline tlm::tlm_response_status AccessFwdGetM (unsigned char* data_ptr, AddressType addr, size_t size, idx_t initiatorId, sc_time& delay, sc_time timestamp){
      if (DataSupport) return accessCache<FwdGetM>(data_ptr, size, addr, initiatorId, delay, timestamp, nullptr);
      else             return accessCache<FwdGetM>(NULL, size, addr, initiatorId, delay, timestamp, nullptr);
    }
    virtual inline tlm::tlm_response_status AccessFwdGetS (unsigned char* data_ptr, AddressType addr,  size_t size, idx_t initiatorId, sc_time& delay, sc_time timestamp){
      if (DataSupport) return accessCache<FwdGetS>(data_ptr, size, addr, initiatorId, delay, timestamp, nullptr);
      else             return accessCache<FwdGetS>(NULL, size, addr, initiatorId, delay, timestamp, nullptr);
    }
    virtual inline tlm::tlm_response_status AccessPutS (unsigned char* data_ptr, AddressType addr, size_t size, idx_t initiatorId, sc_time& delay, sc_time timestamp){
      if (DataSupport) return accessCache<PutS>(data_ptr, size, addr, initiatorId, delay, timestamp, nullptr);
      else             return accessCache<PutS>(NULL, size, addr, initiatorId, delay, timestamp, nullptr);
    }
    virtual inline tlm::tlm_response_status AccessPutM (unsigned char* data_ptr, AddressType addr, size_t size, idx_t initiatorId, sc_time& delay, sc_time timestamp){
      if (DataSupport) return accessCache<PutM>(data_ptr, size, addr, initiatorId, delay, timestamp, nullptr);
      else             return accessCache<PutM>(NULL, size, addr, initiatorId, delay, timestamp, nullptr);
    }

    virtual inline tlm::tlm_response_status AccessPutI (unsigned char* data_ptr, AddressType addr, size_t size, idx_t initiatorId, sc_time& delay, sc_time timestamp){
      if (DataSupport) return accessCache<PutI>(data_ptr, size, addr, initiatorId, delay, timestamp, nullptr);
      else             return accessCache<PutI>(NULL, size, addr, initiatorId, delay, timestamp, nullptr);
    }

    virtual inline tlm::tlm_response_status AccessInvS (unsigned char* data_ptr, AddressType addr, size_t size, idx_t initiatorId, sc_time& delay, sc_time timestamp){
      if (DataSupport) return accessCache<InvS>(data_ptr, size, addr, initiatorId, delay, timestamp, nullptr);
      else             return accessCache<InvS>(NULL, size, addr, initiatorId, delay, timestamp, nullptr);
    }

    virtual inline tlm::tlm_response_status AccessInvM (unsigned char* data_ptr, AddressType addr, size_t size, idx_t initiatorId, sc_time& delay, sc_time timestamp){
      if (DataSupport) return accessCache<InvM>(data_ptr, size, addr, initiatorId, delay, timestamp, nullptr);
      else             return accessCache<InvM>(NULL, size, addr, initiatorId, delay, timestamp, nullptr);
    }
    virtual inline tlm::tlm_response_status AccessReadBack (unsigned char* data_ptr, AddressType addr, size_t size, idx_t initiatorId, sc_time& delay, sc_time timestamp){
      if (DataSupport) return accessCache<ReadBack>(data_ptr, size, addr, initiatorId, delay, timestamp, nullptr);
      else             return accessCache<ReadBack>(NULL, size, addr, initiatorId, delay, timestamp, nullptr);
    }

    /* TODO: Fix functions below */
    //!
    //! Performs a flushing of all cache lines between address @param Begin and address @param End
    //! depending on the cache policies, flushing may forward read or write access to next cache level
    //! FIXME on the other hand the flush is not forwarded to any next-level of cache
    //!
    void Flush(AddressType Begin, AddressType End) {
      assert(End > Begin);
      for(uint64_t i = 0; i < NbSets; i++){
	for(unsigned j = 0; j < Associativity; j++){
	  AddressType BeginLineAddress = CacheLines[i].Lines[j].line.Address;
	  AddressType EndLineAddress = BeginLineAddress + (this -> CacheLinesize) - 1;
	  if ((BeginLineAddress    >= Begin && BeginLineAddress <= End)
	      || (EndLineAddress   >= Begin && EndLineAddress   <= End)
	      || (BeginLineAddress <= Begin && Begin            <= EndLineAddress))
	    {
	      if (CacheLines[i].Lines[j].line.Valid
		  && CacheLines[i].Lines[j].line.Modified
		  && WritePolicy == WBack)
		ForwardWrite (BeginLineAddress, CacheLineSize, sc_time (0, SC_NS));
	      CacheLines[i].Lines[j].line.Valid=false;
	    }
	}
      }
    }

  };





}//end vpsim namespace

#endif //CACHEBASE_HPP

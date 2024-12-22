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

#ifndef COHERENCEEXTENSION_HPP_
#define COHERENCEEXTENSION_HPP_

namespace vpsim {

  //idx_t is the type to use for index or ids
  typedef uint32_t idx_t;
  #define NULL_IDX ((idx_t) 0xffffffff)

  enum CoherenceCommand { GetS, GetM, FwdGetS, FwdGetM, PutS, PutM, PutI, Read, Write, Evict, Invalidate, InvS, InvM, ReadBack };

  struct CoherencePayloadExtension : public tlm::tlm_extension<CoherencePayloadExtension> {

    idx_t initiatorId ;
    idx_t requesterId = NULL_IDX;
    set<idx_t> targetIds = set<idx_t>();
    CoherenceCommand Command;
    bool ToHome = false; // Used in non-coherent mode, to determine the target of RD/WR commands
    /* bool defaultTargetEnabled = false;
       const int defaultTarget = -1; */

    tlm::tlm_extension_base* clone() const {
      //CoherencePayloadExtension* copy = new CoherencePayloadExtension;
      //*copy=*this;
      throw runtime_error("method clone not supported\n");
    }
    void copy_from(tlm::tlm_extension_base const &ext) {
      //*this=dynamic_cast<CoherencePayloadExtension const &>(ext);
      throw runtime_error("method copy_from not supported\n");
    }

    inline void setInitiatorId  (const idx_t id)  { initiatorId = id; }
    inline idx_t getInitiatorId ()                { return initiatorId; }

    inline void setRequesterId  (const idx_t id)  { requesterId = id; }
    inline idx_t getRequesterId ()                { return requesterId; }

    inline void setTargetIds    (set<idx_t> ids)  { targetIds = ids; }
    inline set<idx_t> getTargetIds()              { return targetIds; }

    /*
      void setSharerIds (vector<int>& sharerIds){ targetIds = sharerIds; }
      vector<int>& getTargetIds() { return targetIds; }*/
    /* inline int getDefaultTarget (){ return defaultTarget; } */
    /*
      void disableDefaultTarget (){ defaultTargetEnabled = false; }
      void enableDefaultTarget (){ defaultTargetEnabled = true; }
      bool isDefaultTarget (){ return defaultTargetEnabled; }
    */
    inline void setCoherenceCommand (CoherenceCommand command) { Command = command; }
    inline CoherenceCommand getCoherenceCommand(){ return Command; }

    inline void setToHome (bool b) { ToHome = b; }
    inline bool getToHome (){ return ToHome; }

  };
}
#endif /* COHERENCEEXTENSION_HPP_ */

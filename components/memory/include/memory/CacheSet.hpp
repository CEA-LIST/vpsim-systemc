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

#ifndef CACHESET_HPP
#define CACHESET_HPP

using namespace std;

namespace vpsim{

  //fwd declaration
  //!
  //! All available replacement policies for caches
  //!
  enum  CacheReplacementPolicy {
    FIFO, //!< first-in-first-out replacement policy
    LRU,  //!< Least recently used replacement policy
    MRU   // most recently used
  };


  template<typename CacheLineType, typename AddrType> class CacheSet;
  //! TODO : update comment
  //!
  //! The CacheLine represents the data stored in a line inside a cache.
  //! It includes its Data as a char[] and all the status bits necessary
  //! for the correct evolution of the cache state (i.e) its validity
  //! its age or the corresponding address.
  //!
  //! The class is templated to support different Address data type
  //! as well as variable line lenght in bytes. Hence it supports template:
  //! @tparam LineSize unsigned (the size of the cache line in bytes)
  //! @tparam AddressType typename (the type of the address e.g.:uint_64t, uint_32t, void*...)
  //!

  template<typename CacheLineType, typename AddrType>
  class CacheSet {

  private :

    struct _LineWrapper {
      CacheLineType line;
      unsigned repl_data;
    };
    unsigned Associativity;
    CacheReplacementPolicy Policy;
    _LineWrapper* Lines;
    int NextVictim;
    unsigned CountUntilRepl;

  public :

    CacheSet (unsigned lineSize, uint64_t assoc, CacheReplacementPolicy pol/*, unsigned higherCacheNb*/)
      : Associativity  (assoc)
      , Policy         (pol)
      , NextVictim     (0)
      , CountUntilRepl (0)
    {
      initSet (lineSize/*, higherCacheNb*/);
     }

    CacheSet ()
      : Associativity  (0) //TODO
      , Policy         (LRU)
      , NextVictim     (0)
      , CountUntilRepl (0)
    {}
    // setters to be used with the default constructor
    void setAssociativity (uint64_t assoc) {
      Associativity = assoc;
    }
    void setPolicy (CacheReplacementPolicy pol) {
      Policy = pol;
    }

    ~CacheSet() {}

    void printSet () {
      cout.clear();
      for (unsigned i = 0; i < Associativity; i++) {
        Lines[i].line.printLine();
        cout << " || ReplData: " << Lines[i].repl_data << endl;
      }
    }
    void printCountAcess () {
      cout << ", CountUntilRepl: " << CountUntilRepl << flush;
    }
    void printReplacementData () {
      for (unsigned i = 0; i < Associativity; i++)
        cout  << "Line [" << i << "] -> " << Lines[i].repl_data << endl;
    }
    inline void incrementCountUntilRepl (){
      // TODO: verify wether it is okay for MRU
      if (CountUntilRepl < Associativity){
        for (unsigned i = 0; i < Associativity; i++)
          if (Lines[i].line.getTag() == 0){
            CountUntilRepl++;
            return;
          }
      }
    }
    inline bool accessSet (unsigned line_tag, CacheLineType** linePtrAddr) {
      int  lineIndex = locateLineInSet (line_tag);
      bool lineInCache  = (-1 < lineIndex) && ((unsigned) lineIndex < Associativity);
      switch (Policy) {
      case LRU:
        if (lineInCache){ // hit
          *linePtrAddr = &Lines[lineIndex].line;
          updateReplacementData (lineIndex);
        } else { // miss
          *linePtrAddr = &Lines[NextVictim].line;
          updateReplacementData (NextVictim);
          //electNextVictim (); merged with updateReplacementData to enhance performance
        };
        break;
      case MRU: // TODO: Not tested, perhaps a bit more complicated
        if (lineInCache){ // hit
          *linePtrAddr = &Lines[lineIndex].line;
          updateReplacementData (lineIndex);
          //electNextVictim ();
        } else { // miss
          *linePtrAddr = &Lines[NextVictim].line;
          updateReplacementData (NextVictim);
          //electNextVictim ();
          // todo : what to do with cacheline flag and data
          // updateCacheLine();
        };
        break;
      case FIFO: // TODO
        break;
      }
      return lineInCache;
    }

  private :

    void initSet (unsigned lineSize/*, unsigned higherCacheNb*/) {
      //Lines.resize(Associativity);
      Lines = new _LineWrapper [lineSize];
      switch (Policy) {
      case LRU:
        for (unsigned i = 0; i < Associativity; i++) {
          Lines[i].line = CacheLine<AddrType>(lineSize/*, higherCacheNb*/);
          Lines[i].repl_data = Associativity - i - 1;
        }
        break;
      case MRU:
        for (unsigned i = 0; i < Associativity; i++) {
          Lines[i].line = CacheLine<AddrType>(lineSize/*, higherCacheNb*/);
          Lines[i].repl_data = Associativity - i - 1;
        }
        break;
      case FIFO:
        for (unsigned i = 0; i < Associativity; i++) {
          Lines[i].line = CacheLine<AddrType>(lineSize/*, higherCacheNb*/);
          Lines[i].repl_data = 0;
          }
          break;
      }
    }

    void updateReplacementData (int line_id) {
      // update replacement data and elect next victim
      // update attribute next_victim
      switch (Policy) {
      case LRU:
        for (unsigned i = 0; i < Associativity; i++){
           if (Lines[i].repl_data < Lines[line_id].repl_data) Lines[i].repl_data++;
          if (Lines[i].repl_data == Associativity - 1) NextVictim = i;
        }
        Lines[line_id].repl_data = 0;
        break;
      case MRU: // TODO: check
        assert (CountUntilRepl < Associativity);
        for (unsigned i = 0; i < Associativity; i++){
          if (Lines[i].repl_data > Lines[line_id].repl_data) Lines[i].repl_data--;
          if (Lines[i].repl_data == Associativity-1 && CountUntilRepl >= Associativity-1) NextVictim = i;
        }
        if (CountUntilRepl < Associativity-1) { // Still free space on cache
          NextVictim++;
          CountUntilRepl++;
        }
        Lines[line_id].repl_data = Associativity - 1;
        break;
      case FIFO:
        NextVictim = (NextVictim + 1) % Associativity;
        break;
      }
    }
    void electNextVictim () { // Merged with updateReplacementData to enhance performance
      // update NextVictim and CountUntilRepl
      switch (Policy) {
      case LRU:
        for (unsigned i = 0; i < Associativity; i++)
          if (Lines[i].repl_data == Associativity - 1) NextVictim = i;
        break;
      case MRU:
        assert (CountUntilRepl < Associativity);
        if (CountUntilRepl < Associativity - 1) { // Still free space on cache
          NextVictim++;
         CountUntilRepl++;
        } else { // No free space on cache, old data should be replaced
          for (unsigned i = 0; i < Associativity; i++)
            if (Lines[i].repl_data ==  Associativity - 1) NextVictim = i;
        }
        break;
      case FIFO: // TODO: verify
        NextVictim = (NextVictim + 1) % Associativity;
        break;
      }
    }
    int locateLineInSet (unsigned tag) {
      for (unsigned i = 0; i < Associativity; i++)
        if (Lines[i].line.getTag() == tag && Lines[i].line.getState() != Invalid){
          return i;
        }
      return -1;
    }
  };
}

#endif //CACHESET_HPP

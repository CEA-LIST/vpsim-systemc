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

#ifndef CACHELINE_HPP
#define CACHELINE_HPP


using namespace std;

namespace vpsim{


  enum CoherenceState {
    Modified, //!< If directory, exactly one higher-level cache has the line in Modified state
              //!< If not, current cache has the most recent data
    Shared,   //!< If directory, one or several higher-level caches have the line in Shared state
              //!< If not, current cache has the most recent data
    Invalid   //!< If directory, all higher-level caches have the line in Invalid state
              //!< If not, directory cache has the most recent data
  };


  //fwd declaration
  template<typename AddressType> class CacheLine;
  template<typename AddressType> std::ostream& operator<<(std::ostream&, const CacheLine<AddressType>&);

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
  template <typename AddressType>
  class CacheLine{

  private :

    AddressType    Address;        //!< Address of the line in cache (base address aligned on Line size)
    unsigned       LineSize;       //!< the value of the data in the line
    unsigned       Tag;            //!< most significant bits of the address, checked against all rows in the current set
    unsigned char* Data;
    //bool           Valid;          //!< true if line is valid, ie if contains up to date data
    //bool           Dirty;       //!< true if the line was written to
    //unsigned HigherCacheNb = 0;
    //vector<int> SharerIds;
    CoherenceState State = Invalid;
    int OwnerId;
    CoherenceState HigherState = Invalid;

  public :

    // TODO: make private with setter and getter
    void* handle; // Add ability to notify a line eviction event.

    /******* Constructors and destructors *******/
    //!
    //! CacheLine default constructor
    //! Builds an empty line (invalid)
    //!
     CacheLine<AddressType>()
      : Address  (0)
      , LineSize (0)
      , Tag      (0)
      , State (Invalid)
        //, Data     (NULL)
        //, Valid    (false)
        //, Dirty (false)
    {};

    CacheLine<AddressType>(unsigned lineSize/*, unsigned higherCacheNb*/)
      : Address  (0)
      , LineSize (lineSize)
      , Tag      (0)
      , Data     (NULL)
      //, Valid    (false)
      //, Dirty (false)
      //, HigherCacheNb (higherCacheNb)
      , State (Invalid)
    {
      //Data = new unsigned char [LineSize];
      //SharerIds.resize(higherCacheNb, -1);
    };
    //!
    //! CacheLine copy constructor
    //! deep copy of the source line
    //! @param OtherLine another cacheline reference with the same template parameters
    //!
    CacheLine< AddressType>(const CacheLine< AddressType> & OtherLine) {};
    //!
    //! CacheLine destructor
    //!
    ~CacheLine<AddressType>()
    {
      //delete [] Data;
    };

    /******* Setters *******/
    inline void setSize (const unsigned sz) {
      LineSize = sz;
    }
    /*inline void setValid (const bool value) {
      Valid = value;
    }
    inline void setDirty (const bool value) {
      Dirty = value;
      }*/
    inline void setAddress (const AddressType value) {
      Address = value;
    }
    inline void setTag (const unsigned value) {
      Tag = value;
    }
    inline void setState (const CoherenceState state) {
      State = state;
    }
    inline void setOwner (const int owner) {
      OwnerId = owner;
    }
    /*inline void setNewLine (const AddressType address, const unsigned tag, const bool valid) {
      Address  = address;
      Tag      = tag;
      Valid    = valid;
      SharerIds.assign (HigherCacheNb, -1);
    }*/
    inline void setNewLine (const AddressType address, const unsigned tag) {
      Address  = address;
      Tag      = tag;
      State    = Invalid;
      //SharerIds.assign (HigherCacheNb, -1);
    }
    inline void setHigherState (CoherenceState s) {
      HigherState = s;
    }

    /*inline void addSharer (int id) {
      assert (SharerIds.size() != 0);
      for (unsigned i = 0; i<SharerIds.size(); i++) if (SharerIds[i] == id) return;
      for (unsigned i = 0; i<SharerIds.size(); i++) if (SharerIds[i] == -1) { SharerIds[i] = id; return; }
      throw runtime_error("Cannot add new sharer, maximum number of sharers has been reached \n");
      }*/

    /******** Getters *******/
    /*inline bool getValid () const {
      return Valid;
    }
    inline bool getDirty () const {
      return Dirty;
      }*/
    inline AddressType getAddress () const {
      return Address;
    }
    inline unsigned getTag () const {
      return Tag;
    }
    inline CoherenceState getState () const {
      return State;
    }
    inline int getOwner () const {
      return OwnerId;
    }
    inline unsigned char* getDataPtr () {
      return Data;
    }
    inline void printData () {
      cout << " | " << "Line index = " << hex << Address << dec;
      for (size_t i=0; i< LineSize; i++) cout << (unsigned) Data[i];
    }
    inline unsigned getSize () {
      return LineSize;
    }
    inline CoherenceState getHigherState () {
      return HigherState;
    }

    /*inline vector<int>& getSharerIds () {
      return SharerIds;
      }*/

    /*inline void printLine () {
      //cout << " | " << "index = " << hex << Address << dec;
      cout << " | " << "tag = "   << Tag;
      cout << " | " << "addr = "  << Address;
      cout << " | " << "valid = " << Valid;
      cout << " | " << "Dirty = " << Dirty;
      //cout << " | " << "Data  = [";
      //for (unsigned i=0; i< LineSize; i++) cout << (unsigned) Data[i];
      //cout << "]" << endl;
      }*/
    //!
    //! ostream operator is friend on the CacheLine to support CacheLine cout
    //!
    friend ostream & operator<< <>(ostream & os, const CacheLine</*LineSize,*/ AddressType>& l);

  };


  //!
  //! ostream operation for pretty printing of cache line state
  //! @param os : an output stream where the CacheLine information are going to be printed to
  //! @param l : a CacheLine to print to os
  //! @tparam LineSize unsigned (the size of the cache line in bytes)
  //! @tparam AddressType typename (the type of the address e.g.:uint_64t, uint_32t, void*...)
  //!
    template <typename AddressType>
    std::ostream& operator<<(std::ostream& os, const CacheLine</*LineSize,*/ AddressType>& l)
    {
      os << "CacheLine " << hex << l.Address << dec;
      os << " valid = "  << l.Valid;
      os << " Dirty = " << l.Dirty;
      /* os << " Data  = [";
         for(unsigned int i=0; i<LineSize; i++) os << (unsigned) l.Data[i]<< " ";
         os << "]"; */
      return os;
    }
  }


#endif //CACHELINE_HPP

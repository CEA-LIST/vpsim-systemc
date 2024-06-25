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

#ifndef _CALLBACKREG_HPP
#define _CALLBACKREG_HPP

#include <functional>
#include <map>
#include <systemc>
#include <tlm>
#include <logger/log.hpp>
#include <core/TargetIf.hpp>
#include <core/TlmCallbackPrivate.hpp>

namespace vpsim{

    template<class T>
    class CallbackRegister {
        using callback_t = std::function<void()>;

        T mReg;
        std::multimap<T, callback_t> mCallbacks;

        uint64_t mReadAccesses;
        uint64_t mWriteAccesses;

    public:
        std::map<std::string, callback_t> callbacks = {
                {"stop_simulation", []{sc_core::sc_stop();}}
        };

    public:
        uint64_t getNbReads() const {
            return mReadAccesses;
        }

        uint64_t getNbWrites() const {
            return mWriteAccesses;
        }

        T read(){
            mReadAccesses++;
            return mReg;
        }

        void write(const T& v){
            mWriteAccesses++;
            mReg = v;

            for(auto& cb: mCallbacks){
                auto& val = cb.first;
                auto& callback = cb.second;
                if(v == val) callback();
            }
        }

        void registerCallback(const T& val, const std::string& callback){
            mCallbacks.insert({val, callbacks[callback]});
        }
    };

    template<class T>
    class TLMCallbackRegister:
            public CallbackRegister<T>,
            public sc_core::sc_module,
            public TargetIf<T> {

        tlm::tlm_response_status read (payload_t & payload, sc_time & delay){
            LOG_DEBUG(dbg1) << "Read access to " << this->mName << endl;
            LOG_DEBUG(dbg2) << "\tAt address: 0x" << hex << payload.addr
                            << " (length: 0x" << payload.len << dec << ")"<< endl;

            if(payload.addr != this->getBaseAddress()) {
                LOG_ERROR << "Trying to read at an illegal address in TLMCallbackRegister " << this->mName << ": "
                          << hex << payload.addr << dec << endl;
                return tlm::TLM_ADDRESS_ERROR_RESPONSE;
            }

            if(payload.len != sizeof(T)){
                LOG_ERROR << "Trying to read an illegal length in TLMCallbackRegister " << this->mName << ": "
                          << hex << payload.len << dec << endl;
                return tlm::TLM_ADDRESS_ERROR_RESPONSE;
            }

            *reinterpret_cast<T*>(payload.ptr) = CallbackRegister<T>::read();

            return tlm::TLM_OK_RESPONSE;
        }


        tlm::tlm_response_status write (payload_t & payload, sc_time & delay){
            if(payload.addr != this->getBaseAddress() || payload.len != sizeof(T)){
                LOG_ERROR << "Trying to write at an address not allowed by " << this->mName << ": "
                          << hex << payload.addr << dec << endl;
                return tlm::TLM_ADDRESS_ERROR_RESPONSE;
            }

            auto val = *reinterpret_cast<T*>(payload.ptr);

            LOG_DEBUG(dbg1) << "Write access to " << this->mName << endl;
            LOG_DEBUG(dbg2) << "\tAt address: 0x" << hex << payload.addr
                            << " (length: 0x" << payload.len << dec << ")" << endl;
            LOG_DEBUG(dbg3) << "\tvalue: 0x" << hex << val << dec << ")" << endl;

            CallbackRegister<T>::write(val);

            return tlm::TLM_OK_RESPONSE;
        }

    public:
        explicit TLMCallbackRegister(sc_module_name name):
                sc_module(name),
                TargetIf<T>(string(name), sizeof(T)){
            TargetIf <T>::RegisterReadAccess(REGISTER(TLMCallbackRegister,read));
            TargetIf <T>::RegisterWriteAccess(REGISTER(TLMCallbackRegister,write));

        }
        SC_HAS_PROCESS ( TLMCallbackRegister );

    };
}


#endif //_CALLBACKREG_HPP

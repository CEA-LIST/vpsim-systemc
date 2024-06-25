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

#include "ForwardSimpleSocket.hpp"

using namespace std;
using namespace sc_core;
using namespace tlm;
using namespace vpsim;

void ForwardSimpleSocket::b_transport(tlm_generic_payload &trans, sc_time &delay)
{
    auto rw = trans.get_command();
    auto start = trans.get_address();
    auto length = trans.get_data_length();
    auto end = start + length - 1;
    auto blockingTLM = mVpsimModule->getBlockingTLMEnabled(mPortNum, start);

    if(!blockingTLM) {
        //Look for dmi data
        for (auto &dmi: mDmiData) {
            if (start >= dmi.get_start_address() &&
                end <= dmi.get_end_address() &&
                ((rw == TLM_READ_COMMAND && dmi.is_read_allowed()) ||
                 (rw == TLM_WRITE_COMMAND && dmi.is_write_allowed()))
               ) {

                //FIXME: It shouldn't be legal to let data_ptr be nullptr
                if (trans.get_data_ptr()) {
                    auto offset = start - dmi.get_start_address();
                    if (rw == tlm::TLM_READ_COMMAND) {
                        memcpy(trans.get_data_ptr(), dmi.get_dmi_ptr() + offset, length);
                        //FIXME: to big : delay += dmi.get_read_latency();
                    } else if (rw == tlm::TLM_WRITE_COMMAND) {
                        memcpy(dmi.get_dmi_ptr() + offset, trans.get_data_ptr(), length);
                        //FIXME: to big : delay += dmi.get_write_latency();
                    }
                }
                trans.set_response_status(TLM_OK_RESPONSE);
                return;
            }
        }
    }

    //blocking TLM access otherwise
    mSocketOut->b_transport(trans, delay);

    if(trans.is_dmi_allowed() && !blockingTLM) {
        mTrans.deep_copy_from(trans);
        if (get_direct_mem_ptr(mTrans, mDmiTrans)) {
            mDmiData.push_back(mDmiTrans);
        }
    }

    trans.set_dmi_allowed(trans.is_dmi_allowed() && !blockingTLM);
}

bool ForwardSimpleSocket::get_direct_mem_ptr(tlm_generic_payload &trans, tlm_dmi &dmi_data)
{
    const auto start = trans.get_address();
    const auto end = start + trans.get_data_length() - 1;

    if(mVpsimModule->getBlockingTLMEnabled(mPortNum, AddrSpace(start, end))){
        dmi_data.set_granted_access(tlm_dmi::DMI_ACCESS_NONE);
        return false;
    }

    return mSocketOut->get_direct_mem_ptr(trans, dmi_data);
}

unsigned int ForwardSimpleSocket::transport_dbg(tlm_generic_payload &trans)
{
    return mSocketOut->transport_dbg(trans);
}

tlm_sync_enum ForwardSimpleSocket::nb_transport_fw(tlm_generic_payload &trans, tlm_phase &phase,
                                                   sc_time &t)
{
    return mSocketOut->nb_transport_fw(trans, phase, t);
}

tlm_sync_enum ForwardSimpleSocket::nb_transport_bw(tlm_generic_payload &trans, tlm_phase &phase,
                                                               sc_time &t)
{
    return mSocketIn->nb_transport_bw(trans, phase, t);
}

void ForwardSimpleSocket::invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end)
{
    mDmiData.erase(remove_if(mDmiData.begin(), mDmiData.end(),
                             [start, end](const tlm_dmi& dmi) {
                                 return (dmi.get_start_address() <= end) && (dmi.get_end_address() >= start);
                             }),
                   mDmiData.end());

    mSocketIn->invalidate_direct_mem_ptr(start, end);
}

ForwardSimpleSocket::ForwardSimpleSocket(sc_module_name name, const shared_ptr<VpsimModule> &vpsimModule, size_t portNum)
        :
        sc_module(name),
        mSocketIn((string(name) + "_ForwardIn").c_str()),
        mSocketOut((string(name) + "_ForwardOut").c_str()),
        mVpsimModule(vpsimModule),
        mPortNum(portNum)
{
    mSocketIn.register_b_transport(this, &ForwardSimpleSocket::b_transport);
    mSocketIn.register_get_direct_mem_ptr(this, &ForwardSimpleSocket::get_direct_mem_ptr);
    mSocketIn.register_nb_transport_fw(this, &ForwardSimpleSocket::nb_transport_fw);
    mSocketIn.register_transport_dbg(this, &ForwardSimpleSocket::transport_dbg);

    mSocketOut.register_invalidate_direct_mem_ptr(this, &ForwardSimpleSocket::invalidate_direct_mem_ptr);
    mSocketOut.register_nb_transport_bw(this, &ForwardSimpleSocket::nb_transport_bw);

    mVpsimModule->registerUpdateHook([this]{
        std::vector<AddrSpace> toInvalidate;
        for(auto& dd: mDmiData){
            const auto start = dd.get_start_address();
            const auto end = dd.get_end_address();
            if((dd.is_write_allowed() || dd.is_read_allowed()) &&
               mVpsimModule->getBlockingTLMEnabled(mPortNum, AddrSpace(start, end))){
               toInvalidate.emplace_back(start, end);
            }
        }

        for(auto& as: toInvalidate){
            invalidate_direct_mem_ptr(as.getBaseAddress(), as.getEndAddress());
        }
    });
}

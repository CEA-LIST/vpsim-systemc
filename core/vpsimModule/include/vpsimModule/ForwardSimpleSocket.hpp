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

#ifndef _FORWARDSIMPLESOCKET_HPP
#define _FORWARDSIMPLESOCKET_HPP

#define SC_INCLUDE_DYNAMIC_PROCESSES

#include <memory>
#include "systemc"
#include "vpsimModule/vpsimParam.hpp"

#include "tlm.h"
#include "tlm_utils/simple_initiator_socket.h"
#include "tlm_utils/simple_target_socket.h"


namespace vpsim {
    class ForwardSimpleSocket :
            public sc_core::sc_module,
            public tlm::tlm_fw_transport_if<>,
            public tlm::tlm_bw_transport_if<>
    {
        using socketIn_t = tlm_utils::simple_target_socket<ForwardSimpleSocket>;
        using socketOut_t = tlm_utils::simple_initiator_socket<vpsim::ForwardSimpleSocket>;
        socketIn_t mSocketIn;
        socketOut_t mSocketOut;

        std::vector<tlm::tlm_dmi> mDmiData;

        tlm::tlm_generic_payload mTrans;
        tlm::tlm_dmi mDmiTrans;

        std::shared_ptr<VpsimModule> mVpsimModule;
        const size_t mPortNum;

    public:
        ForwardSimpleSocket(sc_module_name name, const shared_ptr<VpsimModule> &vpsimModule, size_t portNum);
        SC_HAS_PROCESS ( ForwardSimpleSocket );

        socketIn_t& socketIn() {return mSocketIn;}
        socketOut_t& socketOut() {return mSocketOut;}

        //---------------------------------------------------
        //TLM 2.0 communication interface
        void b_transport( tlm::tlm_generic_payload& trans, sc_core::sc_time& delay ) override;

        bool get_direct_mem_ptr(tlm::tlm_generic_payload& trans, tlm::tlm_dmi& dmi_data) override;

        unsigned int transport_dbg(tlm::tlm_generic_payload& trans) override;

        tlm::tlm_sync_enum nb_transport_fw(tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t) override;

        tlm::tlm_sync_enum nb_transport_bw ( tlm::tlm_generic_payload& trans, tlm::tlm_phase& phase, sc_core::sc_time& t ) override;

        void invalidate_direct_mem_ptr(sc_dt::uint64 start, sc_dt::uint64 end) override;
    };
}


#endif //_FORWARDSIMPLESOCKET_HPP

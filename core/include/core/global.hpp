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

#ifndef GLOBAL_HPP_
#define GLOBAL_HPP_

#include <systemc>
#include <tlm>
#include "tlm_utils/tlm_quantumkeeper.h"
#include <tlm_utils/simple_target_socket.h>
#include <tlm_utils/simple_initiator_socket.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <set>
#include <string.h>
#include <iostream>
#include <algorithm>
#include <cstring>
#include <inttypes.h>
#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <fcntl.h>
#include <unistd.h>
#include <cerrno>
#include <string>
#include <limits.h>
#include <list>
#include <vector>
#include <sys/resource.h>
#include <semaphore.h>
#include <fstream> //to store to file and not print

#include "TlmCallbackIf.hpp"

using namespace sc_core;
using namespace std;

//!
//! The vpsim namespace is the main VPSim workspace and shall contain all code implemented by CEA.
//! Sub namespace can be implemented for global function storage and differentiation but shall be
//! avoided to reduce code bloat.
//!
namespace vpsim
{

typedef tlm_utils::simple_target_socket<tlm::tlm_fw_transport_if<>> StdTargetSocket ;//!< A TLM target socket compliant with TLM 2.0 standard (typedef to simplify socket type handling)
typedef tlm_utils::simple_initiator_socket<tlm::tlm_bw_transport_if<>> StdInitSocket;//!< A TLM initiator socket compliant with TLM 2.0 standard (typedef to simplify socket type handling)

#define INTERCONNECT_LATENCY	2

typedef uint8_t BYTE;

typedef enum DIAG_LEVEL_ { DBG_L0, DBG_L1, DBG_L2 } DIAG_LEVEL;
typedef enum ACCESS_TYPE_ { READ, WRITE, READ_ATOMIC, WRITE_ATOMIC } ACCESS_TYPE;
enum ARCHI_TYPE { B16, B32, B64 };
enum CACHE_WRITE_POLICY { WRITE_THROUGH, WRITE_AROUND, WRITE_BACK };

extern ofstream StatStream;
extern ofstream DebugStream;
void OpenVpsimStreams(string RefPath);
void CloseVpsimStreams();

} //end namespace vpsim


#endif /* GLOBAL_HPP_ */

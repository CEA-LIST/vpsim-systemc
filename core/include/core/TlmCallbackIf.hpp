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

#ifndef _TLMCALLBACKIF_HPP_
#define _TLMCALLBACKIF_HPP_

#include "Payload.hpp"

namespace vpsim {


class TlmCallbackIf {
public:
  virtual tlm::tlm_response_status operator()( payload_t & payload, sc_core::sc_time & delay )=0;
  virtual ~TlmCallbackIf(){};
};

typedef TlmCallbackIf Callback_t; //TODO do a clean refactoring when valid!

} /* namespace vpsim */

#endif /* _TLMCALLBACKIF_HPP_ */

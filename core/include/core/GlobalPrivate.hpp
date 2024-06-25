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

#ifndef GLOBAL_PRIVATE_HPP_
#define GLOBAL_PRIVATE_HPP_


#include "TlmCallbackPrivate.hpp"

using namespace sc_core;
using namespace std;

//!
//! The vpsim namespace is the main VPSim workspace and shall contain all code implemented by CEA.
//! Sub namespace can be implemented for global function storage and differentiation but shall be
//! avoided to reduce code bloat.
//!
namespace vpsim
{

static inline std::string
get_pwd ()
{
   char the_path[PATH_MAX];
   return ( string( getcwd(the_path, PATH_MAX) ) );
}

} //end namespace vpsim


#endif /* GLOBAL_HPP_ */

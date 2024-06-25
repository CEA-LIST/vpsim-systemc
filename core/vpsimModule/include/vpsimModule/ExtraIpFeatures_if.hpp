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

#ifndef _REGISTERSTRINGPARAMTRIGGER_IF_HPP
#define _REGISTERSTRINGPARAMTRIGGER_IF_HPP

#include <string>
#include "core/AddrSpace.hpp"
#include "moduleParameters.hpp"

namespace vpsim {
    struct ExtraIpFeatures_if {
        virtual void registerStringParamTrigger(
                const string &trigger,
                const string &module,
                const AddrSpace &as,
                const ModuleParameter &param){

            throw std::runtime_error("This Ip doesn't provide registerStringParamTrigger(...)");
        };


        virtual void registerStringParamTrigger(
                const string &trigger,
                const string &module,
                const ModuleParameter &param){
            registerStringParamTrigger(trigger, module, AddrSpace::maxRange, param);
        }

        virtual void registerCallback(uint64_t val, const std::string& callback){
            throw std::runtime_error("This Ip doesn't provide registerCallback(...)");
        }

        virtual ~ExtraIpFeatures_if() = default;
    };
}
#endif //_REGISTERSTRINGPARAMTRIGGER_IF_HPP

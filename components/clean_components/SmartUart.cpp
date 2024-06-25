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

#include <core/TlmCallbackPrivate.hpp>
#include <logger/log.hpp>
#include "components/SmartUart.hpp"
#include <unistd.h>
#include <poll.h>

#include <vpsimModule/VpsimIp.hpp>
#include <core/platform_builder/include/platform_builder/PlatformBuilder.hpp>

using namespace std;
using namespace tlm;

using namespace vpsim;

SmartUart::SmartUart(ostream &output):
        mOutput(output)
{}

void SmartUart::read() {
    ++mReadAccesses;
}

ssize_t (*SystRead)(int fd, void *buf, size_t count)=read;

void SmartUart::write(char c) {
    ++mWriteAccesses;

    if(c != '\0') {
    	//mOutput << c << flush;
        for (auto &t: mTriggers) {
            auto &i = get<0>(t);
            auto &pattern = get<1>(t);
            auto &module = get<0>(get<2>(t));
            auto &addr = get<1>(get<2>(t));
            auto &param = get<2>(get<2>(t));

            if (pattern[i++] == c) {
                if (i == pattern.size()) {
                    ParamManager::get().setParameter(module, addr, *param);
                    //FIXME: Put that somewhere else
                    VpsimIp<InPortType,OutPortType>::pushStatistics();
                    i = 0;
                }
            } else {
                i = 0;
            }
        }
    }
}

void SmartUart::regStringParamTrigger(
        const string &trigger,
        const string &module,
        const AddrSpace &as,
        const ModuleParameter &param) {

    if(trigger.empty()){
        throw runtime_error("The pattern cannot be empty.");
    }

    mTriggers.emplace_back(make_tuple(0,trigger, make_tuple(module, as, param.clone())));
}

uint64_t SmartUart::getNbWrites() const {
    return mWriteAccesses;
}

uint64_t SmartUart::getNbReads() const {
    return mReadAccesses;
}

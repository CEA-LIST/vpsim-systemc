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

#ifndef _SMARTUART_HPP
#define _SMARTUART_HPP


#include <cstdint>
#include <iostream>
#include <core/TargetIf.hpp>
#include "paramManager.hpp"
#include "CommonUartInterface.hpp"

#define THRE 0x20
#define TEMT 0x40

namespace vpsim {
    class SmartUart {

        std::ostream& mOutput;

        uint64_t mWriteAccesses;
    private:

        uint64_t mReadAccesses;
        using ModuleParameter_ptr = std::unique_ptr<ModuleParameter>;

        using ParamDescriptor_t = std::tuple<std::string, AddrSpace, ModuleParameter_ptr>;
        //! Vector containing elements like [pos, pattern, <module, param>]
        //! pos is the number of consecutive matching characters
        std::vector<std::tuple<size_t, std::string, ParamDescriptor_t>> mTriggers;

    public:

        explicit SmartUart(std::ostream& output = std::cout);
        void read();
        void write(char c);
        uint64_t getNbWrites() const;
        uint64_t getNbReads() const;

        void regStringParamTrigger(
                const string &trigger,
                const string &module,
                const AddrSpace &as,
                const ModuleParameter &param);
    };

}


#endif //_SMARTUART_HPP

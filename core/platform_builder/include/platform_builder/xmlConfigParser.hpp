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

#ifndef _XML_CONFIG_PARSER_HPP
#define _XML_CONFIG_PARSER_HPP

#include "platform_builder/PlatformBuilder.hpp"
#include "rapidxml.hpp"
#include "rapidxml_utils.hpp"
#include "rapidxml_print.hpp"
namespace vpsim{

class XmlConfigParser{
        rapidxml::file<> mSource;
        rapidxml::xml_document<> mXml;

        PlatformBuilder mBuilder;

        std::string mSmartUartName;

        std::string mCallbackRegisterType;
        std::string mCallbackRegisterName;
    public:

        explicit XmlConfigParser(const std::string& xmlFile);
        bool read();

        void readFromPythonXml();
        void readVpsim(rapidxml::xml_node<>*);
        void readPlatform(rapidxml::xml_node<>*);
        void readIps(rapidxml::xml_node<>*);
        void readLinks(rapidxml::xml_node<>*);
        void readIpAttributes(rapidxml::xml_node<>*);
        void readLink(rapidxml::xml_node<>*);
        void readSimulation(rapidxml::xml_node<>*);
        void readLogSchedule(rapidxml::xml_node<>*);
        void readBlockingTLMSchedule(rapidxml::xml_node<>*);

        AddrSpace readAddrRange(rapidxml::xml_node<>*);
        void readCallback(rapidxml::xml_node<>*);
        void unsupportedXmlFile();
    };
}

#endif //_XML_CONFIG_PARSER_HPP

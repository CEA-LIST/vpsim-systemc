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


#ifndef PLATFORMBUILDER_HPP_
#define PLATFORMBUILDER_HPP_

#include "VpsimIp.hpp"
#include <vector>
#include <exception>
#include <iostream>

namespace vpsim {

typedef tlm::tlm_target_socket<> InPortType;
typedef tlm::tlm_initiator_socket<> OutPortType;



struct PlatformBuilder {
	PlatformBuilder(std::string platformName="");

	~PlatformBuilder();


	VpsimIp<InPortType, OutPortType>& beginBuild(std::string ipType,
			std::string ipName);

	VpsimIp<InPortType, OutPortType>& endBuild(
			VpsimIp<InPortType, OutPortType>** newIp = nullptr);

	void finalize();

	void setAttribute(std::string attr, std::string value);

	void connect(std::string srcIpName, std::string srcOutPortName,
			  std::string dstIpName, std::string dstInPortName);

	void forwardInPort(std::string childName, std::string childInPortName, std::string portAlias);

	void forwardOutPort(std::string childName, std::string childOutPortName, std::string portAlias);

	static void dumpComponents(std::ostream& stream) ;

private:
	VpsimIp<InPortType, OutPortType>* mCurrentIp;
	std::vector<VpsimIp<InPortType, OutPortType>*> mBuildStack;
	std::vector<std::string> mLocalIps;
	static int Container;
};

}

#endif /* PLATFORMBUILDER_HPP_ */

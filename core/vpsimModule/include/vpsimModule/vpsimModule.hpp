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

#ifndef _VPSIMMODULE_HPP_
#define _VPSIMMODULE_HPP_

#include <vector>
#include <functional>
#include <memory>
#include <set>
#include <systemc>
#include "parameterSet.hpp"
#include "parameterSetModifier.hpp"
#include "paramManager.hpp"

namespace vpsim {

//! @brief Declares the different module types
//! A memory mapped module which has its own address map (e.g. UART)
//! An intermediate module which inherits its address map from its successors (e.g. cache)
//! A dummy module which is the same as an intermediate from the parameters perspective
enum class moduleType {
	memoryMapped,
	intermediate,
	dummy
};

//! @brief class representing a VpsimModule
//! A VpsimModule is responsible for managing the meta information relative to a TLM module
class VpsimModule
{
    friend class ParamManager;

	using vpsimModuleList = std::vector<VpsimModule*>;
	using constVpsimModuleList = std::vector<const VpsimModule*>;

	//! @brief list of the predecessors of the module, i.e. the connected initiators
	vpsimModuleList mPredecessors;

	//! @brief list of the successors of the module on each port, i.e. the connected targets
	vpsimModuleList mSuccessors;

	//! @brief Intrinsec parameters of the module
	//These parameters are independant from the module's neighbours
	ParameterSet mIntrinsecParameters;

	//! @brief Parameters deduced from the parameters of the successors
	//It is used to interact with the targets conencted to the module
	std::vector<ParameterSet> mEffectiveParameters;

	//! @brief Parameters used by the predecessors
	//It is used by the predecessors to compute their own effective parameters
	ParameterSet mExportedParameters;

	//! @brief Modifiers to be applyed to produce mExportedParameters
	std::unique_ptr<ParameterSetModifier> mModifier;

	//! @brief Default address space of the module
	const AddrSpace mOwnAddrSpace;

	//! @brief Type of the module
	//! affects how its intrinsic parameters are used
	moduleType mModuleType;

	//! @brief Number of output ports in the module
	size_t mNbOutPorts;

    //! @brief Handlers called whenever a parameter is updated
    std::vector<std::function<void()>> mUpdateHooks;

private:

	//! @brief calls refreshParameters() on every predecessor
	void refreshPredecessors();

    //! @brief clear the exported parameters of the predecessors
    void clearPredecessors(constVpsimModuleList&& called = constVpsimModuleList());

	//! @brief recalculate the parameters of the module
    //! @param[in] if true, clear all predecessors before proceeding
    void refreshParameters(bool clear = true);

	//! @brief add a new predecessor to the module
	//! @param[in] p the new predecessor
	void addPredecessor(VpsimModule& p);

	//! @brief Generic setter for parameter values
	//! @param[in] as AddrSpace on which the parameter is to be set
	//! @param[in] param Parameter value
	void setParameter(AddrSpace as, const ModuleParameter& p);

	//! @brief Generic setter for parameter values
	//! @param[in] param Parameter value
	void setParameter(const ModuleParameter& p);


public:
	//! @brief name of the VpsimModule
	const std::string mName;

	//! @brief Constructor
	//! @param[in] type type of the module
	//! @param[in] as Maximum address space of the module. Must live longer than the VpsimModule.
	//! @param[in] nbPort number of output ports of the module
	VpsimModule(std::string name, moduleType type, const AddrSpace& as, size_t nbPort);

	//! @brief Constructor
	//! @param[in] type type of the module
	//! @param[in] nbPort number of output ports of the module
	VpsimModule(std::string name, moduleType type, size_t nbPort);

	//! @brief Destructor
	~VpsimModule();

	//! @brief add a new successor to the module
	//! @param[in] s the new successor
	void addSuccessor(VpsimModule& s, size_t port);

	//! @brief add a new paramter set modifier to the module
	//! The modifiers are applyed in the same order they are added
	//! @param[in] m the new modifier
	void addParameterSetModifier(const ParameterSetModifier& modifier);

	//! @brief get the value of the parameter "blocking TLM enabled"
	//! @param[in] addr address where the parameter value is required
	//! @return The parameter value
	BlockingTLMEnabledParameter getBlockingTLMEnabled(uint64_t addr) const;

	//! @brief get the value of the parameter "blocking TLM enabled"
	//! @param[in] addr address where the parameter value is required
	//! @return The parameter value
	BlockingTLMEnabledParameter getBlockingTLMEnabled(size_t port, uint64_t addr) const;

    //! @brief get the value of the parameter "blocking TLM enabled"
    //! @param[in] addr address space where the parameter value is required
    //! @return The parameter value
    BlockingTLMEnabledParameter getBlockingTLMEnabled(size_t port, AddrSpace addr) const;

	//! @brief get the value of the parameter approximate delay.
	//! It is ponderated by the approximate traversal rate of the module
	//! @param[in] addr address where the parameter value is required
	//! @return The parameter value
	ApproximateDelayParameter getApproximateDelay(uint64_t addr) const;

	//! @brief get the value of the parameter approximate delay.
	//! It is ponderated by the approximate traversal rate of the module
	//! @param[in] addr address where the parameter value is required
	//! @return The parameter value
	ApproximateDelayParameter getApproximateDelay(size_t port, uint64_t addr) const;

    //! Register a new handler to call when a parameter is updated
    //! If a handler was registered for the module, it is replacer
    //! \param name     name of the module which own the handler
    //! \param hook  handler to be called upon parameter update
    void registerUpdateHook(std::function<void()> &&hook);

    //! @brief get the allowed spaces without causing loops
    //! Only the initial caller always gets the expected result if calling with an empty argument
    //! @param[in, out] called list of the module where this method has already been called
    std::set<AddrSpace> getAllowedSpaces(constVpsimModuleList &&called = constVpsimModuleList()) const;
};

}

#endif /* _VPSIMMODULE_HPP_ */


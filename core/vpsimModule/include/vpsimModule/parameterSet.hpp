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

#ifndef _PARAMETERSET_HPP_
#define _PARAMETERSET_HPP_

#include <vector>
#include <utility>
#include <functional>
#include <memory>
#include "AddrSpace.hpp"
#include "moduleParameters.hpp"

namespace vpsim {

using paramPair = std::pair<AddrSpace, std::unique_ptr<const ModuleParameter>>;
using paramContainer = std::vector<paramPair>;

//! @brief Gathers the parameters of a module and provides some functions to manipulate them
class ParameterSet
{
	friend class ParameterSetAddressTranslator;

	//! @brief Blocking TLM transaction enabled parameter values
	paramContainer mBlockingTLMEnabledParameter;

	//! @brief Approximate delay of a transaction values
	paramContainer mApproximateDelayParameter;

	//! @brief Approximate traversal rate of transcations values
	paramContainer mApproximateTraversalRateParameter;

private:
	//! @brief Defragment a paramContainer
	//! @param[in,out] pm The paramContainer to be defragmented
	static void defrag(paramContainer& pc);

	//! @brief Generic function to set a parameter in a parameterSet
	//! @param[in,out] pMap map in which the parameter shall be set
	//! @param[in] as AddrSpace on which the parameter is valid
	//! @param[in] param Parameter vlaue
	//! @param[in] pick function which chose which parameter to set in case of a conflict (pick(old, new))
	void setParameterHelper(paramContainer& pc,
						    AddrSpace as,
							const ModuleParameter& param,
							const std::function<std::unique_ptr<ModuleParameter>(
									const ModuleParameter&,const ModuleParameter&)>& pick);

	//! @brief Generic getter for parameter values
	//! @paramt T type of the returned parameter
	//! @param[in] pc parameterContainer from which to pick the parameter value
	//! @param[in] addr Address for which the parameter is asked
	template<class T>
	T getParameterValue(const paramContainer& pc, uint64_t addr) const;

	BlockingTLMEnabledParameter getParameterValForAddrSpace(const paramContainer& pc, AddrSpace addr) const;

public:
	//! @brief Generic setter for parameter values
	//! @param[in] as AddrSpace on which the parameter is to be set
	//! @param[in] param Parameter value
	void setParameter(AddrSpace as, const ModuleParameter& param);

	//! @brief Merge two parameter set together
	//! @param[in] importedParam Other ParameterSet to merge with *this
	//! @return A reference to *this after the merge
	ParameterSet& mergeImportedParam(const ParameterSet& importedParam);

	//! @brief Apply some modification to *this according to an another parameter set before exportation
	//! @param[in] exportedParam Other ParameterSet to add to *this
	//! @return A reference to *this after the addition
	ParameterSet& addExportedParam(const ParameterSet& exportedParam);

	//! @brief After calling trim, no more parameter is specified outside of the restriction set
	//! @param[in] restriction Spaces where this can define parameter values
	void trim(const set<AddrSpace>& restrinction);

	//! @brief Equality comparison operator
	//! @param[in] that The ParameterSet to compare with *this
	//! @return true if *this and that are equal, false otherwise
	bool operator==(const ParameterSet& that) const;

	//! @brief Not equal operator
	//! @param[in] that The ParameterSet to compare with *this
	//! @return true if *this and that are not equal, false otherwise
	bool operator!=(const ParameterSet& that) const {return !(*this == that);};

	//! @brief Get the "blocking TLM enabled" parameter value
	//! @param[in] addr Address where the parameter value is requiered
	//! @return Value of the parameter for the address passed
	BlockingTLMEnabledParameter getBlockingTLMEnabledParameter(uint64_t addr) const;

	BlockingTLMEnabledParameter getBlockingTLMEnabledParameter(AddrSpace addr) const;


	//! @brief Get the approximate delay parameter parameter value
	//! @param[in] addr Address where the parameter value is requiered
	//! @return Value of the parameter for the address passed
	ApproximateDelayParameter getApproximateDelayParameter(uint64_t addr) const;

	//! @brief Default constructor
	ParameterSet();

    ParameterSet(const ParameterSet& that);

    ParameterSet& operator=(const ParameterSet& that);
};

}



#endif /* _PARAMETERSET_HPP_ */

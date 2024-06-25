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

#ifndef _PARAMETERSETMODIFIER_HPP_
#define _PARAMETERSETMODIFIER_HPP_

#include <memory>
#include <set>
#include "AddrSpace.hpp"
#include "parameterSet.hpp"
#include "GlobalPrivate.hpp"

namespace vpsim {

//! @brief Base class for classes which modifies a parameterSet when applied to a ParameterSet
//! ParameterSetModifier is the neutral modifier.
//! It does nothing to the ParameterSet to which it is applied.
class ParameterSetModifier
{
	//! @brief Forms a list of ParameterSetModifiers for multiple transformations
	std::shared_ptr<ParameterSetModifier> mPreviousModifier;

	//! @brief Apply the modification to a ParameterSet without calling the previous modifiers
	//! @param[in, out] ps ParameterSet to which the modification is applied
	//! @return The modified ParameterSet, same as ps
	virtual ParameterSet& applyNonRecursive(ParameterSet& ps) const;

	//! @brief Apply the modification to a set of AddrSpaces without calling the previous modifiers
	//! @param[in, out] asSet AddrSpace set to which the modification is applied
	//! @return The modified set of AddrSpaces, same as asSet
	virtual std::set<AddrSpace> applyNonRecursive(const std::set<AddrSpace>& asSet) const;

public:

	//! @brief Apply recursively, with previous modifier first, the modifiers to a ParameterSet
	//! @param[in, out] ps ParameterSet to which the modification is applied
	//! @return The modified ParameterSet, same as ps
	virtual ParameterSet& apply(ParameterSet& ps) const;

	//! @brief Apply recursively, with previous modifier first, the modifiers to a set of AddrSpaces
	//! @param[in, out] asSet AddrSpace set to which the modification is applied
	//! @return The modified set of AddrSpaces, same as asSet
	virtual std::set<AddrSpace> apply(const std::set<AddrSpace>& asSet) const;

	//! @brief Operator () overload for convenient application of the modification to a ParameterSet
	ParameterSet& operator()(ParameterSet& ps) const {return apply(ps);}

	//! @brief Operator () overload for convenient application of the modification to a set of AddrSpaces
	std::set<AddrSpace> operator()(const std::set<AddrSpace>& asSet) const {return apply(asSet);}

	//! @brief Attach a new ParameterSetModifier to the list
	//! @param[in] The ParameterSetModifier to be attached to the list
	//! @return A erference on *this
	ParameterSetModifier& attach(const ParameterSetModifier& ps);

	//! @brief "virtual copy constructor" replacement
	//! Replaces the expected behaviour of a "virtual copy constructor"
	//! @return Smart pointer to the new object
	virtual std::unique_ptr<ParameterSetModifier> clone() const;

	virtual ~ParameterSetModifier();
};

//! @brief Translates every adress in a ParameterSet or in a set of addresses
//! A positive translation of value 0x10 set on a VpsimModule makes the
//! access to 0x100 on the input of the module an access to 0x110 on the output of the module
class ParameterSetAddressTranslator: public ParameterSetModifier
{
	//! @brief Size of the offset
	uint64_t mOffset;

	//! @brief Sign of the offset
	bool mPositiveOffset;

	//! @brief See ParameterSetMOdifier::applyNonRecursive
	ParameterSet& applyNonRecursive(ParameterSet& ps) const override;

	//! @brief See ParameterSetMOdifier::applyNonRecursive
	std::set<AddrSpace> applyNonRecursive(const std::set<AddrSpace>& asSet) const override;

public:

	//! @brief Constructor for a single modifier
	//! @param[in] offset Size of the offset
	//! @param[in] positive Set to true for a positive offset and to false for a negative offset
	ParameterSetAddressTranslator (uint64_t offset = 0, bool positive = true);

	//! @brief Constructor for a modifier with other modifiers before it
	//! @param[in] offset Size of the offset
	//! @param[in] positive Set to true for a positive offset and to false for a negative offset
	//! @param[in] previous Modifier to be applied before this one
	ParameterSetAddressTranslator (uint64_t offset, bool positive, const ParameterSetModifier& previous);

	//! @brief See ParameterSetModifier::clone()
	//! Up to the caller to perform a cast if needed
	virtual std::unique_ptr<ParameterSetModifier> clone() const override;
};

}



#endif /* _PARAMETERSETMODIFIER_HPP_ */

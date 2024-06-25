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

#ifndef _MODULEPARAMETERS_HPP_
#define _MODULEPARAMETERS_HPP_

#include <memory>
#include <systemc>

namespace vpsim {

//! @brief Abstract class providing the required interface of a parameter class
struct ModuleParameter
{
	//! @brief Strict order comparison operator.
	//! Might not be intuitive depending on the parameter meaning.
	//! @param[in] that parameter to compare with *this
	//! @return true if *this is smaller than that, false otherwise
	virtual bool operator<(const ModuleParameter& that) const = 0;

    //! @brief Addition assignment operator.
    //! Might not be intuitive depending on the parameter meaning.
    //! @param[in] that parameter to add to *this
    //! @return reference to *this after the addition
    virtual ModuleParameter& operator+=(const ModuleParameter& that) = 0;

	//! @brief Addition operator.
	//! Might not be intuitive depending on the parameter meaning.
	//! @param[in] that parameter to add to *this
	//! @return new instance of the parameter obtained after the addition
	virtual std::unique_ptr<ModuleParameter> operator+(const ModuleParameter& that) const = 0;

	//! @brief Equality comparison operator
	//! @param[in] that parameter to compare with *this
	//! @return true if *this is equivalent to that, false otherwise
	virtual bool operator==(const ModuleParameter& that) const = 0;

	//! @brief Difference comparison operator
	//! @param[in] that parameter to compare with *this
	//! @return true if *this different from that, false otherwise
	virtual bool operator!=(const ModuleParameter& that) const final{return !(*this == that);}

	//! @brief "virtual copy constructor" replacement
	//! Replaces the expected behavior of a "virtual copy constructor"
	//! @return smart pointer to the new object
	virtual std::unique_ptr<ModuleParameter> clone() const = 0;

	//! @brief virtual destructor
	virtual ~ModuleParameter();
};


//! @brief class managing the "blocking TLM enabled" parameter
class BlockingTLMEnabledParameter: public ModuleParameter
{
public:
	//! @brief Possible values for the parameter "blocking TLM enabled"
	//Set in ascending order in terms of restrictivity
	enum value{
		BT_DISABLED = 0,
		BT_ENABLED = 1
	};

	//! @brief helper to get a bt_enabled parameter
	static const BlockingTLMEnabledParameter bt_enabled;

	//! @brief helper to get a bt_disabled parameter
	static const BlockingTLMEnabledParameter bt_disabled;

private:
	//! @brief Actual value of the parameter
	BlockingTLMEnabledParameter::value mValue;

	//! @brief Default value to be used for this parameter
	static BlockingTLMEnabledParameter::value mDefaultValue;

public:

	//! @brief Changes the default value
	//! @brief[in] v New default value
	static void setDefault(value v);

	//! @return true if *this is disabled and that is enabled, false otherwise
	bool operator<(const ModuleParameter& that) const override;

	//! @brief See ModuleParameter::operator==
	bool operator==(const ModuleParameter& that) const override;

    //! @brief BlockingTlMEnabledParameter adds up by choosing the max
    ModuleParameter& operator+=(const ModuleParameter& that) override;

    //! @brief BlockingTlMEnabledParameter adds up by choosing the max
	std::unique_ptr<ModuleParameter> operator+(const ModuleParameter& that) const override;

	//! @brief Get the value of the parameter
	//! @return The value of the parameter
	value get() const;

	//! @brief Implicit cast to bool for ease of use.
	operator bool() const;

	//! @brief See ModuleParameter::clone
	std::unique_ptr<ModuleParameter> clone() const override;

	//! @brief default implicit constructor
	//! @param[in] val value to be stored in the object
	BlockingTLMEnabledParameter(value val = mDefaultValue);

	//! @brief implicit conversion constructor from bool
	//! @param[in] b true for BT_ENABLED, false for BT_DISABLED
	BlockingTLMEnabledParameter(bool b);
};


//! @brief class managing the approximate access delay per byte parameter
class ApproximateDelayParameter: public ModuleParameter
{
private:
	//! @brief Actual value of the parameter
	sc_core::sc_time mDelay;

	//! @brief Default value to be used for this parameter
	static sc_core::sc_time mDefaultDelay;

public:

	//! @brief Changes the default delay
	//! @brief[in] v New default delay
	static void setDefault(sc_core::sc_time v);

	//! @return true if the delay of this is smaller than the delay of max
	bool operator<(const ModuleParameter& that) const override;

	//! @brief See ModuleParameter::operator==
	bool operator==(const ModuleParameter& that) const override;

    //! @brief ApproximateDelayParameter adds up by adding the delays
    ModuleParameter& operator+=(const ModuleParameter& that) override;

	//! @brief ApproximateDelayParameter adds up by adding the delays
	std::unique_ptr<ModuleParameter> operator+(const ModuleParameter& that) const override;

	//! @brief Get the value of the parameter
	//! @return The value of the parameter
	sc_core::sc_time get() const;

	//! @brief Implicit convertion to sc_time for convenience. Equivalent to get()
	operator sc_core::sc_time() const;

	//! @brief See ModuleParameter::clone
	std::unique_ptr<ModuleParameter> clone() const override;

	//! @brief default implicit constructor
	//! @param[in] val value to be stored in the object
	ApproximateDelayParameter(sc_core::sc_time delay = mDefaultDelay);
};


//! @brief class managing the approximate traversal rate parameter (mostly for caches)
class ApproximateTraversalRateParameter: public ModuleParameter
{
private:
	//! @brief Actual value of the parameter. mRate == 1 means 100%
	double mRate;

public:

	//! @return true if the traversal rate of *this is GREATER than the one of that
	bool operator<(const ModuleParameter& that) const override;

	//! @brief See ModuleParameter::operator==
	bool operator==(const ModuleParameter& that) const override;

    //! @brief should never be called (throw on call)
    ModuleParameter& operator+=(const ModuleParameter& that) override;

    //! @brief should never be called (throw on call)
	std::unique_ptr<ModuleParameter> operator+(const ModuleParameter& that) const override;

	//! @brief Get the value of the parameter
	//! @return The value of the parameter
	double get() const;

	//! @brief Implicit conversion for convenience
	operator double() const;

	//! @brief See ModuleParameter::clone
	std::unique_ptr<ModuleParameter> clone() const override;

	//! @brief default implicit constructor
	//! @param[in] val value to be stored in the object
	ApproximateTraversalRateParameter(double rate);
};

}

#endif /* _MODULEPARAMETERS_HPP_ */


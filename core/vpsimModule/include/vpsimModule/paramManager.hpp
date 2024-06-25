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

#ifndef _PARAM_MANAGER_HPP_
#define _PARAM_MANAGER_HPP_

#include <memory>
#include <string>
#include <map>
#include "paramScheduler.hpp"


namespace vpsim{

class VpsimModule;

//! @brief Singleton class responsible for managing the parameter system
class ParamManager{
private:

    //! @brief SystemC module responsible for dynamically changing the parameters values during the simulation
    ParamScheduler mParamScheduler;

    //! @brief Map of the registered module
    std::map<std::string, VpsimModule* const> mVpsimModules;

    //! @brief Handlers called whenever a parameter is updated
    std::map<std::string, std::function<void()>> mUpdateHandlers;

public:

    //! @brief Accessor to the unique instance of the class ParamManager
    //! @return Reference to the unique instance of the class ParamManager
    static ParamManager& get();

    //! @brief Call the parameter update handlers
    void callParamUpdateHandlers();

    //! @param[in] module	Name of the module
    //! @param[in] as		Address space where the new parameter is valid
    //! @param[in] param 	New parameter value
    void setParameter(std::string module, AddrSpace as, const ModuleParameter& param);

    //! @brief Set a new parameter value to a module
    //! @param[in] module	Name of the module
    //! @param[in] param 	New parameter value
    void setParameter(std::string module,  const ModuleParameter& param);

    //! @brief Schedule an Appointment to change the value of a parameter during the simulation
    //! @param[in] module   Name of the module
    //! @param[in] as		  Address space where the new parameter is valid
    //! @param[in] date     Date of the Appointment
    //! @param[in] param	  New parameter
    void addAppointment(std::string module,
                        AddrSpace as,
                        sc_core::sc_time date,
                        const ModuleParameter& param);

    //! @brief Schedule an Appointment to change the value of a parameter during the simulation
    //! @param[in] module   Name of the module
    //! @param[in] date     Date of the Appointment
    //! @param[in] param	  New parameter
    void addAppointment(std::string module,
                        sc_core::sc_time date,
                        const ModuleParameter& param);

    //! @brief Register a VpsimModule to access it with its name later
    //! @param[in] module module ot be registered
    void registerModule(VpsimModule& module);

    //! @param[in] name Name of the module to be unregistered
    //! @brief Unregister a VpsimModule
    void unregisterModule(std::string name);

    //! Register a new handler to call when a parameter is updated
    //! If a handler was registered for the module, it is replacer
    //! \param name     name of the module which own the handler
    //! \param handler  handler to be called upon parameter update
    void registerUpdateHook(std::string name, function<void()> &&handler);

    //!@brief Copy constructor deleted to prevent from singleton copy
    ParamManager(const ParamManager&) = delete;

private:
    //! @brief Default constructor made private to prevent from additional instanciations
    ParamManager();
};

}

#endif /* end of include guard: _PARAM_MANAGER_HPP_ */

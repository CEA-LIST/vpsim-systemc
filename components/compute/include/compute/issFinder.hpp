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

#ifndef _ISSFINDER_HPP_
#define _ISSFINDER_HPP_

#include <vector>
#include <string>

#define ISS_LIB_PREFIX "libIssTarget_"
#define ISS_LIB_SUFFIX ".so"
#define DEFAULT_ISS_PATH "./:"

namespace vpsim {

//! @brief Helper class to find suitable iss libraries depending on the architecture
//!
//! The answer returned by this class only relies on files names and directories layout.
//! There is no checking performed regarding the validity of the library.
class IssFinder
{
private:
	std::string mIssBaseDir;
	std::vector<std::string> mIssSubDirs;

public:
	IssFinder() = delete;

	//! @brief sets the list of iss libraries directories
	//! @param[in] issDirs string matching the pattern path/to/base/dir:iss1,iss2,...
	IssFinder(std::string issDirs);

	//! @brief get the absolute path to the first suitable iss library
	//! @param[in] targetArch name of the targeted architecture
	//! @return empty string if no lib available, path to the lib otherwise
	std::string getIssLibPath(const std::string& targetArch) const;
};

}

#endif /* _ISSFINDER_HPP_ */

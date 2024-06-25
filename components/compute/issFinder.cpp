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

#include <istream>
#include <fstream>
#include "issFinder.hpp"
#include "log.hpp"


using namespace std;
using namespace vpsim;

IssFinder::IssFinder(string issDirs)
{
	//Reminder : the syntax for iss paths is
	//	path/to/base/dir:iss1,iss2,...


	//Get the path before ":"
	size_t columnPos = issDirs.find_last_of(':');
	mIssBaseDir = issDirs.substr(0, columnPos) + '/';

	//Erase up to the ":"
	issDirs.erase(0, columnPos + 1);

	if (columnPos == issDirs.npos || issDirs.empty()){
		throw(invalid_argument("invalid IssType list\n"
				"correct syntax is path/to/iss/base/folder:issType1Folder,issType2Folder"));
	}

	//get the iss names
	while(!issDirs.empty()){
		size_t commaPos = issDirs.find_first_of(',');
		mIssSubDirs.push_back(issDirs.substr(0, commaPos));

		//If it is the last iss so that find_first_of didn't finf ','
		if(commaPos == string::npos)
			issDirs.clear();
		else
			issDirs.erase(0, commaPos + 1);
	}

}

string IssFinder::getIssLibPath(const string& targetArch) const
{
	string libFile{ISS_LIB_PREFIX + targetArch + ISS_LIB_SUFFIX};
	string path;

	for(auto iss:mIssSubDirs){
		path = mIssBaseDir + iss + '/' + libFile;
		if(ifstream(path)) //fast test for existence through ifstream opening and close (upon destruction)
			return path;
	}


	//handling error messages
	cerr<<"failed to find ISS library "<<libFile<<endl;
	if (mIssSubDirs.size()>0){
		cerr<<"within any of the following folders"<<endl;
		for(auto iss:mIssSubDirs){
				path = mIssBaseDir + iss ;
				cerr<<path<<endl;
		}
	}
	else{
		cerr<<"no Iss path specified"<<endl;
	}
	exit(EXIT_FAILURE);
}

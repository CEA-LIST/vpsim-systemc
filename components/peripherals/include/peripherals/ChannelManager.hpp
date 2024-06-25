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

#ifndef _CHANNELMANAGER_HPP_
#define _CHANNELMANAGER_HPP_
#include <map>
#include <string>
#include <vector>
#include <stdint.h>

using namespace std;


class ChannelManager {
private:
	ChannelManager();
	virtual ~ChannelManager();

	std::vector<int> mOpenSocks;


	std::map<int,string> ChanNames;
	std::map<string, int> ChanNumbers;
	int ChanCounter;

	std::map<int,std::pair<int,int>> Channels;

public:
	static ChannelManager& get();
	std::pair<int,int> allocChannel(int channel, bool terminal);
	std::pair<int,int> allocChannel(string channel, bool terminal=false);

	std::pair<int,int> allocOutgoingChannel(int channel, string ip, uint16_t port);
	std::pair<int,int> allocOutgoingChannel(string channel, string ip, uint16_t port);

	static bool fdCheckReady(int fd);

	static ChannelManager singleton;
};


#endif /* _CHANNELMANAGER_HPP_ */

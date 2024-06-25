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

#include <ChannelManager.hpp>
#include <iostream>
#include <poll.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>

#define PORT_BASE 4320

using namespace std;

ChannelManager ChannelManager::singleton;

ChannelManager& ChannelManager::get() {
	return singleton;
}

ChannelManager::ChannelManager() {
	ChanCounter=1;
	Channels.insert(make_pair(0,make_pair(0,1)));
	ChanNumbers["stdio"] = 0;
	ChanNames[0]="stdio";
}

ChannelManager::~ChannelManager() {
	for (auto fd: mOpenSocks) {
		close(fd);
	}
}

std::pair<int,int> ChannelManager::allocChannel(string channel, bool terminal) {
	if (ChanNumbers.find(channel) == ChanNumbers.end()) {
		// Allocate a new channel
		ChanNumbers[channel] = ChanCounter++;
		ChanNames[ChanNumbers[channel]] = channel;

		cout.clear(); cout<<"Creating channel "<<channel<<endl;
	}

	return allocChannel(ChanNumbers[channel], terminal);
}

std::pair<int,int> ChannelManager::allocChannel(int channel, bool terminal) {
	if (Channels.empty())
			Channels.insert(make_pair(0,make_pair(0,1)));
	if (channel > 0) {
		if (Channels.find(channel)==Channels.end()) {
			// open new connection
			cout<<"UART opening channel "<<channel<<"... "<<endl;
			int channel_fd = socket(AF_INET, SOCK_STREAM, 0);
			struct sockaddr_in uart_addr, remote_addr;
			uint32_t remote_len=sizeof(remote_addr);
			uart_addr.sin_family = AF_INET;
			uart_addr.sin_addr.s_addr = INADDR_ANY;
			uart_addr.sin_port = htons(PORT_BASE+channel);

			if (channel_fd<0) {
				throw runtime_error("socket(): Unable to create uart TCP socket.");
			}
			int spin=0;
			while (bind(channel_fd,(struct sockaddr *)&uart_addr,sizeof(uart_addr)) < 0) {
				uart_addr.sin_port = htons(PORT_BASE+channel+spin);
				if (spin>=0) spin++;
				spin = -spin;
			}
			cout<<"UART Channel "<<channel<<" Now listening, please connect to port : "<<ntohs(uart_addr.sin_port)<<endl;
			listen(channel_fd,1);
			// now open a putty
			if (terminal) {
				char puttyCmdPort[1024];
				sprintf(puttyCmdPort, "%d",ntohs(uart_addr.sin_port));
				if (!fork()) {
					sleep(1);
					sprintf(puttyCmdPort, "putty -raw localhost %d -loghost %s",ntohs(uart_addr.sin_port),ChanNames[channel].c_str());
					exit(system(puttyCmdPort));
				}
			}
			int uart_fd = accept(channel_fd, (struct sockaddr*)&remote_addr, &remote_len);
			if (uart_fd < 0) {
				throw runtime_error("UART: Something went wrong, could not accept remote connection.");
			}
			Channels.insert(make_pair(channel,make_pair(uart_fd,uart_fd)));
			mOpenSocks.push_back(uart_fd);
			mOpenSocks.push_back(channel_fd);
		}
	}

	return Channels[channel];
}

std::pair<int,int> ChannelManager::allocOutgoingChannel(int channel, string ip, uint16_t port) {
	if (Channels.empty())
			Channels.insert(make_pair(0,make_pair(0,1)));
	if (channel > 0) {
		if (Channels.find(channel)==Channels.end()) {
			// open new connection
			cout<<"UART opening channel "<<channel<<"... "<<endl;
			int channel_fd = socket(AF_INET, SOCK_STREAM, 0);
			struct sockaddr_in uart_addr;
			uart_addr.sin_family = AF_INET;
			uart_addr.sin_addr.s_addr = inet_addr(ip.c_str());
			uart_addr.sin_port = htons(port);

			if (channel_fd<0) {
				throw runtime_error("socket(): Unable to create TCP socket.");
			}
			if (connect(channel_fd,(sockaddr*)&uart_addr,sizeof(uart_addr)) < 0) {
				throw runtime_error("connect(): Unable to connect TCP socket.");
			}

			cout<<"Outgoing connection established with remote."<<endl;

			Channels.insert(make_pair(channel,make_pair(channel_fd,channel_fd)));
			mOpenSocks.push_back(channel_fd);
		}
	}

	return Channels[channel];
}

std::pair<int,int> ChannelManager::allocOutgoingChannel(string channel, string ip, uint16_t port) {
	if (ChanNumbers.find(channel) == ChanNumbers.end()) {
		// Allocate a new channel
		ChanNumbers[channel] = ChanCounter++;
		ChanNames[ChanNumbers[channel]] = channel;
		cout.clear(); cout<<"Creating channel "<<channel<<endl;
	}

	return allocOutgoingChannel(ChanNumbers[channel],  ip,  port);
}


bool ChannelManager::fdCheckReady(int fd) {
	struct pollfd f;
	int t;
	f.fd=fd;
	f.events = POLLIN|POLLPRI;
	t=poll(&f, 1, 0);
	if(t<0)
		throw runtime_error("poll() failed.");
	else
		return (bool)t;
}


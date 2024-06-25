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

#ifndef _REMOTETRANS_HPP_
#define _REMOTETRANS_HPP_
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

#include <stdint.h>
#include "ChannelManager.hpp"

using namespace std;

#define PREALLOC_SIZE 16

enum RemoteTransactionType {
	REMOTE_READ, REMOTE_WRITE
};

struct RemoteTransaction {
	uint32_t type;
	uint64_t address;
	uint64_t size;
	uint8_t data[PREALLOC_SIZE];
};


struct RemoteInterrupt {
	uint32_t line;
	uint32_t value;
};

enum RemoteReponseType {
	REMOTE_READ_OK, REMOTE_WRITE_OK, REMOTE_READ_ERR, REMOTE_WRITE_ERR
};

struct RemoteResponse {
	uint32_t type;
	uint64_t size;
	uint8_t data[PREALLOC_SIZE];
};


class GenericRemoteInitiator {
public:
	GenericRemoteInitiator() {
		buf=get_buf(sizeof(uint64_t));
		mFree=true;
	}

	virtual ~GenericRemoteInitiator() {
		free(buf);
	}
	void setChannel(string name) { mChannel=ChannelManager::get().allocOutgoingChannel(name, mIp, mPort); }
	void setIrqChannel(string name) { mIrqChannel=ChannelManager::get().allocOutgoingChannel(name, mIrqIp, mIrqPort); }
	void setIp(string ip) { mIp=ip; }
	void setPort(uint16_t port) { mPort=port; }
	void setIrqIp(string ip) { mIrqIp=ip; }
	void setIrqPort(uint16_t port) { mIrqPort=port; }

	void setPollPeriod(uint64_t poll_period) {mPollPeriod=poll_period;}
	//virtual void delay(uint64_t cycles) = 0;

	virtual void interrupt(uint32_t line, uint32_t value) {
		RemoteInterrupt irq;
		irq.line=line;
		irq.value=value;
		if (write(mIrqChannel.second, &irq, sizeof(RemoteInterrupt)) == -1){
			std::cerr << "RemoteTransaction: Error on write"<< std::endl;
			return;
		}
		// cout.clear(); cout<<"remote initiator: transmitted interrupt to remote."<<endl;
	}

	virtual void poll() {
		//while (true) {
			//delay(mPollPeriod);
			while (mFree && ChannelManager::fdCheckReady(mChannel.first)) {
				mFree=false;

				if (read(mChannel.first, &trans, sizeof(RemoteTransaction)) < 0){
					std::cerr << "RemoteTransaction: Error on read"<< std::endl;
					return;
				}

				switch (trans.type) {
				case REMOTE_READ: {
					buf = get_buf(trans.size);
					// resp.type =
					localRead(trans.address, trans.size, buf);
					//resp.size= (resp.type==REMOTE_READ_OK? trans.size: 0);


					break;
				}
				case REMOTE_WRITE: {
					buf = get_buf(trans.size);
					memcpy(buf, trans.data, min(trans.size, (uint64_t)PREALLOC_SIZE));
					if (trans.size > PREALLOC_SIZE)
						if (read(mChannel.first, buf+PREALLOC_SIZE, trans.size-PREALLOC_SIZE)< 0){
							std::cerr << "RemoteTransaction: Error on read"<< std::endl;
							return;
						}
					//resp.type =
					localWrite(trans.address, trans.size, buf);
					//resp.size=0;


					break;
				}
				default:
					throw runtime_error("Only READ and WRITE transactions are currently supported.");
				}

			//}
		}
	}

	virtual uint32_t localRead(uint64_t addr, uint64_t size, uint8_t* data) = 0;
	virtual uint32_t localWrite(uint64_t addr, uint64_t size, uint8_t* data) = 0;

	virtual void completeRead(uint32_t response_type, uint8_t* buf) {
		mFree=true;
		resp.type=response_type;
		resp.size= (resp.type==REMOTE_READ_OK? trans.size: 0);

		if (resp.type==REMOTE_READ_OK)
			memcpy(resp.data, buf, min(resp.size, (uint64_t)PREALLOC_SIZE));

		if (write(mChannel.second, &resp, sizeof(RemoteResponse)) == -1){
			std::cerr << "RemoteTransaction: Error on write"<< std::endl;
			return;
		}

		if (resp.size > PREALLOC_SIZE) {
			if (write(mChannel.second, buf+PREALLOC_SIZE, resp.size-PREALLOC_SIZE) == -1){
				std::cerr << "RemoteTransaction: Error on write"<< std::endl;
				return;
			}
		}
	}

	virtual void completeWrite(uint32_t response_type) {
		mFree=true;
		resp.type = response_type;
		resp.size=0;
		if (write(mChannel.second, &resp, sizeof(RemoteResponse)) == -1){
			std::cerr << "RemoteTransaction: Error on write"<< std::endl;
			return;
		}
	}

protected:

	uint8_t *get_buf(uint32_t size) {
		if (size > mBufSize) {
			mBufSize=size;
			free(buf);
			buf=(uint8_t*)malloc(size);
		}
		return buf;
	}
	uint8_t* buf;
	uint32_t mBufSize;
	uint64_t mPollPeriod;
	std::pair<int, int> mChannel;
	std::pair<int, int> mIrqChannel;
	string mIp, mIrqIp;
	uint16_t mPort, mIrqPort;

	RemoteTransaction trans;
	RemoteResponse resp;

	bool mFree;
};

class GenericRemoteTarget {
public:
	virtual ~GenericRemoteTarget() {}
	void setChannel(string name) { mChannel=ChannelManager::get().allocChannel(name,false); }
	void setIrqChannel(string name) { mIrqChannel=ChannelManager::get().allocChannel(name,false); }
	void setPollPeriod(uint64_t cycles) { mPollPeriod=cycles; }

	//virtual void delay(uint64_t cycles) = 0;
	virtual void interrupt(uint32_t line, uint32_t value) = 0;

	virtual void poll() {
		//while (true) {
			//delay(mPollPeriod);
			if (ChannelManager::fdCheckReady(mIrqChannel.first)) {
				RemoteInterrupt irq;
				if (read(mIrqChannel.first, &irq, sizeof(RemoteInterrupt)) < 0){
					std::cerr << "RemoteTransaction: Error on read"<< std::endl;
					return;
				}
				// cout.clear(); cout<<"remote target: received an interrupt request."<<endl;
				interrupt(irq.line, irq.value);
			}
		//}
	}

	virtual uint32_t remoteWrite(uint64_t address, uint64_t size, uint8_t* data) {
		mRequest.type = REMOTE_WRITE;
		mRequest.address = address;
		mRequest.size = size;


		memcpy(mRequest.data, data, std::min(size,(uint64_t)PREALLOC_SIZE));
		if (write(mChannel.second, &mRequest, sizeof(RemoteTransaction))== -1){
			std::cerr << "RemoteTransaction: Error on write"<< std::endl;
			return -1;
		}
		cout.clear(); cout<<"Submitted write on address "<<hex<<address<<endl;

		if (size > PREALLOC_SIZE) {
			if (write(mChannel.second, data+PREALLOC_SIZE, size-PREALLOC_SIZE)== -1){
				std::cerr << "RemoteTransaction: Error on write"<< std::endl;
				return -1;
			}
		}

		if (read(mChannel.first, &mResponse, sizeof(RemoteResponse)) < 0){
			std::cerr << "RemoteTransaction: Error on read"<< std::endl;
			return -1;
		}
		cout.clear(); cout<<"Write complete "<<hex<<address<<endl;

		return mResponse.type;
	}

	virtual uint32_t remoteRead(uint64_t address, uint64_t size, uint8_t* data) {
		mRequest.type = REMOTE_READ;
		mRequest.address = address;
		mRequest.size = size;

		if (write(mChannel.second, &mRequest, sizeof(RemoteTransaction))== -1){
			std::cerr << "RemoteTransaction: Error on write"<< std::endl;
			return -1;
		}
		cout.clear(); cout<<"Submitted read on address "<<hex<<address<<endl;

		// await response
		if (read(mChannel.first, &mResponse, sizeof(RemoteResponse)) < 0){
			std::cerr << "RemoteTransaction: Error on read"<< std::endl;
			return -1;
		}


		if (mResponse.type == REMOTE_READ_OK) {
			memcpy(data, mResponse.data, min(mResponse.size, (uint64_t)PREALLOC_SIZE));
			if (mResponse.size > PREALLOC_SIZE)
				if (read(mChannel.first, data+PREALLOC_SIZE, mResponse.size-PREALLOC_SIZE) < 0){
					std::cerr << "RemoteTransaction: Error on read"<< std::endl;
					return -1;
				}
		}

		cout.clear(); cout<<"Read complete "<<hex<<address<<" = "<<*(uint64_t*)mResponse.data<<endl;

		return mResponse.type;
	}

protected:
	std::pair<int, int> mChannel, mIrqChannel;
	RemoteTransaction mRequest;
	RemoteResponse mResponse;
	uint64_t mPollPeriod;
};

#endif /*_REMOTETRANS_HPP_*/

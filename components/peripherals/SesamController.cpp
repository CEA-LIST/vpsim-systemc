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

#include <SesamController.hpp>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

const unsigned int MAX_BUF_LENGTH = 4096;

namespace vpsim {

    SesamController::SesamController(sc_module_name name) : sc_module(name), TargetIf(string(name), 0x4) {
        TargetIf <REG_T>::RegisterReadAccess(REGISTER(SesamController, read));
        TargetIf <REG_T>::RegisterWriteAccess(REGISTER(SesamController, write));
    }

    SesamController::~SesamController() {
    }

    tlm::tlm_response_status SesamController::read(payload_t & payload, sc_time & delay) {
        uint8_t data = 0;
        if (!payload.ptr) {
            throw runtime_error("Monitor does not support null payloads !");
        }
        if (payload.addr == getBaseAddress() + 1) {
        	if (mCommandOutputBuffer.length() != 0) {
        		data = mCommandOutputBuffer.c_str()[0];
        		string buf = string(&mCommandOutputBuffer.c_str()[1]);
        		mCommandOutputBuffer = buf;
        	}
        } else {
        	data = 42;
        }

        memcpy(payload.ptr, &data, payload.len);
        return tlm::TLM_OK_RESPONSE;
    }

    tlm::tlm_response_status SesamController::write(payload_t & payload, sc_time & delay) {
        uint8_t *data = new uint8_t;
        if (!payload.ptr) {
            throw runtime_error("Monitor does not support null payloads !");
        }
        memcpy(data, payload.ptr, payload.len);
        if (payload.addr == getBaseAddress()) { // command
            switch(*data) {
            	case 0x20 : {
            		strParam.clear();
            		strParam.push_back("list");
            		*sesamState = TAKE_CMD;
            		sesamCommand(strParam);
            		*sesamState = RUN;
            	}
            	break;
                case 0x42 : { // quit w/o question
                    strParam.clear();
                    strParam.push_back("quit");
                    *sesamState = TAKE_CMD;
                    sesamCommand(strParam);
                    *sesamState = RUN;
                }
                break;
                case 0x52 : { // start benchmark mode
                    string tmp = strParam.back();
                    strParam.clear();
                    strParam.push_back("benchmark");
                    strParam.push_back(tmp);
                    *sesamState = TAKE_CMD;
                    sesamCommand(strParam);
                    *sesamState = RUN;
                }
                break;
                case 0x54 : { // end benchmark mode
                    sesamCommand(strParam);
                }
                break;
                case 0x58 : { // start receiving parameter
                    strParam.clear();
                }
                break;
                case 0x62 : { // start receiving string
                    strBuf = new string;
                }
                break;
                case 0x72 : { // end receiving string and add parameter
                    strParam.push_back(*strBuf);
                    delete strBuf;
                }
                break;
                case 0x78 : { // end receiving parameter and execute command
                    *sesamState = TAKE_CMD;
                    sesamCommand(strParam);
                    *sesamState = RUN;
                }
                break;
                default:
                    throw runtime_error("SesamController in unknown command.");
            }
        } else if (payload.addr == (getBaseAddress() + 1)) { // data
            *strBuf += *data;
        }

        return tlm::TLM_OK_RESPONSE;
    }

}

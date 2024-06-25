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

#ifndef _TLMCALLBACKPRIVATE_HPP_
#define _TLMCALLBACKPRIVATE_HPP_

#include "global.hpp"
#include "TlmCallbackIf.hpp"

namespace vpsim {


template<class MODULE>
class TlmCallbackPrivate : public TlmCallbackIf
{
	MODULE* mModulePtr;
	typedef tlm::tlm_response_status (MODULE::*FuncPtrType)( payload_t &, sc_core::sc_time & );
	FuncPtrType mFuncPtr;
//	void * mPtfunc; //pointer to member function

public:
  TlmCallbackPrivate(MODULE* classPtr,FuncPtrType cbProc);
  virtual ~TlmCallbackPrivate();

  //implementation of Callback_t features
  virtual tlm::tlm_response_status operator() ( payload_t & payload, sc_core::sc_time & delay );
//  void * PtFunc();

};

//---------------------------------------------------------------------------//
// Callback features                                                         //
//---------------------------------------------------------------------------//

template <typename MODULE>
TlmCallbackPrivate<MODULE>::TlmCallbackPrivate(MODULE* classPtr,FuncPtrType cbProc):mModulePtr(classPtr),mFuncPtr(cbProc){
		  //mPtfunc = (void*) (mModulePtr->*mFuncPtr);
}

template <typename MODULE>
TlmCallbackPrivate< MODULE >::~TlmCallbackPrivate(){}

template <typename MODULE>
tlm::tlm_response_status TlmCallbackPrivate< MODULE >::operator() ( payload_t & payload, sc_core::sc_time & delay ){
		return (mModulePtr->*mFuncPtr)( payload, delay );
}

#define  REGISTER(type,fun) ( new vpsim::TlmCallbackPrivate<type>( this, &type::fun ) )

} /* namespace vpsim */

#endif /* _TLMCALLBACKPRIVATE_HPP_ */

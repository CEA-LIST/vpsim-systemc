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

#ifndef NOC_HPP
#define NOC_HPP

//#include "NoCQuantumHistory.hpp"
#include "NoCIF.hpp"
#include "NoCBase.hpp"
#include "NoCCycleAccurate.hpp"
#include "NoCNoContention.hpp"
#include <sys/time.h>

namespace vpsim
{

//!
//! central class use to instanciate all levels of description and implementing
//! the requested level of description
//!
class C_NoC: public C_NoCBase
{

	public:
	//!
	//! Defines all acceptable level of abstraction
	//!
	enum E_ModellingLevel {
		Undef,//!< undefined level (default value)
		CycleAccurate,//!< use cycle accurate NoC models
		QuantumHistory,//!< unimplemented : target fast estimation of contention based on historical knowledge
		QuantumProba,//!< unimplemented : target fast estimation of contention based on probabilities
		NoContention,//!< do not compute any contention (hop count) but use timings
		NoDelay//!< do not compute any timings for the communication
	};

	private:
	E_ModellingLevel ModelLevel;//!< The currently used level of abstraction
	bool IsModelLevelSet;

	//timing features
	bool SimuPerfAnalysis; //!< flag to enable perf analysis
	long int TotalCalcLatencyTime;//!< keeps track of

	bool BeforeElaborationDone;

	C_NoCCycleAccurate* NoCCycleAccurate;
	C_NoCNoContention*  NoCNoContention;
	C_NoCBase* NoCNoCBase; //common to all

	public:
		C_NoC(sc_module_name name_):C_NoCBase(name_)
		{
			ModelLevel=Undef;
			TotalCalcLatencyTime=0;
			SimuPerfAnalysis=true;
			BeforeElaborationDone=false;
			IsModelLevelSet=false;

			NoCNoCBase=(C_NoCBase* )this;
			NoCCycleAccurate=NULL;
			NoCNoContention=NULL;
		};

		~C_NoC()
		{
			cout<<"TotalCalcLatencyTime "<<TotalCalcLatencyTime<<endl;
		}

		void SetAccuracyLevel(E_ModellingLevel lvl)
		{
			if(BeforeElaborationDone)
				SYSTEMC_ERROR("BeforeElaborationDone already invoked");
			ModelLevel=lvl;
			cout << "called SetAccuracyLevel with param "<<lvl<<endl;
			IsModelLevelSet=true;
		}

		void SetAccuracyLevel(E_ModellingLevel lvl)
		{
			if(BeforeElaborationDone)
				SYSTEMC_ERROR("BeforeElaborationDone already invoked, cannot set level of description");
			ModelLevel=lvl;
			//cout << "called SetAccuracyLevel with param "<<lvl<<endl;
			IsModelLevelSet=true;
		}


		void before_end_of_elaboration()
		{
			if(!IsModelLevelSet)
			{
				 SYSTEMC_ERROR("undefined level of description in before_end_of_elaboration");
			}

			switch( ModelLevel)
			{
				case CycleAccurate: {
						stringstream ss;
						ss<<this->name()<<"_CycleAccurate";
						NoCCycleAccurate=new C_NoCCycleAccurate(ss.str().c_str(),NoCNoCBase);
						break;
				}
				case NoContention: {
						stringstream ss;
						ss<<this->name()<<"_NoContention";
						NoCNoContention=new C_NoCNoContention(ss.str().c_str(),NoCNoCBase);
						break;
				}
				case QuantumProba:
				case Undef:
				default: SYSTEMC_ERROR("undefined level of description"<<ModelLevel); break;
			}


			BeforeElaborationDone=true;
		}
};

};//namespace vpsim

#endif //NOC_HPP

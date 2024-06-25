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

#include <gic.hpp>
#include <core/TlmCallbackPrivate.hpp>
#include <string.h>
#include <stdlib.h>
#include "InitiatorIf.hpp"
#include "log.hpp"




using namespace std;
using namespace tlm;

namespace vpsim {

gic::gic(sc_module_name name): sc_module(name),TargetIf(string(name),0x100000) {
	RegisterReadAccess(REGISTER(gic,read));
	RegisterWriteAccess(REGISTER(gic,write));

	// SC_THREAD(distributor_thread);

	//mInterruptInputs = &getLocalMem()[(mDistBase+0x200)/4];
	//getLocalMem()[2]=0x3b;

	mRoundRobin=0;



}

void gic::distributor_thread() {
	while(1) {
		sc_core::wait(1,SC_NS);
		comb_logic();
		throw(0);
	}
}

void gic::connectCpu(InterruptIf* cpu, uint32_t id) {
	mCpus.push_back(make_pair(id, cpu));
	mAck.push_back(1023);
	mAlreadyNotified.push_back(false);
	mCpuQueue.push_back(std::deque<uint32_t>());
	mVCpuQueue.push_back(std::deque<uint32_t>());
	getLocalMem()[mDistBase/4+1] = 3 | ((mCpus.size())<<5);
	mInterruptOrigins.push_back(std::unordered_map<uint32_t,uint32_t>());
	mPendingInterrupts.push_back(std::vector<uint32_t>());
	mPendingInterrupts.back().resize(N_GIC_INTERRUPTS/32);
	mTouched.push_back(std::vector<uint32_t>());
	mEnableStatus.push_back(std::map<uint32_t,bool>());

	(((uint32_t*)getLocalMem())+mVDistBase+0x1000*id+0x30)[0*4]=0xffffffff;
	(((uint32_t*)getLocalMem())+mVDistBase+0x1000*id+0x30)[1*4]=0xffffffff;
	//setInterruptTarget(id,1<<id);
}


tlm::tlm_response_status gic::read (payload_t & payload, sc_time & delay) {
	mGicLock.lock();
	tlm::tlm_generic_payload* tlm_pl = payload.original_payload;
	GicCpuExtension *cpu_if;
	tlm_pl->get_extension<GicCpuExtension>(cpu_if);
	if (!cpu_if) {
		cpu_if = dynamic_cast<GicCpuExtension*>(tlm_pl->get_extension(155));
	}
	if (!cpu_if) {
		throw runtime_error("GicCpuExtension is mendatory for correct Gic operation !");
	}
	uint32_t cpu_id = (cpu_if ? cpu_if->cpu_id: 0);
	uint64_t offset = payload.addr-getBaseAddress();
	if (offset >= mDistBase && offset < mDistBase+mDistSize) {
		uint64_t off=(offset-mDistBase);
		if (off >= 0xf000)
			off-=0xf000;
		*(uint32_t*)payload.ptr = readDist(off,payload.len, cpu_id);
	} else if (offset >= mCpuBase && offset  < mCpuBase+mCpuSize) {
		uint64_t off=(offset-mCpuBase);
		if (off >= 0xf000)
			off-=0xf000;
		*(uint32_t*)payload.ptr = readCpu(off,payload.len,cpu_id);
	} else if (offset >= mVCPUBase && offset  < mVCPUBase+mVCPUSize) {
		uint64_t off=(offset-mVCPUBase);
		if (off >= 0xf000)
			off-=0xf000;
		*(uint32_t*)payload.ptr = readVCpu(off,payload.len,cpu_id);
	} else if (offset >= mVDistBase && offset  < mVDistBase+mVDistSize) {
		uint64_t off=(offset-mVDistBase);
		if (off >= 0xf000)
			off-=0xf000;
		*(uint32_t*)payload.ptr = readVDist(off,payload.len,cpu_id);
	}

	else {
		throw runtime_error(to_string(payload.addr) + " : Unknown GIC region.");
	}
	mGicLock.unlock();
	LOG_DEBUG(dbg3)<<hex<<cpu_if->cpu_id<<" "<<payload.addr<<" READ "<<(*(uint32_t*)payload.ptr)<<dec<<" LEN "<<payload.len<<endl;

	return tlm::TLM_OK_RESPONSE;
}
tlm::tlm_response_status gic::write (payload_t & payload, sc_time & delay) {
	mGicLock.lock();
	tlm::tlm_generic_payload* tlm_pl = payload.original_payload;
	GicCpuExtension *cpu_if;
	tlm_pl->get_extension<GicCpuExtension>(cpu_if);
	if (!cpu_if) {
		cpu_if = dynamic_cast<GicCpuExtension*>(tlm_pl->get_extension(155));
	}
	if (!cpu_if) {
		throw runtime_error("GicCpuExtension is mendatory for correct Gic operation !");
	}
	uint32_t cpu_id = (cpu_if ? cpu_if->cpu_id: 0);
	LOG_DEBUG(dbg3)<<hex<<cpu_if->cpu_id<<" "<<payload.addr<<" WRITE "<<(*(uint32_t*)payload.ptr)<<dec<<" LEN "<<payload.len<<endl;
	//cout<<"CPU ID: "<<cpu_if->cpu_id<<endl;
	uint64_t offset = payload.addr-getBaseAddress();
	if (offset >= mDistBase && offset < mDistBase+mDistSize) {
		uint64_t off=(offset-mDistBase);
				if (off >= 0xf000)
					off-=0xf000;
		writeDist(off,*(uint32_t*)payload.ptr, payload.len, cpu_id);
	} else if (offset >= mCpuBase && offset  < mCpuBase+mCpuSize) {
		uint64_t off=(offset-mCpuBase);
				if (off >= 0xf000)
					off-=0xf000;
		writeCpu(off,*(uint32_t*)payload.ptr,payload.len, cpu_id);
	} else if (offset >= mVCPUBase && offset  < mVCPUBase+mVCPUSize) {
		uint64_t off=(offset-mVCPUBase);
				if (off >= 0xf000)
					off-=0xf000;
		writeVCpu(off,*(uint32_t*)payload.ptr,payload.len, cpu_id);
	} else if (offset >= mVDistBase && offset  < mVDistBase+mVDistSize) {
		uint64_t off=(offset-mVDistBase);
				if (off >= 0xf000)
					off-=0xf000;
		writeVDist(off,*(uint32_t*)payload.ptr,payload.len, cpu_id);
	}

	else {
		throw runtime_error(to_string(payload.addr) + " : Unknown GIC region.");
	}
	comb_logic();
	mGicLock.unlock();
	return tlm::TLM_OK_RESPONSE;
}

void gic::writeCpu(uint64_t offset, uint32_t value, uint64_t size, uint32_t id) {
	uint8_t* base = ((uint8_t*)getLocalMem())+mCpuBase;
	if ((offset==0x10||offset==0x24) || offset==0x1000) {
		/*if (!mAlreadyNotified[id]) {
			throw runtime_error("Very weird, did not notify CPU but got ACK.");
		}*/

		uint32_t curr_mode = base[0+id*0x100+1] & 2;
		if (!curr_mode || offset==0x1000){
			//if(1) {
				LOG_DEBUG(dbg1)<<"cpu "<<id<<" handled: "<<hex;
				for (uint32_t myInterrupt: mCpuQueue[id]) {
					LOG_DEBUG(dbg1)<<myInterrupt<<" ";
				}
				LOG_DEBUG(dbg1)<<dec<<endl;
			//}
			if (mCpuQueue[id].empty()) {
				throw runtime_error("Something went wront with GIC interrupt handling.");
			}
			uint32_t interrupt = value & 0x3FF; // interrupt ID
			setInterruptPending(id, interrupt, 0);
			uint32_t count=0;


			//if (count > 1)
				//cout<<"WARNING: More than one occurence of interrupt: "<<interrupt<<endl;
			if (interrupt != (mCpuQueue[id].front() & 0x3FF)) {
				throw runtime_error(
						to_string(interrupt) + " != " +
						to_string(mCpuQueue[id].front() & 0x3FF) + " -> Bullshit.");
			}
			mCpuQueue[id].pop_front();
			if (!mCpuQueue[id].empty()) {
				setInterruptPending(id, mCpuQueue[id].front() & 0x3FF, 1);
				setCurrentInterruptId(id,mCpuQueue[id].front());
			} else {
				setCurrentInterruptId(id,1023);
				mRoundRobin = (mRoundRobin+1) % mCpus.size();
			}
		}
		/*if (mInterruptInputs[interrupt/32] & (1<<(interrupt%32)) ) {
			cout<<"clearing interrupt line "<<interrupt<<endl;
			cout << "Not disabled..."<<hex<<mInterruptInputs[interrupt/32]<<dec<<endl;
			throw 0;
		}*/
	}
	else
		memcpy(base+offset+id*0x100,&value,size);
}
uint32_t gic::readCpu(uint64_t offset, uint64_t size, uint32_t id) {
	uint32_t val = *(uint32_t*)(((uint8_t*)getLocalMem())+mCpuBase+id*0x100+offset);
	if ((offset == 0xC || offset == 0x20) && (val & 0x3FF)!=1023) // ack
	{
		// mAlreadyNotified[id]=true;
	}
	return val;
}

VIntEntry gic::getVIntEntry(uint32_t index) {
	VIntEntry* dummy = (VIntEntry*) &(((uint8_t*)getLocalMem())+mVDistBase)[0x100+4*index];
	VIntEntry ret = *dummy;
	return ret;
}

void gic::writeVCpu(uint64_t offset, uint32_t value, uint64_t size, uint32_t id) {
	uint8_t* base = ((uint8_t*)getLocalMem())+mVCPUBase;
	if ((offset==0x10||offset==0x24) || offset==0x1000) {

		uint32_t curr_mode = base[0+id*0x100+1] & 2;
		if (!curr_mode || offset==0x1000){
			if (mVCpuQueue[id].empty()) {
				throw runtime_error("Something went wront with GIC interrupt handling.");
			}
			VIntEntry vint = getVIntEntry(mVCpuQueue[id].front());



			LOG_DEBUG(dbg1)<<"cpu "<<id<<"(virt) VM handled: "<<hex;
			for (uint32_t myInterrupt: mVCpuQueue[id]) {
				LOG_DEBUG(dbg1)<<getVIntEntry(myInterrupt).VirtualID<<"("<<myInterrupt<<") ";
			}
			LOG_DEBUG(dbg1)<<dec<<endl;

			uint32_t interrupt = value & 0x3FF; // interrupt ID
			if (interrupt != vint.VirtualID) {
				throw runtime_error(to_string(id) + " " + to_string(interrupt) + " != " + to_string(vint.VirtualID) + " -> Bullshit.");
			}

			uint32_t off = mVCpuQueue[id].front()/32;
			uint32_t list = mVCpuQueue[id].front()%32;
			(((uint8_t*)getLocalMem())+mVDistBase+0x1000*id)[off*4+0x30] |= 1 << list;
			((VIntEntry*)(&(((uint8_t*)getLocalMem())+mVDistBase+0x1000*id)[0x100+mVCpuQueue[id].front()*4]))->State=0;
			//*(uint32_t*)&(((uint8_t*)getLocalMem())+mVDistBase+0x1000*id)[0x100+mVCpuQueue[id].front()]=0xab;

			if (vint.HW==1) {
				setInterruptPending(id, interrupt, 0);
				if (vint.PhysicalID != (mCpuQueue[id].front() & 0x3FF)) {
					throw runtime_error(string("Physical interrupt :") +
							to_string(vint.PhysicalID) + " does not match queue front: " +
							to_string(mCpuQueue[id].front() & 0x3FF));
				}
				mCpuQueue[id].pop_front();
			}
			else {
				if (vint.PhysicalID & (1<<9)) {
					// notify
					(((uint8_t*)getLocalMem())+mVDistBase+0x1000*id)[0x10] |= 1;
					(((uint8_t*)getLocalMem())+mVDistBase+0x1000*id)[0x20+off*4] |= 1<<list;
					//writeDist(0x100,1<<mMaintIrq,4,id);
					//update_irq(1,mMaintIrq | ((1<<id)<<16));
					genMaint(id);

					LOG_DEBUG(dbg1)<<"GIC: Request for maintenance int CPU="<<id<<endl;
				}
			}

			// mark list as empty




			mVCpuQueue[id].pop_front();
			if (!mVCpuQueue[id].empty()) {
				VIntEntry vint = getVIntEntry(mVCpuQueue[id].front());
				uint32_t x = vint.PhysicalID;
				uint32_t next_int=vint.VirtualID;
				if (!vint.HW)
					next_int |= (x<<10);
				setCurrentVirtualInterruptId(id,next_int);
			} else {
				setCurrentVirtualInterruptId(id,1023);

				// notify end of interrupts.
				if ((((uint32_t*)getLocalMem())+mVDistBase+0x1000*id)[0+0x30] == 0xffffffff
						&& (((uint32_t*)getLocalMem())+mVDistBase+0x1000*id)[4+0x30] == 0xffffffff &&
						    ((((uint8_t*)getLocalMem())+mVDistBase+0x1000*id)[0] & (1<<3))) {
					(((uint8_t*)getLocalMem())+mVDistBase+0x1000*id)[0x10] |= 1<<3;
					//writeDist(0x100,1<<mMaintIrq,4,id);
					//update_irq(1,mMaintIrq | ((1<<id)<<16));
					genMaint(id);
					cout<<"Notif !"<<endl;
				}
			}
		}
		/*if (mInterruptInputs[interrupt/32] & (1<<(interrupt%32)) ) {
			cout<<"clearing interrupt line "<<interrupt<<endl;
			cout << "Not disabled..."<<hex<<mInterruptInputs[interrupt/32]<<dec<<endl;
			throw 0;
		}*/
	}
	else
		memcpy(base+offset+id*0x100,&value,size);
}
uint32_t gic::readVCpu(uint64_t offset, uint64_t size, uint32_t id) {
	uint32_t val = *(uint32_t*)(((uint8_t*)getLocalMem())+mVCPUBase+id*0x100+offset);
	if ((offset == 0xC || offset == 0x20) && (val & 0x3FF)!=1023) // ack
	{
		// mAlreadyNotified[id]=true;
	}
	return val;
}

void gic::writeDist(uint64_t offset, uint32_t value, uint64_t size, uint32_t cpu_id) {
	uint8_t* base = ((uint8_t*)getLocalMem())+mDistBase;
	if (offset >= 0x300 && offset <= 0x37C) {
		//cout<<cpu_id<<" set active !"<<endl;
	} else if (offset >= 0x380 && offset <= 0x3FC) {
		//cout<<cpu_id<<" clear active ! "<<hex<<value<<dec<<endl;
	} else if (offset >= 0x200 && offset <= 0x27C) {
		//cout<<cpu_id<<" set pending !"<<endl;
	} else if (offset >= 0x280 && offset <= 0x2FC) {
		//cout<<cpu_id<<" clear pending !"<<endl;
	}
	if (offset >= 0x100 && offset <= 0x17C) {
		// interrupt enable
		*(uint32_t*)&base[offset] |= value;
		uint32_t idx=(offset-0x100);
		for (uint32_t i=0; i <32; i++) {
			if ((1<<i)&value) {
				uint32_t theInterrupt = idx*8+i;

				LOG_DEBUG(dbg1)<<"Interrupt "<<theInterrupt<<" enabled."<<endl;

				if (theInterrupt>31) {
					for (auto cpu: mCpus) {
						mEnableStatus[cpu.first][theInterrupt]=true;
					}
				} else
					mEnableStatus[cpu_id][theInterrupt]=true;
			}
		}

		// cout<<cpu_id<<" Enable !"<<hex<<offset<<" value = "<<value<<dec<<endl;
	} else if (offset >= 0x180 && offset <= 0x1FC) {
		// interrupt disable
		*(uint32_t*)&base[offset-0x80] &= ~value;
		*(uint32_t*)&base[offset] |= value;
		uint32_t idx=(offset-0x100);
		for (uint32_t i=0; i <32; i++) {
			if ((1<<i)&value) {
				uint32_t theInterrupt = idx*8+i;
				LOG_DEBUG(dbg1)<<"Interrupt "<<theInterrupt<<" disabled."<<endl;
				if (theInterrupt>31) {
					for (auto cpu: mCpus) {
						mEnableStatus[cpu.first][theInterrupt]=false;
					}
				} else
					mEnableStatus[cpu_id][theInterrupt]=false;
			}
		}
		// cout<<cpu_id<<" disable !"<<hex<<offset<<" value = "<<value<<dec<<endl;
	} else if (offset == 0xF00) {
		// software interrupt
		uint8_t targetlist = 0xFF;
		uint8_t policy = (value >> 24) & 3;
		if (policy==0) {
			targetlist=(value>>16) & 0xFF;
		} else if (policy==1) {
			targetlist = (~(1<<cpu_id)) & 0xFF;
		} else if (policy==2) {
			targetlist = (1<<cpu_id);
		}
		setInterruptTarget(value & 0xF, targetlist);
		mPendingCpuIds[value & 0xF]=cpu_id;
		for (uint32_t cpuT=0; cpuT<mCpus.size(); cpuT++)
			if ((1<<cpuT) & targetlist) {
				mInterruptOrigins[cpuT][value&0xF] = cpu_id;
				setInterruptPending(cpuT, value & 0xF, 1);
			    LOG_DEBUG(dbg1)<<"cpu "<<cpu_id<<" software interrupt -> "<<hex<<cpuT<<endl;
				//cout<<cpu_id<<"request to wake up "<<cpuT<<endl;
			}

	} else  {
		memcpy(base+offset,&value,size);
	}
}
uint32_t gic::readDist(uint64_t offset, uint64_t size, uint32_t cpu_id) {
	//cout<<"Reading dist offset "<<hex<<offset<<dec<<endl;
	if (offset>=0x800 && offset < 0x800+8*4) {
		return (1<<cpu_id)|(1<<(cpu_id+8))|(1<<(cpu_id+16))|(1<<(cpu_id+24));
	}
	return *(uint32_t*)(((uint8_t*)getLocalMem())+mDistBase+offset);
}

uint8_t gic::getVCpuPriorityMask(uint32_t cpu_id) {
	uint8_t* bytebase = (uint8_t*)getLocalMem();
	return bytebase[mVCPUBase+cpu_id*0x100+4];
}

void gic::writeVDist(uint64_t offset, uint32_t value, uint64_t size, uint32_t id) {
	if (offset>=0x100 && offset < 0x200) {
		// List registers

		LOG_DEBUG(dbg3)<<"cpu "<<id<<" writing list register "<<hex<<offset<<"="<<value<<endl;

		auto which = (offset - 0x100) / 4;
		uint32_t off = which/32;
		uint32_t list = which%32;

		VIntEntry* vint=(VIntEntry*)&value;

		if (vint->State==0) {
			//cout<<id<<" -> clearing virtual interrupt: "<<which<<" value="<<value<<endl;
			(((uint8_t*)getLocalMem())+mVDistBase+0x1000*id+0x30)[off*4] |=  (1<<list);
		}
		else /*if (vint->Priority > getVCpuPriorityMask(id))*/ {
			bool found=false;
			for (uint32_t k : mVCpuQueue[id])
				if (getVIntEntry(k).VirtualID==vint->VirtualID && getVIntEntry(k).PhysicalID==vint->PhysicalID)
					found=true;
			if (!found) {
				mVCpuQueue[id].push_back(which);

			// List is no longer empty
				(((uint8_t*)getLocalMem())+mVDistBase+0x1000*id+0x30)[off*4] &= ~ (1<<list);


				//cout<<id<<" -> sending virtual interrupt: "<<vint->VirtualID<<" HW:"<<vint->HW<<" Phys: "<<vint->PhysicalID<<endl;
			}
		}

		if (vint->VirtualID==0xb) throw(0);

		if (mVCpuQueue[id].empty() && ((((uint8_t*)getLocalMem())+mVDistBase+0x1000*id)[0] & (1<<3))) {
			// NP interrupt
			(((uint8_t*)getLocalMem())+mVDistBase+0x1000*id)[0x10] |= 8;
			//writeDist(0x100,1<<mMaintIrq,4,id);
			//update_irq(1,mMaintIrq | ((1<<id)<<16));
			// cout<<"Sent NP interrupt."<<endl;
			genMaint(id);
		}
	}
	*(uint32_t*)(((uint8_t*)getLocalMem())+mVDistBase+0x1000*id+offset)=value;

	if (offset == 8) {
		// forward alias to correct register
		uint32_t gicv_ctrl=value & 0x1ff;
		uint32_t o_gicv_ctrl=readVCpu(0,4,id);
		writeVCpu(0, (o_gicv_ctrl&~0x1ff)|gicv_ctrl,4, id);
		uint32_t pmr=value >> 27;
		uint32_t o_pmr = readVCpu(4,4,id);
		writeVCpu(4, (pmr<<3) | (o_pmr & ~(0x1f<<3)),4, id);
	}

	if (offset == 0) {
		LOG_DEBUG(dbg3)<<"cpu "<<id<<": New VCPU interface config value: "<<hex<<value<<dec<<endl;
		if (value != 0 && value != 1)
			throw runtime_error(to_string(id) + " --> Current value: " + to_string(value));
	}

}
uint32_t gic::readVDist(uint64_t offset, uint64_t size, uint32_t id) {
	if (offset==4) {
		return 0x3f | (4<<26) | (4<<29);
	}
	if (offset == 8) // vm ctrl
	{
		uint32_t vmc=0;
		uint32_t gicv_ctrl=readVCpu(0,4,id);
		uint32_t gicv_pmr=readVCpu(4,4,id);
		uint32_t gicv_bpr=readVCpu(8,4,id);
		uint32_t gicv_abpr=readVCpu(0x1c,4,id);
		struct vmcd{
			uint32_t grp0en:1;
			uint32_t grp1en:1;
			uint32_t ackctrl:1;
			uint32_t fiqen:1;
			uint32_t cbpr:1;
			uint32_t _reserved:4;
			uint32_t vem:1;
			uint32_t _res:8;
			uint32_t abp:3;
			uint32_t bp:3;
			uint32_t __n:3;
			uint32_t primask:5;
		} vmcprox;
		vmc |= gicv_ctrl&0x1ff;
		vmc |= (gicv_bpr&7) << 21;
		vmc |= (gicv_abpr&7) << 18;
		vmc |= (gicv_pmr>>3) << 27;
		return vmc;
	}
	return *(uint32_t*)(((uint8_t*)getLocalMem())+mVDistBase+0x1000*id+offset);
}

void gic::genMaint(uint32_t cpu) {
	bool alreadyIn=false;
	for (auto in: mCpuQueue[cpu]) {
		if (in == mMaintIrq) {
			alreadyIn=true;
			break;
		}
	}
	if (!alreadyIn)
		mCpuQueue[cpu].push_back(mMaintIrq);
	//setCurrentInterruptId(cpu, mCpuQueue[cpu].front());
	comb_logic();
}


void gic::update_irq  ( uint64_t val, uint32_t irq_idx ) {
	mGicLock.lock();
	uint16_t actual_irq = irq_idx & 0xFFFF;
	uint16_t targets = irq_idx >> 16;


	if (actual_irq >= 32) // spi
	{
		for (uint32_t i=0; i < mPendingInterrupts.size(); i++)
			setInterruptPending(i, actual_irq,val);
	}
	else if(actual_irq >= 16) // ppi
	{
		for (uint32_t i=0; i < mCpus.size(); i++)
			if ((1<<mCpus[i].first) & targets) {
				setInterruptTarget(actual_irq, targets);
				setInterruptPending(mCpus[i].first,actual_irq,val);
			}
	}
	else {
		throw runtime_error("GIC: Unexpected SGI in update_irq(). Trying to send a soft-int from anything other than software ?");
	}

	//if (val)cout<<"set pending."<<actual_irq<<endl;
	//cout<<"Triggered by : "<<irq_idx<<endl;
	comb_logic();
	mGicLock.unlock();
}

void gic::setInterruptTarget(uint32_t interrupt, uint8_t targetlist) {
	uint8_t* base = ((uint8_t*)getLocalMem()) + mDistBase + 0x800;
	//cout<<"target "<<hex<<(uint32_t)targetlist<<dec<<endl;
	base[interrupt] = targetlist;
}
bool gic::cpuInTarget(uint32_t interrupt, uint32_t cpu) {
	uint8_t* base = ((uint8_t*)getLocalMem()) + mDistBase + 0x800;
	//if (interrupt==30){
	//	cout<<hex<<*(uint32_t*)&base[interrupt]<<dec<<endl;
	//}
	return ((base[interrupt] & (1<<cpu)) != 0);
}
void gic::setInterruptPending(uint32_t cpu, uint32_t interrupt, uint32_t value) {
	uint32_t index = interrupt/32;
	if (value){
		mPendingInterrupts[cpu][index] |= (1 << (interrupt%32));
		//cout<<"setting int "<<interrupt<<endl;
	}
	else {
		//cout<<"before "<<getLocalMem()[(mDistBase+0x200)/4+index]<<endl;
		mPendingInterrupts[cpu][index] &= ~(1 << (interrupt%32));
		//cout<<"after "<<getLocalMem()[(mDistBase+0x200)/4+index]<<endl;
	}
}
bool gic::isEnabled() {
	return getLocalMem()[mDistBase/4] & 1;
}
bool gic::cpuTakesInterrupts(uint32_t cpu_id) {
	return getLocalMem()[(mCpuBase + 0x100*cpu_id)/4] & 1;
}
void gic::setCurrentInterruptId(uint32_t cpu_id, uint32_t value) {
	/*
	if (value < 16) {
		// sgi
		if (mInterruptOrigins[cpu_id].find(value) == mInterruptOrigins[cpu_id].end()) {
			throw runtime_error("GIC: Could not determine interrupt origin CPU.");
		}
		value |=(mInterruptOrigins[cpu_id][value]<<10);
	} else if (value != 1023) {
		//cout<<"int "<<value<<endl;
	}*/
	getLocalMem()[(mCpuBase + 0x100*cpu_id + 0xC)/4] = value;
	//if (value!=1023)if(value==1){cout<<"current: "<<cpu_id<<" "<<value<<endl;throw 0;}
}

void gic::setCurrentVirtualInterruptId(uint32_t cpu_id, uint32_t value) {
	/*
	if (value < 16) {
		// sgi
		if (mInterruptOrigins[cpu_id].find(value) == mInterruptOrigins[cpu_id].end()) {
			throw runtime_error("GIC: Could not determine interrupt origin CPU.");
		}
		value |=(mInterruptOrigins[cpu_id][value]<<10);
	} else if (value != 1023) {
		//cout<<"int "<<value<<endl;
	}*/
	getLocalMem()[(mVCPUBase + 0x100*cpu_id + 0xC)/4] = value;
	//if (value!=1023)if(value==1){cout<<"current: "<<cpu_id<<" "<<value<<endl;throw 0;}
}

uint32_t gic::getInterruptEnableDist(uint32_t i) {
	return getLocalMem()[(mDistBase + 0x100 + i*4)/4];
}

void gic::comb_logic() {
	uint32_t cpu_iter=mRoundRobin;

	for (uint32_t cpuI=0; cpuI<mCpus.size(); cpuI++ ) {
		cpu_iter = (cpu_iter + 1) % mCpus.size();
		auto i_cpu = mCpus[cpu_iter];
		uint32_t cpu_id = i_cpu.first;
		InterruptIf* cpu = i_cpu.second;
		bool interrupt=false;
		if (!isEnabled() || !cpuTakesInterrupts(cpu_id)) {
			// cout<<"sending null interrupt to "<<cpu_id<<endl;
			cpu->update_irq(interrupt, 0 | (cpu_id<<16));
			continue;
		}

		uint32_t n_ints=mCpuQueue[cpu_id].size();
		//if (mCpuQueue[cpu_id].empty()) {
		for (auto tch_pair : mEnableStatus[cpu_id]) {
			uint32_t tch = tch_pair.first;
			uint32_t i=tch/32;
			uint32_t j=tch%32;
			//cout<<tch<<" ";
			uint32_t masked = mPendingInterrupts[cpu_id][i];
					//& getInterruptEnableDist(i);
			//if (tch==70) cout<<"masked "<<hex<<masked<<dec<<endl;
			//if (tch==70) cout<<"in target "<<hex<<cpuInTarget(j+i*32, cpu_id)<<dec<<endl;
			//if (tch==70) cout<<"i"<<hex<<cpuInTarget(j+i*32, cpu_id)<<dec<<endl;
			/*if (masked) */{
				/*for (uint32_t j = 0; j < 32; j++)*/ {

					uint32_t ackValue=j+i*32;
					if (ackValue<16){
						ackValue |= (mInterruptOrigins[cpu_id][ackValue]<<10);
					}
					int alreadyQueued = 0;
					int ctr=0;



					if (getInterruptPriority(j+i*32)>getCpuPriorityMask(cpu_id)) {
						//if (alreadyQueued) n_ints--;
						continue;
					}

					if (((masked & (1<<j)) && cpuInTarget(j+i*32, cpu_id))) {
						//cout<<"interrupt ! "<<j+i*32<<" to "<<hex<<cpu_id<<dec<<endl;

						if (mEnableStatus[cpu_id].find(j+i*32)!=mEnableStatus[cpu_id].end() && mEnableStatus[cpu_id][j+i*32] ) {
							for (auto inter: mCpuQueue[cpu_id]){
								if (ackValue==inter) {
									alreadyQueued++;
									break;
								}
								ctr++;
							}
							if (!alreadyQueued) {mCpuQueue[cpu_id].push_back(ackValue); /*if (j+i*32 >= 32) cout<<cpu_id<<": interrupt planned: "<<ackValue<<endl;*/ }

							n_ints++;
						}
						//if (mCpuQueue[cpu_id].size()>1)
							//throw 0;
					}
				}
			}
		}

		//cout<<"\n\n";

		//if (cpu_id==0 && getInterruptEnableDist(0) & (1<<30))
			//mCpuQueue[cpu_id].push_back(30);
		//}
		int line=0;
		if (!mCpuQueue[cpu_id].empty()/* && !mAlreadyNotified[cpu_id]*/) {
			setCurrentInterruptId(cpu_id, mCpuQueue[cpu_id].front());
			//mCpuQueue[cpu_id].pop_front();
			interrupt=true;
			LOG_DEBUG(dbg2)<<getName()<<": interrupt "<<mCpuQueue[cpu_id].front()<<" on cpu "<<hex<<cpu_id<<dec<<endl;
			//if (mCpuQueue[cpu_id].front() == mMaintIrq)
				//line=1;
			/*for (auto interr: mCpuQueue[cpu_id])
			    cout<<"interrupt ! "<<mCpuQueue[cpu_id].front()<<" to "<<hex<<cpu_id<<dec<<endl;
			cout<<endl;*/
		}
		//if (!mAlreadyNotified[cpu_id]) {
			cpu->update_irq(interrupt, line | (cpu_id<<16));

		bool virt_int=false;
		if (mVCpuQueue[cpu_id].empty() == false) {
			VIntEntry vint = getVIntEntry(mVCpuQueue[cpu_id].front());
			uint32_t x = vint.PhysicalID;
			uint32_t next_int=vint.VirtualID;
			if (!vint.HW)
				next_int |= (x<<10);
			if (next_int<16)
				next_int |= ((vint.PhysicalID&7)<<10);
			setCurrentVirtualInterruptId(cpu_id,next_int);
			virt_int=true;

		}
		cpu->update_irq(virt_int,2 | (cpu_id<<16));
			//if (interrupt)
				// yield now !
				//sc_core::wait(1, SC_NS);
			//if (!interrupt)
				//cout<<cpu_id<<" Lowering "<<endl;
		//}
		// yield immediately
		//if (interrupt) cout<<"interrupting."<<endl;
	}
}

uint8_t gic::getInterruptPriority(uint32_t interrupt) {
	uint8_t* bytebase = (uint8_t*)getLocalMem();
	uint8_t* priobase = bytebase + mDistBase + 0x400;
	return priobase[interrupt];
}
uint8_t gic::getCpuPriorityMask(uint32_t cpu_id) {
	uint8_t* bytebase = (uint8_t*)getLocalMem();
	return bytebase[mCpuBase+cpu_id*0x100+4];
}

} /* namespace vpsim */

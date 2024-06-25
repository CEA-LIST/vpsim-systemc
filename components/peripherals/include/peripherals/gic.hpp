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

#ifndef _GIC_HPP_
#define _GIC_HPP_
#include <cstdint>
#include <iostream>
#include <deque>
#include <unordered_map>
#include <core/TargetIf.hpp>
#include "paramManager.hpp"
#include "InterruptIf.hpp"
#include <mutex>

#define N_GIC_INTERRUPTS 512
namespace vpsim {

struct VIntEntry {
	uint32_t VirtualID: 10;
	uint32_t PhysicalID: 10;
	uint32_t _reserved: 3;
	uint32_t Priority: 5;
	uint32_t State: 2;
	uint32_t Grp1: 1;
	uint32_t HW: 1;
};

class gic :  public sc_module,
			 public TargetIf<uint32_t>,
			 public InterruptIf {
public:
	gic(sc_module_name name);

	void setDistBase(uint64_t dist_base) { mDistBase=dist_base; }
	void setCPUBase(uint64_t cpu_base)  { mCpuBase=cpu_base; }
	void setDistSize(uint64_t dist_size) { mDistSize=dist_size; }
	void setCPUSize(uint64_t cpu_size) { mCpuSize=cpu_size; }
	void setVDistBase(uint64_t vdist_base) { mVDistBase=vdist_base; }
	void setVDistSize(uint64_t vdist_size) { mVDistSize=vdist_size; }
	void setVCPUBase(uint64_t vcpu_base) { mVCPUBase=vcpu_base; }
	void setVCPUSize(uint64_t vcpu_size) { mVCPUSize=vcpu_size; }

	void setMaintenanceInterrupt(uint32_t irq) { mMaintIrq=irq; }

	VIntEntry getVIntEntry(uint32_t index);

	tlm::tlm_response_status read (payload_t & payload, sc_time & delay);
	tlm::tlm_response_status write (payload_t & payload, sc_time & delay);

	void writeCpu(uint64_t offset, uint32_t value, uint64_t size, uint32_t id);
	uint32_t readCpu(uint64_t offset, uint64_t size, uint32_t id);
	void writeDist(uint64_t offset, uint32_t value, uint64_t size, uint32_t id);
	uint32_t readDist(uint64_t offset, uint64_t size, uint32_t id);

	void writeVCpu(uint64_t offset, uint32_t value, uint64_t size, uint32_t id);
	uint32_t readVCpu(uint64_t offset, uint64_t size, uint32_t id);
	void writeVDist(uint64_t offset, uint32_t value, uint64_t size, uint32_t id);
	uint32_t readVDist(uint64_t offset, uint64_t size, uint32_t id);


	void update_irq  ( uint64_t val, uint32_t irq_idx ) override;
	void comb_logic(); // combinational logic function
	// void update_virt(); // combinational logic function

	void connectCpu(InterruptIf* cpu, uint32_t id);

	void genMaint(uint32_t cpu);

	void setInterruptTarget(uint32_t interrupt, uint8_t targetlist);
	void setInterruptPending(uint32_t cpu, uint32_t interrupt, uint32_t value);
	bool isEnabled();
	bool cpuTakesInterrupts(uint32_t cpu_id);
	void setCurrentInterruptId(uint32_t cpu_id, uint32_t value);
	void setCurrentVirtualInterruptId(uint32_t cpu_id, uint32_t value);
	uint32_t getInterruptEnableDist(uint32_t i);
	bool cpuInTarget(uint32_t interrupt, uint32_t cpu);

	uint8_t getInterruptPriority(uint32_t interr);
	uint8_t getCpuPriorityMask(uint32_t cpu_id);
	uint8_t getVCpuPriorityMask(uint32_t cpu_id);

	SC_HAS_PROCESS(gic);

	void distributor_thread();
private:
	// GIC's address spaces
	uint64_t mDistBase, mDistSize, mCpuBase, mCpuSize, mVDistBase, mVDistSize, mVCPUBase, mVCPUSize;

	uint32_t mMaintIrq;
	// Input wires
	uint32_t *mInterruptInputs;

	// Who requested the SGI
	uint32_t mPendingCpuIds[16];

	// Current Interrupt Acknowledge value per CPU
	std::vector<uint32_t> mAck;

	std::vector<bool> mAlreadyNotified;

	// Pending interrupt queue
	std::vector< std::deque<uint32_t> > mCpuQueue;
	std::vector< std::deque<uint32_t> > mVCpuQueue;

	// Output wires
	std::vector<std::pair<uint32_t,InterruptIf*>> mCpus;

	std::vector< std::unordered_map<uint32_t,uint32_t> > mInterruptOrigins;

	std::vector<std::vector<uint32_t>> mPendingInterrupts;

	uint32_t mRoundRobin;

	std::vector<std::vector<uint32_t>> mTouched;

	std::mutex mGicLock;

	std::vector<std::map<uint32_t,bool>> mEnableStatus;

	uint64_t mCurrentAccessArea;

};

} /* namespace vpsim */

#endif /* _GIC_HPP_ */

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

#ifndef _COSIMEXTENSIONS_HPP_
#define _COSIMEXTENSIONS_HPP_

namespace vpsim {

struct SourceExtension : public tlm::tlm_extension<SourceExtension> {
	uint8_t type;//0 for cpu, 1 for other devices
    sc_time time_stamp;
 	virtual tlm::tlm_extension_base* clone() const {
		SourceExtension* copy = new SourceExtension;
		*copy=*this;
		return copy;
	}
	virtual void copy_from(tlm::tlm_extension_base const &ext) {
		*this=dynamic_cast<SourceExtension const &>(ext);
	}
};

struct SourceCpuExtension : public SourceExtension {
	uint32_t cpu_id;
 	virtual tlm::tlm_extension_base* clone() const override{
		SourceCpuExtension* copy = new SourceCpuExtension;
		*copy=*this;
        copy->type=0;
		return copy;
	}
	virtual void copy_from(tlm::tlm_extension_base const &ext) override{
		*this=dynamic_cast<SourceCpuExtension const &>(ext);
	}

};

struct SourceDeviceExtension : public SourceExtension {
	uint32_t device_id;
 	virtual tlm::tlm_extension_base* clone() const override{
		SourceDeviceExtension* copy = new SourceDeviceExtension;
		*copy=*this;
		copy->type=1;
		return copy;
	}
	virtual void copy_from(tlm::tlm_extension_base const &ext) override {
		*this=dynamic_cast<SourceDeviceExtension const &>(ext);
	}
};

}

#endif /* _COSIMEXTENSIONS_HPP_ */

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

#ifndef PYTHON_DEVICE
#define PYTHON_DEVICE

#define PY_SSIZE_T_CLEAN
#include <map>
#include <string>
#include <core/TlmCallbackPrivate.hpp>
#include <poll.h>
#include <unistd.h>
#include "InterruptSource.hpp"


#include "log.hpp"
using namespace std;

namespace vpsim{
#include "Python.h"
static PyObject*
pyvp_interrupt(PyObject *mod, PyObject *args);

static PyObject*
pyvp_yield(PyObject *mod, PyObject *args);

static PyObject*
PyInit_pyvp(void);


struct PyDevice   : 	public sc_module, public TargetIf<uint8_t>, public InterruptSource {
    SC_HAS_PROCESS(PyDevice);

    tlm::tlm_response_status Read(payload_t &payload, sc_time &delay){
    	uint64_t res=this->read(payload.addr,payload.len);
    	memcpy(payload.ptr,&res,payload.len);
    	return tlm::TLM_OK_RESPONSE;
    }
    tlm::tlm_response_status Write(payload_t &payload, sc_time &delay){
    	uint64_t dt=0;
    	memcpy(&dt,payload.ptr,payload.len);
    	this->write(payload.addr,dt,payload.len);
    	return tlm::TLM_OK_RESPONSE;
    }

	PyDevice(sc_module_name nameS, std::string typeName, std::map<std::string, std::string>& args, uint64_t size):
    	sc_module(nameS),
            TargetIf(string(nameS), size)
 {
    	string name = string(nameS);

    	TargetIf<REG_T>::RegisterReadAccess(REGISTER(PyDevice, Read));
		TargetIf<REG_T>::RegisterWriteAccess(REGISTER(PyDevice, Write));
		char Env[1024];
		sprintf(Env, "%s/lib/py", getenv("VPSIM_HOME"));
		setenv("PYTHONHOME", Env, 1);

        Init();

    	PyObject* pName = VpsimNamespace_PyUnicode_DecodeFSDefault(typeName.c_str());

        if (!pName) {
            fprintf(stderr, "DecodeFSDefault failed...\n");
            VpsimNamespace_PyErr_Print();
            exit(1);
        }

        PyObject* pModule = VpsimNamespace_PyImport_Import(pName);
        Py_DECREF(pName);

        if (pModule != NULL) {
            PyObject* pFunc = VpsimNamespace_PyObject_GetAttrString(pModule, typeName.c_str());
            /* pFunc is the class */

            if (pFunc && VpsimNamespace_PyCallable_Check(pFunc)) {
                PyObject* pArgs = VpsimNamespace_PyDict_New();
                for (auto p : args) {
                    PyObject* pValue = VpsimNamespace_PyBytes_FromString(p.second.c_str());
                    if (!pValue) {
                        Py_DECREF(pArgs);
                        Py_DECREF(pModule);
                        fprintf(stderr, "Cannot convert argument\n");
                        exit(1);
                    }
                    /* pValue reference stolen here: */
                    VpsimNamespace_PyDict_SetItemString(pArgs, p.first.c_str(), pValue);
                }

                PyObject* pValue = VpsimNamespace_PyObject_Call(pFunc, VpsimNamespace_PyTuple_New(0), pArgs);
                Py_DECREF(pArgs);
                if (pValue != NULL) {
                    PyObject* self = VpsimNamespace_PyCapsule_New((void*)this,NULL,NULL);
                    printf("Created capsule.\n");
                    VpsimNamespace_PyObject_SetAttrString(pValue,"_vpsim_dev_ptr",self);
                    printf("Saved capsule pointer.\n");
                    hdl = pValue;
                }
                else {
                    Py_DECREF(pFunc);
                    Py_DECREF(pModule);
                    VpsimNamespace_PyErr_Print();
                    fprintf(stderr,"Call failed\n");
                    exit(1);
                }
            }
            else {
                if (VpsimNamespace_PyErr_Occurred())
                	VpsimNamespace_PyErr_Print();
                fprintf(stderr, "Cannot find function \"%s\"\n", typeName.c_str());
                exit(1);
            }
            Py_XDECREF(pFunc);
            Py_DECREF(pModule);
        }
        else {
        	VpsimNamespace_PyErr_Print();
            fprintf(stderr, "Failed to load \"%s\"\n", typeName.c_str());
            exit(1);
        }

        SC_THREAD(loop);
    }

    void write(uint64_t addr, uint64_t value, uint64_t size) {
    	VpsimNamespace_PyObject_CallMethod(hdl, "write", "(lll)", addr,value,size);
    }

    uint64_t read(uint64_t addr, uint64_t size) {
        PyObject* res = VpsimNamespace_PyObject_CallMethod(hdl, "read", "(ll)",addr,size);
        if (!res) {
        	VpsimNamespace_PyErr_Print();
            exit(1);
        }
        uint64_t l = VpsimNamespace_PyLong_AsLong(res);
        Py_DECREF(res);
        return l;
    }

    void loop() {
    	VpsimNamespace_PyObject_CallMethod(hdl, "loop", "()");
    }

    void wait(uint64_t t) {
    	// printf(" waiting for %ld\n", t);
        sc_core::wait(t,SC_NS);
    }

    void irq(int value, int line) {
    	// printf("IRQ %d = %d\n", line, value);
    	setInterruptLine(line);
    	if(value) raiseInterrupt();
    	else lowerInterrupt();
    }

    ~PyDevice() {
        Py_DECREF(hdl);
    }

    static bool PythonInit;

    static void Init() {
    	if (!PythonInit) {
    		PythonInit=true;
    		if (VpsimNamespace_Py_IsInitialized()) {
    			assert(false);
    			*(uint64_t*)0x5465216 = 1;
    			*(uint64_t*)0x5465216 = 1;
    			*(uint64_t*)0x5465216 = 1;
    			*(uint64_t*)0x5465216 = 1;
    			*(uint64_t*)0x5465216 = 1;
    			throw runtime_error("Local python lib should not be initialized.");
    		}
    		VpsimNamespace_PyImport_AppendInittab("pyvp", &PyInit_pyvp);
    		VpsimNamespace_Py_Initialize();
    	}
    }

private:
    PyObject* hdl;
};

static PyObject*
pyvp_interrupt(PyObject *mod, PyObject *args)
{
    int value, irq;
    PyObject* self;
    if(!VpsimNamespace_PyArg_ParseTuple(args, "Oii", &self, &irq, &value)) {
    	VpsimNamespace_PyErr_Print();
        return NULL;
    }

    PyObject* ptr = VpsimNamespace_PyObject_GetAttrString(self,"_vpsim_dev_ptr");
    PyDevice* x = (PyDevice*) VpsimNamespace_PyCapsule_GetPointer(ptr, NULL);
    x->irq(value,irq);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyObject*
pyvp_yield(PyObject *mod, PyObject *args)
{
    uint64_t  duration;
    PyObject* self;
    if(!VpsimNamespace_PyArg_ParseTuple(args, "Ol", &self, &duration)) {
    	VpsimNamespace_PyErr_Print();
        return NULL;
    }

    PyObject* ptr = VpsimNamespace_PyObject_GetAttrString(self,"_vpsim_dev_ptr");
    PyDevice* x = (PyDevice*) VpsimNamespace_PyCapsule_GetPointer(ptr, NULL);
    x->wait(duration);
    Py_INCREF(Py_None);
    return Py_None;
}

static PyMethodDef EmbMethods[] = {
    {"interrupt", pyvp_interrupt, METH_VARARGS,
     "Send an interrupt to the VPSim Subsystem."},
    {"wait", pyvp_yield, METH_VARARGS,
     "Yield execution to simulation kernel."},
    {NULL, NULL, 0, NULL}
};

static PyModuleDef EmbModule = {
    PyModuleDef_HEAD_INIT, "pyvp", NULL, -1, EmbMethods,
    NULL, NULL, NULL, NULL
};

static PyObject*
PyInit_pyvp(void)
{
    return PyModule_Create(&EmbModule);
}


} //namespace vpsim

#endif

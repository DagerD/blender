/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#include <iostream>
#include <cstdlib>

#include <Python.h>

#include "usd_common.h"
#include "glog/logging.h"

#include "stage.h"
#include "usd_node.h"
#include "session.h"
#include "utils.h"
#include "usd_collection.h"


namespace usdhydra {

static PyObject *init_func(PyObject * /*self*/, PyObject *args)
{
  LOG(INFO) << "init_func";
  //putenv("PXR_PLUGINPATH_NAME=D:\\amd\\blender-git\\usd\\plugin");
  blender::io::usd::ensure_usd_plugin_path_registered();
  stage_init();

  Py_RETURN_NONE;
}

static PyObject *exit_func(PyObject * /*self*/, PyObject * /*args*/)
{
  LOG(INFO) << "exit_func";
  stageCache = nullptr;
  Py_RETURN_NONE;
}

static PyMethodDef methods[] = {
  {"init", init_func, METH_VARARGS, ""},
  {"exit", exit_func, METH_VARARGS, ""},
  {NULL, NULL, 0, NULL},
};

static struct PyModuleDef module = {
  PyModuleDef_HEAD_INIT,
  "_usdhydra",
  "USDHydra render integration",
  -1,
  methods,
  NULL,
  NULL,
  NULL,
  NULL,
};

}   // namespace usdhydra

#ifdef __cplusplus
extern "C" {
#endif

PyObject *USDHydra_initPython(void)
{
  PyObject *mod = PyModule_Create(&usdhydra::module);
  usdhydra::addPythonSubmodule_stage(mod);
  usdhydra::addPythonSubmodule_usd_node(mod);
  usdhydra::addPythonSubmodule_session(mod);
  usdhydra::addPythonSubmodule_utils(mod);
  usdhydra::addPythonSubmodule_usd_collection(mod);

  return mod;
}

#ifdef __cplusplus
}
#endif

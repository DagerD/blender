/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#include <iostream>
#include <cstdlib>

#include <Python.h>

#include <pxr/pxr.h>
#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>

#include "usd_common.h"
#include "glog/logging.h"
#include "BKE_appdir.h"

#include "stage.h"
#include "usd_node.h"
#include "session.h"
#include "utils.h"
#include "usd_collection.h"


namespace usdhydra {

static PyObject *init_func(PyObject * /*self*/, PyObject *args)
{
  static std::string defaultPath = getenv("PATH");

  char *delegates_dir;
  if (!PyArg_ParseTuple(args, "s", &delegates_dir)) {
    Py_RETURN_NONE;
  }

  std::string delegates_dir_str(delegates_dir);

  LOG(INFO) << "init_func(" << delegates_dir_str << ")";

  std::string env("PATH=");
  env += delegates_dir_str + "/lib;";
  env += defaultPath;
  putenv(env.c_str());

  blender::io::usd::ensure_usd_plugin_path_registered();
  pxr::PlugRegistry &registry = pxr::PlugRegistry::GetInstance();
  std::vector<std::string> paths;
  paths.push_back(BKE_appdir_folder_id(BLENDER_DATAFILES, "usd"));
  paths.push_back(delegates_dir_str + "/plugin");
  registry.RegisterPlugins(paths);

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

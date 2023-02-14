/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#include <Python.h>

#include "BKE_material.h"
#include "BKE_lib_id.h"

#include "material.h"

using namespace pxr;

namespace blender::render::hydra {

MaterialData::MaterialData()
  : material(nullptr)
{
}

MaterialData::MaterialData(Material *material)
  : material(material)
{
}

std::string MaterialData::name()
{
  char str[MAX_ID_FULL_NAME];
  BKE_id_full_name_get(str, (ID *)material, 0);
  return str;
}

void MaterialData::export_mtlx()
{
  /* Call of python function hydra.export_mtlx() */

  PyObject *module, *dict, *func, *result;

  PyGILState_STATE gstate;
  gstate = PyGILState_Ensure();

  module = PyImport_ImportModule("hydra");
  dict = PyModule_GetDict(module);
  func = PyDict_GetItemString(dict, "export_mtlx");
  result = PyObject_CallFunction(func, "s", name().c_str());

  std::string path = PyUnicode_AsUTF8(result);

  Py_DECREF(result);
  Py_DECREF(module);

  PyGILState_Release(gstate);

  mtlx_path = SdfAssetPath(path, path);
}

} // namespace blender::render::hydra

/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#pragma once

#include "BKE_main.h"
#include "BKE_context.h"
#include "BKE_collection.h"
#include "BKE_layer.h"
#include "BLI_listbase.h"
#include "DNA_collection_types.h"
#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"
#include "outliner_intern.hh"
#include "WM_api.h"
#include "DNA_space_types.h"

#include "usd_collection.h"
#include "stage.h"

using namespace std;
using namespace pxr;

namespace usdhydra {


void update(BL::Context b_context, int stageId) {

}

static PyObject *update_func(PyObject * /*self*/, PyObject *args)
{
  PyObject *pycontext;

  int stageId = 0;

  if (!PyArg_ParseTuple(args, "Oi", &pycontext, &stageId)) {
    Py_RETURN_NONE;
  }

  PointerRNA contextptr;
  RNA_pointer_create(NULL, &RNA_Context, (ID *)PyLong_AsVoidPtr(pycontext), &contextptr);
  BL::Context b_context(contextptr);

  const string COLLECTION_NAME = "USD NodeTree";

  UsdStageRefPtr stage = stageCache->Find(UsdStageCache::Id::FromLongInt(stageId));

  bContext *C = (bContext *)b_context.ptr.data;
  Main *bmain = CTX_data_main(C);
  Scene *bscene = CTX_data_scene(C);
  ScrArea *area = CTX_wm_area(C);
  SpaceOutliner *space_outliner = CTX_wm_space_outliner(C);

  Collection *scene_collection = bscene->master_collection;
  Collection *usd_collection = nullptr;
  
  for (CollectionChild *collection = (CollectionChild *)scene_collection->children.first; collection != NULL;
         collection = collection->next) {

    if (BKE_collection_ui_name_get(collection->collection) == COLLECTION_NAME) {
      usd_collection = collection->collection;
      break;
    }

  }
  
  if (!usd_collection) {
    usd_collection = BKE_collection_add(bmain, scene_collection, COLLECTION_NAME.c_str());
    DEG_id_tag_update(&usd_collection->id, ID_RECALC_COPY_ON_WRITE);
    DEG_relations_tag_update(bmain);

    if ((area != nullptr) && (area->spacetype == SPACE_OUTLINER)) {
      outliner_cleanup_tree(space_outliner);
    }

    WM_main_add_notifier(NC_SCENE | ND_LAYER, nullptr);
  }




  Py_RETURN_NONE;
}

static PyMethodDef methods[] = {
  {"update", update_func, METH_VARARGS, ""},
  {NULL, NULL, 0, NULL},
};

static struct PyModuleDef module = {
  PyModuleDef_HEAD_INIT,
  "usd_collection",
  "This module provides access to usd_collection functions.",
  -1,
  methods,
  NULL,
  NULL,
  NULL,
  NULL,
};

PyObject* addPythonSubmodule_usd_collection(PyObject* mod) {
  PyObject *submodule = PyModule_Create(&module);
  PyModule_AddObject(mod, "usd_collection", submodule);
  return submodule;
}

} // namespace usdhydra

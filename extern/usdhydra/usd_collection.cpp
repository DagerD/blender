/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#pragma once

#include <pxr/usd/usd/primRange.h>
#include <algorithm>
#include <string.h>

#include "BKE_main.h"
#include "BKE_context.h"
#include "BKE_collection.h"
#include "BKE_layer.h"
#include "BKE_idprop.h"
#include "BKE_object.h"
#include "BLI_listbase.h"
#include "DNA_collection_types.h"
#include "DEG_depsgraph.h"
#include "DEG_depsgraph_build.h"
#include "outliner_intern.hh"
#include "WM_api.h"
#include "DNA_space_types.h"

#include "usd_collection.h"
#include "stage.h"
#include "utils.h"
#include "session.h"

using namespace std;
using namespace pxr;

namespace usdhydra {


void update(BL::Context b_context, int stageId) {

}

static PyObject *update_func(PyObject * /*self*/, PyObject *args)
{
  PyObject *pycontext, *pysession;

  if (!PyArg_ParseTuple(args, "OO", &pycontext, &pysession)) {
    Py_RETURN_NONE;
  }

  PointerRNA contextptr;
  RNA_pointer_create(NULL, &RNA_Context, (ID *)PyLong_AsVoidPtr(pycontext), &contextptr);
  BL::Context b_context(contextptr);

  const string COLLECTION_NAME = "USD NodeTree";

  BlenderSession *session = (BlenderSession *)PyLong_AsVoidPtr(pysession);

  if (!session->stage) {
    Py_RETURN_NONE;
  }

  bContext *C = (bContext *)b_context.ptr.data;
  
  BL::SpaceView3D space_data = (BL::SpaceView3D)b_context.space_data();
  View3D *b_v3d = (::View3D *) space_data.ptr.data;
  Main *bmain = CTX_data_main(C);
  Scene *bscene = CTX_data_scene(C);
  ScrArea *area = CTX_wm_area(C);
  ViewLayer *view_layer = CTX_data_view_layer(C);
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
  }

  //map<SdfPath, Object*> objects;
  map<Object*, SdfPath> objects;

  LISTBASE_FOREACH (CollectionObject *, coll_ob, &usd_collection->gobject) {
    Object *obj = coll_ob->ob;
    ID id = obj->id;
    IDProperty *idprop = IDP_GetProperties(&id, true);

    //// IDPropertyData idpropdata = idprop->data;
    //// eIDPropertyType proptype = (eIDPropertyType)idprop->type;
    //  
    //IDProperty *obj_prop_usd_sdf_path = IDP_NewString("", PROP_USD_SDF_PATH_NAME, PROP_USD_SDF_PATH_MAX_LEN);

    //if (IDP_AddToGroup(idprop, obj_prop_usd_sdf_path)) {
    //  DEG_id_tag_update(&id, ID_RECALC_ALL);
    //};

    IDProperty *obj_prop_usd_sdf_path = IDP_GetPropertyFromGroup(idprop, PROP_USD_SDF_PATH_NAME);

    //objects.insert(pair<SdfPath, Object*>(SdfPath((char *)obj_prop_usd_sdf_path->data.pointer), obj));
    objects.insert(pair<Object*, SdfPath>(obj, SdfPath((char *)obj_prop_usd_sdf_path->data.pointer)));
  }

  set<SdfPath> obj_paths, prim_paths, paths_to_remove, paths_to_add, path_to_update;
  //set<Object*> obj_paths, prim_paths, paths_to_remove, paths_to_add, path_to_update;

  for(map<Object*, SdfPath>::iterator it = objects.begin(); it != objects.end(); ++it) {
    //obj_paths.insert(it->first);
    obj_paths.insert(it->second);
  }

  for (UsdPrim prim : session->stage->TraverseAll()) {
    printf("%s %s\r\n", prim.GetPath().GetAsString().c_str(), prim.GetTypeName().GetString().c_str());
    if (!ignore_prim(prim))  {
      prim_paths.insert(prim.GetPath());
    }
  }

  set_difference(obj_paths.begin(), obj_paths.end(),
                  prim_paths.begin(), prim_paths.end(),
                  inserter(paths_to_remove, paths_to_remove.end()));

  set_difference(prim_paths.begin(), prim_paths.end(),
                  obj_paths.begin(), obj_paths.end(),
                  inserter(paths_to_add, paths_to_add.end()));

  set_intersection(obj_paths.begin(), obj_paths.end(),
                    prim_paths.begin(), prim_paths.end(),
                    inserter(path_to_update, path_to_update.begin()));

  /*for (SdfPath path : paths_to_remove) {
    Object *obj = objects.find(path)->second;
    objects.erase(path);
    BKE_collection_object_remove(bmain, usd_collection, obj, false);
    string as = path.GetAsString();
    int as1 = 123;
  }*/

  for (SdfPath path : path_to_update) {
    string as = path.GetAsString();
    int as2 = 123;
  }

  for (SdfPath path : paths_to_add) {
    SdfPath parent_path = path.GetParentPath();
    Object *parent_obj = nullptr;

    if (parent_path.GetAsString() != "/") {
      /*auto it = objects.find(parent_path);
      if (it != objects.end()) {
        parent_obj =  it->second;
      }*/
      for (auto it = objects.begin(); it != objects.end(); ++it)
        if (it->second == parent_path)
            parent_obj = it->first;
    }
    //Object *obj_from_scene = objects.find(path)->second;

    Object *obj_from_scene = nullptr;

    for (auto it = objects.begin(); it != objects.end(); ++it)
        if (it->second == path)
            obj_from_scene = it->first;

    UsdPrim obj_prim = session->stage->GetPrimAtPath(path);

    Object *obj = BKE_object_add_only_object(bmain, OB_EMPTY, obj_prim.GetName().GetString().c_str());

    add_usd_sdf_path_prop_to_object(obj, obj_prim.GetPath().GetText());

    obj->parent = parent_obj;
       
    if (SUPPORTED_PRIM_TYPES.count(obj_prim.GetTypeName()) == 0) {
      obj->visibility_flag |= 1;
    }

    BKE_collection_object_add(bmain, usd_collection, obj);
    /*if (!BKE_collection_has_object(usd_collection, obj)) {
      BKE_collection_object_add(bmain, usd_collection, obj);
    }*/
    //objects.insert(pair<SdfPath, Object*>(path, obj));
    objects.insert(pair<Object*, SdfPath>(obj, path));
  }

  //const char *PROP_USD_SDF_PATH_NAME = "USD SDF path";
  //const int PROP_USD_SDF_PATH_MAX_LEN = 255;

  //map<SdfPath, Object*> objects;

  //LISTBASE_FOREACH (CollectionObject *, coll_ob, &usd_collection->gobject) {
  //  Object *ob = coll_ob->ob;
  //  ID id = ob->id;

  //  IDProperty *idprop = id.properties;
  //  if (idprop) {
  //    IDPropertyData idpropdata = idprop->data;
  //    // eIDPropertyType proptype = (eIDPropertyType)idprop->type;
  //    
  //    IDProperty *obj_prop_usd_sdf_path = nullptr;

  //    LISTBASE_FOREACH (IDProperty *, prop, &idprop->data.group) {
  //      if (!strcmp(prop->name, PROP_USD_SDF_PATH_NAME)) {
  //        obj_prop_usd_sdf_path = prop;
  //        objects.insert(pair<SdfPath, Object*>(SdfPath((char *)prop->data.pointer), ob));
  //        break;
  //      }  
  //    }

  //    if (!obj_prop_usd_sdf_path) {
  //      obj_prop_usd_sdf_path = IDP_NewString("", PROP_USD_SDF_PATH_NAME, PROP_USD_SDF_PATH_MAX_LEN);
  //      idprop->len++;
  //      BLI_addtail(&idprop->data.group, obj_prop_usd_sdf_path);
  //    }

  //    
  //  }
  //  else {

  //  }
  //}

  //set<SdfPath> obj_paths, prim_paths, paths_to_remove, paths_to_add, path_to_update;

  //for(map<SdfPath, Object*>::iterator it = objects.begin(); it != objects.end(); ++it) {
  //  obj_paths.insert(it->first);
  //}

  //for (UsdPrim prim : session->stage->TraverseAll()) {
  //  if (!ignore_prim(prim)) {
  //    prim_paths.insert(prim.GetPath());
  //    // printf("%s %s\r\n", prim.GetPath().GetAsString().c_str(), prim.GetTypeName().GetString().c_str());
  //  }
  //}

  //set_difference(obj_paths.begin(), obj_paths.end(),
  //                prim_paths.begin(), prim_paths.end(),
  //                inserter(paths_to_remove, paths_to_remove.end()));

  //set_difference(prim_paths.begin(), prim_paths.end(),
  //                obj_paths.begin(), obj_paths.end(),
  //                inserter(paths_to_add, paths_to_add.end()));

  //set_intersection(obj_paths.begin(), obj_paths.end(),
  //                  prim_paths.begin(), prim_paths.end(),
  //                  inserter(path_to_update, path_to_update.begin()));

  //for (SdfPath path : paths_to_remove) {
  //  Object *obj = objects.find(path)->second;
  //  objects.erase(path);
  //  BKE_collection_object_remove(bmain, usd_collection, obj, false);
  //  string as = path.GetAsString();
  //  int as1 = 123;
  //}

  //for (SdfPath path : path_to_update) {
  //  string as = path.GetAsString();
  //  int as2 = 123;
  //}

  //for (SdfPath path : paths_to_add) {
  //  Object *obj = BKE_object_add_only_object(bmain, OB_EMPTY, path.GetAsString().c_str());
  //  if (!BKE_collection_has_object(usd_collection, obj)) {
  //    BKE_collection_object_add(bmain, usd_collection, obj);
  //  }
  //  string as = path.GetAsString();
  //  int as3 = 123;
  //}

  WM_main_add_notifier(NC_SCENE | ND_LAYER, nullptr);
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

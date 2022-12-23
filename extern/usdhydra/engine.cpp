/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#include <pxr/imaging/hd/rendererPluginRegistry.h>
#include <pxr/base/plug/plugin.h>
#include <pxr/base/plug/registry.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>

#include "glog/logging.h"

#include "engine.h"
#include "finalEngine.h"
#include "viewportEngine.h"

using namespace pxr;

namespace usdhydra {

Engine::Engine(BL::RenderEngine &b_engine, const std::string &delegateId)
  : b_engine(b_engine)
{
  HdRendererPluginRegistry& registry = HdRendererPluginRegistry::GetInstance();

  TF_PY_ALLOW_THREADS_IN_SCOPE();
  renderDelegate = registry.CreateRenderDelegate(TfToken(delegateId));
  renderIndex.reset(HdRenderIndex::New(renderDelegate.Get(), {}));
  freeCameraDelegate = std::make_unique<HdxFreeCameraSceneDelegate>(
    renderIndex.get(), SdfPath::AbsoluteRootPath().AppendElementString("freeCamera"));
  renderTaskDelegate = std::make_unique<RenderTaskDelegate>(
    renderIndex.get(), SdfPath::AbsoluteRootPath().AppendElementString("renderTask"));
}

Engine::~Engine()
{
  sceneDelegate = nullptr;
  renderTaskDelegate = nullptr;
  freeCameraDelegate = nullptr;
  renderIndex = nullptr;
  renderDelegate = nullptr;
}

float Engine::getRendererPercentDone()
{
  VtDictionary render_stats = renderDelegate->GetRenderStats();
  auto it = render_stats.find("percentDone");
  if (it == render_stats.end()) {
    return 0.0;
  }
  return (float)it->second.UncheckedGet<double>();
}

/* ------------------------------------------------------------------------- */
/* Python API for Engine
 */

static PyObject *create_func(PyObject * /*self*/, PyObject *args)
{
  LOG(INFO) << "create_func";

  PyObject *b_pyengine;
  char *engineType, *delegateId;
  if (!PyArg_ParseTuple(args, "Oss", &b_pyengine, &engineType, &delegateId)) {
    Py_RETURN_NONE;
  }

  PointerRNA b_engineptr;
  RNA_pointer_create(NULL, &RNA_RenderEngine, (void *)PyLong_AsVoidPtr(b_pyengine), &b_engineptr);
  BL::RenderEngine b_engine(b_engineptr);

  Engine *engine;
  if (std::string(engineType) == "VIEWPORT") {
    engine = new ViewportEngine(b_engine, delegateId);
  }
  else {
    engine = new FinalEngine(b_engine, delegateId);
  }

  return PyLong_FromVoidPtr(engine);
}

static PyObject *free_func(PyObject * /*self*/, PyObject *args)
{
  LOG(INFO) << "free_func";

  PyObject *pyengine;
  if (!PyArg_ParseTuple(args, "O", &pyengine)) {
    Py_RETURN_NONE;
  }

  delete (Engine *)PyLong_AsVoidPtr(pyengine);
  Py_RETURN_NONE;
}

static PyObject *sync_func(PyObject * /*self*/, PyObject *args)
{
  LOG(INFO) << "sync_func";

  PyObject *pyengine, *pydepsgraph, *pycontext, *pysettings;
  if (!PyArg_ParseTuple(args, "OOOO", &pyengine, &pydepsgraph, &pycontext, &pysettings)) {
    Py_RETURN_NONE;
  }

  Engine *engine = (Engine *)PyLong_AsVoidPtr(pyengine);

  PointerRNA depsgraphptr;
  RNA_pointer_create(NULL, &RNA_Depsgraph, (ID *)PyLong_AsVoidPtr(pydepsgraph), &depsgraphptr);
  BL::Depsgraph b_depsgraph(depsgraphptr);

  PointerRNA contextptr;
  RNA_pointer_create(NULL, &RNA_Context, (ID *)PyLong_AsVoidPtr(pycontext), &contextptr);
  BL::Context b_context(contextptr);

  HdRenderSettingsMap settings;
  PyObject *pyiter = PyObject_GetIter(pysettings);
  if (pyiter) {
    PyObject *pykey, *pyval;
    while (pykey = PyIter_Next(pyiter)) {
      TfToken key(PyUnicode_AsUTF8(pykey));
      pyval = PyDict_GetItem(pysettings, pykey);
      if (PyLong_Check(pyval)) {
        settings[key] = PyLong_AsLong(pyval);
      }
      else if (PyFloat_Check(pyval)) {
        settings[key] = PyFloat_AsDouble(pyval);
      }
      else if (PyUnicode_Check(pyval)) {
        settings[key] = PyUnicode_AsUTF8(pyval);
      }
      Py_DECREF(pykey);
    }
    Py_DECREF(pyiter);
  }

  engine->sync(b_depsgraph, b_context, settings);
  Py_RETURN_NONE;
}

static PyObject *render_func(PyObject * /*self*/, PyObject *args)
{
  LOG(INFO) << "render_func";
  
  PyObject *pyengine, *pydepsgraph;
  if (!PyArg_ParseTuple(args, "OO", &pyengine, &pydepsgraph)) {
    Py_RETURN_NONE;
  }

  FinalEngine *engine = (FinalEngine *)PyLong_AsVoidPtr(pyengine);

  PointerRNA depsgraphptr;
  RNA_pointer_create(NULL, &RNA_Depsgraph, (ID *)PyLong_AsVoidPtr(pydepsgraph), &depsgraphptr);
  BL::Depsgraph depsgraph(depsgraphptr);

  /* Allow Blender to execute other Python scripts. */
  Py_BEGIN_ALLOW_THREADS
    engine->render(depsgraph);
  Py_END_ALLOW_THREADS

  Py_RETURN_NONE;
}

static PyObject *view_draw_func(PyObject * /*self*/, PyObject *args)
{
  LOG(INFO) << "view_draw_func";

  PyObject *pyengine, *pydepsgraph, *pycontext;
  if (!PyArg_ParseTuple(args, "OOO", &pyengine, &pydepsgraph, &pycontext)) {
    Py_RETURN_NONE;
  }

  ViewportEngine *engine = (ViewportEngine *)PyLong_AsVoidPtr(pyengine);

  PointerRNA depsgraphptr;
  RNA_pointer_create(NULL, &RNA_Depsgraph, (ID *)PyLong_AsVoidPtr(pydepsgraph), &depsgraphptr);
  BL::Depsgraph b_depsgraph(depsgraphptr);

  PointerRNA contextptr;
  RNA_pointer_create(NULL, &RNA_Context, (ID *)PyLong_AsVoidPtr(pycontext), &contextptr);
  BL::Context b_context(contextptr);

  /* Allow Blender to execute other Python scripts. */
  Py_BEGIN_ALLOW_THREADS
    engine->viewDraw(b_depsgraph, b_context);
  Py_END_ALLOW_THREADS

  Py_RETURN_NONE;
}

static PyObject* get_render_plugins_func(PyObject* /*self*/, PyObject* args)
{
  LOG(INFO) << "get_render_plugins_func";

  PlugRegistry &registry = PlugRegistry::GetInstance();
  TfTokenVector pluginsIds = UsdImagingGLEngine::GetRendererPlugins();
  PyObject *ret = PyTuple_New(pluginsIds.size());
  for (int i = 0; i < pluginsIds.size(); ++i) {
    PyObject *descr = PyDict_New();
    PyDict_SetItemString(descr, "id", PyUnicode_FromString(pluginsIds[i].GetText()));
    PyDict_SetItemString(descr, "name", PyUnicode_FromString(UsdImagingGLEngine::GetRendererDisplayName(pluginsIds[i]).c_str()));

    std::string plugin_name = pluginsIds[i];
    plugin_name = plugin_name.substr(0, plugin_name.size()-6);
    plugin_name[0] = tolower(plugin_name[0]);
    std::string path = "";
    PlugPluginPtr plugin = registry.GetPluginWithName(plugin_name);
    if (plugin) {
        path = plugin->GetPath();
    }
    PyDict_SetItemString(descr, "path", PyUnicode_FromString(path.c_str()));

    PyTuple_SetItem(ret, i, descr);
  }
  return ret;
}

static PyMethodDef methods[] = {
  {"create", create_func, METH_VARARGS, ""},
  {"free", free_func, METH_VARARGS, ""},
  {"render", render_func, METH_VARARGS, ""},
  {"sync", sync_func, METH_VARARGS, ""},
  {"view_draw", view_draw_func, METH_VARARGS, ""},
  {"get_render_plugins", get_render_plugins_func, METH_VARARGS, ""},
  {NULL, NULL, 0, NULL},
};

static struct PyModuleDef module = {
  PyModuleDef_HEAD_INIT,
  "engine",
  "",
  -1,
  methods,
  NULL,
  NULL,
  NULL,
  NULL,
};

PyObject *addPythonSubmodule_engine(PyObject *mod)
{
  PyObject *submodule = PyModule_Create(&module);
  PyModule_AddObject(mod, "engine", submodule);
  return submodule;
}

}   // namespace usdhydra

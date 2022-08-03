/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#include <GL/glew.h>

#include <pxr/pxr.h>
#include <pxr/usd/usd/stage.h>
#include <pxr/base/gf/camera.h>
#include <pxr/imaging/glf/drawTarget.h>
#include <pxr/usd/usdGeom/camera.h>
#include <pxr/usdImaging/usdImagingGL/engine.h>
#include <pxr/usdImaging/usdImagingGL/renderParams.h>
#include <pxr/usdImaging/usdAppUtils/camera.h>

#include "glog/logging.h"

#include "session.h"

using namespace pxr;

namespace usdhydra {

BlenderSession::BlenderSession(BL::RenderEngine &b_engine)
    : b_engine(b_engine)
{
}

BlenderSession::~BlenderSession()
{
}

void BlenderSession::reset(BL::Context b_context, Depsgraph *depsgraph, bool is_blender_scene, int stageId)
{
  if (is_blender_scene) {
    stage = export_scene_to_usd(b_context, depsgraph);
  }
  else {
    stage = stageCache->Find(pxr::UsdStageCache::Id::FromLongInt(stageId));
  }
}

void BlenderSession::render(BL::Depsgraph &b_depsgraph)
{
  imagingGLEngine = std::make_unique<pxr::UsdImagingGLEngine>();

  if (!imagingGLEngine->SetRendererPlugin(TfToken("HdStormRendererPlugin"))) {
    return;
  }

  BL::Scene b_scene = b_depsgraph.scene_eval();
  BL::ViewLayer b_view_layer = b_depsgraph.view_layer();
  string b_render_layer_name = b_view_layer.name();
  vector<vector<float>> border ={{0.0, 0.0}, {1.0, 1.0}};

  if (b_scene.render().use_border()) {
    border = {
      {b_scene.render().border_min_x(),
       b_scene.render().border_min_y()},
      {b_scene.render().border_max_x() - b_scene.render().border_min_x(),
       b_scene.render().border_max_y() - b_scene.render().border_min_y()}
    };
  }

  int screen_width = int(b_scene.render().resolution_x() * b_scene.render().resolution_percentage() / 100);
  int screen_height = int(b_scene.render().resolution_y() * b_scene.render().resolution_percentage() / 100);

  int width = int(screen_width * border[1][0]);
  int height = int(screen_height * border[1][1]);

  pxr::GlfDrawTargetRefPtr draw_target_ptr = pxr::GlfDrawTarget::New(pxr::GfVec2i(width, height));

  draw_target_ptr->Bind();
  draw_target_ptr->AddAttachment("color", GL_RGBA, GL_FLOAT, GL_RGBA);

  pxr::UsdGeomCamera usd_camera = pxr::UsdAppUtilsGetCameraAtPath(stage, pxr::SdfPath(pxr::TfMakeValidIdentifier(b_scene.camera().data().name())));
  pxr::UsdTimeCode usd_timecode = pxr::UsdTimeCode(b_scene.frame_current());
  pxr::GfCamera gf_camera = usd_camera.GetCamera(usd_timecode);

  imagingGLEngine->SetCameraState(gf_camera.GetFrustum().ComputeViewMatrix(),
                                           gf_camera.GetFrustum().ComputeProjectionMatrix());

  imagingGLEngine->SetRenderViewport(pxr::GfVec4d(0, 0, width, height));
  imagingGLEngine->SetRendererAov(pxr::HdAovTokens->color);

  render_params.frame = usd_timecode;
  render_params.clearColor = pxr::GfVec4f(1.0, 1.0, 1.0, 0.0);

  imagingGLEngine->Render(stage->GetPseudoRoot(), render_params);

  BL::RenderResult b_result = b_engine.begin_result(0, 0, width, height, b_render_layer_name.c_str(), NULL);
  BL::CollectionRef b_render_passes = b_result.layers[0].passes;

  int channels = 4;
  vector<float> pixels(width * height * channels);

  glReadPixels(0, 0, width, height, GL_RGBA, GL_FLOAT, pixels.data());
  draw_target_ptr->Unbind();

  map<string, vector<float>> render_images{{"Combined", pixels}};
  vector<float> images;

  for (BL::RenderPass b_pass : b_render_passes) {
    map<string, vector<float>>::iterator it_image = render_images.find(b_pass.name());
    vector<float> image = it_image->second;

    if (it_image == render_images.end()) {
      image = vector<float>(width * height * channels);
    }

    if (b_pass.channels() != channels) {
      for (int i = image.size(); i >= b_pass.channels(); i -= b_pass.channels()) {
        image.erase(image.end() - i);
      }
    }

    images.insert(images.end(), image.begin(), image.end());
  }

  for (BL::RenderPass b_pass : b_render_passes) {
    b_pass.rect(images.data());
  }

  b_engine.end_result(b_result, false, false, false);
}

void BlenderSession::view_draw(BL::Depsgraph &b_depsgraph, BL::Context &b_context)
{
  BL::Scene b_scene = b_depsgraph.scene_eval();
  
  ViewSettings view_settings(b_context);

  if (view_settings.get_width() * view_settings.get_height() == 0) {
    return;
  };

  pxr::GfCamera gf_camera = view_settings.export_camera();

  vector<pxr::GfVec4f> clip_planes = gf_camera.GetClippingPlanes();

  for (int i = 0; i < clip_planes.size(); i++) {
    render_params.clipPlanes.push_back((pxr::GfVec4d)clip_planes[i]);
  }

  imagingGLEngine->SetCameraState(gf_camera.GetFrustum().ComputeViewMatrix(),
                                           gf_camera.GetFrustum().ComputeProjectionMatrix());
  imagingGLEngine->SetRenderViewport(pxr::GfVec4d((double)view_settings.border[0][0], (double)view_settings.border[0][1],
                                                  (double)view_settings.border[1][0], (double)view_settings.border[1][1]));

  b_engine.bind_display_space_shader(b_scene);

  imagingGLEngine->Render(stage->GetPseudoRoot(), render_params);

  b_engine.unbind_display_space_shader();
}

void BlenderSession::view_update(BL::Depsgraph &b_depsgraph, BL::Context &b_context)
{
  if (!imagingGLEngine) {
    imagingGLEngine = std::make_unique<pxr::UsdImagingGLEngine>();
    imagingGLEngine->SetRendererPlugin(TfToken("HdRprPlugin"));
  }

  if (imagingGLEngine->IsPauseRendererSupported()) {
    imagingGLEngine->PauseRenderer();
  }

  sync(b_depsgraph, b_context);

  if (imagingGLEngine->IsPauseRendererSupported()) {
    imagingGLEngine->ResumeRenderer();
  }
}

void BlenderSession::sync(BL::Depsgraph &b_depsgraph, BL::Context &b_context)
{
  BL::Scene b_scene = b_depsgraph.scene_eval();
  ViewSettings view_settings(b_context);

  render_params.frame = pxr::UsdTimeCode(b_scene.frame_current());  
}

pxr::UsdStageRefPtr BlenderSession::export_scene_to_usd(BL::Context b_context, Depsgraph *depsgraph)
{
  LOG(INFO) << "export_scene_to_usd";

  Scene *scene = DEG_get_input_scene(depsgraph);

  DEG_graph_build_for_all_objects(depsgraph);

  /* For restoring the current frame after exporting animation is done. */
  const int orig_frame = CFRA;

  string filepath = usdhydra::get_temp_file(".usda");
  pxr::UsdStageRefPtr usd_stage = pxr::UsdStage::CreateNew(filepath);

  usd_stage->SetMetadata(pxr::UsdGeomTokens->upAxis, pxr::VtValue(pxr::UsdGeomTokens->z));
  usd_stage->SetMetadata(pxr::UsdGeomTokens->metersPerUnit, static_cast<double>(scene->unit.scale_length));
  usd_stage->GetRootLayer()->SetDocumentation(std::string("Blender v") + BKE_blender_version_string());

  /* Set up the stage for animated data. */
  /*if (data->params.export_animation) {
    usd_stage->SetTimeCodesPerSecond(FPS);
    usd_stage->SetStartTimeCode(scene->r.sfra);
    usd_stage->SetEndTimeCode(scene->r.efra);
  }*/

  bContext *C = (bContext *)b_context.ptr.data;
  Main *bmain = CTX_data_main(C);
  USDExportParams usd_export_params;

  usd_export_params.selected_objects_only = false;
  usd_export_params.visible_objects_only = false;

  blender::io::usd::USDHierarchyIterator iter(bmain, depsgraph, usd_stage, usd_export_params);

  //if (data->params.export_animation) {
  //  /* Writing the animated frames is not 100% of the work, but it's our best guess. */
  //  float progress_per_frame = 1.0f / std::max(1, (scene->r.efra - scene->r.sfra + 1));

  //  for (float frame = scene->r.sfra; frame <= scene->r.efra; frame++) {
  //    if (G.is_break || (stop != nullptr && *stop)) {
  //      break;
  //    }

  //    /* Update the scene for the next frame to render. */
  //    scene->r.cfra = static_cast<int>(frame);
  //    scene->r.subframe = frame - scene->r.cfra;
  //    BKE_scene_graph_update_for_newframe(data->depsgraph);

  //    iter.set_export_frame(frame);
  //    iter.iterate_and_write();

  //    *progress += progress_per_frame;
  //    *do_update = true;
  //  }
  //}
  //else {
  //  /* If we're not animating, a single iteration over all objects is enough. */
  //  iter.iterate_and_write();
  //}

  iter.iterate_and_write();
  iter.release_writers();

  /* Finish up by going back to the keyframe that was current before we started. */
  if (CFRA != orig_frame) {
    CFRA = orig_frame;
    BKE_scene_graph_update_for_newframe(depsgraph);
  }

  return usd_stage;
}

/* ------------------------------------------------------------------------- */
/* Python API for BlenderSession
 */

static PyObject *create_func(PyObject * /*self*/, PyObject *args)
{
  DLOG(INFO) << "create_func";
  PyObject *pyengine;
  if (!PyArg_ParseTuple(args, "O", &pyengine)) {
    Py_RETURN_NONE;
  }

  PointerRNA engineptr;
  RNA_pointer_create(NULL, &RNA_RenderEngine, (void *)PyLong_AsVoidPtr(pyengine), &engineptr);
  BL::RenderEngine engine(engineptr);

  /* create session */
  BlenderSession *session = new BlenderSession(engine);

  return PyLong_FromVoidPtr(session);
}

static PyObject *free_func(PyObject * /*self*/, PyObject *args)
{
  LOG(INFO) << "free_func";
  PyObject *pysession;
  if (!PyArg_ParseTuple(args, "O", &pysession)) {
    Py_RETURN_NONE;
  }

  delete (BlenderSession *)PyLong_AsVoidPtr(pysession);
  Py_RETURN_NONE;
}

static PyObject *reset_func(PyObject * /*self*/, PyObject *args)
{
  LOG(INFO) << "reset_func";
  PyObject *pysession, *pydata, *pycontext, *pydepsgraph;

  int stageId = 0;
  int is_blender_scene = 1;

  if (!PyArg_ParseTuple(args, "OOOOii", &pysession, &pydata, &pycontext, &pydepsgraph, &is_blender_scene, &stageId)) {
    Py_RETURN_NONE;
  }

  PointerRNA depsgraphptr;
  RNA_pointer_create(NULL, &RNA_Context, (ID *)PyLong_AsVoidPtr(pydepsgraph), &depsgraphptr);
  Depsgraph *depsgraph = (::Depsgraph *)depsgraphptr.data;

  PointerRNA contextptr;
  RNA_pointer_create(NULL, &RNA_Context, (ID *)PyLong_AsVoidPtr(pycontext), &contextptr);
  BL::Context b_context(contextptr);

  BlenderSession *session = (BlenderSession *)PyLong_AsVoidPtr(pysession);

  //PointerRNA dataptr;
  //RNA_main_pointer_create((Main *)PyLong_AsVoidPtr(pydata), &dataptr);
  //BL::BlendData data(dataptr);

  //PointerRNA depsgraphptr;
  //RNA_pointer_create(NULL, &RNA_Depsgraph, (ID *)PyLong_AsVoidPtr(pydepsgraph), &depsgraphptr);
  //BL::Depsgraph depsgraph(depsgraphptr);

  session->reset(b_context, depsgraph, is_blender_scene, stageId);

  Py_RETURN_NONE;
}


static PyObject *render_func(PyObject * /*self*/, PyObject *args)
{
  LOG(INFO) << "render_func";
  PyObject *pysession, *pydepsgraph;

  if (!PyArg_ParseTuple(args, "OO", &pysession, &pydepsgraph)) {
    Py_RETURN_NONE;
  }

  PointerRNA depsgraphptr;
  RNA_pointer_create(NULL, &RNA_Depsgraph, (ID *)PyLong_AsVoidPtr(pydepsgraph), &depsgraphptr);
  BL::Depsgraph depsgraph(depsgraphptr);

  BlenderSession *session = (BlenderSession *)PyLong_AsVoidPtr(pysession);
  session->render(depsgraph);

  Py_RETURN_NONE;
}

static PyObject *render_frame_finish_func(PyObject * /*self*/, PyObject *args)
{
  Py_RETURN_NONE;
}

static PyObject *view_update_func(PyObject * /*self*/, PyObject *args)
{
  LOG(INFO) << "view_update_func";
  PyObject *pysession, *pydepsgraph, *pycontext, *pyspaceData, *pyregionData;

  if (!PyArg_ParseTuple(args, "OOOOO", &pysession, &pydepsgraph, &pycontext, &pyspaceData, &pyregionData)) {
    Py_RETURN_NONE;
  }

  PointerRNA depsgraphptr;
  RNA_pointer_create(NULL, &RNA_Depsgraph, (ID *)PyLong_AsVoidPtr(pydepsgraph), &depsgraphptr);
  BL::Depsgraph b_depsgraph(depsgraphptr);

  PointerRNA contextptr;
  RNA_pointer_create(NULL, &RNA_Context, (ID *)PyLong_AsVoidPtr(pycontext), &contextptr);
  BL::Context b_context(contextptr);

  BlenderSession *session = (BlenderSession *)PyLong_AsVoidPtr(pysession);

  session->view_update(b_depsgraph, b_context);

  Py_RETURN_NONE;
}

static PyObject *view_draw_func(PyObject * /*self*/, PyObject *args)
{
  LOG(INFO) << "view_draw_func";

  PyObject *pysession, *pydepsgraph, *pycontext, *pyspaceData, *pyregionData;

  if (!PyArg_ParseTuple(args, "OOOOO", &pysession, &pydepsgraph, &pycontext, &pyspaceData, &pyregionData)) {
    Py_RETURN_NONE;
  }


  PointerRNA depsgraphptr;
  RNA_pointer_create(NULL, &RNA_Depsgraph, (ID *)PyLong_AsVoidPtr(pydepsgraph), &depsgraphptr);
  BL::Depsgraph b_depsgraph(depsgraphptr);

  PointerRNA contextptr;
  RNA_pointer_create(NULL, &RNA_Context, (ID *)PyLong_AsVoidPtr(pycontext), &contextptr);
  BL::Context b_context(contextptr);

  BlenderSession *session = (BlenderSession *)PyLong_AsVoidPtr(pysession);
  session->view_draw(b_depsgraph, b_context);
  
  ///* Allow Blender to execute other Python scripts. */
  //python_thread_state_save(&session->python_thread_state);

  //session->render(b_depsgraph);

  //python_thread_state_restore(&session->python_thread_state);

  Py_RETURN_NONE;
}

static PyObject* get_render_plugins_func(PyObject* /*self*/, PyObject* args)
{
  TfTokenVector pluginsIds = UsdImagingGLEngine::GetRendererPlugins();
  PyObject *ret = PyTuple_New(pluginsIds.size());
  for (int i = 0; i < pluginsIds.size(); ++i) {
    PyObject *descr = PyTuple_New(2);
    PyTuple_SetItem(descr, 0, PyUnicode_FromString(pluginsIds[i].GetText()));
    PyTuple_SetItem(descr, 1, PyUnicode_FromString(UsdImagingGLEngine::GetRendererDisplayName(pluginsIds[i]).c_str()));

    PyTuple_SetItem(ret, i, descr);
  }
  return ret;
}


static PyMethodDef methods[] = {
  {"create", create_func, METH_VARARGS, ""},
  {"free", free_func, METH_VARARGS, ""},
  {"render", render_func, METH_VARARGS, ""},
  {"reset", reset_func, METH_VARARGS, ""},
  {"render_frame_finish", render_frame_finish_func, METH_VARARGS, ""},
  {"view_update", view_update_func, METH_VARARGS, ""},
  {"view_draw", view_draw_func, METH_VARARGS, ""},
  {"get_render_plugins", get_render_plugins_func, METH_VARARGS, ""},
  {NULL, NULL, 0, NULL},
};

static struct PyModuleDef module = {
  PyModuleDef_HEAD_INIT,
  "session",
  "",
  -1,
  methods,
  NULL,
  NULL,
  NULL,
  NULL,
};

PyObject *addPythonSubmodule_session(PyObject *mod)
{
  PyObject *submodule = PyModule_Create(&module);
  PyModule_AddObject(mod, "session", submodule);
  return submodule;
}

}   // namespace usdhydra

/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#include "scene.h"
#include "object.h"

using namespace pxr;

namespace usdhydra {

GfCamera SceneExport::gfCamera(BL::Object &b_cameraObj)
{
  BL::Camera &b_camera = (BL::Camera &)b_cameraObj.data();
  auto res = resolution();
  float ratio = (float)res.first / res.second;
  
  GfCamera gfCamera;
  gfCamera.SetClippingRange(GfRange1f(b_camera.clip_start(), b_camera.clip_end()));
  gfCamera.SetHorizontalAperture(b_camera.sensor_width());
  gfCamera.SetVerticalAperture(b_camera.sensor_width() / ratio);
  gfCamera.SetFocalLength(b_camera.lens());
  gfCamera.SetTransform(ObjectExport(b_cameraObj, b_depsgraph).transform());

  return gfCamera;
}

GfCamera SceneExport::gfCamera()
{
  BL::Object b_cameraObj = b_scene.camera();
  return gfCamera(b_cameraObj);
}

std::pair<int,int> SceneExport::resolution()
{
  BL::RenderSettings b_render = b_scene.render();

  float border_w = 1.0, border_h = 1.0;
  if (b_render.use_border()) {
    border_w = b_render.border_max_x() - b_render.border_min_x();
    border_h = b_render.border_max_y() - b_render.border_min_y();
  }

  return std::make_pair<int, int>(
    int(b_render.resolution_x() * border_w * b_render.resolution_percentage() / 100),
    int(b_render.resolution_y() * border_h * b_render.resolution_percentage() / 100));
}

std::string SceneExport::sceneName()
{
  return b_scene.name();
}

std::string SceneExport::layerName()
{
  return b_depsgraph.view_layer().name();
}

} // namespace usdhydra

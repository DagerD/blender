/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#pragma once

#include <map>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/usd/sdf/assetPath.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/base/vt/value.h>
#include "pxr/base/tf/staticTokens.h"

#include "DNA_view3d_types.h"
#include "DNA_world_types.h"

namespace blender::render::hydra {

class WorldData {
public:
  WorldData();
  WorldData(View3DShading *shading, World *world);

  std::string name();
  int type();
  pxr::TfToken prim_type();
  pxr::GfMatrix4d transform();

  pxr::VtValue &get_data(pxr::TfToken const &key);
  template<class T>
  const T &get_data(pxr::TfToken const &key);
  bool has_data(pxr::TfToken const &key);
  void update_world();

  View3DShading *shading;
  World *world;

 private:
  
  std::map<pxr::TfToken, pxr::VtValue> data;

  void set_as_world();
  void set_as_shading();
  std::string get_image_filepath(const bNode *tex_node);
};

template<class T>
const T &WorldData::get_data(pxr::TfToken const &key)
{
  return get_data(key).Get<T>();
}

} // namespace blender::render::hydra

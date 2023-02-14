/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#pragma once

#include <map>

#include <pxr/base/gf/matrix4d.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/base/vt/value.h>
#include "pxr/base/tf/staticTokens.h"

#include "DNA_object_types.h"

#include "material.h"

PXR_NAMESPACE_OPEN_SCOPE
#define HD_BLENDER_TOKENS \
  (materialId) \
  (faceCounts) \
  (empty)

TF_DECLARE_PUBLIC_TOKENS(HdBlenderTokens, HD_BLENDER_TOKENS);
PXR_NAMESPACE_CLOSE_SCOPE

namespace blender::render::hydra {


class ObjectData {
public:
  ObjectData();
  ObjectData(Object *object);

  std::string name();
  int type();
  pxr::TfToken prim_type();
  pxr::GfMatrix4d transform();
  Material *material();

  pxr::VtValue &get_data(pxr::TfToken const &key);
  template<class T>
  const T &get_data(pxr::TfToken const &key);
  bool has_data(pxr::TfToken const &key);

  void set_material_id(pxr::SdfPath const &id);
  bool update_visibility(View3D *view3d);
  bool is_visible();

 private:
  Object *object;
  std::map<pxr::TfToken, pxr::VtValue> data;
  bool visible;

  void set_as_mesh();
  void set_as_meshable();
  void set_mesh(Mesh *mesh);
  void set_as_light();
};

using ObjectDataMap = std::map<pxr::SdfPath, ObjectData>;

template<class T>
const T &ObjectData::get_data(pxr::TfToken const &key)
{
  return get_data(key).Get<T>();
}

} // namespace blender::render::hydra

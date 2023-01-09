/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#include <pxr/base/gf/vec3f.h>

#include "light.h"

using namespace pxr;

namespace usdhydra {

LightExport::LightExport(BL::Light &b_light)
  : light((Light *)b_light.ptr.data)
{
}

VtValue LightExport::energy() {
  return VtValue(light->energy);
}

VtValue LightExport::spotsize() {
  return VtValue(light->spotsize);
}

VtValue LightExport::color() {
  return VtValue(pxr::GfVec3f(light->r, light->g, light->b));
}

} // namespace usdhydra

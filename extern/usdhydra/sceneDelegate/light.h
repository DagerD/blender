/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#pragma once

#include <map>

#include <pxr/base/vt/value.h>
#include <pxr/imaging/hd/tokens.h>

#include "DNA_light_types.h"

#include "MEM_guardedalloc.h"
#include "RNA_blender_cpp.h"

namespace usdhydra {

class LightExport
{
public:
  LightExport(BL::Light &b_light);
  pxr::VtValue intensity();
  pxr::VtValue width();
  pxr::VtValue height();
  pxr::VtValue radius();
  pxr::VtValue color();
  pxr::VtValue angle();

private:
  Light *light;
};

pxr::TfToken GetLightType(Light *light);

inline std::map<int, pxr::TfToken> light_types {    
    { LA_SUN, pxr::HdPrimTypeTokens->distantLight},
    { LA_LOCAL, pxr::HdPrimTypeTokens->sphereLight},
    { LA_SPOT, pxr::HdPrimTypeTokens->sphereLight},
};

inline std::map<int, pxr::TfToken> light_shape_types {    
    { LA_AREA_SQUARE, pxr::HdPrimTypeTokens->rectLight},
    { LA_AREA_RECT, pxr::HdPrimTypeTokens->rectLight},
    { LA_AREA_DISK, pxr::HdPrimTypeTokens->diskLight},
    { LA_AREA_ELLIPSE, pxr::HdPrimTypeTokens->diskLight}
};

} // namespace usdhydra

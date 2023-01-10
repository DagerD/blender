/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#pragma once

#include <map>

#include <pxr/pxr.h>
#include <pxr/imaging/hd/camera.h>
#include <pxr/imaging/hd/sceneDelegate.h>
#include <pxr/imaging/hd/renderIndex.h>
#include <pxr/usd/usd/stage.h>

#include "MEM_guardedalloc.h"
#include "RNA_blender_cpp.h"

#include "object.h"

using namespace pxr;

namespace usdhydra {

class BlenderSceneDelegate : public HdSceneDelegate {
public:
  BlenderSceneDelegate(HdRenderIndex* renderIndex, SdfPath const &delegateId, BL::Depsgraph &b_depsgraph);
  ~BlenderSceneDelegate() override = default;

  void Populate();

  // delegate methods
  HdMeshTopology GetMeshTopology(SdfPath const& id) override;
  GfMatrix4d GetTransform(SdfPath const& id) override;
  VtValue Get(SdfPath const& id, TfToken const& key) override;
  VtValue GetCameraParamValue(SdfPath const& id, TfToken const& key) override;
  VtValue GetLightParamValue(SdfPath const& id, TfToken const& key) override;
  HdPrimvarDescriptorVector GetPrimvarDescriptors(SdfPath const& id, HdInterpolation interpolation) override;

private:
  BL::Depsgraph b_depsgraph;
  bool isPopulated;

  std::unique_ptr<ObjectExport> objectExport(SdfPath const& id);

  std::map<SdfPath, std::string> objects;
};

} // namespace usdhydra

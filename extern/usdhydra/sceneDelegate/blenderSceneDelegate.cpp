/* SPDX-License-Identifier: Apache-2.0
 * Copyright 2011-2022 Blender Foundation */

#include <pxr/imaging/hd/light.h>

#include "glog/logging.h"

#include "blenderSceneDelegate.h"
#include "object.h"

namespace usdhydra {

BlenderSceneDelegate::BlenderSceneDelegate(HdRenderIndex* parentIndex, SdfPath const& delegateID, BL::Depsgraph &b_depsgraph)
  : HdSceneDelegate(parentIndex, delegateID)
  , b_depsgraph(b_depsgraph)
  , _isPopulated(false)
{
}

std::unique_ptr<ObjectExport> BlenderSceneDelegate::objectExport(SdfPath const & id)
{
  std::string name = id.GetName();
  for (BL::Object &obj : b_depsgraph.objects) {
    if (obj.name() == name) {
      return std::make_unique<ObjectExport>(obj, b_depsgraph);
    }
  }
  return nullptr;
}

void BlenderSceneDelegate::Populate()
{
  LOG(INFO) << "Populate " << _isPopulated;

  if (_isPopulated) {
    // TODO: check depsgraph updates
    
      //GetRenderIndex().GetChangeTracker().MarkRprimDirty(objId, 
      //  HdChangeTracker::DirtyPoints | HdChangeTracker::DirtyNormals | HdChangeTracker::AllDirty);
    return;
  }

  auto &instances = b_depsgraph.object_instances;
  for (auto& inst : instances) {
    if (inst.is_instance()) {
      continue;
    }
    
    BL::Object obj = inst.object();
    SdfPath objId = GetDelegateID().AppendElementString(obj.name_full());
    
    if (obj.type() == BL::Object::type_MESH) {
      GetRenderIndex().InsertRprim(HdPrimTypeTokens->mesh, this, objId);
      continue;
    }
    if (obj.type() == BL::Object::type_LIGHT) {
      Light * light = (Light *)((BL::Light &)obj.data()).ptr.data;
      TfToken light_type;

      if (light->type == LA_AREA) {
        if (auto it = light_shape_types.find(light->area_shape); it != light_shape_types.end())
          light_type = it->second;
      }
      else {
        if (auto it = light_types.find(light->type); it != light_types.end())
          light_type = it->second;
      }
      if (light_type.IsEmpty()) {
        LOG(WARNING) << "Unsupported light type: " << light->id.name + 2;
        continue;
      }
      
      GetRenderIndex().InsertSprim(light_type, this, objId);
      continue;
    }
  }
  _isPopulated = true;
}

HdMeshTopology BlenderSceneDelegate::GetMeshTopology(SdfPath const& id)
{
  LOG(INFO) << "GetMeshTopology: " << id.GetAsString();
  MeshExport meshExport = objectExport(id)->meshExport();
  return HdMeshTopology(PxOsdOpenSubdivTokens->catmullClark, HdTokens->rightHanded,
                        meshExport.faceVertexCounts(), meshExport.faceVertexIndices());
}

VtValue BlenderSceneDelegate::Get(SdfPath const& id, TfToken const& key)
{
  LOG(INFO) << "Get: " << id.GetAsString() << " [" << key.GetString() << "]";
  if (key == HdTokens->points) {
    VtVec3fArray points = objectExport(id)->meshExport().vertices();
    return VtValue(points);
  }
  if (key == HdTokens->normals) {
    VtVec3fArray normals = objectExport(id)->meshExport().normals();
    return VtValue(normals);
  }
  return VtValue();
}

HdPrimvarDescriptorVector BlenderSceneDelegate::GetPrimvarDescriptors(SdfPath const& id, HdInterpolation interpolation)
{
  LOG(INFO) << "GetPrimvarDescriptors: " << id.GetAsString() << " " << interpolation;
  HdPrimvarDescriptorVector primvars;
  if (interpolation == HdInterpolationVertex) {
    primvars.emplace_back(HdTokens->points, interpolation, HdPrimvarRoleTokens->point);
  }
  if (interpolation == HdInterpolationFaceVarying) {
    primvars.emplace_back(HdTokens->normals, interpolation, HdPrimvarRoleTokens->normal);
  }
  return primvars;
}

GfMatrix4d BlenderSceneDelegate::GetTransform(SdfPath const& id)
{
  LOG(INFO) << "GetTransform: " << id.GetAsString();
  return objectExport(id)->transform();
}

VtValue BlenderSceneDelegate::GetCameraParamValue(SdfPath const& id, TfToken const& key)
{
  LOG(INFO) << "GetCameraParamValue: " << id.GetAsString() << " [" << key.GetString() << "]";
  return VtValue();
}

VtValue BlenderSceneDelegate::GetLightParamValue(SdfPath const& id, TfToken const& key)
{
  LOG(INFO) << "GetLightParamValue: " << id.GetAsString() << " [" << key.GetString() << "]";

  if (key == HdLightTokens->intensity) {
    return objectExport(id)->lightExport().energy();
  }

  if (key == HdLightTokens->radius) {
    return objectExport(id)->lightExport().spotsize();
  }
  
  if (key == HdLightTokens->color) {
    return objectExport(id)->lightExport().color();
  }

  return VtValue();
}

} // namespace usdhydra
